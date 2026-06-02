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
 *
 * @brief Return a human-readable, NUL-terminated string describing a
 *        {@link pico_kiss_proto_decoder_status_t} value. Useful for
 *        logging and diagnostics.
 *
 * @param status Decoder status value to stringify.
 * @return Pointer to a constant string describing the status.
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
 * @brief Prepare a {@link pico_kiss_proto_decoder_t} for decoding KISS
 *        frames. This zero/sets up internal state and registers the user
 *        callback pointers and context.
 *
 * @param decoder Pointer to the decoder state object to initialize. Must
 *                point to a valid, writable object.
 * @param data User-provided opaque context pointer which will be forwarded
 *             to callbacks registered below (may be NULL).
 * @param start_cb Optional callback invoked when a new frame starts (may
 *                 be NULL).
 * @param data_cb Optional callback invoked for each decoded payload byte
 *                (may be NULL).
 * @param end_cb Optional callback invoked when a frame ends (may be NULL).
 * @param error_cb Optional callback invoked on byte-level decode errors
 *                 (may be NULL).
 * @return This function returns no value.
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
 *
 * @brief Set behaviour flags for an initialized decoder. Flags control
 *        runtime options such as strict escape handling.
 *
 * @param decoder Pointer to an initialized {@link pico_kiss_proto_decoder_t}.
 * @param flags Bitwise-or of {@link pico_kiss_proto_decoder_flags_t} values.
 * @return This function returns no value.
 */
void pico_kiss_proto_decoder_set_flags(
    pico_kiss_proto_decoder_t* decoder,
    pico_kiss_proto_decoder_flags_t flags
);

/**
 * Feed a single raw byte into the decoder.
 *
 * @brief Process one raw input byte. The decoder will handle frame
 *        delimiters and escape sequences and will invoke the registered
 *        callbacks as payload bytes are decoded.
 *
 * @param decoder Pointer to an initialized decoder instance.
 * @param byte Raw input byte to process.
 * @return This function returns no value.
 */
void pico_kiss_proto_decoder_put(
    pico_kiss_proto_decoder_t* decoder,
    uint8_t byte
);

/**
 * Feed an array of raw bytes into the decoder.
 *
 * @brief Convenience helper that feeds `len` bytes from `bytes` into the
 *        decoder by repeatedly calling {@link pico_kiss_proto_decoder_put}.
 *
 * @param decoder Pointer to an initialized decoder instance.
 * @param bytes Pointer to the input byte array (may be NULL if len is 0).
 * @param len Number of bytes in the `bytes` array.
 * @return This function returns no value.
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
 * @brief Prepare a {@link pico_kiss_proto_encoder_t} for encoding KISS
 *        frames. Registers the output callback and user context.
 *
 * @param encoder Pointer to the encoder state object to initialize.
 * @param data User-provided opaque context pointer forwarded to `byte_cb`.
 * @param byte_cb Callback invoked for each encoded output byte (must not
 *                be NULL for normal operation).
 * @return This function returns no value.
 */
void pico_kiss_proto_encoder_init(
    pico_kiss_proto_encoder_t* encoder,
    void *data,
    pico_kiss_proto_encoder_cb_t byte_cb
);

/**
 * Write the frame start delimiter (FEND) to the output stream.
 *
 * @brief Emit the KISS frame start byte. This must be called before
 *        encoding payload bytes for a new frame.
 *
 * @param encoder Pointer to an initialized encoder instance.
 * @return This function returns no value.
 */
void pico_kiss_proto_encoder_start(
    pico_kiss_proto_encoder_t* encoder
);

/**
 * Encode a single payload byte.
 *
 * @brief Encode `byte` and emit one or more bytes via the encoder's
 *        `byte_cb`. Special bytes (FEND/FESC) are escaped per KISS rules.
 *
 * @param encoder Pointer to an initialized encoder instance.
 * @param byte Payload byte to encode.
 * @return This function returns no value.
 */
void pico_kiss_proto_encoder_put(
    pico_kiss_proto_encoder_t* encoder,
    uint8_t byte
);

/**
 * Write the frame end delimiter (FEND) to the output stream.
 *
 * @brief Emit the KISS frame end byte. Call after all payload bytes have
 *        been encoded to terminate the frame.
 *
 * @param encoder Pointer to an initialized encoder instance.
 * @return This function returns no value.
 */
void pico_kiss_proto_encoder_end(
    pico_kiss_proto_encoder_t* encoder
);

/**
 * Encode an array of payload bytes.
 *
 * @brief Convenience helper that encodes `len` payload bytes from `bytes`.
 *        Each byte is processed through {@link pico_kiss_proto_encoder_put}.
 *
 * @param encoder Pointer to an initialized encoder instance.
 * @param bytes Pointer to the payload byte array (may be NULL if len is 0).
 * @param len Number of payload bytes to encode.
 * @return This function returns no value.
 */
void pico_kiss_proto_encoder_put_array(
    pico_kiss_proto_encoder_t* encoder,
    uint8_t* bytes,
    size_t len
);

/**
 * Encode a complete frame: start delimiter, escaped payload, then end delimiter.
 *
 * @brief Convenience function that emits a start delimiter, encodes the
 *        supplied payload bytes (escaping as required), and then emits the
 *        end delimiter.
 *
 * @param encoder Pointer to an initialized encoder instance.
 * @param bytes Pointer to the payload byte array (may be NULL if len is 0).
 * @param len Number of payload bytes to encode.
 * @return This function returns no value.
 */
void pico_kiss_proto_encoder_put_frame(
    pico_kiss_proto_encoder_t* encoder,
    uint8_t* bytes,
    size_t len
);

#ifdef __cplusplus
}
#endif
 