#include "rsafefs/layers/metadata_cache/metadata_cache.hpp"
#include "rsafefs/layers/metadata_cache/cache.hpp"
#include "rsafefs/layers/metadata_cache/drivers/lru.hpp"
#include "rsafefs/layers/metadata_cache/drivers/rnd.hpp"
#include "rsafefs/utils/logging.hpp"
#include "rsafefs/utils/utils.hpp"
#include <filesystem>

namespace rsafefs
{

namespace fs = std::filesystem;

static fuse_operations next_layer;
static metadata_cache::cache::config config;
static metadata_cache::cache *cache = nullptr;

static void *
metadata_cache_init(fuse_conn_info *conn)
{
  cache = new metadata_cache::cache(config);
  if (next_layer.init != nullptr) {
    return next_layer.init(conn);
  }
  return nullptr;
}

static void
metadata_cache_destroy(void *private_data)
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
metadata_cache_getattr(const char *path, struct stat *stbuf)
{
  if (cache->get(path, stbuf)) {
    return 0;
  }

  const int res = next_layer.getattr(path, stbuf);

  if (res == 0) {
    cache->put(path, stbuf);
  }

  return res;
}

static int
metadata_cache_fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
  if (cache->get(path, stbuf)) {
    return 0;
  }

  const int res = next_layer.fgetattr(path, stbuf, fi);

  if (res == 0) {
    cache->put(path, stbuf);
  }

  return res;
}

static int
metadata_cache_mknod(const char *path, mode_t mode, dev_t rdev)
{
  cache->remove(path);
  fs::path p = path;
  if (p.has_parent_path()) {
    cache->remove(p.parent_path());
  }
  return next_layer.mknod(path, mode, rdev);
}

static int
metadata_cache_mkdir(const char *path, mode_t mode)
{
  cache->remove(path);
  fs::path p = path;
  if (p.has_parent_path()) {
    cache->remove(p.parent_path());
  }
  return next_layer.mkdir(path, mode);
}

static int
metadata_cache_symlink(const char *from, const char *to)
{
  cache->remove(from);
  cache->remove(to);
  fs::path t = to;
  if (t.has_parent_path()) {
    cache->remove(t.parent_path());
  }
  return next_layer.symlink(from, to);
}

static int
metadata_cache_unlink(const char *path)
{
  cache->remove(path);
  fs::path p = path;
  if (p.has_parent_path()) {
    cache->remove(p.parent_path());
  }
  return next_layer.unlink(path);
}

static int
metadata_cache_rmdir(const char *path)
{
  cache->remove(path);
  fs::path p = path;
  if (p.has_parent_path()) {
    cache->remove(p.parent_path());
  }
  return next_layer.rmdir(path);
}

static int
metadata_cache_rename(const char *from, const char *to)
{
  cache->remove(from);
  cache->remove(to);
  fs::path f = from;
  if (f.has_parent_path()) {
    cache->remove(f.parent_path());
  }
  fs::path t = to;
  if (t.has_parent_path()) {
    cache->remove(t.parent_path());
  }
  return next_layer.rename(from, to);
}

static int
metadata_cache_link(const char *from, const char *to)
{
  cache->remove(from);
  cache->remove(to);
  fs::path t = to;
  if (t.has_parent_path()) {
    cache->remove(t.parent_path());
  }
  return next_layer.link(from, to);
}

static int
metadata_cache_chmod(const char *path, mode_t mode)
{
  cache->remove(path);
  return next_layer.chmod(path, mode);
}

static int
metadata_cache_chown(const char *path, uid_t uid, gid_t gid)
{
  cache->remove(path);
  return next_layer.chown(path, uid, gid);
}

static int
metadata_cache_truncate(const char *path, off_t size)
{
  cache->remove(path);
  return next_layer.truncate(path, size);
}

static int
metadata_cache_ftruncate(const char *path, off_t size, struct fuse_file_info *fi)
{
  cache->remove(path);
  return next_layer.ftruncate(path, size, fi);
}

static int
metadata_cache_utimens(const char *path, const struct timespec ts[2])
{
  cache->remove(path);
  return next_layer.utimens(path, ts);
}

static int
metadata_cache_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  cache->remove(path);
  fs::path p = path;
  if (p.has_parent_path()) {
    cache->remove(p.parent_path());
  }
  return next_layer.create(path, mode, fi);
}

static int
metadata_cache_open(const char *path, struct fuse_file_info *fi)
{
  cache->remove(path);
  return next_layer.open(path, fi);
}

static int
metadata_cache_flush(const char *path, struct fuse_file_info *fi)
{
  cache->remove(path);
  return next_layer.flush(path, fi);
}

static int
metadata_cache_release(const char *path, struct fuse_file_info *fi)
{
  cache->remove(path);
  return next_layer.release(path, fi);
}

static int
metadata_cache_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
  cache->remove(path);
  return next_layer.fsync(path, isdatasync, fi);
}

metadata_cache_config::metadata_cache_config(YAML::Node data)
{
  logging::debug("configuring metadata caching layer...");

  // Default configuration
  config.size_ = 100UL * 1024UL * 1024UL; // 100 MiB
  config.time_out_ = 60;                  // 60 seconds
  config.eviction_policy_ =
      std::make_shared<metadata_cache::rnd_eviction>(); // random eviction

  parser_.emplace("size", [&]() {
    config.size_ = data["size"].as<size_t>();
  });

  parser_.emplace("time_out", [&]() {
    config.time_out_ = data["time_out"].as<int>();
  });

  parser_.emplace("eviction_policy", [&]() {
    const std::string eviction_policy = data["eviction_policy"].as<std::string>();
    if (eviction_policy == "lru") {
      config.eviction_policy_ = std::make_shared<metadata_cache::lru_eviction>();
    } else if (eviction_policy == "rnd") {
      config.eviction_policy_ = std::make_shared<metadata_cache::rnd_eviction>();
    } else {
      throw metadata_cache_wrong_config_exception("invalid replacement policy");
    }
  });

  for (const auto &kv : data) {
    const std::string &option = kv.first.as<std::string>();
    if (parser_.contains(option)) {
      parser_.at(option)();
    } else {
      logging::warn("Ignoring option: \"{}\", metadata cache layer doesn't recognises it",
                    option);
    }
  }
}

metadata_cache_config::~metadata_cache_config() {}

void
metadata_cache_config::init_layer(fuse_operations &operations)
{
  logging::debug("init metadata cache layer...");

  // Copy the fuse operations from the lower layer
  next_layer = operations;

  // Update the fuse operations
  utils::stack_operation(metadata_cache_init, operations.init);
  utils::stack_operation(metadata_cache_destroy, operations.destroy);
  utils::stack_operation(metadata_cache_getattr, operations.getattr);
  utils::stack_operation(metadata_cache_fgetattr, operations.fgetattr);
  utils::stack_operation(metadata_cache_mknod, operations.mknod);
  utils::stack_operation(metadata_cache_mkdir, operations.mkdir);
  utils::stack_operation(metadata_cache_symlink, operations.symlink);
  utils::stack_operation(metadata_cache_unlink, operations.unlink);
  utils::stack_operation(metadata_cache_rmdir, operations.rmdir);
  utils::stack_operation(metadata_cache_rename, operations.rename);
  utils::stack_operation(metadata_cache_link, operations.link);
  utils::stack_operation(metadata_cache_chmod, operations.chmod);
  utils::stack_operation(metadata_cache_chown, operations.chown);
  utils::stack_operation(metadata_cache_truncate, operations.truncate);
  utils::stack_operation(metadata_cache_ftruncate, operations.ftruncate);
  utils::stack_operation(metadata_cache_create, operations.create);
  utils::stack_operation(metadata_cache_open, operations.open);
  utils::stack_operation(metadata_cache_flush, operations.flush);
  utils::stack_operation(metadata_cache_release, operations.release);
  utils::stack_operation(metadata_cache_fsync, operations.fsync);
}

void
metadata_cache_config::clean_layer()
{
  logging::debug("cleaning metadata cache layer...");
}

} // namespace rsafefs