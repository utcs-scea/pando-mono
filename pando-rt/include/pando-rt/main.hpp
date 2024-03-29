// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MAIN_HPP_
#define PANDO_RT_MAIN_HPP_

/**
 * @brief Entry point to user/application code.
 *
 * @note All applications are expected to define/implement this function. Users are expected to call
 *       @ref pando::waitAll within this function before returning for correct execution semantics.
 *       One CP thread per node (PXN) will initialize the PANDO runtime, then begin executing this
 *       function. On runtime initialization, all PandoHammer (PH) cores/harts spin up and wait for
 *       work on their respective queues. The CP may dispatch work to PH harts (local or remote
 *       node). Once `pandoMain` terminates, the CP thread cleans up the runtime and exits.
 *
 * @ingroup ROOT
 */
extern "C" int pandoMain(int argc, char* argv[]);

#endif // PANDO_RT_MAIN_HPP_
