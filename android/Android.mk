LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

APP_DIR := $(LOCAL_PATH)

LOCAL_MODULE := recursiveRunner

LOCAL_CFLAGS := -DANDROID_NDK -DENABLE_PROFILING \
                -DDISABLE_IMPORTGL \
				-I$(LOCAL_PATH)/..

LOCAL_CXXFLAGS := -DANDROID_NDK -DENABLE_LOG -DENABLE_PROFILING \
                -DDISABLE_IMPORTGL  -fvisibility=hidden \
            -I$(LOCAL_PATH)/../sources \
				-I$(LOCAL_PATH)/.. \
				-I$(LOCAL_PATH)/../sac/ \
				-I$(LOCAL_PATH)/../sac/libs/libpng/jni/ \
				-I$(LOCAL_PATH)/../sac/libs/

LOCAL_SRC_FILES := \
    recursiveRunner.cpp \
    ../sources/RecursiveRunnerGame.cpp \
    ../sources/systems/RunnerSystem.cpp \
    ../sources/systems/PlayerSystem.cpp \
    ../sources/systems/CameraTargetSystem.cpp \
    ../sources/systems/RangeFollowerSystem.cpp \
    ../sources/api/android/StorageAPIAndroidImpl.cpp \
    ../sac/android/sacjnilib.cpp

LOCAL_STATIC_LIBRARIES := sac png tremor jsoncpp
LOCAL_LDLIBS := -lGLESv2 -lGLESv1_CM -lEGL -llog -lz

include $(BUILD_SHARED_LIBRARY)

include $(APP_DIR)/../sac/build/android/Android.mk
include $(APP_DIR)/../sac/libs/build/android/tremor/Android.mk
include $(APP_DIR)/../sac/libs/build/android/libpng/Android.mk
include $(APP_DIR)/../sac/libs/build/android/jsoncpp/Android.mk

$(call import-module,android/native_app_glue)
