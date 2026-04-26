#include "pico-kiss-protocol.h"

const char *pico_kiss_proto_decoder_status_to_string(pico_kiss_proto_decoder_status_t status) {
    switch (status) {
        case PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE:
            return "Frame Complete";
        case PICO_KISS_PROTO_DECODER_STATUS_INVALID_ESCAPE_SEQUENCE:
            return "Invalid Escape Sequence";
        default:
            return "Unknown Status";
    }
}

void pico_kiss_proto_decoder_init(
    pico_kiss_proto_decoder_t* decoder,
    void *data,
    pico_kiss_proto_decoder_start_cb_t start_cb,
    pico_kiss_proto_decoder_data_cb_t data_cb,
    pico_kiss_proto_decoder_end_cb_t end_cb
) {
    decoder->state = PICO_KISS_PROTO_DECODER_STATE_WAITING_FOR_FEND;
    decoder->escape_next_byte = 0;
    decoder->data = data;
    decoder->start_cb = start_cb;
    decoder->data_cb = data_cb;
    decoder->end_cb = end_cb;
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
        else {
            // Invalid escape sequence, reset decoder
            // End of frame, call frame callback
            if (decoder->end_cb) {
                decoder->end_cb(decoder->data, PICO_KISS_PROTO_DECODER_STATUS_INVALID_ESCAPE_SEQUENCE);
            }
            decoder->state = PICO_KISS_PROTO_DECODER_STATE_WAITING_FOR_FEND;
            decoder->escape_next_byte = 0;
            return;
        }
        decoder->escape_next_byte = 0;
    }
    else if (byte == PICO_KISS_PROTO_FESC) {
        decoder->escape_next_byte = 1;
        return;
    }
    else if (byte == PICO_KISS_PROTO_FEND) {
        decoder->escape_next_byte = 0;

        if (decoder->state == PICO_KISS_PROTO_DECODER_STATE_WAITING_FOR_FEND) {
            // Start of frame, next byte will be command
            decoder->state = PICO_KISS_PROTO_DECODER_STATE_RECEIVING_DATA;
            if (decoder->start_cb) {
                decoder->start_cb(decoder->data);
            }
        }
        else {
            // End of frame, call frame callback
            if (decoder->end_cb) {
                decoder->end_cb(decoder->data, PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE);
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

void pico_kiss_proto_encoder_init(
    pico_kiss_proto_encoder_t* encoder,
    void *data,
    pico_kiss_proto_encoder_cb_t byte_cb
) {
    encoder->data = data;
    encoder->byte_cb = byte_cb;
}

void pico_kiss_proto_encoder_put(
    pico_kiss_proto_encoder_t* encoder,
    uint8_t byte
) {
    if (byte == PICO_KISS_PROTO_FEND) {
        encoder->byte_cb(encoder->data, PICO_KISS_PROTO_FESC);
        encoder->byte_cb(encoder->data, PICO_KISS_PROTO_TFEND);
    } 
    else if (byte == PICO_KISS_PROTO_FESC) {
        encoder->byte_cb(encoder->data, PICO_KISS_PROTO_FESC);
        encoder->byte_cb(encoder->data, PICO_KISS_PROTO_TFESC);
    }
    else {
        encoder->byte_cb(encoder->data, byte);
    }
}
