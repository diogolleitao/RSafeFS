#pragma once

#include "fuse_operations.pb.h"
#include "rsafefs/fuse_rpc/utils/dir_info.hpp"
#include "rsafefs/fuse_wrapper/fuse31.hpp"

namespace rsafefs::fuse_rpc::grpc
{

inline void
fill_StructFuseFileInfo(fuse_grpc_proto::StructFuseFileInfo *sfi,
                        const struct fuse_file_info *fi)
{
  sfi->set_flags(fi->flags);
  sfi->set_fh_old(fi->fh_old);
  sfi->set_writepage(fi->writepage);
  sfi->set_direct_io(fi->direct_io);
  sfi->set_keep_cache(fi->keep_cache);
  sfi->set_flush(fi->flush);
  sfi->set_nonseekable(fi->nonseekable);
  sfi->set_flock_release(fi->flock_release);
  sfi->set_padding(fi->padding);
  sfi->set_fh(fi->fh);
  sfi->set_lock_owner(fi->lock_owner);
}

inline void
fill_fuse_file_info(struct fuse_file_info *fi,
                    const fuse_grpc_proto::StructFuseFileInfo &sfi)
{
  fi->flags = sfi.flags();
  fi->fh_old = sfi.fh_old();
  fi->writepage = sfi.writepage();
  fi->direct_io = sfi.direct_io();
  fi->keep_cache = sfi.keep_cache();
  fi->flush = sfi.flush();
  fi->nonseekable = sfi.nonseekable();
  fi->flock_release = sfi.flock_release();
  fi->padding = sfi.padding();
  fi->fh = sfi.fh();
  fi->lock_owner = sfi.lock_owner();
}

inline void
fill_StructStat(fuse_grpc_proto::StructStat *buf, const struct stat &stbuf)
{
  buf->set_dev(stbuf.st_dev);
  buf->set_ino(stbuf.st_ino);
  buf->set_mode(stbuf.st_mode);
  buf->set_nlink(stbuf.st_nlink);
  buf->set_uid(stbuf.st_uid);
  buf->set_gid(stbuf.st_gid);
  buf->set_rdev(stbuf.st_rdev);
  buf->set_size(stbuf.st_size);
  buf->set_blksize(stbuf.st_blksize);
  buf->set_blocks(stbuf.st_blocks);
#ifdef __APPLE__
  buf->mutable_atim()->set_sec(stbuf.st_atimespec.tv_sec);
  buf->mutable_atim()->set_nsec(stbuf.st_atimespec.tv_nsec);
  buf->mutable_mtim()->set_sec(stbuf.st_mtimespec.tv_sec);
  buf->mutable_mtim()->set_nsec(stbuf.st_mtimespec.tv_nsec);
  buf->mutable_ctim()->set_sec(stbuf.st_ctimespec.tv_sec);
  buf->mutable_ctim()->set_nsec(stbuf.st_ctimespec.tv_nsec);
#else
  buf->mutable_atim()->set_sec(stbuf.st_atim.tv_sec);
  buf->mutable_atim()->set_nsec(stbuf.st_atim.tv_nsec);
  buf->mutable_mtim()->set_sec(stbuf.st_mtim.tv_sec);
  buf->mutable_mtim()->set_nsec(stbuf.st_mtim.tv_nsec);
  buf->mutable_ctim()->set_sec(stbuf.st_ctim.tv_sec);
  buf->mutable_ctim()->set_nsec(stbuf.st_ctim.tv_nsec);
#endif
}

inline void
fill_struct_stat(struct stat *stbuf, const fuse_grpc_proto::StructStat &buf)
{
  stbuf->st_dev = buf.dev();
  stbuf->st_ino = buf.ino();
  stbuf->st_mode = buf.mode();
  stbuf->st_nlink = buf.nlink();
  stbuf->st_uid = buf.uid();
  stbuf->st_gid = buf.gid();
  stbuf->st_rdev = buf.rdev();
  stbuf->st_size = buf.size();
  stbuf->st_blksize = buf.blksize();
  stbuf->st_blocks = buf.blocks();
#ifdef __APPLE__
  const auto atim = buf.atim();
  stbuf->st_atimespec.tv_sec = atim.sec();
  stbuf->st_atimespec.tv_nsec = atim.nsec();
  const auto mtim = buf.mtim();
  stbuf->st_mtimespec.tv_sec = mtim.sec();
  stbuf->st_mtimespec.tv_nsec = mtim.nsec();
  const auto ctim = buf.ctim();
  stbuf->st_ctimespec.tv_sec = ctim.sec();
  stbuf->st_ctimespec.tv_nsec = ctim.nsec();
#else
  const auto atim = buf.atim();
  stbuf->st_atim.tv_sec = atim.sec();
  stbuf->st_atim.tv_nsec = atim.nsec();
  const auto mtim = buf.mtim();
  stbuf->st_mtim.tv_sec = mtim.sec();
  stbuf->st_mtim.tv_nsec = mtim.nsec();
  const auto ctim = buf.ctim();
  stbuf->st_ctim.tv_sec = ctim.sec();
  stbuf->st_ctim.tv_nsec = ctim.nsec();
#endif
}

inline void
fill_StructStatvfs(fuse_grpc_proto::StructStatvfs *buf, const struct statvfs &stbuf)
{
  buf->set_bsize(stbuf.f_bsize);
  buf->set_frsize(stbuf.f_frsize);
  buf->set_blocks(stbuf.f_blocks);
  buf->set_bfree(stbuf.f_bfree);
  buf->set_bavail(stbuf.f_bavail);
  buf->set_files(stbuf.f_files);
  buf->set_ffree(stbuf.f_ffree);
  buf->set_favail(stbuf.f_favail);
  buf->set_fsid(stbuf.f_fsid);
  buf->set_flag(stbuf.f_flag);
  buf->set_namemax(stbuf.f_namemax);
}

inline void
fill_struct_statvfs(struct statvfs *stbuf, const fuse_grpc_proto::StructStatvfs &buf)
{
  stbuf->f_bsize = buf.bsize();
  stbuf->f_frsize = buf.frsize();
  stbuf->f_blocks = buf.blocks();
  stbuf->f_bfree = buf.bfree();
  stbuf->f_bavail = buf.bavail();
  stbuf->f_files = buf.files();
  stbuf->f_ffree = buf.ffree();
  stbuf->f_favail = buf.favail();
  stbuf->f_fsid = buf.fsid();
  stbuf->f_flag = buf.flag();
  stbuf->f_namemax = buf.namemax();
}

inline void
fill_StructTimespec(fuse_grpc_proto::StructTimespec *sts, const struct timespec ts)
{
  sts->set_sec(ts.tv_sec);
  sts->set_nsec(ts.tv_nsec);
}

inline void
fill_struct_timespec(struct timespec &ts, const fuse_grpc_proto::StructTimespec &sts)
{
  ts.tv_sec = sts.sec();
  ts.tv_nsec = sts.nsec();
}

inline void
fill_StructDirEntryInfo(fuse_grpc_proto::StructDirInfoEntry *sdie, DirInfo::Entry entry)
{
  sdie->set_name(entry.name_);
  fill_StructStat(sdie->mutable_stbuf(), entry.st_);
  sdie->set_offset(entry.offset_);
}

} // namespace rsafefs::fuse_rpc::grpc
