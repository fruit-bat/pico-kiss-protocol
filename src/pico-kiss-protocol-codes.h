// SPDX-License-Identifier: MIT
// Copyright (c) 2026 fruit-bat
#pragma once

#include <stdint.h>

// KISS Protocol defines
#define PICO_KISS_PROTO_BYTE(X) ((uint8_t)X)
// Frame End
#define PICO_KISS_PROTO_FEND PICO_KISS_PROTO_BYTE(0xC0)
// Frame Escape
#define PICO_KISS_PROTO_FESC PICO_KISS_PROTO_BYTE(0xDB)
// Transposed Frame End
#define PICO_KISS_PROTO_TFEND PICO_KISS_PROTO_BYTE(0xDC)
// Transposed Frame Escape
#define PICO_KISS_PROTO_TFESC PICO_KISS_PROTO_BYTE(0xDD)
