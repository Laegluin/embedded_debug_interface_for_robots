#ifndef MODEL_LIST_H
#define MODEL_LIST_H

#include "parser.h"
#include <DIALOG.h>
#include <GUI.h>
#include <string>
#include <vector>

class ModelList {
  public:
    struct Item {
        uint16_t model_number;
        std::string model_name;
        std::string label;
        bool is_disconnected;
    };

    ModelList(uint32_t x, uint32_t y, uint32_t width, uint32_t height, WM_HWIN parent);

    ModelList(const ModelList&) = delete;

    ModelList(ModelList&&) = delete;

    ~ModelList();

    static ModelList* from_handle(WM_HWIN handle) {
        ModelList* self;
        WM_GetUserData(handle, &self, sizeof(void*));
        return self;
    }

    static void handle_message(WM_MESSAGE* msg) {
        ModelList::from_handle(msg->hWin)->on_message(msg);
    }

    ModelList& operator=(const ModelList&) = delete;

    ModelList& operator=(ModelList&&) = delete;

    void set_item_size(uint32_t size) {
        this->item_size = size;
        WM_InvalidateWindow(this->handle);
    }

    void set_item_margin(uint32_t margin) {
        this->item_margin = margin;
        WM_InvalidateWindow(this->handle);
    }

    template <typename F>
    void insert_or_modify(uint16_t model_number, F modify) {
        size_t item_idx = 0;

        for (auto& item : this->items) {
            if (item.model_number == model_number) {
                break;
            } else if (item.model_number > model_number) {
                this->items.insert(
                    this->items.begin() + item_idx, Item{model_number, "", "", false});

                if (this->clicked_item_idx >= ssize_t(item_idx)) {
                    this->clicked_item_idx++;
                }

                if (this->pressed_item_idx >= ssize_t(item_idx)) {
                    this->pressed_item_idx++;
                }

                break;
            }

            item_idx++;
        }

        if (item_idx == this->items.size()) {
            this->items.push_back(Item{model_number, "", "", false});
        }

        modify(this->items[item_idx]);
        WM_InvalidateWindow(this->handle);
    }

    bool has_clicked_item() const {
        return this->clicked_item_idx != -1;
    }

    const Item& clicked_item() const {
        return this->items[this->clicked_item_idx];
    }

    WM_HWIN raw_handle() const {
        return this->handle;
    }

  private:
    void on_message(WM_MESSAGE* msg);

    void on_paint(const GUI_RECT* desktop_invalid_rect);

    void scroll_x(int32_t move_by);

    uint32_t scroll_area_width() const;

    WM_HWIN handle;
    std::vector<Item> items;
    ssize_t clicked_item_idx;
    ssize_t pressed_item_idx;
    bool is_window_pressed;
    uint32_t item_size;
    uint32_t item_margin;
    uint32_t scroll_pos_x;
};

#endif
