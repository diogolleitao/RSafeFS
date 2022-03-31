#pragma once

#include "rsafefs/config.hpp"
#include "rsafefs/fuse_wrapper/fuse31.hpp"
#include "rsafefs/utils/utils.hpp"

namespace rsafefs
{

class read_ahead_wrong_config_exception : public std::runtime_error
{
public:
  read_ahead_wrong_config_exception(const std::string &message)
      : std::runtime_error(std::string("read ahead: ").append(message))
  {
  }
};

class read_ahead_config final : public layer_config
{
private:
  std::map<std::string, std::function<void()>> parser_;

public:
  read_ahead_config(YAML::Node data);
  ~read_ahead_config();
  virtual void init_layer(fuse_operations &operations) override;
  virtual void clean_layer() override;
};

} // namespace rsafefs
