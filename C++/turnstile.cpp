#include "turnstile.h"
#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>
#include <vector>

std::mutex mut;

struct VectorValue {
  std::unique_ptr<std::condition_variable> cv{new std::condition_variable};
  int waiting_num = 0;
  bool occupied = true;
};

std::vector<VectorValue> locks;
std::vector<bool> freeIds;

int addLock() {
  for (size_t i = 0; i < freeIds.size(); i++) {
    if (freeIds[i] == true) {
      freeIds[i] = false;
      locks[i].occupied = true;
      return i;
    }
  }

  freeIds.emplace_back(false);
  locks.emplace_back();
  return locks.size() - 1;
}

Mutex::Mutex() : threadsInScope(0), cvId(-1) {}
void Mutex::lock() {
  std::unique_lock<std::mutex> lk(mut);
  threadsInScope++;
  if (threadsInScope == 1) return;
  if (cvId == -1) cvId = addLock();

  locks[cvId].waiting_num++;
  while (locks[cvId].occupied) locks[cvId].cv->wait(lk);
  locks[cvId].waiting_num--;
  locks[cvId].occupied = true;

  if (locks[cvId].waiting_num == 0) {
    freeIds[cvId] = true;
    cvId = -1;
  }
}

void Mutex::unlock() {
  std::unique_lock<std::mutex> lk(mut);
  threadsInScope--;
  if (cvId == -1) return;
  locks[cvId].occupied = false;
  locks[cvId].cv->notify_one();
}
