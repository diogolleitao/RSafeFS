#pragma once

#include "rsafefs/common/cache/cache_manager.hpp"
#include <absl/container/flat_hash_set.h>
#include <mutex>
#include <random>

namespace rsafefs
{

template <typename T> class rnd_cache_manager : public cache_manager<T>
{
public:
  rnd_cache_manager()
      : rand_gen_(std::random_device{}())
  {
  }

  void touch(const T &t) override
  {
    std::unique_lock lock(mtx_);
    keys_.insert(t);
  }

  void remove(const T &t) override
  {
    std::unique_lock lock(mtx_);
    keys_.erase(t);
  }

  std::optional<T> evict() override
  {
    std::unique_lock lock(mtx_);
    if (keys_.empty()) {
      return {};
    }
    std::uniform_int_distribution<int> dist(0, keys_.size() - 1);
    auto it = keys_.begin();
    std::advance(it, dist(rand_gen_));
    T t = *it;
    keys_.erase(it);
    return t;
  }

private:
  std::mutex mtx_;
  std::mt19937 rand_gen_;
  absl::flat_hash_set<T> keys_;
};

} // namespace rsafefs
