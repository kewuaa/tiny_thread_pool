#include <thread>

#include "tiny_thread_pool.hpp"


TinyThreadPool::TinyThreadPool(
    int max_worker_num
): terminated{false}, threads{}, tasks{} {
    threads.reserve(max_worker_num);
    for (int i = 0; i < max_worker_num; i++) {
        threads.emplace_back(
            [this]() {
                while (!this->terminated) {
                    auto task = this->tasks.get();
                    if (!task) {
                        std::unique_lock<std::mutex> lock{this->condition_mutex};
                        this->condition_lock.wait(lock);
                        continue;
                    }
                    (*task)();
                }
            }
        );
    }
}

TinyThreadPool::~TinyThreadPool() {
    if (!terminated) {
        terminate();
    }
}

void TinyThreadPool::terminate() {
    terminated = true;
    condition_lock.notify_all();
    for (auto& t : threads) {
        if (t.joinable())
            t.join();
    }
}

TinyThreadPool& TinyThreadPool::init(int max_worker_num) {
    static TinyThreadPool pool{max_worker_num};
    return pool;
}
