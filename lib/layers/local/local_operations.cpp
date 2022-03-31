#include "rsafefs/layers/local/local_operations.hpp"
#include "rsafefs/utils/utils.hpp"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __linux__
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

namespace rsafefs
{

static std::string root_path;

static void *
local_init(fuse_conn_info *conn)
{
  return nullptr;
}

static void
local_destroy(void *private_data)
{
}

static int
local_getattr(const char *path, struct stat *stbuf)
{
  const std::string new_path = root_path + path;
  const int res = lstat(new_path.c_str(), stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
  const int res = fstat(fi->fh, stbuf);

#if FUSE_VERSION >= 29
  // Fall back to global I/O size. See loopback_getattr().
  stbuf->st_blksize = 0;
#endif

  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int
local_flush(const char *path, struct fuse_file_info *fi)
{
  int res;

  res = close(dup(fi->fh));
  if (res == -1) {
    return -errno;
  }

  return 0;
}

static int
local_access(const char *path, int mask)
{
  const std::string new_path = root_path + path;
  const int res = access(new_path.c_str(), mask);
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_readlink(const char *path, char *buf, size_t size)
{
  const std::string new_path = root_path + path;
  const int res = readlink(new_path.c_str(), buf, size - 1);
  if (res == -1)
    return -errno;

  buf[res] = '\0';
  return 0;
}

static int
local_mknod(const char *path, mode_t mode, dev_t rdev)
{
  int res;

  const std::string new_path = root_path + path;

  /* On Linux this could just be 'mknod(path, mode, rdev)' but this
     is more portable */
  if (S_ISREG(mode)) {
    res = open(new_path.c_str(), O_CREAT | O_EXCL | O_WRONLY, mode);
    if (res >= 0)
      res = close(res);
  } else if (S_ISFIFO(mode))
    res = mkfifo(new_path.c_str(), mode);
  else
    res = mknod(new_path.c_str(), mode, rdev);
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_mkdir(const char *path, mode_t mode)
{
  const std::string new_path = root_path + path;
  const int res = mkdir(new_path.c_str(), mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_unlink(const char *path)
{
  const std::string new_path = root_path + path;
  const int res = unlink(new_path.c_str());
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_rmdir(const char *path)
{
  const std::string new_path = root_path + path;
  const int res = rmdir(new_path.c_str());
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_symlink(const char *from, const char *to)
{
  const std::string new_from = root_path + from;
  const std::string new_to = root_path + to;
  const int res = symlink(new_from.c_str(), new_to.c_str());
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_rename(const char *from, const char *to)
{
  const std::string new_from = root_path + from;
  const std::string new_to = root_path + to;
  const int res = rename(new_from.c_str(), new_to.c_str());
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_link(const char *from, const char *to)
{
  const std::string new_from = root_path + from;
  const std::string new_to = root_path + to;
  const int res = link(new_from.c_str(), new_to.c_str());
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_chmod(const char *path, mode_t mode)
{
  const std::string new_path = root_path + path;
  const int res = chmod(new_path.c_str(), mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_chown(const char *path, uid_t uid, gid_t gid)
{
  const std::string new_path = root_path + path;
  const int res = lchown(new_path.c_str(), uid, gid);
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_ftruncate(const char *path, off_t size, struct fuse_file_info *fi)
{
  int res;
  const std::string new_path = root_path + path;

  if (fi != NULL)
    res = ftruncate(fi->fh, size);
  else
    res = truncate(new_path.c_str(), size);
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_truncate(const char *path, off_t size)
{
  const std::string new_path = root_path + path;
  const int res = truncate(new_path.c_str(), size);
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_utime(const char *path, struct utimbuf *buf)
{
  const std::string new_path = root_path + path;
  const int res = utime(new_path.c_str(), buf);
  if (res == -1)
    return -errno;

  return 0;
}

#ifdef HAVE_UTIMENSAT
static int
local_utimens(const char *path, const struct timespec ts[2], struct fuse_file_info *fi)
{
  const std::string new_path = root_path + path;

  /* don't use utime/utimes since they follow symlinks */
  const int res = utimensat(0, new_path.c_str(), ts, AT_SYMLINK_NOFOLLOW);
  if (res == -1)
    return -errno;

  return 0;
}
#endif

static int
local_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  const std::string new_path = root_path + path;
  const int res = open(new_path.c_str(), fi->flags, mode);
  if (res == -1)
    return -errno;

  fi->fh = res;
  return 0;
}

static int
local_open(const char *path, struct fuse_file_info *fi)
{
  const std::string new_path = root_path + path;

  const int res = open(new_path.c_str(), fi->flags);
  if (res == -1)
    return -errno;

  fi->fh = res;
  return 0;
}

static int
local_read(const char *path, char *buf, size_t size, off_t offset,
           struct fuse_file_info *fi)
{
  int fd;
  int res;
  const std::string new_path = root_path + path;

  if (fi == NULL)
    fd = open(new_path.c_str(), O_RDONLY);
  else
    fd = fi->fh;

  if (fd == -1)
    return -errno;

  res = pread(fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  if (fi == NULL)
    close(fd);
  return res;
}

static int
local_write(const char *path, const char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi)
{
  int fd;
  int res;
  const std::string new_path = root_path + path;

  (void)fi;
  if (fi == NULL)
    fd = open(new_path.c_str(), O_WRONLY);
  else
    fd = fi->fh;

  if (fd == -1)
    return -errno;

  res = pwrite(fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  if (fsync(fd) == -1) {
    res = -errno;
  }

  if (fi == NULL)
    close(fd);

  return res;
}

static int
local_statfs(const char *path, struct statvfs *stbuf)
{
  const std::string new_path = root_path + path;
  const int res = statvfs(new_path.c_str(), stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int
local_release(const char *path, struct fuse_file_info *fi)
{
  close(fi->fh);
  return 0;
}

static int
local_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{

  fsync(fi->fh);
  return 0;
}

struct local_dirp {
  DIR *dp;
  struct dirent *entry;
  off_t offset;
};

static int
local_opendir(const char *path, struct fuse_file_info *fi)
{
  int res;

  const std::string new_path = root_path + path;

  struct local_dirp *d = (struct local_dirp *)malloc(sizeof(struct local_dirp));
  if (d == NULL) {
    return -ENOMEM;
  }

  d->dp = opendir(new_path.c_str());
  if (d->dp == NULL) {
    res = -errno;
    free(d);
    return res;
  }

  d->offset = 0;
  d->entry = NULL;

  fi->fh = (unsigned long)d;

  return 0;
}

static inline struct local_dirp *
get_dirp(struct fuse_file_info *fi)
{
  return (struct local_dirp *)(uintptr_t)fi->fh;
}

static int
local_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
              struct fuse_file_info *fi)
{
  struct local_dirp *d = get_dirp(fi);

  (void)path;

  if (offset != d->offset) {
    seekdir(d->dp, offset);
    d->entry = NULL;
    d->offset = offset;
  }

  while (1) {
    struct stat st;
    off_t nextoff;

    if (!d->entry) {
      d->entry = readdir(d->dp);
      if (!d->entry) {
        break;
      }
    }

    memset(&st, 0, sizeof(st));
    st.st_ino = d->entry->d_ino;
    st.st_mode = d->entry->d_type << 12;
    nextoff = telldir(d->dp);
    if (filler(buf, d->entry->d_name, &st, nextoff)) {
      break;
    }

    d->entry = NULL;
    d->offset = nextoff;
  }

  return 0;
}

static int
local_releasedir(const char *path, struct fuse_file_info *fi)
{
  struct local_dirp *d = get_dirp(fi);

  (void)path;

  closedir(d->dp);
  free(d);

  return 0;
}

void
assign_local_operations(fuse_operations &operations, const std::string &path)
{
  root_path = path;

  operations.init = local_init;
  operations.destroy = local_destroy;
  operations.getattr = local_getattr;
  operations.fgetattr = local_fgetattr;
  operations.opendir = local_opendir;
  operations.flush = local_flush;
  operations.ftruncate = local_ftruncate;
  operations.access = local_access;
  operations.readlink = local_readlink;
  operations.releasedir = local_releasedir;
  operations.readdir = local_readdir;
  operations.mknod = local_mknod;
  operations.mkdir = local_mkdir;
  operations.symlink = local_symlink;
  operations.unlink = local_unlink;
  operations.rmdir = local_rmdir;
  operations.rename = local_rename;
  operations.link = local_link;
  operations.chmod = local_chmod;
  operations.chown = local_chown;
  operations.truncate = local_truncate;
  operations.utime = local_utime;
#ifdef HAVE_UTIMENSAT
  operations.utimens = local_utimens;
#endif
  operations.open = local_open;
  operations.create = local_create;
  operations.read = local_read;
  operations.write = local_write;
  operations.statfs = local_statfs;
  operations.release = local_release;
  operations.fsync = local_fsync;
#ifdef HAVE_SETXATTR
  operations.setxattr = local_setxattr;
  operations.getxattr = local_getxattr;
  operations.listxattr = local_listxattr;
  operations.removexattr = local_removexattr;
#endif
}

} // namespace rsafefs