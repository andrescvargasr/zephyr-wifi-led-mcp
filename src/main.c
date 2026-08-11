/**
 * Copyright (c) 2026 Andres Camilo Román Vargas
 *
 * @file main.c
 * @author Andres C. Román V. (camilo.vargas@technaid.com gh: @andrescvargasr)
 * @brief
 * @version 1.1
 * @date 2026-07-24
 *
 * Main application entry point for MCP, LED, and Wi-Fi interfaces.
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>

#if defined(CONFIG_WIFI_CREDENTIALS)
#include <zephyr/net/wifi_credentials.h>
#endif

#if defined(CONFIG_ESP_SPIRAM)
#include <esp_psram.h>
#include <soc/soc_memory_layout.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#endif

// MCP Server
#include "mcp_server.h"

// HTTP Server
#include <stdio.h>
#include <inttypes.h>

#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/util.h>
#include <zephyr/data/json.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/net/net_config.h>
#include "web_assets.h"

// mDNS
#include "mdns_service.h"

#include "params.h"
#include <zephyr/zbus/zbus.h>
#include "led_zbus.h"

// Include external utils
#include "net_sample_common.h"

LOG_MODULE_REGISTER(wifi_test, LOG_LEVEL_INF);

// HTTP Server

struct led_command {
    uint32_t r;
    uint32_t g;
    uint32_t b;
	uint32_t index;
    bool rainbow;
};

static const struct json_obj_descr led_command_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct led_command, r, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct led_command, g, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct led_command, b, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct led_command, index, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct led_command, rainbow, JSON_TOK_TRUE),
};

/* Handlers to return files with correct Content-Type and Gzip headers */

// index.html
static struct http_resource_detail_static index_html_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "text/html",
		},
	.static_data = index_html_gz,
	// .static_data_len = sizeof(index_html_gz),
	.static_data_len = index_html_gz_len,
};

// app.js
static struct http_resource_detail_static app_js_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "text/javascript",
		},
	.static_data = app_js_gz,
	.static_data_len = app_js_gz_len,
	// .static_data_len = sizeof(app_js_gz),
};

// style.css
static struct http_resource_detail_static style_css_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "text/css",
		},
	.static_data = style_css_gz,
	.static_data_len = style_css_gz_len,
};

// ping
static const char ping_response[] = "pong";

static struct http_resource_detail_static ping_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_HEAD) | BIT(HTTP_POST),
			.content_type = "text/plain",
		},
	.static_data = ping_response,
	.static_data_len = sizeof(ping_response) - 1,
};


static void parse_led_post(uint8_t *buf, size_t len)
{
	int ret;
	struct led_command cmd;
	const int expected_return_code = BIT_MASK(ARRAY_SIZE(led_command_descr));

	ret = json_obj_parse(buf, len, led_command_descr, ARRAY_SIZE(led_command_descr), &cmd);
	if (ret != expected_return_code) {
		LOG_WRN("Failed to fully parse JSON payload, ret=%d", ret);
		return;
	}

	LOG_DBG("POST request setting LED to state r: %d, g: %d, b: %d, index: %d, rainbow: %s", cmd.r, cmd.g, cmd.b, cmd.index, cmd.rainbow ? "true" : "false");

	// If r,g,b is greater than CONFIG_WHITE_LED_BRIGHTNESS to built white color, clamp it
	uint8_t brightness = CONFIG_WHITE_LED_BRIGHTNESS;
	if (cmd.r > brightness && cmd.g > brightness && cmd.b > brightness) {
		cmd.r = brightness;
		cmd.g = brightness;
		cmd.b = brightness;
	}

	struct led_msg msg = {
		.action = LED_ACTION_CUSTOM,
		.r = (uint8_t)CLAMP(cmd.r, 0, 255),
		.g = (uint8_t)CLAMP(cmd.g, 0, 255),
		.b = (uint8_t)CLAMP(cmd.b, 0, 255),
		.index = (uint16_t)CLAMP(cmd.index, 0, CONFIG_LED_MATRIX_PIXELS),
		.rainbow = cmd.rainbow
	};

	int pub_rc = zbus_chan_pub(&led_chan, &msg, K_MSEC(200));
	if (pub_rc != 0) {
		LOG_ERR("Failed to publish LED command to Zbus: %d", pub_rc);
	}
}

static int led_handler(struct http_client_ctx *client, enum http_transaction_status status,
		       const struct http_request_ctx *request_ctx,
		       struct http_response_ctx *response_ctx, void *user_data)
{
	static uint8_t post_payload_buf[64];
	static size_t cursor;

	LOG_DBG("LED handler status %d, size %zu", status, request_ctx->data_len);

	if (status == HTTP_SERVER_TRANSACTION_ABORTED ||
	    status == HTTP_SERVER_TRANSACTION_COMPLETE) {
		cursor = 0;
		return 0;
	}

	if (client->method == HTTP_GET) {
		LOG_INF("GET request on LED resource");
		return 0;
	}

	if (request_ctx->data_len + cursor > sizeof(post_payload_buf)) {
		cursor = 0;
		return -ENOMEM;
	}

	/* Copy payload to our buffer. Note that even for a small payload, it may arrive split into
	 * chunks (e.g. if the header size was such that the whole HTTP request exceeds the size of
	 * the client buffer).
	 */
	memcpy(post_payload_buf + cursor, request_ctx->data, request_ctx->data_len);
	cursor += request_ctx->data_len;

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		parse_led_post(post_payload_buf, cursor);
		cursor = 0;
	}

	return 0;
}

// LED POST & GET
static struct http_resource_detail_dynamic led_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
		},
	.cb = led_handler,
	.user_data = NULL,
};

/* Define HTTP Service */
static uint16_t led_http_service_port = CONFIG_NET_SAMPLE_HTTP_SERVER_SERVICE_PORT;
HTTP_SERVICE_DEFINE(led_http_service, NULL, &led_http_service_port,
		    CONFIG_HTTP_SERVER_MAX_CLIENTS, 10, NULL, NULL, NULL);
/* Register static endpoints */
HTTP_RESOURCE_DEFINE(index_html_gz_resource, led_http_service, "/",
		     &index_html_gz_resource_detail);
HTTP_RESOURCE_DEFINE(app_js_gz_resource, led_http_service, "/app.js",
		     &app_js_gz_resource_detail);
HTTP_RESOURCE_DEFINE(style_css_gz_resource, led_http_service, "/style.css",
		     &style_css_gz_resource_detail);

/* Register API Endpoint */
HTTP_RESOURCE_DEFINE(res_api_led, led_http_service, "/api/led", &led_resource_detail);
HTTP_RESOURCE_DEFINE(res_ping, led_http_service, "/api/ping", &ping_resource_detail);

/**
 * @brief Auto-connect to Wi-Fi using credentials saved in flash memory.
 *
 * @return 0 on success, or negative error code on failure.
 */
int auto_connect(void)
{
	struct net_if *iface = net_if_get_wifi_sta();

	if (!iface) {
		LOG_ERR("No Wi-Fi STA interface available for auto-connect");
		return -ENODEV;
	}

#if defined(CONFIG_WIFI_CREDENTIALS)
	if (wifi_credentials_is_empty()) {
		LOG_INF("No stored Wi-Fi credentials found in flash memory");
		return -ENOENT;
	}
#endif

	LOG_INF("Attempting auto-connect with stored Wi-Fi credentials...");
	int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);

	if (ret != 0) {
		LOG_ERR("Auto-connect request failed (err: %d)", ret);
	} else {
		LOG_INF("Auto-connect request submitted successfully");
	}

	return ret;
}

int main(void)
{
#if defined(CONFIG_ESP_SPIRAM)
	if (esp_psram_is_initialized()) {
		printk("PSRAM size: %zu bytes (%zu MB)\n",
		       esp_psram_get_size(),
		       esp_psram_get_size() / (1024 * 1024));
	} else {
		printk("PSRAM is not initialized\n");
	}

#elif DT_NODE_EXISTS(DT_NODELABEL(psram0))
	printk("PSRAM node size (not used): %u bytes (%u MB)\n",
	       DT_PROP(DT_NODELABEL(psram0), size),
	       DT_PROP(DT_NODELABEL(psram0), size) / (1024 * 1024));
#else
	LOG_WRN("PSRAM not available");
#endif

	LOG_INF("Starting Wi-Fi MCP LED Server application...");

	/* Trigger auto-connect on startup */
	auto_connect();

	LOG_INF("Waiting for Wi-Fi connection and IP address");
	wait_for_network();	// From Zephyr net/common in CMakeLists.txt

	LOG_INF("Wi-Fi connected! Set MCP server");
	mcp_server();

	LOG_INF("Set HTTP server");
	http_server_start();
	
	LOG_INF("Set mDNS service");
	mdns_service();

	return 0;
}
