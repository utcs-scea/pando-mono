# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

# include the confing.mk
DRV_DIR ?= $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk

.PHONY: all install clean

LDFLAGS += -L$(DRV_LIB_DIR) -Wl,-rpath,$(DRV_LIB_DIR)
LDFLAGS += $(BOOST_LDFLAGS)

CXX := clang++

CXXFLAGS += $(BOOST_CXXFLAGS)
CXXFLAGS += -std=c++14 -fPIC -Wall -Wextra -Werror -Wno-unused-parameter
CXXFLAGS += -I$(DRV_INCLUDE_DIR)
CXXFLAGS += -I$(DRV_DIR)/api

# libdrvapi
libdrvapi-sources += $(DRV_DIR)/api/DrvAPIThread.cpp
libdrvapi-sources += $(DRV_DIR)/api/DrvAPIAllocator.cpp
libdrvapi-sources += $(DRV_DIR)/api/DrvAPISysConfig.cpp
libdrvapi-sources += $(DRV_DIR)/api/DrvAPIAddressMap.cpp
libdrvapi-sources += $(DRV_DIR)/api/DrvAPIThreadState.cpp
libdrvapi-sources += $(DRV_DIR)/api/DrvAPIAddressToNative.cpp
libdrvapi-sources += $(DRV_DIR)/api/DrvAPINativeToAddress.cpp
libdrvapi-sources += $(DRV_DIR)/api/DrvAPIGlobal.cpp
libdrvapi-objects := $(patsubst %.cpp,%.o,$(libdrvapi-sources))
libdrvapi-headers := $(DRV_DIR)/api/DrvAPIAddress.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPIAddressMap.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPIMain.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPIMemory.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPIReadModifyWrite.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPIThread.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPIThreadState.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPIPointer.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPIAllocator.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPIGlobal.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPIOp.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPISysConfig.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPIInfo.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPIAddressToNative.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPINativeToAddress.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPISystem.hpp
libdrvapi-headers += $(DRV_DIR)/api/DrvAPI.hpp
libdrvapi-install-headers := $(foreach header,$(libdrvapi-headers),$(DRV_INCLUDE_DIR)/$(notdir $(header)))

# libdrvapiapp
libdrvapiapp-sources += $(DRV_DIR)/api/DrvAPIFunction.cpp
libdrvapiapp-objects := $(patsubst %.cpp,%.o,$(libdrvapiapp-sources))
libdrvapiapp-headers := $(DRV_DIR)/api/DrvAPIFunction.hpp
libdrvapiapp-install-headers := $(foreach header,$(libdrvapiapp-headers),$(DRV_INCLUDE_DIR)/$(notdir $(header)))

$(libdrvapiapp-install-headers) $(libdrvapi-install-headers): $(DRV_INCLUDE_DIR)
$(libdrvapiapp-install-headers) $(libdrvapi-install-headers): $(DRV_INCLUDE_DIR)/%: $(DRV_DIR)/api/%
	@cp $< $@

all: install

install-headers: $(libdrvapiapp-install-headers)
install-headers: $(libdrvapi-install-headers)
install-headers: install-lib
install-lib: $(DRV_LIB_DIR)/libdrvapiapp.so
install-lib: $(DRV_LIB_DIR)/libdrvapi.so
install: install-lib install-headers

$(libdrvapi-objects): %.o: %.cpp $(libdrvapi-headers)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(libdrvapiapp-objects): %.o: %.cpp $(libdrvapiapp-headers) $(libdrvapi-headers)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(DRV_LIB_DIR)/libdrvapiapp.so $(DRV_LIB_DIR)/libdrvapi.so: $(DRV_INSTALL_DIR)
$(DRV_LIB_DIR)/libdrvapiapp.so: $(libdrvapiapp-objects)
$(DRV_LIB_DIR)/libdrvapi.so: $(libdrvapi-objects)
$(DRV_LIB_DIR)/libdrvapiapp.so $(DRV_LIB_DIR)/libdrvapi.so:
	$(CXX) $(CXXFLAGS) -shared -fPIC -o $@ $(filter %.o,$^) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(libdrvapi-objects)
	rm -f $(DRV_LIB_DIR)/libdrvapi.so
	rm -f $(libdrvapi-install-headers)
	rm -f $(libdrvapiapp-objects)
	rm -f $(DRV_LIB_DIR)/libdrvapiapp.so
	rm -f $(libdrvapiapp-install-headers)
