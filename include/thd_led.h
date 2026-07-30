/**
 * Copyright (c) 2026 Andrés Camilo Román Vargas
 *
 * @file thd_led.h
 * @author Andres C. Román V. (camilo.vargas@technaid.com gh: @andrescvargasr)
 * @brief Thread for LED control and rainbow animation.
 * @version 0.1
 * @date 2026-07-24
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef THD_LED_H
#define THD_LED_H

#include "params.h"
#include "led_zbus.h"

#ifdef THD_LED
// Thread
void thread_led(void);
#endif // End THD_LED

#endif // End THD_LED_H