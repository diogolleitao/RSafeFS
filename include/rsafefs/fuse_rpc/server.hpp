#pragma once

#include <string>
#include <thread>

namespace rsafefs::fuse_rpc
{

class server
{
public:
  struct config {
    virtual ~config() = default;
  };

  virtual ~server() = default;

  virtual void run() = 0;
};

} // namespace rsafefs::fuse_rpc
