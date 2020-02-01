#ifndef APP_H
#define APP_H

#include "cursor.h"
#include <stddef.h>
#include <stdint.h>
#include <vector>

const uint32_t TEXT_COLOR = 0xff000000;
const uint32_t BACKGROUND_COLOR = 0xffffffff;
const uint32_t BUTTON_COLOR = 0xffcccccc;
const uint32_t BUTTON_HOVER_COLOR = 0xff7a7a7a;
const uint32_t BUTTON_PRESSED_COLOR = 0xff999999;
const uint32_t SCROLLBAR_COLOR = 0xff000000;
const uint32_t SCROLLBAR_THUMB_COLOR = 0xff858585;
const uint32_t MENU_COLOR = 0xffe6e6e6;
const uint32_t MENU_HOVER_COLOR = 0xffcfcfcf;
const uint32_t MENU_PRESSED_COLOR = 0xffb8b8b8;
const uint32_t LIST_HEADER_COLOR = BACKGROUND_COLOR;
const uint32_t LIST_ITEM_COLOR = BACKGROUND_COLOR;
const uint32_t LIST_ITEM_ALT_COLOR = 0xfff2f2f2;

const uint32_t DEVICE_STATUS_TEXT_COLOR = 0xffffffff;
const uint32_t DEVICE_CONNECTED_COLOR = 0xff3ad30b;
const uint32_t DEVICE_DISCONNECTED_COLOR = 0xffed2525;

struct ReceiveBuf {
  public:
    static const size_t LEN = 8192;
    static_assert(LEN % 2 == 0);

    ReceiveBuf() :
        front(Cursor(this->bytes, LEN / 2)),
        back(Cursor(this->bytes + LEN / 2, LEN / 2)),
        is_front_ready(false),
        is_back_ready(false) {}

    volatile uint8_t bytes[LEN];
    Cursor front;
    Cursor back;
    volatile bool is_front_ready;
    volatile bool is_back_ready;
};

void run(const std::vector<ReceiveBuf*>& bufs);

#endif
