#ifndef CURSOR_H
#define CURSOR_H

#include <algorithm>
#include <stdint.h>
#include <string.h>

/// Wraps a buffer to allow tracking the number of bytes that have been read from it.
class Cursor {
  public:
    /// Creates a new `Cursor` that wraps `buf_len` bytes stored at `buf`.
    Cursor(const volatile uint8_t* buf, size_t buf_len) :
        buf(buf),
        buf_len(buf_len),
        current_pos(0) {}

    /// Reads a maximum of `num_bytes` into `dst` and returns the number of bytes read. If the
    /// number of read bytes is less than `num_bytes`, the buffer backing this `Cursor` is exhausted
    /// and consecutive reads will yield no more data.
    size_t read(uint8_t* dst, size_t num_bytes) {
        auto bytes_read = std::min(num_bytes, this->remaining_bytes());

        for (size_t i = 0; i < bytes_read; i++) {
            dst[i] = this->buf[this->current_pos + i];
        }

        this->current_pos += bytes_read;
        return bytes_read;
    }

    /// Resets the current position to 0 (start at the beginning again).
    void reset() {
        this->current_pos = 0;
    }

    /// Discards any bytes that have not been read yet by setting the position
    /// past the last byte.
    void set_empty() {
        this->current_pos = this->buf_len;
    }

    /// Returns the number of bytes remaining.
    size_t remaining_bytes() const {
        return this->buf_len - this->current_pos;
    }

  private:
    const volatile uint8_t* buf;
    size_t buf_len;
    size_t current_pos;
};

#endif
