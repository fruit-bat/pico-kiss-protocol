#include <ctype.h>
#include <stdio.h>
#include "pico-kiss-protocol.h"

// cc -Isrc src/pico-kiss-protocol.c apps/simple_test/src/main.c 

void stdout_decoder_data_cb(void * data, uint8_t byte) {
    printf("Data: 0x%02X", byte);
    if (isprint(byte)) {
        printf(" '%c'", byte);
    }
    printf("\n");
}

void stdout_decoder_start_cb(void * data) {
    printf("Start frame received\n");
}

void stdout_decoder_end_cb(void * data, pico_kiss_proto_decoder_status_t status) {
    printf("End frame received: %s\n", pico_kiss_proto_decoder_status_to_string(status));
}

void test_decoder_1() {
    pico_kiss_proto_decoder_t decoder;
    pico_kiss_proto_decoder_init(&decoder, NULL, stdout_decoder_start_cb, stdout_decoder_data_cb, stdout_decoder_end_cb);

    uint8_t test_frame[] = {PICO_KISS_PROTO_FEND, 0x01, 0x02, PICO_KISS_PROTO_FEND};
    pico_kiss_proto_decoder_put_array(&decoder, test_frame, sizeof(test_frame));
}

// Send the bytes 0xC0, 0xDB out of TNC port 0
// C0 - FEND	00 - DATA FRAME: port 0	DB - FESC	DC - TFEND	DB - FESC	DD - TFESC	C0 - FEND
void test_decoder_2() {
    pico_kiss_proto_decoder_t decoder;
    pico_kiss_proto_decoder_init(&decoder, NULL, stdout_decoder_start_cb, stdout_decoder_data_cb, stdout_decoder_end_cb);

    uint8_t test_frame[] = {
      PICO_KISS_PROTO_FEND, 
      0x00, 
      PICO_KISS_PROTO_FESC, PICO_KISS_PROTO_TFEND, 
      PICO_KISS_PROTO_FESC, PICO_KISS_PROTO_TFESC, 
      PICO_KISS_PROTO_FEND
    };
    pico_kiss_proto_decoder_put_array(&decoder, test_frame, sizeof(test_frame));
}

// Send the characters "Hello" out of TNC port 5
// C0 - FEND	50 - DATA FRAME: port 5	48 - "H"	65 - "e"	6C - "l"	6C - "l"	6F -"o"	C0 - FEND
void test_decoder_3() {
    pico_kiss_proto_decoder_t decoder;
    pico_kiss_proto_decoder_init(&decoder, NULL, stdout_decoder_start_cb, stdout_decoder_data_cb, stdout_decoder_end_cb);

    uint8_t test_frame[] = {
      PICO_KISS_PROTO_FEND, 
      0x50, 
      0x48, 0x65, 0x6C, 0x6C, 0x6F, 
      PICO_KISS_PROTO_FEND
    };
    pico_kiss_proto_decoder_put_array(&decoder, test_frame, sizeof(test_frame));
}

// Send the bytes 0xDB, 0xC0, 0xDB  
void test_decoder_4() {
    pico_kiss_proto_decoder_t decoder;
    pico_kiss_proto_decoder_init(&decoder, NULL, stdout_decoder_start_cb, stdout_decoder_data_cb, stdout_decoder_end_cb);

    uint8_t test_frame[] = {
      PICO_KISS_PROTO_FEND, 
      PICO_KISS_PROTO_FESC, PICO_KISS_PROTO_TFESC, 
      PICO_KISS_PROTO_FESC, PICO_KISS_PROTO_TFEND, 
      PICO_KISS_PROTO_FESC, PICO_KISS_PROTO_TFESC, 
      PICO_KISS_PROTO_FEND
    };
    pico_kiss_proto_decoder_put_array(&decoder, test_frame, sizeof(test_frame));
}

// Test send 0xC0 0x01 0x02 0xC0
void test_decoder_5() {
    pico_kiss_proto_decoder_t decoder;
    pico_kiss_proto_decoder_init(&decoder, NULL, stdout_decoder_start_cb, stdout_decoder_data_cb, stdout_decoder_end_cb);

    uint8_t test_frame[] = {PICO_KISS_PROTO_FEND, 0x01, 0x02, PICO_KISS_PROTO_FEND};
    pico_kiss_proto_decoder_put_array(&decoder, test_frame, sizeof(test_frame));
}

// Test send FESC FEND
void test_decoder_6() {
    pico_kiss_proto_decoder_t decoder;
    pico_kiss_proto_decoder_init(&decoder, NULL, stdout_decoder_start_cb, stdout_decoder_data_cb, stdout_decoder_end_cb);

    uint8_t test_frame[] = {PICO_KISS_PROTO_FEND, PICO_KISS_PROTO_FESC, PICO_KISS_PROTO_FEND};
    pico_kiss_proto_decoder_put_array(&decoder, test_frame, sizeof(test_frame));
    pico_kiss_proto_decoder_put_array(&decoder, test_frame, sizeof(test_frame));
}

/**
 */
int main() {
  printf("Startup\n");
  test_decoder_1();
  test_decoder_2();
  test_decoder_3();
  test_decoder_4();
  test_decoder_5();
  test_decoder_6();

  printf("Done\n");
  return 0; 

}
