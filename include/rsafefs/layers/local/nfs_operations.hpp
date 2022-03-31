#pragma once

#include "rsafefs/fuse_wrapper/fuse31.hpp"
#include <string>

namespace rsafefs
{

void assign_nfs_operations(fuse_operations &operations, const std::string &path);

}