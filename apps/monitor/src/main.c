// SPDX-License-Identifier: MIT
// Copyright (c) 2026 fruit-bat
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "pico-kiss-protocol.h"
#include "pico-kiss-protocol-codes.h"

#define BUFFER_SIZE 1024
#define MAX_FRAME_BYTES 2048

static int baud_rate_to_speed(int baud, speed_t *speed) {
    switch (baud) {
        case 0: *speed = B0; return 0;
        case 50: *speed = B50; return 0;
        case 75: *speed = B75; return 0;
        case 110: *speed = B110; return 0;
        case 134: *speed = B134; return 0;
        case 150: *speed = B150; return 0;
        case 200: *speed = B200; return 0;
        case 300: *speed = B300; return 0;
        case 600: *speed = B600; return 0;
        case 1200: *speed = B1200; return 0;
        case 1800: *speed = B1800; return 0;
        case 2400: *speed = B2400; return 0;
        case 4800: *speed = B4800; return 0;
        case 9600: *speed = B9600; return 0;
        case 19200: *speed = B19200; return 0;
        case 38400: *speed = B38400; return 0;
        case 57600: *speed = B57600; return 0;
        case 115200: *speed = B115200; return 0;
        case 230400: *speed = B230400; return 0;
        default: return -1;
    }
}

static int configure_serial(int fd, int baud_rate) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        return -1;
    }

    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_oflag &= ~OPOST;
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_cflag &= ~(CSIZE | PARENB);
    tty.c_cflag |= CS8;

    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    if (baud_rate > 0) {
        speed_t speed;
        if (baud_rate_to_speed(baud_rate, &speed) != 0) {
            errno = EINVAL;
            return -1;
        }
        cfsetispeed(&tty, speed);
        cfsetospeed(&tty, speed);
    }

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        return -1;
    }

    return 0;
}

struct monitor_context {
    const char *direction;
    uint8_t frame[MAX_FRAME_BYTES];
    size_t frame_len;
};

static void print_hex_frame(const char *prefix, const uint8_t *data, size_t len) {
    printf("%s [len=%zu]:", prefix, len);
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <real_tty_device> [baud_rate]\n", argv[0]);
        return 1;
    }

    const char *real_tty_path = argv[1];
    int baud_rate = 0;
    if (argc >= 3) {
        baud_rate = atoi(argv[2]);
    }

    int real_fd = open(real_tty_path, O_RDWR | O_NOCTTY);
    if (real_fd < 0) {
        perror("Error opening real TTY device");
        return 1;
    }

    if (configure_serial(real_fd, baud_rate) != 0) {
        perror("Error configuring serial port");
        close(real_fd);
        return 1;
    }

    int master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd < 0) {
        perror("Error creating PTY master");
        close(real_fd);
        return 1;
    }

    if (grantpt(master_fd) < 0 || unlockpt(master_fd) < 0) {
        perror("Error setting up PTY permissions");
        close(real_fd);
        close(master_fd);
        return 1;
    }

    char *slave_name = ptsname(master_fd);
    if (slave_name == NULL) {
        perror("Error getting slave PTY name");
        close(real_fd);
        close(master_fd);
        return 1;
    }

    printf("Proxy active.\n");
    printf("Real Device:   %s\n", real_tty_path);
    printf("Virtual TTY:   %s\n", slave_name);
    printf("Connect your application to the Virtual TTY.\n\n");

    struct monitor_context incoming_ctx = {
        .direction = "IN ",
        .frame_len = 0,
    };
    struct monitor_context outgoing_ctx = {
        .direction = "OUT",
        .frame_len = 0,
    };

    pico_kiss_proto_decoder_t incoming_decoder;
    pico_kiss_proto_decoder_t outgoing_decoder;
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

    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    int max_fd = (real_fd > master_fd) ? real_fd : master_fd;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(real_fd, &read_fds);
        FD_SET(master_fd, &read_fds);

        int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("Select error");
            break;
        }

        if (FD_ISSET(real_fd, &read_fds)) {
            ssize_t bytes_read = read(real_fd, buffer, sizeof(buffer));
            if (bytes_read <= 0) {
                if (bytes_read < 0) perror("Read error from real device");
                break;
            }
            for (ssize_t i = 0; i < bytes_read; i++) {
                pico_kiss_proto_decoder_put(&incoming_decoder, (uint8_t)buffer[i]);
            }
            if (write_all(master_fd, buffer, (size_t)bytes_read) < 0) {
                perror("Write error to virtual PTY");
                break;
            }
        }

        if (FD_ISSET(master_fd, &read_fds)) {
            ssize_t bytes_read = read(master_fd, buffer, sizeof(buffer));
            if (bytes_read <= 0) {
                if (bytes_read < 0) perror("Read error from virtual PTY");
                break;
            }
            for (ssize_t i = 0; i < bytes_read; i++) {
                pico_kiss_proto_decoder_put(&outgoing_decoder, (uint8_t)buffer[i]);
            }
            if (write_all(real_fd, buffer, (size_t)bytes_read) < 0) {
                perror("Write error to real device");
                break;
            }
        }
    }

    close(real_fd);
    close(master_fd);
    return 0;
}
