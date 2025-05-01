#pragma once
#ifndef _TINY_THREAD_POOL_
#define _TINY_THREAD_POOL_
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <optional>

#include "tiny_thread_pool_export.hpp"


template<typename T>
class SafeTaskDeque {
public:
    SafeTaskDeque() noexcept = default;

    SafeTaskDeque(SafeTaskDeque<T>&& other) noexcept: _tasks(std::move(other._tasks)) {
        //
    }

    [[nodiscard]] inline bool empty() noexcept {
        return _tasks.empty();
    }

    [[nodiscard]] inline size_t size() noexcept {
        return _tasks.size();
    }

    [[nodiscard]] std::optional<T> get() noexcept {
        std::lock_guard<std::mutex> lock { _mtx };
        if (_tasks.empty()) {
            return std::nullopt;
        }
        auto task = std::make_optional(std::move(_tasks.front()));
        _tasks.pop_front();
        return task;
    }

    void add(T&& task) noexcept {
        std::lock_guard<std::mutex> lock { _mtx };
        _tasks.push_back(std::forward<T>(task));
    }
private:
    std::mutex _mtx {};
    std::deque<T> _tasks {};
};


template<typename T>
struct to_void {
    using type = void;
};
template<typename T>
using to_void_t = typename to_void<T>::type;


class TINY_THREAD_POOL_EXPORT TinyThreadPool {
public:
    TinyThreadPool() = delete;
    TinyThreadPool(TinyThreadPool&) = delete;
    TinyThreadPool(TinyThreadPool&&) = delete;
    TinyThreadPool& operator=(TinyThreadPool&) = delete;
    TinyThreadPool& operator=(TinyThreadPool&&) = delete;
    TinyThreadPool(int max_worker_num) noexcept;
    ~TinyThreadPool() noexcept;
    void terminate() noexcept;

    [[nodiscard]] inline size_t thread_num() const noexcept {
        return _threads.size();
    }

    template<typename F, typename ...Args>
    [[nodiscard]] auto submit(F&& f, Args&&... args) noexcept
    -> std::enable_if_t<
    std::is_same_v<to_void_t<decltype(f(args...))>, void>,
    std::future<decltype(f(args...))>
    > {
        using result_type = decltype(f(args...));
        using task_type = std::packaged_task<result_type(Args...)>;
        auto task = std::make_shared<task_type>(f, std::forward<Args>(args)...);
        _tasks.add(
            [task]() {
                (*task)();
            }
        );
        if (_max_worker_num < 0 || _threads.size() < _max_worker_num) {
            _new_thread();
        }
        _condition.notify_one();
        return task->get_future();
    }
private:
    bool _terminated { false };
    int _max_worker_num { -1 };
    std::mutex _condition_mutex {};
    std::condition_variable _condition {};
    std::vector<std::thread> _threads {};
    SafeTaskDeque<std::function<void()>> _tasks {};

    void _new_thread() noexcept;
};

#endif
