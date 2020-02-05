#include "app.h"
#include "control_table.h"
#include "main.h"
#include "parser.h"

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include <DIALOG.h>
#include <GUI.h>
#include <LISTBOX.h>
#include <LISTVIEW.h>
#include <SCROLLBAR.h>
#include <TEXT.h>

#include <cmath>
#include <deque>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <stm32f7xx.h>
#include <unordered_map>

const int NO_ID = GUI_ID_USER + 0;
const size_t MAX_NUM_LOG_ENTRIES = 100;

static SemaphoreHandle_t CONTROL_TABLE_MAP_MUTEX;
static SemaphoreHandle_t LOG_MUTEX;

static void lock_control_table_map() {
    xSemaphoreTake(CONTROL_TABLE_MAP_MUTEX, portMAX_DELAY);
}

static void release_control_table_map() {
    xSemaphoreGive(CONTROL_TABLE_MAP_MUTEX);
}

static void lock_log() {
    xSemaphoreTake(LOG_MUTEX, portMAX_DELAY);
}

static void release_log() {
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

class Log {
  public:
    Log() :
        max_ui_update_time_(0),
        min_ui_update_time_(0),
        ui_update_time_sum(0),
        num_ui_updates(0),
        max_buf_processing_time_(0),
        min_buf_processing_time_(0),
        buf_processing_time_sum(0),
        num_processed_bufs(0),
        max_time_between_buf_processing_(0) {}

    void error(std::string message) {
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

    void ui_update_time(uint32_t time) {
        this->max_ui_update_time_ = std::max(time, this->max_ui_update_time_);
        this->min_ui_update_time_ = std::min(time, this->min_ui_update_time_);
        this->ui_update_time_sum += time;
        this->num_ui_updates++;
    }

    void buf_processing_time(uint32_t time) {
        this->max_buf_processing_time_ = std::max(time, this->max_buf_processing_time_);
        this->min_buf_processing_time_ = std::max(time, this->min_buf_processing_time_);
        this->buf_processing_time_sum += time;
        this->num_processed_bufs++;
    }

    void time_between_buf_processing(uint32_t time) {
        this->max_time_between_buf_processing_ =
            std::max(time, this->max_time_between_buf_processing_);
    }

    size_t size() const {
        return this->messages.size();
    }

    std::deque<std::string>::const_iterator begin() const {
        return this->messages.begin();
    }

    std::deque<std::string>::const_iterator end() const {
        return this->messages.end();
    }

    uint32_t max_ui_update_time() const {
        return this->max_ui_update_time_;
    }

    float avg_ui_update_time() const {
        return (float) this->ui_update_time_sum / (float) this->num_ui_updates;
    }

    uint32_t min_ui_update_time() const {
        return this->min_ui_update_time_;
    }

    uint32_t max_buf_processing_time() const {
        return this->max_buf_processing_time_;
    }

    float avg_buf_processing_time() const {
        return (float) this->buf_processing_time_sum / (float) this->num_processed_bufs;
    }

    uint32_t min_buf_processing_time() const {
        return this->min_buf_processing_time_;
    }

    uint32_t max_time_between_buf_processing() const {
        return this->max_time_between_buf_processing_;
    }

  private:
    void push_message(std::string message) {
        if (messages.size() >= MAX_NUM_LOG_ENTRIES) {
            this->messages.pop_back();
        }

        this->messages.push_front(std::move(message));
    }

    std::deque<std::string> messages;
    uint32_t max_ui_update_time_;
    uint32_t min_ui_update_time_;
    uint32_t ui_update_time_sum;
    uint32_t num_ui_updates;
    uint32_t max_buf_processing_time_;
    uint32_t min_buf_processing_time_;
    uint32_t buf_processing_time_sum;
    uint32_t num_processed_bufs;
    uint32_t max_time_between_buf_processing_;
};

static void process_buffer(Log&, Connection&, ControlTableMap&);
static void run_ui(Log&, const ControlTableMap&);
static void create_ui(const Log&, const ControlTableMap&);
static void set_ui_theme();
static void set_header_skin();
static void set_scrollbar_skin();
static void set_button_skin();
static void handle_touch_scroll(WM_MESSAGE*, float, float&, void (*)(WM_MESSAGE*));

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
        vTaskDelay(8 / portTICK_PERIOD_MS);
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

class DeviceOverviewWindow {
  public:
    struct DeviceModelWidgets {
        WM_HWIN window;
        TEXT_Handle model_label;
        TEXT_Handle status_label;
    };

    DeviceOverviewWindow(
        const ControlTableMap* control_table_map,
        WM_HWIN handle,
        WM_HWIN log_win,
        WM_HWIN device_info_win) :
        control_table_map(control_table_map),
        handle(handle),
        device_info_win(device_info_win),
        log_win(log_win) {
        auto title = TEXT_CreateEx(
            6,
            0,
            300,
            TITLE_BAR_HEIGHT,
            this->handle,
            WM_CF_SHOW,
            TEXT_CF_LEFT | TEXT_CF_VCENTER,
            NO_ID,
            "Device Overview");

        TEXT_SetFont(title, GUI_FONT_24B_1);

        this->details_button = BUTTON_CreateEx(
            DISPLAY_WIDTH - 2 * (BUTTON_WIDTH + MARGIN),
            MARGIN,
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            this->handle,
            WM_CF_SHOW,
            0,
            NO_ID);

        BUTTON_SetText(this->details_button, "Details");

        this->log_button = BUTTON_CreateEx(
            DISPLAY_WIDTH - (BUTTON_WIDTH + MARGIN),
            MARGIN,
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            this->handle,
            WM_CF_SHOW,
            0,
            NO_ID);

        BUTTON_SetText(this->log_button, "Log");

        this->status_label_win = WINDOW_CreateEx(
            MARGIN,
            TITLE_BAR_HEIGHT,
            DISPLAY_WIDTH - 2 * MARGIN,
            60,
            this->handle,
            WM_CF_SHOW,
            0,
            NO_ID,
            WM_DefaultProc);

        WINDOW_SetBkColor(this->status_label_win, DEVICE_CONNECTED_COLOR);

        this->status_label = TEXT_CreateEx(
            MARGIN,
            0,
            300,
            60,
            this->status_label_win,
            WM_CF_SHOW,
            TEXT_CF_LEFT | TEXT_CF_VCENTER,
            NO_ID,
            "");

        TEXT_SetFont(this->status_label, GUI_FONT_20_1);
        TEXT_SetTextColor(this->status_label, GUI_WHITE);
    }

    static DeviceOverviewWindow* from_handle(WM_HWIN handle) {
        DeviceOverviewWindow* self;
        WINDOW_GetUserData(handle, &self, sizeof(void*));
        return self;
    }

    static void handle_message(WM_MESSAGE* msg) {
        DeviceOverviewWindow::from_handle(msg->hWin)->on_message(msg);
    }

  private:
    void on_message(WM_MESSAGE* msg) {
        switch (msg->MsgId) {
            case WM_USER_DATA: {
                this->update();
                WM_CreateTimer(msg->hWin, 0, 500, 0);
                break;
            }
            case WM_TIMER: {
                this->update();
                WM_RestartTimer(msg->Data.v, 0);
                break;
            }
            case WM_NOTIFY_PARENT: {
                if (msg->Data.v == WM_NOTIFICATION_RELEASED) {
                    if (msg->hWinSrc == this->details_button) {
                        this->on_details_button_click();
                    } else if (msg->hWinSrc == this->log_button) {
                        this->on_log_button_click();
                    }
                }

                break;
            }
            case WM_DELETE: {
                delete this;
                break;
            }
            default: {
                WM_DefaultProc(msg);
                break;
            }
        }
    }

    void update() {
        struct DeviceModelStatus {
            const char* model_name;
            size_t num_connected;
            size_t num_disconnected;
        };

        size_t num_connected = 0;
        size_t num_disconnected = 0;

        std::map<uint16_t, DeviceModelStatus> model_to_status;

        // group by model and count total number of disconnected devices
        lock_control_table_map();

        for (auto& id_and_table : *this->control_table_map) {
            auto device_id = id_and_table.first;
            auto& control_table = id_and_table.second;
            bool is_disconnected = this->control_table_map->is_disconnected(device_id);

            num_connected += !is_disconnected;
            num_disconnected += is_disconnected;

            if (!control_table->is_unknown_model()) {
                auto result = model_to_status.emplace(
                    control_table->model_number(),
                    DeviceModelStatus{control_table->device_name(), 0, 0});

                auto& status = result.first->second;

                if (is_disconnected) {
                    status.num_disconnected++;
                } else {
                    status.num_connected++;
                }
            }
        }

        release_control_table_map();

        // update status label
        std::stringstream fmt;
        fmt << std::to_string(num_connected) << " devices connected\n"
            << std::to_string(num_disconnected) << " devices disconnected";
        TEXT_SetText(this->status_label, fmt.str().c_str());

        if (num_disconnected > 0) {
            WINDOW_SetBkColor(this->status_label_win, DEVICE_DISCONNECTED_COLOR);
        } else {
            WINDOW_SetBkColor(this->status_label_win, DEVICE_CONNECTED_COLOR);
        }

        // update/create model widgets
        size_t model_idx = 0;

        for (auto& model_and_status : model_to_status) {
            auto& status = model_and_status.second;

            while (model_idx >= this->model_widgets.size()) {
                this->push_back_model_widgets();
            }

            auto widgets = this->model_widgets[model_idx];

            if (status.num_disconnected > 0) {
                WINDOW_SetBkColor(widgets.window, DEVICE_DISCONNECTED_COLOR);
            } else {
                WINDOW_SetBkColor(widgets.window, DEVICE_CONNECTED_COLOR);
            }

            TEXT_SetText(widgets.model_label, status.model_name);
            std::stringstream fmt;
            fmt << std::to_string(status.num_connected) << " connected\n"
                << std::to_string(status.num_disconnected) << " disconnected";
            TEXT_SetText(widgets.status_label, fmt.str().c_str());

            model_idx++;
        }
    }

    void push_back_model_widgets() {
        auto window = WINDOW_CreateEx(
            MARGIN + this->model_widgets.size() * (MARGIN + 115),
            116,
            115,
            115,
            this->handle,
            WM_CF_SHOW,
            0,
            NO_ID,
            WM_DefaultProc);

        auto model_label = TEXT_CreateEx(
            MARGIN,
            0,
            115 - 2 * MARGIN,
            25,
            window,
            WM_CF_SHOW,
            TEXT_CF_LEFT | TEXT_CF_VCENTER,
            NO_ID,
            "");

        TEXT_SetFont(model_label, GUI_FONT_20_1);
        TEXT_SetTextColor(model_label, DEVICE_STATUS_TEXT_COLOR);

        auto status_label = TEXT_CreateEx(
            MARGIN,
            25,
            115 - 2 * MARGIN,
            90,
            window,
            WM_CF_SHOW,
            TEXT_CF_LEFT | TEXT_CF_TOP,
            NO_ID,
            "");

        TEXT_SetTextColor(status_label, DEVICE_STATUS_TEXT_COLOR);

        this->model_widgets.push_back(DeviceModelWidgets{
            window,
            model_label,
            status_label,
        });
    }

    void on_log_button_click() {
        WM_EnableWindow(this->log_win);
        WM_ShowWindow(this->log_win);
        WM_HideWindow(this->handle);
        WM_DisableWindow(this->handle);
    }

    void on_details_button_click() {
        WM_EnableWindow(this->device_info_win);
        WM_ShowWindow(this->device_info_win);
        WM_HideWindow(this->handle);
        WM_DisableWindow(this->handle);
    }

    const ControlTableMap* control_table_map;
    WM_HWIN handle;
    TEXT_Handle status_label;
    WM_HWIN status_label_win;
    BUTTON_Handle log_button;
    BUTTON_Handle details_button;
    std::vector<DeviceModelWidgets> model_widgets;
    WM_HWIN device_info_win;
    WM_HWIN log_win;
};

class DeviceInfoWindow {
  public:
    DeviceInfoWindow(
        const ControlTableMap* control_table_map,
        WM_HWIN handle,
        WM_HWIN device_overview_win) :
        control_table_map(control_table_map),
        handle(handle),
        device_overview_win(device_overview_win),
        selected_item_idx(0) {
        WM_HideWindow(this->handle);
        WM_DisableWindow(this->handle);

        this->back_button = BUTTON_CreateEx(
            MARGIN, MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, this->handle, WM_CF_SHOW, 0, NO_ID);
        BUTTON_SetText(this->back_button, "Back");

        auto title = TEXT_CreateEx(
            90,
            0,
            300,
            TITLE_BAR_HEIGHT,
            this->handle,
            WM_CF_SHOW,
            TEXT_CF_LEFT | TEXT_CF_VCENTER,
            NO_ID,
            "Device Details");

        TEXT_SetFont(title, GUI_FONT_24B_1);

        this->device_list = LISTBOX_CreateEx(
            0,
            TITLE_BAR_HEIGHT,
            150,
            DISPLAY_HEIGHT - TITLE_BAR_HEIGHT,
            this->handle,
            WM_CF_SHOW,
            0,
            NO_ID,
            nullptr);

        // TODO: replace listbox with something more sensible and less broken
        LISTBOX_SetBkColor(this->device_list, LISTBOX_CI_UNSEL, MENU_COLOR);
        LISTBOX_SetBkColor(this->device_list, LISTBOX_CI_SEL, MENU_PRESSED_COLOR);
        LISTBOX_SetBkColor(this->device_list, LISTBOX_CI_SELFOCUS, MENU_PRESSED_COLOR);
        LISTBOX_SetBkColor(this->device_list, LISTBOX_CI_DISABLED, DEVICE_DISCONNECTED_COLOR);
        LISTBOX_SetTextColor(this->device_list, LISTBOX_CI_DISABLED, DEVICE_STATUS_TEXT_COLOR);
        LISTBOX_SetAutoScrollV(this->device_list, true);
        LISTBOX_SetItemSpacing(this->device_list, 20);

        WM_SetCallback(this->device_list, [](auto msg) {
            static float state;
            handle_touch_scroll(msg, 0.05, state, LISTBOX_Callback);
        });

        this->field_list = LISTVIEW_CreateEx(
            150 + MARGIN,
            TITLE_BAR_HEIGHT,
            DISPLAY_WIDTH - (150 + MARGIN),
            DISPLAY_HEIGHT - TITLE_BAR_HEIGHT,
            this->handle,
            WM_CF_SHOW,
            0,
            NO_ID);

        LISTVIEW_SetAutoScrollV(this->field_list, true);
        LISTVIEW_AddColumn(this->field_list, 150, "Field", GUI_TA_LEFT | GUI_TA_VCENTER);
        LISTVIEW_AddColumn(
            this->field_list, DISPLAY_WIDTH - 150, "Value", GUI_TA_LEFT | GUI_TA_VCENTER);
        LISTVIEW_SetTextAlign(this->field_list, 0, GUI_TA_LEFT | GUI_TA_VCENTER);
        LISTVIEW_SetTextAlign(this->field_list, 1, GUI_TA_LEFT | GUI_TA_VCENTER);
        auto listview_back_color = LISTVIEW_GetBkColor(this->field_list, LISTVIEW_CI_UNSEL);
        LISTVIEW_SetBkColor(this->field_list, LISTVIEW_CI_SEL, listview_back_color);
        LISTVIEW_SetBkColor(this->field_list, LISTVIEW_CI_SELFOCUS, listview_back_color);
        auto listview_text_color = LISTVIEW_GetTextColor(this->field_list, LISTVIEW_CI_UNSEL);
        LISTVIEW_SetTextColor(this->field_list, LISTVIEW_CI_SEL, listview_text_color);
        LISTVIEW_SetTextColor(this->field_list, LISTVIEW_CI_SELFOCUS, listview_text_color);

        WM_SetCallback(this->field_list, [](auto msg) {
            static float state;
            handle_touch_scroll(msg, 0.15, state, LISTVIEW_Callback);
        });
    }

    static DeviceInfoWindow* from_handle(WM_HWIN handle) {
        DeviceInfoWindow* self;
        WINDOW_GetUserData(handle, &self, sizeof(void*));
        return self;
    }

    static void handle_message(WM_MESSAGE* msg) {
        DeviceInfoWindow::from_handle(msg->hWin)->on_message(msg);
    }

  private:
    void on_message(WM_MESSAGE* msg) {
        switch (msg->MsgId) {
            case WM_USER_DATA: {
                this->update();
                WM_CreateTimer(msg->hWin, 0, 500, 0);
                break;
            }
            case WM_TIMER: {
                this->update();
                WM_RestartTimer(msg->Data.v, 0);
                break;
            }
            case WM_NOTIFY_PARENT: {
                switch (msg->Data.v) {
                    case WM_NOTIFICATION_SEL_CHANGED: {
                        if (msg->hWinSrc == this->device_list) {
                            this->selected_item_idx = LISTBOX_GetSel(this->device_list);
                            this->update();
                        }

                        break;
                    }
                    case WM_NOTIFICATION_RELEASED: {
                        if (msg->hWinSrc == this->back_button) {
                            this->on_back_button_click();
                        }

                        break;
                    }
                    default: { break; }
                }

                break;
            }
            case WM_DELETE: {
                delete this;
                break;
            }
            default: {
                WM_DefaultProc(msg);
                break;
            }
        }
    }

    void update() {
        lock_control_table_map();

        for (auto& id_and_table : *this->control_table_map) {
            auto device_id = id_and_table.first;
            auto& control_table = id_and_table.second;

            auto item_idx = this->update_device_list(device_id, *control_table);

            if (item_idx == this->selected_item_idx) {
                this->update_field_list(*control_table);
            } else if (this->selected_item_idx < 0) {
                this->clear_field_list();
            }
        }

        release_control_table_map();
    }

    /// Adds or updates a device in the list. This function assumes that devices are only
    /// added, but not removed. Returns the index of the device in the list.
    int update_device_list(DeviceId device_id, const ControlTable& control_table) {
        auto iter = this->device_to_idx.find(device_id);
        if (iter != this->device_to_idx.end()) {
            auto item_idx = iter->second;
            this->update_device_list_item_color(device_id, item_idx);
            return item_idx;
        }

        // find index for insertion while preserving order by id
        int insert_idx = LISTBOX_GetNumItems(this->device_list);
        bool is_last_device_id_set = false;
        DeviceId last_device_id(0);

        for (auto& device_and_idx : this->device_to_idx) {
            auto current_device_id = device_and_idx.first;

            // id should be greater (since it will be shifted to the right)
            // and as small as possible (otherwise we're inserting with greater elements
            // to the left)
            if (current_device_id > device_id
                && (!is_last_device_id_set || current_device_id < last_device_id)) {
                insert_idx = device_and_idx.second;
                is_last_device_id_set = true;
                last_device_id = current_device_id;
            }
        }

        // correct indices of items past the insertion point
        for (auto& device_and_idx : this->device_to_idx) {
            if (device_and_idx.second >= insert_idx) {
                device_and_idx.second++;
            }
        }

        std::stringstream stream;
        stream << control_table.device_name() << " (" << device_id << ")";

        LISTBOX_InsertString(this->device_list, stream.str().c_str(), insert_idx);
        this->device_to_idx.emplace(device_id, insert_idx);
        this->update_device_list_item_color(device_id, insert_idx);

        return insert_idx;
    }

    void update_device_list_item_color(DeviceId device_id, int item_idx) {
        // TODO: use custom function to change background color instead
        if (this->control_table_map->is_disconnected(device_id)) {
            LISTBOX_SetItemDisabled(this->device_list, item_idx, true);
        } else {
            LISTBOX_SetItemDisabled(this->device_list, item_idx, false);
        }
    }

    void clear_field_list() {
        auto num_rows = LISTVIEW_GetNumRows(this->field_list);

        for (size_t i = 0; i < num_rows; i++) {
            LISTVIEW_DeleteRow(this->field_list, num_rows - 1 - i);
        }
    }

    void update_field_list(const ControlTable& control_table) {
        size_t row_idx = 0;
        auto num_rows = LISTVIEW_GetNumRows(this->field_list);
        auto formatted_fields = control_table.fmt_fields();

        for (auto& name_and_value : formatted_fields) {
            auto name = name_and_value.first;
            auto value = name_and_value.second.c_str();

            if (row_idx >= num_rows) {
                const char* cells[] = {name, value};
                LISTVIEW_AddRow(this->field_list, cells);
                num_rows++;
            } else {
                LISTVIEW_SetItemText(this->field_list, 0, row_idx, name);
                LISTVIEW_SetItemText(this->field_list, 1, row_idx, value);
            }

            row_idx++;
        }

        // delete no longer used rows
        size_t num_unused_rows = num_rows - row_idx;

        for (size_t i = 0; i < num_unused_rows; i++) {
            LISTVIEW_DeleteRow(this->field_list, num_rows - 1 - i);
        }
    }

    void on_back_button_click() {
        WM_EnableWindow(this->device_overview_win);
        WM_ShowWindow(this->device_overview_win);
        WM_HideWindow(this->handle);
        WM_DisableWindow(this->handle);
    }

    const ControlTableMap* control_table_map;
    WM_HWIN handle;
    WM_HWIN device_overview_win;
    std::unordered_map<DeviceId, int> device_to_idx;
    int selected_item_idx;
    BUTTON_Handle back_button;
    LISTBOX_Handle device_list;
    LISTVIEW_Handle field_list;
};

class LogWindow {
  public:
    LogWindow(const Log* log, WM_HWIN handle, WM_HWIN device_overview_win) :
        log(log),
        handle(handle),
        device_overview_win(device_overview_win) {
        WM_HideWindow(this->handle);
        WM_DisableWindow(this->handle);

        this->back_button = BUTTON_CreateEx(
            MARGIN, MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, this->handle, WM_CF_SHOW, 0, NO_ID);
        BUTTON_SetText(this->back_button, "Back");

        auto title = TEXT_CreateEx(
            90,
            0,
            300,
            TITLE_BAR_HEIGHT,
            this->handle,
            WM_CF_SHOW,
            TEXT_CF_LEFT | TEXT_CF_VCENTER,
            NO_ID,
            "Log");

        TEXT_SetFont(title, GUI_FONT_24B_1);

        this->refresh_button = BUTTON_CreateEx(
            DISPLAY_WIDTH - (BUTTON_WIDTH + MARGIN),
            MARGIN,
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            this->handle,
            WM_CF_SHOW,
            0,
            NO_ID);

        BUTTON_SetText(this->refresh_button, "Refresh");

        auto stats_window = WINDOW_CreateEx(
            0,
            TITLE_BAR_HEIGHT,
            150,
            DISPLAY_HEIGHT - TITLE_BAR_HEIGHT,
            this->handle,
            WM_CF_SHOW,
            0,
            NO_ID,
            WM_DefaultProc);

        WINDOW_SetBkColor(stats_window, MENU_COLOR);

        this->stats_label = TEXT_CreateEx(
            MARGIN,
            MARGIN,
            150 - 2 * MARGIN,
            DISPLAY_HEIGHT - TITLE_BAR_HEIGHT - 2 * MARGIN,
            stats_window,
            WM_CF_SHOW,
            TEXT_CF_LEFT | TEXT_CF_TOP,
            NO_ID,
            "<no data>");

        this->log_list = LISTVIEW_CreateEx(
            150 + MARGIN,
            TITLE_BAR_HEIGHT,
            DISPLAY_WIDTH - (150 + MARGIN),
            DISPLAY_HEIGHT - TITLE_BAR_HEIGHT,
            this->handle,
            WM_CF_SHOW,
            0,
            NO_ID);

        LISTVIEW_SetHeaderHeight(this->log_list, 0);
        LISTVIEW_AddColumn(this->log_list, DISPLAY_WIDTH, "", GUI_TA_LEFT | GUI_TA_VCENTER);
        LISTVIEW_SetAutoScrollV(this->log_list, true);
        auto background_color = LISTVIEW_GetBkColor(this->log_list, LISTVIEW_CI_UNSEL);
        LISTVIEW_SetBkColor(this->log_list, LISTVIEW_CI_SEL, background_color);
        LISTVIEW_SetBkColor(this->log_list, LISTVIEW_CI_SELFOCUS, background_color);
        auto text_color = LISTVIEW_GetTextColor(this->log_list, LISTVIEW_CI_UNSEL);
        LISTVIEW_SetTextColor(this->log_list, LISTVIEW_CI_SEL, text_color);
        LISTVIEW_SetTextColor(this->log_list, LISTVIEW_CI_SELFOCUS, text_color);

        WM_SetCallback(this->log_list, [](auto msg) {
            static float state;
            handle_touch_scroll(msg, 0.15, state, LISTVIEW_Callback);
        });
    }

    static LogWindow* from_handle(WM_HWIN handle) {
        LogWindow* self;
        WINDOW_GetUserData(handle, &self, sizeof(void*));
        return self;
    }

    static void handle_message(WM_MESSAGE* msg) {
        LogWindow::from_handle(msg->hWin)->on_message(msg);
    }

  private:
    void on_message(WM_MESSAGE* msg) {
        switch (msg->MsgId) {
            case WM_NOTIFY_PARENT: {
                if (msg->Data.v == WM_NOTIFICATION_RELEASED) {
                    if (msg->hWinSrc == this->back_button) {
                        this->on_back_button_click();
                    } else if (msg->hWinSrc == this->refresh_button) {
                        this->on_refresh_button_click();
                    }
                }

                break;
            }
            case WM_DELETE: {
                delete this;
                break;
            }
            default: {
                WM_DefaultProc(msg);
                break;
            }
        }
    }

    void on_back_button_click() {
        WM_EnableWindow(this->device_overview_win);
        WM_ShowWindow(this->device_overview_win);
        WM_HideWindow(this->handle);
        WM_DisableWindow(this->handle);
    }

    void on_refresh_button_click() {
        lock_log();

        std::stringstream fmt;
        fmt << "Max. time btw. buffers\n"
            << this->log->max_time_between_buf_processing() << " ms\n"
            << "Min. time per buffer\n"
            << this->log->min_buf_processing_time() << " ms\n"
            << "Avg. time per buffer\n"
            << this->log->avg_buf_processing_time() << " ms\n"
            << "Max. time per buffer\n"
            << this->log->max_buf_processing_time() << " ms\n\n"
            << "Max. UI update time\n"
            << this->log->max_ui_update_time() << " ms\n"
            << "Avg. UI update time\n"
            << this->log->avg_ui_update_time() << " ms";
        TEXT_SetText(this->stats_label, fmt.str().c_str());

        auto num_items = LISTVIEW_GetNumRows(this->log_list);

        // delete or add items as necessary
        if (num_items < this->log->size()) {
            auto num_missing_items = this->log->size() - num_items;

            for (size_t i = 0; i < num_missing_items; i++) {
                const char* cells[]{""};
                LISTVIEW_AddRow(this->log_list, cells);
            }
        } else if (num_items > this->log->size()) {
            auto num_extra_items = num_items - this->log->size();

            for (size_t i = 0; i < num_extra_items; i++) {
                LISTVIEW_DeleteRow(this->log_list, num_items - 1 - i);
            }
        }

        size_t item_idx = 0;

        for (auto& log_entry : *this->log) {
            LISTVIEW_SetItemText(this->log_list, 0, item_idx, log_entry.c_str());
            item_idx++;
        }

        release_log();
    }

    const Log* log;
    WM_HWIN handle;
    TEXT_Handle stats_label;
    LISTVIEW_Handle log_list;
    BUTTON_Handle back_button;
    BUTTON_Handle refresh_button;
    WM_HWIN device_overview_win;
};

static void run_ui(Log& log, const ControlTableMap& control_table_map) {
    create_ui(log, control_table_map);

    while (true) {
        auto update_start = HAL_GetTick();
        GUI_Exec();
        auto update_end = HAL_GetTick();

        lock_log();
        log.ui_update_time(update_end - update_start);
        release_log();
    }
}

static void create_ui(const Log& log, const ControlTableMap& control_table_map) {
    set_ui_theme();

    auto device_overview_win = WINDOW_CreateUser(
        0,
        0,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT,
        0,
        WM_CF_SHOW,
        0,
        NO_ID,
        DeviceOverviewWindow::handle_message,
        sizeof(void*));

    auto device_info_win = WINDOW_CreateUser(
        0,
        0,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT,
        0,
        WM_CF_SHOW,
        0,
        NO_ID,
        DeviceInfoWindow::handle_message,
        sizeof(void*));

    auto log_win = WINDOW_CreateUser(
        0,
        0,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT,
        0,
        WM_CF_SHOW,
        0,
        NO_ID,
        LogWindow::handle_message,
        sizeof(void*));

    // create objects for widgets
    auto device_overview_obj =
        new DeviceOverviewWindow(&control_table_map, device_overview_win, log_win, device_info_win);
    WINDOW_SetUserData(device_overview_win, &device_overview_obj, sizeof(void*));

    auto device_info_obj =
        new DeviceInfoWindow(&control_table_map, device_info_win, device_overview_win);
    WINDOW_SetUserData(device_info_win, &device_info_obj, sizeof(void*));

    auto log_obj = new LogWindow(&log, log_win, device_overview_win);
    WINDOW_SetUserData(log_win, &log_obj, sizeof(void*));
}

static void set_ui_theme() {
    // set skins
    set_header_skin();
    set_scrollbar_skin();
    set_button_skin();
    WIDGET_SetDefaultEffect(&WIDGET_Effect_None);

    // fonts
    TEXT_SetDefaultFont(GUI_FONT_16_1);
    HEADER_SetDefaultFont(GUI_FONT_20_1);
    BUTTON_SetDefaultFont(GUI_FONT_20_1);
    LISTVIEW_SetDefaultFont(GUI_FONT_16_1);
    LISTBOX_SetDefaultFont(GUI_FONT_16_1);

    // colors
    TEXT_SetDefaultTextColor(TEXT_COLOR);

    WINDOW_SetDefaultBkColor(BACKGROUND_COLOR);

    BUTTON_SetDefaultBkColor(BUTTON_CI_UNPRESSED, BUTTON_COLOR);
    BUTTON_SetDefaultBkColor(BUTTON_CI_PRESSED, BUTTON_PRESSED_COLOR);
    BUTTON_SetDefaultTextColor(BUTTON_CI_UNPRESSED, TEXT_COLOR);
    BUTTON_SetDefaultTextColor(BUTTON_CI_PRESSED, TEXT_COLOR);

    HEADER_SetDefaultBkColor(LIST_HEADER_COLOR);
    HEADER_SetDefaultTextColor(TEXT_COLOR);

    LISTBOX_SetDefaultBkColor(LISTBOX_CI_UNSEL, LIST_ITEM_COLOR);
    LISTBOX_SetDefaultTextColor(LISTBOX_CI_UNSEL, TEXT_COLOR);
    LISTBOX_SetDefaultTextColor(LISTBOX_CI_SEL, TEXT_COLOR);
    LISTBOX_SetDefaultTextColor(LISTBOX_CI_SELFOCUS, TEXT_COLOR);

    LISTVIEW_SetDefaultBkColor(LISTVIEW_CI_UNSEL, LIST_ITEM_COLOR);
    LISTVIEW_SetDefaultTextColor(0, TEXT_COLOR);
    LISTVIEW_SetDefaultTextColor(1, TEXT_COLOR);
    LISTVIEW_SetDefaultTextColor(2, TEXT_COLOR);

    // size, margin and alignment
    SCROLLBAR_SetDefaultWidth(2);
    LISTBOX_SetDefaultTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    FRAMEWIN_SetDefaultBorderSize(0);
}

static void set_header_skin() {
    HEADER_SKINFLEX_PROPS props;
    props.aColorFrame[0] = LIST_HEADER_COLOR;
    props.aColorFrame[1] = LIST_HEADER_COLOR;
    props.aColorUpper[0] = LIST_HEADER_COLOR;
    props.aColorUpper[1] = LIST_HEADER_COLOR;
    props.aColorLower[0] = LIST_HEADER_COLOR;
    props.aColorLower[1] = LIST_HEADER_COLOR;
    props.ColorArrow = LIST_HEADER_COLOR;
    HEADER_SetSkinFlexProps(&props, 0);
}

static void set_scrollbar_skin() {
    SCROLLBAR_SetDefaultSkin([](const WIDGET_ITEM_DRAW_INFO* draw_info) -> int {
        switch (draw_info->Cmd) {
            case WIDGET_ITEM_CREATE: {
                WM_SetHasTrans(draw_info->hWin);
                return 0;
            }
            case WIDGET_ITEM_DRAW_BUTTON_L:
            case WIDGET_ITEM_DRAW_BUTTON_R:
            case WIDGET_ITEM_DRAW_OVERLAP:
            case WIDGET_ITEM_DRAW_SHAFT_L:
            case WIDGET_ITEM_DRAW_SHAFT_R: {
                return 0;
            }
            case WIDGET_ITEM_DRAW_THUMB: {
                SCROLLBAR_SKINFLEX_INFO* scrollbar_info = (SCROLLBAR_SKINFLEX_INFO*) draw_info->p;
                GUI_SetColor(SCROLLBAR_THUMB_COLOR);

                if (scrollbar_info->IsVertical) {
                    // coords are for some reason not translated when scrollbar is not horizontal
                    GUI_FillRect(draw_info->y0, draw_info->x0, draw_info->y1, draw_info->x1);
                } else {
                    GUI_FillRect(draw_info->x0, draw_info->y0, draw_info->x1, draw_info->y1);
                }

                return 0;
            }
            case WIDGET_ITEM_GET_BUTTONSIZE: {
                return 0;
            }
            default: { return SCROLLBAR_DrawSkinFlex(draw_info); }
        }
    });
}

static void set_button_skin() {
    BUTTON_SKINFLEX_PROPS props;
    props.aColorFrame[0] = BUTTON_COLOR;
    props.aColorFrame[1] = BUTTON_COLOR;
    props.aColorFrame[2] = BUTTON_COLOR;
    props.aColorUpper[0] = BUTTON_COLOR;
    props.aColorUpper[1] = BUTTON_COLOR;
    props.aColorLower[0] = BUTTON_COLOR;
    props.aColorLower[1] = BUTTON_COLOR;
    props.Radius = 0;
    BUTTON_SetSkinFlexProps(&props, BUTTON_SKINFLEX_PI_ENABLED);
    BUTTON_SetSkinFlexProps(&props, BUTTON_SKINFLEX_PI_FOCUSED);

    props.aColorFrame[0] = BUTTON_PRESSED_COLOR;
    props.aColorFrame[1] = BUTTON_PRESSED_COLOR;
    props.aColorFrame[2] = BUTTON_PRESSED_COLOR;
    props.aColorUpper[0] = BUTTON_PRESSED_COLOR;
    props.aColorUpper[1] = BUTTON_PRESSED_COLOR;
    props.aColorLower[0] = BUTTON_PRESSED_COLOR;
    props.aColorLower[1] = BUTTON_PRESSED_COLOR;
    props.Radius = 0;
    BUTTON_SetSkinFlexProps(&props, BUTTON_SKINFLEX_PI_PRESSED);
}

static void handle_touch_scroll(
    WM_MESSAGE* msg,
    float inc_per_pixel,
    float& state,
    void (*default_handler)(WM_MESSAGE*)) {
    switch (msg->MsgId) {
        case WM_MOTION: {
            SCROLLBAR_Handle scrollbar = WM_GetScrollbarV(msg->hWin);
            if (scrollbar == 0) {
                break;
            }

            WM_MOTION_INFO* motion_info = (WM_MOTION_INFO*) msg->Data.p;

            switch (motion_info->Cmd) {
                case WM_MOTION_INIT: {
                    state = 0.0;
                    motion_info->Flags |= WM_MOTION_MANAGE_BY_WINDOW | WM_CF_MOTION_Y;
                    break;
                }
                case WM_MOTION_MOVE: {
                    state += motion_info->dy * -inc_per_pixel;
                    int move_by = std::trunc(state);
                    state -= move_by;
                    SCROLLBAR_AddValue(scrollbar, move_by);
                    break;
                }
                default: { break; }
            }

            break;
        }
        default: {
            default_handler(msg);
            break;
        }
    }
}
