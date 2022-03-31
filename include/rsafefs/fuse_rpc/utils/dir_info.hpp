#pragma once

#include <string>
#include <sys/stat.h>
#include <vector>

namespace rsafefs
{

struct DirInfo {
  struct Entry {
    Entry(std::string name, const struct stat &st, off_t offset);

    const std::string name_;
    const struct stat st_;
    const off_t offset_;
  };

  std::vector<Entry> buf_;
};

int rpc_filler(void *buf, const char *name, const struct stat *stbuf, off_t off);

} // namespace rsafefs
