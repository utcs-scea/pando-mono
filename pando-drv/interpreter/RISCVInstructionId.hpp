// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef RISCVINSTRUCTIONID_HPP
#define RISCVINSTRUCTIONID_HPP
enum RISCVInstructionId {
#define DEFINSTR(mnemonic, value_under_mask, mask...) mnemonic##InstructionId,
#include "InstructionTable.h"
#undef DEFINSTR
  NumInstructionIds
};
#endif
