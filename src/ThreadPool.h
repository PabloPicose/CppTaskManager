#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <mutex>
#include <chrono>

/**
 * @brief Data structure that holds an individual scheduled task.
 */
struct ScheduledTask
{
    std::function<void()>        func;        ///< The code to run
    std::function<bool()>        condition;   ///< If false => stop repeating
    std::chrono::steady_clock::time_point nextRunTime; ///< When to run
    std::chrono::milliseconds    repeatInterval;        ///< If 0 => immediate re-queue if condition != nullptr

    ScheduledTask(std::function<void()> f,
                  std::function<bool()> cond,
                  std::chrono::steady_clock::time_point start,
                  std::chrono::milliseconds interval)
        : func(std::move(f))
        , condition(std::move(cond))
        , nextRunTime(start)
        , repeatInterval(interval)
    {}
};

/**
 * @brief Comparison functor for the priority queue.
 *
 * We want the *earliest* nextRunTime at the *top* of the queue,
 * so we compare in reverse order (smallest time => highest priority).
 */
struct TaskCompare
{
    bool operator()(const ScheduledTask& lhs, const ScheduledTask& rhs) const
    {
        // "true" => lhs is 'lower priority'
        // so if lhs is scheduled later than rhs => lhs is lower priority
        return lhs.nextRunTime > rhs.nextRunTime;
    }
};

/**
 * @brief Timer-based thread pool that schedules tasks for future execution,
 *        supports repeated tasks, and optionally uses a condition to stop repeating.
 */
class ThreadPool
{
public:
    /**
     * @brief Construct the pool with a given number of worker threads.
     * @param numThreads Number of threads to spawn.
     */
    explicit ThreadPool(std::size_t numThreads);

    /**
     * @brief Destructor joins threads and stops all scheduling.
     */
    ~ThreadPool();

    /**
     * @brief Schedule a one-shot task that runs after the given delay.
     * @param func   The function (lambda) to run.
     * @param delay  How long to wait before first run (default = 0).
     *
     * If @p delay is zero, it runs as soon as possible.
     */
    void scheduleOnce(std::function<void()> func,
                      std::chrono::steady_clock::duration delay = std::chrono::milliseconds(0));

    /**
     * @brief Schedule a repeating task.
     *
     * The task first runs after @p delay, then repeats every @p interval
     * as long as @p condition returns true (if provided). If @p condition is null,
     * it repeats indefinitely.
     *
     * If @p interval == 0 and condition != null, we treat it as 'run as quickly as possible,'
     * re-queueing with a tiny offset so new tasks can jump in.
     *
     * If @p interval == 0 and condition == nullptr => it's effectively a one-shot.
     */
    void scheduleRepeating(std::function<void()> func,
                           std::chrono::steady_clock::duration delay,
                           std::chrono::milliseconds interval,
                           std::function<bool()> condition = nullptr);

private:
    /**
     * @brief The main function each worker thread runs.
     */
    void workerThread();

private:
    std::vector<std::thread>  workers_;
    std::priority_queue<ScheduledTask, std::vector<ScheduledTask>, TaskCompare> taskQueue_;

    std::mutex                queueMutex_;
    std::condition_variable   cv_;
    std::atomic<bool>         stop_{false};
};
