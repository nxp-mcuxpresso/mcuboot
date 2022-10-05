if (CONFIG_MCUX_COMPONENT_middleware.mcuboot.bootutil)

    mcux_add_include(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/boot/bootutil
        PROJECT_BASE_PATH bootutil
        INCLUDES
            src
            include
    )

    mcux_add_source(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/boot/bootutil
        PROJECT_BASE_PATH bootutil
        SOURCES
            include/bootutil/bench.h
            include/bootutil/boot_hooks.h
            include/bootutil/boot_public_hooks.h
            include/bootutil/boot_record.h
            include/bootutil/boot_status.h
            include/bootutil/bootutil.h
            include/bootutil/bootutil_log.h
            include/bootutil/bootutil_public.h
            include/bootutil/bootutil_test.h
            include/bootutil/caps.h
            include/bootutil/enc_key.h
            include/bootutil/enc_key_public.h
            include/bootutil/fault_injection_hardening.h
            include/bootutil/fault_injection_hardening_delay_rng.h
            include/bootutil/ignore.h
            include/bootutil/image.h
            include/bootutil/mcuboot_status.h
            include/bootutil/ramload.h
            include/bootutil/security_cnt.h
            include/bootutil/sign_key.h
            include/bootutil/crypto/aes_ctr.h
            include/bootutil/crypto/aes_kw.h
            include/bootutil/crypto/common.h
            include/bootutil/crypto/ecdh_p256.h
            include/bootutil/crypto/ecdh_x25519.h
            include/bootutil/crypto/ecdsa.h
            include/bootutil/crypto/hmac_sha256.h
            include/bootutil/crypto/rsa.h
            include/bootutil/crypto/sha.h
            src/boot_record.c
            src/bootutil_misc.c
            src/bootutil_misc.h
            src/bootutil_priv.h
            src/bootutil_public.c
            src/caps.c
            src/encrypted.c
            src/fault_injection_hardening.c
            src/fault_injection_hardening_delay_rng_mbedtls.c
            src/image_ecdsa.c
            src/image_ed25519.c
            src/image_rsa.c
            src/image_validate.c
            src/loader.c
            src/swap_misc.c
            src/swap_move.c
            src/swap_priv.h
            src/swap_scratch.c
            src/tlv.c
    )


endif ()


if (CONFIG_MCUX_COMPONENT_middleware.mcuboot.tinycrypt)

    mcux_add_include(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/ext/tinycrypt/lib
        PROJECT_BASE_PATH tinycrypt
        INCLUDES include
    )

    mcux_add_source(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/ext/tinycrypt/lib
        PROJECT_BASE_PATH tinycrypt
        SOURCES
            include/tinycrypt/aes.h
            include/tinycrypt/cbc_mode.h
            include/tinycrypt/ccm_mode.h
            include/tinycrypt/cmac_mode.h
            include/tinycrypt/constants.h
            include/tinycrypt/ctr_mode.h
            include/tinycrypt/ctr_prng.h
            include/tinycrypt/ecc.h
            include/tinycrypt/ecc_dsa.h
            include/tinycrypt/hmac.h
            include/tinycrypt/hmac_prng.h
            include/tinycrypt/sha256.h
            include/tinycrypt/utils.h
            include/tinycrypt/ecc_platform_specific.h
            source/aes_decrypt.c
            source/aes_encrypt.c
            source/cbc_mode.c
            source/ccm_mode.c
            source/cmac_mode.c
            source/ctr_mode.c
            source/ctr_prng.c
            source/ecc.c
            source/ecc_dsa.c
            source/hmac.c
            source/hmac_prng.c
            source/sha256.c
            source/utils.c
    )

endif ()

if (CONFIG_MCUX_COMPONENT_middleware.mcuboot.mbedtls-asn1)

    mcux_add_include(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/ext/mbedtls-asn1
        PROJECT_BASE_PATH mbedtls-asn1
        INCLUDES include
    )

    mcux_add_source(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/ext/mbedtls-asn1
        PROJECT_BASE_PATH mbedtls-asn1
        SOURCES
            include/mbedtls/asn1.h
            include/mbedtls/bignum.h
            include/mbedtls/build_info.h
            include/mbedtls/check_config.h
            include/mbedtls/ecdsa.h
            include/mbedtls/ecp.h
            include/mbedtls/error.h
            include/mbedtls/mbedtls_config.h
            include/mbedtls/md.h
            include/mbedtls/oid.h
            include/mbedtls/pk.h
            include/mbedtls/platform.h
            include/mbedtls/platform_util.h
            include/mbedtls/private_access.h
            include/mbedtls/rsa.h
            include/mbedtls/threading.h
            include/mbedtls/version.h
            include/common.h
            src/asn1parse.c
            src/platform_util.c
    )

endif ()

if (CONFIG_MCUX_COMPONENT_middleware.mcuboot.nxp_encrypted_xip)

    mcux_add_include(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/ext/nxp_encrypted_xip
        PROJECT_BASE_PATH encrypted_xip
        INCLUDES include
    )

    mcux_add_source(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/ext/nxp_encrypted_xip
        PROJECT_BASE_PATH nxp_encrypted_xip
        SOURCES
            include/mcuboot_enc_support.h
            include/platform_enc_common.h
            src/platform_enc_bee.c
            src/mcuboot_enc_support.c
    )

endif ()

if (CONFIG_MCUX_COMPONENT_middleware.mcuboot.nxp_bootutil_port)

    mcux_add_include(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/boot/nxp_mcux_sdk
        PROJECT_BASE_PATH bootutil
        INCLUDES flashapi
                 keys
                 include
                 include/mcuboot_config
                 .
    )

    mcux_add_source(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/boot/nxp_mcux_sdk
        PROJECT_BASE_PATH source
        SOURCES
            boot.c
            boot.h
            bootutil_hooks.c
            keys.c
    )

    mcux_add_source(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/boot/nxp_mcux_sdk
        PROJECT_BASE_PATH include
        SOURCES
            include/flash_map.h
            include/flash_map_backend/flash_map_backend.h
            include/mcuboot_config/mcuboot_config.h
            include/mcuboot_config/mcuboot_logging.h
            include/os/os_malloc.h
            include/sysflash/sysflash.h
    )

endif ()


if (CONFIG_MCUX_COMPONENT_middleware.mcuboot.nxp_app_support)
    mcux_add_include(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/boot/nxp_mcux_sdk
        PROJECT_BASE_PATH source
        INCLUDES app_support
                 include
                 include/mcuboot_config
                 .
    )

    mcux_add_source(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/boot/nxp_mcux_sdk
        PROJECT_BASE_PATH source
        SOURCES
            app_support/mcuboot_app_support.c
            app_support/mcuboot_app_support.h
            include/mcuboot_config/mcuboot_config.h
            include/flash_map.h
            include/sysflash/sysflash.h
    )
endif ()

if (CONFIG_MCUX_COMPONENT_middleware.mcuboot.bootutil.flashapi_mcx)
    mcux_add_include(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/boot/nxp_mcux_sdk
        INCLUDES flashapi
    )
    mcux_add_source(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/boot/nxp_mcux_sdk/flashapi
        SOURCES
            flash_api_mcxn10.c
    )
endif()

if (CONFIG_MCUX_COMPONENT_middleware.mcuboot.bootutil.flashapi_rt)
    mcux_add_include(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/boot/nxp_mcux_sdk
        INCLUDES flashapi
    )
    mcux_add_source(
        BASE_PATH ${SdkRootDirPath}/middleware/mcuboot_opensource/boot/nxp_mcux_sdk/flashapi
        SOURCES
            flash_api.c
    )
endif()



# Stack and heap setup

if (CONFIG_MCUX_COMPONENT_middleware.mcuboot.stack_heap_default_mbedtls)
mcux_add_iar_configuration(
    LD "--config_def=__stack_size__=0x2000 --config_def=__heap_size__=0x8000"
)

mcux_add_mdk_configuration(
    LD "--predefine=\"-D__stack_size__=0x2000\" --predefine=\"-D__heap_size__=0x8000\""
)

mcux_add_armgcc_configuration(
    LD "-Xlinker --defsym=__stack_size__=0x2000 -Xlinker --defsym=__heap_size__=0x8000"
)

endif()

if (CONFIG_MCUX_COMPONENT_middleware.mcuboot.stack_heap_default_tinycrypt)
mcux_add_iar_configuration(
    LD "--config_def=__stack_size__=0x1000 --config_def=__heap_size__=0x400"
)

mcux_add_mdk_configuration(
    LD "--predefine=\"-D__stack_size__=0x1000\" --predefine=\"-D__heap_size__=0x400\""
)

mcux_add_armgcc_configuration(
    LD "-Xlinker --defsym=__stack_size__=0x1000 -Xlinker --defsym=__heap_size__=0x400"
)

endif ()
