#pragma once

#include "rsafefs/fuse_wrapper/fuse31.hpp"
#include "yaml-cpp/yaml.h"
#include <string>
#include <vector>

namespace rsafefs
{

class wrong_config_exception : public std::runtime_error
{
public:
  wrong_config_exception(const std::string &message)
      : std::runtime_error(std::string("config file: ").append(message))
  {
  }
};

class layer_config
{
public:
  virtual ~layer_config() = default;
  virtual void init_layer(fuse_operations &operations) = 0;
  virtual void clean_layer() = 0;
};

class config
{
private:
  explicit config(std::string &config_file);
  static void activate_debug_mode();
  void is_valid();
  std::vector<std::unique_ptr<layer_config>> layers_;
  std::string config_file_path_;
  fuse_operations operations_;
  bool debug_mode_;

public:
  static std::unique_ptr<config> make(std::string &config_file, bool debug = false);
  config() = delete;
  ~config();
  bool is_client();
  bool is_server();

  friend int client_main(std::unique_ptr<config> config, std::string &mount_point);
  friend int server_main(std::unique_ptr<config> config);
};

} // namespace rsafefs
