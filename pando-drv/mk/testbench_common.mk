# include environment
DRV_DIR ?= $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk

# define TESTS_DIR
TESTS_DIRS ?= $(TESTS)

###########################################
# rules for creating all test directories #
###########################################
# rules for {test name}/app_path.mk
$(addsuffix /app_path.mk,$(TESTS_DIRS)): %/app_path.mk: app_path.mk
  @echo Creating $@
  @mkdir -p $(dir $@)
  @cp $< $@

# rules for {test name}Makefile
$(addsuffix /Makefile,$(TESTS_DIRS)): %/Makefile: template.mk
  @echo Creating $@
  @mkdir -p $(dir $@)
  @cp template.mk $@

# rule for creating {test name}
$(TESTS_DIRS): %: %/app_path.mk
$(TESTS_DIRS): %: %/parameters.mk
$(TESTS_DIRS): %: %/Makefile

# generate all test directories
generate: $(TESTS_DIRS)

####################################################
# rules for running {test name}.{profile|exec|...} #
####################################################
$(addsuffix .run,$(TESTS)): %.run: %
  $(MAKE) -C $< run
$(addsuffix .debug,$(TESTS)): %.debug: %
  $(MAKE) -C $< debug
$(addsuffix .clean,$(TESTS)): %.clean: %
  $(MAKE) -C $< clean
####################################
# meta rules for running all tests #
####################################
.PHONY: run
run: $(addsuffix .run,$(TESTS))
.PHONY: debug
debug: $(addsuffix .debug,$(TESTS))
.PHONY: clean
clean: $(addsuffix .clean,$(TESTS))
##############################
# purge all test directories #
##############################
purge:
  rm -rf $(TESTS_DIRS)
