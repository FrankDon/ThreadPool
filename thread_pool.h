//
// Created by frank on 2020/3/22.
//
#ifndef THREADPOOL_THREAD_POOL_H
#define THREADPOOL_THREAD_POOL_H

#include <atomic>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>

class ThreadPool {
public:
    ThreadPool(size_t size);
    ~ThreadPool();
    template <class F, class... Args>
    void push(F &&f, Args &&...args);

private:
    // flag for stop
    std::atomic<bool> _stop;
    // workers
    std::vector<std::thread> _workers;
    // tasks
    std::queue<std::function<void()>> _task_queue;
    // mutex
    std::mutex _task_mutex;
};

ThreadPool::ThreadPool(size_t size) {
    _stop.store(false);

    for(size_t i = 0; i < size; ++i) {
        _workers.emplace_back([this](){
            thread_local std::function<void()> thread_task = nullptr;

            while(!_stop.load() || !_task_queue.empty()) {
                // get task from queue
                _task_mutex.lock();
                if (!_task_queue.empty()) {
                    thread_task = _task_queue.front();
                    _task_queue.pop();
                }
                _task_mutex.unlock();

                if (thread_task != nullptr) {
                    thread_task();
                    thread_task = nullptr;
                }
            }
        });
    }
}

template <class F, class...Args>
void ThreadPool::push(F &&f, Args &&...args) {
    if (_stop.load()) {
        return;
    }
    _task_mutex.lock();
    _task_queue.push(std::move(std::bind(f, std::forward<Args>(args)...)));
    _task_mutex.unlock();
}

ThreadPool::~ThreadPool() {
    _stop.store(true);

    for(auto &w : _workers) {
        w.join();
    }
}



#endif //THREADPOOL_THREAD_POOL_H
