/**
 * Copyright (c) 2026 Andrés Camilo Román Vargas
 *
 * @file thd_led.c
 * @author Andres C. Román V. (camilo.vargas@technaid.com gh: @andrescvargasr)
 * @brief Thread for LED control and rainbow animation.
 * @version 0.1
 * @date 2026-07-24
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <string.h>

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_rgb);

#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>

#include "led_zbus.h"

#define STRIP_NODE		DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS	DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define DELAY_TIME K_MSEC(CONFIG_SAMPLE_LED_UPDATE_DELAY)

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

#if defined(CONFIG_ESP_SPIRAM)
__attribute__ ((section (".ext_ram.bss"))) static struct led_rgb pixels[STRIP_NUM_PIXELS];
#else
static struct led_rgb pixels[STRIP_NUM_PIXELS];
#endif // CONFIG_ESP_SPIRAM


static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

/* Zbus channel and subscriber definition */
ZBUS_SUBSCRIBER_DEFINE(led_sub, 4);

ZBUS_CHAN_DEFINE(led_chan,
		 struct led_msg,
		 NULL,
		 NULL,
		 ZBUS_OBSERVERS(led_sub),
		 ZBUS_MSG_INIT(.action = LED_ACTION_ON, .r = 0, .g = 0, .b = 0, .rainbow = true)
);

ZBUS_CHAN_DEFINE(led_ready_chan,
		 struct led_ready_msg,
		 NULL,
		 NULL,
		 ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(.is_ready = false, .status = -ENODEV)
);

enum led_mode {
	LED_MODE_OFF,
	LED_MODE_RAINBOW,
	LED_MODE_SOLID,
};

static void hsv_to_rgb(uint16_t h, uint8_t *r, uint8_t *g, uint8_t *b)
{
	float hh = (h % 360) / 60.0f;
	int i = (int)hh;
	float ff = hh - i;
	float p = 0.0f;
	float q = 1.0f - ff;
	float t = ff;

	float rf, gf, bf;
	switch (i) {
	case 0: rf = 1; gf = t; bf = p; break;
	case 1: rf = q; gf = 1; bf = p; break;
	case 2: rf = p; gf = 1; bf = t; break;
	case 3: rf = p; gf = q; bf = 1; break;
	case 4: rf = t; gf = p; bf = 1; break;
	case 5:
	default: rf = 1; gf = p; bf = q; break;
	}
	*r = (uint8_t)(rf * CONFIG_SAMPLE_LED_BRIGHTNESS);
	*g = (uint8_t)(gf * CONFIG_SAMPLE_LED_BRIGHTNESS);
	*b = (uint8_t)(bf * CONFIG_SAMPLE_LED_BRIGHTNESS);
}

int led_is_ready(void)
{
	struct led_ready_msg ready_msg = {0};

	if (device_is_ready(strip)) {
		LOG_INF("Found LED strip device %s", strip->name);
		ready_msg.is_ready = true;
		ready_msg.status = 0;
	} else {
		LOG_ERR("LED strip device %s is not ready", strip->name);
		ready_msg.is_ready = false;
		ready_msg.status = -ENODEV;
	}

	int pub_rc = zbus_chan_pub(&led_ready_chan, &ready_msg, K_MSEC(100));
	if (pub_rc != 0) {
		LOG_ERR("Failed to publish LED ready status to Zbus: %d", pub_rc);
	}

	return ready_msg.status;
}

int led_set(uint8_t r, uint8_t g, uint8_t b)
{
	int rc;

	memset(&pixels, 0x00, sizeof(pixels));
	for (size_t i = 0; i < STRIP_NUM_PIXELS; i++) {
		pixels[i].r = r;
		pixels[i].g = g;
		pixels[i].b = b;
	}

	rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
	if (rc) {
		LOG_ERR("couldn't update strip: %d", rc);
	}
	return rc;
}

void thread_led(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int rc = led_is_ready();
	if (rc) {
		return;
	}

	const struct zbus_channel *chan;
	struct led_msg msg;
	enum led_mode mode = LED_MODE_RAINBOW;
	uint8_t solid_r = 0, solid_g = 0, solid_b = 0;
	uint8_t red = 0, green = 0, blue = 0;
	uint16_t hue = 0;

	while (1) {
		rc = zbus_sub_wait(&led_sub, &chan, DELAY_TIME);
		if (rc == 0) {
			if (zbus_chan_read(chan, &msg, K_NO_WAIT) == 0) {
				LOG_INF("Zbus received action: %d", msg.action);
				switch (msg.action) {
				case LED_ACTION_OFF:
					mode = LED_MODE_OFF;
					led_set(0, 0, 0);
					break;
				case LED_ACTION_ON:
					mode = LED_MODE_RAINBOW;
					break;
				case LED_ACTION_TOGGLE:
					if (mode == LED_MODE_OFF) {
						mode = LED_MODE_RAINBOW;
					} else {
						mode = LED_MODE_OFF;
						led_set(0, 0, 0);
					}
					break;
				case LED_ACTION_RED:
					mode = LED_MODE_SOLID;
					solid_r = CONFIG_SAMPLE_LED_BRIGHTNESS;
					solid_g = 0;
					solid_b = 0;
					led_set(solid_r, solid_g, solid_b);
					break;
				case LED_ACTION_GREEN:
					mode = LED_MODE_SOLID;
					solid_r = 0;
					solid_g = CONFIG_SAMPLE_LED_BRIGHTNESS;
					solid_b = 0;
					led_set(solid_r, solid_g, solid_b);
					break;
				case LED_ACTION_BLUE:
					mode = LED_MODE_SOLID;
					solid_r = 0;
					solid_g = 0;
					solid_b = CONFIG_SAMPLE_LED_BRIGHTNESS;
					led_set(solid_r, solid_g, solid_b);
					break;
				case LED_ACTION_CUSTOM:
					if (msg.rainbow) {
						mode = LED_MODE_RAINBOW;
					} else {
						mode = LED_MODE_SOLID;
						solid_r = msg.r;
						solid_g = msg.g;
						solid_b = msg.b;
						led_set(solid_r, solid_g, solid_b);
					}
					break;
				}
			}
		} else {
			/* No new message, execute loop step according to mode */
			if (mode == LED_MODE_RAINBOW) {
				hsv_to_rgb(hue, &red, &green, &blue);
				hue = (hue + 1) % 360;
				led_set(red, green, blue);
			} else if (mode == LED_MODE_SOLID) {
				led_set(solid_r, solid_g, solid_b);
			} else if (mode == LED_MODE_OFF) {
				led_set(0, 0, 0);
			}
		}
	}
}
