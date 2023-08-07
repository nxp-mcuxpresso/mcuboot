include_guard(GLOBAL)


if (CONFIG_USE_middleware_mcuboot)
# Add set(CONFIG_USE_middleware_mcuboot true) in config.cmake to use this component

message("middleware_mcuboot component is included from ${CMAKE_CURRENT_LIST_FILE}.")


endif()


if (CONFIG_USE_middleware_mcuboot_bootutil)
# Add set(CONFIG_USE_middleware_mcuboot_bootutil true) in config.cmake to use this component

message("middleware_mcuboot_bootutil component is included from ${CMAKE_CURRENT_LIST_FILE}.")

if(CONFIG_USE_middleware_mbedtls)

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/boot_record.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/bootutil_misc.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/bootutil_public.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/caps.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/encrypted.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/fault_injection_hardening.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/fault_injection_hardening_delay_rng_mbedtls.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/image_ec.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/image_ec256.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/image_ed25519.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/image_rsa.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/image_validate.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/loader.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/swap_misc.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/swap_move.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/swap_scratch.c
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src/tlv.c
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/src
  ${CMAKE_CURRENT_LIST_DIR}/boot/bootutil/include
)

else()

message(SEND_ERROR "middleware_mcuboot_bootutil dependency does not meet, please check ${CMAKE_CURRENT_LIST_FILE}.")

endif()

endif()

