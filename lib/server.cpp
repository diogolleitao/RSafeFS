#include "rsafefs/server.hpp"
#include "rsafefs/fuse_rpc/grpc/server.hpp"
#include "rsafefs/utils/logging.hpp"
#include <cassert>

namespace rsafefs
{

static fuse_rpc::server::config *server_config;

rpc_server_config::rpc_server_config(YAML::Node data)
{
  logging::debug("configuring RPC server...");

  // Default configuration
  size_t n_queues = 1;
  size_t n_pollers_per_queue = 16;
  std::string server_address = "";

  if (!data["server_address"]) {
    throw server_wrong_config_exception("rpc_server: requires server address");
  }

  parser_.emplace("server_address", [&]() {
    server_address = data["server_address"].as<std::string>();
  });

  parser_.emplace("n_queues", [&]() {
    n_queues = data["n_queues"].as<size_t>();
  });

  parser_.emplace("n_pollers_per_queue", [&]() {
    n_pollers_per_queue = data["n_pollers_per_queue"].as<size_t>();
  });

  for (const auto &kv : data) {
    const std::string &option = kv.first.as<std::string>();
    if (parser_.contains(option)) {
      parser_.at(option)();
    } else {
      logging::warn("Ignoring option: \"{}\", server doesn't recognises it", option);
    }
  }

  server_config =
      new fuse_rpc::grpc::server::config(server_address, n_queues, n_pollers_per_queue);
}

rpc_server_config::~rpc_server_config()
{
  if (server_config != nullptr) {
    delete server_config;
    server_config = nullptr;
  }
}

void
rpc_server_config::init_layer(fuse_operations &operations)
{
  logging::debug("init RPC server...");
}

void
rpc_server_config::clean_layer()
{
  logging::debug("cleaning RPC server...");
}

int
server_main(std::unique_ptr<config> config)
{
  assert(config->is_server());

  auto s_config = dynamic_cast<fuse_rpc::grpc::server::config *>(server_config);

  fuse_rpc::grpc::server server(*s_config, config->operations_);
  server.run();

  return 0;
}

} // namespace rsafefs