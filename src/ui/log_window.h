#ifndef LOG_WINDOW_H
#define LOG_WINDOW_H

#include "app.h"
#include "control_table.h"
#include "ui/window_registry.h"
#include <BUTTON.h>
#include <DIALOG.h>
#include <GUI.h>
#include <LISTVIEW.h>
#include <TEXT.h>

class LogWindow {
  public:
    LogWindow(WindowRegistry* registry, const Mutex<Log>* log);

    LogWindow(const LogWindow&) = delete;

    LogWindow(LogWindow&&) = delete;

    ~LogWindow();

    static LogWindow* from_handle(WM_HWIN handle) {
        LogWindow* self;
        WINDOW_GetUserData(handle, &self, sizeof(void*));
        return self;
    }

    static void handle_message(WM_MESSAGE* msg) {
        LogWindow::from_handle(msg->hWin)->on_message(msg);
    }

    LogWindow& operator=(const LogWindow&) = delete;

    LogWindow& operator=(LogWindow&&) = delete;

  private:
    void on_message(WM_MESSAGE* msg);

    void update_last_refresh();

    void on_back_button_click();

    void on_refresh_button_click();

    WindowRegistry* registry;
    const Mutex<Log>* log;
    uint32_t last_refresh;
    WM_HWIN handle;
    TEXT_Handle last_update_label;
    TEXT_Handle stats_label;
    LISTVIEW_Handle log_list;
    BUTTON_Handle back_button;
    BUTTON_Handle refresh_button;
};

#endif
