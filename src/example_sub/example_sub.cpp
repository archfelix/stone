#include "stone/stone.hpp"
#include "example_pub/example_pub.hpp"
#include <iostream>

void rgb_handler(const std::shared_ptr<rgb_t> &msg)
{
    printf("Receive: rgb=(%d,%d,%d)\r\n", msg->r, msg->g, msg->b);
}

stone::subscriber<rgb_t> *subscriber1;

void handle()
{
    subscriber1->spin();
}

void example_sub_main()
{
    subscriber1 = stone::subscribe<rgb_t>("color", rgb_handler);
    auto task = stone::make_interval_task(handle);
    stone::scheduleInterval(task, 100_ms);
}
