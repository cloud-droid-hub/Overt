cmake_minimum_required(VERSION 3.22.1)

project("overt")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 基础优化选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -fvisibility=hidden -Os")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections -fvisibility=hidden -Os")

# 启用链接时剔除无用代码（--gc-sections）
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")

# 开启 LTO（可选）进一步减小体积
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto")

# 设置编译类型为 MinSizeRel（Release 的最小尺寸版本）
set(CMAKE_BUILD_TYPE MinSizeRel)

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

# 禁用调试信息和符号表
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")

# 链接时符号处理 - 隐藏静态库符号
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--strip-all")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs,ALL")

# 额外的符号隐藏选项
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/symbols.map")



add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        zHttps.cpp

        zUtil.cpp
        root_file_info.cpp
        mounts_info.cpp
        system_prop_info.cpp
        linker_info.cpp
        time_info.cpp
        zClassLoader.cpp
        zElf.cpp
        zLinker.cpp
        package_info.cpp
        class_loader_info.cpp
        zJavaVm.cpp
        system_setting_info.cpp
        zDevice.cpp
        zFile.cpp
        maps_info.cpp
        task_info.cpp
        zCrc32.cpp
        port_info.cpp
        tee_info.cpp
        tee_cert_parser.cpp


        ssl_info.cpp

        zLog.cpp
        nonstd_libc.cpp
        string.cpp
        native-lib.cpp)

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