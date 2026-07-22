/*
 * Copyright (c) 2026 Andrés Camilo Román Vargas
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

#define STRIP_NODE		DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS	DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define DELAY_TIME K_MSEC(CONFIG_SAMPLE_LED_UPDATE_DELAY)

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

static struct led_rgb pixels[STRIP_NUM_PIXELS];

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

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
	if (device_is_ready(strip)) {
		LOG_INF("Found LED strip device %s", strip->name);
	} else {
		LOG_ERR("LED strip device %s is not ready", strip->name);
		return -ENODEV;
	}
	return 0;
}

int led_set(uint8_t r, uint8_t g, uint8_t b)
{
	int rc;

	memset(&pixels, 0x00, sizeof(pixels));
	pixels[0].r = r;
	pixels[0].g = g;
	pixels[0].b = b;

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

	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;
	uint16_t hue = 0; /* angle 0..359 */
	int32_t last_log_time = 0;

	while (1) {
		hsv_to_rgb(hue, &red, &green, &blue);
		hue = (hue + 1) % 360;

		int64_t now = k_uptime_get();
		if (now - last_log_time >= 5000) {
			LOG_INF("Hue %u => RGB %u,%u,%u", hue, red, green, blue);
			last_log_time = now;
		}

		pixels[0].r = red;
		pixels[0].g = green;
		pixels[0].b = blue;

		rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
		if (rc) {
			LOG_ERR("couldn't update strip: %d", rc);
		}

		k_sleep(DELAY_TIME);
	}
}
