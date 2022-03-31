#include "rsafefs/fuse_rpc/grpc/async_client.hpp"
#include "rsafefs/fuse_rpc/grpc/structs_fillers.hpp"
#include "rsafefs/utils/logging.hpp"

namespace rsafefs::fuse_rpc::grpc
{

async_client::async_client(async_client::config &config)
    : sync_client(config)
    , config_(config)
    , blocks_queue_size_(0)
    , cache_size_(0)
    , scheduler_(&async_client::scheduler, this)
    , io_context_thread_(&async_client::run_io_context, this)
    , timer_(io_context_)
{
  config_.threads_ = std::max(1UL, config_.threads_);
  config_.flush_threshold_ =
      config_.cache_size_ * std::min(1.0, std::abs(config_.flush_threshold_));

  for (size_t i = 0; i < config_.threads_; i++) {
    cq_threads_.emplace_back(&async_client::async_complete, std::ref(cq_));
  }
}

async_client::~async_client()
{
  schedule_flush().get();
  channel_.send(event::TERMINATED);
  scheduler_.join();

  cq_.Shutdown();
  for (auto &thread : cq_threads_)
    thread.join();

  io_context_.stop();
  io_context_thread_.join();
}

int
async_client::write(const char *path, const char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
  auto buffer = std::make_unique<char[]>(size);
  std::memcpy(buffer.get(), buf, size);

  std::unique_lock lock(mtx_blocks_);
  timer_.cancel();

  if (cache_size_ > config_.cache_size_) {
    schedule_flush().get();
  } else if (blocks_queue_size_ > config_.flush_threshold_) {
    schedule_flush();
  }

  blocks_.emplace_back(path, std::move(buffer), size, offset, fi);
  blocks_queue_size_ += size;
  cache_size_ += size;

  timer_.expires_after(std::chrono::seconds(1));
  timer_.async_wait([this](const asio::error_code &e) {
    if (e != asio::error::operation_aborted) {
      schedule_flush();
    }
  });

  return size;
}

int
async_client::flush(const char *path, struct fuse_file_info *fi)
{
  schedule_flush().get();
  return sync_client::flush(path, fi);
}

int
async_client::fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
  schedule_flush().get();
  return sync_client::fsync(path, isdatasync, fi);
}

void
async_client::async_complete(CompletionQueue &cq)
{
  void *got_tag;
  bool ok;
  while (cq.Next(&got_tag, &ok)) {
    static_cast<call_data *>(got_tag)->proceed(ok);
  }
}

void
async_client::run_io_context()
{
  asio::executor_work_guard work = asio::make_work_guard(io_context_);
  io_context_.run();
}

void
async_client::scheduler()
{
  bool terminated = false;
  bool flushing = false;

  while (!terminated) {
    event event = channel_.receive();
    std::unique_lock<std::mutex> lockCalls(mtx_calls_);

    switch (event) {
    case CALL_ENDED:
      flushing = false;
      if (!calls_.empty()) {
        call_data *call = calls_.front();
        calls_.pop_front();
        call->start();
        flushing = true;
      }
      break;
    case FLUSH_REQUEST:
      if (!calls_.empty() && !flushing) {
        call_data *call = calls_.front();
        calls_.pop_front();
        call->start();
        flushing = true;
      }
      break;
    case TERMINATED:
      if (channel_.has_pending_messages()) {
        channel_.send(event::TERMINATED);
      } else {
        terminated = true;
      }
      break;
    }
  }
}

std::future<bool>
async_client::schedule_flush()
{
  std::unique_lock lock(mtx_blocks_);

  auto call =
      new call_data(stub_, config_.block_size_, cq_, blocks_, cache_size_, &channel_);
  std::future<bool> future = call->promise_.get_future();
  blocks_queue_size_ = 0;

  std::unique_lock lock_calls(mtx_calls_);
  calls_.push_back(call);
  channel_.send(event::FLUSH_REQUEST);
  lock_calls.unlock();

  return future;
}

async_client::block::block(const char *path, std::unique_ptr<char[]> buf, size_t size,
                           off_t offset, const struct fuse_file_info *fi)
    : path_(path)
    , buf_(std::move(buf))
    , size_(size)
    , offset_(offset)
    , fi_(*fi)
{
}

bool
async_client::block::is_mergeable(async_client::block &block) const
{
  return path_ == block.path_ &&
         (offset_ + size_) == static_cast<unsigned long>(block.offset_) &&
         fi_.direct_io == block.fi_.direct_io && fi_.fh == block.fi_.fh &&
         fi_.fh_old == block.fi_.fh_old && fi_.flags == block.fi_.flags &&
         fi_.flock_release == block.fi_.flock_release && fi_.flush == block.fi_.flush &&
         fi_.keep_cache == block.fi_.keep_cache &&
         fi_.lock_owner == block.fi_.lock_owner &&
         fi_.nonseekable == block.fi_.nonseekable && fi_.padding == block.fi_.padding &&
         fi_.writepage == block.fi_.writepage;
}

async_client::call_data::call_data(
    const std::unique_ptr<fuse_grpc_proto::FuseOps::Stub> &stub,
    const size_t max_block_size, CompletionQueue &cq, std::deque<block> &blocks,
    std::atomic<size_t> &cache_size, channel<event> *channel)
    : max_block_size_(max_block_size)
    , blocks_(std::move(blocks))
    , cache_size_(cache_size)
    , call_status_(CREATE)
    , writer_(stub->PrepareAsyncACStreamWrite(&context_, &reply_, &cq))
    , n_blocks_sent_(0)
    , channel_(channel)
{
  blocks = std::deque<block>();
}

void
async_client::call_data::start()
{
  if (blocks_.empty()) {
    end(true);
  } else {
    writer_->StartCall(this);
  }
}

void
async_client::call_data::end(bool status)
{
  channel_->send(event::CALL_ENDED);
  promise_.set_value(status);
  delete this;
}

void
async_client::call_data::proceed([[maybe_unused]] bool ok)
{
  switch (call_status_) {
  case CREATE: {
    if (blocks_.empty()) {
      call_status_ = FINISHED;
      writer_->Finish(&status_, this);
    } else {
      call_status_ = WRITE;
      send_block();
    }
    break;
  }
  case WRITE: {
    for (int i = 0; i < n_blocks_sent_; i++) {
      block &block = blocks_.front();
      cache_size_ -= block.size_;
      blocks_.pop_front();
    }
    n_blocks_sent_ = 0;

    if (blocks_.empty()) {
      call_status_ = WRITES_DONE;
      writer_->WritesDone(this);
    } else {
      call_status_ = WRITE;
      send_block();
    }
    break;
  }
  case WRITES_DONE: {
    call_status_ = FINISHED;
    writer_->Finish(&status_, this);
    break;
  }
  default: {
    GPR_ASSERT(call_status_ == FINISHED);
    if (!status_.ok()) {
      logging::critical("[async stream write] [{}]", status_.error_message());
    }
    end(status_.ok());
  }
  }
}

void
async_client::call_data::send_block()
{
  block &first_block = blocks_.front();
  fuse_grpc_proto::WriteRequest request;
  request.set_path(first_block.path_);
  request.set_offset(first_block.offset_);
  fill_StructFuseFileInfo(request.mutable_info(), &first_block.fi_);

  size_t n_blocks = blocks_.size();
  n_blocks_sent_ = 1;

  if (n_blocks > 1) {
    block *last_block = &first_block;
    size_t bufSize = last_block->size_;

    for (size_t i = 1; i < n_blocks; i++) {
      block &current_block = blocks_[i];
      if (bufSize + current_block.size_ <= max_block_size_ &&
          last_block->is_mergeable(current_block)) {
        bufSize += current_block.size_;
        last_block = &current_block;
        n_blocks_sent_++;
      } else {
        break;
      }
    }

    std::string *buf = request.mutable_buf();
    buf->resize(bufSize);
    char *buf_ptr = buf->data();

    size_t n_copied_bytes = 0;
    for (int i = 0; i < n_blocks_sent_; i++) {
      block &block_to_send = blocks_[i];
      std::memcpy(buf_ptr + n_copied_bytes, block_to_send.buf_.get(),
                  block_to_send.size_);
      n_copied_bytes += block_to_send.size_;
    }
    request.set_size(bufSize);
  } else {
    request.set_buf(first_block.buf_.get(), first_block.size_);
    request.set_size(first_block.size_);
  }

  writer_->Write(request, this);
}

} // namespace rsafefs::fuse_rpc::grpc