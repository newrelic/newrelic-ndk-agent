/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _AGENT_NDK_SERIALIZER_H
#define _AGENT_NDK_SERIALIZER_H

#include <stddef.h>

/**
 * Pass a serialized native report to a delegate.
 * Code must be *very fast and light* since we're
 * called form a signal handler (mostly asysnc-safe)
 *
 * Default behaviour should be persist to disk *FIRST*
 * using async-safe i/o functions.
 */
class serializer {
public:
    /**
     * Pass a serialized crash report to its delegate.
     *
     * @param buffer character buffer containing the flattened crash report
     * @param cbsz size of cbuffer
     */
    static void from_crash(const char *buffer, size_t cbsz);

    /**
     * Pass a serialized exception to its delegate.
     *
     * @param buffer character buffer containing the flattened exception report
     * @param cbsz size of cbuffer
     */
    static void from_exception(const char *buffer, size_t cbsz);

    /**
     * Pass a ANR exception to its delegate.
     *
     * @param buffer character buffer containing the flattened exception report
     * @param cbsz size of cbuffer
     */
    static void from_anr(const char *buffer, size_t cbsz);

protected:
    /**
     * Write the payload locally using only MT thread-safe functions
     *
     * @param payload char buffer holding data
     * @param cbsz size of payload
     */
    static void to_storage(const char* filepath, const char *payload, size_t cbsz);
};


#endif // _AGENT_NDK_SERIALIZER_H
