#include "CLI/CLI.hpp"
#include "fmt/core.h"
#include "rsafefs/client.hpp"
#include "rsafefs/config.hpp"
#include "rsafefs/server.hpp"

int
main(int argc, char *argv[])
{
  CLI::App app{"RSafeFS: A Modular File System for Remote Storage"};

  std::string configuration_file;
  std::string mount_point;
  bool debug = false;

  app.add_option("-c,--conf", configuration_file)
      ->description("configuration file")
      ->check(CLI::ExistingFile)
      ->required();

  app.add_option("-m,--mount", mount_point)
      ->description("mount point (required only on clients)")
      ->check(CLI::ExistingDirectory);

  app.add_flag("-d,--debug", debug, "Show debug info");

  CLI11_PARSE(app, argc, argv);

  try {
    auto config = rsafefs::config::make(configuration_file, debug);

    if (config->is_client()) {
      rsafefs::client_main(std::move(config), mount_point);
    } else if (config->is_server()) {
      rsafefs::server_main(std::move(config));
    } else {
      fmt::print(stderr, "file \"{}\" is not a valid configuration\n",
                 configuration_file);
    }

  } catch (const std::exception &e) {
    fmt::print(stderr, "{}\n", e.what());
  }

  return 0;
}
