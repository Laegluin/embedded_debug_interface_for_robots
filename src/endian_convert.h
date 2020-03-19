#ifndef ENDIAN_CONVERT_H
#define ENDIAN_CONVERT_H

#include <stdint.h>
#include <string.h>

/// Converts `bytes` into a 16-bit unsigned integer. The bytes are interpreted as
/// little-endian.
inline uint16_t uint16_from_le(const uint8_t bytes[2]) {
    uint16_t val;
    memcpy(&val, bytes, 2);
    return val;
}

/// Converts `bytes` into a 32-bit unsigned integer. The bytes are interpreted as
/// little-endian.
inline uint32_t uint32_from_le(const uint8_t bytes[4]) {
    uint32_t val;
    memcpy(&val, bytes, 4);
    return val;
}

/// Converts `bytes` into a single precision floating point number. The bytes are
/// interpreted as little-endian.
inline float float32_from_le(const uint8_t bytes[4]) {
    float val;
    memcpy(&val, bytes, 4);
    return val;
}

/// Converts the 16-bit unsigned integer `val` into bytes with little-endian byte order.
/// The bytes are written to `dst`.
inline void uint16_to_le(uint8_t dst[2], uint16_t val) {
    memcpy(dst, &val, 2);
}

/// Converts the 32-bit unsigned integer `val` into bytes with little-endian byte order.
/// The bytes are written to `dst`.
inline void uint32_to_le(uint8_t dst[4], uint32_t val) {
    memcpy(dst, &val, 4);
}

/// Converts the single precision floating point number `val` into bytes with little-endian
/// byte order. The bytes are written to `dst`.
inline void float32_to_le(uint8_t dst[4], float val) {
    memcpy(dst, &val, 4);
}

#endif
