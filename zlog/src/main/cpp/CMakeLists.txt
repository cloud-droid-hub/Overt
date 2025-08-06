cmake_minimum_required(VERSION 3.22.1)

project("zLog")

# 设置 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/../../../../zconfig/src/main/cpp)

add_library(${CMAKE_PROJECT_NAME} SHARED
        zLog.cpp
        zLogTest.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log)