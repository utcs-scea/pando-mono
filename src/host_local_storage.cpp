// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <pando-lib-galois/containers/host_local_storage.hpp>

pando::NodeSpecificStorage<galois::HostLocalStorageHeap::ModestArray>
    galois::HostLocalStorageHeap::heap;
pando::SlabMemoryResource<galois::HostLocalStorageHeap::Granule>*
    galois::HostLocalStorageHeap::LocalHeapSlab;

void galois::HostLocalStorageHeap::HeapInit() {
  using galois::HostLocalStorageHeap::Granule;
  using galois::HostLocalStorageHeap::heap;
  using galois::HostLocalStorageHeap::LocalHeapSlab;
  using galois::HostLocalStorageHeap::ModestArray;
  using galois::HostLocalStorageHeap::Size;
  pando::GlobalPtr<ModestArray> heapStartTyped = heap.getPointerAt(pando::NodeIndex{0});
  pando::GlobalPtr<void> heapStartNoType = static_cast<pando::GlobalPtr<void>>(heapStartTyped);
  pando::GlobalPtr<std::byte> heapStartByte =
      static_cast<pando::GlobalPtr<std::byte>>(heapStartNoType);
  std::uint64_t diff = heapStartNoType.address % Granule;
  if (diff != 0) {
    heapStartByte += Granule - diff;
  }
  LocalHeapSlab = new pando::SlabMemoryResource<Granule>(heapStartByte, Size);
}
