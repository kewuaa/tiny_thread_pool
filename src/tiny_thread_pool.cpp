#include <cassert>
#include <thread>

#include "tiny_thread_pool.hpp"


TinyThreadPool::TinyThreadPool(int max_worker_num, std::chrono::milliseconds timeout) noexcept:
    _max_worker_num(max_worker_num),
    _timeout(timeout) {}

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
