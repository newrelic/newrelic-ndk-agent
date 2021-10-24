#!/bin/bash

##
# Copyright 2021-present New Relic Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Symbolicate a stripped stackframe given a SYM file and address
#
# Usage:
#   symbolize.sh <path to sym> <address as hex>
#
# Example:
#   symbolize.sh ./x86_64/libnative-lib.so.sym 0xf97c
# Produces:
#  0xf97c: crashing_thread(void*) at .../Android-Test-Native/app/src/main/cpp/native-lib.cpp:151:5
#
# Requires:
# Current NDK installed. $ANDROID_NDK contains the full path to the NDK root directory
##

source $(dirname $0)/ndk.sh

# ndk-which resolves the current binary tool
SYMBOLIZER=$(ndkrequire "${ABI}" "llvm-symbolizer")
[[ -n $SYMBOLIZER ]] && SYMBOLIZER="$ANDROID_NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin/llvm-symbolizer"
dbug "SYMBOLIZER[$SYMBOLIZER]"

SYMS=${1}
ADDR=${2}
OPTS+=" -p"

${SYMBOLIZER} ${OPTS} --obj ${SYMS} ${ADDR}
