#pragma once


#include <type_traits>
#include <utility>
#include <memory>
#include <uv.h>
#include "resource.hpp"


namespace uvw {


template<typename T, typename U>
class Request: public Resource<T, U> {
protected:
    using Resource<T, U>::Resource;

    template<typename R, typename E>
    static void defaultCallback(R *req, int status) {
        T &res = *(static_cast<T*>(req->data));

        auto ptr = res.shared_from_this();
        (void)ptr;

        res.reset();

        if(status) {
            res.publish(ErrorEvent{status});
        } else {
            res.publish(E{});
        }
    }

    template<typename F, typename... Args>
    auto invoke(F &&f, Args&&... args)
    -> std::enable_if_t<not std::is_void<std::result_of_t<F(Args...)>>::value, int> {
        auto err = std::forward<F>(f)(std::forward<Args>(args)...);
        if(err) { Emitter<T>::publish(ErrorEvent{err}); }
        else { this->leak(); }
        return err;
    }

    template<typename F, typename... Args>
    auto invoke(F &&f, Args&&... args)
    -> std::enable_if_t<std::is_void<std::result_of_t<F(Args...)>>::value> {
        std::forward<F>(f)(std::forward<Args>(args)...);
        this->leak();
    }

public:
    void cancel() {
        auto err = uv_cancel(this->template get<uv_req_t>());
        if(err) { Emitter<T>::publish(ErrorEvent{err}); }
    }

    std::size_t size() const noexcept {
        return uv_req_size(this->template get<uv_req_t>()->type);
    }
};


}
