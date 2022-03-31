#include "rsafefs/config.hpp"
#include "rsafefs/layers/data_cache/data_cache.hpp"
#include "rsafefs/layers/local/local.hpp"
#include "rsafefs/layers/metadata_cache/metadata_cache.hpp"
#include "rsafefs/layers/read_ahead/read_ahead.hpp"
#include "rsafefs/layers/rpc_client/rpc_client.hpp"
#include "rsafefs/server.hpp"
#include "rsafefs/utils/logging.hpp"
#include "rsafefs/utils/utils.hpp"

namespace rsafefs
{

std::unique_ptr<config>
config::make(std::string &config_file_path, bool debug)
{
  if (debug) {
    activate_debug_mode();
  }
  auto conf = std::unique_ptr<config>(new config(config_file_path));
  conf->is_valid();
  conf->debug_mode_ = debug;
  return conf;
}

void
config::activate_debug_mode()
{
  logging::debug(true);
}

config::config(std::string &config_file_path)
    : config_file_path_(config_file_path)
{
  YAML::Node config = YAML::LoadFile(config_file_path);
  if (!config.IsMap()) {
    throw wrong_config_exception("configuration file is not yaml map");
  }

  memset(&operations_, 0, sizeof(operations_));

  for (const auto &kv : config) {
    const std::string &layer = kv.first.as<std::string>();
    const YAML::Node &layer_config = kv.second;

    if (layer == "data_cache") {
      layers_.push_back(std::make_unique<data_cache_config>(layer_config));
    } else if (layer == "local") {
      layers_.push_back(std::make_unique<local_config>(layer_config));
    } else if (layer == "metadata_cache") {
      layers_.push_back(std::make_unique<metadata_cache_config>(layer_config));
    } else if (layer == "read_ahead") {
      layers_.push_back(std::make_unique<read_ahead_config>(layer_config));
    } else if (layer == "rpc_client") {
      layers_.push_back(std::make_unique<rpc_client_config>(layer_config));
    } else if (layer == "rpc_server") {
      layers_.push_back(std::make_unique<rpc_server_config>(layer_config));
    } else {
      throw wrong_config_exception(fmt::format("layer \"{}\" not implemented", layer));
    }
  }

  // Compose the layers stack
  for (auto &layer : layers_) {
    layer->init_layer(operations_);
  }
}

config::~config()
{
  // Clean the layers stack
  for (auto &layer : layers_) {
    layer->clean_layer();
  }
}

void
config::is_valid()
{
  if (layers_.empty()) {
    throw wrong_config_exception("empty configuration file");
  }

  uint n_servers = 0;
  uint n_clients = 0;

  for (auto &layer : layers_) {
    if (utils::instance_of<rpc_client_config>(layer.get())) {
      n_clients++;
    } else if (utils::instance_of<rpc_server_config>(layer.get())) {
      n_servers++;
    }
  }

  if (n_servers > 1) {
    throw wrong_config_exception("too many rpc_server configurations");
  }

  if (n_clients > 1) {
    throw wrong_config_exception("too many rpc_client configurations");
  }

  if (n_clients == 1 && !utils::instance_of<rpc_client_config>(layers_.front().get())) {
    throw wrong_config_exception("rpc client must be the first layer");
  }

  if (n_servers == 1 && !utils::instance_of<rpc_server_config>(layers_.back().get())) {
    throw wrong_config_exception("rpc server must be the last layer");
  }

  if (n_servers == 1 && n_clients == 1) {
    throw wrong_config_exception(
        "rpc client and rpc server must not be used in the same stack");
  }
}

bool
config::is_client()
{
  // a valid client configuration is a configuration without the rpc server layer
  for (auto &layer : layers_) {
    if (utils::instance_of<rpc_server_config>(layer.get())) {
      return false;
    }
  }

  return true;
}

bool
config::is_server()
{
  // rpc server must be the last layer
  const auto last_layer = layers_.back().get();
  if (!utils::instance_of<rpc_server_config>(last_layer)) {
    return false;
  }

  return true;
}

} // namespace rsafefs
