#pragma once

#include <optional>

namespace rsafefs
{

template <typename T> class cache_manager
{
public:
  virtual void touch(const T &t) = 0;

  virtual void remove(const T &t) = 0;

  virtual std::optional<T> evict() = 0;
};

} // namespace rsafefs