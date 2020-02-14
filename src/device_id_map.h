#ifndef DEVICE_ID_MAP_H
#define DEVICE_ID_MAP_H

#include "parser.h"
#include <functional>
#include <vector>

template <typename V>
class DeviceIdMapIter;

/// A map optimized for mapping `DeviceId`s to values. All possible entries are stored in a
/// single vector. This makes iteration for sparse maps slow and access guaranteed O(1).
template <typename V>
class DeviceIdMap {
  public:
    class Entry {
      public:
        Entry(DeviceIdMap<V>* map) : is_present_(false), map(map) {}

        Entry(const Entry&) = delete;

        Entry(Entry&& src) : is_present_(std::move(src.is_present_)), map(std::move(src.map)) {
            if (this->is_present_) {
                new (&this->value_) V(std::move(src.value_));
            }
        }

        ~Entry() {
            if (this->is_present_) {
                this->value_.~V();
            }
        }

        Entry& operator=(const Entry&) = delete;

        Entry& operator=(Entry&&) = delete;

        bool is_present() const {
            return this->is_present_;
        }

        const V& value() const {
            return this->value_;
        }

        V& value() {
            return this->value_;
        }

        template <typename D>
        V& or_insert(D&& default_value) {
            if (!this->is_present_) {
                new (&this->value_) V(std::forward<D>(default_value));
                this->is_present_ = true;
                map->size_++;
            }

            return this->value_;
        }

        template <typename F>
        V& or_insert_with(F create_value) {
            if (!this->is_present_) {
                new (&this->value_) V(create_value());
                this->is_present_ = true;
                map->size_++;
            }

            return this->value_;
        }

        template <typename N>
        V& set_value(N&& new_value) {
            if (this->is_present_) {
                this->value_.~V();
            }

            new (&this->value_) V(std::forward<N>(new_value));
            map->size_ += !this->is_present_;
            this->is_present_ = true;

            return this->value_;
        }

        void clear_value() {
            if (this->is_present_) {
                this->is_present_ = false;
                this->value_.~V();
            }
        }

      private:
        bool is_present_;
        union {
            V value_;
        };
        DeviceIdMap<V>* map;
    };

    using const_iterator = DeviceIdMapIter<V>;

    DeviceIdMap() : size_(0) {
        this->entries.reserve(DeviceId::num_values());

        for (size_t i = 0; i < DeviceId::num_values(); i++) {
            this->entries.push_back(Entry(this));
        }
    }

    Entry& get(DeviceId id) {
        return this->entries[id.to_byte()];
    }

    const Entry& get(DeviceId id) const {
        return this->entries[id.to_byte()];
    }

    size_t size() const {
        return this->size_;
    }

    const_iterator begin() const {
        return DeviceIdMapIter<V>(&this->entries);
    }

    const_iterator end() const {
        return DeviceIdMapIter<V>::end(&this->entries);
    }

    void clear() {
        this->size_ = 0;

        for (auto& entry : this->entries) {
            entry.clear_value();
        }
    }

  private:
    std::vector<Entry> entries;
    size_t size_;
};

template <typename V>
class DeviceIdMapIter {
  public:
    DeviceIdMapIter() = delete;

    DeviceIdMapIter(const std::vector<typename DeviceIdMap<V>::Entry>* entries) :
        DeviceIdMapIter(entries, 0) {}

    static DeviceIdMapIter<V> end(const std::vector<typename DeviceIdMap<V>::Entry>* entries) {
        return DeviceIdMapIter<V>(entries, DeviceId::num_values());
    }

    std::pair<DeviceId, const V&> operator*() const {
        auto& value = (*this->entries)[this->idx].value();
        return std::make_pair(DeviceId(this->idx), std::ref(value));
    }

    DeviceIdMapIter<V>& operator++() {
        if (this->idx >= DeviceId::num_values()) {
            return *this;
        }

        while (true) {
            this->idx++;

            if (this->idx >= DeviceId::num_values()) {
                return *this;
            }

            if ((*this->entries)[this->idx].is_present()) {
                return *this;
            }
        }
    }

    template <typename L, typename R>
    friend bool operator==(const DeviceIdMapIter<L>&, const DeviceIdMapIter<R>&);

  private:
    DeviceIdMapIter(const std::vector<typename DeviceIdMap<V>::Entry>* entries, size_t idx) :
        entries(entries),
        idx(idx) {
        // make sure we start with a valid index (or go all the way to the end if empty)
        if (!(*this->entries)[this->idx].is_present()) {
            ++(*this);
        }
    }

    const std::vector<typename DeviceIdMap<V>::Entry>* entries;
    size_t idx;
};

template <typename L, typename R>
bool operator==(const DeviceIdMapIter<L>& lhs, const DeviceIdMapIter<R>& rhs) {
    return lhs.idx == rhs.idx;
}

template <typename L, typename R>
bool operator!=(const DeviceIdMapIter<L>& lhs, const DeviceIdMapIter<R>& rhs) {
    return !(lhs == rhs);
}

#endif
