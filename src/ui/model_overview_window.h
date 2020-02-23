#ifndef MODEL_OVERVIEW_WINDOW_H
#define MODEL_OVERVIEW_WINDOW_H

#include "ui/device_list.h"
#include "ui/window_registry.h"
#include <BUTTON.h>
#include <DIALOG.h>
#include <GUI.h>
#include <TEXT.h>
#include <vector>

class ModelOverviewWindow {
  public:
    struct DeviceStatus {
        DeviceId device_id;
        const char* device_name;
        uint16_t model_number;
        bool is_disconnected;
    };

    ModelOverviewWindow(WindowRegistry* registry, WM_HWIN handle);

    static ModelOverviewWindow* from_handle(WM_HWIN handle) {
        ModelOverviewWindow* self;
        WINDOW_GetUserData(handle, &self, sizeof(void*));
        return self;
    }

    static void handle_message(WM_MESSAGE* msg) {
        ModelOverviewWindow::from_handle(msg->hWin)->on_message(msg);
    }

    void update(const std::vector<DeviceStatus>& statuses);

    void select_model_number(uint16_t model_number);

  private:
    void on_message(WM_MESSAGE* msg);

    void on_back_button_click();

    void on_device_list_selection();

    WindowRegistry* registry;
    uint16_t selected_model_number;
    WM_HWIN handle;
    TEXT_Handle title_label;
    TEXT_Handle status_label;
    WM_HWIN status_label_win;
    BUTTON_Handle back_button;
    DeviceList device_list;
};

#endif
