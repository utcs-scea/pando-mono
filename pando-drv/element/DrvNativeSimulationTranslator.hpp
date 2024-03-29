// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>
namespace SST {
namespace Drv {
// translate simulation system types to
// native environment types
class DrvNativeSimulationTranslator {
public:
  DrvNativeSimulationTranslator() {}

  std::vector<unsigned char> nativeToSimulator_stat(const struct stat*);
  int simulatorToNative_openflags(int32_t sim_openflags);
};
} // namespace Drv
} // namespace SST
