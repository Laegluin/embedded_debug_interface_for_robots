#include "app.h"
#include "control_table.h"
#include "main.h"
#include "parser.h"

#include <DIALOG.h>
#include <GUI.h>
#include <LISTBOX.h>
#include <LISTVIEW.h>
#include <SCROLLBAR.h>

#include <cmath>
#include <memory>
#include <sstream>
#include <stm32f7xx.h>
#include <unordered_map>

struct Connection {
  public:
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

static void handle_incoming_packets(Connection&, ControlTableMap&);
static void create_ui(const ControlTableMap&);
void with_touch_scrolling(WM_MESSAGE*, void (*)(WM_MESSAGE*));

void run(const std::vector<ReceiveBuf*>& bufs) {
    ControlTableMap control_table_map;
    std::vector<Connection> connections;

    connections.reserve(bufs.size());

    for (auto buf : bufs) {
        connections.push_back(Connection(buf));
    }

    create_ui(control_table_map);
    uint32_t last_render = 0;

    while (true) {
        for (auto& connection : connections) {
            auto now = HAL_GetTick();

            // render every 16ms (roughly 60 fps)
            if (now - last_render > 16) {
                GUI_Exec();
                last_render = now;
            }

            handle_incoming_packets(connection, control_table_map);
        }
    }
}

static void handle_incoming_packets(Connection& connection, ControlTableMap& control_table_map) {
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

    while (cursor->remaining_bytes() > 0) {
        auto parse_result = connection.parser.parse(*cursor, &connection.last_packet);

        if (parse_result != ParseResult::PacketAvailable) {
            if (parse_result == ParseResult::NeedMoreData) {
                return;
            }

            // TODO: err handling
            continue;
        }

        auto result = control_table_map.receive(connection.last_packet);
        if (result != ProtocolResult::Ok) {
            // TODO: err handling
        }
    }
}

class DeviceInfoWindow {
  public:
    DeviceInfoWindow(
        const ControlTableMap* control_table_map,
        LISTBOX_Handle device_list,
        LISTVIEW_Handle field_list) :
        control_table_map(control_table_map),
        selected_item_idx(0),
        device_list(device_list),
        field_list(field_list) {}

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
                if (msg->Data.v == WM_NOTIFICATION_SEL_CHANGED
                    && msg->hWinSrc == this->device_list) {
                    this->selected_item_idx = LISTBOX_GetSel(this->device_list);
                    this->update();
                }

                break;
            }
            case WM_DELETE: {
                this->~DeviceInfoWindow();
                break;
            }
            default: {
                WM_DefaultProc(msg);
                break;
            }
        }
    }

    void update() {
        for (auto& id_and_table : *this->control_table_map) {
            auto device_id = id_and_table.first;
            auto& control_table = id_and_table.second;

            auto item_idx = this->update_device_list(device_id, *control_table);

            if (item_idx == this->selected_item_idx) {
                this->update_field_list(*control_table);
            } else if (item_idx < 0) {
                this->clear_field_list();
            }
        }
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

    const ControlTableMap* control_table_map;
    std::unordered_map<DeviceId, int> device_to_idx;
    int selected_item_idx;
    LISTBOX_Handle device_list;
    LISTVIEW_Handle field_list;
};

static void create_ui(const ControlTableMap& control_table_map) {
    const int NO_ID = GUI_ID_USER + 0;

    auto dev_info_win = WINDOW_CreateUser(
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

    auto device_list =
        LISTBOX_CreateEx(0, 0, 150, DISPLAY_HEIGHT, dev_info_win, WM_CF_SHOW, 0, NO_ID, nullptr);

    LISTBOX_SetAutoScrollV(device_list, true);
    LISTBOX_SetTextAlign(device_list, GUI_TA_LEFT | GUI_TA_VCENTER);
    LISTBOX_SetItemSpacing(device_list, 20);
    WM_SetCallback(device_list, [](auto msg) { with_touch_scrolling(msg, LISTBOX_Callback); });

    auto field_list = LISTVIEW_CreateEx(
        150, 0, DISPLAY_WIDTH - 150, DISPLAY_HEIGHT, dev_info_win, WM_CF_SHOW, 0, NO_ID);

    LISTVIEW_SetAutoScrollV(field_list, true);
    LISTVIEW_AddColumn(field_list, 150, "Field", GUI_TA_LEFT | GUI_TA_VCENTER);
    LISTVIEW_AddColumn(field_list, DISPLAY_WIDTH - 150, "Value", GUI_TA_LEFT | GUI_TA_VCENTER);
    auto listview_back_color = LISTVIEW_GetBkColor(field_list, LISTVIEW_CI_UNSEL);
    LISTVIEW_SetBkColor(field_list, LISTVIEW_CI_SEL, listview_back_color);
    LISTVIEW_SetBkColor(field_list, LISTVIEW_CI_SELFOCUS, listview_back_color);
    auto listview_text_color = LISTVIEW_GetTextColor(field_list, LISTVIEW_CI_UNSEL);
    LISTVIEW_SetTextColor(field_list, LISTVIEW_CI_SEL, listview_text_color);
    LISTVIEW_SetTextColor(field_list, LISTVIEW_CI_SELFOCUS, listview_text_color);
    WM_SetCallback(field_list, [](auto msg) { with_touch_scrolling(msg, LISTVIEW_Callback); });

    auto device_info = new DeviceInfoWindow(&control_table_map, device_list, field_list);
    WINDOW_SetUserData(dev_info_win, &device_info, sizeof(void*));
}

void with_touch_scrolling(WM_MESSAGE* msg, void (*default_handler)(WM_MESSAGE*)) {
    switch (msg->MsgId) {
        case WM_MOTION: {
            SCROLLBAR_Handle scrollbar = WM_GetScrollbarV(msg->hWin);
            if (scrollbar == 0) {
                break;
            }

            WM_MOTION_INFO* motion_info = (WM_MOTION_INFO*) msg->Data.p;

            switch (motion_info->Cmd) {
                case WM_MOTION_INIT: {
                    motion_info->Flags |= WM_MOTION_MANAGE_BY_WINDOW | WM_CF_MOTION_Y;
                    break;
                }
                case WM_MOTION_MOVE: {
                    int move_by = std::round(motion_info->dy * -0.2);
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
