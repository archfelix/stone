#include "example_pub.hpp"
#include "stone/stone.hpp"

void publish_task()
{
    auto msg = std::make_shared<rgb_t>();
    msg->r = 100;
    msg->g = 200;
    msg->b = 255;
    printf("Publish: rgb=(%d,%d,%d)\r\n", msg->r, msg->g, msg->b);
    stone::publish("color", msg);
}

void example_pub_main()
{
    auto task = stone::make_interval_task(publish_task);
    stone::scheduleInterval(task, 500_ms);
}