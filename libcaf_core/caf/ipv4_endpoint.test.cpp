// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#include "caf/ipv4_endpoint.hpp"

#include "caf/test/test.hpp"

#include "caf/actor_system.hpp"
#include "caf/actor_system_config.hpp"
#include "caf/binary_deserializer.hpp"
#include "caf/binary_serializer.hpp"
#include "caf/byte_buffer.hpp"
#include "caf/detail/parse.hpp"
#include "caf/ipv4_address.hpp"
#include "caf/span.hpp"

#include <cassert>
#include <cstddef>
#include <vector>

using namespace caf;

namespace {

ipv4_endpoint operator""_ep(const char* str, size_t size) {
  ipv4_endpoint result;
  if (auto err = detail::parse(std::string_view{str, size}, result))
    test::runnable::current().fail("unable to parse input: {}", err);
  return result;
}

struct fixture {
  actor_system_config cfg;
  actor_system sys{cfg};

  template <class T>
  T roundtrip(T x) {
    byte_buffer buf;
    binary_serializer sink(sys, buf);
    if (!sink.apply(x))
      test::runnable::current().fail("serialization failed: {}",
                                     sink.get_error());
    binary_deserializer source(sys, make_span(buf));
    T y;
    if (!source.apply(y))
      test::runnable::current().fail("deserialization failed: {}",
                                     source.get_error());
    return y;
  }
};

} // namespace

#define CHECK_TO_STRING(addr) check_eq(addr, to_string(addr##_ep))

#define CHECK_COMPARISON(addr1, addr2)                                         \
  check_gt(addr2##_ep, addr1##_ep);                                            \
  check_ge(addr2##_ep, addr1##_ep);                                            \
  check_ge(addr1##_ep, addr1##_ep);                                            \
  check_ge(addr2##_ep, addr2##_ep);                                            \
  check_eq(addr1##_ep, addr1##_ep);                                            \
  check_eq(addr2##_ep, addr2##_ep);                                            \
  check_le(addr1##_ep, addr2##_ep);                                            \
  check_le(addr1##_ep, addr1##_ep);                                            \
  check_le(addr2##_ep, addr2##_ep);                                            \
  check_ne(addr1##_ep, addr2##_ep);                                            \
  check_ne(addr2##_ep, addr1##_ep);

#define CHECK_SERIALIZATION(addr) check_eq(addr##_ep, roundtrip(addr##_ep))

WITH_FIXTURE(fixture) {

TEST("constructing assigning and hash_code") {
  const uint16_t port = 8888;
  auto addr = make_ipv4_address(127, 0, 0, 1);
  ipv4_endpoint ep1(addr, port);
  check_eq(ep1.address(), addr);
  check_eq(ep1.port(), port);
  ipv4_endpoint ep2;
  ep2.address(addr);
  ep2.port(port);
  check_eq(ep2.address(), addr);
  check_eq(ep2.port(), port);
  check_eq(ep1, ep2);
  check_eq(ep1.hash_code(), ep2.hash_code());
}

TEST("to string") {
  CHECK_TO_STRING("127.0.0.1:8888");
  CHECK_TO_STRING("192.168.178.1:8888");
  CHECK_TO_STRING("255.255.255.1:17");
  CHECK_TO_STRING("192.168.178.1:8888");
  CHECK_TO_STRING("127.0.0.1:111");
  CHECK_TO_STRING("123.123.123.123:8888");
  CHECK_TO_STRING("127.0.0.1:8888");
}

TEST("comparison") {
  CHECK_COMPARISON("127.0.0.1:8888", "127.0.0.2:8888");
  CHECK_COMPARISON("192.168.178.1:8888", "245.114.2.89:8888");
  CHECK_COMPARISON("188.56.23.97:1211", "189.22.36.0:1211");
  CHECK_COMPARISON("0.0.0.0:8888", "255.255.255.1:8888");
  CHECK_COMPARISON("127.0.0.1:111", "127.0.0.1:8888");
  CHECK_COMPARISON("192.168.178.1:8888", "245.114.2.89:8888");
  CHECK_COMPARISON("123.123.123.123:8888", "123.123.123.123:8889");
}

TEST("serialization") {
  CHECK_SERIALIZATION("127.0.0.1:8888");
  CHECK_SERIALIZATION("192.168.178.1:8888");
  CHECK_SERIALIZATION("255.255.255.1:17");
  CHECK_SERIALIZATION("192.168.178.1:8888");
  CHECK_SERIALIZATION("127.0.0.1:111");
  CHECK_SERIALIZATION("123.123.123.123:8888");
  CHECK_SERIALIZATION("127.0.0.1:8888");
}

} // WITH_FIXTURE(fixture)
