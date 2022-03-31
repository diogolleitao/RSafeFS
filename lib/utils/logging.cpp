#include "rsafefs/utils/logging.hpp"

namespace rsafefs::logging
{

void
debug(bool debug)
{
  if (debug) {
    spdlog::set_level(spdlog::level::debug);
  }
}

} // namespace rsafefs::logging
