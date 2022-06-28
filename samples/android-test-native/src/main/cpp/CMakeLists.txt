# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.

project("native_test")

add_compile_options(-Wall -Wextra -Wformat)
add_compile_options(-fvisibility=hidden -funwind-tables -fexceptions)
message(STATUS "Cmake build type is ${CMAKE_BUILD_TYPE}")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        native-lib
        # Sets the library as a shared library.
        SHARED
        # Provides a relative path to your source file(s).
        native-lib.cpp procfs.cpp
        )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library(log-lib log)
target_link_libraries(native-lib ${log-lib})


## Testing: load agent-ndk as a submodule
# add_subdirectory(agent-ndk)
# target_include_directories(agent-ndk-a PUBLIC agent-ndk)
# target_include_directories(agent-ndk-a PUBLIC agent-ndk/include)
# target_link_libraries(
#         native-lib
#         PUBLIC agent-ndk-a
#         ${log-lib})
