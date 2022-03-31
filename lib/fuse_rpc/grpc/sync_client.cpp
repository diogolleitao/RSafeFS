#include "rsafefs/fuse_rpc/grpc/sync_client.hpp"
#include "rsafefs/fuse_rpc/grpc/structs_fillers.hpp"
#include "rsafefs/utils/logging.hpp"
#include <grpcpp/grpcpp.h>

namespace rsafefs::fuse_rpc::grpc
{

sync_client::sync_client(sync_client::config &config)
    : config_(config)
    , stub_(fuse_grpc_proto::FuseOps::NewStub(create_channel(config.server_address_)))
{
}

std::shared_ptr<::grpc::Channel>
sync_client::create_channel(std::string &server_address)
{
  ::grpc::ChannelArguments ca;
  // ca.SetMaxReceiveMessageSize(-1);
  // ca.SetMaxSendMessageSize(-1);
  return ::grpc::CreateCustomChannel(server_address, ::grpc::InsecureChannelCredentials(),
                                     ca);
}

int
sync_client::getattr(const char *path, struct stat *stbuf)
{
  fuse_grpc_proto::GetattrRequest request;
  fuse_grpc_proto::GetattrReply reply;
  ClientContext context;

  request.set_path(path);

  const Status status = stub_->Getattr(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[getattr] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  fill_struct_stat(stbuf, reply.stbuf());

  return reply.result();
}

int
sync_client::fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
  fuse_grpc_proto::FgetattrRequest request;
  fuse_grpc_proto::FgetattrReply reply;
  ClientContext context;

  request.set_path(path);
  fill_StructFuseFileInfo(request.mutable_info(), fi);

  const Status status = stub_->Fgetattr(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[fgetattr] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  fill_struct_stat(stbuf, reply.stbuf());
  fill_fuse_file_info(fi, reply.info());

  return reply.result();
}

int
sync_client::access(const char *path, int mask)
{
  fuse_grpc_proto::AccessRequest request;
  fuse_grpc_proto::AccessReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_mask(mask);

  const Status status = stub_->Access(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[access] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  return reply.result();
}

int
sync_client::readlink(const char *path, char *buf, size_t size)
{
  fuse_grpc_proto::ReadlinkRequest request;
  fuse_grpc_proto::ReadlinkReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_size(size);

  const Status status = stub_->Readlink(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[readlink] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  std::memcpy(buf, reply.buf().c_str(), size);

  return reply.result();
}

int
sync_client::opendir(const char *path, struct fuse_file_info *fi)
{
  fuse_grpc_proto::OpendirRequest request;
  fuse_grpc_proto::OpendirReply reply;
  ClientContext context;

  request.set_path(path);
  fill_StructFuseFileInfo(request.mutable_info(), fi);

  const Status status = stub_->Opendir(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[opendir] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  fill_fuse_file_info(fi, reply.info());

  return reply.result();
}

int
sync_client::readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                     struct fuse_file_info *fi)
{
  fuse_grpc_proto::ReaddirRequest request;
  fuse_grpc_proto::ReaddirReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_offset(offset);
  fill_StructFuseFileInfo(request.mutable_info(), fi);

  const Status status = stub_->Readdir(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[readdir] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  for (auto &entry : reply.dir_info_entries()) {
    struct stat st {
    };
    fill_struct_stat(&st, entry.stbuf());

    if (filler(buf, entry.name().c_str(), &st, entry.offset()))
      break;
  }

  fill_fuse_file_info(fi, reply.info());

  return reply.result();
}

int
sync_client::releasedir(const char *path, struct fuse_file_info *fi)
{
  fuse_grpc_proto::ReleasedirRequest request;
  fuse_grpc_proto::ReleasedirReply reply;
  ClientContext context;

  request.set_path(path);
  fill_StructFuseFileInfo(request.mutable_info(), fi);

  const Status status = stub_->Releasedir(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[releasedir] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  fill_fuse_file_info(fi, reply.info());

  return reply.result();
}

int
sync_client::mknod(const char *path, mode_t mode, dev_t rdev)
{
  fuse_grpc_proto::MknodRequest request;
  fuse_grpc_proto::MknodReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_mode(mode);
  request.set_rdev(rdev);

  const Status status = stub_->Mknod(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[mknod] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  return reply.result();
}

int
sync_client::mkdir(const char *path, mode_t mode)
{
  fuse_grpc_proto::MkdirRequest request;
  fuse_grpc_proto::MkdirReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_mode(mode);

  const Status status = stub_->Mkdir(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[mkdir] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  return reply.result();
}

int
sync_client::symlink(const char *from, const char *to)
{
  fuse_grpc_proto::SymlinkRequest request;
  fuse_grpc_proto::SymlinkReply reply;
  ClientContext context;

  request.set_from(from);
  request.set_to(to);

  const Status status = stub_->Symlink(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[symlink] [{}] from: {} to: {}", status.error_message(), from, to);
    return -1;
  }

  return reply.result();
}

int
sync_client::unlink(const char *path)
{
  fuse_grpc_proto::UnlinkRequest request;
  fuse_grpc_proto::UnlinkReply reply;
  ClientContext context;

  request.set_path(path);

  const Status status = stub_->Unlink(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[unlink] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  return reply.result();
}

int
sync_client::rmdir(const char *path)
{
  fuse_grpc_proto::RmdirRequest request;
  fuse_grpc_proto::RmdirReply reply;
  ClientContext context;

  request.set_path(path);

  const Status status = stub_->Rmdir(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[rmdir] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  return reply.result();
}

int
sync_client::rename(const char *from, const char *to)
{
  fuse_grpc_proto::RenameRequest request;
  fuse_grpc_proto::RenameReply reply;
  ClientContext context;

  request.set_from(from);
  request.set_to(to);

  const Status status = stub_->Rename(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[rename] [{}] from: {} to: {}", status.error_message(), from, to);
    return -1;
  }

  return reply.result();
}

int
sync_client::link(const char *from, const char *to)
{
  fuse_grpc_proto::LinkRequest request;
  fuse_grpc_proto::LinkReply reply;
  ClientContext context;

  request.set_from(from);
  request.set_to(to);

  const Status status = stub_->Link(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[link] [{}] from: {} to: {}", status.error_message(), from, to);
    return -1;
  }

  return reply.result();
}

int
sync_client::chmod(const char *path, mode_t mode)
{
  fuse_grpc_proto::ChmodRequest request;
  fuse_grpc_proto::ChmodReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_mode(mode);

  const Status status = stub_->Chmod(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[chmod] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  return reply.result();
}

int
sync_client::chown(const char *path, uid_t uid, gid_t gid)
{
  fuse_grpc_proto::ChownRequest request;
  fuse_grpc_proto::ChownReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_uid(uid);
  request.set_gid(gid);

  const Status status = stub_->Chown(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[chown] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  return reply.result();
}

int
sync_client::truncate(const char *path, off_t size)
{
  fuse_grpc_proto::TruncateRequest request;
  fuse_grpc_proto::TruncateReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_size(size);

  const Status status = stub_->Truncate(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[truncate] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  return reply.result();
}

int
sync_client::ftruncate(const char *path, off_t size, struct fuse_file_info *fi)
{
  fuse_grpc_proto::FtruncateRequest request;
  fuse_grpc_proto::FtruncateReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_size(size);
  fill_StructFuseFileInfo(request.mutable_info(), fi);

  const Status status = stub_->Ftruncate(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[ftruncate] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  fill_fuse_file_info(fi, reply.info());

  return reply.result();
}

int
sync_client::utimens(const char *path, const struct timespec ts[2])
{
  fuse_grpc_proto::UtimensRequest request;
  fuse_grpc_proto::UtimensReply reply;
  ClientContext context;

  request.set_path(path);
  fill_StructTimespec(request.mutable_tim0(), ts[0]);
  fill_StructTimespec(request.mutable_tim1(), ts[1]);

  const Status status = stub_->Utimens(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[utimens] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  return reply.result();
}

int
sync_client::create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  fuse_grpc_proto::CreateRequest request;
  fuse_grpc_proto::CreateReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_mode(mode);
  fill_StructFuseFileInfo(request.mutable_info(), fi);

  const Status status = stub_->Create(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[create] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  const int res = reply.result();

  if (res != -1) {
    create_streams(path, fi->flags);
  }

  fill_fuse_file_info(fi, reply.info());

  return res;
}

int
sync_client::open(const char *path, struct fuse_file_info *fi)
{
  fuse_grpc_proto::OpenRequest request;
  fuse_grpc_proto::OpenReply reply;
  ClientContext context;

  request.set_path(path);
  fill_StructFuseFileInfo(request.mutable_info(), fi);

  const Status status = stub_->Open(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[open] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  const int res = reply.result();

  if (res != -1) {
    create_streams(path, fi->flags);
  }

  fill_fuse_file_info(fi, reply.info());

  return reply.result();
}

int
sync_client::read(const char *path, char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi)
{
  fuse_grpc_proto::ReadRequest request;
  fuse_grpc_proto::ReadReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_size(size);
  request.set_offset(offset);
  fill_StructFuseFileInfo(request.mutable_info(), fi);

  std::unique_lock read_streams_lock(mtx_read_streams_);
  const auto streams_iterator = read_streams_.find(path);
  if (streams_iterator != read_streams_.end()) {
    auto &read_stream = streams_iterator->second;
    read_streams_lock.unlock();

    std::unique_lock write_unique_lock(read_stream.mtx_write_);
    if (!read_stream.stream_->Write(request)) {
      logging::critical("[read] [write to stream failed] path: {}", path);
      write_unique_lock.unlock();
      remove_streams(path);
      goto sync_request;
    }

    std::unique_lock read_unique_lock(read_stream.mtx_read_);
    write_unique_lock.unlock();

    if (!read_stream.stream_->Read(&reply)) {
      logging::critical("[read] [read from stream failed] path: {}", path);
      read_unique_lock.unlock();
      remove_streams(path);
      goto sync_request;
    }
    read_unique_lock.unlock();
  } else {
    read_streams_lock.unlock();
  sync_request:
    const Status status = stub_->Read(&context, request, &reply);
    if (!status.ok()) {
      logging::critical("[read] [{}] path: {}", status.error_message(), path);
      return -1;
    }
    create_streams(path, O_RDONLY);
  }

  const int res = reply.result();

  if (res > 0) {
    std::memcpy(buf, reply.buf().c_str(), res);
  }
  fill_fuse_file_info(fi, reply.info());

  return res;
}

int
sync_client::write(const char *path, const char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi)
{
  fuse_grpc_proto::WriteRequest request;
  fuse_grpc_proto::WriteReply reply;
  ClientContext context;

  // Set fuse_file_info flush flag to indicate a synchronous request
  fi->flush = 1;

  request.set_path(path);
  request.set_buf(buf, size);
  request.set_size(size);
  request.set_offset(offset);
  fill_StructFuseFileInfo(request.mutable_info(), fi);

  std::unique_lock write_streams_lock(mtx_write_streams_);
  const auto streams_iterator = write_streams_.find(path);
  if (streams_iterator != write_streams_.end()) {
    auto &write_stream = streams_iterator->second;
    write_streams_lock.unlock();

    std::unique_lock write_unique_lock(write_stream.mtx_write_);
    if (!write_stream.stream_->Write(request)) {
      logging::critical("[write] [write to stream failed] path: {}", path);
      write_unique_lock.unlock();
      remove_streams(path);
      goto sync_request;
    }

    std::unique_lock read_unique_lock(write_stream.mtx_read_);
    write_unique_lock.unlock();

    if (!write_stream.stream_->Read(&reply)) {
      logging::critical("[write] [read from stream failed] path: {}", path);
      read_unique_lock.unlock();
      remove_streams(path);
      goto sync_request;
    }
    read_unique_lock.unlock();
  } else {
    write_streams_lock.unlock();
  sync_request:
    const Status status = stub_->Write(&context, request, &reply);
    if (!status.ok()) {
      logging::critical("[write] [{}] path: {}", status.error_message(), path);
      return -1;
    }
    create_streams(path, O_WRONLY);
  }

  fill_fuse_file_info(fi, reply.info());

  return reply.result();
}

int
sync_client::statfs(const char *path, struct statvfs *stbuf)
{
  fuse_grpc_proto::StatfsRequest request;
  fuse_grpc_proto::StatfsReply reply;
  ClientContext context;

  request.set_path(path);

  const Status status = stub_->Statfs(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[statfs] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  fill_struct_statvfs(stbuf, reply.stbuf());

  return reply.result();
}

int
sync_client::flush(const char *path, struct fuse_file_info *fi)
{
  fuse_grpc_proto::FlushRequest request;
  fuse_grpc_proto::FlushReply reply;
  ClientContext context;

  request.set_path(path);
  fill_StructFuseFileInfo(request.mutable_info(), fi);

  const Status status = stub_->Flush(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[flush] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  fill_fuse_file_info(fi, reply.info());

  return reply.result();
}

int
sync_client::release(const char *path, struct fuse_file_info *fi)
{
  fuse_grpc_proto::ReleaseRequest request;
  fuse_grpc_proto::ReleaseReply reply;
  ClientContext context;

  request.set_path(path);
  fill_StructFuseFileInfo(request.mutable_info(), fi);

  const Status status = stub_->Release(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[release] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  remove_streams(path);

  fill_fuse_file_info(fi, reply.info());

  return reply.result();
}

int
sync_client::fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
  fuse_grpc_proto::FsyncRequest request;
  fuse_grpc_proto::FsyncReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_isdatasync(isdatasync);
  fill_StructFuseFileInfo(request.mutable_info(), fi);

  const Status status = stub_->Fsync(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[fsync] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  fill_fuse_file_info(fi, reply.info());

  return reply.result();
}

int
sync_client::fallocate(const char *path, int mode, off_t offset, off_t length,
                       struct fuse_file_info *fi)
{
  fuse_grpc_proto::FallocateRequest request;
  fuse_grpc_proto::FallocateReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_mode(mode);
  request.set_offset(offset);
  request.set_length(length);
  fill_StructFuseFileInfo(request.mutable_info(), fi);

  const Status status = stub_->Fallocate(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[fallocate] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  fill_fuse_file_info(fi, reply.info());

  return reply.result();
}

int
sync_client::setxattr(const char *path, const char *name, const char *value, size_t size,
                      int flags)
{
  fuse_grpc_proto::SetxattrRequest request;
  fuse_grpc_proto::SetxattrReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_name(name);
  request.set_value(value, size);
  request.set_size(size);
  request.set_flags(flags);

  const Status status = stub_->Setxattr(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[setxattr] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  return reply.result();
}

int
sync_client::getxattr(const char *path, const char *name, char *value, size_t size)
{
  fuse_grpc_proto::GetxattrRequest request;
  fuse_grpc_proto::GetxattrReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_name(name);
  request.set_size(size);

  const Status status = stub_->Getxattr(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[getxattr] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  const int res = reply.result();

  if (res > 0) {
    std::memcpy(value, reply.value().c_str(), res);
  }

  return reply.result();
}

int
sync_client::listxattr(const char *path, char *list, size_t size)
{
  fuse_grpc_proto::ListxattrRequest request;
  fuse_grpc_proto::ListxattrReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_size(size);

  const Status status = stub_->Listxattr(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[listxattr] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  const int res = reply.result();

  if (res > 0) {
    std::memcpy(list, reply.list().c_str(), res);
  }

  return reply.result();
}

int
sync_client::removexattr(const char *path, const char *name)
{
  fuse_grpc_proto::RemovexattrRequest request;
  fuse_grpc_proto::RemovexattrReply reply;
  ClientContext context;

  request.set_path(path);
  request.set_name(name);

  const Status status = stub_->Removexattr(&context, request, &reply);

  if (!status.ok()) {
    logging::critical("[removexattr] [{}] path: {}", status.error_message(), path);
    return -1;
  }

  return reply.result();
}

void
sync_client::create_streams(const std::string &path, int flags)
{
  const int mode = flags & O_ACCMODE;

  if (mode == O_RDONLY || flags == O_RDONLY) {
    std::unique_lock read_streams_unique_lock(mtx_read_streams_);
    read_streams_.try_emplace(path, stub_);
  } else if (mode == O_WRONLY || flags == O_WRONLY) {
    std::unique_lock write_streams_unique_lock(mtx_write_streams_);
    write_streams_.try_emplace(path, stub_);
  } else if (mode == O_RDWR || flags == O_RDWR) {
    std::unique_lock read_streams_unique_lock(mtx_read_streams_);
    read_streams_.try_emplace(path, stub_);
    read_streams_unique_lock.unlock();
    std::unique_lock write_streams_unique_lock(mtx_write_streams_);
    write_streams_.try_emplace(path, stub_);
    write_streams_unique_lock.unlock();
  }
}

void
sync_client::remove_streams(const std::string &path)
{
  std::unique_lock read_streams_unique_lock(mtx_read_streams_);
  const auto read_streams_iterator = read_streams_.find(path);
  if (read_streams_iterator != read_streams_.end()) {
    read_stream &read_stream = read_streams_iterator->second;
    {
      std::scoped_lock stream_lock(read_stream.mtx_write_, read_stream.mtx_read_);
      read_stream.stream_->WritesDone();
    }
    read_streams_.erase(read_streams_iterator);
  }
  read_streams_unique_lock.unlock();

  std::unique_lock write_streams_unique_lock(mtx_write_streams_);
  const auto write_streams_iterator = write_streams_.find(path);
  if (write_streams_iterator != write_streams_.end()) {
    write_stream &write_stream = write_streams_iterator->second;
    {
      std::scoped_lock stream_lock(write_stream.mtx_write_, write_stream.mtx_read_);
      write_stream.stream_->WritesDone();
    }
    write_streams_.erase(write_streams_iterator);
  }
  write_streams_unique_lock.unlock();
}

sync_client::read_stream::read_stream(
    const std::unique_ptr<fuse_grpc_proto::FuseOps::Stub> &stub)
    : client_context_()
    , mtx_write_()
    , mtx_read_()
{
  std::scoped_lock lock(mtx_write_, mtx_read_);
  stream_ = stub->StreamRead(&client_context_);
}

sync_client::write_stream::write_stream(
    const std::unique_ptr<fuse_grpc_proto::FuseOps::Stub> &stub)
    : client_context_()
    , mtx_write_()
    , mtx_read_()
{
  std::scoped_lock lock(mtx_write_, mtx_read_);
  stream_ = stub->StreamWrite(&client_context_);
}

} // namespace rsafefs::fuse_rpc::grpc