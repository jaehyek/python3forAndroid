LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := memtester
LOCAL_SRC_FILES := memtester.c tests.c get_phy_addr.c
LOCAL_CFLAGS += -DPOSIX -D_POSIX_C_SOURCE=200809L -DTEST_NARROW_WRITES
include $(BUILD_EXECUTABLE)