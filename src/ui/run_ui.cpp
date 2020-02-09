#include "ui/run_ui.h"
#include "main.h"
#include "ui/device_info_window.h"
#include "ui/device_overview_window.h"
#include "ui/log_window.h"
#include <cmath>

static void create_ui(const Log&, const ControlTableMap&);
static void set_ui_theme();
static void set_header_skin();
static void set_scrollbar_skin();
static void set_button_skin();

void run_ui(Log& log, const ControlTableMap& control_table_map) {
    create_ui(log, control_table_map);

    while (true) {
        GUI_Exec();
    }
}

static void create_ui(const Log& log, const ControlTableMap& control_table_map) {
    set_ui_theme();

    auto device_overview_win = WINDOW_CreateUser(
        0,
        0,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT,
        0,
        WM_CF_SHOW,
        0,
        NO_ID,
        DeviceOverviewWindow::handle_message,
        sizeof(void*));

    auto device_info_win = WINDOW_CreateUser(
        0,
        0,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT,
        0,
        WM_CF_SHOW,
        0,
        NO_ID,
        DeviceInfoWindow::handle_message,
        sizeof(void*));

    auto log_win = WINDOW_CreateUser(
        0,
        0,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT,
        0,
        WM_CF_SHOW,
        0,
        NO_ID,
        LogWindow::handle_message,
        sizeof(void*));

    // create objects for widgets
    auto device_overview_obj =
        new DeviceOverviewWindow(&control_table_map, device_overview_win, log_win, device_info_win);
    WINDOW_SetUserData(device_overview_win, &device_overview_obj, sizeof(void*));

    auto device_info_obj =
        new DeviceInfoWindow(&control_table_map, device_info_win, device_overview_win);
    WINDOW_SetUserData(device_info_win, &device_info_obj, sizeof(void*));

    auto log_obj = new LogWindow(&log, log_win, device_overview_win);
    WINDOW_SetUserData(log_win, &log_obj, sizeof(void*));
}

static void set_ui_theme() {
    // set skins
    set_header_skin();
    set_scrollbar_skin();
    set_button_skin();
    WIDGET_SetDefaultEffect(&WIDGET_Effect_None);

    // fonts
    TEXT_SetDefaultFont(GUI_FONT_16_1);
    HEADER_SetDefaultFont(GUI_FONT_20_1);
    BUTTON_SetDefaultFont(GUI_FONT_20_1);
    LISTVIEW_SetDefaultFont(GUI_FONT_16_1);
    LISTBOX_SetDefaultFont(GUI_FONT_16_1);

    // colors
    TEXT_SetDefaultTextColor(TEXT_COLOR);

    WINDOW_SetDefaultBkColor(BACKGROUND_COLOR);

    BUTTON_SetDefaultBkColor(BUTTON_CI_UNPRESSED, BUTTON_COLOR);
    BUTTON_SetDefaultBkColor(BUTTON_CI_PRESSED, BUTTON_PRESSED_COLOR);
    BUTTON_SetDefaultTextColor(BUTTON_CI_UNPRESSED, TEXT_COLOR);
    BUTTON_SetDefaultTextColor(BUTTON_CI_PRESSED, TEXT_COLOR);

    HEADER_SetDefaultBkColor(LIST_HEADER_COLOR);
    HEADER_SetDefaultTextColor(TEXT_COLOR);

    LISTBOX_SetDefaultBkColor(LISTBOX_CI_UNSEL, LIST_ITEM_COLOR);
    LISTBOX_SetDefaultTextColor(LISTBOX_CI_UNSEL, TEXT_COLOR);
    LISTBOX_SetDefaultTextColor(LISTBOX_CI_SEL, TEXT_COLOR);
    LISTBOX_SetDefaultTextColor(LISTBOX_CI_SELFOCUS, TEXT_COLOR);

    LISTVIEW_SetDefaultBkColor(LISTVIEW_CI_UNSEL, LIST_ITEM_COLOR);
    LISTVIEW_SetDefaultTextColor(0, TEXT_COLOR);
    LISTVIEW_SetDefaultTextColor(1, TEXT_COLOR);
    LISTVIEW_SetDefaultTextColor(2, TEXT_COLOR);

    // size, margin and alignment
    SCROLLBAR_SetDefaultWidth(2);
    LISTBOX_SetDefaultTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    FRAMEWIN_SetDefaultBorderSize(0);
}

static void set_header_skin() {
    HEADER_SKINFLEX_PROPS props;
    props.aColorFrame[0] = LIST_HEADER_COLOR;
    props.aColorFrame[1] = LIST_HEADER_COLOR;
    props.aColorUpper[0] = LIST_HEADER_COLOR;
    props.aColorUpper[1] = LIST_HEADER_COLOR;
    props.aColorLower[0] = LIST_HEADER_COLOR;
    props.aColorLower[1] = LIST_HEADER_COLOR;
    props.ColorArrow = LIST_HEADER_COLOR;
    HEADER_SetSkinFlexProps(&props, 0);
}

static void set_scrollbar_skin() {
    SCROLLBAR_SetDefaultSkin([](const WIDGET_ITEM_DRAW_INFO* draw_info) -> int {
        switch (draw_info->Cmd) {
            case WIDGET_ITEM_CREATE: {
                WM_SetHasTrans(draw_info->hWin);
                return 0;
            }
            case WIDGET_ITEM_DRAW_BUTTON_L:
            case WIDGET_ITEM_DRAW_BUTTON_R:
            case WIDGET_ITEM_DRAW_OVERLAP:
            case WIDGET_ITEM_DRAW_SHAFT_L:
            case WIDGET_ITEM_DRAW_SHAFT_R: {
                return 0;
            }
            case WIDGET_ITEM_DRAW_THUMB: {
                SCROLLBAR_SKINFLEX_INFO* scrollbar_info = (SCROLLBAR_SKINFLEX_INFO*) draw_info->p;
                GUI_SetColor(SCROLLBAR_THUMB_COLOR);

                if (scrollbar_info->IsVertical) {
                    // coords are for some reason not translated when scrollbar is not horizontal
                    GUI_FillRect(draw_info->y0, draw_info->x0, draw_info->y1, draw_info->x1);
                } else {
                    GUI_FillRect(draw_info->x0, draw_info->y0, draw_info->x1, draw_info->y1);
                }

                return 0;
            }
            case WIDGET_ITEM_GET_BUTTONSIZE: {
                return 0;
            }
            default: { return SCROLLBAR_DrawSkinFlex(draw_info); }
        }
    });
}

static void set_button_skin() {
    BUTTON_SKINFLEX_PROPS props;
    props.aColorFrame[0] = BUTTON_COLOR;
    props.aColorFrame[1] = BUTTON_COLOR;
    props.aColorFrame[2] = BUTTON_COLOR;
    props.aColorUpper[0] = BUTTON_COLOR;
    props.aColorUpper[1] = BUTTON_COLOR;
    props.aColorLower[0] = BUTTON_COLOR;
    props.aColorLower[1] = BUTTON_COLOR;
    props.Radius = 0;
    BUTTON_SetSkinFlexProps(&props, BUTTON_SKINFLEX_PI_ENABLED);
    BUTTON_SetSkinFlexProps(&props, BUTTON_SKINFLEX_PI_FOCUSED);

    props.aColorFrame[0] = BUTTON_PRESSED_COLOR;
    props.aColorFrame[1] = BUTTON_PRESSED_COLOR;
    props.aColorFrame[2] = BUTTON_PRESSED_COLOR;
    props.aColorUpper[0] = BUTTON_PRESSED_COLOR;
    props.aColorUpper[1] = BUTTON_PRESSED_COLOR;
    props.aColorLower[0] = BUTTON_PRESSED_COLOR;
    props.aColorLower[1] = BUTTON_PRESSED_COLOR;
    props.Radius = 0;
    BUTTON_SetSkinFlexProps(&props, BUTTON_SKINFLEX_PI_PRESSED);
}

void handle_touch_scroll(
    WM_MESSAGE* msg,
    float inc_per_pixel,
    float& state,
    void (*default_handler)(WM_MESSAGE*)) {
    switch (msg->MsgId) {
        case WM_MOTION: {
            SCROLLBAR_Handle scrollbar = WM_GetScrollbarV(msg->hWin);
            if (scrollbar == 0) {
                break;
            }

            WM_MOTION_INFO* motion_info = (WM_MOTION_INFO*) msg->Data.p;

            switch (motion_info->Cmd) {
                case WM_MOTION_INIT: {
                    state = 0.0;
                    motion_info->Flags |= WM_MOTION_MANAGE_BY_WINDOW | WM_CF_MOTION_Y;
                    break;
                }
                case WM_MOTION_MOVE: {
                    state += motion_info->dy * -inc_per_pixel;
                    int move_by = std::trunc(state);
                    state -= move_by;
                    SCROLLBAR_AddValue(scrollbar, move_by);
                    break;
                }
                default: { break; }
            }

            break;
        }
        default: {
            default_handler(msg);
            break;
        }
    }
}
