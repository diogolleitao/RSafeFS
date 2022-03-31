#pragma once

#include "rsafefs/config.hpp"

namespace rsafefs
{

int client_main(std::unique_ptr<config> config, std::string &mount_point);

} // namespace rsafefs