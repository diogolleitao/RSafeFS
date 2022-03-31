#pragma once

#include "fuse_operations.grpc.pb.h"
#include "rsafefs/fuse_rpc/server.hpp"
#include "rsafefs/fuse_wrapper/fuse31.hpp"
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <thread>

namespace rsafefs::fuse_rpc::grpc
{

namespace proto = fuse_grpc_proto;

using Service = proto::FuseOps::AsyncService;
using Server = ::grpc::Server;
using Status = ::grpc::Status;
using ServerCompletionQueue = ::grpc::ServerCompletionQueue;
using ServerContext = ::grpc::ServerContext;
template <typename T>
using ServerAsyncResponseWriter = ::grpc::ServerAsyncResponseWriter<T>;
template <typename T, typename V>
using ServerAsyncReader = ::grpc::ServerAsyncReader<T, V>;
template <typename T, typename V>
using ServerAsyncReaderWriter = ::grpc::ServerAsyncReaderWriter<T, V>;

class server : public fuse_rpc::server
{
public:
  struct config : fuse_rpc::server::config {
    config(std::string server_address, size_t n_queues, size_t n_pollers_per_queue)
        : server_address_(server_address)
        , n_queues_(n_queues)
        , n_pollers_per_queue_(n_pollers_per_queue)
    {
    }

    std::string server_address_;
    size_t n_queues_;
    size_t n_pollers_per_queue_;
  };

  server(server::config &config, const fuse_operations &operations);

  ~server() override;

  void run() override;

private:
  static void handle_async_requests(ServerCompletionQueue *cq);

  class call_data
  {
  public:
    call_data(Service &service, ServerCompletionQueue *cq,
              const fuse_operations &operations);

    virtual ~call_data() = default;

    virtual void proceed(bool ok) = 0;

  protected:
    Service &service_;
    ServerCompletionQueue *cq_;
    ServerContext srv_ctx_;
    const fuse_operations &operations_;
  };

  class unary_call_data : protected call_data
  {
  public:
    unary_call_data(Service &service, ServerCompletionQueue *cq,
                    const fuse_operations &operations);

    void proceed(bool ok) override = 0;

  protected:
    enum call_status {
      PROCESS,
      FINISHED,
    };
    call_status call_status_;
  };

  class bidi_stream_call_data : protected call_data
  {
  public:
    bidi_stream_call_data(Service &service, ServerCompletionQueue *cq,
                          const fuse_operations &operations);

    void proceed(bool ok) override = 0;

  protected:
    enum call_status {
      CREATE,
      READ,
      WRITE,
      FINISHED,
    };
    call_status call_status_;
  };

  class getattr_data : unary_call_data
  {
  public:
    getattr_data(Service &service, ServerCompletionQueue *cq,
                 const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::GetattrReply> responder_;
    proto::GetattrRequest request_;
    proto::GetattrReply reply_;
  };

  class fgetattr_data : unary_call_data
  {
  public:
    fgetattr_data(Service &service, ServerCompletionQueue *cq,
                  const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::FgetattrReply> responder_;
    proto::FgetattrRequest request_;
    proto::FgetattrReply reply_;
  };

  class access_data : unary_call_data
  {
  public:
    access_data(Service &service, ServerCompletionQueue *cq,
                const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::AccessReply> responder_;
    proto::AccessRequest request_;
    proto::AccessReply reply_;
  };

  class readlink_data : unary_call_data
  {
  public:
    readlink_data(Service &service, ServerCompletionQueue *cq,
                  const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::ReadlinkReply> responder_;
    proto::ReadlinkRequest request_;
    proto::ReadlinkReply reply_;
  };

  class opendir_data : unary_call_data
  {
  public:
    opendir_data(Service &service, ServerCompletionQueue *cq,
                 const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::OpendirReply> responder_;
    proto::OpendirRequest request_;
    proto::OpendirReply reply_;
  };

  class readdir_data : unary_call_data
  {
  public:
    readdir_data(Service &service, ServerCompletionQueue *cq,
                 const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::ReaddirReply> responder_;
    proto::ReaddirRequest request_;
    proto::ReaddirReply reply_;
  };

  class releasedir_data : unary_call_data
  {
  public:
    releasedir_data(Service &service, ServerCompletionQueue *cq,
                    const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::ReleasedirReply> responder_;
    proto::ReleasedirRequest request_;
    proto::ReleasedirReply reply_;
  };

  class mknod_data : unary_call_data
  {
  public:
    mknod_data(Service &service, ServerCompletionQueue *cq,
               const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::MknodReply> responder_;
    proto::MknodRequest request_;
    proto::MknodReply reply_;
  };

  class mkdir_data : unary_call_data
  {
  public:
    mkdir_data(Service &service, ServerCompletionQueue *cq,
               const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::MkdirReply> responder_;
    proto::MkdirRequest request_;
    proto::MkdirReply reply_;
  };

  class symlink_data : unary_call_data
  {
  public:
    symlink_data(Service &service, ServerCompletionQueue *cq,
                 const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::SymlinkReply> responder_;
    proto::SymlinkRequest request_;
    proto::SymlinkReply reply_;
  };

  class unlink_data : unary_call_data
  {
  public:
    unlink_data(Service &service, ServerCompletionQueue *cq,
                const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::UnlinkReply> responder_;
    proto::UnlinkRequest request_;
    proto::UnlinkReply reply_;
  };

  class rmdir_data : unary_call_data
  {
  public:
    rmdir_data(Service &service, ServerCompletionQueue *cq,
               const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::RmdirReply> responder_;
    proto::RmdirRequest request_;
    proto::RmdirReply reply_;
  };

  class rename_data : unary_call_data
  {
  public:
    rename_data(Service &service, ServerCompletionQueue *cq,
                const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::RenameReply> responder_;
    proto::RenameRequest request_;
    proto::RenameReply reply_;
  };

  class link_data : unary_call_data
  {
  public:
    link_data(Service &service, ServerCompletionQueue *cq,
              const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::LinkReply> responder_;
    proto::LinkRequest request_;
    proto::LinkReply reply_;
  };

  class chmod_data : unary_call_data
  {
  public:
    chmod_data(Service &service, ServerCompletionQueue *cq,
               const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::ChmodReply> responder_;
    proto::ChmodRequest request_;
    proto::ChmodReply reply_;
  };

  class chown_data : unary_call_data
  {
  public:
    chown_data(Service &service, ServerCompletionQueue *cq,
               const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::ChownReply> responder_;
    proto::ChownRequest request_;
    proto::ChownReply reply_;
  };

  class truncate_data : unary_call_data
  {
  public:
    truncate_data(Service &service, ServerCompletionQueue *cq,
                  const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::TruncateReply> responder_;
    proto::TruncateRequest request_;
    proto::TruncateReply reply_;
  };

  class ftruncate_data : unary_call_data
  {
  public:
    ftruncate_data(Service &service, ServerCompletionQueue *cq,
                   const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::FtruncateReply> responder_;
    proto::FtruncateRequest request_;
    proto::FtruncateReply reply_;
  };

  class utimens_data : unary_call_data
  {
  public:
    utimens_data(Service &service, ServerCompletionQueue *cq,
                 const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::UtimensReply> responder_;
    proto::UtimensRequest request_;
    proto::UtimensReply reply_;
  };

  class create_data : unary_call_data
  {
  public:
    create_data(Service &service, ServerCompletionQueue *cq,
                const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::CreateReply> responder_;
    proto::CreateRequest request_;
    proto::CreateReply reply_;
  };

  class open_data : unary_call_data
  {
  public:
    open_data(Service &service, ServerCompletionQueue *cq,
              const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::OpenReply> responder_;
    proto::OpenRequest request_;
    proto::OpenReply reply_;
  };

  class read_data : unary_call_data
  {
  public:
    read_data(Service &service, ServerCompletionQueue *cq,
              const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::ReadReply> responder_;
    proto::ReadRequest request_;
    proto::ReadReply reply_;
  };

  class write_data : unary_call_data
  {
  public:
    write_data(Service &service, ServerCompletionQueue *cq,
               const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::WriteReply> responder_;
    proto::WriteRequest request_;
    proto::WriteReply reply_;
  };

  class statfs_data : unary_call_data
  {
  public:
    statfs_data(Service &service, ServerCompletionQueue *cq,
                const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::StatfsReply> responder_;
    proto::StatfsRequest request_;
    proto::StatfsReply reply_;
  };

  class flush_data : unary_call_data
  {
  public:
    flush_data(Service &service, ServerCompletionQueue *cq,
               const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::FlushReply> responder_;
    proto::FlushRequest request_;
    proto::FlushReply reply_;
  };

  class release_data : unary_call_data
  {
  public:
    release_data(Service &service, ServerCompletionQueue *cq,
                 const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::ReleaseReply> responder_;
    proto::ReleaseRequest request_;
    proto::ReleaseReply reply_;
  };

  class fsync_data : unary_call_data
  {
  public:
    fsync_data(Service &service, ServerCompletionQueue *cq,
               const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::FsyncReply> responder_;
    proto::FsyncRequest request_;
    proto::FsyncReply reply_;
  };

  class fallocate_data : unary_call_data
  {
  public:
    fallocate_data(Service &service, ServerCompletionQueue *cq,
                   const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::FallocateReply> responder_;
    proto::FallocateRequest request_;
    proto::FallocateReply reply_;
  };

  class setxattr_data : unary_call_data
  {
  public:
    setxattr_data(Service &service, ServerCompletionQueue *cq,
                  const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::SetxattrReply> responder_;
    proto::SetxattrRequest request_;
    proto::SetxattrReply reply_;
  };

  class getxattr_data : unary_call_data
  {
  public:
    getxattr_data(Service &service, ServerCompletionQueue *cq,
                  const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::GetxattrReply> responder_;
    proto::GetxattrRequest request_;
    proto::GetxattrReply reply_;
  };

  class listxattr_data : unary_call_data
  {
  public:
    listxattr_data(Service &service, ServerCompletionQueue *cq,
                   const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::ListxattrReply> responder_;
    proto::ListxattrRequest request_;
    proto::ListxattrReply reply_;
  };

  class removexattr_data : unary_call_data
  {
  public:
    removexattr_data(Service &service, ServerCompletionQueue *cq,
                     const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncResponseWriter<proto::RemovexattrReply> responder_;
    proto::RemovexattrRequest request_;
    proto::RemovexattrReply reply_;
  };

  class stream_read_data : bidi_stream_call_data
  {
  public:
    stream_read_data(Service &service, ServerCompletionQueue *cq,
                     const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncReaderWriter<proto::ReadReply, proto::ReadRequest> stream_;
    proto::ReadRequest request_;
    proto::ReadReply reply_;
  };

  class stream_write_data : bidi_stream_call_data
  {
  public:
    stream_write_data(Service &service, ServerCompletionQueue *cq,
                      const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncReaderWriter<proto::WriteReply, proto::WriteRequest> stream_;
    proto::WriteRequest request_;
    proto::WriteReply reply_;
  };

  class ac_stream_write_data : call_data
  {
  public:
    ac_stream_write_data(Service &service, ServerCompletionQueue *cq,
                         const fuse_operations &operations);

    void proceed(bool ok) override;

  private:
    ServerAsyncReader<proto::WriteReply, proto::WriteRequest> reader_;
    proto::WriteRequest request_;
    enum call_status {
      CREATE,
      READ,
      FINISHED,
    };
    call_status call_status_;
  };

  grpc::server::config config_;
  const fuse_operations &operations_;

  Service service_;
  std::unique_ptr<Server> server_;
  std::vector<std::unique_ptr<ServerCompletionQueue>> cqs_;
  std::vector<std::thread> pollers_;
};

} // namespace rsafefs::fuse_rpc::grpc
