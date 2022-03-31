#pragma once

#include "rsafefs/fuse_wrapper/fuse31.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <optional>
#include <random>
#include <shared_mutex>
#include <sys/stat.h>
#include <thread>
#include <unordered_map>

namespace rsafefs::metadata_cache
{
using key = std::string;

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

    friend class metadata_cache::cache;
  };

  struct config {
    size_t size_;
    int time_out_;
    std::shared_ptr<eviction_policy> eviction_policy_;
  };

  cache(config &config);

  void put(const std::string &path, struct stat *stbuf);

  bool get(const std::string &path, struct stat *stbuf);

  void remove(const std::string &path);

private:
  struct metadata {
    metadata(struct stat *stbuf);

    [[nodiscard]] bool is_valid(int time_out) const;

    struct stat stbuf_;
    std::chrono::high_resolution_clock::time_point timestamp_;
  };

  const config config_;

  std::unordered_map<key, metadata> cache_;
  std::shared_mutex mtx_;
};

} // namespace rsafefs::metadata_cache