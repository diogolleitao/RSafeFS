#pragma once

#include "rsafefs/fuse_wrapper/fuse31.hpp"
#include "rsafefs/utils/logging.hpp"
#include <string>

namespace rsafefs::utils
{

template <typename Base, typename T>
inline bool
instance_of(const T *ptr)
{
  return dynamic_cast<const Base *>(ptr) != nullptr;
}

class stack_operation_exception : public std::runtime_error
{
public:
  stack_operation_exception(const std::string &message)
      : std::runtime_error(message)
  {
  }
};

template <typename T>
inline void
stack_operation_(T &top_opr, T *bottom_opr, std::string file, std::string func, int line)
{
  if (bottom_opr == nullptr) {
    throw stack_operation_exception(fmt::format(
        "bottom operation is a null pointer. file: {}({}) `{}`", file, line, func));
  }

  bottom_opr = top_opr;
};

#define stack_operation(top, bottom)                                                     \
  stack_operation_(top, bottom, __FILE__, __PRETTY_FUNCTION__, __LINE__)

} // namespace rsafefs::utils
