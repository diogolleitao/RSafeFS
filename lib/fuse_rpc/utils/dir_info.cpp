#include "rsafefs/fuse_rpc/utils/dir_info.hpp"

namespace rsafefs
{

DirInfo::Entry::Entry(std::string name, const struct stat &st, const off_t offset)
    : name_(std::move(name))
    , st_(st)
    , offset_(offset)
{
}

int
rpc_filler(void *buf, const char *name, const struct stat *stbuf, off_t off)
{
  auto *di = static_cast<struct DirInfo *>(buf);
  di->buf_.emplace_back(name, *stbuf, off);
  return 0;
}

} // namespace rsafefs