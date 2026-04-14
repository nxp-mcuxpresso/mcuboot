/*
 * Copyright 2026 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "sblconfig.h"
#if defined(ENCRYPTED_XIP_NPX) && defined(CONFIG_BOOT_MODE_ENCRYPTED_XIP_OVERWRITE)
#include <ctype.h>
#include "fsl_debug_console.h"
#include "flash_map.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*
 * @brief Helper to assert function return status.
 *
 * @details Print finshed message and trap forever upon failure.
 */
#define ASSERT_NPX(expected, actual, ...) \
    do                                    \
    {                                     \
        if ((expected) != (actual))       \
        {                                 \
            PRINTF("Assertion failed: "); \
            PRINTF(__VA_ARGS__);          \
            while (1)                     \
                ;                         \
        }                                 \
    } while (0);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Static
 ******************************************************************************/

/*******************************************************************************
 * Externs
 ******************************************************************************/
status_t platform_enc_xip_init(void)
{
    /* Nothing to do here */
    return kStatus_Success;
}

size_t platform_enc_xip_config_getSize(void)
{
    /* Nothing to do here */
    return 0;
}

status_t platform_enc_xip_config_region(const struct flash_area *fa_meta, const struct flash_area *fa_slot)
{
    /* Nothing to do here */
    return kStatus_Success;
}

status_t platform_enc_xip_config_persist(const struct flash_area *fa_meta)
{
    /* Nothing to do here */
    return kStatus_Success;
}

status_t platform_enc_xip_config_initEncryption(const struct flash_area *fa_meta)
{
    /* Nothing to do here */
    return kStatus_Success;
}

status_t platform_enc_xip_config_isValid(const struct flash_area *fa_meta, bool *isValid)
{
    /* Nothing to do here */
    return kStatus_Success;
}

status_t platform_enc_xip_finish(void)
{
    /* Nothing to do here */
    return kStatus_Success;
}

status_t platform_enc_xip_flash_write(const struct flash_area *area, uint32_t off, const void *src, uint32_t len)
{
    /* Nothing to do here */
    return kStatus_Success;
}

status_t platform_enc_xip_flash_write_finish(const struct flash_area *area)
{
    //nothing to do here
    return kStatus_Success;
}

#endif /* ENCRYPTED_XIP_NPX && CONFIG_BOOT_MODE_ENCRYPTED_XIP_OVERWRITE */
