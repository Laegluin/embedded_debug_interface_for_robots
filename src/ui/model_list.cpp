#include "ui/model_list.h"
#include "app.h"
#include <cmath>

ModelList::ModelList(uint32_t x, uint32_t y, uint32_t width, uint32_t height, WM_HWIN parent) :
    clicked_item_idx(-1),
    pressed_item_idx(-1),
    is_window_pressed(false),
    item_size(110),
    item_margin(MARGIN),
    scroll_pos_x(0) {
    this->handle = WM_CreateWindowAsChild(
        x, y, width, height, parent, WM_CF_SHOW, ModelList::handle_message, sizeof(void*));

    void* self = this;
    WM_SetUserData(this->handle, &self, sizeof(void*));
}

ModelList::~ModelList() {
    WM_DeleteWindow(this->handle);
}

void ModelList::on_message(WM_MESSAGE* msg) {
    switch (msg->MsgId) {
        case WM_TOUCH: {
            auto state = static_cast<const GUI_PID_STATE*>(msg->Data.p);

            // only check hits if the state changed (this saves time and allows
            // the motion event to clear the index of the pressed item)
            if (!state) {
                this->pressed_item_idx = -1;
                this->is_window_pressed = false;
                break;
            } else if (state->Pressed == this->is_window_pressed) {
                break;
            } else {
                this->is_window_pressed = state->Pressed;
            }

            if (state->Pressed) {
                // check y (top margin and there may be trailing empty space as well)
                if (uint32_t(state->y) < this->item_margin
                    || uint32_t(state->y) >= this->item_margin + this->item_size) {
                    break;
                }

                // the calculation includes the starting margin for every item,
                // so we have to make sure that the offset into the item is greater
                // than the margin (otherwise it was actually a hit on the margin)
                uint32_t scroll_x = state->x + this->scroll_pos_x;
                size_t idx = scroll_x / (this->item_margin + this->item_size);
                uint32_t offset = scroll_x % (this->item_margin + this->item_size);

                if (idx < this->items.size() && offset > this->item_margin) {
                    this->pressed_item_idx = idx;
                }
            } else if (this->pressed_item_idx != -1) {
                this->clicked_item_idx = this->pressed_item_idx;
                this->pressed_item_idx = -1;
                WM_NotifyParent(this->handle, WM_NOTIFICATION_CLICKED);
            }

            WM_InvalidateWindow(this->handle);
            break;
        }
        case WM_MOTION: {
            auto motion_info = static_cast<WM_MOTION_INFO*>(const_cast<void*>(msg->Data.p));

            switch (motion_info->Cmd) {
                case WM_MOTION_INIT: {
                    motion_info->Flags |= WM_MOTION_MANAGE_BY_WINDOW | WM_CF_MOTION_X;
                    break;
                }
                case WM_MOTION_MOVE: {
                    // reset presses so that moving cannot select an item
                    this->pressed_item_idx = -1;
                    this->scroll_x(-motion_info->dx);
                    break;
                }
                default: { break; }
            }

            break;
        }
        case WM_PAINT: {
            this->on_paint(static_cast<const GUI_RECT*>(msg->Data.p));
            break;
        }
        default: { WM_DefaultProc(msg); }
    }
}

void ModelList::on_paint(const GUI_RECT* desktop_invalid_rect) {
    GUI_RECT invalid_rect = *desktop_invalid_rect;
    GUI_MoveRect(&invalid_rect, -WM_GetWindowOrgX(this->handle), -WM_GetWindowOrgY(this->handle));

    GUI_SetColor(BACKGROUND_COLOR);
    GUI_FillRectEx(&invalid_rect);

    ssize_t item_idx = 0;
    int start_x = this->item_margin - this->scroll_pos_x;
    int end_x = start_x + this->item_size;

    for (auto& item : this->items) {
        if (start_x > invalid_rect.x1 || end_x - 1 < invalid_rect.x0) {
            start_x += this->item_size + this->item_margin;
            end_x = start_x + this->item_size;
            item_idx++;
            continue;
        }

        int start_y = this->item_margin;
        int end_y = start_y + this->item_size - (2 * this->item_margin);

        uint32_t color;

        if (item_idx == this->pressed_item_idx) {
            if (item.is_disconnected) {
                color = DEVICE_DISCONNECTED_PRESSED_COLOR;
            } else {
                color = DEVICE_CONNECTED_PRESSED_COLOR;
            }
        } else {
            if (item.is_disconnected) {
                color = DEVICE_DISCONNECTED_COLOR;
            } else {
                color = DEVICE_CONNECTED_COLOR;
            }
        }

        GUI_SetColor(color);
        GUI_FillRect(start_x, start_y, end_x - 1, end_y - 1);

        GUI_SetBkColor(color);
        GUI_SetColor(DEVICE_STATUS_TEXT_COLOR);
        GUI_SetFont(GUI_FONT_20_1);
        GUI_DispStringAt(item.model_name.c_str(), start_x + MARGIN, 2 * MARGIN);

        uint32_t font_height = GUI_GetFontSizeY();
        GUI_SetFont(GUI_FONT_16_1);
        GUI_SetLBorder(start_x + MARGIN);
        GUI_DispStringAt(item.label.c_str(), start_x + MARGIN, font_height + 3 * MARGIN);

        start_x += this->item_size + this->item_margin;
        end_x = start_x + this->item_size;
        item_idx++;
    }
}

void ModelList::scroll_x(int32_t move_by) {
    uint32_t prev_pos = this->scroll_pos_x;

    if (move_by < 0) {
        // saturating sub
        this->scroll_pos_x += std::max(-int32_t(this->scroll_pos_x), move_by);
    } else {
        uint32_t width = WM_GetWindowSizeX(this->handle);
        uint32_t max_pos = this->scroll_area_width() - width;
        this->scroll_pos_x += std::min(max_pos - this->scroll_pos_x, uint32_t(move_by));
    }

    if (prev_pos != this->scroll_pos_x) {
        WM_InvalidateWindow(this->handle);
    }
}

uint32_t ModelList::scroll_area_width() const {
    uint32_t width = WM_GetWindowSizeX(this->handle);
    uint32_t total_items_width =
        this->items.size() * (this->item_margin + this->item_size) + this->item_margin;

    // no scrolling when there's less than one page of items
    return std::max(width, total_items_width);
}
