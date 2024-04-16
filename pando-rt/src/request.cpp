// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */
#ifdef PANDO_RT_USE_BACKEND_PREP

#include "pando-rt/execution/request.hpp"
#include "pando-rt/stdlib.hpp"

#include "prep/hart_context_fwd.hpp"
#include "prep/nodes.hpp"

namespace pando {

Status detail::RequestBuffer::acquire(NodeIndex nodeIdx, std::size_t size) {
  m_size = size;
  return Nodes::requestAcquire(nodeIdx, size, &m_storage, &m_metadata);
}

void detail::RequestBuffer::release() {
  Nodes::requestRelease(m_size, m_metadata);
  //hartYield();
}

} // namespace pando
#endif
