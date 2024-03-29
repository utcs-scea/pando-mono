# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

# include the config.mk
DRV_DIR ?= $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk

CXX=$(shell $(SST_CONFIG) --CXX)
CXXFLAGS=$(shell $(SST_CONFIG) --ELEMENT_CXXFLAGS) -fno-stack-protector -O2
CXXFLAGS+=$(BOOST_CXXFLAGS)
CXXFLAGS+=$(SST_ELEMENTS_CXXFLAGS)
CXXFLAGS+=-I$(RISCV_INSTALL_DIR)
CXXFLAGS+=-Wall -Wextra -Werror
CXXFLAGS+=-Wno-reorder -Wno-ignored-qualifiers -Wno-unused-parameter
CXXFLAGS+=-DDEBUG -g
LDFLAGS =  $(shell $(SST_CONFIG) --ELEMENT_LDFLAGS) -fno-stack-protector
LDFLAGS += -L$(DRV_LIB_DIR) -Wl,-rpath,$(DRV_LIB_DIR)
LDFLAGS += $(BOOST_LDFLAGS)
LDFLAGS += $(SST_ELEMENTS_LDFLAGS)

CXXFLAGS += -I$(DRV_DIR)/element/
CXXFLAGS += -I$(DRV_INCLUDE_DIR)

LIBS += -lboost_coroutine -lboost_context -ldrvapi

CXXFLAGS-$(RAMULATOR) += $(RAMULATOR_CXXFLAGS)
LDFLAGS-$(RAMULATOR)  += $(RAMULATOR_LDFLAGS)
LIBS-$(RAMULATOR)     += $(RAMULATOR_LIBS)
CXXFLAGS-$(DRAMSIM3)  += $(DRAMSIM3_CXXFLAGS)
LDFLAGS-$(DRAMSIM3)   += $(DRAMSIM3_LDFLAGS)
LIBS-$(DRAMSIM3)      += $(DRAMSIM3_LIBS)

CXXFLAGS += $(CXXFLAGS-yes)
LDFLAGS += $(LDFLAGS-yes)
LIBS += $(LIBS-yes)

drvsim-sources += DrvAddressMap.cpp
drvsim-headers += DrvAddressMap.hpp
drvsim-sources += DrvCore.cpp
drvsim-headers += DrvCore.hpp
drvsim-sources += DrvCustomStdMem.cpp
drvsim-headers += DrvCustomStdMem.hpp
drvsim-sources += DrvMemory.cpp
drvsim-headers += DrvMemory.hpp
drvsim-sources += DrvNativeSimulationTranslator.cpp
drvsim-headers += DrvNativeSimulationTranslator.hpp
drvsim-sources += DrvSelfLinkMemory.cpp
drvsim-headers += DrvSelfLinkMemory.hpp
drvsim-sources += DrvSimpleMemory.cpp
drvsim-headers += DrvSimpleMemory.hpp
drvsim-sources += DrvStdMemory.cpp
drvsim-headers += DrvStdMemory.hpp
drvsim-sources += DrvThread.cpp
drvsim-headers += DrvThread.hpp
drvsim-headers += DrvEvent.hpp
drvsim-headers += DrvMemEvent.hpp
drvsim-headers += DrvNopEvent.hpp
drvsim-headers += DrvSysConfig.hpp
drvsim-headers += DrvSystem.hpp
drvsim-sources += DrvSystem.cpp
drvsim-sources += SSTRISCVCore.cpp
drvsim-sources += SSTRISCVSimulator.cpp
drvsim-headers += SSTRISCVSimulator.hpp
drvsim-headers += SSTRISCVCore.hpp
drvsim-headers += SSTRISCVHart.hpp
drvsim-headers += DrvStats.hpp

drvsim-sources-$(RAMULATOR) += DrvCustomRamulatorMem.cpp
drvsim-headers-$(RAMULATOR) += DrvCustomRamulatorMem.hpp
drvsim-sources-$(DRAMSIM3) += DrvCustomDRAMSim3Mem.cpp
drvsim-headers-$(DRAMSIM3) += DrvCustomDRAMSim3Mem.hpp

drvsim-headers += $(drvsim-headers-yes)
drvsim-sources += $(drvsim-sources-yes)

ifdef NO_DRVR
# DrvX does not depend on RISCV-related code
drvsim-sources := $(filter-out $(wildcard SSTRISCV*.cpp), $(drvsim-sources))
endif

drvsim-objects = $(patsubst %.cpp,%.o,$(drvsim-sources))

$(drvsim-objects): %.o: %.cpp $(drvsim-headers)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@


libDrv.so: $(drvsim-objects) $(drvsim-headers)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -shared -o $@ $(drvsim-objects) $(LIBS)

install: libDrv.so
	$(SST_REGISTER) Drv Drv_LIBDIR=$(CURDIR)

clean:
	rm -Rf *.o *.so
