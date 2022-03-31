#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace rsafefs::fuse_rpc::grpc
{

template <typename T> class channel
{
public:
  channel() = default;

  T receive()
  {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] {
      return !queue_.empty();
    });
    T item = queue_.front();
    queue_.pop();
    return item;
  }

  void send(const T &item)
  {
    std::unique_lock lock(mutex_);
    queue_.push(item);
    lock.unlock();
    cv_.notify_one();
  }

  void send(T &&item)
  {
    std::unique_lock lock(mutex_);
    queue_.push(std::move(item));
    lock.unlock();
    cv_.notify_one();
  }

  size_t n_pending_messages()
  {
    std::unique_lock lock(mutex_);
    return queue_.size();
  }

  bool has_pending_messages()
  {
    std::unique_lock lock(mutex_);
    return !queue_.empty();
  }

private:
  std::queue<T> queue_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

} // namespace rsafefs::fuse_rpc::grpc