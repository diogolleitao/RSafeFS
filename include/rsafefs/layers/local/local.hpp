#pragma once

#include "rsafefs/config.hpp"
#include "rsafefs/fuse_wrapper/fuse31.hpp"

namespace rsafefs
{

class local_wrong_config_exception : public std::runtime_error
{
public:
  local_wrong_config_exception(const std::string &message)
      : std::runtime_error(std::string("local: ").append(message))
  {
  }
};

class local_config final : public layer_config
{
private:
  std::map<std::string, std::function<void()>> parser_;

  enum mode { LOCAL, NFS };
  std::string path_;
  mode mode_;

public:
  local_config(YAML::Node data);
  ~local_config();
  virtual void init_layer(fuse_operations &operations) override;
  virtual void clean_layer() override;
};

} // namespace rsafefs