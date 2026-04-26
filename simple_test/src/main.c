#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/clocks.h"
#include "pico_kiss_protocol.h"
#include "pico/stdio_uart.h"

/**
 * Read joypads every 40ms and send output to the serial port
 */
int main() {
  stdio_uart_init();
  printf("Startup\n");

  printf("Waiting...\n");

  while(true) {

  }
}
