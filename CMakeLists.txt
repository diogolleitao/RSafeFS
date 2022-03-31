cmake_minimum_required(VERSION 3.22)
project(RSafeFS VERSION 0.1.0 DESCRIPTION "RSafeFS: A Modular File System for Remote Storage")

include(FetchContent)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTS "Build unit tests" ON)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(FETCHCONTENT_QUIET OFF)
endif ()

message(STATUS "Using gRPC via FetchContent")
FetchContent_Declare(
  grpc
  GIT_REPOSITORY https://github.com/grpc/grpc.git
  GIT_TAG        v1.37.0
  GIT_PROGRESS   TRUE
)

message(STATUS "Using spdlog via FetchContent")
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.8.0
)

message(STATUS "Using fmt via FetchContent")
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        8.0.1
)

message(STATUS "Using CLI11 via FetchContent")
FetchContent_Declare(
  CLI11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG        v2.1.2
)

message(STATUS "Using yaml-cpp via FetchContent")
FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG        yaml-cpp-0.7.0
)

message(STATUS "Using asio via FetchContent.")
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-22-1
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
)

FetchContent_MakeAvailable(gRPC spdlog fmt CLI11 yaml-cpp asio)

# Find required packages installations
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(FUSE REQUIRED fuse)

# Build lib subdirectory
add_subdirectory(lib)

# Build src subdirectory
add_subdirectory(src)

if (BUILD_TESTS)
  # Build tests subdirectory
  add_subdirectory(tests)
endif (BUILD_TESTS)