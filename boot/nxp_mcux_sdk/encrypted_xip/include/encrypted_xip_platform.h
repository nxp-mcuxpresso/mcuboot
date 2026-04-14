/*
 * Copyright 2026 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ENCRYPTED_XIP_PLATFORM_H__
#define __ENCRYPTED_XIP_PLATFORM_H__

#include "fsl_common.h"
#include "flash_map.h"

status_t platform_enc_xip_init(void);

size_t platform_enc_xip_config_getSize(void);

status_t platform_enc_xip_config_region(const struct flash_area *fa_meta, const struct flash_area *fa_slot);

status_t platform_enc_xip_config_persist(const struct flash_area *fa_meta);

status_t platform_enc_xip_config_initEncryption(const struct flash_area *fa_meta);

uint32_t platform_enc_xip_region_getImageMaxSz(uint32_t region_sz);

status_t platform_enc_xip_config_isValid(const struct flash_area *fa_meta, bool *isValid);

status_t platform_enc_xip_finish(void);

status_t platform_enc_xip_flash_write(const struct flash_area *area, uint32_t off, const void *src, uint32_t len);

status_t platform_enc_xip_flash_write_finish(const struct flash_area *area);

#endif
