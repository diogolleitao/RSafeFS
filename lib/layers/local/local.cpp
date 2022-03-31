#include "rsafefs/layers/local/local.hpp"
#include "rsafefs/layers/local/local_operations.hpp"
#include "rsafefs/layers/local/nfs_operations.hpp"
#include "rsafefs/utils/utils.hpp"
#include <filesystem>

namespace rsafefs
{

local_config::local_config(YAML::Node data)
    : mode_(LOCAL)
{
  logging::debug("configuring local layer...");

  if (!data["path"]) {
    throw local_wrong_config_exception("requires path");
  }

  parser_.emplace("path", [&]() {
    path_ = data["path"].as<std::string>();
    if (!std::filesystem::is_directory(path_)) {
      throw local_wrong_config_exception(
          std::string("path: \"").append(path_).append("\" is not a valid directory"));
    }
  });

  parser_.emplace("mode", [&]() {
    const std::string eviction_policy = data["mode"].as<std::string>();
    if (eviction_policy == "local") {
      mode_ = LOCAL;
    } else if (eviction_policy == "nfs") {
      mode_ = NFS;
    } else {
      throw local_wrong_config_exception("invalid mode");
    }
  });

  for (const auto &kv : data) {
    const std::string &option = kv.first.as<std::string>();
    if (parser_.contains(option)) {
      parser_.at(option)();
    } else {
      logging::warn("Ignoring option: \"{}\", local layer doesn't recognises it", option);
    }
  }
}

local_config::~local_config() {}

void
local_config::init_layer(fuse_operations &operations)
{
  logging::debug("init local layer...");

  switch (mode_) {
  case mode::NFS:
    assign_nfs_operations(operations, path_);
    break;
  default:
    assign_local_operations(operations, path_);
  }
}

void
local_config::clean_layer()
{
  logging::debug("cleaning local layer...");
}

} // namespace rsafefs
