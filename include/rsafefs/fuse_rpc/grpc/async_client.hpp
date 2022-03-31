#pragma once

#include "fuse_operations.grpc.pb.h"
#include "rsafefs/fuse_rpc/grpc/channel.hpp"
#include "rsafefs/fuse_rpc/grpc/sync_client.hpp"
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>

namespace rsafefs::fuse_rpc::grpc
{

using CompletionQueue = ::grpc::CompletionQueue;

class async_client : public fuse_rpc::grpc::sync_client
{
public:
  struct config : fuse_rpc::grpc::sync_client::config {
    config(const std::string &server_address, size_t cache_size, size_t block_size,
           size_t threads, double flush_threshold)
        : sync_client::config(server_address)
        , cache_size_(cache_size)
        , block_size_(block_size)
        , threads_(threads)
        , flush_threshold_(flush_threshold)
    {
    }

    size_t cache_size_;
    size_t block_size_;
    size_t threads_;
    double flush_threshold_;
  };

  explicit async_client(async_client::config &config);

  ~async_client() override;

  int write(const char *path, const char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi) override;

  int flush(const char *path, struct fuse_file_info *fi) override;

  int fsync(const char *path, int isdatasync, struct fuse_file_info *fi) override;

private:
  static void async_complete(CompletionQueue &cq);

  void run_io_context();

  void scheduler();

  std::future<bool> schedule_flush();

  enum event { TERMINATED, CALL_ENDED, FLUSH_REQUEST };

  struct block {
    block(const char *path, std::unique_ptr<char[]> buf, size_t size, off_t offset,
          const struct fuse_file_info *fi);

    bool is_mergeable(block &block) const;

    const std::string path_;
    std::unique_ptr<char[]> buf_;
    const size_t size_;
    const off_t offset_;
    const struct fuse_file_info fi_;
  };

  struct call_data {
    call_data(const std::unique_ptr<fuse_grpc_proto::FuseOps::Stub> &stub,
              size_t max_block_size, CompletionQueue &cq, std::deque<block> &blocks,
              std::atomic<size_t> &cache_size, channel<event> *channel);

    void start();

    void end(bool status);

    void proceed(bool ok);

    void send_block();

    ::grpc::ClientContext context_;
    const size_t max_block_size_;
    std::deque<block> blocks_;
    std::atomic<size_t> &cache_size_;
    ::grpc::Status status_;
    enum call_status { CREATE, WRITE, WRITES_DONE, FINISHED };
    call_status call_status_;
    std::promise<bool> promise_;
    fuse_grpc_proto::WriteReply reply_;
    const std::unique_ptr<::grpc::ClientAsyncWriter<fuse_grpc_proto::WriteRequest>>
        writer_;
    int n_blocks_sent_;
    channel<event> *channel_;
  };

  grpc::async_client::config config_;

  std::deque<block> blocks_;
  std::recursive_mutex mtx_blocks_;

  std::deque<call_data *> calls_;
  std::mutex mtx_calls_;

  size_t blocks_queue_size_;
  std::atomic<size_t> cache_size_;

  channel<event> channel_;
  std::thread scheduler_;

  CompletionQueue cq_;
  std::vector<std::thread> cq_threads_;

  asio::io_context io_context_;
  std::thread io_context_thread_;
  asio::steady_timer timer_;
};

} // namespace rsafefs::fuse_rpc::grpc
