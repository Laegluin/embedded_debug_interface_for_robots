#ifndef DEVICE_LIST_H
#define DEVICE_LIST_H

#include "parser.h"
#include <DIALOG.h>
#include <GUI.h>
#include <string>
#include <vector>

class DeviceList {
  public:
    struct Item {
        DeviceId id;
        std::string label;
        bool is_disconnected;
    };

    DeviceList(uint32_t x, uint32_t y, uint32_t width, uint32_t height, WM_HWIN parent);

    DeviceList(const DeviceList&) = delete;

    DeviceList(DeviceList&&) = delete;

    ~DeviceList();

    static DeviceList* from_handle(WM_HWIN handle) {
        DeviceList* self;
        WM_GetUserData(handle, &self, sizeof(void*));
        return self;
    }

    static void handle_message(WM_MESSAGE* msg) {
        DeviceList::from_handle(msg->hWin)->on_message(msg);
    }

    DeviceList& operator=(const DeviceList&) = delete;

    DeviceList& operator=(DeviceList&&) = delete;

    void set_item_height(uint32_t height) {
        this->item_height = height;
        WM_InvalidateWindow(this->handle);
    }

    void set_item_margin(uint32_t margin) {
        this->item_margin = margin;
        WM_InvalidateWindow(this->handle);
    }

    template <typename F>
    void insert_or_modify(DeviceId id, F modify) {
        size_t item_idx = 0;

        for (auto& item : this->items) {
            if (item.id == id) {
                break;
            } else if (item.id > id) {
                this->items.insert(this->items.begin() + item_idx, Item{id, "", false});

                if (this->selected_item_idx >= ssize_t(item_idx)) {
                    this->selected_item_idx++;
                }

                if (this->pressed_item_idx >= ssize_t(item_idx)) {
                    this->pressed_item_idx++;
                }

                break;
            }

            item_idx++;
        }

        if (item_idx == this->items.size()) {
            this->items.push_back(Item{id, "", false});
        }

        modify(this->items[item_idx]);
        WM_InvalidateWindow(this->handle);
    }

    void clear() {
        this->items.clear();
        this->selected_item_idx = -1;
        this->pressed_item_idx = -1;
        this->scroll_pos_y = 0;
    }

    void select_device(DeviceId id);

    void clear_selection() {
        this->selected_item_idx = -1;
    }

    bool is_item_selected() const {
        return this->selected_item_idx != -1;
    }

    const Item& selected_item() const {
        return this->items[this->selected_item_idx];
    }

    WM_HWIN raw_handle() const {
        return this->handle;
    }

  private:
    void on_message(WM_MESSAGE* msg);

    void on_paint(const GUI_RECT* desktop_invalid_rect);

    void scroll_y(int32_t move_by);

    uint32_t scroll_area_height() const;

    WM_HWIN handle;
    std::vector<Item> items;
    ssize_t selected_item_idx;
    ssize_t pressed_item_idx;
    bool is_window_pressed;
    uint32_t item_height;
    uint32_t item_margin;
    uint32_t scroll_pos_y;
};

#endif
