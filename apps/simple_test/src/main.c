#include <ctype.h>
#include <stdio.h>
#include "pico-kiss-protocol.h"


void stdout_decoder_data_cb(void * data, uint8_t byte) {
    printf("Data: 0x%02X", byte);
    if (isprint(byte)) {
        printf(" '%c'", byte);
    }
    printf("\n");
}
void stdout_decoder_frame_cb(void * data) {
    printf("Frame received\n");
}

void test_decoder_1() {
    pico_kiss_proto_decoder_t decoder;
    pico_kiss_proto_decoder_init(&decoder, NULL, stdout_decoder_data_cb, stdout_decoder_frame_cb);

    uint8_t test_frame[] = {PICO_KISS_PROTO_FEND, 0x01, 0x02, PICO_KISS_PROTO_FEND};
    pico_kiss_proto_decoder_put_array(&decoder, test_frame, sizeof(test_frame));
}

// Send the bytes 0xC0, 0xDB out of TNC port 0
// C0 - FEND	00 - DATA FRAME: port 0	DB - FESC	DC - TFEND	DB - FESC	DD - TFESC	C0 - FEND
void test_decoder_2() {
    pico_kiss_proto_decoder_t decoder;
    pico_kiss_proto_decoder_init(&decoder, NULL, stdout_decoder_data_cb, stdout_decoder_frame_cb);

    uint8_t test_frame[] = {PICO_KISS_PROTO_FEND, 0x00, PICO_KISS_PROTO_FESC, PICO_KISS_PROTO_TFEND, PICO_KISS_PROTO_FESC, PICO_KISS_PROTO_TFESC, PICO_KISS_PROTO_FEND};
    pico_kiss_proto_decoder_put_array(&decoder, test_frame, sizeof(test_frame));
}

// Send the characters "Hello" out of TNC port 5
// C0 - FEND	50 - DATA FRAME: port 5	48 - "H"	65 - "e"	6C - "l"	6C - "l"	6F -"o"	C0 - FEND
void test_decoder_3() {
    pico_kiss_proto_decoder_t decoder;
    pico_kiss_proto_decoder_init(&decoder, NULL, stdout_decoder_data_cb, stdout_decoder_frame_cb);

    uint8_t test_frame[] = {PICO_KISS_PROTO_FEND, 0x50, 0x48, 0x65, 0x6C, 0x6C, 0x6F, PICO_KISS_PROTO_FEND};
    pico_kiss_proto_decoder_put_array(&decoder, test_frame, sizeof(test_frame));
}

/**
 */
int main() {
  printf("Startup\n");
  test_decoder_1();
  test_decoder_2();
  test_decoder_3();
  printf("Done\n");
  return 0; 

}
