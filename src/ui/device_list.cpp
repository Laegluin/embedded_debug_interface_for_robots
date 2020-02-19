#include "ui/device_list.h"
#include "app.h"
#include <cmath>

DeviceList::DeviceList(uint32_t x, uint32_t y, uint32_t width, uint32_t height, WM_HWIN parent) :
    selected_item_idx(-1),
    pressed_item_idx(-1),
    is_window_pressed(false),
    item_height(BUTTON_HEIGHT),
    item_margin(MARGIN),
    scroll_pos_y(0) {
    this->handle = WM_CreateWindowAsChild(
        x, y, width, height, parent, WM_CF_SHOW, DeviceList::handle_message, sizeof(void*));

    void* self = this;
    WM_SetUserData(this->handle, &self, sizeof(void*));
}

DeviceList::~DeviceList() {
    WM_DeleteWindow(this->handle);
}

void DeviceList::on_message(WM_MESSAGE* msg) {
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
                // the calculation includes the starting margin for every item,
                // so we have to make sure that the offset into the item is greater
                // than the margin (otherwise it was actually a hit on the margin)
                uint32_t scroll_y = state->y + this->scroll_pos_y;
                size_t idx = scroll_y / (this->item_margin + this->item_height);
                uint32_t offset = scroll_y % (this->item_margin + this->item_height);

                if (idx < this->items.size() && offset > this->item_margin) {
                    this->pressed_item_idx = idx;
                }
            } else if (this->pressed_item_idx != -1) {
                this->selected_item_idx = this->pressed_item_idx;
                this->pressed_item_idx = -1;
                WM_NotifyParent(this->handle, WM_NOTIFICATION_SEL_CHANGED);
            }

            WM_InvalidateWindow(this->handle);
            break;
        }
        case WM_MOTION: {
            auto motion_info = static_cast<WM_MOTION_INFO*>(const_cast<void*>(msg->Data.p));

            switch (motion_info->Cmd) {
                case WM_MOTION_INIT: {
                    motion_info->Flags |= WM_MOTION_MANAGE_BY_WINDOW | WM_CF_MOTION_Y;
                    break;
                }
                case WM_MOTION_MOVE: {
                    // reset presses so that moving cannot select an item
                    this->pressed_item_idx = -1;
                    this->scroll_y(-motion_info->dy);
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

void DeviceList::on_paint(const GUI_RECT* desktop_invalid_rect) {
    uint32_t width = WM_GetWindowSizeX(this->handle);
    uint32_t height = WM_GetWindowSizeY(this->handle);

    GUI_RECT invalid_rect = *desktop_invalid_rect;
    GUI_MoveRect(&invalid_rect, -WM_GetWindowOrgX(this->handle), -WM_GetWindowOrgY(this->handle));

    GUI_SetColor(BACKGROUND_COLOR);
    GUI_FillRectEx(&invalid_rect);

    ssize_t item_idx = 0;
    int start_y = this->item_margin - this->scroll_pos_y;
    int end_y = start_y + this->item_height;

    for (auto& item : this->items) {
        if (start_y > invalid_rect.y1 || end_y - 1 < invalid_rect.y0) {
            start_y += this->item_height + this->item_margin;
            end_y = start_y + this->item_height;
            item_idx++;
            continue;
        }

        int start_x = this->item_margin;
        int end_x = start_x + width - (2 * this->item_margin);

        uint32_t color;

        if (item_idx == this->selected_item_idx || item_idx == this->pressed_item_idx) {
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

        GUI_SetFont(GUI_FONT_16_1);
        uint32_t font_height = GUI_GetFontSizeY();
        uint32_t text_start = start_y + ((this->item_height - font_height) / 2);
        GUI_SetBkColor(color);
        GUI_SetColor(DEVICE_STATUS_TEXT_COLOR);
        GUI_DispStringAt(item.label.c_str(), this->item_margin + MARGIN, text_start);

        start_y += this->item_height + this->item_margin;
        end_y = start_y + this->item_height;
        item_idx++;
    }

    uint32_t scroll_height = this->scroll_area_height();

    if (scroll_height > height) {
        float thumb_start_ratio = float(this->scroll_pos_y) / float(scroll_height);
        uint32_t thumb_start = height * thumb_start_ratio;
        uint32_t thumb_height = std::ceil((float(height) / float(scroll_height)) * height);
        GUI_SetColor(SCROLLBAR_THUMB_COLOR);
        GUI_FillRect(width - 2, thumb_start, width - 1, thumb_start + thumb_height - 1);
    }
}

void DeviceList::scroll_y(int32_t move_by) {
    uint32_t prev_pos = this->scroll_pos_y;

    if (move_by < 0) {
        // saturating sub
        this->scroll_pos_y += std::max(-int32_t(this->scroll_pos_y), move_by);
    } else {
        uint32_t height = WM_GetWindowSizeY(this->handle);
        uint32_t max_pos = this->scroll_area_height() - height;
        this->scroll_pos_y += std::min(max_pos - this->scroll_pos_y, uint32_t(move_by));
    }

    if (prev_pos != this->scroll_pos_y) {
        WM_InvalidateWindow(this->handle);
    }
}

uint32_t DeviceList::scroll_area_height() const {
    // area must be exactly large enough to allow only one item on screen after scrolling
    // to the bottom
    uint32_t height = WM_GetWindowSizeY(this->handle);
    uint32_t total_items_height =
        this->items.size() * (this->item_margin + this->item_height) + this->item_margin;

    // no scrolling when there's less than one page of items
    if (height >= total_items_height) {
        return height;
    }

    return total_items_height + (height - (2 * this->item_margin) - this->item_height);
}
