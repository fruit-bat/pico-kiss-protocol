// SPDX-License-Identifier: MIT
// Copyright (c) 2026 fruit-bat
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/**
 * Decoder status values returned by the frame parser.
 *
 * Note: FRAME_INCOMPLETE is not used by the current implementation, but is
 * provided for completeness and future use.
 */
typedef enum {
    PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE = 0,
    PICO_KISS_PROTO_DECODER_STATUS_INVALID_ESCAPE_SEQUENCE = 1,
    PICO_KISS_PROTO_DECODER_STATUS_FRAME_INCOMPLETE = 2,
    PICO_KISS_PROTO_DECODER_STATUS_CB_FRAME_ERROR = 3,
} pico_kiss_proto_decoder_status_t;

/**
 * Return value for the data callback.
 *
 * PICO_KISS_PROTO_DECODER_DATA_CB_STATUS_OK indicates that decoding should
 * continue normally. PICO_KISS_PROTO_DECODER_DATA_CB_STATUS_FRAME_ERROR tells
 * the decoder to abort the current frame immediately.
 */
typedef enum {
    PICO_KISS_PROTO_DECODER_DATA_CB_STATUS_OK = 0,
    PICO_KISS_PROTO_DECODER_DATA_CB_STATUS_FRAME_ERROR = 1,
} pico_kiss_proto_decoder_data_cb_status_t;

/**
 * Convert a decoder status into a readable string.
 */
const char *pico_kiss_proto_decoder_status_to_string(pico_kiss_proto_decoder_status_t status);

/**
 * Information passed to the end-of-frame callback.
 *
 * - len: length of the decoded frame payload in bytes
 * - status: final decoder status for the frame
 */
typedef struct {
    uint32_t len;
    pico_kiss_proto_decoder_status_t status;
} pico_kiss_proto_frame_info_t;

/**
 * Callback invoked for each decoded payload byte.
 *
 * Parameters:
 * - data: user-provided context pointer
 * - byte: decoded payload byte
 * - byte_index: zero-based index within the current frame payload
 *
 * Return:
 * - PICO_KISS_PROTO_DECODER_DATA_CB_STATUS_OK to continue decoding
 * - PICO_KISS_PROTO_DECODER_DATA_CB_STATUS_FRAME_ERROR to abort the current frame
 */
typedef pico_kiss_proto_decoder_data_cb_status_t (*pico_kiss_proto_decoder_data_cb_t)(
    void * data,
    uint8_t byte,
    uint32_t byte_index);

/**
 * Callback invoked when a new frame begins.
 *
 * The decoder calls this after receiving the initial FEND delimiter and before
 * any payload bytes are delivered.
 */
typedef void (*pico_kiss_proto_decoder_start_cb_t)(
    void * data);

/**
 * Callback invoked when a frame ends.
 *
 * The decoder calls this after a complete frame has been parsed, or when a
 * strict escape error causes the frame to abort.
 */
typedef void (*pico_kiss_proto_decoder_end_cb_t)(
    void * data,
    pico_kiss_proto_frame_info_t* status);

/**
 * Callback invoked when a byte-level decode error occurs.
 *
 * Parameters:
 * - data: user-provided context pointer
 * - status: decoder error code
 * - index: index of the byte that triggered the error within the current frame
 */
typedef void (*pico_kiss_proto_decoder_error_cb_t)(
    void * data,
    pico_kiss_proto_decoder_status_t status,
    uint32_t index);

/**
 * Decoder option flags.
 */
typedef enum {
    /**
     * Enforce strict escape handling.
     *
     * If an invalid escape sequence is seen, the decoder aborts the current
     * frame and reports the error through the end callback.
     */
    STRICT_ESCAPE_SEQUENCES = (1 << 0)
} pico_kiss_proto_decoder_flags_t;

/**
 * Decoder state object.
 *
 * This object must be initialized with pico_kiss_proto_decoder_init() before
 * use and may be reused for multiple frames.
 */
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

/**
 * Initialize a decoder instance.
 *
 * Parameters:
 * - decoder: decoder state object to initialize
 * - data: user-provided context pointer forwarded to callbacks
 * - start_cb: frame-start callback, may be NULL
 * - data_cb: payload byte callback, may be NULL
 * - end_cb: frame-end callback, may be NULL
 * - error_cb: error callback, may be NULL
 */
void pico_kiss_proto_decoder_init(
    pico_kiss_proto_decoder_t* decoder,
    void *data,
    pico_kiss_proto_decoder_start_cb_t start_cb,
    pico_kiss_proto_decoder_data_cb_t data_cb,
    pico_kiss_proto_decoder_end_cb_t end_cb,
    pico_kiss_proto_decoder_error_cb_t error_cb
);

/**
 * Configure decoder flags.
 */
void pico_kiss_proto_decoder_set_flags(
    pico_kiss_proto_decoder_t* decoder,
    pico_kiss_proto_decoder_flags_t flags
);

/**
 * Feed a single raw byte into the decoder.
 *
 * The decoder processes KISS delimiters and escape sequences, delivering
 * decoded payload bytes through the data callback.
 */
void pico_kiss_proto_decoder_put(
    pico_kiss_proto_decoder_t* decoder,
    uint8_t byte
);

/**
 * Feed an array of raw bytes into the decoder.
 */
void pico_kiss_proto_decoder_put_array(
    pico_kiss_proto_decoder_t* decoder,
    uint8_t* bytes,
    size_t len
);

/**
 * Byte-level callback used by the encoder.
 */
typedef void (*pico_kiss_proto_encoder_cb_t)(void * data, uint8_t byte);

/**
 * Encoder state object.
 */
typedef struct {
    void *data;
    pico_kiss_proto_encoder_cb_t byte_cb;
} pico_kiss_proto_encoder_t;

/**
 * Initialize an encoder instance.
 *
 * Parameters:
 * - encoder: encoder state object to initialize
 * - data: user-provided context pointer forwarded to byte_cb
 * - byte_cb: callback invoked for each encoded byte
 */
void pico_kiss_proto_encoder_init(
    pico_kiss_proto_encoder_t* encoder,
    void *data,
    pico_kiss_proto_encoder_cb_t byte_cb
);

/**
 * Write the frame start delimiter (FEND) to the output stream.
 */
void pico_kiss_proto_encoder_start(
    pico_kiss_proto_encoder_t* encoder
);

/**
 * Encode a single payload byte.
 *
 * The encoder escapes FEND and FESC bytes according to KISS rules.
 */
void pico_kiss_proto_encoder_put(
    pico_kiss_proto_encoder_t* encoder,
    uint8_t byte
);

/**
 * Write the frame end delimiter (FEND) to the output stream.
 */
void pico_kiss_proto_encoder_end(
    pico_kiss_proto_encoder_t* encoder
);

/**
 * Encode an array of payload bytes.
 */
void pico_kiss_proto_encoder_put_array(
    pico_kiss_proto_encoder_t* encoder,
    uint8_t* bytes,
    size_t len
);

/**
 * Encode a complete frame: start delimiter, escaped payload, then end delimiter.
 */
void pico_kiss_proto_encoder_put_frame(
    pico_kiss_proto_encoder_t* encoder,
    uint8_t* bytes,
    size_t len
);

#ifdef __cplusplus
}
#endif
 