#!/bin/sh

##
# Copyright 2021-present New Relic Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Upload all symbols for the project as a a ZIP archibe to the NewRelic /symbol end point.
#
# Usage:
#   APPTOKEN=<app token> post-syms.sh
#
# Requires:
# Current NDK installed. $ANDROID_NDK contains the full path to the NDK root directory
##

source $(dirname $0)/ndk.sh

[[ -s "${ARCHIVE}" ]] || $(dirname $0)/collect-syms.sh
[[ -s "${ARCHIVE}" ]] || fatal "Error: archive [${ARCHIVE}] is missing or empty! Run collect-syms.sh first"
ls -aFl ${ARCHIVE}

[[ -n "${APPTOKEN}" ]] || APPTOKEN=$(find . -name newrelic.properties -exec grep com.newrelic.application_token= {} \; | cut -c32-)
[[ -n "${APPTOKEN}" ]] || fatal "APPTOKEN is empty!"
dbug "APPTOKEN[$APPTOKEN]"

CURL="curl -v"
# CURL+=" --proxy 127.0.0.1:8888"

URI=${URI:-"http://symbol-upload-api.staging-service.newrelic.com/symbol"}
HEADERS="-H X-APP-LICENSE-KEY:${APPTOKEN}"
HEADERS+=" -H X-APP-REQUEST-DEBUG:NRMA"

${CURL} $HEADERS -F dsym=@"${ARCHIVE}" $URI
echo $?
