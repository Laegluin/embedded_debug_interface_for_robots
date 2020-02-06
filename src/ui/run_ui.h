#ifndef RUN_UI_H
#define RUN_UI_H

#include "app.h"
#include "control_table.h"
#include <DIALOG.h>
#include <GUI.h>

const int NO_ID = GUI_ID_USER + 0;

void run_ui(Mutex<Log>& log, const Mutex<ControlTableMap>& control_table_map);

void handle_touch_scroll(
    WM_MESSAGE* msg,
    float inc_per_pixel,
    float& state,
    void (*default_handler)(WM_MESSAGE*));

#endif
