#ifndef WINDOW_REGISTRY_H
#define WINDOW_REGISTRY_H

#include <DIALOG.h>
#include <GUI.h>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

class WindowRegistry {
  public:
    WindowRegistry() {}

    template <typename W>
    void register_window(WM_HWIN handle) {
        this->window_to_handle.emplace(std::type_index(typeid(W)), handle);
    }

    template <typename W>
    W* get_window() {
        auto iter = this->window_to_handle.find(std::type_index(typeid(W)));
        if (iter == this->window_to_handle.end()) {
            return nullptr;
        }

        WM_HWIN handle = iter->second;
        return W::from_handle(handle);
    }

    template <typename W>
    void navigate_to() {
        auto iter = this->window_to_handle.find(std::type_index(typeid(W)));
        if (iter == this->window_to_handle.end()) {
            return;
        }

        WM_HWIN handle = iter->second;
        WM_EnableWindow(handle);
        WM_ShowWindow(handle);

        if (!this->nav_history.empty()) {
            WM_HideWindow(this->nav_history.back());
            WM_DisableWindow(this->nav_history.back());
        }

        this->nav_history.push_back(handle);
    }

    void navigate_back() {
        if (this->nav_history.size() < 2) {
            return;
        }

        WM_HWIN current = this->nav_history[this->nav_history.size() - 1];
        WM_HWIN prev = this->nav_history[this->nav_history.size() - 2];

        WM_EnableWindow(prev);
        WM_ShowWindow(prev);
        WM_HideWindow(current);
        WM_DisableWindow(current);

        this->nav_history.pop_back();
    }

  private:
    std::unordered_map<std::type_index, WM_HWIN> window_to_handle;
    std::vector<WM_HWIN> nav_history;
};

#endif
