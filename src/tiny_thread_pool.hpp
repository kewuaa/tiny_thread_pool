#pragma once
#ifndef _TINY_THREAD_POOL_
#define _TINY_THREAD_POOL_
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <optional>


template<typename T>
class SafeTaskDeque {
    private:
        std::mutex lock;
        std::deque<T> tasks;
    public:
        SafeTaskDeque() noexcept = default;

        SafeTaskDeque(SafeTaskDeque<T>&& other) noexcept: lock{} {
            tasks = std::move(other.tasks);
        }

        [[nodiscard]] bool empty() noexcept {
            std::lock_guard<std::mutex> guard{lock};
            return tasks.empty();
        }

        [[nodiscard]] size_t size() noexcept {
            std::lock_guard<std::mutex> guard{lock};
            return tasks.size();
        }

        [[nodiscard]] std::optional<T> get() noexcept {
            std::lock_guard<std::mutex> guard{lock};
            if (tasks.empty()) {
                return std::nullopt;
            }
            auto task = std::make_optional(std::move(tasks.front()));
            tasks.pop_front();
            return task;
        }

        void add(T&& task) noexcept {
            std::lock_guard<std::mutex> guard{lock};
            tasks.push_back(task);
        }
};


template<typename T>
struct to_void {
    using type = void;
};
template<typename T>
using to_void_t = typename to_void<T>::type;


class TinyThreadPool {
    private:
        bool terminated;
        std::mutex condition_mutex;
        std::condition_variable condition_lock;
        std::vector<std::thread> threads;
        SafeTaskDeque<std::function<void()>> tasks;
    public:
        TinyThreadPool() = delete;
        TinyThreadPool(TinyThreadPool&) = delete;
        TinyThreadPool(TinyThreadPool&&) = delete;
        TinyThreadPool& operator=(TinyThreadPool&) = delete;
        TinyThreadPool& operator=(TinyThreadPool&&) = delete;
        TinyThreadPool(int max_worker_num) noexcept;
        ~TinyThreadPool() noexcept;
        void terminate() noexcept;
        [[nodiscard]] size_t thread_num() const;

        template<typename F, typename ...Args>
        [[nodiscard]] auto submit(F&& f, Args&&... args) noexcept
        -> std::enable_if_t<
            std::is_same_v<to_void_t<decltype(f(args...))>, void>,
            std::future<decltype(f(args...))>
        > {
            using result_type = decltype(f(args...));
            using task_type = std::packaged_task<result_type(Args...)>;
            auto task = std::make_shared<task_type>(f, std::forward<Args>(args)...);
            tasks.add(
                [task]() {
                    (*task)();
                }
            );
            condition_lock.notify_one();
            return task->get_future();
        }
};
#endif
