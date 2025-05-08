#include "stone/stone.hpp"
#include "example_pub/example_pub.hpp"
#include "example_sub/example_sub.hpp"

int main(int argc, char *argv[])
{
    example_pub_main();
    example_sub_main();
    stone::run();
    return 0;
}