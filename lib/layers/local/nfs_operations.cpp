#include "rsafefs/layers/local/nfs_operations.hpp"

#ifdef __linux__
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <memory>
#include <mutex>
#include <sys/stat.h>
#include <unistd.h>

namespace rsafefs
{

// Most data-modifying operations in the NFS protocol are
// synchronous.  That is, when a data modifying procedure returns
// to the client, the client can assume that the operation has
// completed and any modified data associated with the request is
// now on stable storage.
// ...
// The following data modifying procedures are
// synchronous: WRITE (with stable flag set to FILE_SYNC), CREATE,
// MKDIR, SYMLINK, MKNOD, REMOVE, RMDIR, RENAME, LINK, and COMMIT.
//
// from https://tools.ietf.org/html/rfc1813#page-10

namespace fs = std::filesystem;

static std::string root_path;

static inline int
sync_path(const char *path)
{
  const int fd = open(path, O_RDONLY);
  if (fd == -1) {
    return -errno;
  }

  if (fsync(fd) == -1) {
    const int error = errno;
    close(fd);
    return -error;
  }

  if (close(fd) == -1) {
    return -errno;
  }

  return 0;
}

static void *
nfs_init([[maybe_unused]] fuse_conn_info *conn)
{
  return nullptr;
}

static void
nfs_destroy([[maybe_unused]] void *private_data)
{
}

static int
nfs_getattr(const char *path, struct stat *stbuf)
{
  const std::string new_path = root_path + path;
  return (lstat(new_path.c_str(), stbuf) == 0) ? 0 : -errno;
}

static int
nfs_fgetattr([[maybe_unused]] const char *path, struct stat *stbuf,
             struct fuse_file_info *fi)
{
  return (fstat(fi->fh, stbuf) == 0) ? 0 : -errno;
}

static int
nfs_access(const char *path, int mask)
{
  const std::string new_path = root_path + path;
  return (access(new_path.c_str(), mask) == 0 ? 0 : -errno);
}

static int
nfs_readlink(const char *path, char *buf, size_t size)
{
  const std::string new_path = root_path + path;
  const int res = readlink(new_path.c_str(), buf, size - 1);
  if (res == -1) {
    return -errno;
  }

  buf[res] = '\0';
  return 0;
}

struct dir_handle {
  DIR *dir_ptr;
  struct dirent *entry;
  off_t offset;
};

inline static struct dir_handle *
get_dir_handle(struct fuse_file_info *fi)
{
  return (struct dir_handle *)(uintptr_t)fi->fh;
}

static int
nfs_opendir(const char *path, struct fuse_file_info *fi)
{
  const std::string new_path = root_path + path;
  struct dir_handle *d = (struct dir_handle *)malloc(sizeof(struct dir_handle));
  if (d == nullptr)
    return -ENOMEM;

  d->dir_ptr = opendir(new_path.c_str());
  d->offset = 0;
  d->entry = nullptr;

  if (d->dir_ptr == nullptr) {
    const int error = -errno;
    free(d);
    return error;
  }

  fi->fh = (unsigned long)d;

  return 0;
}

static int
nfs_readdir([[maybe_unused]] const char *path, void *buf, fuse_fill_dir_t filler,
            off_t offset, struct fuse_file_info *fi)
{
  struct dir_handle *d = get_dir_handle(fi);

  if (offset != d->offset) {
    seekdir(d->dir_ptr, offset);
    d->entry = nullptr;
    d->offset = offset;
  }

  while (true) {
    if (!d->entry) {
      d->entry = readdir(d->dir_ptr);
      if (!d->entry)
        break;
    }

    struct stat st {
    };
    memset(&st, 0, sizeof(st));
    st.st_ino = d->entry->d_ino;
    st.st_mode = d->entry->d_type << 12;

    const off_t nextoff = telldir(d->dir_ptr);
    if (filler(buf, d->entry->d_name, &st, nextoff))
      break;

    d->entry = nullptr;
    d->offset = nextoff;
  }

  return 0;
}

static int
nfs_releasedir([[maybe_unused]] const char *path, struct fuse_file_info *fi)
{
  struct dir_handle *d = get_dir_handle(fi);

  if (closedir(d->dir_ptr) != 0) {
    return -errno;
  }

  free(d);

  return 0;
}

static int
nfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
  const std::string new_path = root_path + path;

  const int res = S_ISFIFO(mode) ? mkfifo(new_path.c_str(), mode)
                                 : mknod(new_path.c_str(), mode, rdev);
  if (res == -1) {
    return -errno;
  }

  const int sync_parent_res = sync_path(fs::path(new_path).parent_path().c_str());
  if (sync_parent_res < 0) {
    return sync_parent_res;
  }

  return 0;
}

static int
nfs_mkdir(const char *path, mode_t mode)
{
  const std::string new_path = root_path + path;

  const int res = mkdir(new_path.c_str(), mode);
  if (res == -1) {
    return -errno;
  }

  const int sync_res = sync_path(new_path.c_str());
  if (sync_res < 0) {
    return sync_res;
  }

  const int sync_parent_res = sync_path(fs::path(new_path).parent_path().c_str());
  if (sync_parent_res < 0) {
    return sync_parent_res;
  }

  return 0;
}

static int
nfs_unlink(const char *path)
{
  const std::string new_path = root_path + path;

  const int res = unlink(new_path.c_str());
  if (res == -1) {
    return -errno;
  }

  const int sync_parent_res = sync_path(fs::path(new_path).parent_path().c_str());
  if (sync_parent_res < 0) {
    return sync_parent_res;
  }

  return 0;
}

static int
nfs_rmdir(const char *path)
{
  const std::string new_path = root_path + path;

  const int res = rmdir(new_path.c_str());
  if (res == -1) {
    return -errno;
  }

  const int sync_parent_res = sync_path(fs::path(new_path).parent_path().c_str());
  if (sync_parent_res < 0) {
    return sync_parent_res;
  }

  return 0;
}

static int
nfs_symlink(const char *from, const char *to)
{
  const std::string new_from = root_path + from;
  const std::string new_to = root_path + to;

  const int res = symlink(new_from.c_str(), new_to.c_str());
  if (res == -1) {
    return -errno;
  }

  const int sync_parent_res = sync_path(fs::path(new_to).parent_path().c_str());
  if (sync_parent_res < 0) {
    return sync_parent_res;
  }

  return 0;
}

static int
nfs_rename(const char *from, const char *to)
{
  const std::string new_from = root_path + from;
  const std::string new_to = root_path + to;

  const int res = rename(new_from.c_str(), new_to.c_str());
  if (res == -1) {
    return -errno;
  }

  const int sync_res = sync_path(new_to.c_str());
  if (sync_res < 0) {
    return sync_res;
  }

  const int sync_from_parent_res = sync_path(fs::path(new_from).parent_path().c_str());
  if (sync_from_parent_res < 0) {
    return sync_from_parent_res;
  }

  const int sync_to_parent_res = sync_path(fs::path(new_to).parent_path().c_str());
  if (sync_to_parent_res < 0) {
    return sync_to_parent_res;
  }

  return 0;
}

static int
nfs_link(const char *from, const char *to)
{
  const std::string new_from = root_path + from;
  const std::string new_to = root_path + to;

  const int res = link(new_from.c_str(), new_to.c_str());
  if (res == -1) {
    return -errno;
  }

  const int sync_from = sync_path(new_from.c_str());
  if (sync_from < 0) {
    return sync_from;
  }

  const int sync_to = sync_path(new_to.c_str());
  if (sync_to < 0) {
    return sync_to;
  }

  const int sync_from_dir_res = sync_path(fs::path(new_from).parent_path().c_str());
  if (sync_from_dir_res < 0) {
    return sync_from_dir_res;
  }

  const int sync_to_dir_res = sync_path(fs::path(new_to).parent_path().c_str());
  if (sync_to_dir_res < 0) {
    return sync_to_dir_res;
  }

  return 0;
}

static int
nfs_chmod(const char *path, mode_t mode)
{
  const std::string new_path = root_path + path;
  return (chmod(new_path.c_str(), mode) == 0) ? 0 : -errno;
}

static int
nfs_chown(const char *path, uid_t uid, gid_t gid)
{
  const std::string new_path = root_path + path;
  return (lchown(new_path.c_str(), uid, gid) == 0) ? 0 : -errno;
}

static int
nfs_truncate(const char *path, off_t size)
{
  const std::string new_path = root_path + path;
  return (truncate(new_path.c_str(), size) == 0) ? 0 : -errno;
}

static int
nfs_ftruncate([[maybe_unused]] const char *path, off_t size, struct fuse_file_info *fi)
{
  return (ftruncate(fi->fh, size) == 0) ? 0 : -errno;
}

static int
nfs_utimens(const char *path, const struct timespec ts[2])
{
  const std::string new_path = root_path + path;
  return (utimensat(AT_FDCWD, new_path.c_str(), ts, AT_SYMLINK_NOFOLLOW) == 0) ? 0
                                                                               : -errno;
}

static int
nfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  const std::string new_path = root_path + path;

#ifdef __linux__
  if (fi->flags & O_DIRECT) {
    // If O_DIRECT was specified, enable direct IO for the FUSE file system,
    // but ignore it when accessing the underlying file system. (If it was
    // not ignored, pread and pwrite could fail, as the buffer given by FUSE
    // may not be correctly aligned.)

    fi->flags &= ~O_DIRECT;
    fi->direct_io = 1;
  }
#endif

  const int fd = open(new_path.c_str(), fi->flags, mode);
  if (fd == -1) {
    return -errno;
  }

  if (fsync(fd) == -1) {
    return -errno;
  }

  const int sync_parent_res = sync_path(fs::path(new_path).parent_path().c_str());
  if (sync_parent_res < 0) {
    return sync_parent_res;
  }

  fi->fh = fd;

  return 0;
}

static int
nfs_open(const char *path, struct fuse_file_info *fi)
{
  const std::string new_path = root_path + path;

#ifdef __linux__
  if (fi->flags & O_DIRECT) {
    // If O_DIRECT was specified, enable direct IO for the FUSE file system,
    // but ignore it when accessing the underlying file system. (If it was
    // not ignored, pread and pwrite could fail, as the buffer given by FUSE
    // may not be correctly aligned.)

    fi->flags &= ~O_DIRECT;
    fi->direct_io = 1;
  }
#endif

  const int fd = open(new_path.c_str(), fi->flags);
  if (fd == -1) {
    return -errno;
  }

  fi->fh = fd;

  return 0;
}

static int
nfs_read([[maybe_unused]] const char *path, char *buf, size_t size, off_t offset,
         struct fuse_file_info *fi)
{
  const int res = pread(fi->fh, buf, size, offset);
  if (res == -1) {
    return -errno;
  }

  return res;
}

static int
nfs_write([[maybe_unused]] const char *path, const char *buf, size_t size, off_t offset,
          struct fuse_file_info *fi)
{
  const int res = pwrite(fi->fh, buf, size, offset);
  if (res == -1) {
    return -errno;
  }

  // the fuse_file_info flush flag is used as the nfs stable flag in write
  // requests see https://tools.ietf.org/html/rfc1813#page-49 for more
  // information

  if (fi->flush && fsync(fi->fh) == -1) {
    return -errno;
  }

  return res;
}

static int
nfs_statfs(const char *path, struct statvfs *stbuf)
{
  const std::string new_path = root_path + path;
  return (statvfs(new_path.c_str(), stbuf) == 0) ? 0 : -errno;
}

static int
nfs_flush([[maybe_unused]] const char *path, struct fuse_file_info *fi)
{
  return (fsync(fi->fh) == 0) ? 0 : -errno;
}

static int
nfs_release([[maybe_unused]] const char *path, struct fuse_file_info *fi)
{
  return (close(fi->fh) == 0) ? 0 : -errno;
}

static int
nfs_fsync([[maybe_unused]] const char *path, int isdatasync, struct fuse_file_info *fi)
{
#ifdef __linux__
  const int res = isdatasync ? fdatasync(fi->fh) : fsync(fi->fh);
#else
  const int res = fsync(fi->fh);
#endif

  return (res == 0) ? 0 : -errno;
}

void
assign_nfs_operations(fuse_operations &operations, const std::string &path)
{
  root_path = path;

  operations.init = nfs_init;
  operations.destroy = nfs_destroy;
  operations.getattr = nfs_getattr;
  operations.fgetattr = nfs_fgetattr;
  operations.access = nfs_access;
  operations.readlink = nfs_readlink;
  operations.opendir = nfs_opendir;
  operations.readdir = nfs_readdir;
  operations.releasedir = nfs_releasedir;
  operations.mknod = nfs_mknod;
  operations.mkdir = nfs_mkdir;
  operations.symlink = nfs_symlink;
  operations.unlink = nfs_unlink;
  operations.rmdir = nfs_rmdir;
  operations.rename = nfs_rename;
  operations.link = nfs_link;
  operations.chmod = nfs_chmod;
  operations.chown = nfs_chown;
  operations.truncate = nfs_truncate;
  operations.ftruncate = nfs_ftruncate;
  operations.utimens = nfs_utimens;
  operations.create = nfs_create;
  operations.open = nfs_open;
  operations.read = nfs_read;
  operations.write = nfs_write;
  operations.statfs = nfs_statfs;
  operations.flush = nfs_flush;
  operations.release = nfs_release;
  operations.fsync = nfs_fsync;
}

} // namespace rsafefs