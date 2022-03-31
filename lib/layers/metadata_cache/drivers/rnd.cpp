#include "rsafefs/layers/metadata_cache/drivers/rnd.hpp"

namespace rsafefs::metadata_cache
{

rnd_eviction::rnd_eviction()
    : manager_()
{
}

rnd_eviction::~rnd_eviction() {}

void
rnd_eviction::touch(const key &key)
{
  manager_.touch(key);
}

void
rnd_eviction::remove(const key &key)
{
  manager_.remove(key);
}

std::optional<key>
rnd_eviction::evict()
{
  return manager_.evict();
}

} // namespace rsafefs::metadata_cache