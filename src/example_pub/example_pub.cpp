#include "example_pub.hpp"
#include "stone/stone.hpp"

std::chrono::steady_clock::time_point t0;
std::chrono::steady_clock::time_point t1;

void publish_task()
{
    // t1 = stone::timepoint_now();
    // printf("Time=%lld \r\n", std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    // t0 = t1;
    auto msg = std::make_shared<rgb_t>();
    msg->r = 100;
    msg->g = 200;
    msg->b = 255;
    stone::publish("color", msg);
    printf("Publish: rgb=(%d,%d,%d)\r\n", msg->r, msg->g, msg->b);
    stone::emitEvent("color_event");
}

void example_pub_main()
{
    auto task = stone::make_interval_task(publish_task);
    stone::scheduleInterval(task, 100_ms);
}