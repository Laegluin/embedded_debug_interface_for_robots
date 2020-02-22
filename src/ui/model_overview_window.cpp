#include "ui/model_overview_window.h"
#include "app.h"
#include "run_ui.h"
#include "ui/device_info_window.h"
#include <sstream>

ModelOverviewWindow::ModelOverviewWindow(
    WM_HWIN handle,
    WM_HWIN device_info_win,
    WM_HWIN device_overview_win) :
    handle(handle),
    device_list(
        0,
        TITLE_BAR_HEIGHT + 2 * MARGIN + 60,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT - TITLE_BAR_HEIGHT - 2 * MARGIN - 60,
        handle),
    device_info_win(device_info_win),
    device_overview_win(device_overview_win),
    selected_model_number(0) {
    WM_HideWindow(this->handle);
    WM_DisableWindow(this->handle);

    this->back_button = BUTTON_CreateEx(
        MARGIN, MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, this->handle, WM_CF_SHOW, 0, NO_ID);
    BUTTON_SetText(this->back_button, "Back");

    this->title_label = TEXT_CreateEx(
        90,
        0,
        300,
        TITLE_BAR_HEIGHT,
        this->handle,
        WM_CF_SHOW,
        TEXT_CF_LEFT | TEXT_CF_VCENTER,
        NO_ID,
        "");

    TEXT_SetFont(this->title_label, GUI_FONT_24B_1);

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

    this->device_list.insert_or_modify(DeviceId(0), [](auto& item) { item.label = "0 blabla"; });
    this->device_list.insert_or_modify(DeviceId(1), [](auto& item) { item.label = "1 ahaha"; });
    this->device_list.insert_or_modify(DeviceId(2), [](auto& item) { item.label = "2 ahaha"; });
    this->device_list.insert_or_modify(DeviceId(3), [](auto& item) { item.label = "3 ahaha"; });
    this->device_list.insert_or_modify(DeviceId(4), [](auto& item) { item.label = "4 ahaha"; });
    this->device_list.insert_or_modify(DeviceId(5), [](auto& item) { item.label = "5 ahaha"; });
}

void ModelOverviewWindow::update(const std::vector<DeviceStatus>& statuses) {
    std::stringstream fmt;

    const char* model_name = "<unknown>";
    size_t num_connected = 0;
    size_t num_disconnected = 0;

    // device list
    for (auto& status : statuses) {
        if (status.model_number == this->selected_model_number) {
            model_name = status.device_name;
            num_connected += !status.is_disconnected;
            num_disconnected += status.is_disconnected;

            this->device_list.insert_or_modify(status.device_id, [&](auto& item) {
                fmt << model_name << " (" << status.device_id << ")";

                item.label = fmt.str();
                item.is_disconnected = status.is_disconnected;

                fmt.str("");
            });
        }
    }

    // labels and status colors
    fmt << model_name << " Overview";
    TEXT_SetText(this->title_label, fmt.str().c_str());
    fmt.str("");

    fmt << num_connected << " devices connected\n" << num_disconnected << " devices disconnected";
    TEXT_SetText(this->status_label, fmt.str().c_str());

    if (num_disconnected > 0) {
        WINDOW_SetBkColor(this->status_label_win, DEVICE_DISCONNECTED_COLOR);
    } else {
        WINDOW_SetBkColor(this->status_label_win, DEVICE_CONNECTED_COLOR);
    }
}

void ModelOverviewWindow::select_model_number(uint16_t model_number) {
    if (this->selected_model_number == model_number) {
        return;
    }

    this->selected_model_number = model_number;
    this->device_list.clear();
}

void ModelOverviewWindow::on_message(WM_MESSAGE* msg) {
    switch (msg->MsgId) {
        case WM_NOTIFY_PARENT: {
            switch (msg->Data.v) {
                case WM_NOTIFICATION_RELEASED: {
                    if (msg->hWinSrc == this->back_button) {
                        this->on_back_button_click();
                    }

                    break;
                }
                case WM_NOTIFICATION_SEL_CHANGED: {
                    if (msg->hWinSrc == this->device_list.raw_handle()) {
                        this->on_device_list_selection();
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

void ModelOverviewWindow::on_back_button_click() {
    WM_EnableWindow(this->device_overview_win);
    WM_ShowWindow(this->device_overview_win);
    WM_HideWindow(this->handle);
    WM_DisableWindow(this->handle);
}

void ModelOverviewWindow::on_device_list_selection() {
    DeviceInfoWindow* device_info_obj = DeviceInfoWindow::from_handle(this->device_info_win);

    if (this->device_list.is_item_selected()) {
        device_info_obj->select_device(this->device_list.selected_item().id);
    } else {
        device_info_obj->clear_selection();
    }

    WM_EnableWindow(this->device_info_win);
    WM_ShowWindow(this->device_info_win);
    WM_HideWindow(this->handle);
    WM_DisableWindow(this->handle);

    this->device_list.clear_selection();
}
