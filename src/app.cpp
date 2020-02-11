#include "app.h"
#include "control_table.h"
#include "main.h"
#include "parser.h"
#include "ui/run_ui.h"

#include <FreeRTOS.h>
#include <task.h>

#include <iomanip>
#include <sstream>

struct Connection {
    Connection(ReceiveBuf* buf) :
        last_processing_start(0),
        buf(buf),
        parser(Parser()),
        last_packet(Packet{
            DeviceId(0),
            Instruction::Ping,
            Error(),
            std::vector<uint8_t>(),
        }) {
        // prevent allocations in the main loop
        this->last_packet.data.reserve(MAX_PACKET_DATA_LEN);
    }

    uint32_t last_processing_start;
    ReceiveBuf* buf;
    Parser parser;
    Packet last_packet;
};

static void process_buffer(Mutex<Log>&, Connection&, Mutex<ControlTableMap>&);

Log::Log() :
    max_buf_processing_time_(0),
    min_buf_processing_time_(std::numeric_limits<uint32_t>::max()),
    buf_processing_time_sum(0),
    num_processed_bufs(0),
    max_time_between_buf_processing_(0) {
    // make sure no allocations are required in the main loop
    // since std::deque has no reserve method for some reason, we
    // have to use resize as a workaround
    this->messages.resize(MAX_NUM_LOG_ENTRIES);
    this->messages.shrink_to_fit();
    this->messages.resize(0);
}

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
    this->min_buf_processing_time_ = std::min(time, this->min_buf_processing_time_);
    this->buf_processing_time_sum += time;
    this->num_processed_bufs++;
}

void Log::time_between_buf_processing(uint32_t time) {
    this->max_time_between_buf_processing_ = std::max(time, this->max_time_between_buf_processing_);
}

size_t Log::size() const {
    return this->messages.size();
}

std::deque<std::shared_ptr<std::string>>::const_iterator Log::begin() const {
    return this->messages.begin();
}

std::deque<std::shared_ptr<std::string>>::const_iterator Log::end() const {
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

void Log::push_message(std::string&& message) {
    if (messages.size() >= MAX_NUM_LOG_ENTRIES) {
        this->messages.pop_back();
    }

    this->messages.push_front(std::make_shared<std::string>(std::move(message)));
}

void run(const std::vector<ReceiveBuf*>& bufs) {
    Mutex<Log> log;
    Mutex<ControlTableMap> control_table_map;

    void* args[2] = {&log, &control_table_map};

    xTaskCreate(
        [](void* args) {
            auto log = (Mutex<Log>*) ((void**) args)[0];
            auto control_table_map = (Mutex<ControlTableMap>*) ((void**) args)[1];

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

    std::vector<Connection> connections;
    connections.reserve(bufs.size());

    for (auto buf : bufs) {
        connections.push_back(Connection(buf));
    }

    while (true) {
        for (auto& connection : connections) {
            if (connection.last_processing_start == 0) {
                connection.last_processing_start = HAL_GetTick();
            }

            process_buffer(log, connection, control_table_map);
        }

        // delay for a while to allow UI updates
        // for 2Mbs a delay of up to 16ms between buffers is okay
        vTaskDelay(4 / portTICK_PERIOD_MS);
    }
}

static void process_buffer(
    Mutex<Log>& log,
    Connection& connection,
    Mutex<ControlTableMap>& control_table_map) {
    Cursor* cursor;

    if (connection.buf->is_front_ready) {
        connection.buf->back.reset();
        cursor = &connection.buf->front;
    } else if (connection.buf->is_back_ready) {
        connection.buf->front.reset();
        cursor = &connection.buf->back;
    } else {
        on_error();
    }

    auto processing_start = HAL_GetTick();
    auto is_buf_empty = cursor->remaining_bytes() == 0;
    std::vector<ParseResult> parse_errors;
    std::vector<ProtocolResult> protocol_errors;

    auto& control_table_map_ref = control_table_map.lock();

    while (cursor->remaining_bytes() > 0) {
        auto parse_result = connection.parser.parse(*cursor, &connection.last_packet);

        if (parse_result != ParseResult::PacketAvailable) {
            if (parse_result == ParseResult::NeedMoreData) {
                break;
            }

            parse_errors.push_back(parse_result);
            continue;
        }

        auto result = control_table_map_ref.receive(connection.last_packet);

        if (result != ProtocolResult::Ok) {
            protocol_errors.push_back(result);
        }
    }

    // never lock both at the same time to prevent deadlocks
    control_table_map.unlock();
    auto& log_ref = log.lock();

    // only report time for non empty buffers in order to have useful min and average
    if (!is_buf_empty) {
        auto processing_end = HAL_GetTick();
        log_ref.buf_processing_time(processing_end - processing_start);
    }

    for (auto parse_error : parse_errors) {
        log_ref.error(to_string(parse_error));
    }

    for (auto protocol_error : protocol_errors) {
        log_ref.error(to_string(protocol_error));
    }

    log_ref.time_between_buf_processing(processing_start - connection.last_processing_start);
    connection.last_processing_start = processing_start;

    log.unlock();
}
