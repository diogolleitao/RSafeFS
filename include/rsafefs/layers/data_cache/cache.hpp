#pragma once

#include "rsafefs/fuse_wrapper/fuse31.hpp"
#include <absl/hash/hash.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

namespace rsafefs::data_cache
{
using key = std::pair<std::string, size_t>;

class cache
{
public:
  class eviction_policy
  {
  public:
    virtual ~eviction_policy() = default;

  private:
    virtual void touch(const key &key) = 0;
    virtual void remove(const key &key) = 0;
    virtual std::optional<key> evict() = 0;

    friend class data_cache::cache;
  };

  struct config {
    size_t size_;
    size_t block_size_;
    int time_out_;
    std::shared_ptr<eviction_policy> eviction_policy_;
  };

  cache(config &config, const fuse_operations &operations);

  int open(const char *path, struct fuse_file_info *fi);

  int release(const char *path, struct fuse_file_info *fi);

  int read(const char *path, char *buf, size_t size, off_t offset,
           struct fuse_file_info *fi);

private:
  struct block {
    block(std::unique_ptr<char[]> buf, size_t size, off_t offset, struct timespec &mtime);

    [[nodiscard]] bool is_valid(int time_out, struct timespec &current_file_mtime) const;

    std::unique_ptr<char[]> buf_;
    size_t size_;
    const off_t offset_;
    std::chrono::high_resolution_clock::time_point timestamp_;
    struct timespec mtime_;
    std::shared_mutex mtx_;
  };

  void remove_block(key &key);

  const config config_;
  const fuse_operations &operations_;

  std::atomic<size_t> cache_size_;
  std::shared_mutex cache_mtx_;
  std::unordered_map<key, block, absl::Hash<key>> cache_;
  std::mutex mtimes_mtx_;
  std::unordered_map<std::string, struct timespec> mtimes_;
};

} // namespace rsafefs::data_cache