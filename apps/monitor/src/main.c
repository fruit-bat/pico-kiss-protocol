// SPDX-License-Identifier: MIT
// Copyright (c) 2026 fruit-bat

// Enable GNU miscellaneous extensions so cfmakeraw() is declared.
// cfmakeraw() is a glibc extension guarded by __USE_MISC.
// _XOPEN_SOURCE 700 is also required so POSIX/X/Open PTY helpers
// like posix_openpt(), grantpt(), unlockpt(), and ptsname() are exposed.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "pico-kiss-protocol.h"
#include "pico-kiss-protocol-codes.h"
#include "pico-serial-proxy.h"
#include "pico-serial-recording.h"

#define BUFFER_SIZE 1024
#define MAX_FRAME_BYTES 2048

// The serial configuration and PTY management is handled by
// the pico-serial-proxy library. The monitor app provides a
// callback to receive bytes forwarded in either direction.

struct monitor_context {
    const char *direction;
    uint8_t frame[MAX_FRAME_BYTES];
    size_t frame_len;
};

static void print_hex_frame(const char *prefix, const uint8_t *data, size_t len) {
    printf("%s [%zu]:", prefix, len);
    for (size_t i = 0; i < len; i++) {
        printf(" %02X", data[i]);
    }
    printf("\n");
}

static void decoder_start(void *data) {
    struct monitor_context *ctx = (struct monitor_context *)data;
    ctx->frame_len = 0;
}

static pico_kiss_proto_decoder_data_cb_status_t decoder_data(
    void *data,
    uint8_t byte,
    uint32_t byte_index
) {
    (void)byte_index;
    struct monitor_context *ctx = (struct monitor_context *)data;
    if (ctx->frame_len < sizeof(ctx->frame)) {
        ctx->frame[ctx->frame_len++] = byte;
    }
    return PICO_KISS_PROTO_DECODER_DATA_CB_STATUS_OK;
}

static void decoder_end(void *data, pico_kiss_proto_frame_info_t *info) {
    struct monitor_context *ctx = (struct monitor_context *)data;
    if (info->status == PICO_KISS_PROTO_DECODER_STATUS_FRAME_COMPLETE) {
        print_hex_frame(ctx->direction, ctx->frame, ctx->frame_len);
    } else {
        printf("%s [error=%s, len=%u]\n",
               ctx->direction,
               pico_kiss_proto_decoder_status_to_string(info->status),
               info->len);
    }
}

static void decoder_error(
    void *data,
    pico_kiss_proto_decoder_status_t status,
    uint32_t index
) {
    struct monitor_context *ctx = (struct monitor_context *)data;
    printf("%s decode error: %s at byte index %u\n",
           ctx->direction,
           pico_kiss_proto_decoder_status_to_string(status),
           index);
}

static ssize_t write_all(int fd, const void *buffer, size_t len) {
    const uint8_t *ptr = (const uint8_t *)buffer;
    size_t remaining = len;

    while (remaining > 0) {
        ssize_t written = write(fd, ptr, remaining);
        if (written <= 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        ptr += written;
        remaining -= (size_t)written;
    }
    return (ssize_t)len;
}

static pico_serial_proxy_t monitor_proxy;
static pico_serial_recording_writer_t monitor_recorder = { .fd = -1, .entry_count = 0 };
static int recording_enabled = 0;

static void proxy_signal_handler(int signum) {
    const char *signal_name = "unknown";
    switch (signum) {
        case SIGINT: signal_name = "SIGINT"; break;
        case SIGTERM: signal_name = "SIGTERM"; break;
        case SIGHUP: signal_name = "SIGHUP"; break;
    }
    fprintf(stderr, "Received %s, shutting down proxy...\n", signal_name);
    pico_serial_proxy_request_stop(&monitor_proxy);
}

static void print_help(const char *prog) {
    printf("Usage: %s [<real_tty_device>] [baud_rate] [--record <file>] [--replay <file>]\n", prog);
    printf("\nProxy a real serial device to a virtual PTY, record traffic, or replay a saved recording.\n");
    printf("\nOptions:\n");
    printf("  --record <file>  Write captured traffic to a recording file\n");
    printf("  --replay <file>  Replay a saved recording through the decoder\n");
    printf("  --help           Show this help text\n");
}

int main(int argc, char *argv[]) {
    if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        print_help(argv[0]);
        return 0;
    }

    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    const char *real_tty_path = NULL;
    int baud_rate = 0;
    const char *record_path = NULL;
    const char *replay_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--record") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing path for --record\n");
                return 1;
            }
            record_path = argv[++i];
        } else if (strcmp(argv[i], "--replay") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing path for --replay\n");
                return 1;
            }
            replay_path = argv[++i];
        } else if (real_tty_path == NULL && argv[i][0] != '-') {
            real_tty_path = argv[i];
        } else if (argv[i][0] >= '0' && argv[i][0] <= '9') {
            baud_rate = atoi(argv[i]);
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            return 1;
        }
    }

    if (replay_path != NULL && real_tty_path != NULL) {
        fprintf(stderr, "--replay cannot be combined with a real serial device path\n");
        return 1;
    }

    if (replay_path == NULL && real_tty_path == NULL) {
        fprintf(stderr, "Either a serial device path or --replay <file> is required\n");
        return 1;
    }

    static struct monitor_context incoming_ctx = {
        .direction = "H<-D",
        .frame_len = 0,
    };
    static struct monitor_context outgoing_ctx = {
        .direction = "H->D",
        .frame_len = 0,
    };

    static pico_kiss_proto_decoder_t incoming_decoder;
    static pico_kiss_proto_decoder_t outgoing_decoder;

    pico_kiss_proto_decoder_init(&incoming_decoder,
                                 &incoming_ctx,
                                 decoder_start,
                                 decoder_data,
                                 decoder_end,
                                 decoder_error);

    pico_kiss_proto_decoder_init(&outgoing_decoder,
                                 &outgoing_ctx,
                                 decoder_start,
                                 decoder_data,
                                 decoder_end,
                                 decoder_error);

    if (replay_path != NULL) {
        pico_serial_recording_reader_t replay_reader;
        uint8_t replay_buffer[4096];
        size_t replay_len = 0;
        pico_serial_recording_direction_t replay_direction;
        int replay_rc;

        if (pico_serial_recording_reader_open(&replay_reader, replay_path) != 0) {
            perror("Error opening replay file");
            return 1;
        }

        printf("[replay] reading from %s\n", replay_path);

        while ((replay_rc = pico_serial_recording_reader_read(&replay_reader,
                                                             &replay_direction,
                                                             replay_buffer,
                                                             sizeof(replay_buffer),
                                                             &replay_len)) == 0) {
            if (replay_direction == PICO_SERIAL_RECORDING_DIRECTION_HOST_TO_DEVICE) {
                for (size_t i = 0; i < replay_len; i++) {
                    pico_kiss_proto_decoder_put(&outgoing_decoder, replay_buffer[i]);
                }
            } else {
                for (size_t i = 0; i < replay_len; i++) {
                    pico_kiss_proto_decoder_put(&incoming_decoder, replay_buffer[i]);
                }
            }
        }

        if (replay_rc < 0) {
            fprintf(stderr, "Failed while replaying recording\n");
            pico_serial_recording_reader_close(&replay_reader);
            return 1;
        }

        (void)pico_serial_recording_reader_close(&replay_reader);
        return 0;
    }

    signal(SIGINT, proxy_signal_handler);
    signal(SIGTERM, proxy_signal_handler);
    signal(SIGHUP, proxy_signal_handler);

    // The pico-serial-proxy library will open and configure the real
    // device and create the virtual PTY; we just initialize it below.

    if (record_path != NULL) {
        if (pico_serial_recording_writer_open(&monitor_recorder, record_path) != 0) {
            perror("Error opening recording file");
            return 1;
        }
        recording_enabled = 1;
        printf("[recording] writing to %s\n", record_path);
    }

    // callback invoked by the proxy when bytes are forwarded; host_to_device
    // is true for data coming from the virtual PTY towards the real device.
    void proxy_data_cb(void *context, bool host_to_device, const uint8_t *data, size_t len) {
        (void)context;
        if (recording_enabled) {
            if (pico_serial_recording_writer_write(
                    &monitor_recorder,
                    host_to_device ? PICO_SERIAL_RECORDING_DIRECTION_HOST_TO_DEVICE : PICO_SERIAL_RECORDING_DIRECTION_DEVICE_TO_HOST,
                    data,
                    len) != 0) {
                fprintf(stderr, "Failed to write recording entry\n");
            }
        }

        if (host_to_device) {
            for (size_t i = 0; i < len; i++) {
                pico_kiss_proto_decoder_put(&outgoing_decoder, data[i]);
            }
        } else {
            for (size_t i = 0; i < len; i++) {
                pico_kiss_proto_decoder_put(&incoming_decoder, data[i]);
            }
        }
    }

    // lifecycle callback receives STARTED/ERROR/STOPPED events
    void proxy_lifecycle_cb(void *context, pico_serial_proxy_event_t event, const char *message) {
        (void)context;
        switch (event) {
            case PICO_SERIAL_PROXY_EVENT_STARTED:
                printf("[proxy] started, virt_tty=%s\n", message ? message : "(unknown)");
                break;
            case PICO_SERIAL_PROXY_EVENT_ERROR:
                printf("[proxy] error: %s\n", message ? message : "(no message)");
                break;
            case PICO_SERIAL_PROXY_EVENT_STOPPED:
                printf("[proxy] stopped: %s\n", message ? message : "");
                break;
        }
    }

    // Initialize and run the proxy library.
    if (pico_serial_proxy_init(&monitor_proxy,
                               real_tty_path,
                               baud_rate,
                               NULL,
                               proxy_data_cb,
                               proxy_lifecycle_cb) != 0) {
        perror("Error initializing serial proxy");
        return 1;
    }

    const char *virt_path = pico_serial_proxy_get_virt_tty(&monitor_proxy);
    if (virt_path) {
        printf("Proxy active.\n");
        printf("Real Device:   %s\n", real_tty_path);
        printf("Virtual TTY:   %s\n", virt_path);
        printf("Connect your application to the Virtual TTY.\n\n");
        printf("HOST<->DEV\n\n");
    }

    int rv = pico_serial_proxy_run(&monitor_proxy);
    (void)rv;

    if (recording_enabled) {
        (void)pico_serial_recording_writer_close(&monitor_recorder);
    }
    return 0;
}
