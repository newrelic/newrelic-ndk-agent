# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.11.0)

# Declares and names the project.

project("agent-ndk")

include(FetchContent)


if (0)
    # abi's don't have JNI?
    find_package(JNI REQUIRED)
    if (JNI_FOUND)
        message(STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
        message(STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
        include_directories(${JAVA_INCLUDE_PATH})
    else ()
        message(STATUS "JNI NOT FOUND")
    endif ()
endif ()

add_compile_options(-Wall -Wextra -Wformat)
add_compile_options(-fvisibility=hidden)

set(PROJECT_TEST_DIR ${CMAKE_HOME_DIRECTORY}/../../test/cpp)
message(STATUS "${PROJECT_TEST_DIR}")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        agent-ndk

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        agent-ndk.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

add_library(
        glue-lib
        STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        agent-ndk

        # Links the target library to the NDK libraries included in the NDK.
        ${log-lib}
        ${glue-lib}
        )


##
# Include testing framework(s) and test app
##

include(UnitTests.cmake)



