// SPDX-License-Identifier: MIT
// Copyright (c) 2026 fruit-bat
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*
 * If an invalid escape sequence is encountered:
 *
 * - the pending escape byte (FESC) is emitted literally
 * - the invalid byte is reprocessed normally
 * - if the invalid byte is FEND, it terminates the frame
 * - an error callback is generated
 *
 * This guarantees stream resynchronisation.
 */

typedef enum {
    PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE = 0,
    PICO_KISS_PROTO_DECODER_STATUS_INVALID_ESCAPE_SEQUENCE = 1,
    PICO_KISS_PROTO_DECODER_STATUS_FRAME_INCOMPLETE = 2,
    PICO_KISS_PROTO_DECODER_STATUS_CB_FRAME_ERROR = 3,
} pico_kiss_proto_decoder_status_t;

typedef enum {
    // If the data callback returns this status, it indicates that the decoder should continue as normal.
    PICO_KISS_PROTO_DECODER_DATA_CB_STATUS_OK = 0,
    // If the data callback returns this status, it indicates that the decoder should treat this as an error condition and abort the frame.
    PICO_KISS_PROTO_DECODER_DATA_CB_STATUS_FRAME_ERROR = 1,
} pico_kiss_proto_decoder_data_cb_status_t;

const char *pico_kiss_proto_decoder_status_to_string(pico_kiss_proto_decoder_status_t status);

typedef struct {
    uint32_t len;
    pico_kiss_proto_decoder_status_t status;
} pico_kiss_proto_frame_info_t;

typedef pico_kiss_proto_decoder_data_cb_status_t (*pico_kiss_proto_decoder_data_cb_t)(
    void * data, 
    uint8_t byte, 
    uint32_t byte_index);

typedef void (*pico_kiss_proto_decoder_start_cb_t)(
    void * data);

typedef void (*pico_kiss_proto_decoder_end_cb_t)(
    void * data, 
    pico_kiss_proto_frame_info_t* status);

typedef void (*pico_kiss_proto_decoder_error_cb_t)(
    void * data, 
    pico_kiss_proto_decoder_status_t status,
    uint32_t index);

typedef enum {    
    STRICT_ESCAPE_SEQUENCES = (1 << 0)
} pico_kiss_proto_decoder_flags_t;  

typedef struct {
    uint8_t state;
    uint8_t escape_next_byte;
    uint32_t data_len;
    pico_kiss_proto_decoder_flags_t flags;
    void *data;
    pico_kiss_proto_decoder_start_cb_t start_cb;
    pico_kiss_proto_decoder_data_cb_t data_cb;
    pico_kiss_proto_decoder_end_cb_t end_cb;
    pico_kiss_proto_decoder_error_cb_t error_cb;
} pico_kiss_proto_decoder_t;

void pico_kiss_proto_decoder_init(
    pico_kiss_proto_decoder_t* decoder,
    void *data,
    pico_kiss_proto_decoder_start_cb_t start_cb,
    pico_kiss_proto_decoder_data_cb_t data_cb,
    pico_kiss_proto_decoder_end_cb_t end_cb,
    pico_kiss_proto_decoder_error_cb_t error_cb
);

void pico_kiss_proto_decoder_set_flags(
    pico_kiss_proto_decoder_t* decoder,
    pico_kiss_proto_decoder_flags_t flags
);

void pico_kiss_proto_decoder_put(
    pico_kiss_proto_decoder_t* decoder,
    uint8_t byte
);

void pico_kiss_proto_decoder_put_array(
    pico_kiss_proto_decoder_t* decoder,
    uint8_t* bytes,
    size_t len
);

typedef void (*pico_kiss_proto_encoder_cb_t)(void * data, uint8_t byte);

typedef struct {
    void *data;
    pico_kiss_proto_encoder_cb_t byte_cb;
} pico_kiss_proto_encoder_t;

void pico_kiss_proto_encoder_init(
    pico_kiss_proto_encoder_t* encoder,
    void *data,
    pico_kiss_proto_encoder_cb_t byte_cb
);

void pico_kiss_proto_encoder_start(
    pico_kiss_proto_encoder_t* encoder
);

void pico_kiss_proto_encoder_put(
    pico_kiss_proto_encoder_t* encoder,
    uint8_t byte
);

void pico_kiss_proto_encoder_end(
    pico_kiss_proto_encoder_t* encoder
);

void pico_kiss_proto_encoder_put_array(
    pico_kiss_proto_encoder_t* encoder,
    uint8_t* bytes,
    size_t len
);

void pico_kiss_proto_encoder_put_frame(
    pico_kiss_proto_encoder_t* encoder,
    uint8_t* bytes,
    size_t len
);

#ifdef __cplusplus
}
#endif  
 