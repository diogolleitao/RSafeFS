#pragma once

#include "rsafefs/fuse_wrapper/fuse31.hpp"
#include <string>
#include <thread>

namespace rsafefs::fuse_rpc
{

class client
{
public:
  struct config {
    virtual ~config() = default;
  };

  virtual ~client() = default;

  virtual int getattr(const char *path, struct stat *stbuf) = 0;

  virtual int fgetattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi) = 0;

  virtual int access(const char *path, int mask) = 0;

  virtual int readlink(const char *path, char *buf, size_t size) = 0;

  virtual int opendir(const char *path, struct fuse_file_info *fi) = 0;

  virtual int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                      struct fuse_file_info *fi) = 0;

  virtual int releasedir(const char *path, struct fuse_file_info *fi) = 0;

  virtual int mknod(const char *path, mode_t mode, dev_t rdev) = 0;

  virtual int mkdir(const char *path, mode_t mode) = 0;

  virtual int symlink(const char *from, const char *to) = 0;

  virtual int unlink(const char *path) = 0;

  virtual int rmdir(const char *path) = 0;

  virtual int rename(const char *from, const char *to) = 0;

  virtual int link(const char *from, const char *to) = 0;

  virtual int chmod(const char *path, mode_t mode) = 0;

  virtual int chown(const char *path, uid_t uid, gid_t gid) = 0;

  virtual int truncate(const char *path, off_t size) = 0;

  virtual int ftruncate(const char *path, off_t size, struct fuse_file_info *fi) = 0;

  virtual int utimens(const char *path, const struct timespec ts[2]) = 0;

  virtual int create(const char *path, mode_t mode, struct fuse_file_info *fi) = 0;

  virtual int open(const char *path, struct fuse_file_info *fi) = 0;

  virtual int read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi) = 0;

  virtual int write(const char *path, const char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) = 0;

  virtual int statfs(const char *path, struct statvfs *stbuf) = 0;

  virtual int flush(const char *path, struct fuse_file_info *fi) = 0;

  virtual int release(const char *path, struct fuse_file_info *fi) = 0;

  virtual int fsync(const char *path, int isdatasync, struct fuse_file_info *fi) = 0;

  virtual int fallocate(const char *path, int mode, off_t offset, off_t length,
                        struct fuse_file_info *fi) = 0;

  virtual int setxattr(const char *path, const char *name, const char *value, size_t size,
                       int flags) = 0;

  virtual int getxattr(const char *path, const char *name, char *value, size_t size) = 0;

  virtual int listxattr(const char *path, char *list, size_t size) = 0;

  virtual int removexattr(const char *path, const char *name) = 0;
};

} // namespace rsafefs::fuse_rpc
