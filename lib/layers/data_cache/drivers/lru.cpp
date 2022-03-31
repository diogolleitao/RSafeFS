#include "rsafefs/layers/data_cache/drivers/lru.hpp"

namespace rsafefs::data_cache
{

lru_eviction::lru_eviction()
    : manager_()
{
}

lru_eviction::~lru_eviction() {}

void
lru_eviction::touch(const key &key)
{
  manager_.touch(key);
}

void
lru_eviction::remove(const key &key)
{
  manager_.remove(key);
}

std::optional<key>
lru_eviction::evict()
{
  return manager_.evict();
}

} // namespace rsafefs::data_cache