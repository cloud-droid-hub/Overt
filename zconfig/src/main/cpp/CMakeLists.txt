cmake_minimum_required(VERSION 3.22.1)

project("zConfig")

add_library(${CMAKE_PROJECT_NAME} SHARED
        zConfigTest.cpp
        zConfig.h
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log)