#!/bin/sh

##
# Copyright 2021-present New Relic Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Requires:
# Current NDK installed. $ANDROID_NDK contains the full path to the NDK root directory
##

dbug() { 
	# echo $*
	return 0 
}

fatal() {
	echo $*
	exit 1
}

[[ -x ${ANDROID_NDK} ]] || fatal "ANDROID_NDK is not set!"
dbug "ANDROID_NDK[$ANDROID_NDK]"

ABIS=${ABIS:-"x86_64 x86 arm64-v8a armeabi-v7a"}
ABI=${ABI:-"x86_64"}
BUILDDIR=${BUILDDIR:-"build"}
VARIANT=${VARIANT:="release"}
ARCHIVE=${ARCHIVE:-"${BUILDDIR}/intermediates/cmake/${VARIANT}/obj/allsyms.zip"}


#
# ndk-require(ABI tool): Return and verify existence of required NDK tool
#
ndkrequire() {
    local tool=$(${ANDROID_NDK}/ndk-which --abi $1 $2)
    if [[ -n "$tool" && -x "$tool" ]] ; then
	    echo $tool
	    return 0
	fi
	dbug "NDK ABI[$1] tool[$2] does not exist or is not executable!" 1>&2
	echo $2
	return 1
}

# Expected resource layout:
# ${BUILD_DIR}/intermediates/cmake/<variant>/obj/
# │
# ├── x86_64/
# │   └── libvnative.so
# ├── x86/
# │   └── libnative.so
# ├── arm64-v8a/
# │   └── libnative.so
# └── armeabi-v7a/
#     └── libnative.so


## AGP 4.1+ : Collection also found here
# ${BUILD_DIR}}/outputs/native-debug-symbols/variant-name/native-debug-symbols.zip

