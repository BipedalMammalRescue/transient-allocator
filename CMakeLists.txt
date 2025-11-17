cmake_minimum_required(VERSION 3.31)

project(transient_allocator)

add_executable(App app.cpp)
target_include_directories(App PUBLIC CMAKE_SOURCE_DIR)