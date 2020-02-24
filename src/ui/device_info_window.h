#ifndef DEVICE_INFO_WINDOW_H
#define DEVICE_INFO_WINDOW_H

#include "app.h"
#include "control_table.h"
#include "ui/device_list.h"
#include "ui/window_registry.h"
#include <BUTTON.h>
#include <DIALOG.h>
#include <GUI.h>
#include <LISTBOX.h>
#include <LISTVIEW.h>
#include <unordered_map>

class DeviceInfoWindow {
  public:
    DeviceInfoWindow(WindowRegistry* registry, const Mutex<ControlTableMap>* control_table_map);

    DeviceInfoWindow(const DeviceInfoWindow&) = delete;

    DeviceInfoWindow(DeviceInfoWindow&&) = delete;

    ~DeviceInfoWindow();

    static DeviceInfoWindow* from_handle(WM_HWIN handle) {
        DeviceInfoWindow* self;
        WINDOW_GetUserData(handle, &self, sizeof(void*));
        return self;
    }

    static void handle_message(WM_MESSAGE* msg) {
        DeviceInfoWindow::from_handle(msg->hWin)->on_message(msg);
    }

    DeviceInfoWindow& operator=(const DeviceInfoWindow&) = delete;

    DeviceInfoWindow& operator=(DeviceInfoWindow&&) = delete;

    void select_device(DeviceId id);

    void clear_selection();

  private:
    void on_message(WM_MESSAGE* msg);

    void update();

    void clear_field_list();

    void update_field_list(const ControlTable& control_table);

    void on_back_button_click();

    WindowRegistry* registry;
    const Mutex<ControlTableMap>* control_table_map;
    WM_HWIN handle;
    BUTTON_Handle back_button;
    DeviceList device_list;
    LISTVIEW_Handle field_list;
};

#endif
