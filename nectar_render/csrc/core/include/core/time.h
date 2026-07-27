#pragma once

#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <condition_variable>

class Timer {
public:

    __host__ ~Timer() { stop(); }

    __host__ Timer(size_t interval, const bool repeating = true) 
      : interval(std::chrono::milliseconds(interval)), 
        repeating(repeating) 
    { }

    __host__ void start() {
        if (running.exchange(true)) return;
        worker = std::thread(&Timer::run, this);
    }

    __host__ void stop() {
        if (!running.exchange(false)) return;
        condition.notify_all();
        if (worker.joinable()) worker.join();
    }

    __host__ void register_callback(std::function<void()> callback) {
        callbacks.push_back(std::move(callback));
    }

private:

    std::chrono::milliseconds interval;
    bool repeating;

    std::vector<std::function<void()>> callbacks{};
    std::atomic<bool> running { false };
    
    std::mutex mutex;
    std::thread worker;
    std::condition_variable condition;

    __host__ void run() {
        std::unique_lock<std::mutex> lock(mutex);
        while (running.load()) {
            bool finished = condition.wait_for(
                lock, interval, [this] { return !running.load(); }
            );
            if (finished) break;
            lock.unlock(); 
            for (std::function<void()> callback : callbacks) callback();
            lock.lock();
            if (!repeating) stop();
        }
    }

};


class Ticker {
public:

    __host__ explicit Ticker(std::chrono::milliseconds interval)
        : interval(interval), last(std::chrono::steady_clock::now()) {}

    bool poll() {
        auto now = std::chrono::steady_clock::now();
        if (now - last >= interval) { last = now; return true; }
        return false;
    }

private:

    std::chrono::milliseconds interval;
    std::chrono::steady_clock::time_point last;

};


