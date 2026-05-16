#include "utils/get_current_time_ms/get_current_time_ms.h"

std::chrono::milliseconds::rep get_current_time_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
};