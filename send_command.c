#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <unistd.h>

void flash(const char* serial_path, const char* image_path);
void write_stuffed(int fd, const uint8_t* buf, size_t len, uint8_t* last_byte);
void start(const char* serial_path);

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "error: missing command\n");
        exit(1);
    }

    if (!strcmp(argv[1], "flash")) {
        if (argc < 4) {
            fprintf(stderr, "error: missing serial device or image path\n");
            exit(1);
        }

        flash(argv[2], argv[3]);
    } else if (!strcmp(argv[1], "start")) {
        if (argc < 3) {
            fprintf(stderr, "error: missing serial device\n");
            exit(1);
        }

        start(argv[2]);
    } else {
        fprintf(stderr, "error: unknown command `%s`\n", argv[1]);
        exit(1);
    }

    exit(0);
}

void flash(const char* serial_path, const char* image_path) {
    int image_fd = open(image_path, O_RDONLY);
    if (image_fd == -1) {
        fprintf(stderr, "error: cannot open `%s`\n", image_path);
        exit(1);
    }

    int serial_fd = open(serial_path, O_RDWR);
    if (serial_fd == -1) {
        fprintf(stderr, "error: cannot open `%s`\n", serial_path);
        exit(1);
    }

    if (ioctl(serial_fd, TCFLSH, TCIFLUSH) == -1) {
        fprintf(stderr, "error: cannot flush receive buffer for `%s`\n", serial_path);
        exit(1);
    }

    struct stat info;
    if (fstat(image_fd, &info) == -1) {
        fprintf(stderr, "error: cannot stat `%s`\n", image_path);
        exit(1);
    }

    uint8_t command[] = {0x00, 0xff, 0x00, 0x00};

    if (write(serial_fd, command, sizeof(command)) == -1) {
        fprintf(stderr, "error: cannot write command to `%s`\n", serial_path);
        exit(1);
    }

    uint32_t st_size = info.st_size;
    uint8_t len[4];
    memcpy(len, &st_size, 4);

    uint8_t last_byte = 0x00;
    write_stuffed(serial_fd, len, 4, &last_byte);

    while (1) {
        uint8_t buf[4096];
        ssize_t bytes_read = read(image_fd, buf, sizeof(buf));

        if (bytes_read == -1) {
            fprintf(stderr, "error: failed read from `%s`", image_path);
            exit(1);
        } else if (bytes_read == 0) {
            break;
        }

        write_stuffed(serial_fd, buf, bytes_read, &last_byte);
    }
}

void write_stuffed(int fd, const uint8_t* buf, size_t len, uint8_t* last_byte) {
    uint8_t* stuffed_buf = malloc(len * 2);
    size_t stuffed_len = 0;

    for (size_t i = 0; i < len; i++) {
        uint8_t byte = buf[i];

        if (*last_byte != 0xff && byte == 0xff) {
            stuffed_buf[stuffed_len] = byte;
            stuffed_buf[stuffed_len + 1] = byte;
            stuffed_len += 2;
        } else {
            stuffed_buf[stuffed_len] = byte;
            stuffed_len++;
        }

        *last_byte = byte;
    }

    if (write(fd, stuffed_buf, stuffed_len) == -1) {
        fprintf(stderr, "error: cannot write to serial device");
        exit(1);
    }

    free(stuffed_buf);
}

void start(const char* serial_path) {
    int serial_fd = open(serial_path, O_RDWR);
    if (serial_fd == -1) {
        fprintf(stderr, "error: cannot open `%s`\n", serial_path);
        exit(1);
    }

    if (ioctl(serial_fd, TCFLSH, TCIFLUSH) == -1) {
        fprintf(stderr, "error: cannot flush receive buffer for `%s`\n", serial_path);
        exit(1);
    }

    uint8_t command[] = {0x00, 0xff, 0x00, 0x01};
    if (write(serial_fd, command, sizeof(command)) == -1) {
        fprintf(stderr, "error: cannot write command to `%s`\n", serial_path);
        exit(1);
    }
}
