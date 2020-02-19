#ifndef DEVICE_INFO_WINDOW_H
#define DEVICE_INFO_WINDOW_H

#include "app.h"
#include "control_table.h"
#include "ui/device_list.h"
#include <BUTTON.h>
#include <DIALOG.h>
#include <GUI.h>
#include <LISTBOX.h>
#include <LISTVIEW.h>
#include <unordered_map>

class DeviceInfoWindow {
  public:
    DeviceInfoWindow(
        const Mutex<ControlTableMap>* control_table_map,
        WM_HWIN handle,
        WM_HWIN device_overview_win);

    static DeviceInfoWindow* from_handle(WM_HWIN handle) {
        DeviceInfoWindow* self;
        WINDOW_GetUserData(handle, &self, sizeof(void*));
        return self;
    }

    static void handle_message(WM_MESSAGE* msg) {
        DeviceInfoWindow::from_handle(msg->hWin)->on_message(msg);
    }

  private:
    void on_message(WM_MESSAGE* msg);

    void update();

    void clear_field_list();

    void update_field_list(const ControlTable& control_table);

    void on_back_button_click();

    const Mutex<ControlTableMap>* control_table_map;
    WM_HWIN handle;
    WM_HWIN device_overview_win;
    BUTTON_Handle back_button;
    DeviceList device_list;
    LISTVIEW_Handle field_list;
};

#endif
