#pragma once

#include "rsafefs/common/cache/rnd_manager.hpp"
#include "rsafefs/layers/metadata_cache/cache.hpp"

namespace rsafefs::metadata_cache
{

class rnd_eviction : public metadata_cache::cache::eviction_policy
{
public:
  rnd_eviction();

  ~rnd_eviction();

  virtual void touch(const key &key) override;

  virtual void remove(const key &key) override;

  virtual std::optional<key> evict() override;

  rnd_cache_manager<key> manager_;
};

} // namespace rsafefs::metadata_cache