#ifndef PANDO_RT_SRC_DRVX_TERMINATION_HPP_
#define PANDO_RT_SRC_DRVX_TERMINATION_HPP_

#include <mutex>

extern std::mutex global_mutex;
extern bool terminateFlag;

void setTerminateFlag();

bool getTerminateFlag();

#endif // PANDO_RT_SRC_DRVX_TERMINATION_HPP_