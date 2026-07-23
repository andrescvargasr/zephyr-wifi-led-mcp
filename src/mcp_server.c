/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mcp/mcp_server.h>
#include <zephyr/net/mcp/mcp_server_http.h>
#include <zephyr/zbus/zbus.h>

#include "led_zbus.h"

LOG_MODULE_REGISTER(mcp_sample_hello, LOG_LEVEL_INF);

// #define LED0_NODE DT_ALIAS(led0)
#define DELAYED_RESPONSE_TEXT "Hello from the delayed response tool!"

mcp_server_ctx_t server;

// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static bool led_initialized;

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
 * @brief Simplified callback that will trigger the SSE-style response. Can be used to test
 * that the client reacts appropriately. Shows the necessary ping and cancel mechanisms required
 * from tool callbacks by the MCP Server state machine.
 *
 * @param event MCP tool event type, either a call request or a cancel request
 * @param arguments Arguments sent by the client requesting this call request
 * @param execution_token Token that identifies the execution
 *
 * @return 0 on success, negative errno on failure
 */
static int delayed_response_tool_callback(enum mcp_tool_event_type event,
					const char *arguments, const char *execution_token)
{
	struct mcp_tool_message response;
	struct mcp_tool_message ping;

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

	response = (struct mcp_tool_message){
		.type = MCP_USR_TOOL_RESPONSE,
		.data = DELAYED_RESPONSE_TEXT,
		.length = strlen(DELAYED_RESPONSE_TEXT)
	};

	ping = (struct mcp_tool_message){
		.type = MCP_USR_TOOL_PING,
		.data = NULL,
		.length = 0
	};

	k_msleep(3000);
	mcp_server_submit_tool_message(server, &ping, execution_token);
	k_msleep(3000);
	mcp_server_submit_tool_message(server, &ping, execution_token);
	k_msleep(3000);
	mcp_server_submit_tool_message(server, &ping, execution_token);
	k_msleep(3000);

	printk("Delayed response tool executed with arguments: %s, token: %s\n",
		arguments ? arguments : "none", execution_token);

	return mcp_server_submit_tool_message(server, &response, execution_token);
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
	char response_buffer[64];
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

	if (!led_initialized) {
		struct mcp_tool_message error_response = {
			.type = MCP_USR_TOOL_RESPONSE,
			.data = "LED not initialized",
			.length = strlen("LED not initialized")
		};
		mcp_server_submit_tool_message(server, &error_response, execution_token);
		return -ENODEV;
	}

	printk("received arguments: %s\n", arguments ? arguments : "none");

	action = extract_json_string_value(arguments, "\"action\"");

	bool valid_action = true;
	struct led_msg msg;

	if ((action != NULL) && (strcmp(action, "on") == 0)) {
		msg.action = LED_ACTION_ON;
		printk("Zbus publishing: LED ON\n");
		snprintk(response_buffer, sizeof(response_buffer), "LED turned ON via Zbus");
	} else if ((action != NULL) && (strcmp(action, "off") == 0)) {
		msg.action = LED_ACTION_OFF;
		printk("Zbus publishing: LED OFF\n");
		snprintk(response_buffer, sizeof(response_buffer), "LED turned OFF via Zbus");
	} else if ((action != NULL) && (strcmp(action, "toggle") == 0)) {
		msg.action = LED_ACTION_TOGGLE;
		printk("Zbus publishing: LED TOGGLE\n");
		snprintk(response_buffer, sizeof(response_buffer), "LED toggled via Zbus");
	} else if ((action != NULL) && (strcmp(action, "red") == 0)) {
		msg.action = LED_ACTION_RED;
		printk("Zbus publishing: LED RED\n");
		snprintk(response_buffer, sizeof(response_buffer), "LED turned red via Zbus");
	} else if ((action != NULL) && (strcmp(action, "green") == 0)) {
		msg.action = LED_ACTION_GREEN;
		printk("Zbus publishing: LED GREEN\n");
		snprintk(response_buffer, sizeof(response_buffer), "LED turned green via Zbus");
	} else if ((action != NULL) && (strcmp(action, "blue") == 0)) {
		msg.action = LED_ACTION_BLUE;
		printk("Zbus publishing: LED BLUE\n");
		snprintk(response_buffer, sizeof(response_buffer), "LED turned blue via Zbus");
	} else {
		valid_action = false;
		printk("Invalid action. Use: on, off, toggle, red, green, or blue\n");
		snprintk(response_buffer, sizeof(response_buffer),
				"Invalid action. Use: on, off, toggle, red, green, or blue");
	}

	if (valid_action) {
		int pub_rc = zbus_chan_pub(&led_chan, &msg, K_MSEC(200));
		if (pub_rc != 0) {
			printk("Zbus publish failed with error %d\n", pub_rc);
			snprintk(response_buffer, sizeof(response_buffer),
				 "Zbus publish failed: %d", pub_rc);
		}
	}

	response = (struct mcp_tool_message){
		.type = MCP_USR_TOOL_RESPONSE,
		.data = response_buffer,
		.length = strlen(response_buffer)
	};

	printk("LED control tool executed with arguments: %s, token: %s\n",
		arguments ? arguments : "none", execution_token);
	mcp_server_submit_tool_message(server, &response, execution_token);
	return ret;
}

/* Tool definitions */
static const struct mcp_tool_record delayed_response_tool = {
	.metadata = {
			.name = "delayed_response",
			.input_schema =
			"{"
			"\"type\":\"object\","
			"\"properties\":{"
				"\"message\":{"
					"\"type\":\"string\","
					"\"description\":\"The message to display\""
				"}"
			"},"
			"\"required\":[]"
			"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Sends a hello message after 12000 ms.",
#endif
#ifdef CONFIG_MCP_TOOL_TITLE
			.title = "Delayed Response Tool",
#endif
#ifdef CONFIG_MCP_TOOL_OUTPUT_SCHEMA
			.output_schema = "{\"type\":\"object\",\"properties\":{\"response\":{"
					 "\"type\":\"string\"}}}",
#endif
		},
	.callback = delayed_response_tool_callback
};

static const struct mcp_tool_record led_control_tool = {
	.metadata = {
			.name = "led_control",
			.input_schema =
			"{"
			"\"type\":\"object\","
			"\"properties\":{"
				"\"action\":{"
					"\"type\":\"string\","
					"\"enum\":[\"on\",\"off\",\"toggle\",\"red\",\"green\",\"blue\"],"
					"\"description\":\"The LED action to perform\""
				"}"
			"},"
			"\"required\":[\"action\"]"
			"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Controls the LED based on input command (on/off/toggle/red/green/blue)",
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

int mcp_server(void)
{
	int ret;

	printk("Initializing MCP Server...\n");
	led_initialized = false;

	// if (gpio_is_ready_dt(&led)) {
	// 	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	// 	if (ret < 0) {
	// 		printk("LED GPIO configuration failed: %d\n", ret);
	// 	} else {
			led_initialized = true;
	// 		printk("LED initialized successfully\n");
	// 	}
	// } else {
	// 	printk("LED GPIO not ready\n");
	// }

	server = mcp_server_init();
	if (server == NULL) {
		printk("MCP Server initialization failed");
		return -ENOMEM;
	}

	ret = mcp_server_http_init(server);
	if (ret != 0) {
		printk("MCP HTTP Server initialization failed: %d\n", ret);
		return ret;
	}

	printk("Registering Tool #1: Delayed response...\n");
	ret = mcp_server_add_tool(server, &delayed_response_tool);
	if (ret != 0) {
		printk("Tool #1 registration failed.\n");
		return ret;
	}
	printk("Tool #1 registered.\n");

	printk("Registering Tool #2: LED Control...\n");
	ret = mcp_server_add_tool(server, &led_control_tool);
	if (ret != 0) {
		printk("Tool #2 registration failed.\n");
		return ret;
	}
	printk("Tool #2 registered.\n");

	printk("Starting...\n");
	ret = mcp_server_start(server);
	if (ret != 0) {
		printk("MCP Server start failed: %d\n", ret);
		return ret;
	}

	ret = mcp_server_http_start(server);
	if (ret != 0) {
		printk("MCP HTTP Server start failed: %d\n", ret);
		return ret;
	}

	printk("MCP Server running...\n");
	return 0;
}
