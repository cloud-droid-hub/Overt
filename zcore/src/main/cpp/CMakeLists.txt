cmake_minimum_required(VERSION 3.22.1)

project("zCore")

# 设置 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/../../../../zconfig/src/main/cpp)
include_directories(${CMAKE_SOURCE_DIR}/../../../../zlog/src/main/cpp)
include_directories(${CMAKE_SOURCE_DIR}/../../../../zlibc/src/main/cpp)
include_directories(${CMAKE_SOURCE_DIR}/../../../../zstd/src/main/cpp)

add_library(${CMAKE_PROJECT_NAME} SHARED
        ${CMAKE_SOURCE_DIR}/../../../../zlog/src/main/cpp/zLog.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zlibc/src/main/cpp/zLibc.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zstd/src/main/cpp/zString.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zstd/src/main/cpp/zStdUtil.cpp

        zHttps.cpp
        zLinker.cpp
        zElf.cpp
        zClassLoader.cpp
        zCrc32.cpp
        zFile.cpp
        zJavaVm.cpp
        zTee.cpp
        zBroadCast.cpp
        zJson.cpp

        zCoreTest.cpp)

# 添加mbedtls库
add_library(mbedtls STATIC IMPORTED)
set_target_properties(mbedtls PROPERTIES IMPORTED_LOCATION
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/mbedtls/libmbedtls.a)

add_library(mbedx509 STATIC IMPORTED)
set_target_properties(mbedx509 PROPERTIES IMPORTED_LOCATION
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/mbedtls/libmbedx509.a)

add_library(mbedcrypto STATIC IMPORTED)
set_target_properties(mbedcrypto PROPERTIES IMPORTED_LOCATION
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/mbedtls/libmbedcrypto.a)

# 包含头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${CMAKE_PROJECT_NAME}
        mbedtls
        mbedx509
        mbedcrypto
        android
        log)