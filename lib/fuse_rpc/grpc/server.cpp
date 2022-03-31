#include "rsafefs/fuse_rpc/server.hpp"
#include "rsafefs/fuse_rpc/grpc/server.hpp"
#include "rsafefs/fuse_rpc/grpc/structs_fillers.hpp"
#include "rsafefs/utils/logging.hpp"
#include <grpcpp/server_builder.h>

#ifdef __APPLE__
#include <sys/xattr.h>
#endif

namespace rsafefs::fuse_rpc::grpc
{

server::server(server::config &config, const fuse_operations &operations)
    : config_(config)
    , operations_(operations)
{
  config_.n_queues_ = std::max(1UL, config.n_queues_);
  config_.n_pollers_per_queue_ = std::max(1UL, config.n_pollers_per_queue_);
}

server::~server()
{
  if (server_ != nullptr) {
    server_->Shutdown();
  }

  for (auto &cq : cqs_) {
    cq->Shutdown();
  }

  void *got_tag;
  bool ok;
  for (auto &cq : cqs_) {
    while (cq->Next(&got_tag, &ok)) {
      static_cast<call_data *>(got_tag)->proceed(ok);
    }
  }

  for (auto &thread : pollers_) {
    thread.join();
  }

  if (operations_.destroy != nullptr) {
    operations_.destroy(nullptr);
  }
}

void
server::run()
{
  ::grpc::ServerBuilder builder;
  builder.AddListeningPort(config_.server_address_, ::grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);
  builder.SetMaxReceiveMessageSize(-1);
  builder.SetMaxSendMessageSize(-1);

  for (size_t i = 0; i < config_.n_queues_; i++) {
    cqs_.emplace_back(builder.AddCompletionQueue());
  }

  server_ = builder.BuildAndStart();
  logging::info("Starting Server... listening on {}", config_.server_address_);

  for (auto &cq : cqs_) {
    for (int i = 0; i < 20; i++) {
      new getattr_data(service_, cq.get(), operations_);
      new fgetattr_data(service_, cq.get(), operations_);
      new access_data(service_, cq.get(), operations_);
      new readlink_data(service_, cq.get(), operations_);
      new opendir_data(service_, cq.get(), operations_);
      new readdir_data(service_, cq.get(), operations_);
      new releasedir_data(service_, cq.get(), operations_);
      new mknod_data(service_, cq.get(), operations_);
      new mkdir_data(service_, cq.get(), operations_);
      new symlink_data(service_, cq.get(), operations_);
      new unlink_data(service_, cq.get(), operations_);
      new rmdir_data(service_, cq.get(), operations_);
      new rename_data(service_, cq.get(), operations_);
      new link_data(service_, cq.get(), operations_);
      new chmod_data(service_, cq.get(), operations_);
      new chown_data(service_, cq.get(), operations_);
      new truncate_data(service_, cq.get(), operations_);
      new ftruncate_data(service_, cq.get(), operations_);
      new utimens_data(service_, cq.get(), operations_);
      new create_data(service_, cq.get(), operations_);
      new open_data(service_, cq.get(), operations_);
      new read_data(service_, cq.get(), operations_);
      new write_data(service_, cq.get(), operations_);
      new statfs_data(service_, cq.get(), operations_);
      new flush_data(service_, cq.get(), operations_);
      new release_data(service_, cq.get(), operations_);
      new fsync_data(service_, cq.get(), operations_);
      new fallocate_data(service_, cq.get(), operations_);
      new setxattr_data(service_, cq.get(), operations_);
      new getxattr_data(service_, cq.get(), operations_);
      new listxattr_data(service_, cq.get(), operations_);
      new removexattr_data(service_, cq.get(), operations_);

      new stream_read_data(service_, cq.get(), operations_);
      new stream_write_data(service_, cq.get(), operations_);
      new ac_stream_write_data(service_, cq.get(), operations_);
    }
  }

  for (auto &cq : cqs_) {
    for (size_t i = 0; i < config_.n_pollers_per_queue_; i++) {
      pollers_.emplace_back(&server::handle_async_requests, cq.get());
    }
  }

  if (operations_.init != nullptr) {
    operations_.init(nullptr);
  }

  server_->Wait();
}

void
server::handle_async_requests(ServerCompletionQueue *cq)
{
  void *got_tag;
  bool ok;
  while (true) {
    GPR_ASSERT(cq->Next(&got_tag, &ok));
    static_cast<call_data *>(got_tag)->proceed(ok);
  }
}

server::call_data::call_data(Service &service, ServerCompletionQueue *cq,
                             const fuse_operations &operations)
    : service_(service)
    , cq_(cq)
    , operations_(operations)
{
}

server::unary_call_data::unary_call_data(Service &service, ServerCompletionQueue *cq,
                                         const fuse_operations &operations)
    : call_data(service, cq, operations)
    , call_status_(PROCESS)
{
}

server::bidi_stream_call_data::bidi_stream_call_data(Service &service,
                                                     ServerCompletionQueue *cq,
                                                     const fuse_operations &operations)
    : call_data(service, cq, operations)
    , call_status_(CREATE)
{
}

server::getattr_data::getattr_data(Service &service, ServerCompletionQueue *cq,
                                   const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestGetattr(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::getattr_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new getattr_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    struct stat stbuf {
    };

    const int res = operations_.getattr(path, &stbuf);

    fill_StructStat(reply_.mutable_stbuf(), stbuf);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::fgetattr_data::fgetattr_data(Service &service, ServerCompletionQueue *cq,
                                     const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestFgetattr(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::fgetattr_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new fgetattr_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    struct stat stbuf {
    };
    fuse_file_info fi{};
    fill_fuse_file_info(&fi, request_.info());

    const int res = operations_.fgetattr(path, &stbuf, &fi);

    fill_StructStat(reply_.mutable_stbuf(), stbuf);
    fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::access_data::access_data(Service &service, ServerCompletionQueue *cq,
                                 const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestAccess(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::access_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new access_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const int mask = request_.mask();

    const int res = operations_.access(path, mask);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::readlink_data::readlink_data(Service &service, ServerCompletionQueue *cq,
                                     const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestReadlink(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::readlink_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new readlink_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const size_t size = request_.size();
    std::string *buf = reply_.mutable_buf();
    buf->resize(size);

    const int res = operations_.readlink(path, buf->data(), size);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::opendir_data::opendir_data(Service &service, ServerCompletionQueue *cq,
                                   const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestOpendir(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::opendir_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new opendir_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    fuse_file_info fi{};
    fill_fuse_file_info(&fi, request_.info());

    const int res = operations_.opendir(path, &fi);

    fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::readdir_data::readdir_data(Service &service, ServerCompletionQueue *cq,
                                   const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestReaddir(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::readdir_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new readdir_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const off_t offset = request_.offset();
    fuse_file_info fi{};
    fill_fuse_file_info(&fi, request_.info());
    DirInfo di;

    const int res = operations_.readdir(path, &di, rpc_filler, offset, &fi);

    for (auto &entry : di.buf_)
      fill_StructDirEntryInfo(reply_.add_dir_info_entries(), entry);

    fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::releasedir_data::releasedir_data(Service &service, ServerCompletionQueue *cq,
                                         const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestReleasedir(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::releasedir_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new releasedir_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    fuse_file_info fi{};
    fill_fuse_file_info(&fi, request_.info());

    const int res = operations_.releasedir(path, &fi);

    fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::mknod_data::mknod_data(Service &service, ServerCompletionQueue *cq,
                               const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestMknod(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::mknod_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new mknod_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const mode_t mode = request_.mode();
    const dev_t rdev = request_.rdev();

    const int res = operations_.mknod(path, mode, rdev);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::mkdir_data::mkdir_data(Service &service, ServerCompletionQueue *cq,
                               const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestMkdir(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::mkdir_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new mkdir_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const mode_t mode = request_.mode();

    const int res = operations_.mkdir(path, mode);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::symlink_data::symlink_data(Service &service, ServerCompletionQueue *cq,
                                   const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestSymlink(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::symlink_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new symlink_data(service_, cq_, operations_);

    const char *from = request_.from().c_str();
    const char *to = request_.to().c_str();

    const int res = operations_.symlink(from, to);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::unlink_data::unlink_data(Service &service, ServerCompletionQueue *cq,
                                 const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestUnlink(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::unlink_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new unlink_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();

    const int res = operations_.unlink(path);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::rmdir_data::rmdir_data(Service &service, ServerCompletionQueue *cq,
                               const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestRmdir(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::rmdir_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new rmdir_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();

    const int res = operations_.rmdir(path);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::rename_data::rename_data(Service &service, ServerCompletionQueue *cq,
                                 const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestRename(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::rename_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new rename_data(service_, cq_, operations_);

    const char *from = request_.from().c_str();
    const char *to = request_.to().c_str();

    const int res = operations_.rename(from, to);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::link_data::link_data(Service &service, ServerCompletionQueue *cq,
                             const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestLink(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::link_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new link_data(service_, cq_, operations_);

    const char *from = request_.from().c_str();
    const char *to = request_.to().c_str();

    const int res = operations_.link(from, to);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::chmod_data::chmod_data(Service &service, ServerCompletionQueue *cq,
                               const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestChmod(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::chmod_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new chmod_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const mode_t mode = request_.mode();

    const int res = operations_.chmod(path, mode);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::chown_data::chown_data(Service &service, ServerCompletionQueue *cq,
                               const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestChown(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::chown_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new chown_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const uid_t uid = request_.uid();
    const gid_t gid = request_.gid();

    const int res = operations_.chown(path, uid, gid);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::truncate_data::truncate_data(Service &service, ServerCompletionQueue *cq,
                                     const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestTruncate(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::truncate_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new truncate_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const size_t size = request_.size();

    const int res = operations_.truncate(path, size);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::ftruncate_data::ftruncate_data(Service &service, ServerCompletionQueue *cq,
                                       const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestFtruncate(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::ftruncate_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new ftruncate_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const size_t size = request_.size();
    fuse_file_info fi{};
    fill_fuse_file_info(&fi, request_.info());

    const int res = operations_.ftruncate(path, size, &fi);

    fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::utimens_data::utimens_data(Service &service, ServerCompletionQueue *cq,
                                   const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestUtimens(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::utimens_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new utimens_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    struct timespec ts[2];

    fill_struct_timespec(ts[0], request_.tim0());
    fill_struct_timespec(ts[1], request_.tim1());

    const int res = operations_.utimens(path, ts);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::create_data::create_data(Service &service, ServerCompletionQueue *cq,
                                 const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestCreate(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::create_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new create_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const mode_t mode = request_.mode();
    fuse_file_info fi{};
    fill_fuse_file_info(&fi, request_.info());

    const int res = operations_.create(path, mode, &fi);

    fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::open_data::open_data(Service &service, ServerCompletionQueue *cq,
                             const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestOpen(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::open_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new open_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    fuse_file_info fi{};
    fill_fuse_file_info(&fi, request_.info());

    const int res = operations_.open(path, &fi);

    fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::read_data::read_data(Service &service, ServerCompletionQueue *cq,
                             const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestRead(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::read_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new read_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const size_t size = request_.size();
    const off_t offset = request_.offset();
    fuse_file_info fi{};
    fill_fuse_file_info(&fi, request_.info());
    std::string *buf = reply_.mutable_buf();
    buf->resize(size);

    const int res = operations_.read(path, buf->data(), size, offset, &fi);

    fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::write_data::write_data(Service &service, ServerCompletionQueue *cq,
                               const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestWrite(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::write_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new write_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const size_t size = request_.size();
    const off_t offset = request_.offset();
    fuse_file_info fi{};
    fill_fuse_file_info(&fi, request_.info());

    const int res = operations_.write(path, request_.buf().c_str(), size, offset, &fi);

    fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::statfs_data::statfs_data(Service &service, ServerCompletionQueue *cq,
                                 const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestStatfs(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::statfs_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new statfs_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    struct statvfs stbuf {
    };

    const int res = operations_.statfs(path, &stbuf);

    fill_StructStatvfs(reply_.mutable_stbuf(), stbuf);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::flush_data::flush_data(Service &service, ServerCompletionQueue *cq,
                               const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestFlush(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::flush_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new flush_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    fuse_file_info fi{};
    fill_fuse_file_info(&fi, request_.info());

    const int res = operations_.flush(path, &fi);

    fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::release_data::release_data(Service &service, ServerCompletionQueue *cq,
                                   const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestRelease(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::release_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new release_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    fuse_file_info fi{};
    fill_fuse_file_info(&fi, request_.info());

    const int res = operations_.release(path, &fi);

    fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::fsync_data::fsync_data(Service &service, ServerCompletionQueue *cq,
                               const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestFsync(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::fsync_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new fsync_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const int isdatasync = request_.isdatasync();
    fuse_file_info fi{};
    fill_fuse_file_info(&fi, request_.info());

    const int res = operations_.fsync(path, isdatasync, &fi);

    fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::fallocate_data::fallocate_data(Service &service, ServerCompletionQueue *cq,
                                       const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestFallocate(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::fallocate_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new fallocate_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const int mode = request_.mode();
    const off_t offset = request_.offset();
    const off_t length = request_.length();
    struct fuse_file_info fi {
    };
    fill_fuse_file_info(&fi, request_.info());

    const int res = operations_.fallocate(path, mode, offset, length, &fi);

    fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::setxattr_data::setxattr_data(Service &service, ServerCompletionQueue *cq,
                                     const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestSetxattr(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::setxattr_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new setxattr_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const std::string &name = request_.name();
    const std::string &value = request_.value();
    const size_t size = request_.size();
    const int flags = request_.flags();

#ifdef __APPLE__
    const int res = operations_.setxattr(path, name.c_str(), value.c_str(), size, flags,
                                         XATTR_NOFOLLOW);
#else
    const int res = operations_.setxattr(path, name.c_str(), value.c_str(), size, flags);
#endif

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::getxattr_data::getxattr_data(Service &service, ServerCompletionQueue *cq,
                                     const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestGetxattr(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::getxattr_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new getxattr_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const std::string &name = request_.name();
    const size_t size = request_.size();
    std::string *value = reply_.mutable_value();
    value->resize(size);

#ifdef __APPLE__
    const int res =
        operations_.getxattr(path, name.c_str(), value->data(), size, XATTR_NOFOLLOW);
#else
    const int res = operations_.getxattr(path, name.c_str(), value->data(), size);
#endif

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::listxattr_data::listxattr_data(Service &service, ServerCompletionQueue *cq,
                                       const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestListxattr(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::listxattr_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new listxattr_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const size_t size = request_.size();
    std::string *list = reply_.mutable_list();
    list->resize(size);

    const int res = operations_.listxattr(path, list->data(), size);

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::removexattr_data::removexattr_data(Service &service, ServerCompletionQueue *cq,
                                           const fuse_operations &operations)
    : unary_call_data(service, cq, operations)
    , responder_(&srv_ctx_)
{
  service_.RequestRemovexattr(&srv_ctx_, &request_, &responder_, cq_, cq_, this);
}

void
server::removexattr_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case PROCESS: {
    new removexattr_data(service_, cq_, operations_);

    const char *path = request_.path().c_str();
    const std::string &name = request_.name();

    const int res = operations_.removexattr(path, name.c_str());

    reply_.set_result(res);

    call_status_ = FINISHED;
    responder_.Finish(reply_, Status::OK, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::stream_read_data::stream_read_data(Service &service, ServerCompletionQueue *cq,
                                           const fuse_operations &operations)
    : bidi_stream_call_data(service, cq, operations)
    , stream_(&srv_ctx_)
{
  service_.RequestStreamRead(&srv_ctx_, &stream_, cq_, cq_, this);
}

void
server::stream_read_data::proceed(bool ok)
{
  switch (call_status_) {
  case CREATE: {
    new stream_read_data(service_, cq_, operations_);
    call_status_ = READ;
    stream_.Read(&request_, this);
    break;
  }
  case READ: {
    if (ok) {
      const std::string &path = request_.path();
      const size_t size = request_.size();
      const off_t offset = request_.offset();
      struct fuse_file_info fi {
      };
      fill_fuse_file_info(&fi, request_.info());
      std::string *buf = reply_.mutable_buf();
      buf->resize(size);

      const int res = operations_.read(path.c_str(), buf->data(), size, offset, &fi);

      fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
      reply_.set_result(res);

      call_status_ = WRITE;
      stream_.Write(reply_, this);
    } else {
      call_status_ = FINISHED;
      stream_.Finish(Status::OK, this);
    }
    break;
  }
  case WRITE: {
    call_status_ = READ;
    stream_.Read(&request_, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::stream_write_data::stream_write_data(Service &service, ServerCompletionQueue *cq,
                                             const fuse_operations &operations)
    : bidi_stream_call_data(service, cq, operations)
    , stream_(&srv_ctx_)
{
  service_.RequestStreamWrite(&srv_ctx_, &stream_, cq_, cq_, this);
}

void
server::stream_write_data::proceed(bool ok)
{
  switch (call_status_) {
  case CREATE: {
    new stream_write_data(service_, cq_, operations_);
    call_status_ = READ;
    stream_.Read(&request_, this);
    break;
  }
  case READ: {
    if (ok) {
      const std::string &path = request_.path();
      const size_t size = request_.size();
      const off_t offset = request_.offset();
      fuse_file_info fi{};
      fill_fuse_file_info(&fi, request_.info());

      const int res =
          operations_.write(path.c_str(), request_.buf().c_str(), size, offset, &fi);

      fill_StructFuseFileInfo(reply_.mutable_info(), &fi);
      reply_.set_result(res);

      call_status_ = WRITE;
      stream_.Write(reply_, this);
    } else {
      call_status_ = FINISHED;
      stream_.Finish(Status::OK, this);
    }
    break;
  }
  case WRITE: {
    call_status_ = READ;
    stream_.Read(&request_, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

server::ac_stream_write_data::ac_stream_write_data(Service &service,
                                                   ServerCompletionQueue *cq,
                                                   const fuse_operations &operations)
    : call_data(service, cq, operations)
    , reader_(&srv_ctx_)
    , call_status_(CREATE)
{
  service_.RequestACStreamWrite(&srv_ctx_, &reader_, cq_, cq_, this);
}

void
server::ac_stream_write_data::proceed(bool ok)
{
  switch (call_status_) {
  case CREATE: {
    new ac_stream_write_data(service_, cq_, operations_);
    call_status_ = READ;
    reader_.Read(&request_, this);
    break;
  }
  case READ: {
    if (ok) {
      const std::string &path = request_.path();
      const size_t size = request_.size();
      const off_t offset = request_.offset();
      fuse_file_info fi{};
      fill_fuse_file_info(&fi, request_.info());

      const int res =
          operations_.write(path.c_str(), request_.buf().c_str(), size, offset, &fi);
      if (res == -1) {
      }

      reader_.Read(&request_, this);
    } else {
      call_status_ = FINISHED;
      proto::WriteReply reply;
      reader_.Finish(reply, Status::OK, this);
    }
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    delete this;
  }
  }
}

} // namespace rsafefs::fuse_rpc::grpc