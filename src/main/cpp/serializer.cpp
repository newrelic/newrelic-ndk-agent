/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <jni.h>
#include <cstdio>
#include <iostream>
#include <fstream>

#include <agent-ndk.h>
#include "jni/native-context.h"
#include "jni/jni-delegate.h"
#include "serializer.h"

/**
    Consider using https://github.com/kgabis/parson.git for Json persistence

    void serialization_example(void) {
        JSON_Value *root_value = json_value_init_object();
        JSON_Object *root_object = json_value_get_object(root_value);
        char *serialized_string = NULL;
        json_object_set_string(root_object, "name", "John Smith");
        json_object_set_number(root_object, "age", 25);
        json_object_dotset_string(root_object, "address.city", "Cupertino");
        json_object_dotset_value(root_object, "contact.emails", json_parse_string("[\"email@example.com\",\"email2@example.com\"]"));
        serialized_string = json_serialize_to_string_pretty(root_value);
        puts(serialized_string);
        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
    }
*/

void serializer::from_crash(const char *buffer, size_t buffsz) {
    to_storage("FIXME-crash.dat", buffer, buffsz);
    on_native_crash(buffer);
}

void serializer::from_exception(const char *buffer, __unused size_t buffsz) {
    to_storage("FIXME-ex.dat", buffer, buffsz);
    on_native_exception(buffer);
}

void serializer::from_anr(const char *buffer, __unused size_t buffsz) {
    to_storage("FIXME-anr.dat", buffer, buffsz);
    on_application_not_responding(buffer);
}

/**
 * Write the payload locally using only MT thread-safe functions

 * @param payload
 * @param payloadSize
 */
void serializer::to_storage(const char *filepath, const char *payload, size_t payloadSize) {
    jni::native_context_t &native_context = jni::get_native_context();
    std::string report_path(native_context.report_path_absolute);

    report_path.append("/");
    report_path.append(filepath);
    std::ofstream os{report_path.c_str(), std::ios::out | std::ios::binary};

    if (!os) {
        _LOGE("ofstream error %d: %s", errno, strerror(errno));
    } else {
        os.write(payload, payloadSize);
        os.flush();
        os.close();
    }
}
