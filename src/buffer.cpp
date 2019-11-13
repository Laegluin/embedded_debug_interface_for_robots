#include "buffer.h"
#include <algorithm>

size_t Cursor::read(uint8_t* dst, size_t num_bytes) {
    auto bytes_read = std::min(num_bytes, this->remaining_bytes());
    memcpy(dst, this->buf + this->current_pos, bytes_read);
    this->current_pos += bytes_read;
    return bytes_read;
}
