#pragma once

#include "rsafefs/config.hpp"
#include "rsafefs/fuse_wrapper/fuse31.hpp"
#include "rsafefs/utils/utils.hpp"

namespace rsafefs
{

class rpc_client_wrong_config_exception : public std::runtime_error
{
public:
  rpc_client_wrong_config_exception(const std::string &message)
      : std::runtime_error(std::string("rpc client: ").append(message))
  {
  }
};

class rpc_client_config final : public layer_config
{
private:
  std::map<std::string, std::function<void()>> parser_;

public:
  rpc_client_config(YAML::Node data);
  ~rpc_client_config();
  virtual void init_layer(fuse_operations &operations) override;
  virtual void clean_layer() override;
};

} // namespace rsafefs