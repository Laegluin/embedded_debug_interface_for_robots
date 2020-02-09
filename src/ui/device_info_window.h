#ifndef DEVICE_INFO_WINDOW_H
#define DEVICE_INFO_WINDOW_H

#include "app.h"
#include "control_table.h"
#include <BUTTON.h>
#include <DIALOG.h>
#include <GUI.h>
#include <LISTBOX.h>
#include <LISTVIEW.h>

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

    /// Adds a device to the list if it is missing. Returns the index of the device in the list.
    int update_device_list(DeviceId device_id, const ControlTable& control_table);

    void clear_field_list();

    void update_field_list(const ControlTable& control_table);

    void on_back_button_click();

    const Mutex<ControlTableMap>* control_table_map;
    WM_HWIN handle;
    WM_HWIN device_overview_win;
    std::unordered_map<DeviceId, int> device_to_idx;
    int selected_item_idx;
    BUTTON_Handle back_button;
    LISTBOX_Handle device_list;
    LISTVIEW_Handle field_list;
};

#endif