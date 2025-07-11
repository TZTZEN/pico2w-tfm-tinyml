/*
 * Copyright (c) 2017-2022, Arm Limited. All rights reserved.
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company)
 * or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "cmsis_os2.h"
#include "cmsis_compiler.h"
#include "tfm_ns_interface.h"
#include "tfm_nsid_manager.h"
#include "app.h"
#include "tfm_plat_ns.h"
#include "Driver_USART.h"
#include "device_cfg.h"
#ifdef TFM_NS_MAILBOX_API
#include "tfm_multi_core_api.h"
#include "tfm_ns_mailbox.h"
#endif
#include "tfm_log.h"
#include "uart_stdout.h"
#if (CONFIG_TFM_FLOAT_ABI >= 1)
#include "cmsis.h"
#endif

/**
 * \brief Modified table template for user defined SVC functions
 *
 * \details RTX has a weak definition of osRtxUserSVC, which
 *          is overridden here
 */
#if defined(__ARMCC_VERSION)
#if (__ARMCC_VERSION == 6110004)
/* Workaround needed for a bug in Armclang 6.11, more details at:
 * http://www.keil.com/support/docs/4089.htm
 */
__attribute__((section(".gnu.linkonce")))
#endif

/* Avoids the semihosting issue */
#if (__ARMCC_VERSION >= 6010050)
__asm("  .global __ARM_use_no_argv\n");
#endif
#endif

/**
 * \brief List of RTOS thread attributes
 */
static const osThreadAttr_t thread_attr = {
    .name = "tinyml_thread",
    .stack_size = 4096U,
    .tz_module = ((TZ_ModuleId_t)TFM_DEFAULT_NSID)
};
/**
 * \brief Static globals to hold RTOS related quantities,
 *        main thread
 */
static osThreadFunc_t thread_func = run_all_tests;

#ifdef TFM_MULTI_CORE_NS_OS_MAILBOX_THREAD
static osThreadFunc_t mailbox_thread_func = tfm_ns_mailbox_thread_runner;
static const osThreadAttr_t mailbox_thread_attr = {
    .name = "mailbox_thread",
    .stack_size = 1024U
};
#endif

#ifdef TFM_NS_MAILBOX_API
static struct ns_mailbox_queue_t ns_mailbox_queue;

static void tfm_ns_multi_core_boot(void)
{
    int32_t ret;

    LOG_MSG("Non-secure code running on non-secure core.\r\n");

    if (tfm_ns_wait_for_s_cpu_ready()) {
        LOG_MSG("Error sync'ing with secure core.\r\n");

        /* Avoid undefined behavior after multi-core sync-up failed */
        for (;;) {
        }
    }

    ret = tfm_ns_mailbox_init(&ns_mailbox_queue);
    if (ret != MAILBOX_SUCCESS) {
        LOG_MSG("Non-secure mailbox initialization failed.\r\n");

        /* Avoid undefined behavior after NS mailbox initialization failed */
        for (;;) {
        }
    }
}
#else /* TFM_NS_MAILBOX_API */
extern uint32_t tfm_ns_interface_init(void);
#endif /* TFM_NS_MAILBOX_API */

/**
 * \brief Platform peripherals and devices initialization.
 *        Can be overridden for platform specific initialization.
 *
 * \return  ARM_DRIVER_OK if the initialization succeeds
 */
__WEAK int32_t tfm_ns_platform_init(void)
{
    stdio_init();

    return ARM_DRIVER_OK;
}

/**
 * \brief Platform peripherals and devices de-initialization.
 *        Can be overridden for platform specific initialization.
 *
 * \return  ARM_DRIVER_OK if the de-initialization succeeds
 */
__WEAK int32_t tfm_ns_platform_uninit(void)
{
    stdio_uninit();

    return ARM_DRIVER_OK;
}


__WEAK int32_t tfm_ns_cp_init(void)
{
#if (CONFIG_TFM_FLOAT_ABI >= 1)
#ifdef __GNUC__
    /* Enable NSPE privileged and unprivilged access to the FP Extension */
    SCB->CPACR |= (3U << 10U*2U)     /* enable CP10 full access */
                  | (3U << 11U*2U);  /* enable CP11 full access */
#endif
#endif
    return ARM_DRIVER_OK;
}

/**
 * \brief main() function
 */
#ifndef __GNUC__
__attribute__((noreturn))
#endif
int main(void)
{
    if (tfm_ns_platform_init() != ARM_DRIVER_OK) {
        /* Avoid undefined behavior if platform init failed */
        while(1);
    }

    /* Early LOG test - should work after platform init */
    LOG_MSG("Platform initialized, LOG system active\r\n");

    if (tfm_ns_cp_init() != ARM_DRIVER_OK) {
        /* Avoid undefined behavior if co-porcessor init failed */
        while(1);
    }

    LOG_MSG("Co-processor initialized\r\n");

    (void) osKernelInitialize();
    LOG_MSG("RTOS kernel initialized\r\n");

#ifdef TFM_NS_MAILBOX_API
    tfm_ns_multi_core_boot();
#else
    /* Initialize the TFM NS interface */
    tfm_ns_interface_init();
    LOG_MSG("TFM NS interface initialized\r\n");
#endif

#ifdef TFM_MULTI_CORE_NS_OS_MAILBOX_THREAD
    (void) osThreadNew(mailbox_thread_func, NULL, &mailbox_thread_attr);
#endif

    (void) osThreadNew(thread_func, NULL, &thread_attr);

    LOG_MSG("Non-Secure system starting...\r\n");
    (void) osKernelStart();

    /* Reached only in case of error */
    for (;;) {
    }
}
