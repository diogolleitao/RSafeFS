#include "rsafefs/layers/local/local.hpp"
#include <gtest/gtest.h>

using namespace rsafefs;

TEST(LocalTest, EmptyConfig)
{
  YAML::Node config = YAML::Load("");
  ASSERT_THROW(std::make_unique<local_config>(config), local_wrong_config_exception);
}

TEST(LocalTest, ValidConfig)
{
  YAML::Node config = YAML::Load("{path: /tmp, mode: local}");
  ASSERT_NO_THROW(std::make_unique<local_config>(config));
}

TEST(LocalTest, PathInvalid)
{
  YAML::Node config = YAML::Load("{path: /really_awkward_path, mode: local}");
  ASSERT_THROW(std::make_unique<local_config>(config), local_wrong_config_exception);
}

TEST(LocalTest, WrongMode)
{
  YAML::Node config = YAML::Load("{path: /tmp, mode: invalid_mode}");
  ASSERT_THROW(std::make_unique<local_config>(config), local_wrong_config_exception);
}