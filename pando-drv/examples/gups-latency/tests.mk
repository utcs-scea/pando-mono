####################
# CHANGE ME: TESTS #
####################
# TESTS += $(call test-name,[pod-cores],[core-threads],[dram-latency],[dram-banks])
TESTS += $(call test-name,1,16,128ns,8)
TESTS += $(call test-name,2,16,128ns,8)
TESTS += $(call test-name,4,16,128ns,8)
TESTS += $(call test-name,8,16,128ns,8)
TESTS += $(call test-name,16,16,128ns,8)
TESTS += $(call test-name,32,16,128ns,8)
TESTS += $(call test-name,64,16,128ns,8)
