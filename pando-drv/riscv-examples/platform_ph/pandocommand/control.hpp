// SPDX-License-Identifier: MIT
// Copyright (c) 2024 University of Washington
#ifndef PANDOCOMMAND_CONTROL_HPP
#define PANDOCOMMAND_CONTROL_HPP
namespace pandocommand {

/**
 * @brief Reset all the cores
 *
 * @param reset - 0 to reset, 1 to release from reset
 */
void assertResetAll(bool reset);

} // namespace pandocommand
#endif
