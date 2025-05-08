#ifndef STONE_SCHEDULER_HPP
#define STONE_SCHEDULER_HPP

#include <thread>
#include <condition_variable>
#include <vector>
#include <functional>
#include <queue>
#include <deque>
#include <future>
#include <chrono>
#include <map>
#include <unordered_map>
#include <tuple>

#include "stoneconfig.hpp"

constexpr unsigned long long operator"" _us(unsigned long long value)
{
    return value;
}

constexpr unsigned long long operator"" _ms(unsigned long long value)
{
    return value * 1000;
}

constexpr unsigned long long operator"" _sec(unsigned long long value)
{
    return value * 1000000;
}

namespace stone
{
    inline auto timepoint_now()
    {
        return std::chrono::high_resolution_clock::now();
    }

    inline auto timepoint_shift(unsigned long long us)
    {
        auto tp = timepoint_now() + std::chrono::microseconds(us);
        return tp;
    }

    class WorkItem
    {
        friend class ThreadPool;
        friend class Scheduler;
        friend class WorkItemFlow;

    public:
        enum class ScheduleType
        {
            ONCE,
            INTERVAL,
        };

        std::function<void()> fn;

        WorkItem() {}
        ~WorkItem() {}

        template <class F, class... Args>
        auto bind(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
        {
            using return_type = typename std::result_of<F(Args...)>::type;
            auto _fn = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            auto future = _fn->get_future();
            this->fn = [_fn]()
            {
                (*_fn)();
            };
            this->schedule_type = ScheduleType::ONCE;
            return future;
        }

        template <class F, class... Args>
        void bind_interval(F &&f, Args &&...args)
        {
            auto _fn = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            this->fn = [_fn]()
            {
                _fn();
            };
            this->schedule_type = ScheduleType::INTERVAL;
            this->interval_stop = false;
        }

        void set_priority(std::size_t priority)
        {
            this->priority = priority;
        }

        void clear_interval()
        {
            this->interval_stop = true;
        }

    private:
        void add_dependency(const std::shared_ptr<WorkItem> &workitem)
        {
            this->dependencies_count++;
            workitem->super_dependencies.push_back(this);
        }
        // priority
        std::size_t priority = 0;

        // used for waking up the task
        std::size_t dependencies_count = 0;

        // record the tasks which depend on this.
        // the tasks in this vector, must have not been called.
        std::vector<WorkItem *> super_dependencies;

        // indicate the schedule type
        ScheduleType schedule_type = ScheduleType::ONCE;

        // used in DELAY and INTERVAL
        std::chrono::steady_clock::time_point wakeup_time;

        // used in INTERVAL
        volatile bool interval_stop = false;
        std::chrono::microseconds interval_us = std::chrono::microseconds(0);

        // needed by scheduling.
        std::function<void(const std::shared_ptr<WorkItem> &)> fn_done;
    };

    template <class F, class... Args>
    inline auto make_task(F &&f, Args &&...args)
        -> std::tuple<std::shared_ptr<WorkItem>,
                      std::future<typename std::result_of<F(Args...)>::type>>
    {
        auto task = std::make_shared<WorkItem>();
        auto future = task->bind(f, args...);
        return std::make_tuple(std::move(task), std::move(future));
    }

    template <class F, class... Args>
    inline std::shared_ptr<WorkItem> make_interval_task(F &&f, Args &&...args)
    {
        auto task = std::make_shared<WorkItem>();
        task->bind_interval(f, args...);
        return task;
    }

    class ThreadPool
    {
    private:
        std::vector<std::thread> _threads;
        // std::queue<std::shared_ptr<WorkItem>> work_queue;
        std::multimap<std::size_t, std::shared_ptr<WorkItem>> work_queue;
        std::mutex work_queue_mtx;
        std::condition_variable work_queue_cv;
        volatile bool stop = false;

        void worker_loop()
        {
            while (true)
            {
                std::shared_ptr<WorkItem> item;
                {
                    std::unique_lock<std::mutex> ulock(work_queue_mtx);
                    work_queue_cv.wait(ulock, [this]
                                       { return stop || !work_queue.empty(); });
                    if (stop)
                    {
                        return;
                    }
                    item = work_queue.begin()->second;
                    work_queue.erase(work_queue.begin());
                }
                if (item->fn)
                {
                    item->fn();
                }
                if (item->fn_done)
                {
                    item->fn_done(item);
                }
            }
        }

    public:
        ThreadPool(std::size_t count)
        {
            this->initThreads(count);
        }

        ThreadPool() {}

        ~ThreadPool()
        {
            this->shutdown();
        }

        void initThreads(std::size_t count)
        {
            _threads.reserve(count);
            for (size_t i = 0; i < count; i++)
            {
                _threads.push_back(std::thread(&ThreadPool::worker_loop, this));
            }
        }

        void shutdown()
        {
            this->stop = true;
            work_queue_cv.notify_all();
            for (auto &&t : _threads)
            {
                t.join();
            }
        }

        void push(const std::shared_ptr<WorkItem> &item)
        {
            {
                std::lock_guard<std::mutex> glock(work_queue_mtx);
                this->work_queue.insert({item->priority, item});
            }
            work_queue_cv.notify_one();
        }
    };

    class WorkItemFlow
    {
        friend class Scheduler;

    private:
        bool finished_flag = false;
        std::size_t priority = 20;
        std::vector<std::deque<std::shared_ptr<WorkItem>>> levels;

    public:
        WorkItemFlow(std::size_t level_count, std::size_t priority = 20)
        {
            if (level_count < 2)
            {
                level_count = 2;
            }
            levels.resize(level_count);
            this->priority = priority;
        }

        ~WorkItemFlow() {}

        bool finished() const
        {
            return this->finished_flag;
        }

        // the smaller the level, the earlier it will be executed.
        bool add(std::size_t level, const std::shared_ptr<WorkItem> &item)
        {
            if (item->schedule_type != WorkItem::ScheduleType::ONCE)
            {
                return false;
            }
            if (finished_flag)
            {
                return false;
            }
            if (level >= levels.size())
            {
                return false;
            }
            item->set_priority(this->priority);
            levels.at(level).push_back(item);
            return true;
        }

        bool del(std::size_t level)
        {
            if (level >= levels.size())
            {
                return false;
            }
            levels.at(level).clear();
        }

        bool del(const std::shared_ptr<WorkItem> &item)
        {
            for (auto &&l : levels)
            {
                decltype(l.end()) todel = l.end();
                for (auto i = l.begin(); i != l.end(); i++)
                {
                    if ((*i) == item)
                    {
                        todel = i;
                        break;
                    }
                }
                if (todel == l.end())
                {
                    return false;
                }
                else
                {
                    l.erase(todel);
                    return true;
                }
            }
        }

        bool del(std::size_t level, const std::shared_ptr<WorkItem> &item)
        {
            if (level >= levels.size())
            {
                return false;
            }
            auto &l = levels.at(level);
            decltype(l.end()) todel = l.end();
            for (auto i = l.begin(); i != l.end(); i++)
            {
                if ((*i) == item)
                {
                    todel = i;
                    break;
                }
            }
            if (todel == l.end())
            {
                return false;
            }
            else
            {
                l.erase(todel);
                return true;
            }
        }

        void finish()
        {
            for (size_t i = 1; i < levels.size(); i++)
            {
                for (auto &&super_item : levels.at(i))
                {
                    for (auto &&item : levels.at(i - 1))
                    {
                        super_item->add_dependency(item);
                    }
                }
            }
            finished_flag = true;
        }
    };

    class Scheduler
    {
    private:
        ThreadPool *pool;
        std::thread th_schedule;

        std::mutex sleep_items_mtx;
        std::unordered_map<WorkItem *, std::shared_ptr<WorkItem>> sleep_items;

        std::condition_variable timed_items_cv;
        std::mutex timed_items_mtx;
        std::multimap<std::chrono::steady_clock::time_point, std::shared_ptr<WorkItem>> timed_items;

        volatile bool stop = false;

        void work_done_handler(const std::shared_ptr<WorkItem> &item)
        {
            // wake up the super tasks
            for (auto &&i : item->super_dependencies)
            {
                i->dependencies_count--;
                if (i->dependencies_count == 0)
                {
                    // ensure that the super task exists
                    if (sleep_items.find(i) != sleep_items.end())
                    {
                        pool->push(sleep_items[i]);
                        sleep_items.erase(i);
                    }
                }
            }

            // interval schedule
            if (item->schedule_type == WorkItem::ScheduleType::INTERVAL)
            {
                item->wakeup_time = timepoint_now() + item->interval_us;
                std::lock_guard<std::mutex> glock(timed_items_mtx);
                timed_items.insert({item->wakeup_time, item});
                timed_items_cv.notify_one();
            }
        }

    public:
        Scheduler(ThreadPool *pool) : pool(pool) {}
        ~Scheduler()
        {
            this->shutdown();
        }

        void shutdown()
        {
            this->stop = true;
        }

        void run()
        {
            while (true)
            {
                std::shared_ptr<stone::WorkItem> item = nullptr;
                if (timed_items.empty())
                {
                    std::unique_lock<std::mutex> ulock(timed_items_mtx);
                    timed_items_cv.wait(ulock, [this]
                                        { return stop || !timed_items.empty(); });
                    if (stop)
                    {
                        return;
                    }
                    continue;
                }

                auto current_tp = timepoint_now();
                long long microseconds_diff = 0;
                {
                    std::lock_guard<std::mutex> glock(timed_items_mtx);
                    microseconds_diff = std::chrono::duration_cast<std::chrono::microseconds>(
                                            timed_items.begin()->first - current_tp)
                                            .count();
                }

                if (microseconds_diff > 0)
                {
                    std::unique_lock<std::mutex> ulock(timed_items_mtx);
                    timed_items_cv.wait_for(ulock,
                                            std::chrono::microseconds(microseconds_diff),
                                            [this]
                                            { return stop || !timed_items.empty(); });
                    continue;
                }
                else
                {
                    std::lock_guard<std::mutex> glock(timed_items_mtx);
                    auto upper = timed_items.upper_bound(current_tp);
                    for (auto i = timed_items.begin(); i != upper;)
                    {
                        if (!i->second->interval_stop)
                        {
                            this->pool->push(i->second);
                        }
                        i = timed_items.erase(i);
                    }
                }
            }
        }

        bool scheduleNow(const WorkItemFlow &flow)
        {
            std::lock_guard<std::mutex> glock(sleep_items_mtx);
            if (!flow.finished())
            {
                return false;
            }
            for (auto i = flow.levels.rbegin(); i != flow.levels.rend(); i++)
            {
                if (i == (flow.levels.rend() - 1))
                {
                    for (auto &&item : (*i))
                    {
                        item->fn_done = std::bind(&Scheduler::work_done_handler, this, std::placeholders::_1);
                        pool->push(item);
                    }
                }
                else
                {
                    for (auto &&item : (*i))
                    {
                        item->fn_done = std::bind(&Scheduler::work_done_handler, this, std::placeholders::_1);
                        sleep_items[item.get()] = item;
                    }
                }
            }
            return true;
        }

        bool scheduleNow(const std::shared_ptr<WorkItem> &item)
        {
            if (item->schedule_type != WorkItem::ScheduleType::ONCE)
            {
                return false;
            }
            item->fn_done = nullptr;
            if (item->dependencies_count == 0)
            {
                pool->push(item);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool scheduleAt(const std::shared_ptr<WorkItem> &item,
                        const std::chrono::steady_clock::time_point &tp)
        {
            if (item->schedule_type != WorkItem::ScheduleType::ONCE)
            {
                return false;
            }
            item->fn_done = std::bind(&Scheduler::work_done_handler, this, std::placeholders::_1);
            item->wakeup_time = tp;
            std::lock_guard<std::mutex> glock(timed_items_mtx);
            timed_items.insert({item->wakeup_time, item});
            timed_items_cv.notify_one();
            return true;
        }

        bool scheduleInterval(const std::shared_ptr<WorkItem> &item,
                              unsigned long long interval_us)
        {
            if (item->schedule_type != WorkItem::ScheduleType::INTERVAL)
            {
                return false;
            }
            item->fn_done = std::bind(&Scheduler::work_done_handler, this, std::placeholders::_1);
            item->interval_us = std::chrono::microseconds(interval_us);
            pool->push(item);
            return true;
        }
    };

    extern Scheduler defaultScheduler;

    inline void run()
    {
        defaultScheduler.run();
    }

    inline bool scheduleNow(const WorkItemFlow &flow)
    {
        return defaultScheduler.scheduleNow(flow);
    }

    inline bool scheduleNow(const std::shared_ptr<WorkItem> &item)
    {
        return defaultScheduler.scheduleNow(item);
    }

    inline bool scheduleAt(const std::shared_ptr<WorkItem> &item,
                           const std::chrono::steady_clock::time_point &tp)
    {
        return defaultScheduler.scheduleAt(item, tp);
    }

    inline bool scheduleInterval(const std::shared_ptr<WorkItem> &item,
                                 unsigned long long interval_us)
    {
        return defaultScheduler.scheduleInterval(item, interval_us);
    }
} // namespace stone

#endif