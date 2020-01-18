#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
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

    FixedByteVector(std::initializer_list<uint8_t> init) : size_(0) {
        this->push_slice(init.begin(), init.size());
    }

    bool try_push(uint8_t byte) {
        return try_push_slice(&byte, 1);
    }

    void push_slice(const uint8_t* bytes, size_t len) {
        if (!try_push_slice(bytes, len)) {
            throw OutOfRange();
        }
    }

    bool try_push_slice(const uint8_t* bytes, size_t len) {
        if (this->size() + len > CAPACITY) {
            return false;
        }

        memcpy(this->buf + this->size(), bytes, len);
        this->size_ += len;
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

    const uint8_t* data() const {
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

template <size_t LHS_CAP, size_t RHS_CAP>
inline bool operator==(const FixedByteVector<LHS_CAP>& lhs, const FixedByteVector<RHS_CAP>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (size_t i = 0; i < lhs.size(); i++) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }

    return true;
}

template <size_t CAPACITY>
inline std::ostream& operator<<(std::ostream& out, const FixedByteVector<CAPACITY>& vector) {
    out << "[";

    if (vector.size() > 0) {
        char str_buf[5];

        for (size_t i = 0; i < vector.size() - 1; i++) {
            sprintf(str_buf, "0x%02x", vector[i]);
            out << str_buf << ", ";
        }

        sprintf(str_buf, "0x%02x", vector[vector.size() - 1]);
        out << str_buf;
    }

    out << "]";
    return out;
}

class Cursor {
  public:
    Cursor(const uint8_t* buf, size_t buf_len) : buf(buf), buf_len(buf_len), current_pos(0) {}

    /// Reads a maximum of `num_bytes` into `dst` and returns the number of bytes read. If the
    /// number of read bytes is less than `num_bytes`, the buffer backing this `Cursor` is exhausted
    /// and consecutive reads will yield no more data.
    size_t read(uint8_t* dst, size_t num_bytes);

    void reset() {
        this->current_pos = 0;
    }

    size_t remaining_bytes() const {
        return this->buf_len - this->current_pos;
    }

  private:
    const uint8_t* buf;
    size_t buf_len;
    size_t current_pos;
};

#endif
