#pragma once

#include "rsafefs/common/cache/lru_manager.hpp"
#include "rsafefs/layers/data_cache/cache.hpp"

namespace rsafefs::data_cache
{

class lru_eviction : public data_cache::cache::eviction_policy
{
public:
  lru_eviction();

  ~lru_eviction();

  virtual void touch(const key &key) override;

  virtual void remove(const key &key) override;

  virtual std::optional<key> evict() override;

  lru_cache_manager<key> manager_;
};

} // namespace rsafefs::data_cache