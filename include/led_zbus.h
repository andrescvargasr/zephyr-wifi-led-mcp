/**
 * Copyright (c) 2026 Andrés Camilo Román Vargas
 *
 * @file led_zbus.h
 * @author Andres C. Román V. (camilo.vargas@technaid.com gh: @andrescvargasr)
 * @brief Zbus channels and messages for LED strip control.
 * @version 0.1
 * @date 2026-07-28
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LED_ZBUS_H
#define LED_ZBUS_H

#include <zephyr/zbus/zbus.h>

enum led_action {
	LED_ACTION_OFF = 0,
	LED_ACTION_ON,
	LED_ACTION_TOGGLE,
	LED_ACTION_RED,
	LED_ACTION_GREEN,
	LED_ACTION_BLUE,
};

struct led_msg {
	enum led_action action;
};

struct led_ready_msg {
	bool is_ready;
	int status;
};

ZBUS_CHAN_DECLARE(led_chan);
ZBUS_CHAN_DECLARE(led_ready_chan);

#endif /* LED_ZBUS_H */
