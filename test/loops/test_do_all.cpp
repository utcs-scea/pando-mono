
#include <gtest/gtest.h>

#include <pando-rt/export.h>

#include <variant>

#include "pando-rt/pando-rt.hpp"
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/sync/notification.hpp>

TEST(doALL, SimpleCopy) {
  pando::Status err;
  pando::Notification necessary;
  err = necessary.init();
  EXPECT_EQ(err, pando::Status::Success);
  auto f = +[](pando::Notification::HandleType done) {
    auto const size = 10;
    pando::Vector<std::uint64_t> src;
    pando::Status err;

    err = src.initialize(size);
    EXPECT_EQ(err, pando::Status::Success);
    for (std::uint64_t i; i < src.size(); i++) {
      src[i] = i;
    }
    auto plusOne = +[](pando::GlobalRef<std::uint64_t> v) {
      v = v + 1;
    };
    galois::doAll(src, plusOne);
    EXPECT_EQ(src.size(), size);
    for (std::uint64_t i = 0; i < size; i++) {
      EXPECT_EQ(src[i], i + 1);
    }
    auto plusN = +[](std::uint64_t n, pando::GlobalRef<std::uint64_t> v) {
      v = v + n;
    };
    const std::uint64_t N = 10;

    galois::doAll(N, src, plusN);

    for (std::uint64_t i = 0; i < size; i++) {
      EXPECT_EQ(src[i], i + 11);
    }

    done.notify();
  };

  err = pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore}, f,
                         necessary.getHandle());
  EXPECT_EQ(err, pando::Status::Success);
  necessary.wait();
}
