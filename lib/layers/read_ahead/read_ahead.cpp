#include "rsafefs/layers/read_ahead/read_ahead.hpp"
#include "rsafefs/layers/read_ahead/read_ahead_cache.hpp"
#include "rsafefs/utils/logging.hpp"
#include "rsafefs/utils/utils.hpp"

namespace rsafefs
{

static fuse_operations next_layer;
static read_ahead::cache::config config;
static read_ahead::cache *cache = nullptr;

static void *
read_ahead_init(fuse_conn_info *conn)
{
  cache = new read_ahead::cache(config, next_layer);

  if (next_layer.init != nullptr) {
    return next_layer.init(conn);
  }

  return nullptr;
}

static void
read_ahead_destroy(void *private_data)
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
read_ahead_open(const char *path, struct fuse_file_info *fi)
{
  return cache->open(path, fi);
}

static int
read_ahead_read(const char *path, char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi)
{
  return cache->read(path, buf, size, offset, fi);
}

static int
read_ahead_release(const char *path, struct fuse_file_info *fi)
{
  return cache->release(path, fi);
}

read_ahead_config::read_ahead_config(YAML::Node data)
{
  logging::debug("configuring read ahead layer...");

  // Default configuration
  config.size_ = 1UL * 1024UL * 1024UL; // 1 MiB

  parser_.emplace("size", [&]() {
    config.size_ = data["size"].as<size_t>();
  });

  for (const auto &kv : data) {
    const std::string &option = kv.first.as<std::string>();
    if (parser_.contains(option)) {
      parser_.at(option)();
    } else {
      logging::warn("Ignoring option: \"{}\", read ahead layer doesn't recognises it",
                    option);
    }
  }
}

read_ahead_config::~read_ahead_config() {}

void
read_ahead_config::init_layer(fuse_operations &operations)
{
  logging::debug("init read ahead layer...");

  // Copy the fuse operations from the lower layer
  next_layer = operations;

  // Update the fuse operations
  utils::stack_operation(read_ahead_init, operations.init);
  utils::stack_operation(read_ahead_destroy, operations.destroy);
  utils::stack_operation(read_ahead_open, operations.open);
  utils::stack_operation(read_ahead_read, operations.read);
  utils::stack_operation(read_ahead_release, operations.release);
}

void
read_ahead_config::clean_layer()
{
  logging::debug("cleaning read ahead layer...");
}

} // namespace rsafefs