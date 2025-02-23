#include "ThreadPool.h"
#include <iostream>
int main() {
  ThreadPool pool(2);

  // 1) One-shot after 1 second
  pool.scheduleOnce([] { std::cout << "[One-shot] Runs after 1s.\n"; },
                    std::chrono::seconds(1));

  // 2) Repeating with 500ms interval, no condition => runs forever
  //    (We'll kill it by letting main end.)
  pool.scheduleRepeating(
      [] {
        static int c = 0;
        std::cout << "[Interval=500ms] c=" << c++ << " on thread "
                  << std::this_thread::get_id() << "\n";
      },
      std::chrono::milliseconds(0), std::chrono::milliseconds(500));

  // 3) Repeating with zero interval => "run as quickly as possible"
  //    but we have a condition "run 5 times total"
  auto zeroCount = std::make_shared<int>(0);
  pool.scheduleRepeating(
      [zeroCount]() {
        std::cout << "[Zero Interval] count=" << *zeroCount << " on thread "
                  << std::this_thread::get_id() << "\n";
        (*zeroCount)++;
      },
      std::chrono::milliseconds(0), // no initial delay
      std::chrono::milliseconds(0), // zero => immediate re-queue
      [zeroCount]() {
        // condition => run while count < 5
        return (*zeroCount < 5);
      });

  std::this_thread::sleep_for(std::chrono::seconds(3));
  std::cout << "Main is done, pool destructor will stop the threads.\n";
  return 0;
}
