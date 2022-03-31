
message(STATUS "Using googletest via FetchContent.")
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.11.0
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  rsafefs_tests
  channel_test.cpp
  config_test.cpp
  data_cache_test.cpp
  local_test.cpp
  metadata_cache_test.cpp
  read_ahead_test.cpp
  utils_test.cpp
)

target_link_libraries(
  rsafefs_tests
  remote-safefs
  gtest_main
)

include(GoogleTest)
gtest_discover_tests(rsafefs_tests)