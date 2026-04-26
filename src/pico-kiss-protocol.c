#include "pico-kiss-protocol.h"

void pico_kiss_proto_decoder_init(
    pico_kiss_proto_decoder_t* decoder,
    void *data,
    pico_kiss_proto_decoder_data_cb_t data_cb,
    pico_kiss_proto_decoder_frame_cb_t frame_cb
) {
    decoder->state = PICO_KISS_PROTO_DECODER_STATE_WAITING_FOR_FEND;
    decoder->escape_next_byte = 0;
    decoder->data = data;
    decoder->data_cb = data_cb;
    decoder->frame_cb = frame_cb;
}

void pico_kiss_proto_decoder_put(
    pico_kiss_proto_decoder_t* decoder,
    uint8_t byte
) {
    if (decoder->escape_next_byte) {
        if (byte == PICO_KISS_PROTO_TFEND) {
            byte = PICO_KISS_PROTO_FEND;
        } 
        else if (byte == PICO_KISS_PROTO_TFESC) {
            byte = PICO_KISS_PROTO_FESC;
        }
        decoder->escape_next_byte = 0;
    } 
    else if (byte == PICO_KISS_PROTO_FESC) {
        decoder->escape_next_byte = 1;
        return;
    }
    else if (byte == PICO_KISS_PROTO_FEND) {
        if (decoder->state == PICO_KISS_PROTO_DECODER_STATE_WAITING_FOR_FEND) {
            // Start of frame, next byte will be command
            decoder->state = PICO_KISS_PROTO_DECODER_STATE_RECEIVING_DATA;
            if (decoder->frame_cb) {
                decoder->frame_cb(decoder->data);
            }
        }
        return;
    }

    if (decoder->state == PICO_KISS_PROTO_DECODER_STATE_WAITING_FOR_FEND) {
        // Ignore bytes until we get a FEND
        return;
    }

    // We have a data byte, pass it to the callback
    if (decoder->data_cb) {
        decoder->data_cb(decoder->data, byte);
    }
}

void pico_kiss_proto_decoder_put_array(
    pico_kiss_proto_decoder_t* decoder,
    uint8_t* byte,
    size_t len
) {
    for (size_t i = 0; i < len; i++) {
        pico_kiss_proto_decoder_put(decoder, byte[i]);
    }
}

