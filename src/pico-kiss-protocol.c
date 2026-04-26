#include "pico-kiss-protocol.h"

void pico_kiss_proto_decoder_init(
    pico_kiss_proto_decoder_t* decoder,
    void *data,
    pico_kiss_proto_decoder_byte_cb_t byte_cb,
    pico_kiss_proto_decoder_frame_cb_t frame_cb
) {
    decoder->state = PICO_KISS_PROTO_DECODER_STATE_WAITING_FOR_FEND;
    decoder->escape_next_byte = 0;
    decoder->data = data;
    decoder->byte_cb = byte_cb;
    decoder->frame_cb = frame_cb;
}

