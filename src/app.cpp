#include "app.h"
#include "control_table.h"
#include "main.h"
#include "parser.h"
#include "ui/run_ui.h"

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include <iomanip>
#include <sstream>

static SemaphoreHandle_t CONTROL_TABLE_MAP_MUTEX;
static SemaphoreHandle_t LOG_MUTEX;

void lock_control_table_map() {
    xSemaphoreTake(CONTROL_TABLE_MAP_MUTEX, portMAX_DELAY);
}

void release_control_table_map() {
    xSemaphoreGive(CONTROL_TABLE_MAP_MUTEX);
}

void lock_log() {
    xSemaphoreTake(LOG_MUTEX, portMAX_DELAY);
}

void release_log() {
    xSemaphoreGive(LOG_MUTEX);
}

struct Connection {
    Connection(ReceiveBuf* buf) :
        buf(buf),
        parser(Parser()),
        last_packet(Packet{
            DeviceId(0),
            Instruction::Ping,
            Error(),
            std::vector<uint8_t>(),
        }) {}

    ReceiveBuf* buf;
    Parser parser;
    Packet last_packet;
};

static void process_buffer(Log&, Connection&, ControlTableMap&);

void Log::error(std::string message) {
    auto now = HAL_GetTick();

    auto minutes = now / (60 * 1000);
    auto remaining_millis = now % (60 * 1000);
    auto seconds = remaining_millis / 1000;
    remaining_millis = remaining_millis % 1000;
    auto millis = remaining_millis;

    std::stringstream fmt;
    fmt << "[+" << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0')
        << std::setw(2) << seconds << "." << std::setfill('0') << std::setw(3) << std::left
        << millis << "] error: " << message;

    this->push_message(fmt.str());
}

void Log::buf_processing_time(uint32_t time) {
    this->max_buf_processing_time_ = std::max(time, this->max_buf_processing_time_);
    this->min_buf_processing_time_ = std::max(time, this->min_buf_processing_time_);
    this->buf_processing_time_sum += time;
    this->num_processed_bufs++;
}

void Log::time_between_buf_processing(uint32_t time) {
    this->max_time_between_buf_processing_ = std::max(time, this->max_time_between_buf_processing_);
}

size_t Log::size() const {
    return this->messages.size();
}

std::deque<std::string>::const_iterator Log::begin() const {
    return this->messages.begin();
}

std::deque<std::string>::const_iterator Log::end() const {
    return this->messages.end();
}

uint32_t Log::max_buf_processing_time() const {
    return this->max_buf_processing_time_;
}

float Log::avg_buf_processing_time() const {
    return (float) this->buf_processing_time_sum / (float) this->num_processed_bufs;
}

uint32_t Log::min_buf_processing_time() const {
    return this->min_buf_processing_time_;
}

uint32_t Log::max_time_between_buf_processing() const {
    return this->max_time_between_buf_processing_;
}

void Log::push_message(std::string message) {
    if (messages.size() >= MAX_NUM_LOG_ENTRIES) {
        this->messages.pop_back();
    }

    this->messages.push_front(std::move(message));
}

void run(const std::vector<ReceiveBuf*>& bufs) {
    CONTROL_TABLE_MAP_MUTEX = xSemaphoreCreateMutex();
    LOG_MUTEX = xSemaphoreCreateMutex();

    if (!CONTROL_TABLE_MAP_MUTEX || !LOG_MUTEX) {
        on_error();
    }

    Log log;
    ControlTableMap control_table_map;
    std::vector<Connection> connections;

    connections.reserve(bufs.size());

    for (auto buf : bufs) {
        connections.push_back(Connection(buf));
    }

    void* args[2] = {&log, &control_table_map};

    xTaskCreate(
        [](void* args) {
            auto log = (Log*) ((void**) args)[0];
            auto control_table_map = (ControlTableMap*) ((void**) args)[1];

            try {
                run_ui(*log, *control_table_map);
            } catch (...) {
                on_error();
            }
        },
        "ui",
        TASK_STACK_SIZE,
        args,
        0,
        nullptr);

    std::vector<uint32_t> last_buffer_starts(connections.size(), HAL_GetTick());

    while (true) {
        size_t connection_idx = 0;

        for (auto& connection : connections) {
            auto now = HAL_GetTick();
            lock_log();
            log.time_between_buf_processing(now - last_buffer_starts[connection_idx]);
            release_log();
            last_buffer_starts[connection_idx] = now;

            process_buffer(log, connection, control_table_map);
            connection_idx++;
        }

        // delay for a while to allow UI updates
        // for 2Mbs a delay of up to 16ms between buffers is okay
        vTaskDelay(4 / portTICK_PERIOD_MS);
    }
}

static void process_buffer(Log& log, Connection& connection, ControlTableMap& control_table_map) {
    Cursor* cursor;

    if (connection.buf->is_front_ready) {
        connection.buf->back.reset();
        cursor = &connection.buf->front;
    } else if (connection.buf->is_back_ready) {
        connection.buf->front.reset();
        cursor = &connection.buf->back;
    } else {
        return;
    }

    auto is_buf_empty = cursor->remaining_bytes() > 0;
    auto buf_processing_start = HAL_GetTick();

    while (cursor->remaining_bytes() > 0) {
        auto parse_result = connection.parser.parse(*cursor, &connection.last_packet);

        if (parse_result != ParseResult::PacketAvailable) {
            if (parse_result == ParseResult::NeedMoreData) {
                return;
            }

            lock_log();
            log.error(to_string(parse_result));
            release_log();
            continue;
        }

        lock_control_table_map();
        auto result = control_table_map.receive(connection.last_packet);
        release_control_table_map();

        if (result != ProtocolResult::Ok) {
            lock_log();
            log.error(to_string(result));
            release_log();
        }
    }

    if (!is_buf_empty) {
        auto buf_processing_end = HAL_GetTick();

        lock_log();
        log.buf_processing_time(buf_processing_end - buf_processing_start);
        release_log();
    }
}
