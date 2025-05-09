# Introduction

```
   _______________  _   ________
  / ___/_  __/ __ \/ | / / ____/
  \__ \ / / / / / /  |/ / __/   
 ___/ // / / /_/ / /|  / /___   
/____//_/  \____/_/ |_/_____/   
                                 
```

"Stone", as the name suggests, is a C++ project framework. The project originated from robot algorithm development, where scheduling issues caused chaotic code structures. Introducing ROS would reduce real-time performance and increase system complexity. Therefore, this project aims to provide a general-purpose C++ framework that considers real-time performance and data exchange.

The core concept of this project: Single-process multi-thread model

Features include:
- In-memory message publishing and subscription model
- Thread-pool model
- Priority-based task scheduling
- Dependency-based task scheduling
- Moment-based task scheduling
- Fixed-frequency task scheduling
- Event Trigger task scheduling

Advantages:
- Efficient and stable-latency message communication
- Priority-based thread-pool scheduling
- Programmers decide on system resource allocation
- Fewer thread switches

As for preemptive response, it depends on the operating system's characteristics and can be modified accordingly. For example, on RT Linux, users can modify thread priorities as needed.

# Getting Started

## Project Structure
```txt
- ROOT
  - assets
  - cmake
  - src
    - stone
    - pkg1
    - pkgN
```

The project is package-based. Packages are compiled into static libraries for other packages to use. CMake hasn't been specially encapsulated yet, as it's currently unnecessary. If more complex features are added later, CMake functions may be encapsulated or a Kconfig configuration system may be introduced.

## Message Communication

```cpp
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
```

## Task Scheduling

### Regular Tasks

Create tasks using bind:
```cpp
int fn1(int a)
{
    printf("a=%d\n", a);
    return a * a;
}

int main()
{
    auto item = std::make_shared<stone::WorkItem>();
    auto future1 = item->bind(fn1, 2);
    stone::scheduleNow(item);
}
```

Create tasks with a simpler template function:
```cpp
int main()
{
    auto [item, future1] = stone::make_once_task(fn1, 2);
    stone::scheduleNow(item);
}
```

### Dependent Tasks

Create tasks with dependencies. Dependencies form a layered "graph":
```
Level[2]:[task6]
Level[1]:[task3, task4, task5]
Level[0]:[task1, task2]
```

During scheduling, tasks in level[0] run first. After all level[0] tasks complete, level[1] tasks start. Here's the code:
```cpp
int main(){
    stone::WorkItemFlow flow(2);
    auto [task1, future1] = stone::make_once_task(fn1, 2);
    auto [task2, future2] = stone::make_once_task(fn1, 3);
    auto [task3, future3] = stone::make_once_task(fn1, 4);

    flow.add(0, task1);
    flow.add(1, task2);
    flow.add(1, task3);

    flow.finish();

    stone::scheduleNow(flow);
}
```

### Timed Tasks

Execute at a specific time:
```cpp
int main(){
    auto [task1, future1] = stone::make_once_task(fn1, 2);
    stone::scheduleAt(task1, stone::timepoint_shift(1_sec));
}
```

Execute at fixed intervals:
```cpp
int main(){
    auto task1 = stone::make_interval_task(fn2, 2);
    stone::scheduleInterval(task1, 100_us);
}
```

### Event Task
Create an event task:
```cpp
int main(){
    auto task1 = stone::make_event_task(fn2, 2);
    stone::scheduleEvent(task1, "event_name");
}
```

On the other place, `emitEvent` can be used to wake up the tasks related with the event.
```cpp
int main(){
    stone::emitEvent("event_name");
}
```