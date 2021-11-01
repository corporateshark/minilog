cmake_minimum_required(VERSION 3.10)

project(minilog CXX C)

option(MINILOG_BUILD_EXAMPLE "Build example" ON)

message(STATUS "MINILOG_BUILD_EXAMPLE = ${MINILOG_BUILD_EXAMPLE}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(minilog minilog.cpp minilog.h)

if(MINILOG_BUILD_EXAMPLE)
	set(CMAKE_CXX_STANDARD 20)
	add_executable(minilog_example example.cpp)
	target_link_libraries(minilog_example minilog)
endif()
