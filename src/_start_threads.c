/**
 * Copyright (c) 2026 Andrés Camilo Román Vargas
 *
 * @file _start_threads.c
 * @author Andres C. Román V. (camilo.vargas@technaid.com gh: @andrescvargasr)
 * @brief
 * @version 1.0
 * @date 2026-07-24
 *
 * Define and initialize the Threads and Workqueues for the project.
 *
 * SPDX-License-Identifier: MIT
 */

// Parameters
#include "params.h"

// Threads
#ifdef THD_LED
#include "thd_led.h"
#define THD_LED_STACKSIZE 512 // Should always be a power of two.
#define THREAD_LED_PRIORITY K_PRIO_PREEMPT(2)
#define THD_LED_DELAY_MS 10 // ms
#endif // THD_LED

#define THD_STACKSIZE 512 // Should always be a power of two.

#define DELAY_THD 10 // ms

/******************************************************************************/
/*                          Zephyr Workqueue Table	                          */
/******************************************************************************/
// #ifdef THD_WQ
// K_THREAD_STACK_DEFINE(my_stack_area, WORQ_THREAD_STACK_SIZE * 2); // Check first thd_wq.c
// #endif

/******************************************************************************/
/*                          Zephyr Message Queue	                          */
/******************************************************************************/
// K_FIFO_DEFINE(my_fifo);

/******************************************************************************/
/*                          Zephyr Multithreading Table                       */
/******************************************************************************/
// #define THD_LED_STACKSIZE THD_STACKSIZE
// static Z_KERNEL_STACK_DEFINE_IN(thd_led_stack, THD_LED_STACKSIZE, __attribute__((section(".ext_ram.bss"))));

#ifdef THD_LED

/*
 * Thread LED Definition was moved to main file to use PSRAM
*/

K_THREAD_DEFINE(thd_led, THD_LED_STACKSIZE, thread_led, NULL, NULL, NULL,
				THREAD_LED_PRIORITY, 0, (0 * THD_LED_DELAY_MS));
#endif // End THD_LED