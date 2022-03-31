#include "rsafefs/layers/data_cache/data_cache.hpp"
#include <gtest/gtest.h>

using namespace rsafefs;

TEST(DataCacheTest, EmptyConfig)
{
  YAML::Node config = YAML::Load("");
  ASSERT_NO_THROW(std::make_unique<data_cache_config>(config));
}

TEST(DataCacheTest, ValidConfig)
{
  YAML::Node config = YAML::Load(
      "{size: 1073741824, block_size: 1024, time_out: 20, eviction_policy: rnd}");
  ASSERT_NO_THROW(std::make_unique<data_cache_config>(config));
}

TEST(DataCacheTest, WrongEvictionPolicy)
{
  YAML::Node config = YAML::Load("{eviction_policy: lfu}");
  ASSERT_THROW(std::make_unique<data_cache_config>(config),
               data_cache_wrong_config_exception);
}

TEST(DataCacheTest, WrongDataTypes)
{
  YAML::Node config = YAML::Load("{size: string}");
  ASSERT_ANY_THROW(std::make_unique<data_cache_config>(config));
}

TEST(DataCacheTest, InitLayerValid)
{
  YAML::Node config = YAML::Load("");
  auto data_cache_layer = std::make_unique<data_cache_config>(config);

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

  ASSERT_NO_THROW(data_cache_layer->init_layer(bottom_operations));
}

TEST(DataCacheTest, InitLayerInvalid)
{
  YAML::Node config = YAML::Load("");
  auto data_cache_layer = std::make_unique<data_cache_config>(config);

  fuse_operations bottom_operations;
  memset(&bottom_operations, 0, sizeof(bottom_operations));

  ASSERT_THROW(data_cache_layer->init_layer(bottom_operations),
               utils::stack_operation_exception);
}