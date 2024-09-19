#include <iostream>
#include <chrono>
#include <thread>

#include "tiny_thread_pool.hpp"


int test_sum() {
    int s = 0;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    for (int i = 0; i < 1e8; i++) {
        s += i;
    }
    return s;
}


int main() {
    using namespace std::chrono;
    auto t1 = system_clock::now();
    auto& pool = TinyThreadPool::init(5);
    std::vector<std::future<int>> futs;
    for (int i = 0; i < 8; i++) {
        auto fut = pool.submit(test_sum);
        futs.push_back(std::move(fut));
    }
    for (auto& fut : futs) {
        auto s = fut.get();
        std::cout << "sum = " << s << std::endl;
    }
    pool.terminate();
    auto t2 = system_clock::now();
    auto parallel_time = duration_cast<duration<double>>(t2 - t1);
    std::cout << parallel_time.count() << std::endl;
    t1 = system_clock::now();
    for (int i = 0; i < 8; i++) {
        auto s = test_sum();
        std::cout << "sum = " << s << std::endl;
    }
    t2 = system_clock::now();
    auto time = duration_cast<duration<double>>(t2 - t1);
    std::cout << time.count() << std::endl;
}
