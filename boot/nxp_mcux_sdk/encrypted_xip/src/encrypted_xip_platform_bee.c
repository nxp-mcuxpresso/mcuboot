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
#if defined(ENCRYPTED_XIP_BEE) && defined(CONFIG_BOOT_MODE_ENCRYPTED_XIP_OVERWRITE)
#include <ctype.h>

#include "encrypted_xip_platform.h"

#include "fsl_debug_console.h"
#include "fsl_dcp.h"
#include "fsl_bee.h"
#include "fsl_trng.h"

#include "mflash_drv.h"
#include "flash_partitioning.h"
#include "flash_map.h"
#include "sysflash/sysflash.h"

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

#define FLASH_ADDR             BOOT_FLASH_ACT_APP
/* Mcuboot trailer is not encrypted, reserve flash sector */
#define BEE_REGION_MAX_SIZE    (BOOT_FLASH_CAND_APP - BOOT_FLASH_ACT_APP - MFLASH_SECTOR_SIZE)

#define PROT_REGION_ALIGN_SIZE MFLASH_SECTOR_SIZE

/* FAC region configuration registers */
#define GPR_FAC_0_START_ADDR    GPR18
#define GPR_FAC_0_END_ADDR      GPR19

#define GPR_FAC_1_START_ADDR    GPR20
#define GPR_FAC_1_END_ADDR      GPR21

#define GPR_FAC_2_START_ADDR    GPR22
#define GPR_FAC_2_END_ADDR      GPR23

#define GPR_FAC_3_START_ADDR    GPR24
#define GPR_FAC_3_END_ADDR      GPR25

#define GPR_BEE_DE_RX_EN        GPR11
#define BEE_DECRYPT_EN(x)       IOMUXC_GPR_GPR11_BEE_DE_RX_EN(x) /* 0x1/0x2/0x4/0x8 */

/** Software key for HW emulation of BEE_KEY1_SEL fuses (SW, OTPMK, SW_GP2) for development. 
 *  No need to burn any fuses to test L2 Bootloader. BEE_KE1_SEL fuse can be burned anytime.
 *  SW recognize settings and configure keys accordingly for BEE and DCP.
 */
#define SW_AES_KEY              {0x15ab568c, 0x8d84f4c5, 0xacf5952a, 0x456c0b4a}

/*******************************************************************************
 * Types
 ******************************************************************************/

typedef struct {
    uint8_t aes_key[16];       // AES KEY
    uint8_t iv[16];            // AES Initial Vector
} kib_t;

_Static_assert(sizeof(kib_t) == 32,
        "kib_t structure has to be 32 bytes");

/** Structure holds BEE configurations.
 */
typedef struct {
    uint32_t region_1_start;    // BEE region 1 start address, 4kB aligned, rest is region 0
    uint32_t region_1_end;      // BEE region 1 end address, 4kB aligned, rest is region 0
    bee_aes_mode_t aes_mode;    // BEE AES mode - 0 -> ECB, 1 -> CTR
    uint32_t lock_option;       // BEE lock option
    uint32_t aes_ctr_nonce[4];  // BEE nonce for AES CTR algorithm, only the upper 96bits are configurable
    uint8_t reserved[32];       // Reserved for future use
} encrypt_region_info_t;

/** Structure holds FAC region configurations.
 */
typedef struct {
    uint32_t start;             // Start address of FAC protection region, 1kB aligned
    uint32_t end;               // End address of FAC region, 1kB aligned
    uint32_t mode;              // Access control of memory region-0 (0 - no restrictions, 1 - debug disabled)
    uint8_t reserved[20];       // Reserved for future use
} fac_region_t;

#define PRDB_TAGL               0x5F474154      //"TAG_"
#define PRDB_TAGH               0x52444845      //"EHDR"
#define PRDB_VERSION            0x56010000      // Version 1.0.0

/** Structure holds Protection Region Descriptor Block (PRDB) configurations.
 */
typedef struct {
    uint32_t tagl;                             // TAG constant for PRDB identification
    uint32_t tagh;                             // TAG constant for PRDB identification
    uint32_t version;                          // PRDB version
    uint32_t fac_region_count;                 // Flash Access Controlled (FAC) Region within this encrypted region (1-3)
    encrypt_region_info_t encrypt_region_info; // Information for encrypted region
    fac_region_t fac_region_0;                 // Fac configuration for region 0 \ref prdb_fac_reg_t
    fac_region_t fac_region_1;                 // Fac configuration for region 1 \ref prdb_fac_reg_t
    fac_region_t fac_region_2;                 // Fac configuration for region 2 \ref prdb_fac_reg_t
    fac_region_t fac_region_3;                 // Fac configuration for region 3 \ref prdb_fac_reg_t
    uint8_t reserved[48];                      // Reserved for future use
} prdb_t;

_Static_assert(sizeof(prdb_t) == 256,
        "prdb_t structure has to be 256 bytes");

typedef struct {
    uint32_t  ekib[32 / sizeof(uint32_t)];     // Encrypted KIB info (32 bytes)
    uint32_t  eprdb[256 / sizeof(uint32_t)];   // Encrypted PRDB info (256 bytes)
} bee_cfg_ctx_t;

/*
 * Assume that metadata and BEE configuration structs can be written separately 
 * into common flash sector.
 */
_Static_assert(sizeof(bee_cfg_ctx_t) <= MFLASH_SECTOR_SIZE/2,
               "Size of BEE cfg exceeds flash page size");

/*******************************************************************************
 * Variables
 ******************************************************************************/

/** Template holds configuration of BEE region 1 for execution slot. BEE region 0
 *  is reserved for encrypted bootloader.
 */
static prdb_t prdb_template __attribute__((aligned)) = {
       .tagl = PRDB_TAGL,
       .tagh = PRDB_TAGH,
       .version = PRDB_VERSION,
       .fac_region_count = 1,
       .encrypt_region_info = { .region_1_start = FLASH_ADDR, /* Only region 1 - rest is region 0 */
                                .region_1_end = FLASH_ADDR + BEE_REGION_MAX_SIZE,
                                .aes_mode = kBEE_AesCtrMode,
                                .lock_option = 0,
                                .aes_ctr_nonce = {0xDEADBEEF,
                                                  0xDEADBEEF,
                                                  0xDEADBEEF,
                                                  0xDEADBEEF
                                                 },
                              },
       .fac_region_2 = { .start = FLASH_ADDR,
                         .end = FLASH_ADDR + BEE_REGION_MAX_SIZE,
                         .mode = 0,
                       },
       };

bee_cfg_ctx_t bee_cfg_ctx;
/*******************************************************************************
 * Static
 ******************************************************************************/

static status_t dcp_aes_ctr_crypt(dcp_handle_t *handle,
                                  uint8_t *nonce_counter,
                                  uint8_t *input,
                                  uint8_t *output,
                                  uint32_t length);

/**
 Increments counter in nonce.

 \param big_num pointer nonce array
 \param bytes   array size
 */
static void increment_counter(uint8_t *big_num, uint32_t bytes) {
    uint32_t i;

    for (i = bytes - 1; i > 0; i--) {
        big_num[i]++;
        if (big_num[i] != 0) {
            break;
        }
    }
}

/**
 Swaps endianness.

 \param array pointer to array to be swapped
 */
static void aes_block_swap(uint8_t array[16]) {
    uint32_t i;
    uint8_t tmp;

    for (i = 0; i < 8; i++) {
        tmp = array[i];
        array[i] = array[15 - i];
        array[15 - i] = tmp;
    }
}

/**
 Generates random number.

 \param buf pointer to buffer to store random number
 \param size Size of buffer
 */
static status_t generate_rand(uint32_t *buf, uint32_t size) {

    trng_config_t trngConfig;
    status_t status = kStatus_Success;

    TRNG_GetDefaultConfig(&trngConfig);
    trngConfig.sampleMode = kTRNG_SampleModeVonNeumann;

    status = TRNG_Init(TRNG, &trngConfig);

    if (kStatus_Success != status) {
        return status;
    }

    status = TRNG_GetRandomData(TRNG, buf, size);

    TRNG_Deinit(TRNG);

    return status;
}

/**
 Function provides key to DCP based on BEE_KEY1_SEL fuses:
 00 SW key - SW_AES_KEY is loaded
 10 OTPMK  - Unique master key for each chip
 11 SW_GP2 - User key burned into fuses by user
 */
static status_t dcp_bee_key_select(dcp_handle_t *handle)
{
    status_t status;

    handle->channel = kDCP_Channel2;
    handle->swapConfig = kDCP_KeyByteSwap | kDCP_KeyWordSwap;

    /* Read the value BEE_KEY1_SEL (BEE region 1) */
    uint32_t bee_key_sel = OCOTP->CFG5 & 0x0000C000;

    if (bee_key_sel == 0x00008000) {
        /* Select OPTMK */
        handle->keySlot = kDCP_OtpUniqueKey;
        IOMUXC_GPR->GPR10 &= ~IOMUXC_GPR_GPR10_DCPKEY_OCOTP_OR_KEYMUX_MASK;
        IOMUXC_GPR->GPR3 |= IOMUXC_GPR_GPR3_DCP_KEY_SEL_MASK;
    } else if (bee_key_sel == 0x0000C000) {
        /* Select SW_GP2 */
        handle->keySlot = kDCP_OtpKey;
        IOMUXC_GPR->GPR10 |= IOMUXC_GPR_GPR10_DCPKEY_OCOTP_OR_KEYMUX_MASK;
    } else if (bee_key_sel == 0x00000000) {
        /* Use SW keys */
        handle->keySlot = kDCP_KeySlot2;
        handle->swapConfig = kDCP_NoSwap;
    } else {
        PRINTF("Error: Corrupted BEE_KEY1_SEL\n");
        return kStatus_Fail;
    }

    /* sw_aes_key is only used for SW key selection */
    uint32_t sw_aes_key[] = SW_AES_KEY;
    status = DCP_AES_SetKey(DCP, handle, (uint8_t*) sw_aes_key, 16);

    return status;
}

/*
 * Function takes EPRDB and EKIB blocks and decrypt them
 */
static status_t decrypt_prdb_kib(uint32_t *ekib, uint32_t *eprdb, kib_t *kib, prdb_t *prdb) {
    status_t status;
    dcp_handle_t dcp_handle;
    uint32_t kib_tmp[32 / sizeof(uint32_t)];       //Temporary Key info block
    uint32_t prdb_tmp[256 / sizeof(uint32_t)];     //Temporary Protection Region Descriptor Block
    kib_t *p_kib_tmp;
    prdb_t *p_prdb_tmp;

    status = kStatus_Fail;
    p_kib_tmp = (kib_t*) kib_tmp;
    p_prdb_tmp = (prdb_t*) prdb_tmp;

    status = dcp_bee_key_select(&dcp_handle);
    if (status != kStatus_Success)
        return status;

    /* Decrypt EKIB by ECB into KIB */
    DCP_AES_DecryptEcb(DCP, &dcp_handle, (uint8_t*) ekib, (uint8_t*) p_kib_tmp,
            sizeof(kib_t));

    /* Decrypt EPRDB by CBC into PRDB using KIB */
    dcp_handle.channel = kDCP_Channel2;
    dcp_handle.keySlot = kDCP_KeySlot2;
    dcp_handle.swapConfig = kDCP_NoSwap;

    status = DCP_AES_SetKey(DCP, &dcp_handle, (uint8_t*) kib_tmp, 16);
    if (status != kStatus_Success)
        return status;

    DCP_AES_DecryptCbc(DCP, &dcp_handle, (uint8_t*) eprdb, (uint8_t*) prdb_tmp,
            256, p_kib_tmp->iv);

    //PRINTF("Printing PRDB and KIB after decryption\n");
    //printf_prdb_kib(prdb_tmp, kib);

    if ((p_prdb_tmp->tagl != PRDB_TAGL) || (p_prdb_tmp->tagh != PRDB_TAGH)
            || (p_prdb_tmp->version != PRDB_VERSION)) {
        PRINTF("No PRDB found!\n");
        return kStatus_Fail;
    }

    if(prdb != NULL){
        memcpy(prdb, prdb_tmp, sizeof(prdb_t));
    }
    if(kib != NULL){
        memcpy(kib, kib_tmp, sizeof(kib_t));
    }
    memset(prdb_tmp, 0, sizeof(prdb_tmp));
    memset(kib_tmp, 0, sizeof(kib_tmp));

    return status;
}

/*
 * Function generates random KIB and AES-CTR nonce in PRDB and encrypt them into encrypted
 * blocks.
 */
static status_t generate_encrypt_prdb_kib(prdb_t *prdb, void *ekib_addr, void *eprdb_addr) 
{
    status_t status;
    dcp_handle_t dcp_handle;
    uint32_t kib_tmp[32 / sizeof(uint32_t)];    //Key info block
    uint32_t ekib_tmp[32 / sizeof(uint32_t)];   //Encrypted key info block
    uint32_t eprdb_tmp[256 / sizeof(uint32_t)]; //Encrypted Protection Region Descriptor Block
    kib_t *p_kib_tmp = (kib_t*) kib_tmp;

    status = kStatus_Fail;

    /* Generate random KIB */
    status = generate_rand(kib_tmp, 32);
    if (status != kStatus_Success) {
        PRINTF("Warning: TRNG failed to generate KIB");
    }

    /* Generate random AES-CTR nonce */
    status = generate_rand(prdb->encrypt_region_info.aes_ctr_nonce, 16);
    if (status != kStatus_Success) {
        PRINTF("Warning: TRNG failed to generate AES-CTR counter\n");
    }

    //PRINTF("Printing PRDB and KIB before encryption\n");
    //printf_prdb_kib(prdb, kib);

    /* Encrypt KIB into EKIB */
    status = dcp_bee_key_select(&dcp_handle);
    if (status != kStatus_Success)
        return status;

    DCP_AES_EncryptEcb(DCP, &dcp_handle, (uint8_t*) kib_tmp, (uint8_t*) ekib_tmp,
            sizeof(kib_tmp));

    /* Encrypt PRDB into EPRDB using KIB */
    dcp_handle.channel = kDCP_Channel2;
    dcp_handle.keySlot = kDCP_KeySlot2;
    dcp_handle.swapConfig = kDCP_NoSwap;

    status = DCP_AES_SetKey(DCP, &dcp_handle, (uint8_t*) kib_tmp, 16);
    if (status != kStatus_Success)
        return status;

    DCP_AES_EncryptCbc(DCP, &dcp_handle, (uint8_t*) prdb, (uint8_t*) eprdb_tmp, 256,
            p_kib_tmp->iv);

    /* Check if we are able to decrypt the PRDB */
    prdb_t prdb_tmp;
    status = decrypt_prdb_kib(ekib_tmp, eprdb_tmp, NULL, &prdb_tmp);
    if (status != kStatus_Success)
        return status;

    if (memcmp((uint8_t*) &prdb_tmp, (uint8_t*) prdb, sizeof(prdb_t)) != 0) {
        PRINTF("Validation of EPRDB decryption failed\n");
        return kStatus_Fail;
    }

    /* Clear unecrypted security stuff in stack just in case */
    memset(kib_tmp, 0, sizeof(kib_tmp));

    /* Copy finished EKIB and EPRDB back to source */
    memcpy(ekib_addr, ekib_tmp, sizeof(kib_t));
    memcpy(eprdb_addr, eprdb_tmp, sizeof(prdb_t));

    return status;
}

/**
 Function encrypts data by AES-CTR encryption.

 \param handle           pointer to DCP handler object
 \param nonce_counter    pointer to nonce array
 \param input            pointer to input data to be encrypted
 \param output           pointer to output buffer
 \param length           size of encrypted data

 \return SDK Error Code, use kStatus_Success or kStatus_Fail to evaluate.
 */
static status_t dcp_aes_ctr_crypt(dcp_handle_t *handle,
                                  uint8_t *nonce_counter,
                                  uint8_t *input,
                                  uint8_t *output,
                                  uint32_t length) 
{
    int status;

    if ((!nonce_counter) || (!input) || (!output)) {
        status = kStatus_InvalidArgument;
    } else {
        uint8_t tmp;
        uint32_t i;
        volatile uint8_t cipher_nonce[16];
        uint32_t calc_len;

        while (length > 0) {
            calc_len = (length < 16) ? length : 16;
            status = DCP_AES_EncryptEcb(DCP, handle, (uint8_t*) nonce_counter,
                    (uint8_t*) cipher_nonce, 16);
            if (status != kStatus_Success) {
                return status;
            }
            for (i = 0; i < calc_len; i++) {
                tmp = *input++;
                *output++ = (uint8_t) (tmp ^ cipher_nonce[i]);
            }

            length -= calc_len;

            // Increment counter, (128bit big-endian)
            increment_counter(nonce_counter, 16);
        }
    }
    return status;
}

/**
 Function configures FAC regions. Atleast one region has to be configured.

 \param prdb pointer to prdb config structure for BEE and FAC config

 \return SDK Error Code, use kStatus_Success or kStatus_Fail to evaluate.
 */
static status_t fac_regions_setup(prdb_t *prdb) 
{
    status_t status = kStatus_Fail;

    if ((prdb->fac_region_count == 0) || (prdb->fac_region_count > 3))
        return kStatus_Fail;

    if ((prdb->fac_region_0.start > BOOT_FLASH_BASE) && 
        (prdb->fac_region_0.end > prdb->fac_region_0.start)) {
        IOMUXC_GPR->GPR18 = prdb->fac_region_0.start;
        IOMUXC_GPR->GPR19 = prdb->fac_region_0.end;
        IOMUXC_GPR->GPR11 |= IOMUXC_GPR_GPR11_M7_APC_AC_R0_CTRL(prdb->fac_region_0.mode);
        IOMUXC_GPR->GPR11 |= IOMUXC_GPR_GPR11_BEE_DE_RX_EN(0x01);
        status = kStatus_Success;
    }

    if ((prdb->fac_region_1.start > BOOT_FLASH_BASE) && 
        (prdb->fac_region_1.end > prdb->fac_region_1.start)) {
        IOMUXC_GPR->GPR20 = prdb->fac_region_1.start;
        IOMUXC_GPR->GPR21 = prdb->fac_region_1.end;
        IOMUXC_GPR->GPR11 |= IOMUXC_GPR_GPR11_M7_APC_AC_R1_CTRL(prdb->fac_region_1.mode);
        IOMUXC_GPR->GPR11 |= IOMUXC_GPR_GPR11_BEE_DE_RX_EN(0x02);
        status = kStatus_Success;
    }

    if ((prdb->fac_region_2.start > BOOT_FLASH_BASE) &&
        (prdb->fac_region_2.end > prdb->fac_region_2.start)) {
        IOMUXC_GPR->GPR22 = prdb->fac_region_2.start;
        IOMUXC_GPR->GPR23 = prdb->fac_region_2.end;
        IOMUXC_GPR->GPR11 |= IOMUXC_GPR_GPR11_M7_APC_AC_R2_CTRL(prdb->fac_region_2.mode);
        IOMUXC_GPR->GPR11 |= IOMUXC_GPR_GPR11_BEE_DE_RX_EN(0x04);
        status = kStatus_Success;
    }

    if ((prdb->fac_region_3.start > BOOT_FLASH_BASE) && 
        (prdb->fac_region_3.end > prdb->fac_region_3.start)) {
        IOMUXC_GPR->GPR24 = prdb->fac_region_3.start;
        IOMUXC_GPR->GPR25 = prdb->fac_region_3.end;
        IOMUXC_GPR->GPR11 |= IOMUXC_GPR_GPR11_M7_APC_AC_R3_CTRL(prdb->fac_region_3.mode);
        IOMUXC_GPR->GPR11 |= IOMUXC_GPR_GPR11_BEE_DE_RX_EN(0x08);
        status = kStatus_Success;
    }

    return status;
}

static status_t bee_init_encryption(bee_cfg_ctx_t *bee_cfg)
{
    status_t status;
    prdb_t prdb;
    bee_region_config_t beeConfig;
    uint32_t nonce128b[16 / sizeof(uint32_t)];
    uint32_t sw_beeKey[16 / sizeof(uint32_t)] = SW_AES_KEY;
    
    status = decrypt_prdb_kib(bee_cfg->ekib, bee_cfg->eprdb, NULL, &prdb);
    if (status != kStatus_Success) {
        PRINTF("Fatal error: decrypted PRDB is invalid\n");
        goto error;
    }

    //printf_prdb_kib((uint32_t *)&prdb, NULL);

    /* Get default configuration. */
    BEE_GetDefaultConfig(&beeConfig);
    /* Set BEE regions to work in AES CTR mode */
    beeConfig.region0Mode = kBEE_AesCtrMode;
    beeConfig.region1Mode = kBEE_AesCtrMode;

    /* Configure BEE region1 address */
    assert(prdb.encrypt_region_info.region_1_start % PROT_REGION_ALIGN_SIZE == 0);
    assert(prdb.encrypt_region_info.region_1_end % PROT_REGION_ALIGN_SIZE == 0);
    beeConfig.region1Bot = prdb.encrypt_region_info.region_1_start;
    beeConfig.region1Top = prdb.encrypt_region_info.region_1_end;

    beeConfig.endianSwapEn = kBEE_EndianSwapEnabled;

    /* Configure Start address and end address of flash access protected regions */
    if (fac_regions_setup(&prdb) != kStatus_Success) {
        PRINTF("Invalid fac regions\n");
        goto error;
    }

    /* Init BEE driver and apply the configuration */
    BEE_Init(BEE);
    BEE_SetConfig(BEE, &beeConfig);

    /* Set AES user key and nonce for BEE region 1 */
    memcpy(nonce128b, prdb.encrypt_region_info.aes_ctr_nonce, 16);
    nonce128b[0] = 0;
    BEE_SetRegionNonce(BEE, kBEE_Region1, (uint8_t*) nonce128b, 16);

    aes_block_swap((uint8_t*) sw_beeKey);

    /* sw_beeKey is ignored if bee selects SW_GP2 or OTMPK */
    status = BEE_SetRegionKey(BEE, kBEE_Region1, (uint8_t*) sw_beeKey, 16);
    if (status != kStatus_Success) {
            PRINTF("BEE Key setup failed\n");
    }

    BEE_Enable(BEE);

    /* Data cache needed for unaligned access */
    SCB_InvalidateDCache();
    SCB_EnableDCache();
    
    return kStatus_Success;
    error:
    return kStatus_Fail;
}

/**
 Function encrypts data using AES-CTR encryption. Internally selects key based on
 value of BEE_KEY_SELECTION fuse.

 \param flash_addr       adress offset
 \param nonce            pointer to nonce array
 \param input            pointer to input data to be encrypted
 \param output           pointer to output buffer
 \param length           size of encrypted data

 \return SDK Error Code, use kStatus_Success or kStatus_Fail to evaluate.
 */
static status_t bee_encrypt_data(uint32_t flash_addr, uint8_t *nonce,
                                   uint8_t *input, uint8_t *output, uint32_t len)
{
    uint32_t counter[4];
    status_t status;
    dcp_handle_t dcp_handle;

    /* load nonce and convert it to big-endian */
    memcpy(counter, nonce, 16);
    counter[0] = flash_addr >> 4;
    aes_block_swap((uint8_t*) counter);

    /* Configure DCP based on BEE_KEY_SELECTION */
    status = dcp_bee_key_select(&dcp_handle);
    if (kStatus_Success != status) {
        PRINTF("DCP key config fail!\n");
        return status;
    }

    status = dcp_aes_ctr_crypt(&dcp_handle, (uint8_t*) counter, input, output, len);

    return status;
}

static status_t bee_loadNonce(uint8_t *nonce)
{
    status_t status;
    prdb_t prdb;

    /* Load PRDB */
    status = decrypt_prdb_kib(bee_cfg_ctx.ekib, bee_cfg_ctx.eprdb, NULL, &prdb);
    if (status != kStatus_Success) {
        PRINTF("Fatal error: decrypted PRDB is invalid\n");
        goto error;
    }

    memcpy(nonce, prdb.encrypt_region_info.aes_ctr_nonce, 16);
    memset(&prdb, 0, sizeof(prdb_t));
    return kStatus_Success;
    error: return kStatus_Fail;
}
/*******************************************************************************
 * Externs
 ******************************************************************************/
status_t platform_enc_xip_init(void)
{
    /* Initialize DCP */
    /* ToDo Could be initialized in PSA driver */
    dcp_config_t dcpConfig;

    DCP_GetDefaultConfig(&dcpConfig);
    DCP_Init(DCP, &dcpConfig);
    return kStatus_Success;
}

size_t platform_enc_xip_config_getSize(void)
{
    return sizeof(bee_cfg_ctx_t);
}

status_t platform_enc_xip_config_region(const struct flash_area *fa_meta, const struct flash_area *fa_slot)
{
	status_t status = kStatus_Fail;
        const uint32_t region_start = fa_slot->fa_off + BOOT_FLASH_BASE;
        const uint32_t bee_align_sz = PROT_REGION_ALIGN_SIZE;
        /* Reserve a sector for the slot trailer */
        //round down to align boundary
	const uint32_t region_sz = ((fa_slot->fa_size - MFLASH_SECTOR_SIZE) / bee_align_sz) * bee_align_sz;
        const uint32_t region_end = region_start + region_sz;
	uint32_t *p_eprdb = (uint32_t *) &bee_cfg_ctx.eprdb;
	uint32_t *p_ekib = (uint32_t *) &bee_cfg_ctx.ekib;
        prdb_t prdb;

        if(region_sz > BEE_REGION_MAX_SIZE)
        {
            PRINTF("Error: Calculated size of BEE region needed by the image exceeds maximum region size\n");
            goto error;;
        }
        
	/* Invalidate any previous iped configuration in metadata flash area */
	if (flash_area_erase(fa_meta, 0, MFLASH_SECTOR_SIZE) != 0) {
            PRINTF("Erase of metadata sector failed\n");
            goto error;
	}

	/* Prepare configuration blocks */
	memset((void*) &bee_cfg_ctx, 0, sizeof(bee_cfg_ctx_t));
	memcpy(&prdb, &prdb_template, sizeof(prdb_t));
        prdb.encrypt_region_info.region_1_start = region_start;
        prdb.encrypt_region_info.region_1_end = region_end;
        prdb.fac_region_2.start = region_start;
        prdb.fac_region_2.end = region_end;

	status = generate_encrypt_prdb_kib(&prdb, p_ekib, p_eprdb);
	if (status != kStatus_Success) {
            PRINTF("Encryption of PRDB failed\n");
            return status;
	}
        
        bee_init_encryption(&bee_cfg_ctx);

	return kStatus_Success;
	error: return kStatus_Fail;
}

status_t platform_enc_xip_config_persist(const struct flash_area *fa_meta)
{
    /* Persist BEE configuration at particular flash offset */
    if (flash_area_write(fa_meta, 0, &bee_cfg_ctx, sizeof(bee_cfg_ctx_t)) != 0) {
        PRINTF("Failed to write encryption metadata\n");
        return kStatus_Fail;
    }
  
    return kStatus_Success;
}

/**
 Configures BEE encryption unit based on configuration structure.
 In this case the BEE region 1 is configured (as BEE region 0 is reserved for
 bootloader.
 After successful setup the function encrypt_platform_encrypt_data() can be used
 for data encryption and logic access into encrypted region returns decrypted data.

 \param fa pointer to flash area of metadata sector

 \return SDK Error Code, use kStatus_Success or kStatus_Fail to evaluate.
 */
status_t platform_enc_xip_config_initEncryption(const struct flash_area *fa_meta)
{
	status_t status;
	bee_cfg_ctx_t bee_cfg_flash;
	prdb_t prdb;
        
	/* Load PRDB */
	status = flash_area_read(fa_meta, 0, &bee_cfg_flash, sizeof(bee_cfg_ctx_t));
	if (status != kStatus_Success) {
            PRINTF("Flash read failed\n");
            goto error;
	}

	bee_init_encryption(&bee_cfg_flash);

	return kStatus_Success;
	error: return kStatus_Fail;
}

status_t platform_enc_xip_config_isValid(const struct flash_area *fa_meta, bool *isValid)
{
    status_t status;
    prdb_t prdb_decrypted;
    *isValid = false;
    bee_cfg_ctx_t bee_cfg_flash;
    
    /* configuration block is located at the beginning of sector */
    if (flash_area_read(fa_meta, 0, &bee_cfg_flash, sizeof(bee_cfg_ctx_t)) != 0){
        return kStatus_Fail;
    }
    status = decrypt_prdb_kib(bee_cfg_flash.ekib, bee_cfg_flash.eprdb, NULL, &prdb_decrypted);
    if (status != kStatus_Success) {
        PRINTF("decrypt_prdb_kib failed\n");
        goto clean;
    }
    if (prdb_decrypted.tagh == PRDB_TAGH || prdb_decrypted.tagl == PRDB_TAGL) {
        *isValid = true;
    }   
clean:
    /* Destroy exposed prdb in ram */
    memset(&prdb_decrypted, 0, sizeof(prdb_t));
    return kStatus_Success;
}

status_t platform_enc_xip_finish(void)
{
    /* Nothing needed here for BEE */
    return kStatus_Success;
}

#define BUFFER_ENC_SZ   MFLASH_PAGE_SIZE
status_t platform_enc_xip_flash_write(const struct flash_area *area, uint32_t off, 
                                  const void *src, uint32_t len)
{
    static uint32_t page_buffer[BUFFER_ENC_SZ / sizeof(uint32_t)];
    uint32_t addr;
    uint32_t nonce[16 / sizeof(uint32_t)];    
    uint8_t *p_buf = (uint8_t *)page_buffer;
    uint8_t *src_p = (uint8_t *)src;
    
    if (bee_loadNonce((uint8_t *)nonce) != kStatus_Success){
        PRINTF("encrypted_xip_config_getNonce failed\n");
        goto error;
    }
    
    while(len > 0){
        uint32_t chunk_len = (len > BUFFER_ENC_SZ) ? BUFFER_ENC_SZ : len;
    
        memcpy(p_buf, src_p, chunk_len);

        addr = area->fa_off + off + BOOT_FLASH_BASE;
        if(bee_encrypt_data(addr, (uint8_t *)nonce, p_buf, p_buf, chunk_len) != kStatus_Success){
            PRINTF("bee_encrypt_data failed\n");
            goto error;
        }
    
        if(mflash_drv_page_program(addr - BOOT_FLASH_BASE, page_buffer) != kStatus_Success){
            PRINTF("mflash_drv_page_program failed\n");
            goto error;
        }
    
        len -= chunk_len;
        off += chunk_len;
        src_p += chunk_len;
    }
    memset(nonce, 0, 16);
    return kStatus_Success;
    error:
    return kStatus_Fail;
}

status_t platform_enc_xip_flash_write_finish(const struct flash_area *area)
{
    //nothing to do here
    return kStatus_Success;
}

/*******************************************************************************
 * Test
 ******************************************************************************/

#if 0
#define FLASH_PAGE_SIZE         256

static void printf_prdb_kib(uint32_t *p_prdb, uint32_t *p_kib)
{
    if (p_kib != NULL) {
        kib_t kib;

        memcpy((void*) &kib, p_kib, sizeof(kib_t));

        PRINTF("\nKIB\n");
        PRINTF("AES key %X-%X-%X-%X-%X-%X-%X-%X-%X-%X-%X-%X-%X-%X-%X-%X\n",
                kib.aes_key[0], kib.aes_key[1], kib.aes_key[2], kib.aes_key[3],
                kib.aes_key[4], kib.aes_key[5], kib.aes_key[6], kib.aes_key[7],
                kib.aes_key[8], kib.aes_key[9], kib.aes_key[10],
                kib.aes_key[11], kib.aes_key[12], kib.aes_key[13],
                kib.aes_key[14], kib.aes_key[15]);
        PRINTF("IV      %X-%X-%X-%X-%X-%X-%X-%X-%X-%X-%X-%X-%X-%X-%X-%X\n",
                kib.iv[0], kib.iv[1], kib.iv[2], kib.iv[3], kib.iv[4],
                kib.iv[5], kib.iv[6], kib.iv[7], kib.iv[8], kib.iv[9],
                kib.iv[10], kib.iv[11], kib.iv[12], kib.iv[13], kib.iv[14],
                kib.iv[15]);
    }

    if (p_prdb != NULL) {
        prdb_t prdb;

        memcpy((void*) &prdb, p_prdb, sizeof(prdb_t));

        PRINTF("\nPRDB\n");
        PRINTF("tagl: 0x%X \n", prdb.tagl);
        PRINTF("tagh: 0x%X \n", prdb.tagh);
        PRINTF("version: 0x%X \n", prdb.version);
        PRINTF("FAC region count: 0x%X \n", prdb.fac_region_count);

        PRINTF("Information for encrypted region\n");
        PRINTF("Region1 start: 0x%X \n",
                prdb.encrypt_region_info.region_1_start);
        PRINTF("Region1 end: 0x%X \n", prdb.encrypt_region_info.region_1_end);
        PRINTF("AES mode 0x%X\n", prdb.encrypt_region_info.aes_mode);
        PRINTF("Lock options: %d \n", prdb.encrypt_region_info.lock_option);
        PRINTF("Counter W0-W1-W2-W3:\n0x%X-0x%X-0x%X-0x%X \n",
                prdb.encrypt_region_info.aes_ctr_nonce[0],
                prdb.encrypt_region_info.aes_ctr_nonce[1],
                prdb.encrypt_region_info.aes_ctr_nonce[2],
                prdb.encrypt_region_info.aes_ctr_nonce[3]);

        PRINTF("FAC Regions\n");
        PRINTF("Header prdb1 fac0 start: 0x%X \n", prdb.fac_region_0.start);
        PRINTF("Header prdb1 fac0 end: 0x%X \n", prdb.fac_region_0.end);
        PRINTF("Header prdb1 fac1 start: 0x%X \n", prdb.fac_region_1.start);
        PRINTF("Header prdb1 fac1 end: 0x%X \n", prdb.fac_region_1.end);
        PRINTF("Header prdb1 fac2 start: 0x%X \n", prdb.fac_region_2.start);
        PRINTF("Header prdb1 fac2 end: 0x%X \n", prdb.fac_region_2.end);
        PRINTF("Header prdb1 fac3 start: 0x%X \n", prdb.fac_region_3.start);
        PRINTF("Header prdb1 fac3 end: 0x%X \n\n", prdb.fac_region_3.end);
    }
}

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

#endif /* ENCRYPTED_XIP_BEE */
