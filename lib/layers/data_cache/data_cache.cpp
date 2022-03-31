#include "rsafefs/layers/data_cache/data_cache.hpp"
#include "rsafefs/layers/data_cache/cache.hpp"
#include "rsafefs/layers/data_cache/drivers/lru.hpp"
#include "rsafefs/layers/data_cache/drivers/rnd.hpp"
#include "rsafefs/utils/logging.hpp"
#include "rsafefs/utils/utils.hpp"

namespace rsafefs
{

static fuse_operations next_layer;
static data_cache::cache::config config;
static data_cache::cache *cache = nullptr;

static void *
data_cache_init(fuse_conn_info *conn)
{
  cache = new data_cache::cache(config, next_layer);
  if (next_layer.init != nullptr) {
    return next_layer.init(conn);
  }
  return nullptr;
}

static void
data_cache_destroy(void *private_data)
{
  if (cache != nullptr) {
    delete cache;
    cache = nullptr;
  }

  if (next_layer.destroy != nullptr) {
    next_layer.destroy(private_data);
  }
}

static int
data_cache_open(const char *path, struct fuse_file_info *fi)
{
  return cache->open(path, fi);
}

static int
data_cache_read(const char *path, char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi)
{
  return cache->read(path, buf, size, offset, fi);
}

static int
data_cache_release(const char *path, struct fuse_file_info *fi)
{
  return cache->release(path, fi);
}

data_cache_config::data_cache_config(YAML::Node data)
{
  logging::debug("configuring data caching layer...");

  // Default configuration
  config.size_ = 1UL * 1024UL * 1024UL * 1024UL; // 1 GiB
  config.block_size_ = 16UL * 1024UL;            // 16 KiB
  config.time_out_ = 30;                         // 30 seconds
  config.eviction_policy_ =
      std::make_shared<data_cache::rnd_eviction>(); // random eviction

  parser_.emplace("size", [&]() {
    config.size_ = data["size"].as<size_t>();
  });

  parser_.emplace("block_size", [&]() {
    config.block_size_ = data["block_size"].as<size_t>();
  });

  parser_.emplace("time_out", [&]() {
    config.time_out_ = data["time_out"].as<int>();
  });

  parser_.emplace("eviction_policy", [&]() {
    const std::string eviction_policy = data["eviction_policy"].as<std::string>();
    if (eviction_policy == "lru") {
      config.eviction_policy_ = std::make_shared<data_cache::lru_eviction>();
    } else if (eviction_policy == "rnd") {
      config.eviction_policy_ = std::make_shared<data_cache::rnd_eviction>();
    } else {
      throw data_cache_wrong_config_exception("invalid replacement policy");
    }
  });

  for (const auto &kv : data) {
    const std::string &option = kv.first.as<std::string>();
    if (parser_.contains(option)) {
      parser_.at(option)();
    } else {
      logging::warn("Ignoring option: \"{}\", data cache layer doesn't recognises it",
                    option);
    }
  }
}

data_cache_config::~data_cache_config() {}

void
data_cache_config::init_layer(fuse_operations &operations)
{
  logging::debug("init data cache layer...");

  // Copy the fuse operations from the lower layer
  next_layer = operations;

  // Update the fuse operations
  utils::stack_operation(data_cache_init, operations.init);
  utils::stack_operation(data_cache_destroy, operations.destroy);
  utils::stack_operation(data_cache_open, operations.open);
  utils::stack_operation(data_cache_read, operations.read);
  utils::stack_operation(data_cache_release, operations.release);
}

void
data_cache_config::clean_layer()
{
  logging::debug("cleaning data cache layer...");
}

} // namespace rsafefs