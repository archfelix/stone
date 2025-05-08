#include "scheduler.hpp"

namespace stone
{
    ThreadPool defaultPool(THREAD_POOL_SIZE);
    Scheduler defaultScheduler(&defaultPool);
} // namespace stone
