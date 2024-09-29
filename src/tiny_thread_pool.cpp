#include <cassert>
#include <thread>

#include "tiny_thread_pool.hpp"


TinyThreadPool::TinyThreadPool(
    int max_worker_num
) noexcept: terminated{false}, threads{}, tasks{} {
    threads.reserve(max_worker_num);
    for (int i = 0; i < max_worker_num; i++) {
        threads.emplace_back(
            [this]() {
                while (true) {
                    auto task = this->tasks.get();
                    if (task) {
                        (*task)();
                    } else {
                        std::unique_lock<std::mutex> lock{this->condition_mutex};
                        if (this->terminated) {
                            break;
                        }
                        this->condition_lock.wait(lock);
                    }
                }
            }
        );
    }
}

TinyThreadPool::~TinyThreadPool() noexcept {
    if (!terminated) {
        terminate();
    }
}

void TinyThreadPool::terminate() noexcept {
    assert(!terminated && "the loop is already terminated, do not terminate repeatly");
    {
        std::lock_guard<std::mutex> lock{condition_mutex};
        terminated = true;
    }
    condition_lock.notify_all();
    for (auto& t : threads) {
        if (t.joinable())
            t.join();
    }
}

size_t TinyThreadPool::thread_num() const {
    return threads.size();
}
