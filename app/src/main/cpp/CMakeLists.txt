cmake_minimum_required(VERSION 3.22.1)

project("overt")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")

add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        util.cpp
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
        crc.cpp
        port_info.cpp
        tee_info.cpp
        tee_cert_parser.cpp
        zLog.cpp
        native-lib.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log)