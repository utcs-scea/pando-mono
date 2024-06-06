#include "termination.hpp"

std::mutex global_mutex;
bool terminateFlag = false;

void setTerminateFlag() {
  std::lock_guard lock{global_mutex};
  terminateFlag = true;
}

bool getTerminateFlag() {
  std::lock_guard lock{global_mutex};
  return terminateFlag;
}