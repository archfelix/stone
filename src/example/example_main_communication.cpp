#include <iostream>
#include "stone/stone.hpp"

typedef struct _rgb_t
{
    int r, g, b;
} rgb_t;

void rgb_handler(const std::shared_ptr<rgb_t> &msg)
{
    std::cout << msg->r << std::endl
              << msg->g << std::endl
              << msg->b << std::endl
              << std::endl;
}

int main()
{
    auto subscriber1 = stone::subscribe<rgb_t>("test", rgb_handler);

    auto msg = std::make_shared<rgb_t>();
    msg->r = 0;
    msg->g = 1;
    msg->b = 2;
    stone::publish("test", msg);
    subscriber1->spin();
    return 0;
}