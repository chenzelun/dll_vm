# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Declares and names the project.

project("vm")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        vm

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        vm_main.cpp
        VmContext.cpp
        KeyFuncJni.cpp

        common/Util.cpp
        common/AndroidSystem.cpp

        data/VmDataFile.cpp
        data/VmKeyFuncCodeFile.cpp

        vm/Vm.cpp
        vm/JavaException.cpp
        vm/base/VmStack.cpp
        vm/base/VmCache.cpp
        vm/base/VmMethod.cpp
        vm/base/VmMemory.cpp

        vm/interpret/StandardInterpret.cpp
        vm/interpret/VmMethodCaller.cpp
        )


# include headers

target_include_directories(
        vm
        PRIVATE
        ${CMAKE_SOURCE_DIR}/include)


# add the third-lib

add_library(AK SHARED IMPORTED)

set_target_properties(
        AK
        PROPERTIES IMPORTED_LOCATION

        ${CMAKE_SOURCE_DIR}/../thirdLibs/${CMAKE_ANDROID_ARCH_ABI}/libAK.so)


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

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        vm
        AK
        android

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})