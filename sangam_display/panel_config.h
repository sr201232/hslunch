#pragma once
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// ESP32 DevKitC / WROOM-32E, 16MB flash, no PSRAM.
// Conventional 256x32, 1/16 scan HUB75 panel (or four 64x32 in one row).
constexpr int PANEL_WIDTH = 256;
constexpr int PANEL_HEIGHT = 32;
constexpr uint8_t PANEL_BRIGHTNESS = 48; // 0..255
constexpr bool PANEL_CLOCK_PHASE = true;
const HUB75_I2S_CFG::i2s_pins PANEL_PINS = {
  25, 26, 27, 14, 12, 13, // R1 G1 B1 R2 G2 B2
  23, 19, 5, 17, -1,     // A B C D E (unused)
  4, 15, 16             // LAT OE CLK
};
constexpr auto PANEL_DRIVER = HUB75_I2S_CFG::SHIFTREG;
