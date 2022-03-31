#include "rsafefs/layers/data_cache/cache.hpp"
#include <algorithm>
#include <cstring>

namespace rsafefs
{

data_cache::cache::cache(config &config, const fuse_operations &operations)
    : config_(config)
    , operations_(operations)
    , cache_size_(0)
{
}

int
data_cache::cache::open(const char *path, struct fuse_file_info *fi)
{
  const int res = operations_.open(path, fi);
  if (res < 0) {
    return res;
  }

  struct stat stbuf {
  };
  const int attr_res = operations_.getattr(path, &stbuf);
  if (attr_res < 0) {
    return attr_res;
  }

  std::unique_lock mtimes_lock(mtimes_mtx_);
#ifdef __APPLE__
  mtimes_[path] = stbuf.st_mtimespec;
#else
  mtimes_[path] = stbuf.st_mtim;
#endif

  return 0;
}

int
data_cache::cache::release(const char *path, struct fuse_file_info *fi)
{
  {
    std::unique_lock mtimes_lock(mtimes_mtx_);
    mtimes_.erase(path);
  }

  return operations_.release(path, fi);
}

int
data_cache::cache::read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi)
{
  size_t readed = 0;
  off_t seeker = offset;
  struct timespec file_mtime {
  };

  std::unique_lock mtimes_lock(mtimes_mtx_);
  const auto mtimes_iterator = mtimes_.find(path);
  if (mtimes_iterator != mtimes_.end()) {
    file_mtime = mtimes_iterator->second;
  } else {
    // Do not use the cache, call read operation of the next layer
    mtimes_lock.unlock();
    return operations_.read(path, buf, size, offset, fi);
  }
  mtimes_lock.unlock();

  while (readed != size) {
    size_t block_id = seeker / config_.block_size_;
    key key = std::make_pair(path, block_id);

    std::shared_lock cache_shared_lock(cache_mtx_);
    const auto cache_iterator = cache_.find(key);
    if (cache_iterator != cache_.end()) {

      block &block = cache_iterator->second;
      std::shared_lock block_shared_lock(block.mtx_);
      if (!block.is_valid(config_.time_out_, file_mtime)) {
        block_shared_lock.unlock();

        // Update block
        std::unique_lock lock_block(block.mtx_);
        const int res = operations_.read(path, block.buf_.get(), config_.block_size_,
                                         block.offset_, fi);
        block.size_ = res;
        block.timestamp_ = std::chrono::high_resolution_clock::now();
        block.mtime_ = file_mtime;
        lock_block.unlock();

        if (res < 0) {
          // Remove block
          cache_shared_lock.unlock();
          remove_block(key);
          config_.eviction_policy_->remove(key);
          return res;
        }

        block_shared_lock.lock();
      }

      off_t offset_in_the_block = seeker - block.offset_;
      size_t n_bytes_to_copy =
          std::min(size - readed, block.size_ - static_cast<size_t>(offset_in_the_block));
      if (n_bytes_to_copy == 0) {
        break;
      }

      std::memcpy(buf + readed, block.buf_.get() + offset_in_the_block, n_bytes_to_copy);
      block_shared_lock.unlock();
      cache_shared_lock.unlock();

      readed += n_bytes_to_copy;
      seeker += n_bytes_to_copy;
    } else {
      cache_shared_lock.unlock();
      if (cache_size_ > config_.size_) {
        auto selected_key = config_.eviction_policy_->evict();
        if (selected_key) {
          remove_block(selected_key.value());
        }
      }

      const off_t block_offset = block_id * config_.block_size_;
      auto new_buf = std::make_unique<char[]>(config_.block_size_);
      const int block_size =
          operations_.read(path, new_buf.get(), config_.block_size_, block_offset, fi);
      if (block_size < 0)
        return block_size;
      if (block_size == 0)
        break;

      std::unique_lock lock(cache_mtx_);
      auto pair = cache_.try_emplace(key, std::move(new_buf), block_size, block_offset,
                                     file_mtime);
      lock.unlock();
      if (pair.second) {
        cache_size_ += config_.block_size_;
      }
    }
    config_.eviction_policy_->touch(key);
  }
  return readed;
}

void
data_cache::cache::remove_block(key &key)
{
  std::unique_lock lock(cache_mtx_);
  const auto cache_iterator = cache_.find(key);
  if (cache_iterator != cache_.end()) {
    cache_size_ -= config_.block_size_;
    cache_.erase(cache_iterator);
  }
}

data_cache::cache::block::block(std::unique_ptr<char[]> buf, size_t size, off_t offset,
                                struct timespec &mtime)
    : buf_(std::move(buf))
    , size_(size)
    , offset_(offset)
    , timestamp_(std::chrono::high_resolution_clock::now())
    , mtime_(mtime)
{
}

bool
data_cache::cache::block::is_valid(int time_out,
                                   struct timespec &current_file_mtime) const
{
  if (mtime_.tv_sec != current_file_mtime.tv_sec ||
      mtime_.tv_nsec != current_file_mtime.tv_nsec) {
    return false;
  }

  if (time_out > 0) {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed_time =
        std::chrono::duration_cast<std::chrono::seconds>(now - timestamp_).count();
    return elapsed_time < time_out;
  }
  return true;
}

} // namespace rsafefs