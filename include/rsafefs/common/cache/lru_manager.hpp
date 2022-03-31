#pragma once

#include "rsafefs/common/cache/cache_manager.hpp"
#include <absl/container/flat_hash_map.h>
#include <list>
#include <mutex>

namespace rsafefs
{

template <typename T> class lru_cache_manager : public cache_manager<T>
{
public:
  void touch(const T &t) override
  {
    std::unique_lock lock(mtx_);
    const auto it = map_.find(t);
    if (it != map_.end()) {
      keys_.erase(it->second);
    }
    keys_.push_front(t);
    map_[t] = keys_.begin();
  }

  void remove(const T &t) override
  {
    std::unique_lock lock(mtx_);
    const auto it = map_.find(t);
    if (it != map_.end()) {
      keys_.erase(it->second);
      map_.erase(it);
    }
  }

  std::optional<T> evict() override
  {
    std::unique_lock lock(mtx_);
    if (map_.empty()) {
      return {};
    }
    T t = keys_.back();
    keys_.pop_back();
    map_.erase(t);
    return t;
  }

private:
  std::mutex mtx_;
  std::list<T> keys_;
  absl::flat_hash_map<T, typename std::list<T>::iterator> map_;
};

} // namespace rsafefs
