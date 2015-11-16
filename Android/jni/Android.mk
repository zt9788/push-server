
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := Test
LOCAL_SRC_FILES := push.c
LOCAL_LDLIBS:=-L$(SYSROOT)/usr/lib -llog
APP_PLATFORM := android-19

include $(BUILD_SHARED_LIBRARY)
