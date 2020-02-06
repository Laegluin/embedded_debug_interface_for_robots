#include "ui/device_overview_window.h"
#include "main.h"
#include "ui/run_ui.h"
#include <map>
#include <sstream>

DeviceOverviewWindow::DeviceOverviewWindow(
    const Mutex<ControlTableMap>* control_table_map,
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

void DeviceOverviewWindow::on_message(WM_MESSAGE* msg) {
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

void DeviceOverviewWindow::update() {
    struct DeviceModelStatus {
        const char* model_name;
        size_t num_connected;
        size_t num_disconnected;
    };

    size_t num_connected = 0;
    size_t num_disconnected = 0;

    std::map<uint16_t, DeviceModelStatus> model_to_status;

    // group by model and count total number of disconnected devices
    auto& control_table_map = this->control_table_map->lock();

    for (auto& id_and_table : control_table_map) {
        auto device_id = id_and_table.first;
        auto& control_table = id_and_table.second;
        bool is_disconnected = control_table_map.is_disconnected(device_id);

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

    this->control_table_map->unlock();

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

void DeviceOverviewWindow::push_back_model_widgets() {
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

void DeviceOverviewWindow::on_log_button_click() {
    WM_EnableWindow(this->log_win);
    WM_ShowWindow(this->log_win);
    WM_HideWindow(this->handle);
    WM_DisableWindow(this->handle);
}

void DeviceOverviewWindow::on_details_button_click() {
    WM_EnableWindow(this->device_info_win);
    WM_ShowWindow(this->device_info_win);
    WM_HideWindow(this->handle);
    WM_DisableWindow(this->handle);
}
