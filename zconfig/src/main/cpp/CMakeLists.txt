cmake_minimum_required(VERSION 3.22.1)

project("zConfig")

# 设置 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${CMAKE_PROJECT_NAME} SHARED
        zConfigTest.cpp
        zConfig.h
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log)
