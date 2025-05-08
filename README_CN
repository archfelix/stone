# 介绍
```txt
   _______________  _   ________
  / ___/_  __/ __ \/ | / / ____/
  \__ \ / / / / / /  |/ / __/   
 ___/ // / / /_/ / /|  / /___   
/____//_/  \____/_/ |_/_____/   
                                
```

"石头",正如其名。该项目准确来说是一种C++项目框架。  
此项目的来源是，在写机器人相关算法的时候，因为各种调度问题使得代码结构非常混乱。如果引入ROS实时性又会降低，提高了系统的复杂度，所以本项目旨在提供一个通用的C++项目框架，考虑了实时性和数据交换。

本项目的核心思想：单进程多线程模型

特征包含：
- 内存消息发布和订阅模型
- 线程池模型
- 带优先级的任务调度
- 带依赖的任务调度
- 时刻任务调度
- 定频任务调度


优势：
- 消息通信高效且延迟稳定
- 线程池调度具有优先级
- 系统资源分配由程序员决定
- 线程切换少

至于抢占式响应，需要根据操作系统本身特点修改。
例如在RT Linux下，用户可自行修改线程优先级。

# 入门

## 项目结构
```txt
- ROOT
  - assets
  - cmake
  - src
    - stone
    - pkg1
    - pkgN
```

项目按照包的形式来管理，包会被编译成静态库文件，给其他包使用。
本项目没有对cmake进行特殊封装，目前还没有必要进行特殊的封装。  
后期如果增加更加复杂的功能，可能会对cmake函数进行封装，或引入Kconfig配置系统。

## 消息通信
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

## 任务调度

### 普通任务

使用底层bind进行创建任务
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

使用更加简单的模板函数来创建任务：
```cpp
int main()
{
    auto [item, future1] = stone::make_task(fn1, 2);
    stone::scheduleNow(item);
}
```

### 依赖任务

创建具有依赖关系的任务，  
依赖关系需要先构建一个依赖关系“图”
依赖关系是分层进行的，例如：
```txt
Level[2]:[task6]
Level[1]:[task3, task4, task5]
Level[0]:[task1, task2]
```

调度时，会先执行level[0]中所有的任务，当level[0]中的所有任务执行完成后，再去执行level[1]中的所有任务。具体代码如下所示。
```cpp
int main(){
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
```

### 时间任务
创建在指定时刻执行：
```cpp
int main(){
    auto [task1, future1] = stone::make_task(fn1, 2);
    stone::scheduleAt(task1, stone::timepoint_shift(1_sec));
}
```

创建按照固定周期执行：
```cpp
int main(){
    auto task1 = stone::make_interval_task(fn2, 2);
    stone::scheduleInterval(task1, 100_us);
}
```