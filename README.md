# Kiss radio protocol utilities
Just some help encoding and decoding kiss frames

This repository is part of a suite:
* [pico-serial-proxy](https://github.com/fruit-bat/pico-serial-proxy)
* [pico-kiss-protocol](https://github.com/fruit-bat/pico-kiss-protocol)
* [pico-rnode-protocol](https://github.com/fruit-bat/pico-rnode-protocol)

## Terminology
| Term | Meaning | Comment |
| -----| --------| --------| 
| KISS | Keep It Simple Stupid | *sigh* |
| TNC  | Terminal Node Controller | |

# Useful codes
| Hex value	| Abbreviation	| Description |
| --------- | ------------- | ----------- | 
| 0xC0	| FEND	| Frame End |
| 0xDB	| FESC	| Frame Escape |
| 0xDC	| TFEND	| Transposed Frame End |
| 0xDD	| TFESC	| Transposed Frame Escape |

## References
https://en.wikipedia.org/wiki/KISS_(amateur_radio_protocol)<br/>
https://en.wikipedia.org/wiki/Terminal_node_controller<br/>

## Host unit tests
A simple host test target is available under `tests/`.

Build and run from the repository root:

```bash
cd tests
cmake -S . -B build
cmake --build build && ctest --verbose --test-dir build
```

This compiles `pico-kiss-protocol` as a normal host executable and runs the unit tests without Pico hardware.

## Monitor application
The `apps/monitor` target builds a PTY proxy that sits between a real serial TTY device and a virtual pseudo-terminal.

The monitor app opens the real device you provide, configures it for raw serial I/O, creates a virtual PTY, and forwards bytes in both directions. It also decodes KISS frames on both the incoming and outgoing streams and prints them to the console.

Build and run:
```bash
cmake -S apps/monitor -B apps/monitor/build
cmake --build apps/monitor/build --target monitor
./apps/monitor/build/monitor /dev/ttyUSB0 9600
```

The program prints the virtual PTY path when it starts. Connect your KISS-capable application to that virtual device instead of the physical serial port.

Example output:
```text
Proxy active.
Real Device:   /dev/ttyUSB0
Virtual TTY:   /dev/pts/42
Connect your application to the Virtual TTY.
```

The monitor app uses the instance-based `pico_serial_proxy_t` API, so the same process can manage multiple proxy instances if needed.

Example dual-proxy setup:
```c
pico_serial_proxy_t proxy_a;
pico_serial_proxy_t proxy_b;

pico_serial_proxy_init(&proxy_a, "/dev/ttyUSB0", 9600, ctx_a, data_cb_a, lifecycle_cb_a);
pico_serial_proxy_init(&proxy_b, "/dev/ttyUSB1", 9600, ctx_b, data_cb_b, lifecycle_cb_b);
```

This makes it easy to run two serial-to-PTY proxies side by side in one application.

This makes it easy to inspect or log KISS traffic without changing the real device connection.

See `apps/monitor/README.md` for more detailed documentation, build steps, and the updated instance-based `pico_serial_proxy_t` API example.
