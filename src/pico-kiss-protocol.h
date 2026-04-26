#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

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

#define PICO_KISS_PROTO_DECODER_STATE_WAITING_FOR_FEND  PICO_KISS_PROTO_BYTE(0)
#define PICO_KISS_PROTO_DECODER_STATE_RECEIVING_DATA    PICO_KISS_PROTO_BYTE(1)

typedef void (*pico_kiss_proto_decoder_data_cb_t)(void * data, uint8_t byte);
typedef void (*pico_kiss_proto_decoder_frame_cb_t)(void * data);

typedef struct {
    uint8_t state;
    uint8_t escape_next_byte;
    void *data;
    pico_kiss_proto_decoder_data_cb_t data_cb;
    pico_kiss_proto_decoder_frame_cb_t frame_cb;
} pico_kiss_proto_decoder_t;

void pico_kiss_proto_decoder_init(
    pico_kiss_proto_decoder_t* decoder,
    void *data,
    pico_kiss_proto_decoder_data_cb_t data_cb,
    pico_kiss_proto_decoder_frame_cb_t frame_cb
);

void pico_kiss_proto_decoder_put(
    pico_kiss_proto_decoder_t* decoder,
    uint8_t byte
);

void pico_kiss_proto_decoder_put_array(
    pico_kiss_proto_decoder_t* decoder,
    uint8_t* byte,
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

#ifdef __cplusplus
}
#endif  
