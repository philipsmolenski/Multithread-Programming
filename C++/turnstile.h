#ifndef SRC_TURNSTILE_H_
#define SRC_TURNSTILE_H_

#include <cstdint>
#include <type_traits>

class Mutex {
  int32_t threadsInScope;
  int32_t cvId;

 public:
  Mutex();
  Mutex(const Mutex&) = delete;

  void lock();    // NOLINT
  void unlock();  // NOLINT
};

#endif  // SRC_TURNSTILE_H_
