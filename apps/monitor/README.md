# pico-kiss-protocol Monitor App

This monitor app demonstrates using `pico-serial-proxy` together with `pico-kiss-protocol`.

The app opens a real serial device, creates a virtual PTY, and prints KISS frames forwarded in both directions.

## Build

From the app directory:

```sh
cd apps/monitor
mkdir -p build
cd build
cmake ..
cmake --build .
```

This CMake target also builds the sibling `pico-serial-proxy` library from `../../../pico-serial-proxy`.

## Run

```sh
./monitor /dev/ttyUSB0 115200
```

- `/dev/ttyUSB0` should be replaced with the physical serial device path.
- `115200` is the baud rate for the real device.

After startup, the app prints the virtual PTY path. Connect your application to that virtual TTY to forward data through the proxy.

### Command line options

The monitor app also supports recording and replay modes:

```sh
./monitor --help
```

- `--help` shows usage information.
- `--record <file>` records captured traffic to a file.
- `--replay <file>` replays a saved recording through the KISS decoder.

Example:

```sh
./monitor /dev/ttyUSB0 115200 --record capture.rec
./monitor --replay capture.rec
```

## Instance-based proxy API

The app now uses the instance-based proxy API from `pico-serial-proxy`.

That means each proxy stores its state in a `pico_serial_proxy_t` object, so multiple proxies can coexist in one process.

Example usage:

```c
pico_serial_proxy_t proxy;

if (pico_serial_proxy_init(&proxy,
                           real_tty_path,
                           baud_rate,
                           cb_context,
                           proxy_data_cb,
                           proxy_lifecycle_cb) != 0) {
    perror("Error initializing serial proxy");
    return 1;
}

const char *virt_path = pico_serial_proxy_get_virt_tty(&proxy);
printf("Virtual TTY: %s\n", virt_path);

pico_serial_proxy_run(&proxy);
```

## Parallel proxy example

Because the state is stored in `pico_serial_proxy_t`, you can open two proxies side by side:

```c
pico_serial_proxy_t proxy_a;
pico_serial_proxy_t proxy_b;

pico_serial_proxy_init(&proxy_a, "/dev/ttyUSB0", 115200, ctx_a, data_cb_a, lifecycle_cb_a);
pico_serial_proxy_init(&proxy_b, "/dev/ttyUSB1", 115200, ctx_b, data_cb_b, lifecycle_cb_b);
```

Each proxy object is independent, with its own real device, virtual PTY, and callbacks.

## Notes

- The monitor app uses callbacks to decode KISS frames from both directions.
- Signal handling requests the proxy to stop cleanly.
- The app currently supports a single proxy instance in its main example, but the API supports multiple instances.
