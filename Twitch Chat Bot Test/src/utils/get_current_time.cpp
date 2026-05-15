#include "utils/get_current_time.h"

std::chrono::milliseconds::rep get_current_time() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
};