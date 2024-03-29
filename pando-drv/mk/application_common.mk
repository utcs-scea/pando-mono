# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

ifndef APP_NAME
$(error APP_NAME is not set)
endif

DRV_DIR ?= $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk

# include the test directory to the python path
# so we can import modules from this directory
export PYTHONPATH := $(DRV_DIR)/tests:$(PYTHONPATH)

APP_PATH ?= $(DRV_DIR)/examples/$(APP_NAME)
APP_EXE  ?= $(APP_PATH)/$(APP_NAME).so

# Build options
CXX ?= clang++
CC  ?= clang

CXXFLAGS := $(APP_CXXFLAGS)
CFLAGS   := $(APP_CFLAGS)

LDFLAGS := $(APP_LDFLAGS)
LDFLAGS += -Wl,-rpath=$(APP_PATH)

LIBS := $(APP_LIBS)

.PHONY: all clean

all: $(APP_NAME).so

# clean rule
clean:
	rm -f $(APP_NAME).so $(APP_NAME).o

# generic object file build rule
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# build the app as a shared object
$(APP_NAME).so: $(APP_NAME).o
$(APP_NAME).so:
	$(CXX) $(CXXFLAGS) -o $@ $(filter %.o,$^) $(LDFLAGS) $(LIBS)


#SCRIPT ?= drv-multicore-bus-test.py
SCRIPT ?= PANDOHammerDrvX.py
SIM_THREADS ?= 1

.PHONY: run
run: $(APP_NAME).so
	$(SST) -n $(SIM_THREADS)  $(DRV_DIR)/tests/$(SCRIPT) -- $(SIM_OPTIONS) $(APP_EXE) $(SIM_ARGS)
