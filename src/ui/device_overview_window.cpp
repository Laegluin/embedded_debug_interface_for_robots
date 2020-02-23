#include "ui/device_overview_window.h"
#include "main.h"
#include "ui/device_info_window.h"
#include "ui/log_window.h"
#include "ui/model_overview_window.h"
#include "ui/run_ui.h"
#include <algorithm>
#include <sstream>
#include <unordered_map>

DeviceOverviewWindow::DeviceOverviewWindow(
    WindowRegistry* registry,
    const Mutex<ControlTableMap>* control_table_map,
    WM_HWIN handle) :
    registry(registry),
    control_table_map(control_table_map),
    handle(handle),
    model_list(
        0,
        TITLE_BAR_HEIGHT + MARGIN + 60,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT - TITLE_BAR_HEIGHT - MARGIN - 60,
        handle) {
    WM_HideWindow(this->handle);
    WM_DisableWindow(this->handle);

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
            switch (msg->Data.v) {
                case WM_NOTIFICATION_RELEASED: {
                    if (msg->hWinSrc == this->details_button) {
                        this->on_details_button_click();
                    } else if (msg->hWinSrc == this->log_button) {
                        this->on_log_button_click();
                    }

                    break;
                }
                case WM_NOTIFICATION_CLICKED: {
                    if (msg->hWinSrc == this->model_list.raw_handle()) {
                        this->on_model_list_click();
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

void DeviceOverviewWindow::update() {
    struct DeviceModelStatus {
        const char* model_name;
        size_t num_connected;
        size_t num_disconnected;
    };

    // copy only the required data as quickly as possible
    std::vector<ModelOverviewWindow::DeviceStatus> device_statuses;
    device_statuses.reserve(DeviceId::num_values());
    size_t num_connected = 0;
    size_t num_disconnected = 0;

    auto& control_table_map = this->control_table_map->lock();

    for (auto id_and_table : control_table_map) {
        auto device_id = id_and_table.first;
        auto& control_table = id_and_table.second;
        bool is_disconnected = control_table_map.is_disconnected(device_id);

        num_connected += !is_disconnected;
        num_disconnected += is_disconnected;

        if (!control_table->is_unknown_model()) {
            device_statuses.push_back(ModelOverviewWindow::DeviceStatus{
                device_id,
                control_table->device_name(),
                control_table->model_number(),
                is_disconnected,
            });
        }
    }

    this->control_table_map->unlock();

    // update model overview
    this->registry->get_window<ModelOverviewWindow>()->update(device_statuses);

    // group by model; we're doing this here to avoid allocations while holding the lock
    // also transform the data for updating the model overview
    std::unordered_map<uint16_t, DeviceModelStatus> model_to_status;
    model_to_status.reserve(device_statuses.size());

    for (auto& device_status : device_statuses) {
        auto result = model_to_status.emplace(
            device_status.model_number,
            DeviceModelStatus{
                device_status.device_name,
                0,
                0,
            });

        auto& model_status = result.first->second;
        model_status.num_connected += !device_status.is_disconnected;
        model_status.num_disconnected += device_status.is_disconnected;
    }

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
    for (auto& model_and_status : model_to_status) {
        auto model_number = model_and_status.first;
        auto& status = model_and_status.second;

        this->model_list.insert_or_modify(model_number, [&](auto& item) {
            std::stringstream fmt;
            fmt << std::to_string(status.num_connected) << " connected\n"
                << std::to_string(status.num_disconnected) << " disconnected";

            item.model_name = status.model_name;
            item.label = fmt.str();
            item.is_disconnected = status.num_disconnected > 0;
        });
    }
}

void DeviceOverviewWindow::on_log_button_click() {
    this->registry->navigate_to<LogWindow>();
}

void DeviceOverviewWindow::on_details_button_click() {
    this->registry->navigate_to<DeviceInfoWindow>();
}

void DeviceOverviewWindow::on_model_list_click() {
    ModelOverviewWindow* model_overview_win = this->registry->get_window<ModelOverviewWindow>();
    model_overview_win->select_model_number(this->model_list.clicked_item().model_number);
    this->update();

    this->registry->navigate_to<ModelOverviewWindow>();
}
