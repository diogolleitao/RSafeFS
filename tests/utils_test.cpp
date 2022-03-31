#include "rsafefs/layers/data_cache/data_cache.hpp"
#include "rsafefs/layers/metadata_cache/metadata_cache.hpp"
#include "rsafefs/utils/utils.hpp"
#include "yaml-cpp/yaml.h"
#include <gtest/gtest.h>

using namespace rsafefs;

TEST(UtilsTest, InstanceOfValid)
{
  YAML::Node config = YAML::Load("");
  std::unique_ptr<layer_config> layer = std::make_unique<data_cache_config>(config);
  ASSERT_TRUE(utils::instance_of<data_cache_config>(layer.get()));
}

TEST(UtilsTest, InstanceOfInvalid)
{
  YAML::Node config = YAML::Load("");
  std::unique_ptr<layer_config> layer = std::make_unique<data_cache_config>(config);
  ASSERT_FALSE(utils::instance_of<metadata_cache_config>(layer.get()));
}

TEST(UtilsTest, StackNullOperation)
{
  fuse_operations top_operations;
  fuse_operations bottom_operations;

  bottom_operations.access = nullptr;

  ASSERT_THROW(utils::stack_operation(*top_operations.access, bottom_operations.access),
               utils::stack_operation_exception);
}

TEST(UtilsTest, StackValidOperation)
{
  fuse_operations top_operations;
  fuse_operations bottom_operations;

  // Simulate a valid access implementation
  bottom_operations.access = [](const char *, int) {
    return 0;
  };

  ASSERT_NO_THROW(
      utils::stack_operation(*top_operations.access, bottom_operations.access));
}