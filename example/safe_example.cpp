
#include "ThreadPool.h"
#include <iostream>
#include <memory>
#include <thread>

/**
 * This examples uses a safe call to a delete object using
 * smart pointers
 */

class MySafeMem : public std::enable_shared_from_this<MySafeMem> {
public:
  MySafeMem(int value) : m_value(value) {}
  ~MySafeMem() = default;

  std::function<void()> getSafeCallback() {
    std::weak_ptr<MySafeMem> weakSelf = shared_from_this();

    return [weakSelf]() {
      if (auto self = weakSelf.lock()) {
        std::cout << "My safe value is: " << self->m_value << std::endl;
      } else {
        std::cout << "Object destroyed, skipping call" << std::endl;
      }
    };
  }

private:
  int m_value;
};

int main() {
  ThreadPool pool(2);

  {
    auto destr_obj = std::make_shared<MySafeMem>(40);
    // Call it after one second
    pool.scheduleOnce(destr_obj->getSafeCallback(), std::chrono::seconds(1));
  }

  auto obj = std::make_shared<MySafeMem>(12);
  pool.scheduleOnce(obj->getSafeCallback(), std::chrono::seconds(1));

  std::this_thread::sleep_for(std::chrono::seconds(2));

  std::cout << "Main thread exiting" << std::endl;
}
