/**
 * Copyright (c) 2026 Andrés Camilo Román Vargas
 *
 * @file params.h
 * @author Andres C. Román V. (camilo.vargas@technaid.com gh: @andrescvargasr)
 * @brief Call the libraries, constants and parameters for all the code.
 * @version 0.1
 * @date 2026-07-28
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PARAMS_H
#define PARAMS_H

#include <stdlib.h>        // EXIT_[SUCCESS, FAILURE]
#include <zephyr/kernel.h> // k_msleep()
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

// Version control
#include <zephyr/app_version.h>

#include <zephyr/sys/printk.h>  // printk()
#include <zephyr/logging/log.h> // LOG_[ERR, WRN, INF, DBG]
#include <zephyr/drivers/pwm.h> // PWM_DT_SPEC_GET(), pwm_is_ready_dt()
// Serial Communication (UART)
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

// Watchdog Timer (WDT)
#include <zephyr/drivers/watchdog.h>
// NVS
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
// #include <zephyr/fs/nvs.h>
#include <zephyr/kvss/nvs.h>

/*******************************************************************************
 *  FIRMWARE VERSION AND CARD ID                                               *
 ******************************************************************************/
#ifdef APPVERSION
#define SOFTWARE_VERSION APP_VERSION_STRING // String format for SW version
// #define SOFTWARE_VERSION_SEMVER ((APP_VERSION_NUMBER) >> 4U)                                                      /* MAJOR.MINOR.PATCH [0xM.mm.P] */
#define SOFTWARE_VERSION_SEMVER ((APP_VERSION_MAJOR << 12U) + (APP_VERSION_MINOR << 4U) + (APP_PATCHLEVEL << 0U)) /* MAJOR.MINOR.PATCH [0xM.mm.P] */
#else
#define SOFTWARE_VERSION "v0.20.1"      // String format for SW version
#define SOFTWARE_VERSION_SEMVER 0x0141U /* MAJOR.MINOR.PATCH [0xM.mm.P] */
#endif

#define HARDWARE_VERSION_SEMVER 0x0010U   /* MAJOR.MINOR.PATCH [0xM.mm.P] */
#define RELEASE_VERSION_SEMVER 0x0010U    /* MAJOR.MINOR.PATCH [0xM.mm.P] */
#define DESCRIPTOR_VERSION_SEMVER 0x1000U /* MAJOR.MINOR.PATCH [0xM.mm.P] */

#define KIND_ID 83    // 'S' in ASCII DEC (0x53) for Tech'S'tim := Stimulation
#define DEFAULT_ID 65 // 'A' in ASCII DEC (0x41) for first device A = 0
#define PRODUCT_ID (((KIND_ID) << 8) | (DEFAULT_ID))

/*******************************************************************************
 *  DEVICE DESCRIPTOR                                                          *
 ******************************************************************************/
typedef struct
{
    uint8_t len;                 // Device descriptor length
    uint16_t descriptor_version; // Device descriptor version
    uint8_t device_class;        //
    uint8_t device_subclass;
    uint8_t device_id;
    uint8_t device_protocol;
    uint16_t max_packetsize;
    uint16_t vendor;
    uint16_t product;
    uint16_t bcd_device;
    uint8_t serial_number;
    uint8_t ids;
    uint16_t hardware_version;
    uint16_t firmware_version;
    uint8_t w_ip[4];
    uint8_t protocol_supported;
    uint16_t can_id;
    uint16_t port;
    uint8_t byte_speed[3];
    uint8_t mac[6];
} device_descriptor_t;

/*******************************************************************************
 *  THREADS                                                                    *
 ******************************************************************************/
#define THD_LED // Enable WS2812B LED

/*******************************************************************************
 *  LOG                                                                        *
 ******************************************************************************/
/**
 * @brief
 * | LEVEL            | SEVERITY  | MACRO         |
 * | ---------------- | --------- | ------------- |
 * | 1 (most severe)  | Error     | LOG_LEVEL_ERR |
 * | 2                | Warning   | LOG_LEVEL_WRN |
 * | 3                | Info      | LOG_LEVEL_INF |
 * | 4 (least severe) | Debug     | LOG_LEVEL_DBG |
 *
 */
#define LOG_EN_LEVEL 3             /* Enable level messages (default) */
#define LOG_EN_SERIAL_LEVEL 3      /* Enable level msgs in Serial WQ */
#define LOG_EN_WORKQ_LEVEL 3       /* Enable level msgs in Serial WQ */
#define LOG_EN_NVS_LEVEL 3         /* Enable level msgs in thd_nvs */
#define LOG_EN_STIMULATION_LEVEL 3 /* Enable level msgs in thd_stimulation*/
#define LOG_EN_DAC_LEVEL 3         /* Enable level msgs in dac_module */
#define LOG_EN_SWITCH_LEVEL 3      /* Enable level msgs in switch_module */
#define LOG_EN_TIMER_LEVEL 3       /* Enable level msgs in timer_module */

/*******************************************************************************
 *  DELAY                                                                      *
 ******************************************************************************/
#define MIN_DELAY 100
#define MAX_DELAY 1000

/*******************************************************************************
 *  TIMER EVENTS                                                               *
 ******************************************************************************/
#define TIMER_PERIOD_EVENT 0x001
#define TIMER_DELAY_EVENT 0x002
#define TIMER_GROUP_FREQUENCY_EVENT 0x003

/*******************************************************************************
 *  DEFINE MINIMUM, MAXIMUM RANGES AND DEFAULT VALUES                          *
 ******************************************************************************/
#define MAX_PARAMS 68 // [A-Z][AA-AZ][BA-BP]

#define MIN_AMP_RANGE 1   // [mA]
#define MAX_AMP_RANGE 100 // [mA]

#define MIN_PERIOD_RANGE 100  // [us] (Check 2 us - too small, IRQ > 19 us)
#define MAX_PERIOD_RANGE 1000 // [us]

#define MIN_FREQ_RANGE 1   // [Hz] (Check 1 Hz)
#define MAX_FREQ_RANGE 200 // [Hz] (Check for 200 Hz)

#define MIN_RAMP 1    // [ms] (Check 1 ms)
#define MAX_RAMP 1000 // [ms] (Check for 1000 ms)

#define MIN_REP_RANGE 1 // [single, double, triple]
#define MAX_REP_RANGE 3 // [single, double, triple]

// Dafault values
#define ORDER_VALUES 0x00E4U
#define INVERSE_RANGE 0    // [0 = false, 1 = true]
#define STIMULATION_MODE 0 // [0 = symmetrical, 1 = asymmetrical]

#define NO_ERRORS 0x0000 // Value to indicate No Errors in the system

/*******************************************************************************
 *  DEFINE PARAMETERS INDEXES                                                  *
 ******************************************************************************/
#define PARAM_POS_AMP_1 0  // A - Positive amplitude pulse 1 [0 - 100 mA]
#define PARAM_NEG_AMP_1 1  // B - Negative amplitude pulse 1 [0 - 100 mA]
#define PARAM_POS_TIME_1 2 // C - Positive time pulse 1 [100 - 1000 us]
#define PARAM_NEG_TIME_1 3 // D - Negative time pulse 1 [100 - 1000 us]

#define PARAM_POS_AMP_2 4  // E - Positive amplitude pulse 2 [0 - 100 mA]
#define PARAM_NEG_AMP_2 5  // F - Negative amplitude pulse 2 [0 - 100 mA]
#define PARAM_POS_TIME_2 6 // G - Positive time pulse 2 [100 - 1000 us]
#define PARAM_NEG_TIME_2 7 // H - Negative time pulse 2 [100 - 1000 US]

#define PARAM_POS_AMP_3 8   // I - Positive amplitude pulse 3 [0 - 100 mA]
#define PARAM_NEG_AMP_3 9   // J - Negative amplitude pulse 3 [0 - 100 mA]
#define PARAM_POS_TIME_3 10 // K - Positive time pulse 3 [100 - 1000 us]
#define PARAM_NEG_TIME_3 11 // L - Negative time pulse 3 [100 - 1000 us]

#define PARAM_POS_AMP_4 12  // M - Positive amplitude pulse 4 [0 - 100 mA]
#define PARAM_NEG_AMP_4 13  // N - Negative amplitude pulse 4 [0 - 100 mA]
#define PARAM_POS_TIME_4 14 // O - Positive time pulse 4 [100 - 1000 us]
#define PARAM_NEG_TIME_4 15 // P - Negative time pulse 4 [100 - 1000 us]

#define PARAM_ORDER_CHANNELS 16 // Q - Order channels
#define PARAM_REPETITIONS 17    // R - Repetitions [single, double, triple]
#define PARAM_INV_PULSE 18      // S - Reverse pulse position

#define PARAM_UMBRAL_MOTOR_1 19 // T - Umbral motor channel 1
#define PARAM_UMBRAL_MOTOR_2 20 // U - Umbral motor channel 2
#define PARAM_UMBRAL_MOTOR_3 21 // V - Umbral motor channel 3
#define PARAM_UMBRAL_MOTOR_4 22 // W - Umbral motor channel 4

#define PARAM_UMBRAL_COMFORT_1 23 // X - Umbral comfort channel 1
#define PARAM_UMBRAL_COMFORT_2 24 // Y - Umbral comfort channel 2
#define PARAM_UMBRAL_COMFORT_3 25 // Z - Umbral comfort channel 3
#define PARAM_UMBRAL_COMFORT_4 26 // AA - Umbral comfort channel 4

#define PARAM_MIN_CURRENT_1 27 // AB - Min current channel 1 [comfort - motor]
#define PARAM_MIN_CURRENT_2 28 // AC - Min current channel 2 [comfort - motor]
#define PARAM_MIN_CURRENT_3 29 // AD - Min current channel 3 [comfort - motor]
#define PARAM_MIN_CURRENT_4 30 // AE - Min current channel 4 [comfort - motor]

#define PARAM_MAX_CURRENT_1 31 // AF - Max current channel 1 [comfort - motor]
#define PARAM_MAX_CURRENT_2 32 // AG - Max current channel 2 [comfort - motor]
#define PARAM_MAX_CURRENT_3 33 // AH - Max current channel 3 [comfort - motor]
#define PARAM_MAX_CURRENT_4 34 // AI - Max current channel 4 [comfort - motor]

#define PARAM_INTRA_FREQ 35 // AJ - Intragroup Frequency [1 - 200 Hz]
#define PARAM_GROUP_FREQ 36 // AK - Group Frequency [1 - 200 Hz]

#define PARAM_UPGRADE_RAMP 37   // AL - Upgrade Ramp [0 - 1000 us]
#define PARAM_DOWNGRADE_RAMP 38 // AM - Downgrade Ramp [0 - 1000 us]

#define PARAM_STIMULATION_MODE 39 // AN - Stimulation Mode [Sym/Asym]

#define PARAM_DELAY 40 // AO - Delay for pulses [100 - 1000 us]

#define PARAM_DEVICE_ID 47        // AV - Device ID [PRODUCT_ID]
#define PARAM_FIRMWARE_VERSION 48 // AW - FW version [SOFTWARE_VERSION_SEMVER]
#define PARAM_ERROR 49            // AX - Errors
#define PARAM_STATUS 50           // AY - currently active services
#define PARAM_ENABLED 51          // AZ - enabled services (set by user)

#define PARAM_LOG_COUNTER (MAX_PARAMS - 1) // BP - Log Counter

#define PARAM_QUALIFIER PARAM_DEVICE_ID // Card ID index for the product

/*******************************************************************************
 *  FLAG DEFINITION                                                            *
 ******************************************************************************/
// PARAM_ERROR Flags
#define FLAG_CHANNEL_1_ERROR 0         // Channel 1 error
#define FLAG_CHANNEL_2_ERROR 1         // Channel 2 error
#define FLAG_CHANNEL_3_ERROR 2         // Channel 3 error
#define FLAG_CHANNEL_4_ERROR 3         // Channel 4 error
#define MASK_CHANNELS_ERROR 0b00001111 // Where are the bits for every channel

#define FLAG_POWER_SOURCE_ERROR 4 // the power supply is not enough
#define FLAG_DAC_ERROR 5          // DAC communication is failing
#define FLAG_SWITCH_ERROR 6       // Switch enable has a problem
#define FLAG_LOW_LOAD_ERROR 7     // Electrode impedance seems to be very low
#define FLAG_HIGH_LOAD_ERROR 8    // Electrode impedance seems to be very high
#define FLAG_FREQUENCY_ERROR 9    // The intragroup frequency is less than group frequency
// #define MASK_LOAD_ERROR 0b01110000 // where are the bit for other errors

// PARAM_STATUS & PARAM_ENABLED Flags
// the following flags are defined for PARAM_STATUS and PARAM_ENABLED
#define FLAG_CHANNEL_1 0         // Channel 1
#define FLAG_CHANNEL_2 1         // Channel 2
#define FLAG_CHANNEL_3 2         // Channel 3
#define FLAG_CHANNEL_4 3         // Channel 4
#define MASK_CHANNELS 0b00001111 // Where are the bits for every channel

// PARAM_ENABLED only
#define FLAG_STIMULATION_CONTROL 4  // Current control (default)
#define FLAG_RAMP_CONTROL 5         // TODO: Ramp control (Not implemented yet)
#define FLAG_PERIODIC_STIMULATION 6 // TODO: Stimulate for a setting time(Not implemented yet)

// PARAM_ORDER_CHANNELS
#define FLAG_ORDER_1_0 0        // Order pulse Flag (bit 0) for channel 1
#define FLAG_ORDER_1_1 1        // Order pulse Flag (bit 1) for channel 1
#define FLAG_ORDER_2_0 2        // Order pulse Flag (bit 0) for channel 2
#define FLAG_ORDER_2_1 3        // Order pulse Flag (bit 1) for channel 2
#define FLAG_ORDER_3_0 4        // Order pulse Flag (bit 0) for channel 3
#define FLAG_ORDER_3_1 5        // Order pulse Flag (bit 1) for channel 3
#define FLAG_ORDER_4_0 6        // Order pulse Flag (bit 0) for channel 4
#define FLAG_ORDER_4_1 7        // Order pulse Flag (bit 1) for channel 4
#define MASK_ORDER_1 0b00000011 // Where are the bits for channel 1
#define MASK_ORDER_2 0b00001100 // Where are the bits for channel 2
#define MASK_ORDER_3 0b00110000 // Where are the bits for channel 3
#define MASK_ORDER_4 0b11000000 // Where are the bits for channel 4
#define MASK_ORDER 0b11111111   // Where are the bits for every channel order

// PARAM_REPETITIONS
#define FLAG_REPETITIONS_1_0 0        // Order pulse Flag (bit 0) for channel 1
#define FLAG_REPETITIONS_1_1 1        // Order pulse Flag (bit 1) for channel 1
#define FLAG_REPETITIONS_2_0 2        // Order pulse Flag (bit 0) for channel 2
#define FLAG_REPETITIONS_2_1 3        // Order pulse Flag (bit 1) for channel 2
#define FLAG_REPETITIONS_3_0 4        // Order pulse Flag (bit 0) for channel 3
#define FLAG_REPETITIONS_3_1 5        // Order pulse Flag (bit 1) for channel 3
#define FLAG_REPETITIONS_4_0 6        // Order pulse Flag (bit 0) for channel 4
#define FLAG_REPETITIONS_4_1 7        // Order pulse Flag (bit 1) for channel 4
#define MASK_REPETITIONS_1 0b00000011 // Where are the bits for channel 1
#define MASK_REPETITIONS_2 0b00001100 // Where are the bits for channel 2
#define MASK_REPETITIONS_3 0b00110000 // Where are the bits for channel 3
#define MASK_REPETITIONS_4 0b11000000 // Where are the bits for channel 4
#define MASK_REPETITIONS 0b11111111   // Where are the bits for every channel order

// PARAM_INV_PULSE
#define NO_INV 0b00000000   // All the channels are in normal position
#define FLAG_INV_1 0        // Inverse pulse Flag for channel 1
#define FLAG_INV_2 2        // Inverse pulse Flag for channel 2
#define FLAG_INV_3 4        // Inverse pulse Flag for channel 3
#define FLAG_INV_4 6        // Inverse pulse Flag for channel 4
#define MASK_INV 0b01010101 // Where are the bits for every channel inverse

// value that should not be taken into account
// in case of error the parameter is set to this value
#define ERROR_VALUE -32768

/*******************************************************************************
 *  LOGGER                                                                     *
 ******************************************************************************/
#ifdef CONFIG_LOGS
#define LOG_INTERVAL 60 // Interval in (s) between logs logger
#else
#define LOG_INTERVAL (65535 / 1000) // Max 32 bits time log in (s)
#endif                              // End CONFIG_LOGS

#ifndef PARAM_LOG_COUNTER // Position to store number of logs in memory
#define PARAM_LOG_COUNTER (MAX_PARAMS - 1)
#endif // End PARAM_LOG_COUNTER

/*******************************************************************************
 *  WORKQUEUE THREAD                                                           *
 ******************************************************************************/
#define WORQ_THREAD_STACK_SIZE 512
#define WORKQ_LOWEST_PRIORITY CONFIG_NUM_PREEMPT_PRIORITIES // Lowest priority

#ifdef WORKQ_LOWEST_PRIORITY
#define WORKQ_PRIORITY K_PRIO_PREEMPT(WORKQ_LOWEST_PRIORITY)
#else
#define WORKQ_PRIORITY K_PRIO_PREEMPT(4)
#endif

/*******************************************************************************
 *  SERIAL                                                                     *
 ******************************************************************************/
#define SERIAL_BUFFER_LENGTH 64
#define SERIAL_MAX_PARAM_VALUE_LENGTH 32 // FIXME: It's not used
#define RECEIVE_TIMEOUT 16

// DMA & Ring buffer settings
#define RING_BUF_SIZE 64
#define RX_BUF_SIZE 10
// #ifdef SERIAL_BUFFER_LENGTH
// #define RX_BUF_SIZE SERIAL_BUFFER_LENGTH
// #else
// #define RX_BUF_SIZE 10
// #endif

// Call a uart extern variable
extern const struct device *uart;

#define MAX_TX_SERIAL_ERROR 10

/*******************************************************************************
 *  SERIAL THROUGH USB-CDC-UCM                                                 *
 ******************************************************************************/
extern struct ring_buf ring_buf; // To echo msg (main.c)
extern const struct device *const dev;
// USB-CDC-UCM or Serial communication
// printk("%s\r\n", tx_buffer);
#define SEND_SERIAL(tx_buffer, tx_buffer_len, err)            \
    do                                                        \
    {                                                         \
        if (CONFIG_USB_CDC_ACM)                               \
        {                                                     \
            size_t size_tx_buff =                             \
                tx_buffer_len +                               \
                snprintk(&tx_buffer[tx_buffer_len],           \
                         3, "\r\n");                          \
            while (ring_buf_space_get(&ring_buf) < 12)        \
                k_yield();                                    \
                                                              \
            ring_buf_put(&ring_buf, tx_buffer, size_tx_buff); \
            uart_irq_tx_enable(dev);                          \
        }                                                     \
        else                                                  \
        {                                                     \
            err = send_tx(tx_buffer, tx_buffer_len);          \
            if (err != 0)                                     \
            {                                                 \
                LOG_ERR("Serial TX doesn't work: %d", err);   \
            }                                                 \
        }                                                     \
    } while (0)

/*******************************************************************************
 *  WORKQUEUE THREAD                                                           *
 ******************************************************************************/
// FIFO pkt
typedef struct serial_pkt_recv
{
    char pkt_recv[SERIAL_BUFFER_LENGTH];
    size_t len_recv;
} serial_pkt_recv_t;

// Workqueue thd struct (main.c)
typedef struct work_serial_info
{
    struct k_work work;
    char name[25];
    serial_pkt_recv_t serial_pkt;
} work_serial_info_t;

// Workqueue thd struct (thd_0.c)
typedef struct work_info
{
    struct k_work work;
    char name[25];
} work_info_t;

/*******************************************************************************
 *  WORKQUEUE NVS MESSAGE                                                      *
 ******************************************************************************/
// All the parameters stored in the NVS
typedef struct data_nvs
{
    int16_t parameters[MAX_PARAMS];
} data_nvs_t;

// Parameter to send to NVS
typedef struct data_nvs_store
{
    size_t number;
    int16_t value;
} data_nvs_store_t;

// Time parameters to send to stimulation time thread
typedef struct stimulation_time_params
{
    int16_t on;
    int16_t off;
    int8_t iterations;
} stimulation_time_params_t;

// Mode parameters to send to stimulation time thread
typedef struct stimulation_mode_params
{
    int16_t enable;
} stimulation_mode_params_t;

/*******************************************************************************
 *  TIM THD                                                                    *
 ******************************************************************************/
// Call a led extern variable
extern const struct gpio_dt_spec led;

#endif // End PARAMS_H
