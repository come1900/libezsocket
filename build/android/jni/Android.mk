LOCAL_PATH := $(call my-dir)
#####################################################################
## 
#include $(CLEAR_VARS)
#
##defined in Application.mk noneed here
##LOCAL_C_INCLUDES += $(NDK_ROOT)/sources/cxx-stl/stlport/stlport
##LOCAL_LDLIBS += -lstdc++ -L$(NDK_ROOT)/sources/cxx-stl/stlport/libs/armeabi -lstlport_static
#
#LOCAL_SRC_FILES := test.cpp
#LOCAL_MODULE := testA
#
#include $(BUILD_EXECUTABLE)

####################################################################

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

PREFIX_CFLAGS +=       -DLINUX
PREFIX_CFLAGS +=       -D_ANDROID

PREFIX_BASEDIR_SRC=../../../src/
PREFIX_BASEDIR_MKF=../../src/#
include $(PREFIX_BASEDIR_MKF)/Makefile.SrcLists
LOCAL_C_INCLUDES := ../$(PREFIX_BASEDIR_SRC)
LOCAL_CFLAGS+=$(PREFIX_CFLAGS)
LOCAL_CXXFLAGS+=$(PREFIX_CFLAGS)
LOCAL_MODULE    := ezsocket 
LOCAL_SRC_FILES := $(LIB_SRCS)
include $(BUILD_STATIC_LIBRARY)
####################################################################
