# CppTaskManager

CppTaskManager is a simple thread pool implementation in C++ that allows scheduling tasks for future execution. It supports both one-shot and repeating tasks, with optional conditions to stop repeating.

## Features

- **One-shot tasks**: Schedule a task to run once after a specified delay.
- **Repeating tasks**: Schedule a task to run repeatedly at a specified interval.
- **Conditional repeating tasks**: Schedule a task to run repeatedly at a specified interval, with an optional condition to stop repeating.

## Getting Started

### Prerequisites

- C++17 compatible compiler
- CMake 3.10 or higher
- Ninja build system (optional, but recommended)

### Building

1. Clone the repository:
    ```sh
    git clone https://github.com/yourusername/CppTaskManager.git
    cd CppTaskManager
    ```

2. Configure the proyect and build with CMake:
    ```sh
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --target all
    ```




### Example code
```cpp
#include "TimerThreadPool.h"
#include <iostream>
#include <chrono>

int main()
{
    TimerThreadPool pool(2);

    // 1) One-shot after 1 second
    pool.scheduleOnce([]{
        std::cout << "[One-shot] Runs after 1s.\n";
    }, std::chrono::seconds(1));

    // 2) Another one-shot with no delay
    pool.scheduleOnce([]{
        std::cout << "[One-shot immediate] Runs ASAP.\n";
    });

    // 3) Repeating every 500ms, no condition => infinite (until destructor)
    pool.scheduleRepeating(
        []{
            static int c = 0;
            std::cout << "[Interval=500ms] c=" << c++
                      << " on thread " << std::this_thread::get_id()
                      << "\n";
        },
        std::chrono::milliseconds(0),
        std::chrono::milliseconds(500)
    );

    // 4) Zero-interval repeating with a condition => up to 3 times
    auto zeroCount = std::make_shared<int>(0);
    pool.scheduleRepeating(
        [zeroCount](){
            std::cout << "[Zero Interval] count=" << *zeroCount
                      << " on thread " << std::this_thread::get_id()
                      << "\n";
            (*zeroCount)++;
        },
        std::chrono::milliseconds(0),
        std::chrono::milliseconds(0), // "instant" re-queue
        [zeroCount](){
            return (*zeroCount < 3);
        }
    );

    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Main done, pool destructor will stop.\n";
    return 0;
}
```


### Running the Example

After building the project, you can run the example program:

```sh
./build/CppTaskManagerExample
```
Posible output:
```sh
[One-shot immediate] Runs ASAP.
[Interval=500ms] c=0 on thread 139808845993728
[Zero Interval] count=0 on thread 139808837601024
[Zero Interval] count=1 on thread 139808837601024
[Zero Interval] count=2 on thread 139808837601024
[One-shot] Runs after 1s.
[Interval=500ms] c=1 on thread 139808845993728
[Interval=500ms] c=2 on thread 139808845993728
Main done, pool destructor will stop.

```