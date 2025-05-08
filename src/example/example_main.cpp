#include <iostream>
#include "stone/stone.hpp"

//
// simple example
//

int fn1(int a)
{
    printf("a=%d\n", a);
    return a * a;
}

/// @brief A basic usage
void test1()
{
    auto item = std::make_shared<stone::WorkItem>();
    auto future1 = item->bind(fn1, 2);
    stone::scheduleNow(item);
}

/// @brief a convinent way to use
void test2()
{
    auto [item, future1] = stone::make_task(fn1, 2);
    stone::scheduleNow(item);
}

/// @brief tasks with dependencies
void test3()
{
    stone::WorkItemFlow flow(2);
    auto [task1, future1] = stone::make_task(fn1, 2);
    auto [task2, future2] = stone::make_task(fn1, 3);
    auto [task3, future3] = stone::make_task(fn1, 4);

    flow.add(0, task1);
    flow.add(1, task2);
    flow.add(1, task3);

    flow.finish();

    stone::scheduleNow(flow);
}

/// @brief The usage of member funtion.
class test4
{
private:
    /* data */
public:
    int fn(int a)
    {
        printf("a=%d\n", a);
        return a * a;
    }
    test4()
    {
        auto [task1, future1] = stone::make_task(&test4::fn, this, 2);
        stone::scheduleNow(task1);
        std::cout << future1.get() << std::endl;
    }
    ~test4() {}
};

/// @brief delay schedule
void test5()
{
    auto [task1, future1] = stone::make_task(fn1, 2);
    stone::scheduleAt(task1, stone::timepoint_shift(1_sec));
    // std::cout << future1.get() << std::endl;
}

std::chrono::steady_clock::time_point t0 = stone::timepoint_now();
std::chrono::steady_clock::time_point t1 = stone::timepoint_now();

void fn2(int x)
{
    t1 = stone::timepoint_now();
    auto delay = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    std::cout << "x=" << x << "  delay=" << delay << std::endl;
    t0 = t1;
}

/// @brief interval schedule
void test6()
{
    auto task1 = stone::make_interval_task(fn2, 2);
    stone::scheduleInterval(task1, 100_us);
}

int main()
{
    test6();
    stone::run();
    return 0;
}