#ifndef LOG_WINDOW_H
#define LOG_WINDOW_H

#include "app.h"
#include "control_table.h"
#include <BUTTON.h>
#include <DIALOG.h>
#include <GUI.h>
#include <LISTVIEW.h>
#include <TEXT.h>

class LogWindow {
  public:
    LogWindow(const Mutex<Log>* log, WM_HWIN handle, WM_HWIN device_overview_win);

    static LogWindow* from_handle(WM_HWIN handle) {
        LogWindow* self;
        WINDOW_GetUserData(handle, &self, sizeof(void*));
        return self;
    }

    static void handle_message(WM_MESSAGE* msg) {
        LogWindow::from_handle(msg->hWin)->on_message(msg);
    }

  private:
    void on_message(WM_MESSAGE* msg);

    void on_back_button_click();

    void on_refresh_button_click();

    const Mutex<Log>* log;
    WM_HWIN handle;
    TEXT_Handle stats_label;
    LISTVIEW_Handle log_list;
    BUTTON_Handle back_button;
    BUTTON_Handle refresh_button;
    WM_HWIN device_overview_win;
};

#endif
