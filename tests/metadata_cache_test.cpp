#include "rsafefs/layers/metadata_cache/metadata_cache.hpp"
#include <gtest/gtest.h>

using namespace rsafefs;

TEST(MetadataCacheTest, EmptyConfig)
{
  YAML::Node config = YAML::Load("");
  ASSERT_NO_THROW(std::make_unique<metadata_cache_config>(config));
}

TEST(MetadataCacheTest, ValidConfig)
{
  YAML::Node config =
      YAML::Load("{size: 1073741824, time_out: 20, eviction_policy: rnd}");
  ASSERT_NO_THROW(std::make_unique<metadata_cache_config>(config));
}

TEST(MetadataCacheTest, WrongEvictionPolicy)
{
  YAML::Node config = YAML::Load("{eviction_policy: lfu}");
  ASSERT_THROW(std::make_unique<metadata_cache_config>(config),
               metadata_cache_wrong_config_exception);
}

TEST(MetadataCacheTest, WrongDataTypes)
{
  YAML::Node config = YAML::Load("{size: string}");
  ASSERT_ANY_THROW(std::make_unique<metadata_cache_config>(config));
}

TEST(MetadataCacheTest, InitLayerValid)
{
  YAML::Node config = YAML::Load("");
  auto metadata_cache_layer = std::make_unique<metadata_cache_config>(config);

  fuse_operations bottom_operations;
  memset(&bottom_operations, 0, sizeof(bottom_operations));

  bottom_operations.init = [](fuse_conn_info *conn) {
    return (void *)conn;
  };

  bottom_operations.destroy = [](void *) {
    return;
  };

  bottom_operations.getattr = [](const char *, struct stat *) {
    return 0;
  };

  bottom_operations.fgetattr = [](const char *, struct stat *, fuse_file_info *) {
    return 0;
  };

  bottom_operations.mknod = [](const char *, mode_t, dev_t) {
    return 0;
  };

  bottom_operations.mkdir = [](const char *, mode_t) {
    return 0;
  };

  bottom_operations.symlink = [](const char *, const char *) {
    return 0;
  };

  bottom_operations.unlink = [](const char *) {
    return 0;
  };

  bottom_operations.rmdir = [](const char *) {
    return 0;
  };

  bottom_operations.rename = [](const char *, const char *) {
    return 0;
  };

  bottom_operations.link = [](const char *, const char *) {
    return 0;
  };

  bottom_operations.chmod = [](const char *, mode_t) {
    return 0;
  };

  bottom_operations.chown = [](const char *, uid_t, gid_t) {
    return 0;
  };

  bottom_operations.truncate = [](const char *, off_t) {
    return 0;
  };

  bottom_operations.ftruncate = [](const char *, off_t, fuse_file_info *) {
    return 0;
  };

  bottom_operations.create = [](const char *, mode_t, fuse_file_info *) {
    return 0;
  };

  bottom_operations.open = [](const char *, fuse_file_info *) {
    return 0;
  };

  bottom_operations.flush = [](const char *, fuse_file_info *) {
    return 0;
  };

  bottom_operations.release = [](const char *, fuse_file_info *) {
    return 0;
  };

  bottom_operations.fsync = [](const char *, int, fuse_file_info *) {
    return 0;
  };

  ASSERT_NO_THROW(metadata_cache_layer->init_layer(bottom_operations));
}

TEST(MetadataCacheTest, InitLayerInvalid)
{
  YAML::Node config = YAML::Load("");
  auto metadata_cache_layer = std::make_unique<metadata_cache_config>(config);

  fuse_operations bottom_operations;
  memset(&bottom_operations, 0, sizeof(bottom_operations));

  ASSERT_THROW(metadata_cache_layer->init_layer(bottom_operations),
               utils::stack_operation_exception);
}