#pragma once

#include "rsafefs/config.hpp"
#include "rsafefs/fuse_wrapper/fuse31.hpp"

namespace rsafefs
{

class server_wrong_config_exception : public std::runtime_error
{
public:
  server_wrong_config_exception(const std::string &message)
      : std::runtime_error(std::string("server: ").append(message))
  {
  }
};

class rpc_server_config final : public layer_config
{
private:
  std::map<std::string, std::function<void()>> parser_;

public:
  rpc_server_config(YAML::Node data);
  ~rpc_server_config();

  virtual void init_layer(fuse_operations &operations);
  virtual void clean_layer();

  friend int server_main(std::unique_ptr<config> config);
};

int server_main(std::unique_ptr<config> config);

} // namespace rsafefs
