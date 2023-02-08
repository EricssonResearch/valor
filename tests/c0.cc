#include "valet.h"
#include <gtest/gtest.h>
#include <clang/Tooling/Tooling.h>

TEST(dummy, just_check) {
  EXPECT_TRUE(clang::tooling::runToolOnCode(std::make_unique<valor::valet>(), "class X {};"));
}