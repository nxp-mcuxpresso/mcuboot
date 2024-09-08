/*
 * Copyright 2023 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLATFORM_ENC_COMMON_H__
#define __PLATFORM_ENC_COMMON_H__

#include "fsl_common.h"
#include "flash_map.h"

/* Checks whether encryption metadata are valid  */
status_t platform_enc_cfg_read(struct flash_area *fa_meta, uint32_t *active_slot);

/* Initializes new encryption metadata with random nonce */
status_t platform_enc_cfg_write(struct flash_area *fa_meta, uint32_t active_slot);

/* Initializes platform on-the-fly encryption based on configuration in encryption metadata */
status_t platform_enc_cfg_init(struct flash_area *fa_meta, uint8_t *nonce);

status_t platform_enc_cfg_getNonce(struct flash_area *fa_meta, uint8_t *nonce);

/* Encrypts data */
status_t platform_enc_encrypt_data(uint32_t flash_addr, uint8_t *nonce, uint8_t *input, uint8_t *output, uint32_t len);

void hexdump(const void *src, size_t size);

#endif
