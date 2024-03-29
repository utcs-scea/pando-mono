// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2024 University of Washington
#ifndef PANDOCOMMAND_EXECUTABLE_HPP
#define PANDOCOMMAND_EXECUTABLE_HPP
#include <DrvAPI.hpp>
#include <elf.h>
#include <pandocommand/place.hpp>
#include <string>
#include <unordered_map>
namespace pandocommand {

class PANDOHammerExe {
public:
  PANDOHammerExe() : fp_(nullptr), ehdr_(nullptr) {}
  PANDOHammerExe(const char* fname) : fp_(nullptr), ehdr_(nullptr) {
    fp_ = fopen(fname, "rb");
    if (fp_ == nullptr) {
      throw std::runtime_error("Could not open file");
    }

    struct stat st;
    if (stat(fname, &st) != 0) {
      throw std::runtime_error("Could not stat file");
    }

    ehdr_ = (Elf64_Ehdr*)mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fileno(fp_), 0);
    if (ehdr_ == MAP_FAILED) {
      throw std::runtime_error("Could not mmap file");
    }
    symtab_init();
  }

  virtual ~PANDOHammerExe() {
    if (ehdr_ != nullptr) {
      munmap(ehdr_, sizeof(Elf64_Ehdr));
    }
    if (fp_ != nullptr) {
      fclose(fp_);
    }
    symtab_.clear();
  }

  PANDOHammerExe(const PANDOHammerExe&) = delete;
  PANDOHammerExe(PANDOHammerExe&&) = delete;
  PANDOHammerExe& operator=(const PANDOHammerExe&) = delete;
  PANDOHammerExe& operator=(PANDOHammerExe&&) = delete;

  static std::shared_ptr<PANDOHammerExe> Open(const char* fname) {
    return std::make_shared<PANDOHammerExe>(fname);
  }

  DrvAPI::DrvAPIVAddress symbol(const std::string& symname) const {
    if (symtab_.count(symname) == 0) {
      throw std::runtime_error("Symbol not found");
    }
    return DrvAPI::DrvAPIVAddress{symtab_.at(symname)};
  }

  template <typename T>
  DrvAPI::DrvAPIPointer<T> symbol(const std::string& symname, const Place& place) const {
    DrvAPI::DrvAPIVAddress addr = symbol(symname);
    addr.global() = true;
    if (addr.is_l1()) {
      addr.pxn() = place.pxn;
      addr.pod() = place.pod;
      addr.core_y() = place.core_y;
      addr.core_x() = place.core_x;
    } else if (addr.is_l2()) {
      addr.pxn() = place.pxn;
      addr.pod() = place.pod;
    }
    return DrvAPI::DrvAPIPointer<T>(addr.encode());
  }

  template <typename T>
  typename DrvAPI::DrvAPIPointer<T>::value_handle symbol_ref(const std::string& symname,
                                                             const Place& place) const {
    DrvAPI::DrvAPIVAddress addr = symbol(symname);
    addr.global() = true;
    if (addr.is_l1()) {
      addr.pxn() = place.pxn;
      addr.pod() = place.pod;
      addr.core_y() = place.core_y;
      addr.core_x() = place.core_x;
    } else if (addr.is_l2()) {
      addr.pxn() = place.pxn;
      addr.pod() = place.pod;
    }
    return *DrvAPI::DrvAPIPointer<T>(addr.encode());
  }

  Elf64_Phdr* segments_begin() const {
    return (Elf64_Phdr*)((char*)ehdr_ + ehdr_->e_phoff);
  }

  Elf64_Phdr* segments_end() const {
    return (Elf64_Phdr*)((char*)segments_begin() + ehdr_->e_phnum * ehdr_->e_phentsize);
  }

  Elf64_Shdr* sections_begin() const {
    return (Elf64_Shdr*)((char*)ehdr_ + ehdr_->e_shoff);
  }

  Elf64_Shdr* sections_end() const {
    return (Elf64_Shdr*)((char*)sections_begin() + ehdr_->e_shnum * ehdr_->e_shentsize);
  }

  char* segment_data(Elf64_Phdr* phdr) {
    return ((char*)ehdr_) + phdr->p_offset;
  }

protected:
  void symtab_init() {
    // scan for symbol tables
    for (Elf64_Shdr* shdr = sections_begin(); shdr != sections_end(); shdr++) {
      if (shdr->sh_type == SHT_SYMTAB)
        symtab_add(shdr);
    }
  }

  Elf64_Sym* symtab_begin(Elf64_Shdr* symtab_shdr) const {
    return (Elf64_Sym*)((char*)ehdr_ + symtab_shdr->sh_offset);
  }

  Elf64_Sym* symtab_end(Elf64_Shdr* symtab_shdr) const {
    return (Elf64_Sym*)((char*)symtab_begin(symtab_shdr) + symtab_shdr->sh_size);
  }

  const char* sym_name(Elf64_Shdr* strtab_shdr, Elf64_Sym* sym) const {
    return (const char*)ehdr_ + strtab_shdr->sh_offset + sym->st_name;
  }

  void symtab_add(Elf64_Shdr* symtab_shdr) {
    // get the string table
    Elf64_Shdr* strtab = sections_begin() + symtab_shdr->sh_link;
    for (Elf64_Sym* sym = symtab_begin(symtab_shdr); sym != symtab_end(symtab_shdr); sym++) {
      if (sym->st_name != 0) {
        symtab_[sym_name(strtab, sym)] = sym->st_value;
      }
    }
  }

protected:
  FILE* fp_;
  Elf64_Ehdr* ehdr_;
  std::unordered_map<std::string, DrvAPI::DrvAPIAddress> symtab_;
};

} // namespace pandocommand
#endif
