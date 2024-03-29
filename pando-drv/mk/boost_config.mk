# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

# CHANGME - set the path to your boost installation
ifndef _BOOST_CONFIG_MK_
_BOOST_CONFIG_MK_ = 1
BOOST_INSTALL_DIR=$(DRV_ROOT)
BOOST_CXXFLAGS += -I$(BOOST_INSTALL_DIR)/include
BOOST_LDFLAGS  += -L$(BOOST_INSTALL_DIR)/lib -Wl,-rpath=$(BOOST_INSTALL_DIR)/lib

endif
