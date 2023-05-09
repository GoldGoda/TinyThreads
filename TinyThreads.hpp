
#include <iostream>

#include <cstdint>
#include <functional>

#include <vector>
#include <queue>


#include <thread>
#include <mutex>
#include <condition_variable>


namespace tt
{
    

class ThreadPool {

public:
    using ThreadList_t = std::vector<std::thread>;
    using TaskList_t   = std::deque<std::function<void()>>;
    
private:
    //Workers
    ThreadList_t threads_{ std::thread::hardware_concurrency() };

    //tasks
    TaskList_t tasks_;

    //theads manage    
    bool isRunning_{true};
    std::mutex mtx_{};
    std::condition_variable cv_{};


public:
    ThreadPool(std::size_t size = std::thread::hardware_concurrency()) : threads_(size) {  }

    template <typename TASK, typename... ARGS>
    void addTask(TASK &&task, ARGS &&... args) noexcept {
        std::unique_lock<std::mutex> lock(mtx_);
        tasks_.emplace_back(std::bind(std::forward<TASK>(task), std::forward<ARGS>(args)...));
        lock.unlock();
        cv_.notify_one();
    }

    void stop() noexcept {
        isRunning_ = false;
        cv_.notify_all();
    }

    void run() {
        prepareThreadWorkers();
        executeThreadWorkers();
    }

    ~ThreadPool() {
        stop();
    }

private:
    void prepareThreadWorkers() {
        for (auto& worker : threads_) {
            worker = std::thread([this]() {
                while (isRunning_) {    
                    std::unique_lock<std::mutex> lock(mtx_);
                    cv_.wait(lock, [this]() { return !tasks_.empty() || !isRunning_; });
                    
                    if (!isRunning_ && tasks_.empty()) {  return;  }
                        auto task = std::move(tasks_.front());
                        tasks_.pop_front();
                    lock.unlock();
                    task();
                }
            });
        }
    }

    void executeThreadWorkers() {
        for (auto &worker : threads_) {
            if (worker.joinable()) {
                worker.join();
            } else {
                throw std::runtime_error("Thread is not joinable.");
            }
        }
    }
};

} // namespace TinyThreads tt


