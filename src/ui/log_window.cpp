#include "ui/log_window.h"
#include "main.h"
#include "ui/run_ui.h"
#include <FreeRTOS.h>
#include <sstream>

LogWindow::LogWindow(const Mutex<Log>* log, WM_HWIN handle, WM_HWIN device_overview_win) :
    log(log),
    handle(handle),
    device_overview_win(device_overview_win) {
    WM_HideWindow(this->handle);
    WM_DisableWindow(this->handle);

    this->back_button = BUTTON_CreateEx(
        MARGIN, MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, this->handle, WM_CF_SHOW, 0, NO_ID);
    BUTTON_SetText(this->back_button, "Back");

    auto title = TEXT_CreateEx(
        90,
        0,
        300,
        TITLE_BAR_HEIGHT,
        this->handle,
        WM_CF_SHOW,
        TEXT_CF_LEFT | TEXT_CF_VCENTER,
        NO_ID,
        "Log");

    TEXT_SetFont(title, GUI_FONT_24B_1);

    this->refresh_button = BUTTON_CreateEx(
        DISPLAY_WIDTH - (BUTTON_WIDTH + MARGIN),
        MARGIN,
        BUTTON_WIDTH,
        BUTTON_HEIGHT,
        this->handle,
        WM_CF_SHOW,
        0,
        NO_ID);

    BUTTON_SetText(this->refresh_button, "Refresh");

    auto stats_window = WINDOW_CreateEx(
        0,
        TITLE_BAR_HEIGHT,
        150,
        DISPLAY_HEIGHT - TITLE_BAR_HEIGHT,
        this->handle,
        WM_CF_SHOW,
        0,
        NO_ID,
        WM_DefaultProc);

    WINDOW_SetBkColor(stats_window, MENU_COLOR);

    this->stats_label = TEXT_CreateEx(
        MARGIN,
        MARGIN,
        150 - 2 * MARGIN,
        DISPLAY_HEIGHT - TITLE_BAR_HEIGHT - 2 * MARGIN,
        stats_window,
        WM_CF_SHOW,
        TEXT_CF_LEFT | TEXT_CF_TOP,
        NO_ID,
        "<no data>");

    this->log_list = LISTVIEW_CreateEx(
        150 + MARGIN,
        TITLE_BAR_HEIGHT,
        DISPLAY_WIDTH - (150 + MARGIN),
        DISPLAY_HEIGHT - TITLE_BAR_HEIGHT,
        this->handle,
        WM_CF_SHOW,
        0,
        NO_ID);

    LISTVIEW_SetHeaderHeight(this->log_list, 0);
    LISTVIEW_AddColumn(this->log_list, DISPLAY_WIDTH, "", GUI_TA_LEFT | GUI_TA_VCENTER);
    LISTVIEW_SetAutoScrollV(this->log_list, true);
    auto background_color = LISTVIEW_GetBkColor(this->log_list, LISTVIEW_CI_UNSEL);
    LISTVIEW_SetBkColor(this->log_list, LISTVIEW_CI_SEL, background_color);
    LISTVIEW_SetBkColor(this->log_list, LISTVIEW_CI_SELFOCUS, background_color);
    auto text_color = LISTVIEW_GetTextColor(this->log_list, LISTVIEW_CI_UNSEL);
    LISTVIEW_SetTextColor(this->log_list, LISTVIEW_CI_SEL, text_color);
    LISTVIEW_SetTextColor(this->log_list, LISTVIEW_CI_SELFOCUS, text_color);

    WM_SetCallback(this->log_list, [](auto msg) {
        static float state;
        handle_touch_scroll(msg, 0.15, state, LISTVIEW_Callback);
    });
}

void LogWindow::on_message(WM_MESSAGE* msg) {
    switch (msg->MsgId) {
        case WM_NOTIFY_PARENT: {
            if (msg->Data.v == WM_NOTIFICATION_RELEASED) {
                if (msg->hWinSrc == this->back_button) {
                    this->on_back_button_click();
                } else if (msg->hWinSrc == this->refresh_button) {
                    this->on_refresh_button_click();
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

void LogWindow::on_back_button_click() {
    WM_EnableWindow(this->device_overview_win);
    WM_ShowWindow(this->device_overview_win);
    WM_HideWindow(this->handle);
    WM_DisableWindow(this->handle);
}

void LogWindow::on_refresh_button_click() {
    auto& log = this->log->lock();

    std::stringstream fmt;
    fmt << "Max. time btw. buffers\n"
        << log.max_time_between_buf_processing() << " ms\n"
        << "Min. time per buffer\n"
        << log.min_buf_processing_time() << " ms\n"
        << "Avg. time per buffer\n"
        << log.avg_buf_processing_time() << " ms\n"
        << "Max. time per buffer\n"
        << log.max_buf_processing_time() << " ms\n\n"
        << "Free heap memory\n"
        << xPortGetFreeHeapSize() << " B\n"
        << "Free UI memory\n"
        << GUI_ALLOC_GetNumFreeBytes() << " B";
    TEXT_SetText(this->stats_label, fmt.str().c_str());

    auto num_items = LISTVIEW_GetNumRows(this->log_list);

    // delete or add items as necessary
    if (num_items < log.size()) {
        auto num_missing_items = log.size() - num_items;

        for (size_t i = 0; i < num_missing_items; i++) {
            const char* cells[]{""};
            LISTVIEW_AddRow(this->log_list, cells);
        }
    } else if (num_items > log.size()) {
        auto num_extra_items = num_items - log.size();

        for (size_t i = 0; i < num_extra_items; i++) {
            LISTVIEW_DeleteRow(this->log_list, num_items - 1 - i);
        }
    }

    size_t item_idx = 0;

    for (auto& log_entry : log) {
        LISTVIEW_SetItemText(this->log_list, 0, item_idx, log_entry.c_str());
        item_idx++;
    }

    this->log->unlock();
}