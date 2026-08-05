/**
 * Copyright (c) 2026 Andrés Camilo Román Vargas
 *
 * @file mcp_server.c
 * @author Andres C. Román V. (camilo.vargas@technaid.com gh: @andrescvargasr)
 * @brief
 * @version 1.0
 * @date 2026-07-24
 *
 * MCP Server implementation.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mcp/mcp_server.h>
#include <zephyr/net/mcp/mcp_server_http.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/data/json.h>
#include <zephyr/sys/util.h>

#include "led_zbus.h"

LOG_MODULE_REGISTER(mcp_app_server, LOG_LEVEL_INF);

#define DELAYED_RESPONSE_TEXT "Hello from the delayed response tool!"

__attribute__ ((section (".ext_ram.bss"))) mcp_server_ctx_t server;

__attribute__ ((section (".ext_ram.bss"))) static bool led_initialized;

struct mcp_led_args {
	int32_t r;
	int32_t g;
	int32_t b;
	bool rainbow;
	const char *action;
	const char *color;
};

static const struct json_obj_descr mcp_led_args_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct mcp_led_args, r, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct mcp_led_args, g, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct mcp_led_args, b, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct mcp_led_args, rainbow, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct mcp_led_args, action, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct mcp_led_args, color, JSON_TOK_STRING),
};

static bool parse_rgb_string(const char *str, uint8_t *r, uint8_t *g, uint8_t *b)
{
	int cr, cg, cb;

	if (str == NULL) {
		return false;
	}

	if (sscanf(str, "rgb(%d,%d,%d)", &cr, &cg, &cb) == 3 ||
	    sscanf(str, "rgb(%d, %d, %d)", &cr, &cg, &cb) == 3 ||
	    sscanf(str, "%d,%d,%d", &cr, &cg, &cb) == 3 ||
	    sscanf(str, "%d, %d, %d", &cr, &cg, &cb) == 3) {
		*r = (uint8_t)CLAMP(cr, 0, 255);
		*g = (uint8_t)CLAMP(cg, 0, 255);
		*b = (uint8_t)CLAMP(cb, 0, 255);
		return true;
	}

	return false;
}

/* Tool helper function */
static const char *extract_json_string_value(const char *json, const char *key)
{
	static char value_buffer[32];
	const char *key_pos;
	const char *value_start;
	const char *value_end;
	size_t key_len = strlen(key);
	size_t value_len;

	if ((json == NULL) || (key == NULL)) {
		return NULL;
	}

	key_pos = strstr(json, key);
	if (key_pos == NULL) {
		printk("didn't find key in json\n");
		return NULL;
	}

	value_start = strchr(key_pos + key_len, ':');
	if (value_start == NULL) {
		printk("didn't find : where expected\n");
		return NULL;
	}
	value_start++;

	while ((*value_start == ' ') || (*value_start == '\t') || (*value_start == '\n')) {
		value_start++;
	}

	if (*value_start != '"') {
		printk("didn't find expected value start\n");
		return NULL;
	}
	value_start++;

	value_end = strchr(value_start, '"');
	if (value_end == NULL) {
		printk("didn't find expected value end\n");
		return NULL;
	}

	value_len = value_end - value_start;
	if (value_len >= sizeof(value_buffer)) {
		printk("value len > buffer size\n");
		return NULL;
	}

	memcpy(value_buffer, value_start, value_len);
	value_buffer[value_len] = '\0';

	return value_buffer;
}

/**
 * @brief LED tool callback that serves as a simple reference for tool definitions
 *
 * @param event MCP tool event type, either a call request or a cancel request
 * @param arguments Arguments sent by the client requesting this call request
 * @param execution_token Token that identifies the execution
 *
 * @return 0 on success, negative errno on failure
 */
static int led_control_tool_callback(enum mcp_tool_event_type event,
					const char *arguments, const char *execution_token)
{
	const char *action;
	char response_buffer[96];
	struct mcp_tool_message response;
	int ret = 0;

	if (event == MCP_TOOL_CANCEL_REQUEST) {
		struct mcp_tool_message cancel_ack = {
			.type = MCP_USR_TOOL_CANCEL_ACK,
			.data = NULL,
			.length = 0
		};

		mcp_server_submit_tool_message(server, &cancel_ack, execution_token);

		/* Handle cancellation */

		return 0;
	}

	struct led_ready_msg ready_msg = {0};
	if (zbus_chan_read(&led_ready_chan, &ready_msg, K_NO_WAIT) == 0) {
		led_initialized = ready_msg.is_ready;
	}

	if (!led_initialized) {
		struct mcp_tool_message error_response = {
			.type = MCP_USR_TOOL_RESPONSE,
			.data = "LED not initialized",
			.length = strlen("LED not initialized")
		};
		mcp_server_submit_tool_message(server, &error_response, execution_token);
		return -ENODEV;
	}

	struct mcp_led_args args = {0};
	int parsed_bits = 0;

	if (arguments != NULL) {
		parsed_bits = json_obj_parse((char *)arguments, strlen(arguments),
					     mcp_led_args_descr,
					     ARRAY_SIZE(mcp_led_args_descr), &args);
	}

	bool valid_action = true;
	struct led_msg msg = {
		.action = LED_ACTION_CUSTOM,
		.r = 0,
		.g = 0,
		.b = 0,
		.rainbow = false
	};
	uint8_t str_r = 0, str_g = 0, str_b = 0;

	if ((parsed_bits & BIT(3)) && args.rainbow) {
		msg.action = LED_ACTION_CUSTOM;
		msg.rainbow = true;
		snprintk(response_buffer, sizeof(response_buffer),
			 "LED set to rainbow mode via Zbus");
	} else if (parse_rgb_string(args.color, &str_r, &str_g, &str_b)) {
		msg.action = LED_ACTION_CUSTOM;
		msg.r = str_r;
		msg.g = str_g;
		msg.b = str_b;
		msg.rainbow = false;
		snprintk(response_buffer, sizeof(response_buffer),
			 "LED set to RGB(%d, %d, %d) via Zbus", msg.r, msg.g, msg.b);
	} else if (parsed_bits & (BIT(0) | BIT(1) | BIT(2))) {
		msg.action = LED_ACTION_CUSTOM;
		msg.r = (uint8_t)CLAMP(args.r, 0, 255);
		msg.g = (uint8_t)CLAMP(args.g, 0, 255);
		msg.b = (uint8_t)CLAMP(args.b, 0, 255);
		msg.rainbow = args.rainbow;
		if (msg.rainbow) {
			snprintk(response_buffer, sizeof(response_buffer),
				 "LED set to rainbow mode via Zbus");
		} else {
			snprintk(response_buffer, sizeof(response_buffer),
				 "LED set to RGB(%d, %d, %d) via Zbus", msg.r, msg.g, msg.b);
		}
	} else if (args.action != NULL) {
		if (strcmp(args.action, "on") == 0) {
			msg.action = LED_ACTION_ON;
			snprintk(response_buffer, sizeof(response_buffer), "LED turned ON via Zbus");
		} else if (strcmp(args.action, "off") == 0) {
			msg.action = LED_ACTION_OFF;
			snprintk(response_buffer, sizeof(response_buffer), "LED turned OFF via Zbus");
		} else if (strcmp(args.action, "toggle") == 0) {
			msg.action = LED_ACTION_TOGGLE;
			snprintk(response_buffer, sizeof(response_buffer), "LED toggled via Zbus");
		} else if (strcmp(args.action, "red") == 0) {
			msg.action = LED_ACTION_RED;
			snprintk(response_buffer, sizeof(response_buffer), "LED turned red via Zbus");
		} else if (strcmp(args.action, "green") == 0) {
			msg.action = LED_ACTION_GREEN;
			snprintk(response_buffer, sizeof(response_buffer), "LED turned green via Zbus");
		} else if (strcmp(args.action, "blue") == 0) {
			msg.action = LED_ACTION_BLUE;
			snprintk(response_buffer, sizeof(response_buffer), "LED turned blue via Zbus");
		} else if (strcmp(args.action, "rainbow") == 0) {
			msg.action = LED_ACTION_CUSTOM;
			msg.rainbow = true;
			snprintk(response_buffer, sizeof(response_buffer), "LED set to rainbow mode via Zbus");
		} else {
			valid_action = false;
			snprintk(response_buffer, sizeof(response_buffer),
				 "Invalid command. Use r, g, b numbers, 'rgb(r,g,b)', rainbow, or action");
		}
	} else {
		action = extract_json_string_value(arguments, "\"action\"");
		if (action != NULL) {
			if (strcmp(action, "on") == 0) {
				msg.action = LED_ACTION_ON;
				snprintk(response_buffer, sizeof(response_buffer), "LED turned ON via Zbus");
			} else if (strcmp(action, "off") == 0) {
				msg.action = LED_ACTION_OFF;
				snprintk(response_buffer, sizeof(response_buffer), "LED turned OFF via Zbus");
			} else if (strcmp(action, "toggle") == 0) {
				msg.action = LED_ACTION_TOGGLE;
				snprintk(response_buffer, sizeof(response_buffer), "LED toggled via Zbus");
			} else if (strcmp(action, "red") == 0) {
				msg.action = LED_ACTION_RED;
				snprintk(response_buffer, sizeof(response_buffer), "LED turned red via Zbus");
			} else if (strcmp(action, "green") == 0) {
				msg.action = LED_ACTION_GREEN;
				snprintk(response_buffer, sizeof(response_buffer), "LED turned green via Zbus");
			} else if (strcmp(action, "blue") == 0) {
				msg.action = LED_ACTION_BLUE;
				snprintk(response_buffer, sizeof(response_buffer), "LED turned blue via Zbus");
			} else {
				valid_action = false;
				snprintk(response_buffer, sizeof(response_buffer),
					 "Invalid command. Use r, g, b numbers, 'rgb(r,g,b)', rainbow, or action");
			}
		} else {
			valid_action = false;
			snprintk(response_buffer, sizeof(response_buffer),
				 "Invalid command. Use r, g, b numbers, 'rgb(r,g,b)', rainbow, or action");
		}
	}

	if (valid_action) {
		int pub_rc = zbus_chan_pub(&led_chan, &msg, K_MSEC(200));
		if (pub_rc != 0) {
			snprintk(response_buffer, sizeof(response_buffer),
				 "Zbus publish failed: %d", pub_rc);
		}
	}

	response = (struct mcp_tool_message){
		.type = MCP_USR_TOOL_RESPONSE,
		.data = response_buffer,
		.length = strlen(response_buffer)
	};

	mcp_server_submit_tool_message(server, &response, execution_token);
	return ret;
}

/* Tool definitions */
static const struct mcp_tool_record led_control_tool = {
	.metadata = {
			.name = "led_control",
			.input_schema =
			"{"
			"\"type\":\"object\","
			"\"properties\":{"
				"\"r\":{\"type\":\"integer\"},"
				"\"g\":{\"type\":\"integer\"},"
				"\"b\":{\"type\":\"integer\"},"
				"\"rainbow\":{\"type\":\"boolean\"},"
				"\"color\":{\"type\":\"string\"},"
				"\"action\":{\"type\":\"string\"}"
			"}"
			"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Controls LED strip. Set r,g,b (0-255), color 'rgb(r,g,b)', rainbow boolean, or action (on/off/toggle/red/green/blue).",
#endif
#ifdef CONFIG_MCP_TOOL_TITLE
			.title = "LED Control Tool",
#endif
#ifdef CONFIG_MCP_TOOL_OUTPUT_SCHEMA
			.output_schema = "{\"type\":\"object\",\"properties\":{\"response\":{"
					 "\"type\":\"string\"}}}",
#endif
		},
	.callback = led_control_tool_callback
};

/* MCP server is called once, after device initialization */
int mcp_server(void)
{
	int ret;

	/* Check if LED device is ready */
	struct led_ready_msg ready_msg = {0};
	ret = zbus_chan_read(&led_ready_chan, &ready_msg, K_MSEC(500));
	if (ret == 0 && ready_msg.is_ready) {
		led_initialized = true;
		LOG_INF("LED device is ready (checked via Zbus channel)");
	} else {
		led_initialized = false;
		LOG_ERR("LED device is not ready via Zbus channel (read err %d, msg status %d)",
		       ret, ready_msg.status);
	}

	/* Initialize MCP server */
	server = mcp_server_init();
	if (server == NULL) {
		LOG_ERR("MCP Server initialization failed");
		return -ENOMEM;
	}

	/* Initialize HTTP server */
	ret = mcp_server_http_init(server);
	if (ret != 0) {
		LOG_ERR("MCP HTTP Server initialization failed: %d", ret);
		return ret;
	}

	LOG_INF("Registering Tool #1: LED Control...");
	ret = mcp_server_add_tool(server, &led_control_tool);
	if (ret != 0) {
		LOG_ERR("Tool #1 registration failed.");
		return ret;
	}
	// LOG_INF("Tool #1 registered.");

	LOG_INF("Starting...");
	ret = mcp_server_start(server);
	if (ret != 0) {
		LOG_ERR("MCP Server start failed: %d", ret);
		return ret;
	}

	ret = mcp_server_http_start(server);
	if (ret != 0) {
		LOG_ERR("MCP HTTP Server start failed: %d", ret);
		return ret;
	}

	LOG_INF("MCP Server running...");
	return 0;
}
