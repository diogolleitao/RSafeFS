#pragma once

#include "rsafefs/config.hpp"
#include "rsafefs/fuse_wrapper/fuse31.hpp"
#include "rsafefs/utils/utils.hpp"

namespace rsafefs
{

class metadata_cache_wrong_config_exception : public std::runtime_error
{
public:
  metadata_cache_wrong_config_exception(const std::string &message)
      : std::runtime_error(std::string("metadata cache: ").append(message))
  {
  }
};

class metadata_cache_config final : public layer_config
{
private:
  std::map<std::string, std::function<void()>> parser_;

public:
  metadata_cache_config(YAML::Node data);
  ~metadata_cache_config();
  virtual void init_layer(fuse_operations &operations) override;
  virtual void clean_layer() override;
};

} // namespace rsafefs
