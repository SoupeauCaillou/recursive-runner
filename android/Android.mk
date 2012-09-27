LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

APP_DIR := $(LOCAL_PATH)

LOCAL_MODULE := prototype

LOCAL_CFLAGS := -DANDROID_NDK \
                -DDISABLE_IMPORTGL \
				-I$(LOCAL_PATH)/..

LOCAL_CXXFLAGS := -DANDROID_NDK -DDEBUG -DENABLE_PROFILING \
                -DDISABLE_IMPORTGL \
            -I$(LOCAL_PATH)/../sources \
				-I$(LOCAL_PATH)/.. \
				-I$(LOCAL_PATH)/../sac/ \
				-I$(LOCAL_PATH)/../sac/libs/libpng/jni/ \
				-I$(LOCAL_PATH)/../sac/libs/

LOCAL_SRC_FILES := \
    prototype.cpp \
    ../sources/PrototypeGame.cpp \
    ../sources/systems/RunnerSystem.cpp \
    ../sources/systems/CameraTargetSystem.cpp \
    ../sac/android/sacjnilib.cpp

LOCAL_STATIC_LIBRARIES := sac png tremor jsoncpp
LOCAL_LDLIBS := -lGLESv2 -lGLESv1_CM -lEGL -llog -lz

include $(BUILD_SHARED_LIBRARY)

include $(APP_DIR)/../sac/build/android/Android.mk
include $(APP_DIR)/../sac/libs/build/android/tremor/Android.mk
include $(APP_DIR)/../sac/libs/build/android/libpng/Android.mk
include $(APP_DIR)/../sac/libs/build/android/jsoncpp/Android.mk

$(call import-module,android/native_app_glue)
