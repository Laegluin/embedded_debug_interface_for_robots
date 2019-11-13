#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <exception>
#include <stdint.h>
#include <string.h>

class OutOfRange : public std::exception {
  public:
    OutOfRange() {}

    const char* what() const noexcept override {
        return "index out of range";
    }
};

template <size_t CAPACITY>
class FixedByteVector {
  public:
    FixedByteVector() : size_(0) {}

    bool try_push(uint8_t byte) {
        return try_push_slice(&byte, 1);
    }

    bool try_push_slice(uint8_t* bytes, size_t len) {
        if (this->size() + len > CAPACITY) {
            return false;
        }

        memcpy(this->buf + this->size(), bytes, len);
        return true;
    }

    void clear() {
        this->size_ = 0;
    }

    const uint8_t& operator[](size_t idx) const {
        return this->buf[idx];
    }

    uint8_t* data() {
        return this->buf;
    }

    void shrink_by(size_t num_removed_bytes) {
        this->size_ -= std::min(this->size_, num_removed_bytes);
    }

    /// Extends the vector by `num_new_bytes`. This simply sets a new size but does not actually
    /// initialize the new storage. You must initialize the memory before using it.
    void extend_by(size_t num_new_bytes) {
        if (!this->try_extend_by(num_new_bytes)) {
            throw OutOfRange();
        }
    }

    /// Like `extend_by` but returns false instead of throwing.
    bool try_extend_by(size_t num_new_bytes) {
        if (this->size() + num_new_bytes > CAPACITY) {
            return false;
        }

        this->size_ += num_new_bytes;
        return true;
    }

    size_t size() const {
        return this->size_;
    }

  private:
    uint8_t buf[CAPACITY];
    size_t size_;
};

class Cursor {
  public:
    Cursor(const uint8_t* buf, size_t buf_len) : buf(buf), buf_len(buf_len), current_pos(0) {}

    /// Reads a maximum of `num_bytes` into `dst` and returns the number of bytes read. If the
    /// number of read bytes is less than `num_bytes`, the buffer backing this `Cursor` is exhausted
    /// and consecutive reads will yield no more data.
    size_t read(uint8_t* dst, size_t num_bytes);

    size_t remaining_bytes() const {
        return this->buf_len - this->current_pos;
    }

  private:
    const uint8_t* buf;
    size_t buf_len;
    size_t current_pos;
};

#endif
