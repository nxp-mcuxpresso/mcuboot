/*
 * Copyright 2026 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ENCRYPTED_XIP_H__
#define __ENCRYPTED_XIP_H__

#include "fsl_common.h"
#include "flash_map.h"

/*
 * Identify type of encryption unit.
 *
 * These CPU macros should be declared in the project or makefile.
 */
#if !(defined(ENCRYPTED_XIP_BEE) || defined(ENCRYPTED_XIP_OTFAD) || \
      defined(ENCRYPTED_XIP_NPX) || defined(ENCRYPTED_XIP_IPED))
#error "Please define target encryption unit type"
#endif

/**
 * @brief Initialization of platform encryption unit
 *
 * This function performs the initial setup and initialization of the platform's
 * encryption hardware unit required for encrypted XIP operations.
 *
 * @return status_t Status code indicating success or failure:
 *         - kStatus_Success: Encryption unit initialized successfully
 *         - kStatus_Fail: Initialization failed
 *
 * @note This function should be called before any other encrypted XIP operations.
 * @see encrypted_xip_finish()
 */
status_t encrypted_xip_init(void);

/**
 * @brief Generate new configuration block and initialize the encryption for selected region
 *
 * Creates a new encryption configuration block and sets up encryption parameters
 * for the specified flash region. The configuration is prepared but not yet persisted.
 *
 * @param[in] fa_meta  Pointer to flash area structure for configuration storage (metadata)
 * @param[in] fa_slot  Pointer to flash area structure for the region to be encrypted
 *
 * @return status_t Status code indicating success or failure:
 *         - kStatus_Success: Configuration generated and region initialized successfully
 *         - kStatus_Fail: Configuration generation or initialization failed
 *
 * @note The configuration is hold in a context and must be written using 
 *       encrypted_xip_config_write() to persist it.
 * @see encrypted_xip_config_write()
 */
status_t encrypted_xip_config_region(const struct flash_area *fa_meta, const struct flash_area *fa_slot);

/**
 * @brief Persist configuration block in metadata flash area, confirm the integrity of the block
 *
 * Writes the previously generated configuration block to the metadata flash area
 * and verifies its integrity to ensure correct storage.
 *
 * @param[in] fa_meta  Pointer to flash area structure for metadata storage
 *
 * @return status_t Status code indicating success or failure:
 *         - kStatus_Success: Configuration written and verified successfully
 *         - kStatus_Fail: Write operation or integrity check failed
 *
 * @note This function should be called after finished OTA update using 
 *       encrypted_xip_flash_write().
 * @see encrypted_xip_config_region()
 * @see encrypted_xip_config_isValid()
 */
status_t encrypted_xip_config_write(const struct flash_area *fa_meta);

/**
 * @brief Checks whether configuration block in metadata flash area is valid
 *
 * Validates the integrity and correctness of the configuration block stored
 * in the metadata flash area, so it is safe to initialiaze encrypted XIP using
 * this configuration block
 *
 * @param[in]  fa_meta   Pointer to flash area structure for metadata storage
 * @param[out] isValid   Pointer to boolean that will be set to true if valid, false otherwise
 *
 * @return status_t Status code indicating success or failure of the check operation:
 *         - kStatus_Success: Validation check completed successfully (see isValid for result)
 *         - kStatus_Fail: Unable to perform validation check
 *
 * @note The isValid parameter indicates the validation result, while the return value
 *       indicates whether the check itself was successful.
 * @see encrypted_xip_config_isValid()
 */
status_t encrypted_xip_config_isValid(const struct flash_area *fa_meta, bool *isValid);

/**
 * @brief Initializes encryption unit based on configuration block in metadata flash area
 *
 * Reads the configuration block from metadata flash area and configures the
 * encryption unit accordingly. This function is typically called after a reboot
 * from bootloader context to restore encryption settings.
 *
 * @param[in] fa_meta  Pointer to flash area structure for metadata storage
 *
 * @return status_t Status code indicating success or failure:
 *         - kStatus_Success: Encryption unit initialized from configuration successfully
 *         - kStatus_Fail: Configuration read or initialization failed
 *
 * @note This function is typically called during boot sequence by the bootloader.
 * @see encrypted_xip_config_isValid()
 */
status_t encrypted_xip_config_initEncryption(const struct flash_area *fa_meta);

/**
 * @brief Returns max image size value which can be written to region
 *
 * Calculates the maximum image size that can be stored in an encrypted region,
 * accounting for encryption overhead and alignment requirements.
 *
 * @param[in] region_sz  Total size of the flash region in bytes
 *
 * @return uint32_t Maximum image size in bytes that can be written to the region.
 *                  Returns 0 if region_sz is invalid or too small.
 *
 * @note The returned value may be smaller than region_sz due to encryption overhead,
 *       and alignment requirements.
 */
uint32_t encrypted_xip_region_getImageMaxSz(uint32_t region_sz);

/**
 * @brief Finish initialization of encryption unit
 *
 * Completes the initialization sequence of the encryption unit, performing any
 * final setup steps required before normal operation.
 *
 * @return status_t Status code indicating success or failure:
 *         - kStatus_Success: Encryption unit finalization completed successfully
 *         - kStatus_Fail: Finalization failed
  *
 * @note This function should be called after finishing all write operations.
 */
status_t encrypted_xip_finish(void);

/* Write operations */

/**
 * @brief Encrypt and write data
 *
 * Encrypts the provided data and writes it to the specified flash area at the
 * given offset. The data is encrypted on-the-fly before being written to flash.
 *
 * @param[in] area  Pointer to flash area structure where data will be written
 * @param[in] off   Offset within the flash area (in bytes)
 * @param[in] src   Pointer to source data buffer to be encrypted and written
 * @param[in] len   Length of data to write (in bytes)
 *
 * @return status_t Status code indicating success or failure:
 *         - kStatus_Success: Data encrypted and written successfully
 *         - kStatus_Fail: Encryption or write operation failed
 *
 * @see encrypted_xip_flash_write_finish()
 */
status_t encrypted_xip_flash_write(const struct flash_area *area, uint32_t off, const void *src, uint32_t len);

/**
 * @brief Some encryption engines (e.g. IPED) require finish operation after an OTA update
 *
 * Performs any necessary finalization steps required by certain encryption engines
 * after completing a series of write operations, such as during an OTA update.
 * This may include flushing buffers, updating state, or finalizing encryption state.
 *
 * @param[in] area  Pointer to flash area structure that was written to
 *
 * @return status_t Status code indicating success or failure:
 *         - kStatus_Success: Write finalization completed successfully
 *
 * @note This function must be called after completing all write operations for
 *       encryption engines that require it (e.g., IPED). For engines that don't
 *       require finalization, this function may return kStatus_Success immediately.
 * @see encrypted_xip_flash_write()
 */
status_t encrypted_xip_flash_write_finish(const struct flash_area *area);

#endif
