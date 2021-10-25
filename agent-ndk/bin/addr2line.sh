#!/bin/bash

##
# Copyright 2021-present New Relic Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Symbolicate a stripped stackframe given a SYM file and address, with source cointext
#
# Usage:
#   addr2line.sh <path to sym> <address as hex>
#
# Example:
#   OPTS="--print-source-context-lines=3" addr2line.sh ./x86_64/libnative-lib.so.sym 0xf97c
# Produces:
# 150  :     sleep(thread_args->sleepSec);
# 151 >:     crashBySignal(thread_args->signo);
# 152  : }
#
# Requires:
# Current NDK installed. $ANDROID_NDK contains the full path to the NDK root directory
##

source $(dirname $0)/ndk.sh

# resolve fqfn of addr2line tool
A2L=$(ndkrequire "${ABI}" "llvm-addr2line")
[[ -n $A2L ]] && A2L="$ANDROID_NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin/llvm-addr2line"
dbug "ADDR2LINE[$A2L]"

SYMS=${1}
ADDR=${2}
OPTS+=" -p --print-source-context-lines=5"

${A2L} ${OPTS} --obj ${SYMS} ${ADDR}
