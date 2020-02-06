#include "ui/device_info_window.h"
#include "app.h"
#include "main.h"
#include "ui/run_ui.h"
#include <sstream>

DeviceInfoWindow::DeviceInfoWindow(
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

void DeviceInfoWindow::update() {
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

int DeviceInfoWindow::update_device_list(DeviceId device_id, const ControlTable& control_table) {
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

void DeviceInfoWindow::update_device_list_item_color(DeviceId device_id, int item_idx) {
    // TODO: use custom function to change background color instead
    if (this->control_table_map->is_disconnected(device_id)) {
        LISTBOX_SetItemDisabled(this->device_list, item_idx, true);
    } else {
        LISTBOX_SetItemDisabled(this->device_list, item_idx, false);
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
