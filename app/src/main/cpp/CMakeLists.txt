cmake_minimum_required(VERSION 3.22.1)

project("overt")

# 设置 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ANDROID_PLATFORM android-26)

# 基础优化选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -fvisibility=hidden -Os")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections -fvisibility=hidden -Os")

# 启用链接时剔除无用代码（--gc-sections）
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")

# 额外的体积优化选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables")

# 针对 ARM 架构的额外优化
if(ANDROID_ABI STREQUAL "arm64-v8a")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a -mtune=cortex-a76")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a -mtune=cortex-a76")
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mtune=cortex-a9 -mfpu=neon")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mtune=cortex-a9 -mfpu=neon")
endif()

# 链接时符号处理 - 隐藏静态库符号
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--strip-all")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs,ALL")

# 额外的符号隐藏选项
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/symbols.map")

include_directories(${CMAKE_SOURCE_DIR}/../../../../zconfig/src/main/cpp)
include_directories(${CMAKE_SOURCE_DIR}/../../../../zlog/src/main/cpp)
include_directories(${CMAKE_SOURCE_DIR}/../../../../zlibc/src/main/cpp)
include_directories(${CMAKE_SOURCE_DIR}/../../../../zstd/src/main/cpp)
include_directories(${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp)
include_directories(${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp)

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
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zTeeCert.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zTee.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zBroadCast.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zFile.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zProcMaps.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zTask.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zThread.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zThreadPool.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zShell.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zZip.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zSha256.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zBinder.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/zSensorManager.cpp

        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zRootStateInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zSystemPropInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zLinkerInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zTimeInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zPackageInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zClassLoaderInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zSystemSettingInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zProcInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zPortInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zTeeInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zSslInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zLocalNetworkInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zIsoloatedProcess.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zLogcatInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zSignatureInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zSideChannelInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zSensorInfo.cpp
        ${CMAKE_SOURCE_DIR}/../../../../zinfo/src/main/cpp/zFingerInfo.cpp

        zManager.cpp

        native-lib.cpp)

include_directories(${CMAKE_SOURCE_DIR}/../../../../zcore/src/main/cpp/include)

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

target_link_libraries(${CMAKE_PROJECT_NAME}
        mbedtls
        mbedx509
        mbedcrypto
        android
        mediandk
        log)