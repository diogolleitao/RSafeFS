#include "rsafefs/layers/rpc_client/rpc_client.hpp"
#include "rsafefs/fuse_rpc/client.hpp"
#include "rsafefs/fuse_rpc/grpc/async_client.hpp"
#include "rsafefs/fuse_rpc/grpc/sync_client.hpp"
#include "rsafefs/utils/logging.hpp"
#include "rsafefs/utils/utils.hpp"

namespace rsafefs
{

static fuse_rpc::client::config *config = nullptr;
static fuse_rpc::client *client = nullptr;

static void *
rpc_client_init(fuse_conn_info *conn)
{
  if (utils::instance_of<fuse_rpc::grpc::async_client::config>(config)) {
    auto async_config = dynamic_cast<fuse_rpc::grpc::async_client::config *>(config);
    client = new fuse_rpc::grpc::async_client(*async_config);
  } else if (utils::instance_of<fuse_rpc::grpc::sync_client::config>(config)) {
    auto sync_config = dynamic_cast<fuse_rpc::grpc::sync_client::config *>(config);
    client = new fuse_rpc::grpc::sync_client(*sync_config);
  }

  return client;
}

static void
rpc_client_destroy([[maybe_unused]] void *private_data)
{
  if (client != nullptr) {
    delete client;
    client = nullptr;
  }
  if (config != nullptr) {
    delete config;
    config = nullptr;
  }
}

static int
rpc_client_getattr(const char *path, struct stat *stbuf)
{
  return client->getattr(path, stbuf);
}

static int
rpc_client_fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
  return client->fgetattr(path, stbuf, fi);
}

static int
rpc_client_access(const char *path, int mask)
{
  return client->access(path, mask);
}

static int
rpc_client_readlink(const char *path, char *buf, size_t size)
{
  return client->readlink(path, buf, size);
}

static int
rpc_client_opendir(const char *path, struct fuse_file_info *fi)
{
  return client->opendir(path, fi);
}

static int
rpc_client_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                   struct fuse_file_info *fi)
{
  return client->readdir(path, buf, filler, offset, fi);
}

static int
rpc_client_releasedir(const char *path, struct fuse_file_info *fi)
{
  return client->releasedir(path, fi);
}

static int
rpc_client_mknod(const char *path, mode_t mode, dev_t rdev)
{
  return client->mknod(path, mode, rdev);
}

static int
rpc_client_mkdir(const char *path, mode_t mode)
{
  return client->mkdir(path, mode);
}

static int
rpc_client_symlink(const char *from, const char *to)
{
  return client->symlink(from, to);
}

static int
rpc_client_unlink(const char *path)
{
  return client->unlink(path);
}

static int
rpc_client_rmdir(const char *path)
{
  return client->rmdir(path);
}

static int
rpc_client_rename(const char *from, const char *to)
{
  return client->rename(from, to);
}

static int
rpc_client_link(const char *from, const char *to)
{
  return client->link(from, to);
}

static int
rpc_client_chmod(const char *path, mode_t mode)
{
  return client->chmod(path, mode);
}

static int
rpc_client_chown(const char *path, uid_t uid, gid_t gid)
{
  return client->chown(path, uid, gid);
}

static int
rpc_client_truncate(const char *path, off_t size)
{
  return client->truncate(path, size);
}

static int
rpc_client_ftruncate(const char *path, off_t size, struct fuse_file_info *fi)
{
  return client->ftruncate(path, size, fi);
}

static int
rpc_client_utimens(const char *path, const struct timespec ts[2])
{
  return client->utimens(path, ts);
}

static int
rpc_client_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  return client->create(path, mode, fi);
}

static int
rpc_client_open(const char *path, struct fuse_file_info *fi)
{
  return client->open(path, fi);
}

static int
rpc_client_read(const char *path, char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi)
{
  return client->read(path, buf, size, offset, fi);
}

static int
rpc_client_write(const char *path, const char *buf, size_t size, off_t offset,
                 struct fuse_file_info *fi)
{
  return client->write(path, buf, size, offset, fi);
}

static int
rpc_client_statfs(const char *path, struct statvfs *stbuf)
{
  return client->statfs(path, stbuf);
}

static int
rpc_client_flush(const char *path, struct fuse_file_info *fi)
{
  return client->flush(path, fi);
}

static int
rpc_client_release(const char *path, struct fuse_file_info *fi)
{
  return client->release(path, fi);
}

static int
rpc_client_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
  return client->fsync(path, isdatasync, fi);
}

static int
rpc_client_fallocate(const char *path, int mode, off_t offset, off_t length,
                     struct fuse_file_info *fi)
{
  return client->fallocate(path, mode, offset, length, fi);
}

static int
rpc_client_setxattr(const char *path, const char *name, const char *value, size_t size,
                    int flags)
{
  return client->setxattr(path, name, value, size, flags);
}

static int
rpc_client_getxattr(const char *path, const char *name, char *value, size_t size)
{
  return client->getxattr(path, name, value, size);
}

static int
rpc_client_listxattr(const char *path, char *list, size_t size)
{
  return client->listxattr(path, list, size);
}

static int
rpc_client_removexattr(const char *path, const char *name)
{
  return client->removexattr(path, name);
}

rpc_client_config::rpc_client_config(YAML::Node data)
{
  logging::debug("configuring RPC client layer...");

  std::string server_address = "";
  std::string mode = "";
  // Async cliente default configurations
  size_t cache_size = 1UL * 1024UL * 1024UL * 1024UL; // 1 GiB
  size_t block_size = 1UL * 1024UL * 1024UL;          // 1 MiB
  size_t threads = 1;                                 // 1 thread
  double flush_threshold = 0.3;                       // 30% cache size

  if (!data["server_address"]) {
    throw rpc_client_wrong_config_exception("requires server address");
  }

  parser_.emplace("server_address", [&]() {
    server_address = data["server_address"].as<std::string>();
  });

  parser_.emplace("mode", [&]() {
    mode = data["mode"].as<std::string>();
  });

  parser_.emplace("cache_size", [&]() {
    cache_size = data["cache_size"].as<size_t>();
  });

  parser_.emplace("block_size", [&]() {
    block_size = data["block_size"].as<size_t>();
  });

  parser_.emplace("threads", [&]() {
    threads = data["threads"].as<size_t>();
  });

  parser_.emplace("flush_threshold", [&]() {
    flush_threshold = data["flush_threshold"].as<double>();
  });

  for (const auto &kv : data) {
    const std::string &option = kv.first.as<std::string>();
    if (parser_.contains(option)) {
      parser_.at(option)();
    } else {
      logging::warn("Ignoring option: \"{}\", rpc client layer doesn't recognises it",
                    option);
    }
  }

  if (mode == "" || mode == "sync") {
    config = new fuse_rpc::grpc::sync_client::config(server_address);
  } else if (mode == "async") {
    config = new fuse_rpc::grpc::async_client::config(
        server_address, cache_size, block_size, threads, flush_threshold);
  } else {
    throw rpc_client_wrong_config_exception("invalid mode");
  }
}

rpc_client_config::~rpc_client_config() {}

void
rpc_client_config::init_layer(fuse_operations &operations)
{
  logging::debug("init RPC client layer...");

  // Update the fuse operations
  operations.init = rpc_client_init;
  operations.destroy = rpc_client_destroy;
  operations.getattr = rpc_client_getattr;
  operations.fgetattr = rpc_client_fgetattr;
  operations.access = rpc_client_access;
  operations.readlink = rpc_client_readlink;
  operations.opendir = rpc_client_opendir;
  operations.readdir = rpc_client_readdir;
  operations.releasedir = rpc_client_releasedir;
  operations.mknod = rpc_client_mknod;
  operations.mkdir = rpc_client_mkdir;
  operations.symlink = rpc_client_symlink;
  operations.unlink = rpc_client_unlink;
  operations.rmdir = rpc_client_rmdir;
  operations.rename = rpc_client_rename;
  operations.link = rpc_client_link;
  operations.chmod = rpc_client_chmod;
  operations.chown = rpc_client_chown;
  operations.truncate = rpc_client_truncate;
  operations.ftruncate = rpc_client_ftruncate;
  operations.utimens = rpc_client_utimens;
  operations.create = rpc_client_create;
  operations.open = rpc_client_open;
  operations.read = rpc_client_read;
  operations.write = rpc_client_write;
  operations.statfs = rpc_client_statfs;
  operations.flush = rpc_client_flush;
  operations.release = rpc_client_release;
  operations.fsync = rpc_client_fsync;
  // operations.fallocate = rpc_client_fallocate;
  // operations.setxattr = rpc_client_setxattr;
  // operations.getxattr = rpc_client_getxattr;
  // operations.listxattr = rpc_client_listxattr;
  // operations.removexattr = rpc_client_removexattr;
}

void
rpc_client_config::clean_layer()
{
  logging::debug("cleaning RPC client layer...");
}

} // namespace rsafefs
