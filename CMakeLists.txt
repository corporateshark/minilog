cmake_minimum_required(VERSION 3.19)

project(minilog CXX C)

option(MINILOG_BUILD_EXAMPLE "Build example" ON)
option(MINILOG_RAW_OUTPUT    "Do not apply extra formatting" OFF)

message(STATUS "MINILOG_BUILD_EXAMPLE = ${MINILOG_BUILD_EXAMPLE}")
message(STATUS "MINILOG_RAW_OUTPUT    = ${MINILOG_RAW_OUTPUT}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(minilog minilog.cpp minilog.h)

if(ANDROID)
	target_link_libraries(minilog PUBLIC log)
endif()

if(MINILOG_RAW_OUTPUT)
	target_compile_definitions(minilog PUBLIC "-DMINILOG_RAW_OUTPUT=1")
endif()

if(MINILOG_BUILD_EXAMPLE)
	set(CMAKE_CXX_STANDARD 20)
	add_executable(minilog_example example.cpp)
	target_link_libraries(minilog_example minilog)
	if(MSVC)
		target_compile_definitions(minilog_example PRIVATE _CRT_SECURE_NO_WARNINGS)
	endif()
endif()
