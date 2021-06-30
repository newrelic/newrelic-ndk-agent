# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.11.0)

project("agent-ndk-test")

find_package(JNI REQUIRED)
if (JNI_FOUND)
    message(STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message(STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
    include_directories(${JNI_INCLUDE_DIRS})
else ()
    message(STATUS "JNI NOT FOUND")
endif ()

include(FetchContent)

add_compile_options(-Wall -Wextra -Wformat)
add_compile_options(-fvisibility=hidden)

# Path to the CMake project
set( PROJECT_ROOT_DIR ../../.. )

# Path to the project-level build directory
set( PROJECT_BUILD_DIR ${PROJECT_ROOT_DIR}/build )

# FIXME
set(ARCH x86_64)

# Adds the CMakeLists.txt file located in the specified directory
# as a build dependency.
add_subdirectory(
        # Directory of the project's CMakeLists.txt
        ${PROJECT_ROOT_DIR}/src/main/cpp

        # Directory of the project's build output
        ${PROJECT_BUILD_DIR} )

include_directories(${PROJECT_ROOT_DIR}/include)

add_library(agent_ndk STATIC IMPORTED)


set_target_properties(
        # Specifies the target library
        agent_ndk

        # Specifies the parameter to define
        PROPERTIES IMPORTED_LOCATION

        # Provides the path to the library to import
        ${PROJECT_BUILD_DIR}/intermediates/cmake/debug/obj/${CMAKE_ANDROID_ARCH_ABI}/libagent-ndk.a)


##
# Testing frameworks:
#

# GTest/GMock: https://google.github.io/googletest/primer.html
#

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG master
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
        agent-ndk-test
        TestFixtures.cpp
        SimpleTests.cpp
)

target_link_libraries(
        agent-ndk-test
        # FIXME agent_ndk
        gtest_main
        gmock_main
)

include(GoogleTest)
gtest_discover_tests(agent-ndk-test)


##
# Catch: https://github.com/catchorg/Catch2
#

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v2.13.1)

FetchContent_MakeAvailable(Catch2)

add_executable(
        ndk-catch-test
        catch.cpp
)

target_link_libraries(ndk-catch-test PRIVATE Catch2::Catch2)

include(CTest)

if (FIXME)
    include(Catch)
    catch_discover_tests(ndk-catch-test)
endif ()


get_cmake_property(_variableNames VARIABLES)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

