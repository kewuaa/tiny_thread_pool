# usage

```cpp
TinyThreadPool(int max_worker_num, std::chrono::milliseconds timeout = std::chrono::milliseconds()) noexcept;
```

With timeout > 0, the thread sleeps for more than timeout and will be automatically recycled and destroyed.
Use `submit(f, args)` to submit work.
