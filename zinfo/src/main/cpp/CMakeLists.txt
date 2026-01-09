cmake_minimum_required(VERSION 3.22.1)

project("zInfo")

# 设置 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ANDROID_PLATFORM android-26)

include_directories(${CMAKE_SOURCE_DIR}/../../../../zconfig/src/main/cpp)
include_directories(${CMAKE_SOURCE_DIR}/../../../../zlog/src/main/cpp)
include_directories(${CMAKE_SOURCE_DIR}/../../../../zlibc/src/main/cpp)
include_directories(${CMAKE_SOURCE_DIR}/../../../../zstd/src/main/cpp)
include_directories(${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp)

add_library(${CMAKE_PROJECT_NAME} SHARED
        ${CMAKE_SOURCE_DIR}/../../../../zlog/src/main/cpp/zLog.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zlibc/src/main/cpp/zLibc.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zstd/src/main/cpp/zStdUtil.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zHttps.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zLinker.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zElf.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zClassLoader.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zCrc32.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zJavaVm.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zTee.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zBroadCast.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zFile.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zProcMaps.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zShell.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zZip.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zSha256.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zBinder.cpp

        zLocalNetworkInfo.cpp
        zIsoloatedProcess.cpp
        zRootStateInfo.cpp
        zSystemPropInfo.cpp
        zLinkerInfo.cpp
        zTimeInfo.cpp
        zPackageInfo.cpp
        zClassLoaderInfo.cpp
        zSystemSettingInfo.cpp
        zSideChannelInfo.cpp

        zProcInfo.cpp
        zPortInfo.cpp
        zTeeInfo.cpp
        zSslInfo.cpp
        zLogcatInfo.cpp

        zInfoTest.cpp)


# 添加mbedtls库
add_library(mbedtls STATIC IMPORTED)
set_target_properties(mbedtls PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/lib/mbedtls/libmbedtls.a)

add_library(mbedx509 STATIC IMPORTED)
set_target_properties(mbedx509 PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/lib/mbedtls/libmbedx509.a)

add_library(mbedcrypto STATIC IMPORTED)
set_target_properties(mbedcrypto PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/lib/mbedtls/libmbedcrypto.a)

# 包含头文件目录
include_directories(${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/include)


target_link_libraries(${CMAKE_PROJECT_NAME}
        mbedtls
        mbedx509
        mbedcrypto
        android
        log)