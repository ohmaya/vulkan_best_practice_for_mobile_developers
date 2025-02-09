#[[
 Copyright (c) 2019, Arm Limited and Contributors

 SPDX-License-Identifier: MIT

 Permission is hereby granted, free of charge,
 to any person obtaining a copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 ]]

cmake_minimum_required(VERSION 3.8)

set(SCRIPT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(ROOT_DIR ${SCRIPT_DIR}/../..)
set(GRADLE_BUILD_FILE ${SCRIPT_DIR}/template/gradle/build.gradle.in)
set(GRADLE_SETTINGS_FILE ${SCRIPT_DIR}/template/gradle/settings.gradle.in)
set(VALID_ABI "armeabi" "armeabi-v7a" "arm64-v8a" "x86" "x86_64")

include(${SCRIPT_DIR}/utils.cmake)

# script parameters
set(ANDROID_API 24 CACHE STRING "")
set(ANDROID_MANIFEST "AndroidManifest.xml" CACHE STRING "")
set(ARCH_ABI "arm64-v8a;armeabi-v7a" CACHE STRING "")
set(ASSET_DIRS "assets" CACHE PATHS "")
set(RES_DIRS "res" CACHE PATHS "")
set(JAVA_DIRS "java" CACHE PATHS "")
set(JNI_LIBS_DIRS "jni" CACHE PATHS "")
set(NATIVE_SCRIPT "CMakeLists.txt" CACHE FILE "")
set(NATIVE_ARGUMENTS "ANDROID_TOOLCHAIN=clang;ANDROID_STL=c++_static" CACHE STRING "")
set(OUTPUT_DIR "${ROOT_DIR}/build/android_gradle" CACHE PATH "")

# minSdkVersion
set(MIN_SDK_VERSION "minSdkVersion ${ANDROID_API}")

# manifest.srcFile
if(NOT IS_ABSOLUTE ${ANDROID_MANIFEST})
    set(ANDROID_MANIFEST ${CMAKE_SOURCE_DIR}/${ANDROID_MANIFEST})
endif()

if(EXISTS ${ANDROID_MANIFEST})
    file(RELATIVE_PATH ANDROID_MANIFEST ${OUTPUT_DIR} ${ANDROID_MANIFEST})
    set(MANIFEST_FILE "manifest.srcFile '${ANDROID_MANIFEST}'")
else()
    message(FATAL_ERROR "Manifest file does not exists at `${ANDROID_MANIFEST}`")
endif()

# ndk.abiFilters
set(ABI_LIST)

foreach(ABI_FILTER ${ARCH_ABI})
    if(${ABI_FILTER} IN_LIST VALID_ABI)
        list(APPEND ABI_LIST ${ABI_FILTER})
    else()
        message(STATUS "Filter abi is invalid `${ABI_FILTER}`")
    endif()
endforeach()

string_join(
    GLUE "', '"
    INPUT ${ABI_LIST}
    OUTPUT ABI_LIST)

if(NOT ${ABI_LIST})
    set(NDK_ABI_FILTERS "ndk { abiFilters '${ABI_LIST}' }")
else()
    message(FATAL_ERROR "Minimum one android arch abi required.")
endif()

# assets.srcDirs
set(ASSETS_LIST)

foreach(ASSET_DIR ${ASSET_DIRS})
    if(NOT IS_ABSOLUTE ${ASSET_DIR})
        set(ASSET_DIR ${CMAKE_SOURCE_DIR}/${ASSET_DIR})
    endif()

    if(IS_DIRECTORY ${ASSET_DIR})
        file(RELATIVE_PATH ASSET_DIR ${OUTPUT_DIR} ${ASSET_DIR})
        list(APPEND ASSETS_LIST ${ASSET_DIR})
    else()
        message(STATUS "Asset dir not exists at `${ASSET_DIR}`")
    endif()   
endforeach()

string_join(
    GLUE "', '"    
    INPUT ${ASSETS_LIST}
    OUTPUT ASSETS_LIST)

if(NOT ${ASSETS_LIST})
    set(ASSETS_SRC_DIRS "assets.srcDirs += [ '${ASSETS_LIST}' ]")
endif()

# res.srcDirs
set(RES_LIST)

foreach(RES_DIR ${RES_DIRS})
    if(NOT IS_ABSOLUTE ${RES_DIR})
        set(RES_DIR ${CMAKE_SOURCE_DIR}/${RES_DIR})
    endif()

    if(IS_DIRECTORY ${RES_DIR})
        file(RELATIVE_PATH RES_DIR ${OUTPUT_DIR} ${RES_DIR})
        list(APPEND RES_LIST ${RES_DIR})
    else()
        message(STATUS "Resource dir not exists at `${RES_DIR}`")
    endif()
endforeach()

string_join(
    GLUE "', '"    
    INPUT ${RES_LIST}
    OUTPUT RES_LIST)

if(NOT ${RES_LIST})
    set(RES_SRC_DIRS "res.srcDirs += [ '${RES_LIST}' ]")
endif()

# java.srcDirs
set(JAVA_LIST)

foreach(JAVA_DIR ${JAVA_DIRS})
    if(NOT IS_ABSOLUTE ${JAVA_DIR})
        set(JAVA_DIR ${CMAKE_SOURCE_DIR}/${JAVA_DIR})
    endif()

    if(IS_DIRECTORY ${JAVA_DIR})
        file(RELATIVE_PATH JAVA_DIR ${OUTPUT_DIR} ${JAVA_DIR})
        list(APPEND JAVA_LIST ${JAVA_DIR})
    else()
        message(STATUS "Java dir not exists at `${JAVA_DIR}`")
    endif()
endforeach()

string_join(
    GLUE "', '"
    INPUT ${JAVA_LIST}
    OUTPUT JAVA_LIST)

if(NOT ${JAVA_LIST})
    set(JAVA_SRC_DIRS "java.srcDirs += [ '${JAVA_LIST}' ]")
endif()

# jniLibs.srcDirs
set(JNI_LIBS_DIR_LIST)

foreach(JNI_LIBS_DIR ${JNI_LIBS_DIRS})
    if(NOT IS_ABSOLUTE ${JNI_LIBS_DIR})
        set(JNI_LIBS_DIR ${CMAKE_SOURCE_DIR}/${JNI_LIBS_DIR})
    endif()

    if(IS_DIRECTORY ${JNI_LIBS_DIR})
        file(RELATIVE_PATH JNI_LIBS_DIR ${OUTPUT_DIR} ${JNI_LIBS_DIR})
        list(APPEND JNI_LIBS_DIR_LIST ${JNI_LIBS_DIR})
    else()
        message(STATUS "JNI lib dir not exists at `${JNI_LIBS_DIR}`")
    endif()
endforeach()

string_join(
    GLUE "', '"
    INPUT ${JNI_LIBS_DIR_LIST}
    OUTPUT JNI_LIBS_DIR_LIST)

if(NOT ${JNI_LIBS_DIR_LIST})
    set(JNI_LIBS_SRC_DIRS "jniLibs.srcDirs += [ '${JNI_LIBS_DIR_LIST}' ]")
endif()

# cmake.path
if(NOT IS_ABSOLUTE ${NATIVE_SCRIPT})
    set(NATIVE_SCRIPT ${CMAKE_SOURCE_DIR}/${NATIVE_SCRIPT})
endif()

if(EXISTS ${NATIVE_SCRIPT})
    file(RELATIVE_PATH NATIVE_SCRIPT_TMP ${OUTPUT_DIR} ${NATIVE_SCRIPT})

    set(CMAKE_PATH "cmake {\n\t\t\tpath '${NATIVE_SCRIPT_TMP}'\n\t\t\tbuildStagingDirectory \'build-native\'\n\t\t\tversion \'3.10.2\'\n\t\t} ")
endif()

# cmake.arguments
set(ARGS_LIST)

foreach(NATIVE_ARG ${NATIVE_ARGUMENTS})
    list(APPEND ARGS_LIST "-D${NATIVE_ARG}")
endforeach()

string_join(
    GLUE "', '"
    INPUT ${ARGS_LIST}
    OUTPUT ARGS_LIST)
    
if(NOT ${ARGS_LIST} AND EXISTS ${NATIVE_SCRIPT})
    set(CMAKE_ARGUMENTS "cmake {\n\t\t\t\targuments '${ARGS_LIST}' \n\t\t\t}")
endif()

configure_file(${GRADLE_BUILD_FILE} ${OUTPUT_DIR}/build.gradle)
configure_file(${GRADLE_SETTINGS_FILE} ${OUTPUT_DIR}/settings.gradle)

message(STATUS "Android Gradle Project (With Native Support) generated at:\n\t${OUTPUT_DIR}")
