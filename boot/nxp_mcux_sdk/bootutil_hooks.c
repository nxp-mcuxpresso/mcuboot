/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2017 NXP. Not a Contribution
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "bootutil/bootutil_public.h"
#include "bootutil/boot_hooks.h"
#include "bootutil_priv.h"
#include "bootutil/bootutil_log.h"

#include "fsl_debug_console.h"

#include "sblconfig.h"
#include "flash_map.h"
#include "flash_partitioning.h"

#ifdef CONFIG_BOOT_MODE_ENCRYPTED_XIP
#include "encrypted_xip.h"
#endif

#ifdef NDEBUG
#undef assert
#define assert(x) ((void)(x))
#endif

int boot_read_image_header_hook(int img_index, int slot, struct image_header *img_head)
{
    return BOOT_HOOK_REGULAR;
}

fih_int boot_image_check_hook(int img_index, int slot)
{
    return BOOT_HOOK_REGULAR;
}

int boot_perform_update_hook(int img_index, struct image_header *img_head,
                             const struct flash_area *area)
{
    return BOOT_HOOK_REGULAR;
}

int boot_copy_region_pre_hook(int img_index, const struct flash_area *area, size_t size)
{
#ifdef CONFIG_BOOT_MODE_ENCRYPTED_XIP

#ifdef ENCRYPTED_XIP_IPED
   /* 
    * IPED consumes 1.25 (5/4) time of physical memory and requires data chunks
    * processed by ROM IAP to be aligned to 4*page size. This complicates
    * usage of OVERWRITE_ONLY_FAST mode. For this reason we have to calculate 
    * remaining sectors not erased in loader code and erase them manually here.
    * Assume all sectors has same size.
    */
    int rc;
    uint32_t sector_sz = MFLASH_SECTOR_SIZE;
    uint32_t iped_chunk_sz = 4 * MFLASH_PAGE_SIZE;
    //calculate output IPED size
    //round up to chunk size needed by ROM IAP boundary
    uint32_t payload_sz = ((size + iped_chunk_sz - 1) / iped_chunk_sz) * iped_chunk_sz;
    //calculate resulting physical size with interleaving IPED tags
    uint32_t payload_iped_sz = (payload_sz * 5) / 4;
    //round up to nearest sector boundary
    payload_iped_sz = ((payload_iped_sz + sector_sz - 1) / sector_sz) * sector_sz;
    
    //calculate max IPED binary size and check IPED region boundaries
    //don't touch the slot trailer - reserve one sector
    uint32_t iped_region_max_size = boot_flash_map[0].fa_size - sector_sz;
    if(payload_iped_sz > iped_region_max_size)
    {
        PRINTF("Error: Calculated size of IPED region needed by the image exceeds the maximum region size!\n");
        //Invalidate the image in secondary slot for next reset
        rc = boot_erase_region(&boot_flash_map[1], 0, sector_sz, false);
        assert(rc == 0);
        return -1;
    }
    
    uint32_t sector_iped_cnt = payload_iped_sz / sector_sz;
    //calculate numbers of sectors to be erased by mcuboot code
    uint32_t sector_cnt = ((payload_sz + sector_sz - 1) / sector_sz);
    size_t sect, off = sector_cnt * sector_sz;
    //delete remaining sectors
    for (sect = sector_cnt; sect < sector_iped_cnt; sect++) {
        rc = boot_erase_region(area, off, sector_sz, false);
        assert(rc == 0);
        off += sector_sz;
    }
    
#endif  

    status_t status;
    const uint32_t enc_region_start = area->fa_off + BOOT_FLASH_BASE;
       
    status = encrypted_xip_cfg_write(boot_flash_meta_map, enc_region_start, size);
    if (status != kStatus_Success)
        return -1;
    
    status = encrypted_xip_cfg_initEncryption(boot_flash_meta_map);
    if (status != kStatus_Success)
        return -1;
#endif
    return 0;
}

int boot_copy_region_post_hook(int img_index, const struct flash_area *area, size_t size)
{
    /* 
     * Image was re-encrypted. Confirm the validity of configuration block
     */
#ifdef CONFIG_BOOT_MODE_ENCRYPTED_XIP
    status_t status;
    
    status = encrypted_xip_flash_write_finish(area);
    
    status = encrypted_xip_cfg_confirm(boot_flash_meta_map);
    if (status != kStatus_Success)
      return -1;
#endif
    return 0;
}

int boot_serial_uploaded_hook(int img_index, const struct flash_area *area,
                              size_t size)
{
    return BOOT_HOOK_REGULAR;
}

int boot_read_swap_state_primary_slot_hook(int image_index, struct boot_swap_state *state)
{
    return BOOT_HOOK_REGULAR;
}

int boot_reset_request_hook(bool force)
{
    return 0;
}