#include <cassert>
#include <thread>

#include "tiny_thread_pool.hpp"


TinyThreadPool::TinyThreadPool(int max_worker_num) noexcept:
    _max_worker_num(max_worker_num) {
    _threads.reserve(max_worker_num);
}

TinyThreadPool::~TinyThreadPool() noexcept {
    if (!_terminated) {
        terminate();
    }
}

void TinyThreadPool::terminate() noexcept {
    assert(!_terminated && "the loop is already terminated, do not terminate repeatly");
    {
        std::lock_guard<std::mutex> lock { _condition_mutex };
        _terminated = true;
    }
    _condition.notify_all();
    for (auto& t : _threads) {
        if (t.joinable())
            t.join();
    }
}

void TinyThreadPool::_new_thread() noexcept {
    _threads.emplace_back(
        [this]() {
            while (true) {
                {
                    std::unique_lock<std::mutex> lock { _condition_mutex };
                    if (_terminated) {
                        break;
                    }
                    _condition.wait(lock);
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
