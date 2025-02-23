#include "ThreadPool.h"
//#include <iostream>

ThreadPool::ThreadPool(std::size_t numThreads)
{
    for (std::size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool()
{
    stop_.store(true);
    cv_.notify_all();

    for (auto &t : workers_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void ThreadPool::scheduleOnce(std::function<void()> func,
                                   std::chrono::steady_clock::duration delay)
{
    if (!func) return;

    auto startTime = std::chrono::steady_clock::now() + delay;

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        // condition=null, interval=0 => truly one-shot
        taskQueue_.emplace(std::move(func),
                           nullptr,              // no condition
                           startTime,
                           std::chrono::milliseconds(0)); // 0 interval => one-shot
    }
    cv_.notify_one();
}

void ThreadPool::scheduleRepeating(std::function<void()> func,
                                        std::chrono::steady_clock::duration delay,
                                        std::chrono::milliseconds interval,
                                        std::function<bool()> condition)
{
    if (!func) return;

    auto startTime = std::chrono::steady_clock::now() + delay;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        // If interval==0 but condition==nullptr => effectively one-shot,
        // but let's unify the logic by storing them the same. We'll handle it in worker.
        taskQueue_.emplace(std::move(func),
                           std::move(condition),
                           startTime,
                           interval);
    }
    cv_.notify_one();
}

void ThreadPool::workerThread()
{
    while (!stop_.load()) {
        std::function<void()> taskFunc;
        std::function<bool()> taskCond;
        std::chrono::milliseconds repeatMs(0);

        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            if (taskQueue_.empty()) {
                // Wait until we have a new task or we stop
                cv_.wait(lock, [this](){
                    return stop_.load() || !taskQueue_.empty();
                });
                if (stop_.load()) {
                    return;
                }
            }
            else {
                auto now = std::chrono::steady_clock::now();
                auto &top = taskQueue_.top();

                // If it's not ready yet => timed wait
                if (top.nextRunTime > now) {
                    auto waitDuration = top.nextRunTime - now;
                    cv_.wait_for(lock, waitDuration, [this, &top](){
                        // Wake if new tasks arrived with earlier time, or stop
                        return stop_.load() || taskQueue_.empty()
                               || taskQueue_.top().nextRunTime < top.nextRunTime;
                    });
                    if (stop_.load()) {
                        return;
                    }
                    // re-check in next iteration
                    continue;
                }

                // Now the task is ready
                taskFunc = top.func;
                taskCond = top.condition;
                repeatMs = top.repeatInterval;
                taskQueue_.pop();
            }
        } // unlock

        // Run it outside the lock
        if (taskFunc) {
            taskFunc();
        }

        // Now check if we re-schedule
        if (!stop_.load()) {
            // CASE A: interval > 0 => normal repeating
            // CASE B: interval == 0 and condition != nullptr => zero-interval repeating
            // CASE C: interval == 0 and condition == nullptr => one-shot => do nothing
            if ( (repeatMs.count() > 0) ||
                 ((repeatMs.count() == 0) && taskCond) )
            {
                // We do have a repeating scenario
                bool keepGoing = true;
                if (taskCond) {
                    keepGoing = taskCond(); // check condition
                }
                if (keepGoing) {
                    // If zero interval, we schedule for "now + a tiny offset"
                    auto now = std::chrono::steady_clock::now();
                    auto nextTime = (repeatMs.count() == 0)
                        ? (now + std::chrono::microseconds(1)) // tiny offset
                        : (now + repeatMs);

                    {
                        std::lock_guard<std::mutex> lock(queueMutex_);
                        taskQueue_.emplace(std::move(taskFunc),
                                           std::move(taskCond),
                                           nextTime,
                                           repeatMs);
                    }
                    cv_.notify_one();
                }
            }
            // else => one-shot => do nothing
        }
    }
}
