cmake_minimum_required(VERSION 3.10)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

project(CppTaskManager VERSION 1.0 LANGUAGES CXX)

add_library(CppTaskManager STATIC
    src/ThreadPool.cpp
    src/ThreadPool.h
)

target_include_directories(CppTaskManager PUBLIC src)

# TEST
add_executable(CppTaskManagerExample
    example/main.cpp
)
target_link_libraries(CppTaskManagerExample PRIVATE CppTaskManager)
