#pragma once

#include "spdlog/spdlog.h"
#include <string>

namespace rsafefs::logging
{

void debug(bool debug);

template <typename... Args>
void
info(Args... args)
{
  spdlog::info(args...);
}

template <typename... Args>
void
warn(Args... args)
{
  spdlog::warn(args...);
}

template <typename... Args>
void
error(Args... args)
{
  spdlog::error(args...);
}

template <typename... Args>
void
debug(Args... args)
{
  spdlog::debug(args...);
}

template <typename... Args>
void
critical(Args... args)
{
  spdlog::critical(args...);
}

} // namespace rsafefs::logging