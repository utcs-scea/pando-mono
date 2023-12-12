// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_AGILE_SCHEMA_HPP_
#define PANDO_LIB_GALOIS_UTILITY_AGILE_SCHEMA_HPP_

namespace agile {

enum class TYPES {
  PERSON = 0x1,
  FORUMEVENT = 0x4,
  FORUM = 0x3,
  PUBLICATION = 0x5,
  TOPIC = 0x6,
  PURCHASE,
  SALE,
  AUTHOR,
  WRITTENBY,
  INCLUDES,
  INCLUDEDIN,
  HASTOPIC,
  TOPICIN,
  HASORG,
  ORG_IN,
  DEVICE,
  FRIEND,
  USES,
  COMMUNICATION,
  NONE
};

} // namespace agile

#endif // PANDO_LIB_GALOIS_UTILITY_AGILE_SCHEMA_HPP_
