# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "FreeETarget.html.S"
  "FreeETarget.png.S"
  "bootloader\\bootloader.bin"
  "bootloader\\bootloader.elf"
  "bootloader\\bootloader.map"
  "ca_cert.pem.S"
  "config\\sdkconfig.cmake"
  "config\\sdkconfig.h"
  "esp-idf\\esptool_py\\flasher_args.json.in"
  "esp-idf\\mbedtls\\x509_crt_bundle"
  "flash_app_args"
  "flash_bootloader_args"
  "flash_project_args"
  "flasher_args.json"
  "freeETarget.bin"
  "freeETarget.map"
  "help.html.S"
  "ldgen_libraries"
  "ldgen_libraries.in"
  "menu.html.S"
  "project_elf_src_esp32s3.c"
  "x509_crt_bundle.S"
  )
endif()
