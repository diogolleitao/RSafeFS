#include "rsafefs/layers/read_ahead/read_ahead_cache.hpp"
#include <algorithm>
#include <chrono>

namespace rsafefs
{

read_ahead::cache::cache(config &config, fuse_operations &operations)
    : config_(config)
    , operations_(operations)
{
}

int
read_ahead::cache::open(const char *path, struct fuse_file_info *fi)
{
  std::shared_lock shared_lock(mtx_);
  if (buffers_.find(path) == buffers_.end()) {
    shared_lock.unlock();
    std::unique_lock lock(mtx_);
    buffers_.emplace(std::piecewise_construct, std::forward_as_tuple(path),
                     std::forward_as_tuple());
  } else {
    shared_lock.unlock();
  }
  return operations_.open(path, fi);
}

int
read_ahead::cache::read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi)
{
  int n_of_bytes_read = 0;

  std::shared_lock shared_lock_cache(mtx_);
  buffer &buffer = buffers_.find(path)->second;

  n_of_bytes_read = buffer.read(buf, size, offset);
  if (n_of_bytes_read < 0) {
    const size_t buf_size = std::max(size, config_.size_);
    auto new_buf = std::make_unique<char[]>(buf_size);
    const int res = operations_.read(path, new_buf.get(), buf_size, offset, fi);
    if (res < 0) {
      return res;
    }
    n_of_bytes_read = std::min(static_cast<size_t>(res), size);
    std::memcpy(buf, new_buf.get(), n_of_bytes_read);

    buffer.update(res, offset, std::move(new_buf));
  }

  return n_of_bytes_read;
}

int
read_ahead::cache::release(const char *path, struct fuse_file_info *fi)
{
  std::unique_lock lock(mtx_);
  buffers_.erase(path);
  lock.unlock();
  return operations_.release(path, fi);
}

read_ahead::cache::buffer::buffer()
    : size_(0)
    , offset_(-1)
{
}

void
read_ahead::cache::buffer::update(size_t size, off_t offset, std::unique_ptr<char[]> buf)
{
  std::unique_lock lock(mtx_);
  size_ = size;
  offset_ = offset;
  buf_ = std::move(buf);
}

int
read_ahead::cache::buffer::read(char *buf, size_t size, off_t offset)
{
  std::shared_lock shared_lock(mtx_);
  if (offset_ != -1 && offset >= offset_ && offset + size <= offset_ + size_) {
    off_t offset_in_the_buffer = offset - offset_;
    std::memcpy(buf, buf_.get() + offset_in_the_buffer, size);
    return size;
  } else {
    return -1;
  }
}

} // namespace rsafefs
