#include "ui/device_info_window.h"
#include "main.h"
#include "ui/run_ui.h"
#include <sstream>

DeviceInfoWindow::DeviceInfoWindow(
    const Mutex<ControlTableMap>* control_table_map,
    WM_HWIN handle,
    WM_HWIN device_overview_win) :
    control_table_map(control_table_map),
    handle(handle),
    device_overview_win(device_overview_win),
    device_list(0, TITLE_BAR_HEIGHT, 150, DISPLAY_HEIGHT - TITLE_BAR_HEIGHT, handle) {
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

void DeviceInfoWindow::on_message(WM_MESSAGE* msg) {
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
                    if (msg->hWinSrc == this->device_list.raw_handle()) {
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

void DeviceInfoWindow::update() {
    struct DeviceInfo {
        DeviceId id;
        const char* name;
        bool is_disconnected;
    };

    // copy the required values to minimize the time spent holding the lock
    std::unique_ptr<ControlTable> selected_control_table = nullptr;
    std::vector<DeviceInfo> device_infos;
    device_infos.reserve(DeviceId::num_values());

    auto& control_table_map = this->control_table_map->lock();

    for (auto id_and_table : control_table_map) {
        auto device_id = id_and_table.first;
        auto& control_table = id_and_table.second;

        device_infos.push_back(DeviceInfo{
            device_id,
            control_table->device_name(),
            control_table_map.is_disconnected(device_id),
        });

        if (this->device_list.is_item_selected()
            && this->device_list.selected_item().id == device_id) {
            selected_control_table = control_table->clone();
        }
    }

    this->control_table_map->unlock();

    for (auto& device_info : device_infos) {
        this->device_list.insert_or_modify(device_info.id, [&](auto& item) {
            std::stringstream fmt;
            fmt << device_info.name << " (" << device_info.id << ")";

            item.label = fmt.str();
            item.is_disconnected = device_info.is_disconnected;
        });
    }

    if (selected_control_table) {
        this->update_field_list(*selected_control_table);
    } else {
        this->clear_field_list();
    }
}

void DeviceInfoWindow::clear_field_list() {
    auto num_rows = LISTVIEW_GetNumRows(this->field_list);

    for (size_t i = 0; i < num_rows; i++) {
        LISTVIEW_DeleteRow(this->field_list, num_rows - 1 - i);
    }
}

void DeviceInfoWindow::update_field_list(const ControlTable& control_table) {
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

void DeviceInfoWindow::on_back_button_click() {
    WM_EnableWindow(this->device_overview_win);
    WM_ShowWindow(this->device_overview_win);
    WM_HideWindow(this->handle);
    WM_DisableWindow(this->handle);
}
