APP_PLATFORM := android-9
APP_PROJECT_PATH := $(call my-dir)/../
APP_BUILD_SCRIPT := $(call my-dir)/Android.mk
APP_MODULES = sac prototype libpng libtremor libjsoncpp
APP_OPTIM := release
APP_ABI := armeabi-v7a armeabi
APP_STL := stlport_static

