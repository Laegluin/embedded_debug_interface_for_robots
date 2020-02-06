#ifndef DEVICE_OVERVIEW_WINDOW_H
#define DEVICE_OVERVIEW_WINDOW_H

#include "app.h"
#include "control_table.h"
#include <BUTTON.h>
#include <DIALOG.h>
#include <GUI.h>
#include <TEXT.h>
#include <vector>

class DeviceOverviewWindow {
  public:
    struct DeviceModelWidgets {
        WM_HWIN window;
        TEXT_Handle model_label;
        TEXT_Handle status_label;
    };

    DeviceOverviewWindow(
        const Mutex<ControlTableMap>* control_table_map,
        WM_HWIN handle,
        WM_HWIN log_win,
        WM_HWIN device_info_win);

    static DeviceOverviewWindow* from_handle(WM_HWIN handle) {
        DeviceOverviewWindow* self;
        WINDOW_GetUserData(handle, &self, sizeof(void*));
        return self;
    }

    static void handle_message(WM_MESSAGE* msg) {
        DeviceOverviewWindow::from_handle(msg->hWin)->on_message(msg);
    }

  private:
    void on_message(WM_MESSAGE* msg);

    void update();

    void push_back_model_widgets();

    void on_log_button_click();

    void on_details_button_click();

    const Mutex<ControlTableMap>* control_table_map;
    WM_HWIN handle;
    TEXT_Handle status_label;
    WM_HWIN status_label_win;
    BUTTON_Handle log_button;
    BUTTON_Handle details_button;
    std::vector<DeviceModelWidgets> model_widgets;
    WM_HWIN device_info_win;
    WM_HWIN log_win;
};

#endif
