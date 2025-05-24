#pragma once
#ifndef _TINY_THREAD_POOL_
#define _TINY_THREAD_POOL_
#include <list>
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


class TINY_THREAD_POOL_EXPORT TinyThreadPool {
public:
    TinyThreadPool() = delete;
    TinyThreadPool(TinyThreadPool&) = delete;
    TinyThreadPool(TinyThreadPool&&) = delete;
    TinyThreadPool& operator=(TinyThreadPool&) = delete;
    TinyThreadPool& operator=(TinyThreadPool&&) = delete;
    TinyThreadPool(int max_worker_num, std::chrono::milliseconds timeout = std::chrono::milliseconds()) noexcept;
    ~TinyThreadPool() noexcept;
    void terminate() noexcept;

    [[nodiscard]] inline size_t thread_num() const noexcept {
        return _threads.size() - _stopped_threads.size();
    }

    template<typename F, typename ...Args>
    requires requires (F&& f, Args&&... args) {
        { std::forward<F>(f)(std::forward<Args>(args)...) };
    }
    [[nodiscard]] auto submit(F&& f, Args&&... args) noexcept {
        using result_type = decltype(std::forward<F>(f)(std::forward<Args>(args)...));
        using task_type = std::packaged_task<result_type()>;
        auto task = std::make_shared<task_type>(
            [f = std::forward<F>(f), ...args = std::forward<Args>(args)]() mutable {
                return std::forward<F>(f)(std::forward<Args>(args)...);
            }
        );
        _tasks.add(
            [task]() {
                (*task)();
            }
        );

        if (!_stopped_threads.empty()) {
            for (auto node : _stopped_threads) {
                if (node->joinable()) {
                    node->join();
                }
                _threads.erase(node);
            }
            _stopped_threads.clear();
        }

        // task maybe already be running
        if (_tasks.empty()) {
            return task->get_future();
        }

        if (_max_worker_num < 0 || (int)_threads.size() < _max_worker_num) {
            if (_timeout.count() > 0) {
                _new_thread<true>();
            } else {
                _new_thread<false>();
            }
        }

        _condition.notify_one();

        return task->get_future();
    }
private:
    std::chrono::milliseconds _timeout { 0 };
    bool _terminated { false };
    int _max_worker_num { -1 };
    std::mutex _condition_mutex {};
    std::condition_variable _condition {};
    std::list<std::thread> _threads {};
    std::vector<std::list<std::thread>::iterator> _stopped_threads {};
    SafeTaskDeque<std::function<void()>> _tasks {};

    template<bool with_timeout>
    void _new_thread() noexcept {
        auto prev = _threads.rbegin();
        _threads.emplace_back(
            [this, prev]() {
                while (true) {
                    while (!_tasks.empty()) {
                        if (auto task = _tasks.get(); task) {
                            (*task)();
                        } else {
                            break;
                        }
                    }
                    {
                        std::unique_lock<std::mutex> lock { _condition_mutex };
                        if (_terminated) {
                            break;
                        }
                        if constexpr (with_timeout) {
                            if (_condition.wait_for(lock, _timeout) == std::cv_status::timeout) {
                                _stopped_threads.push_back(prev.base());
                                return;
                            }
                        } else {
                            _condition.wait(lock);
                        }
                    }
                    while (!_tasks.empty()) {
                        if (auto task = _tasks.get(); task) {
                            (*task)();
                        } else {
                            break;
                        }
                    }
                }
            }
        );
    }
};

#endif
