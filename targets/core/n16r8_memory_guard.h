#pragma once

// Include only from the N16R8 ECC target. Check the generated ESP-IDF configuration,
// rather than assuming that a requested YAML setting reached the compiler.
#include "sdkconfig.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3) || !CONFIG_IDF_TARGET_ESP32S3
#error "The N16R8 ECC profile requires ESP32-S3"
#endif
#if !defined(CONFIG_SPIRAM_MODE_OCT) || !CONFIG_SPIRAM_MODE_OCT
#error "The N16R8 ECC profile requires octal PSRAM"
#endif
#if !defined(CONFIG_SPIRAM_ECC_ENABLE) || !CONFIG_SPIRAM_ECC_ENABLE
#error "The N16R8 profile requires PSRAM ECC; do not claim the ECC temperature allowance without it"
#endif
#if !defined(CONFIG_ESPTOOLPY_FLASHSIZE_16MB) || !CONFIG_ESPTOOLPY_FLASHSIZE_16MB
#error "The N16R8 profile requires a 16 MB flash configuration"
#endif
