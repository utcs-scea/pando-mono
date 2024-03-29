# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR ?= $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk

CXXFLAGS += $(BOOST_CXXFLAGS)
CXXFLAGS += -std=c++14 -fPIC -Wall -Wextra -Werror
CXXFLAGS += -Wno-unused-parameter
CXXFLAGS += -Wno-unused-variable

CXXFLAGS += -I$(DRV_INCLUDE_DIR)
CXXFLAGS += -I$(DRV_DIR)/interpreter

libriscvinterp-headers := $(wildcard *.h) $(wildcard *.hpp)
libriscvinterp-sources := $(wildcard *.cpp)
libriscvinterp-objects := $(libriscvinterp-sources:.cpp=.o)
libriscvinterp-install-headers := $(foreach header, $(libriscvinterp-headers), $(DRV_INCLUDE_DIR)/$(header))

$(libriscvinterp-install-headers): $(DRV_INCLUDE_DIR)/%: %
	@mkdir -p $(dir $@)
	@cp $< $@

$(libriscvinterp-objects): %.o: %.cpp $(libriscvinterp-install-headers)
	$(CXX) -fPIC $(CXXFLAGS) -c -o $@ $<

$(DRV_LIB_DIR)/libriscvinterp.so: $(libriscvinterp-objects)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -shared -fPIC -o $@ $(filter %.o, $^) $(LDFLAGS) $(LIBS)

clean:
	rm -f interpreter *.so *.o *~
	rm -f $(libriscvinterp-install-headers)
	rm -f $(DRV_LIB_DIR)/libriscvinterp.so

install-headers: $(libriscvinterp-install-headers)
install-lib: $(DRV_LIB_DIR)/libriscvinterp.so
install: install-headers install-lib
