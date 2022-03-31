#include "rsafefs/layers/metadata_cache/cache.hpp"

namespace rsafefs
{

metadata_cache::cache::cache(config &config)
    : config_(config)
{
}

void
metadata_cache::cache::put(const std::string &path, struct stat *stbuf)
{
  std::unique_lock lock(mtx_);

  const auto cache_size = cache_.size();
  if (cache_size >= config_.size_) {
    std::optional<std::string> selected_path = config_.eviction_policy_->evict();
    if (selected_path) {
      cache_.erase(selected_path.value());
    }
  }

  cache_.emplace(path, stbuf);
  config_.eviction_policy_->touch(path);
}

bool
metadata_cache::cache::get(const std::string &path, struct stat *stbuf)
{
  std::shared_lock shared_lock(mtx_);

  const auto cache_iterator = cache_.find(path);
  if (cache_iterator == cache_.end()) {
    return false;
  }

  metadata metadata = cache_.at(path);
  if (!metadata.is_valid(config_.time_out_)) {
    shared_lock.unlock();

    std::unique_lock lock(mtx_);
    cache_.erase(path);
    config_.eviction_policy_->remove(path);
    return false;
  }

  *stbuf = metadata.stbuf_;
  config_.eviction_policy_->touch(path);
  return true;
}

void
metadata_cache::cache::remove(const std::string &path)
{
  std::unique_lock lock(mtx_);
  cache_.erase(path);
  config_.eviction_policy_->remove(path);
}

metadata_cache::cache::metadata::metadata(struct stat *stbuf)
    : stbuf_(*stbuf)
    , timestamp_(std::chrono::high_resolution_clock::now())
{
}

bool
metadata_cache::cache::metadata::is_valid(int time_out) const
{
  if (time_out > 0) {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed_time =
        std::chrono::duration_cast<std::chrono::seconds>(now - timestamp_).count();
    return elapsed_time < time_out;
  }
  return true;
}

} // namespace rsafefs