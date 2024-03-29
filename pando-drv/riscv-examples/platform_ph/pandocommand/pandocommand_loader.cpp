// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2024 University of Washington
#include <DrvAPI.hpp>
#include <inttypes.h>
#include <pandocommand/debug.hpp>
#include <pandocommand/executable.hpp>
#include <pandocommand/loader.hpp>
#include <stdio.h>
#include <string.h>
using namespace DrvAPI;
using namespace pandocommand;

namespace pandocommand {
void loadProgramSegment(PANDOHammerExe& executable, Elf64_Phdr* phdr, DrvAPIAddress segpaddr) {
  DrvAPIVAddress vaddr{segpaddr};
  cmd_dbg("Loading segment @ 0x%016" PRIx64 " (%s)\n", segpaddr, vaddr.to_string().c_str());
  size_t segsz = phdr->p_filesz;
  static constexpr size_t MAX_REQSZ = 64;
  char* data = executable.segment_data(phdr);
  size_t off = 0;
  // send segment data
  for (; off < phdr->p_filesz && (off % sizeof(uint64_t)); off += 1) {
    DrvAPI::write<char>(segpaddr + off, data[off]);
  }
  for (; off + MAX_REQSZ <= phdr->p_filesz; off += MAX_REQSZ) {
    std::array<char, MAX_REQSZ> request;
    memcpy(&request[0], &data[off], MAX_REQSZ);
    DrvAPI::write(segpaddr + off, request);
  }
  for (; off < phdr->p_filesz; off += 1) {
    DrvAPI::write(segpaddr + off, data[off]);
  }
  // send zero data
  for (; off < phdr->p_memsz && (off % sizeof(uint64_t)); off += 1) {
    DrvAPI::write<char>(segpaddr + off, 0);
  }
  for (; off + MAX_REQSZ <= phdr->p_memsz; off += MAX_REQSZ) {
    std::array<char, MAX_REQSZ> request;
    request.fill(0);
    DrvAPI::write(segpaddr + off, request);
  }
  for (; off < phdr->p_memsz; off += 1) {
    DrvAPI::write<char>(segpaddr + off, 0);
  }
}

void loadDRAMProgramSegment(PANDOHammerExe& executable, Elf64_Phdr* phdr, DrvAPIAddress segpaddr) {
  // load once on this pxn
  DrvAPIVAddress decode{segpaddr};
  decode.global() = true;
  decode.pxn() = myPXNId();
  loadProgramSegment(executable, phdr, decode.encode());
}

void loadL2ProgramSegment(PANDOHammerExe& executable, Elf64_Phdr* phdr, DrvAPIAddress segpaddr) {
  // load on each pod
  for (int pod = 0; pod < numPXNPods(); pod++) {
    DrvAPIVAddress decode{segpaddr};
    decode.global() = true;
    decode.pxn() = myPXNId();
    decode.pod() = pod;
    loadProgramSegment(executable, phdr, decode.encode());
  }
}

void loadL1ProgramSegment(PANDOHammerExe& executable, Elf64_Phdr* phdr, DrvAPIAddress segpaddr) {
  // load on each core
  for (int pod = 0; pod < numPXNPods(); pod++) {
    for (int core = 0; core < numPodCores(); core++) {
      DrvAPIVAddress decode{segpaddr};
      decode.global() = true;
      decode.pxn() = myPXNId();
      decode.pod() = pod;
      decode.core_y() = coreYFromId(core);
      decode.core_x() = coreXFromId(core);
      loadProgramSegment(executable, phdr, decode.encode());
    }
  }
}

void loadProgram(PANDOHammerExe& executable) {
  for (Elf64_Phdr* phdr = executable.segments_begin(); phdr != executable.segments_end(); phdr++) {
    if (phdr->p_type != PT_LOAD)
      continue;
    // decode the address of the segment
    DrvAPIVAddress decoded{phdr->p_paddr};
    // if this is a dram segment, load once
    if (decoded.is_dram()) {
      loadDRAMProgramSegment(executable, phdr, decoded.encode());
    }
    // if this is a l2 segment, load onece per pod
    else if (decoded.is_l2()) {
      loadL2ProgramSegment(executable, phdr, decoded.encode());
    }
    // if this is a l1 segment, load once per core
    else if (decoded.is_l1()) {
      loadL1ProgramSegment(executable, phdr, decoded.encode());
    }
  }
}
} // namespace pandocommand
