#ifndef OPTION_H
#define OPTION_H

#include <exception>
#include <stddef.h>

class NoneUnwrap : public std::exception {
  public:
    NoneUnwrap() {}

    const char* what() const noexcept override {
        return "unwrapped empty Option";
    }
};

template <typename T>
class Option {
  public:
    Option() : is_some_(false) {}

    Option(const T& val) : some(val), is_some_(true) {}

    Option(const Option<T>& src) : is_some_(src.is_some_) {
        if (src.is_some()) {
            this->some = src.some;
        }
    }

    ~Option() {
        if (this->is_some_) {
            this->some.~T();
        }
    }

    Option<T>& operator=(const Option<T>& rhs) {
        this->is_some_ = rhs.is_some_;

        if (this->is_some()) {
            this->some = rhs.some;
        }

        return *this;
    }

    const T& unwrap() const {
        if (this->is_none()) {
            throw NoneUnwrap();
        }

        return this->some;
    }

    T& unwrap() {
        if (this->is_none()) {
            throw NoneUnwrap();
        }

        return this->some;
    }

    bool is_some() const {
        return this->is_some_;
    }

    bool is_none() const {
        return !this->is_some();
    }

  private:
    union {
        T some;
    };
    bool is_some_;
};

#endif
