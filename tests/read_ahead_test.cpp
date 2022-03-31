#include "rsafefs/layers/local/local.hpp"
#include "rsafefs/layers/read_ahead/read_ahead.hpp"
#include <gtest/gtest.h>

using namespace rsafefs;

TEST(ReadAheadTest, EmptyConfig)
{
  YAML::Node config = YAML::Load("");
  ASSERT_NO_THROW(std::make_unique<read_ahead_config>(config));
}

TEST(ReadAheadTest, ValidConfig)
{
  YAML::Node config = YAML::Load("{size: 1024}");
  ASSERT_NO_THROW(std::make_unique<read_ahead_config>(config));
}

TEST(ReadAheadTest, WrongDataTypes)
{
  YAML::Node config = YAML::Load("{size: string}");
  ASSERT_ANY_THROW(std::make_unique<read_ahead_config>(config));
}

TEST(ReadAheadTest, InitLayerValid)
{
  YAML::Node rh_config = YAML::Load("");
  auto read_ahead_layer = std::make_unique<read_ahead_config>(rh_config);

  fuse_operations bottom_operations;
  memset(&bottom_operations, 0, sizeof(bottom_operations));

  bottom_operations.init = [](fuse_conn_info *conn) {
    return (void *)conn;
  };

  bottom_operations.destroy = [](void *) {
    return;
  };

  bottom_operations.open = [](const char *, fuse_file_info *) {
    return 0;
  };

  bottom_operations.read = [](const char *, char *, size_t, off_t, fuse_file_info *) {
    return 0;
  };

  bottom_operations.release = [](const char *, fuse_file_info *) {
    return 0;
  };

  bottom_operations.access = [](const char *, int) {
    return 0;
  };

  ASSERT_NO_THROW(read_ahead_layer->init_layer(bottom_operations));
}

TEST(ReadAheadTest, InitLayerInvalid)
{
  YAML::Node config = YAML::Load("");
  auto read_ahead_layer = std::make_unique<read_ahead_config>(config);

  fuse_operations bottom_operations;
  memset(&bottom_operations, 0, sizeof(bottom_operations));

  ASSERT_THROW(read_ahead_layer->init_layer(bottom_operations),
               utils::stack_operation_exception);
}