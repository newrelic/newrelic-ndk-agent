#!/bin/sh

# https://developer.android.com/ndk/guides/ndk-stack

ABI=${ABI:-x86_64}
NDK_STACK=${NDK_STACK:-$ANDROID_HOME/ndk-bundle/ndk-stack}
OBJ_PATH=`pwd`/app/build/intermediates/cmake/debug

echo "ABI[$ABI]"
echo "NDK_STACK[$NDK_STACK]"
echo "OBJ_PATH[$OBJ_PATH]"

# app/build/intermediates/cmake/debug/obj/x86_64
adb logcat | ${NDK_STACK} -sym ${OBJ_PATH}/obj/${ABI}
