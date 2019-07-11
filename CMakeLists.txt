# Specify the minimum version for CMake
cmake_minimum_required(VERSION 3.0)

# Project's name
project(barbarossa)

# Set the output folder where your program will be created
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# Set C++11 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compiler flags
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -flto -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -msse4")
link_libraries("-flto -Wl,--gc-sections")

# set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_CXX_FLAGS "-msse4 -O2 -Wall -Wextra -flto -ffunction-sections -fdata-sections")
# add_compile_options("-msse4 -O2 -Wall -Wextra -flto -ffunction-sections -fdata-sections")
# link_libraries("-flto -Wl,--gc-sections")
# n -ffunction-section & -fdata-sections options of gcc ...

# Include files
include_directories("${PROJECT_SOURCE_DIR}/include")
# Source files
file(GLOB SOURCES "src/*.cpp")

#
# Third party sources
#

# Abseil C++
#set(ABSL_RUN_TESTS OFF)
#set(ABSL_USE_GOOGLETEST_HEAD OFF)
#set(BUILD_TESTING OFF)
#add_subdirectory(thirdparty/abseil-cpp)

# JSON
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(thirdparty/nlohmann/json)

# Websocketpp
#set(THREADS_PREFER_PTHREAD_FLAG ON)
#find_package(Threads REQUIRED)
#include_directories(thirdparty/websocketpp)
#include_directories(thirdparty/asio-1.12.2/include)
#add_subdirectory(thirdparty/websocketpp)

# spdlog
#include_directories(thirdparty/spdlog/include)
#add_subdirectory(thirdparty/spdlog)

# Our target
add_executable(barbarossa ${SOURCES})
# absl::base absl::memory 
#target_link_libraries(learncpp Threads::Threads absl::memory nlohmann_json::nlohmann_json)
target_link_libraries(barbarossa nlohmann_json::nlohmann_json)