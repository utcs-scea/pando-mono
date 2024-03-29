// SPDX-License-Identifier: MIT
// Copyright (c) 2024 University of Washington
#ifndef PANDOCOMMAND_DEBUG_HPP
#define PANDOCOMMAND_DEBUG_HPP

#ifdef PANDOCOMMAND_DEBUG
#define cmd_dbg(fmt, ...)                                               \
    do {printf("[COMMAND PROCESSOR DEBUG]: " fmt, ##__VA_ARGS__);} while (0)
#else
#define cmd_dbg(fmt, ...)                                               \
    do {} while (0)
#endif
#endif // PANDOCOMMAND_DEBUG_HPP
