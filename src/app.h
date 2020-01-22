#ifndef APP_H
#define APP_H

#include "cursor.h"
#include <stddef.h>
#include <stdint.h>
#include <vector>

struct ReceiveBuf {
  public:
    static const size_t LEN = 8192;
    static_assert(LEN % 2 == 0);

    ReceiveBuf() :
        front(Cursor(this->bytes, LEN / 2)),
        back(Cursor(this->bytes + LEN / 2, LEN / 2)),
        is_front_ready(false),
        is_back_ready(false) {}

    uint8_t bytes[LEN];
    Cursor front;
    Cursor back;
    bool is_front_ready;
    bool is_back_ready;
};

void run(const std::vector<ReceiveBuf*>& bufs);

#endif
