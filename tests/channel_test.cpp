#include "rsafefs/fuse_rpc/grpc/channel.hpp"
#include <gtest/gtest.h>

class ChannelTest : public ::testing::Test
{
protected:
  ChannelTest() {}
  ~ChannelTest() {}
  void SetUp() override {}
  void TearDown() override {}

  rsafefs::fuse_rpc::grpc::channel<int> channel_;
};

TEST_F(ChannelTest, IsEmptyInitially) { ASSERT_FALSE(channel_.has_pending_messages()); }

TEST_F(ChannelTest, SendWorks)
{
  channel_.send(1);
  channel_.send(2);
  channel_.send(3);
  channel_.send(4);

  EXPECT_TRUE(channel_.has_pending_messages());
}

TEST_F(ChannelTest, ReceiveWorks)
{
  channel_.send(1);
  channel_.send(2);
  channel_.send(3);
  channel_.send(4);

  EXPECT_EQ(channel_.n_pending_messages(), 4);

  EXPECT_EQ(channel_.receive(), 1);
  EXPECT_EQ(channel_.receive(), 2);
  EXPECT_EQ(channel_.receive(), 3);
  EXPECT_EQ(channel_.receive(), 4);

  EXPECT_EQ(channel_.n_pending_messages(), 0);
}
