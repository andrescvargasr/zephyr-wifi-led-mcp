/*
 * Copyright (c) 2024 Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
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
#endif

#include "thd_mcp.h"

LOG_MODULE_REGISTER(wifi_test, LOG_LEVEL_INF);

static struct net_mgmt_event_callback wifi_mgmt_cb;
static struct net_mgmt_event_callback dhcp_mgmt_cb;

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				     uint64_t mgmt_event,
				     struct net_if *iface)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
		if (status->status == 0) {
			LOG_INF("Wi-Fi connected successfully!");
		} else {
			LOG_ERR("Wi-Fi connection failed (status: %d)", status->status);
		}
	} else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
		LOG_INF("Wi-Fi disconnected (status: %d)", status->status);
	}
}

static void dhcp_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				     uint64_t mgmt_event,
				     struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IPV4_DHCP_BOUND) {
		char buf[NET_IPV4_ADDR_LEN];
		struct net_if_dhcpv4 *data = (struct net_if_dhcpv4 *)cb->info;

		LOG_INF("DHCP IP Address acquired: %s",
			net_addr_ntop(AF_INET, &data->requested_ip, buf, sizeof(buf)));
	}
}

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
	printk("PSRAM size: %u bytes (%u MB)\n",
	       DT_PROP(DT_NODELABEL(psram0), size),
	       DT_PROP(DT_NODELABEL(psram0), size) / (1024 * 1024));
#else
	printk("PSRAM not available\n");
#endif

	LOG_INF("Starting Wi-Fi Shell application...");

	/* Register Wi-Fi event callbacks */
	net_mgmt_init_event_callback(&wifi_mgmt_cb, wifi_mgmt_event_handler,
				      NET_EVENT_WIFI_CONNECT_RESULT |
				      NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_mgmt_cb);

	/* Register DHCP event callback */
	net_mgmt_init_event_callback(&dhcp_mgmt_cb, dhcp_mgmt_event_handler,
				      NET_EVENT_IPV4_DHCP_BOUND);
	net_mgmt_add_event_callback(&dhcp_mgmt_cb);

	/* Trigger auto-connect on startup */
	auto_connect();

	k_sleep(K_SECONDS(20));

	thread_mcp();

	return 0;
}
