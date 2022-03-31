#pragma once

#include "rsafefs/fuse_wrapper/fuse31.hpp"
#include <cstdio>
#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace rsafefs::read_ahead
{

class cache
{
public:
  struct config {
    size_t size_;
  };

  cache(config &config, fuse_operations &operations);

  int open(const char *path, struct fuse_file_info *fi);

  int read(const char *path, char *buf, size_t size, off_t offset,
           struct fuse_file_info *fi);

  int release(const char *path, struct fuse_file_info *fi);

private:
  struct buffer {
    buffer();

    int read(char *buf, size_t size, off_t offset);

    void update(size_t size, off_t offset, std::unique_ptr<char[]> buf);

    size_t size_;
    off_t offset_;
    std::unique_ptr<char[]> buf_;
    std::shared_mutex mtx_;
  };

  const config config_;
  const fuse_operations &operations_;

  std::unordered_map<std::string, buffer> buffers_;
  std::shared_mutex mtx_;
};

} // namespace rsafefs::read_ahead