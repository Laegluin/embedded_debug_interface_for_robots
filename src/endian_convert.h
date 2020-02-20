#ifndef ENDIAN_CONVERT_H
#define ENDIAN_CONVERT_H

#include <stdint.h>
#include <string.h>

inline uint16_t uint16_from_le(const uint8_t bytes[2]) {
    uint16_t val;
    memcpy(&val, bytes, 2);
    return val;
}

inline uint32_t uint32_from_le(const uint8_t bytes[4]) {
    uint32_t val;
    memcpy(&val, bytes, 4);
    return val;
}

inline float float32_from_le(const uint8_t bytes[4]) {
    float val;
    memcpy(&val, bytes, 4);
    return val;
}

inline void uint16_to_le(uint8_t dst[2], uint16_t val) {
    memcpy(dst, &val, 2);
}

inline void uint32_to_le(uint8_t dst[4], uint32_t val) {
    memcpy(dst, &val, 4);
}

inline void float32_to_le(uint8_t dst[4], float val) {
    memcpy(dst, &val, 4);
}

#endif
