// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_SRC_PREP_HART_CONTEXT_FWD_HPP_
#define PANDO_RT_SRC_PREP_HART_CONTEXT_FWD_HPP_

namespace pando {

struct HartContext;
HartContext* hartContextGet() noexcept;
void hartYield(HartContext& context) noexcept;
void hartYield() noexcept;

/**
 * @brief Yields the hart until @p f() evaluates to true.
 *
 * @ingroup PREP
 */
template <typename F>
void hartYieldUntil(F&& f) {
  auto hartContext = hartContextGet();
  if (hartContext != nullptr) {
    do {
      hartYield(*hartContext);
    } while (f() == false);
  } else {
    while (f() == false) {}
  }
}

} // namespace pando

#endif // PANDO_RT_SRC_PREP_HART_CONTEXT_FWD_HPP_
