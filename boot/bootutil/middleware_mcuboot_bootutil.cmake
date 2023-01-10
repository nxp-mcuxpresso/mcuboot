#Description: MCUboot bootloader; user_visible: False
include_guard(GLOBAL)
message("middleware_mcuboot_bootutil component is included.")

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/boot_record.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bootutil_misc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bootutil_public.c
    ${CMAKE_CURRENT_LIST_DIR}/src/caps.c
    ${CMAKE_CURRENT_LIST_DIR}/src/encrypted.c
    ${CMAKE_CURRENT_LIST_DIR}/src/fault_injection_hardening.c
    ${CMAKE_CURRENT_LIST_DIR}/src/fault_injection_hardening_delay_rng_mbedtls.c
    ${CMAKE_CURRENT_LIST_DIR}/src/image_ec.c
    ${CMAKE_CURRENT_LIST_DIR}/src/image_ec256.c
    ${CMAKE_CURRENT_LIST_DIR}/src/image_ed25519.c
    ${CMAKE_CURRENT_LIST_DIR}/src/image_rsa.c
    ${CMAKE_CURRENT_LIST_DIR}/src/image_validate.c
    ${CMAKE_CURRENT_LIST_DIR}/src/loader.c
    ${CMAKE_CURRENT_LIST_DIR}/src/swap_misc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/swap_move.c
    ${CMAKE_CURRENT_LIST_DIR}/src/swap_scratch.c
    ${CMAKE_CURRENT_LIST_DIR}/src/tlv.c
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/src
)

#OR Logic component
if(${MCUX_DEVICE} STREQUAL "MIMXRT1052")
    include(middleware_mbedtls_MIMXRT1052)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMXRT1064")
    include(middleware_mbedtls_MIMXRT1064)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMXRT1062")
    include(middleware_mbedtls_MIMXRT1062)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMXRT1042")
    include(middleware_mbedtls_MIMXRT1042)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMXRT1176_cm7")
    include(middleware_mbedtls_MIMXRT1176_cm7)
endif()
if(${MCUX_DEVICE} STREQUAL "LPC55S69_cm33_core0")
    include(middleware_mbedtls_LPC55S69_cm33_core0)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMXRT685S_cm33")
    include(middleware_mbedtls_MIMXRT685S_cm33)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMXRT595S_cm33")
    include(middleware_mbedtls_MIMXRT595S_cm33)
endif()

