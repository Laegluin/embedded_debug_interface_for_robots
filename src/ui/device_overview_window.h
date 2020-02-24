#ifndef DEVICE_OVERVIEW_WINDOW_H
#define DEVICE_OVERVIEW_WINDOW_H

#include "app.h"
#include "control_table.h"
#include "ui/model_list.h"
#include "ui/window_registry.h"
#include <BUTTON.h>
#include <DIALOG.h>
#include <GUI.h>
#include <TEXT.h>
#include <vector>

class DeviceOverviewWindow {
  public:
    DeviceOverviewWindow(WindowRegistry* registry, const Mutex<ControlTableMap>* control_table_map);

    DeviceOverviewWindow(const DeviceOverviewWindow&) = delete;

    DeviceOverviewWindow(DeviceOverviewWindow&&) = delete;

    ~DeviceOverviewWindow();

    static DeviceOverviewWindow* from_handle(WM_HWIN handle) {
        DeviceOverviewWindow* self;
        WINDOW_GetUserData(handle, &self, sizeof(void*));
        return self;
    }

    static void handle_message(WM_MESSAGE* msg) {
        DeviceOverviewWindow::from_handle(msg->hWin)->on_message(msg);
    }

    DeviceOverviewWindow& operator=(const DeviceOverviewWindow&) = delete;

    DeviceOverviewWindow& operator=(DeviceOverviewWindow&&) = delete;

  private:
    void on_message(WM_MESSAGE* msg);

    void update();

    void on_log_button_click();

    void on_details_button_click();

    void on_model_list_click();

    WindowRegistry* registry;
    const Mutex<ControlTableMap>* control_table_map;
    WM_HWIN handle;
    TEXT_Handle status_label;
    WM_HWIN status_label_win;
    BUTTON_Handle log_button;
    BUTTON_Handle details_button;
    ModelList model_list;
};

#endif
