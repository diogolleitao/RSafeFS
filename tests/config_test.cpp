#include "rsafefs/config.hpp"
#include <filesystem>
#include <gtest/gtest.h>

TEST(ConfigTest, InvalidConfigs)
{
  for (const auto &conf :
       std::filesystem::directory_iterator("../../tests/invalid_config_examples")) {
    std::string configuration_file = conf.path();
    EXPECT_THROW(rsafefs::config::make(configuration_file),
                 rsafefs::wrong_config_exception);
  }
}

TEST(ConfigTest, ValidConfigs)
{
  for (const auto &conf : std::filesystem::directory_iterator("../../config_examples")) {
    std::string configuration_file = conf.path();
    EXPECT_NO_THROW(rsafefs::config::make(configuration_file));
  }
}
