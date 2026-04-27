#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "pico-kiss-protocol.h"
#include "pico-kiss-protocol-codes.h"

typedef struct {
    uint8_t data[256];
    size_t len;
    uint8_t frame_data[256];
    size_t frame_len;
    int started;
    int ended;
    pico_kiss_proto_decoder_status_t status;
} decoder_capture_t;

typedef struct {
    uint8_t *buffer;
    size_t len;
} encoder_capture_t;

static void capture_start(void *data) {
    decoder_capture_t *capture = data;
    capture->started = 1;
    capture->len = 0;
}

static void capture_data(void *data, uint8_t byte, uint32_t byte_index) {
    decoder_capture_t *capture = data;
    assert(byte_index == capture->len);
    capture->data[capture->len++] = byte;
}

static void capture_end(void *data, pico_kiss_proto_frame_info_t *info) {
    decoder_capture_t *capture = data;
    capture->ended = 1;
    capture->status = info->status;
    capture->frame_len = capture->len;
    memcpy(capture->frame_data, capture->data, capture->frame_len);
}

static void add_encoded_byte(void *data, uint8_t byte) {
    encoder_capture_t *capture = data;
    capture->buffer[capture->len++] = byte;
}

static void assert_equal_bytes(const uint8_t *actual, const uint8_t *expected, size_t len) {
    if (memcmp(actual, expected, len) != 0) {
        fprintf(stderr, "Byte arrays differ:\n");
        fprintf(stderr, "  expected:");
        for (size_t i = 0; i < len; i++) {
            fprintf(stderr, " %02X", expected[i]);
        }
        fprintf(stderr, "\n  actual:  ");
        for (size_t i = 0; i < len; i++) {
            fprintf(stderr, " %02X", actual[i]);
        }
        fprintf(stderr, "\n");
        assert(0);
    }
}

static void test_encoder_simple(void) {
    uint8_t actual[16];
    encoder_capture_t capture = { .buffer = actual, .len = 0 };

    pico_kiss_proto_encoder_t encoder;
    pico_kiss_proto_encoder_init(&encoder, &capture, add_encoded_byte);

    const uint8_t payload[] = {0x01, 0x02};
    pico_kiss_proto_encoder_put_frame(&encoder, (uint8_t *)payload, sizeof(payload));

    const uint8_t expected[] = {PICO_KISS_PROTO_FEND, 0x01, 0x02, PICO_KISS_PROTO_FEND};
    assert(capture.len == sizeof(expected));
    assert_equal_bytes(actual, expected, sizeof(expected));
}

static void test_encoder_escape(void) {
    uint8_t actual[32];
    encoder_capture_t capture = { .buffer = actual, .len = 0 };

    pico_kiss_proto_encoder_t encoder;
    pico_kiss_proto_encoder_init(&encoder, &capture, add_encoded_byte);

    const uint8_t payload[] = {PICO_KISS_PROTO_FEND, PICO_KISS_PROTO_FESC, 0xAA};
    pico_kiss_proto_encoder_put_frame(&encoder, (uint8_t *)payload, sizeof(payload));

    const uint8_t expected[] = {
        PICO_KISS_PROTO_FEND,
        PICO_KISS_PROTO_FESC, PICO_KISS_PROTO_TFEND,
        PICO_KISS_PROTO_FESC, PICO_KISS_PROTO_TFESC,
        0xAA,
        PICO_KISS_PROTO_FEND
    };
    assert(capture.len == sizeof(expected));
    assert_equal_bytes(actual, expected, sizeof(expected));
}

static void test_decoder_simple(void) {
    decoder_capture_t capture = {0};
    pico_kiss_proto_decoder_t decoder = {0};
    pico_kiss_proto_decoder_init(&decoder, &capture, capture_start, capture_data, capture_end);

    const uint8_t frame[] = {PICO_KISS_PROTO_FEND, 0x01, 0x02, PICO_KISS_PROTO_FEND};
    pico_kiss_proto_decoder_put_array(&decoder, (uint8_t *)frame, sizeof(frame));

    assert(capture.started == 1);
    assert(capture.ended == 1);
    assert(capture.status == PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE);
    assert(capture.frame_len == 2);
    assert(capture.frame_data[0] == 0x01);
    assert(capture.frame_data[1] == 0x02);
}

static void test_decoder_invalid_escape(void) {
    decoder_capture_t capture = {0};
    pico_kiss_proto_decoder_t decoder = {0};
    pico_kiss_proto_decoder_init(&decoder, &capture, capture_start, capture_data, capture_end);

    const uint8_t frame[] = {PICO_KISS_PROTO_FEND, PICO_KISS_PROTO_FESC, 0x00};
    pico_kiss_proto_decoder_put_array(&decoder, (uint8_t *)frame, sizeof(frame));

    assert(capture.started == 1);
    assert(capture.ended == 1);
    assert(capture.status == PICO_KISS_PROTO_DECODER_STATUS_INVALID_ESCAPE_SEQUENCE);
    assert(capture.frame_len == 0);
}

static void test_roundtrip(void) {
    uint8_t actual[64];
    encoder_capture_t capture = { .buffer = actual, .len = 0 };

    pico_kiss_proto_encoder_t encoder;
    pico_kiss_proto_encoder_init(&encoder, &capture, add_encoded_byte);

    const uint8_t payload[] = {0x10, PICO_KISS_PROTO_FESC, 0x20, PICO_KISS_PROTO_FEND, 0x30};
    pico_kiss_proto_encoder_put_frame(&encoder, (uint8_t *)payload, sizeof(payload));

    decoder_capture_t decode_capture = {0};
    pico_kiss_proto_decoder_t decoder = {0};
    pico_kiss_proto_decoder_init(&decoder, &decode_capture, capture_start, capture_data, capture_end);
    pico_kiss_proto_decoder_put_array(&decoder, actual, capture.len);

    assert(decode_capture.started == 1);
    assert(decode_capture.ended == 1);
    assert(decode_capture.status == PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE);
    assert(decode_capture.frame_len == sizeof(payload));
    assert_equal_bytes(decode_capture.frame_data, payload, sizeof(payload));
}

static void run_test(const char *name, void (*fn)(void)) {
    printf("[ RUN ] %s\n", name);
    fn();
    printf("[ PASS ] %s\n", name);
}

int main(void) {
    run_test("encoder_simple", test_encoder_simple);
    run_test("encoder_escape", test_encoder_escape);
    run_test("decoder_simple", test_decoder_simple);
    run_test("decoder_invalid_escape", test_decoder_invalid_escape);
    run_test("roundtrip", test_roundtrip);

    printf("All tests passed.\n");
    return 0;
}
