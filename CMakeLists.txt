cmake_minimum_required(VERSION 3.15)

project(MC_DEV_TOOL_ROOT LANGUAGES CXX C)

option(MC_DEV_TOOL_BUILD_TEST "BUILD TEST EXECUTABLE" OFF)

# By Zero123
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    if(MSVC)
        add_compile_options(/utf-8 /EHsc)
        # 使用MT链接运行库
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()