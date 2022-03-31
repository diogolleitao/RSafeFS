#include "rsafefs/client.hpp"
#include <cassert>

namespace rsafefs
{

int
client_main(std::unique_ptr<config> config, std::string &mount_point)
{
  assert(config->is_client());

  struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

  if (config->debug_mode_) {
    // fuse_opt_add_arg(&args, "-d"); // enable debugging output (implies -f)
    fuse_opt_add_arg(&args, "-f"); // keep the application in the foreground
    fuse_opt_add_arg(&args, "-s"); // run single-threaded instead of multi-threaded
  }

  // Add mount point
  fuse_opt_add_arg(&args, mount_point.c_str());

  fuse_main(args.argc, args.argv, &config->operations_, NULL);

  return 0;
}

} // namespace rsafefs