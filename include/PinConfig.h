#pragma once

// Board profile selector.
//
// Default build keeps the original ESP32 DevKit pin map.
// The ESP32-C3 Super Mini build passes BOARD_ESP32_C3_SUPER_MINI
// from platformio.ini and loads the C3 pin profile.

#if defined(BOARD_ESP32_C3_SUPER_MINI)
#include "PinConfigEsp32C3SuperMini.h"
#else
#include "PinConfigEsp32Devkit.h"
#endif
