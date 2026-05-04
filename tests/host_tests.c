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
    uint8_t frame_num;
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
    capture->frame_num++;
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
    pico_kiss_proto_decoder_set_flags(&decoder, STRICT_ECAPE_SEQUENCES);
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






// 1. Basic single frame
// Input
// C0 00 01 02 03 C0
// Expected output
// command = 0x00
// data    = {0x01, 0x02, 0x03}
// length  = 3
static void test_basic_single_frame(void) {
    decoder_capture_t capture = {0};
    pico_kiss_proto_decoder_t decoder = {0};
    pico_kiss_proto_decoder_init(&decoder, &capture, capture_start, capture_data, capture_end);

    const uint8_t frame[] = {0xC0, 0x00, 0x01, 0x02, 0x03, 0xC0};
    pico_kiss_proto_decoder_put_array(&decoder, (uint8_t *)frame, sizeof(frame));

    assert(capture.started == 1);
    assert(capture.ended == 1);
    assert(capture.status == PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE);
    assert(capture.frame_len == 4);

    assert(capture.frame_data[0] == 0x00);
    assert(capture.frame_data[1] == 0x01);
    assert(capture.frame_data[2] == 0x02);
    assert(capture.frame_data[3] == 0x03);
}

// 2. Empty frame (valid and important)
// Input
// C0 00 C0
// Expected
// command = 0x00
// data    = {}
// length  = 0
static void test_empty_frame(void) {
    decoder_capture_t capture = {0};
    pico_kiss_proto_decoder_t decoder = {0};
    pico_kiss_proto_decoder_init(&decoder, &capture, capture_start, capture_data, capture_end);

    const uint8_t frame[] = {0xC0, 0x00, 0xC0};
    pico_kiss_proto_decoder_put_array(&decoder, (uint8_t *)frame, sizeof(frame));

    assert(capture.started == 1);
    assert(capture.ended == 1);
    assert(capture.status == PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE);
    assert(capture.frame_len == 1);
    assert(capture.frame_data[0] == 0x00);
}

// 2.1. Empty frame (valid and important)
// Input
// C0 C0
// Expected
// command = 0x00
// data    = {}
// length  = 0
static void test_empty_frame_2(void) {
    decoder_capture_t capture = {0};
    pico_kiss_proto_decoder_t decoder = {0};
    pico_kiss_proto_decoder_init(&decoder, &capture, capture_start, capture_data, capture_end);

    const uint8_t frame[] = {0xC0, 0xC0};
    pico_kiss_proto_decoder_put_array(&decoder, (uint8_t *)frame, sizeof(frame));

    assert(capture.started == 1);
    assert(capture.ended == 1);
    assert(capture.status == PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE);
    assert(capture.frame_len == 0);
}

// 3. Multiple frames back-to-back
// Input
// C0 00 01 C0 C0 01 02 03 C0
// Expected
// Frame 1:
// cmd=0x00, data={0x01}
// Frame 2:
// cmd=0x01, data={0x02,0x03}
static void test_multiple_frames_capture_end(void *data, pico_kiss_proto_frame_info_t *info) {
    capture_end(data, info);
    decoder_capture_t *capture = data;

    assert(capture->started == 1);
    assert(capture->ended == 1);
    assert(capture->status == PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE);

    switch(capture->frame_num) {
        case 1:
            assert(capture->frame_len == 2);
            assert(capture->frame_data[0] == 0x00);
            assert(capture->frame_data[1] == 0x01);
            break;
        case 2:
            assert(capture->frame_len == 0);
            break;
        case 3:
            assert(capture->frame_len == 3);
            assert(capture->frame_data[0] == 0x01);
            assert(capture->frame_data[1] == 0x02);
            assert(capture->frame_data[2] == 0x03);
            break;
        default:
            assert(0 && "Unexpected frame number");
    }   
}
static void test_multiple_frames(void) {
    decoder_capture_t capture = {0};
    pico_kiss_proto_decoder_t decoder = {0};
    pico_kiss_proto_decoder_init(
        &decoder, 
        &capture, 
        capture_start, 
        capture_data, 
        test_multiple_frames_capture_end
    );

    const uint8_t frame[] = {0xC0, 0x00, 0x01, 0xC0, 0xC0, 0x01, 0x02, 0x03, 0xC0};
    pico_kiss_proto_decoder_put_array(&decoder, (uint8_t *)frame, sizeof(frame));

    assert(capture.frame_num == 3);
}

// 4. Escaped FEND inside payload
// Input
// C0 00 DB DC C0
// Expected
// cmd=0x00
// data={0xC0}
static void test_escaped_fend(void) {
    decoder_capture_t capture = {0};
    pico_kiss_proto_decoder_t decoder = {0};
    pico_kiss_proto_decoder_init(&decoder, &capture, capture_start, capture_data, capture_end);

    const uint8_t frame[] = {0xC0, 0x00, 0xDB, 0xDC, 0xC0};
    pico_kiss_proto_decoder_put_array(&decoder, (uint8_t *)frame, sizeof(frame));

    assert(capture.started == 1);
    assert(capture.ended == 1);
    assert(capture.status == PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE);
    assert(capture.frame_len == 2);
    assert(capture.frame_data[0] == 0x00);
    assert(capture.frame_data[1] == 0xC0);
}

// 5. Escaped FESC inside payload
// Input
// C0 00 DB DD C0
// Expected
// cmd=0x00
// data={0xDB}
static void test_escaped_fesc(void) {
    decoder_capture_t capture = {0};
    pico_kiss_proto_decoder_t decoder = {0};
    pico_kiss_proto_decoder_init(&decoder, &capture, capture_start, capture_data, capture_end);

    const uint8_t frame[] = {0xC0, 0x00, 0xDB, 0xDD, 0xC0};
    pico_kiss_proto_decoder_put_array(&decoder, (uint8_t *)frame, sizeof(frame));

    assert(capture.started == 1);
    assert(capture.ended == 1);
    assert(capture.status == PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE);
    assert(capture.frame_len == 2);
    assert(capture.frame_data[0] == 0x00);
    assert(capture.frame_data[1] == 0xDB);
}

// 6. Mixed escaping
// Input
// C0 00 11 DB DC 22 DB DD 33 C0
// Expected
// cmd=0x00
// data={0x11, 0xC0, 0x22, 0xDB, 0x33}
static void test_mixed_escaping(void) {
    decoder_capture_t capture = {0};
    pico_kiss_proto_decoder_t decoder = {0};
    pico_kiss_proto_decoder_init(&decoder, &capture, capture_start, capture_data, capture_end);

    const uint8_t frame[] = {0xC0, 0x00, 0x11, 0xDB, 0xDC, 0x22, 0xDB, 0xDD, 0x33, 0xC0};
    pico_kiss_proto_decoder_put_array(&decoder, (uint8_t *)frame, sizeof(frame));

    assert(capture.started == 1);
    assert(capture.ended == 1);
    assert(capture.status == PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE);
    assert(capture.frame_len == 6);
    assert(capture.frame_data[0] == 0x00);
    assert(capture.frame_data[1] == 0x11);
    assert(capture.frame_data[2] == 0xC0);
    assert(capture.frame_data[3] == 0x22);
    assert(capture.frame_data[4] == 0xDB);
    assert(capture.frame_data[5] == 0x33);
}

// 7. Split escape sequence across buffers (critical)
// Feed in two chunks:
// Chunk 1
// C0 00 11 DB
// Chunk 2
// DC 22 C0
// Expected
// cmd=0x00
// data={0x11, 0xC0, 0x22}
static void test_across_buffer_escaping(void) {
    decoder_capture_t capture = {0};
    pico_kiss_proto_decoder_t decoder = {0};
    pico_kiss_proto_decoder_init(&decoder, &capture, capture_start, capture_data, capture_end);

    const uint8_t frame1[] = {0xC0, 0x00, 0x11, 0xDB};
    pico_kiss_proto_decoder_put_array(&decoder, (uint8_t *)frame1, sizeof(frame1));
    const uint8_t frame2[] = {0xDC, 0x22, 0xC0};
    pico_kiss_proto_decoder_put_array(&decoder, (uint8_t *)frame2, sizeof(frame2));


    assert(capture.started == 1);
    assert(capture.ended == 1);
    assert(capture.status == PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE);
    assert(capture.frame_len == 4);
    assert(capture.frame_data[0] == 0x00);
    assert(capture.frame_data[1] == 0x11);
    assert(capture.frame_data[2] == 0xC0);
    assert(capture.frame_data[3] == 0x22);
}

// 8. Invalid escape sequence (robustness test)
// Input
// C0 00 DB 99 C0
// Expected (choose a policy, but be consistent)
// Recommended: pass through literally
// data={0xDB, 0x99}
static void test_invalid_escape(void) {
    decoder_capture_t capture = {0};
    pico_kiss_proto_decoder_t decoder = {0};
    pico_kiss_proto_decoder_init(&decoder, &capture, capture_start, capture_data, capture_end);

    const uint8_t frame[] = {0xC0, 0x00, 0xDB, 0x99, 0xC0};
    pico_kiss_proto_decoder_put_array(&decoder, (uint8_t *)frame, sizeof(frame));

        fprintf(stderr, "L Started: %d, Ended: %d, Status: %d, Frame Len: %ld, Frame Num: %d\n", 
            capture.started, capture.ended, capture.status, capture.frame_len, capture.frame_num);

    assert(capture.started == 1);
    assert(capture.ended == 1);
    assert(capture.status == PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE);
    assert(capture.frame_len == 3);
    assert(capture.frame_data[0] == 0x00);
    assert(capture.frame_data[1] == 0xDB);
    assert(capture.frame_data[2] == 0x99);
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

    run_test("test_basic_single_frame", test_basic_single_frame);
    run_test("test_empty_frame", test_empty_frame);
    run_test("test_empty_frame_2", test_empty_frame_2);
    run_test("test_multiple_frames", test_multiple_frames);
    run_test("test_escaped_fend", test_escaped_fend);
    run_test("test_escaped_fesc", test_escaped_fesc);
    run_test("test_mixed_escaping", test_mixed_escaping);
    run_test("test_across_buffer_escaping", test_across_buffer_escaping);
    run_test("test_invalid_escape", test_invalid_escape);

    printf("All tests passed.\n");
    return 0;
}
