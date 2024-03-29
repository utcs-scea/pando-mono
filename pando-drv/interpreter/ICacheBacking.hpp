// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef ICACHEBACKING_HPP
#define ICACHEBACKING_HPP
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <elf.h>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#define pr_info(fmt, ...)                            \
  do {                                               \
    printf("[ICACHE BACKING]: " fmt, ##__VA_ARGS__); \
  } while (0)

// #define ICACHE_BACKING_DEBUG
#ifdef ICACHE_BACKING_DEBUG
#define pr_debug(fmt, ...)                           \
  do {                                               \
    printf("[ICACHE BACKING]: " fmt, ##__VA_ARGS__); \
  } while (0)
#else
#define pr_debug(fmt, ...)
#endif

class ICacheBacking {
public:
  ICacheBacking(const char* file) : data_(nullptr), fd_(-1) {
    fd_ = open(file, O_RDONLY);
    if (fd_ < 0) {
      pr_info("failed to open %s: %m\n", file);
      exit(1);
    }

    struct stat st;
    if (fstat(fd_, &st) < 0) {
      pr_info("failed to stat %s: %m\n", file);
      exit(1);
    }

    data_ = (uint8_t*)mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (data_ == MAP_FAILED) {
      pr_info("failed to mmap %s: %m\n", file);
      exit(1);
    }

    pr_debug("opened %s @ %p: %d sections\n", file, ehdr(), ehdr()->e_shnum);
    findTextPhdrs();
    pr_debug("found %zu text phdrs\n", text_phdrs_.size());
    printEIdent();
    // printSectionsInfo();
    printProgramHeaders();
  }
  ~ICacheBacking() {
    if (data_ != nullptr) {
      munmap(data_, sizeof(Elf64_Ehdr));
    }
    if (fd_ >= 0) {
      close(fd_);
    }
  }
  ICacheBacking(const ICacheBacking&) = delete;
  ICacheBacking& operator=(const ICacheBacking&) = delete;
  ICacheBacking(ICacheBacking&&) = delete;
  ICacheBacking& operator=(ICacheBacking&&) = delete;

  Elf64_Ehdr* ehdr() const {
    return reinterpret_cast<Elf64_Ehdr*>(data_);
  }
  Elf64_Shdr* shdr(int idx) const {
    if (idx >= ehdr()->e_shnum) {
      return nullptr;
    }
    return reinterpret_cast<Elf64_Shdr*>(data_ + ehdr()->e_shoff + idx * ehdr()->e_shentsize);
  }
  Elf64_Phdr* phdr(int idx) const {
    if (idx >= ehdr()->e_phnum) {
      return nullptr;
    }
    return reinterpret_cast<Elf64_Phdr*>(data_ + ehdr()->e_phoff + idx * ehdr()->e_phentsize);
  }

  void* segment(Elf64_Phdr* phdr) const {
    if (phdr == nullptr) {
      return nullptr;
    }
    return data_ + phdr->p_offset;
  }

  void* segment(int idx) const {
    return segment(phdr(idx));
  }

  void findTextPhdrs() {
    for (int i = 0; i < ehdr()->e_phnum; ++i) {
      auto ph = phdr(i);
      if (ph == nullptr) {
        continue;
      }
      if (ph->p_type == PT_LOAD && (ph->p_flags & PF_X)) {
        text_phdrs_.push_back(ph);
      }
    }
    std::sort(text_phdrs_.begin(), text_phdrs_.end(), [](Elf64_Phdr* a, Elf64_Phdr* b) {
      return a->p_vaddr < b->p_vaddr;
    });
  }

  Elf64_Phdr* findTextPhdr(Elf64_Addr addr) {
    if (text_phdrs_.empty()) {
      return nullptr;
    }
    for (size_t i = 0; i < text_phdrs_.size(); ++i) {
      auto ph = text_phdrs_[i];
      if (addr >= ph->p_vaddr && addr < ph->p_vaddr + ph->p_memsz) {
        return ph;
      }
    }
    return nullptr;
  }

  uint32_t read(Elf64_Addr addr) {
    auto ph = findTextPhdr(addr);
    if (ph == nullptr) {
      pr_info("no text phdr for addr %016lx\n", addr);
      exit(1);
    }
    pr_debug(
        "found text phdr for addr 0x%08x: p_vaddr = 0x%08x, p_paddr = 0x%08x, p_offset = 0x%08x\n",
        addr, ph->p_vaddr, ph->p_paddr, ph->p_offset);
    int offset = ph->p_offset + (addr - ph->p_vaddr);
    uint8_t* data = (uint8_t*)(&data_[offset]);
    uint32_t ret = *(uint32_t*)data;
    pr_debug("read %02x %02x %02x %02x from address 0x%08x at offset 0x%08x\n", data[3], data[2],
             data[1], data[0], addr, offset);
    return ret;
  }

  Elf64_Addr getStartAddr() {
    return ehdr()->e_entry;
  }

  void printEIdent() {
#ifdef ICACHE_BACKING_DEBUG
    auto e_ident = ehdr()->e_ident;
    pr_debug("e_ident: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", e_ident[EI_MAG0],
             e_ident[EI_MAG1], e_ident[EI_MAG2], e_ident[EI_MAG3], e_ident[EI_CLASS],
             e_ident[EI_DATA], e_ident[EI_VERSION], e_ident[EI_OSABI], e_ident[EI_ABIVERSION],
             e_ident[EI_PAD]);
#endif
  }

  void printProgramHeaders() {
    for (int i = 0; i < ehdr()->e_phnum; ++i) {
      printProgramHeader(i);
    }
  }

  void printProgramHeader(int i) {
    auto ph = phdr(i);
    if (ph == nullptr) {
      return;
    }
    std::stringstream ss;
    ss << "{ p_type: " << std::setw(10) << programHeaderType(ph->p_type);
    ss << ", p_offset: " << std::hex << std::setw(10) << ph->p_offset;
    ss << ", p_vaddr: " << std::hex << std::setw(10) << ph->p_vaddr;
    ss << ", p_paddr: " << std::hex << std::setw(10) << ph->p_paddr;
    ss << ", p_filesz: " << std::dec << std::setw(10) << ph->p_filesz;
    ss << ", p_memsz: " << std::dec << std::setw(10) << ph->p_memsz;
    ss << ", p_flags: " << std::setw(15) << programHeaderFlags(ph->p_flags);
    ss << ", p_align: " << std::dec << std::setw(10) << ph->p_align;
    ss << " }";
    pr_debug("program header %2d @ %p: %s\n", i, ph, ss.str().c_str());
  }

  std::string programHeaderType(Elf64_Word p_type) {
    switch (p_type) {
      case PT_NULL:
        return "PT_NULL";
      case PT_LOAD:
        return "PT_LOAD";
      case PT_DYNAMIC:
        return "PT_DYNAMIC";
      case PT_INTERP:
        return "PT_INTERP";
      case PT_NOTE:
        return "PT_NOTE";
      case PT_SHLIB:
        return "PT_SHLIB";
      case PT_PHDR:
        return "PT_PHDR";
      case PT_TLS:
        return "PT_TLS";
      case PT_NUM:
        return "PT_NUM";
      default:
        return "UNKNOWN";
    }
  }

  std::string programHeaderFlags(Elf64_Word p_flags) {
    std::stringstream ss;
    if (p_flags & PF_X) {
      ss << "PF_X ";
    }
    if (p_flags & PF_W) {
      ss << "PF_W ";
    }
    if (p_flags & PF_R) {
      ss << "PF_R ";
    }
    return ss.str();
  }

  void printSectionsInfo() {
    for (int i = 0; i < ehdr()->e_shnum; ++i) {
      printSectionInfo(i);
    }
  }

  void printSectionInfo(int idx) {
    auto sh = shdr(idx);
    if (sh == nullptr) {
      return;
    }
    std::stringstream ss;
    ss << "{ sh_name: " << std::dec << std::setw(10) << sh->sh_name;
    ss << ", sh_type: " << std::dec << std::setw(10) << sh->sh_type;
    ss << ", sh_flags: " << std::hex << std::setw(10) << std::setfill('0') << sh->sh_flags
       << std::setfill(' ');
    ss << ", sh_addr: " << std::hex << std::setw(10) << sh->sh_addr;
    ss << ", sh_offset: " << std::hex << std::setw(10) << sh->sh_offset;
    ss << ", sh_size: " << std::dec << std::setw(10) << sh->sh_size;
    ss << ", sh_link: " << std::dec << std::setw(10) << sh->sh_link;
    ss << ", sh_info: " << std::dec << std::setw(10) << sh->sh_info;
    ss << ", sh_addralign: " << std::dec << std::setw(10) << sh->sh_addralign;
    ss << ", sh_entsize: " << std::dec << std::setw(10) << sh->sh_entsize;
    ss << " }";
    pr_debug("section %2d @ %p: %s\n", idx, sh, ss.str().c_str());
  }

  static const char* sht_str(Elf64_Word sh_type) {
    pr_debug("sh_type: %08x\n", sh_type);
    switch (sh_type) {
      case SHT_NULL:
        return "NULL";
      case SHT_PROGBITS:
        return "PROGBITS";
      case SHT_SYMTAB:
        return "SYMTAB";
      case SHT_STRTAB:
        return "STRTAB";
      case SHT_RELA:
        return "RELA";
      case SHT_HASH:
        return "HASH";
      case SHT_DYNAMIC:
        return "DYNAMIC";
      case SHT_NOTE:
        return "NOTE";
      case SHT_NOBITS:
        return "NOBITS";
      case SHT_REL:
        return "REL";
      case SHT_SHLIB:
        return "SHLIB";
      case SHT_DYNSYM:
        return "DYNSYM";
      case SHT_INIT_ARRAY:
        return "INIT_ARRAY";
      case SHT_FINI_ARRAY:
        return "FINI_ARRAY";
      case SHT_PREINIT_ARRAY:
        return "PREINIT_ARRAY";
      case SHT_GROUP:
        return "GROUP";
      case SHT_SYMTAB_SHNDX:
        return "SYMTAB_SHNDX";
      case SHT_NUM:
      default:
        return "UNKNOWN";
    }
  }

  uint8_t* data_;
  int fd_;
  std::vector<Elf64_Phdr*> text_phdrs_;
};
#endif
