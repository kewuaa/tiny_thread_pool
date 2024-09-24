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


class TinyThreadPool {
    private:
        bool terminated;
        std::mutex condition_mutex;
        std::condition_variable condition_lock;
        std::vector<std::thread> threads;
        SafeTaskDeque<std::function<void()>> tasks;

        TinyThreadPool(TinyThreadPool&) noexcept = delete;
        TinyThreadPool& operator=(TinyThreadPool&) noexcept = delete;
    public:
        TinyThreadPool(int max_worker_num) noexcept;
        ~TinyThreadPool() noexcept;
        void terminate() noexcept;
        [[nodiscard]] size_t thread_num() const;

        template<typename F, typename ...Args>
        [[nodiscard]] auto submit(F&& f, Args&&... args) noexcept -> std::future<decltype(f(args...))> {
            std::function<decltype(f(args...))()> func =
                std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            auto task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
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
