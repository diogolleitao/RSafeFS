#pragma once

#include "fuse_operations.grpc.pb.h"
#include "rsafefs/fuse_rpc/client.hpp"
#include <grpcpp/channel.h>

namespace rsafefs::fuse_rpc::grpc
{

using ClientContext = ::grpc::ClientContext;
using Status = ::grpc::Status;

class sync_client : public fuse_rpc::client
{
public:
  struct config : fuse_rpc::client::config {
    explicit config(const std::string &server_address)
        : server_address_(server_address)
    {
    }

    std::string server_address_;
  };

  explicit sync_client(sync_client::config &config);

  static std::shared_ptr<::grpc::Channel> create_channel(std::string &server_address);

  int getattr(const char *path, struct stat *stbuf) override;

  int fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) override;

  int access(const char *path, int mask) override;

  int readlink(const char *path, char *buf, size_t size) override;

  int opendir(const char *path, struct fuse_file_info *fi) override;

  int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
              struct fuse_file_info *fi) override;

  int releasedir(const char *path, struct fuse_file_info *fi) override;

  int mknod(const char *path, mode_t mode, dev_t rdev) override;

  int mkdir(const char *path, mode_t mode) override;

  int symlink(const char *from, const char *to) override;

  int unlink(const char *path) override;

  int rmdir(const char *path) override;

  int rename(const char *from, const char *to) override;

  int link(const char *from, const char *to) override;

  int chmod(const char *path, mode_t mode) override;

  int chown(const char *path, uid_t uid, gid_t gid) override;

  int truncate(const char *path, off_t size) override;

  int ftruncate(const char *path, off_t size, struct fuse_file_info *fi) override;

  int utimens(const char *path, const struct timespec ts[2]) override;

  int create(const char *path, mode_t mode, struct fuse_file_info *fi) override;

  int open(const char *path, struct fuse_file_info *fi) override;

  int read(const char *path, char *buf, size_t size, off_t offset,
           struct fuse_file_info *fi) override;

  int write(const char *path, const char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi) override;

  int statfs(const char *path, struct statvfs *stbuf) override;

  int flush(const char *path, struct fuse_file_info *fi) override;

  int release(const char *path, struct fuse_file_info *fi) override;

  int fsync(const char *path, int isdatasync, struct fuse_file_info *fi) override;

  int fallocate(const char *path, int mode, off_t offset, off_t length,
                struct fuse_file_info *fi) override;

  int setxattr(const char *path, const char *name, const char *value, size_t size,
               int flags) override;

  int getxattr(const char *path, const char *name, char *value, size_t size) override;

  int listxattr(const char *path, char *list, size_t size) override;

  int removexattr(const char *path, const char *name) override;

protected:
  struct read_stream {
    explicit read_stream(const std::unique_ptr<fuse_grpc_proto::FuseOps::Stub> &stub);
    ClientContext client_context_;
    std::unique_ptr<::grpc::ClientReaderWriter<fuse_grpc_proto::ReadRequest,
                                               fuse_grpc_proto::ReadReply>>
        stream_;
    std::mutex mtx_write_;
    std::mutex mtx_read_;
  };

  struct write_stream {
    explicit write_stream(const std::unique_ptr<fuse_grpc_proto::FuseOps::Stub> &stub);
    ClientContext client_context_;
    std::unique_ptr<::grpc::ClientReaderWriter<fuse_grpc_proto::WriteRequest,
                                               fuse_grpc_proto::WriteReply>>
        stream_;
    std::mutex mtx_write_;
    std::mutex mtx_read_;
  };

  void create_streams(const std::string &path, int flags);

  void remove_streams(const std::string &path);

  const grpc::sync_client::config config_;

  const std::unique_ptr<fuse_grpc_proto::FuseOps::Stub> stub_;
  std::mutex mtx_read_streams_;
  std::unordered_map<std::string, read_stream> read_streams_;
  std::mutex mtx_write_streams_;
  std::unordered_map<std::string, write_stream> write_streams_;
};

} // namespace rsafefs::fuse_rpc::grpc