#include <cassert>
#include <thread>

#include "tiny_thread_pool.hpp"


TinyThreadPool::TinyThreadPool(int max_worker_num) noexcept {
    _threads.reserve(max_worker_num);
    for (int i = 0; i < max_worker_num; i++) {
        _threads.emplace_back(
            [this]() {
                while (true) {
                    {
                        std::unique_lock<std::mutex> lock { this->_condition_mutex };
                        if (this->_terminated) {
                            break;
                        }
                        this->_condition_lock.wait(lock);
                    }
                    while (true) {
                        if (auto task = this->_tasks.get(); task) {
                            (*task)();
                        } else {
                            break;
                        }
                    }
                }
            }
        );
    }
}

TinyThreadPool::~TinyThreadPool() noexcept {
    if (!_terminated) {
        terminate();
    }
}

void TinyThreadPool::terminate() noexcept {
    assert(!_terminated && "the loop is already terminated, do not terminate repeatly");
    {
        std::unique_lock<std::mutex> lock { _condition_mutex };
        _terminated = true;
    }
    _condition_lock.notify_all();
    for (auto& t : _threads) {
        if (t.joinable())
            t.join();
    }
}
