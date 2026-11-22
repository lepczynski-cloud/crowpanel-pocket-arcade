#pragma once

#include <Arduino.h>

// Version 2 QR code, error correction L, for:
// https://links.lepczynski.it/
// Rows are packed least-significant bit first to keep the firmware asset small.
namespace AboutQr {

constexpr uint8_t SIZE = 25;
constexpr uint32_t ROWS[SIZE] = {
    0x01FC6F7F, 0x01045841, 0x0175EA5D, 0x01743C5D, 0x01753C5D,
    0x01042A41, 0x01FD557F, 0x00016B00, 0x0105DA5B, 0x00F8BC3C,
    0x0132DBE5, 0x01FC49AC, 0x010D0FC9, 0x0091CCBF, 0x01F3C273,
    0x0168D13D, 0x00DF37DD, 0x00D14100, 0x01155C7F, 0x01112841,
    0x009FDD5D, 0x0186FB5D, 0x01F2EA5D, 0x01DADD41, 0x0121157F,
};

}  // namespace AboutQr
