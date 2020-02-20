#ifndef APP_H
#define APP_H

#include "cursor.h"
#include "main.h"

#include <deque>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#include <FreeRTOS.h>
#include <semphr.h>

const uint32_t TEXT_COLOR = 0xff000000;
const uint32_t BACKGROUND_COLOR = 0xffffffff;
const uint32_t BUTTON_COLOR = 0xffcccccc;
const uint32_t BUTTON_HOVER_COLOR = 0xff7a7a7a;
const uint32_t BUTTON_PRESSED_COLOR = 0xff999999;
const uint32_t SCROLLBAR_COLOR = 0x00000000;
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

const int MARGIN = 4;
const int TITLE_BAR_HEIGHT = 44;
const int BUTTON_WIDTH = 80;
const int BUTTON_HEIGHT = 36;

const size_t MAX_NUM_LOG_ENTRIES = 50;

struct ReceiveBuf {
  public:
    static const size_t LEN = 8192;
    static_assert(LEN % 2 == 0);

    ReceiveBuf() :
        front(Cursor(this->bytes, LEN / 2)),
        back(Cursor(this->bytes + LEN / 2, LEN / 2)),
        is_front_ready(false),
        is_back_ready(true) {
        this->back.set_empty();
    }

    volatile uint8_t bytes[LEN];
    Cursor front;
    Cursor back;
    volatile bool is_front_ready;
    volatile bool is_back_ready;
};

template <typename T>
class Mutex {
  public:
    Mutex() {
        this->mutex = xSemaphoreCreateMutex();

        if (!this->mutex) {
            on_error();
        }
    }

    Mutex(const Mutex&) = delete;

    Mutex(Mutex&&) = delete;

    ~Mutex() {
        vSemaphoreDelete(this->mutex);
    }

    Mutex& operator=(const Mutex&) = delete;

    Mutex& operator=(Mutex&&) = delete;

    T& lock() {
        xSemaphoreTake(this->mutex, portMAX_DELAY);
        return this->obj;
    }

    const T& lock() const {
        xSemaphoreTake(this->mutex, portMAX_DELAY);
        return this->obj;
    }

    void unlock() const {
        xSemaphoreGive(this->mutex);
    }

  private:
    T obj;
    SemaphoreHandle_t mutex;
};

class Log {
  public:
    Log();

    static std::string fmt_tick(uint32_t tick);

    void error(std::string message);

    void buf_processing_time(uint32_t time);

    void time_between_buf_processing(uint32_t time);

    size_t size() const;

    std::deque<std::shared_ptr<std::string>>::const_iterator begin() const;

    std::deque<std::shared_ptr<std::string>>::const_iterator end() const;

    uint32_t max_buf_processing_time() const;

    float avg_buf_processing_time() const;

    uint32_t max_time_between_buf_processing() const;

    float avg_time_between_buf_processing() const;

  private:
    void push_message(std::string&& message);

    std::deque<std::shared_ptr<std::string>> messages;
    uint32_t max_buf_processing_time_;
    uint32_t buf_processing_time_sum;
    uint32_t num_processed_bufs;
    uint32_t max_time_between_buf_processing_;
    uint32_t time_between_buf_processing_sum;
    uint32_t num_times_between_buf_processing;
};

void run(const std::vector<ReceiveBuf*>& bufs);

#endif
