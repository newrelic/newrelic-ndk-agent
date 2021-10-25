#!/bin/bash

##
# Copyright 2021-present New Relic Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# WORK IN PROGRESS

# The ndk-stack tool allows you to symbolize stack traces. It replaces any address
# inside a shared library with the corresponding <source-file>:<line-number> from source code.
#
# ndk-stack requires the *unstripped* versions of the app's shared libraries, normally  found in:
#   app/build/intermediates/cmake/<variant>/obj/<abi>
# where <variant> is the build type (debug, release, etc), and <abi> is the device ABI.
#
# Usage:
#   ndk-stack.sh <symbol path> <stackframes>
#
# Requires:
# Current NDK installed. $ANDROID_NDK contains the full path to the NDK root directory
#

source $(dirname $0)/ndk.sh
NDKSTACK=${NDKSTACK:-"$ANDROID_NDK/ndk-stack"}
echo "NDKSTACK[$NDKSTACK]"

SYMS=${1}
ADDR=${2}
OPTS+=""

${NDKSTACK} ${OPTS} -sym ${SYMS}
