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
#if defined(CONFIG_BOOT_MODE_ENCRYPTED_XIP_OVERWRITE) || defined(CONFIG_BOOT_MODE_ENCRYPTED_XIP_REMAP)
#include <ctype.h>

#include "encrypted_xip.h"
#include "encrypted_xip_platform.h"
#include "fsl_debug_console.h"

#include "mflash_drv.h"
   
#include "flash_partitioning.h"

#include "mflash_drv.h"
#include "sysflash/sysflash.h"
#include "flash_map.h"
#include "flash_partitioning.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*
 * @brief Helper to assert function return status.
 *
 * @details Print finshed message and trap forever upon failure.
 */
#define ASSERT_APP(expected, actual, ...) \
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

#define ENC_MAGIC_SZ 32

/*******************************************************************************
 * Types
 ******************************************************************************/

/** Union holds magic number for encryption metadata block identification.
 */
union enc_magic_t {
	//ToDo MCUBOOT_BOOT_MAX_ALIGN ?
	uint8_t val[ENC_MAGIC_SZ];
};

/** Structure holds a confirmation part (magic number) of configuration block
 */
typedef struct {
	uint8_t magic[ENC_MAGIC_SZ];    // Magic number
} enc_confirm_t;

_Static_assert((sizeof(enc_confirm_t) % 16) == 0,
		"Unaligned size of enc_confirm_t struct");


/*
 * Assume that metadata and configuration structs can be written separately 
 * into common flash sector.
 */
_Static_assert(sizeof(enc_confirm_t) <= MFLASH_PAGE_SIZE,
		"Size of metadata exceeds flash page size");

_Static_assert(sizeof(enc_confirm_t) <= MFLASH_SECTOR_SIZE/2,
		"Size of metadata exceeds half flash sector size");
/*******************************************************************************
 * Variables
 ******************************************************************************/
const union enc_magic_t enc_magic = { .val = { 0xAA, 0xBB, 0xCC, 0xDD, 0x60,
		0x4D, 0xBA, 0x70, 0x34, 0x79, 0x2c, 0x0f, 0x2c, 0xb6, 0x0f, 0x35,
                0x7E, 0x2C, 0xA5, 0x61, 0xD9, 0x4B, 0xF3, 0x18, 0x9A, 0xE7, 0x34, 
                0xBC, 0x50, 0x8D, 0x26, 0xFE} };

#define ENC_MAGIC  (enc_magic.val)
/*******************************************************************************
 * Static
 ******************************************************************************/
#ifndef ENCRYPTED_XIP_NPX
static int magic_check(const uint8_t *magic) {
    if (memcmp(magic, ENC_MAGIC, ENC_MAGIC_SZ) == 0) {
        return 1;
    }
    return 0;
}
#endif
/*******************************************************************************
 * Externs
 ******************************************************************************/
status_t encrypted_xip_init(void)
{
    return platform_enc_xip_init();
}

status_t encrypted_xip_config_isValid(const struct flash_area *fa_meta, bool *isValid)
{
#ifndef ENCRYPTED_XIP_NPX
    enc_confirm_t confirm;
    bool cfg_valid = false;
    uint32_t cfg_block[1024 / sizeof(uint32_t)];
    /* confirmation block (magic) is located at the end of sector */
    const uint32_t off_confirm = fa_meta->fa_size - sizeof(enc_confirm_t);
    
    *isValid = false;
    
    if (flash_area_read(fa_meta, off_confirm, &confirm, sizeof(enc_confirm_t)) != 0){
        return kStatus_Fail;
    }
    
    /* Check configuration block only if magic is present */
    if (magic_check(confirm.magic)) {
        if (platform_enc_xip_config_isValid(fa_meta, isValid) != kStatus_Success){
            return kStatus_Fail;
        }
    }
#endif
    return kStatus_Success;
}

status_t encrypted_xip_config_region(const struct flash_area *fa_meta, const struct flash_area *fa_slot)
{ 
    return platform_enc_xip_config_region(fa_meta, fa_slot);
}

uint32_t encrypted_xip_region_getImageMaxSz(uint32_t region_sz)
{
    return platform_enc_xip_region_getImageMaxSz(region_sz);
}

status_t encrypted_xip_config_initEncryption(const struct flash_area *fa_meta)
{
    return platform_enc_xip_config_initEncryption(fa_meta);
}

/* In case of PRINCE based encryption units there is a risk when accessing to
 * unwritten pages which leads to crash of PRINCE module.
 * This operation ensures the integrity of IPED configuration of the image in
 * execution area.
 */
status_t encrypted_xip_config_write(const struct flash_area *fa_meta)
{
#ifndef ENCRYPTED_XIP_NPX
    uint32_t meta_off = fa_meta->fa_size - sizeof(enc_confirm_t);
    enc_confirm_t confirm;

    platform_enc_xip_config_persist(fa_meta);

    uint32_t len;
    len = platform_enc_xip_config_getSize();
    ASSERT_APP(0 , len % 16, "Unaligned size of configuration block len=%d\n", len);
    
    memcpy(confirm.magic, ENC_MAGIC, ENC_MAGIC_SZ);

    /*
     * Write confirmation at the end of metadata sector - confirm integrity of 
     * configuration block.
     */
    if (flash_area_write(fa_meta, meta_off, &confirm, sizeof(enc_confirm_t)) != 0) {
        PRINTF("Failed to write encryption confirmation\n");
        return kStatus_Fail;
    }
#endif
    return kStatus_Success;
}

status_t encrypted_xip_finish(void)
{
    return platform_enc_xip_finish();
}

status_t encrypted_xip_flash_write(const struct flash_area *area, uint32_t off, const void *src, uint32_t len)
{
    return platform_enc_xip_flash_write(area, off, src, len);
}

status_t encrypted_xip_flash_write_finish(const struct flash_area *area)
{
    return platform_enc_xip_flash_write_finish(area);
}

/*******************************************************************************
 * Test
 ******************************************************************************/

#if 0
#define FLASH_PAGE_SIZE         xxx

void hexdump(const void *src, size_t size)
{
    const unsigned char *src8 = src;
    const int CNT             = 16;

    for (size_t i = 0; i < size; i++)
    {
        int n = i % CNT;
        if (n == 0)
            PRINTF("%08x  ", i);
        PRINTF("%02X ", src8[i]);
        if ((i && n == CNT - 1) || i + 1 == size)
        {
            int rem = CNT - 1 - n;
            for (int j = 0; j < rem; j++)
                PRINTF("   ");
            PRINTF("|");
            for (int j = n; j >= 0; j--)
                PUTCHAR(isprint(src8[i - j]) ? src8[i - j] : '.');
            PRINTF("|\n");
        }
    }
    PUTCHAR('\n');
}



void dump_image(void)
{
  static uint32_t buffer[MFLASH_PAGE_SIZE / sizeof(uint32_t)];
  const struct flash_area *fap_src = boot_enc_flash_map;
  const struct flash_area *fap_dst = boot_dump_flash_map;
  uint32_t bytes_copied = 0;
  uint32_t cur_off = 0;
  uint32_t dump_size = 524288; // 0,5MB
  
  while(bytes_copied < dump_size)
  {
    memcpy(buffer, (void*) (BOOT_FLASH_ENC_PRT + cur_off), MFLASH_PAGE_SIZE);
    if((cur_off % MFLASH_SECTOR_SIZE) == 0){
      if(flash_area_erase(fap_dst, cur_off, MFLASH_SECTOR_SIZE) != 0){
        PRINTF("Flash erase failed");
        goto error;
      }
      PUTCHAR('.');
    }
    if(flash_area_write(fap_dst, cur_off, buffer, MFLASH_PAGE_SIZE) != 0){
      PRINTF("Flash write failed");
      goto error;
    }
    cur_off += MFLASH_PAGE_SIZE;
    bytes_copied += MFLASH_PAGE_SIZE;
  }
  return;
error:
  PRINTF("Dump failed");
}
#endif

#endif /* CONFIG_BOOT_MODE_ENCRYPTED_XIP_OVERWRITE */
