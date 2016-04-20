// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2016 Preferred Networks and Nippon Telegraph and Telephone Corporation.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <msgpack.hpp>

#include "labels.hpp"
#include "../framework/packer.hpp"
#include "../framework/stream_writer.hpp"

namespace jubatus {
namespace core {
namespace storage {

TEST(labels, trivial) {
  labels labels;

  EXPECT_EQ(0u, labels.get().size());

  EXPECT_FALSE(labels.erase("hoge"));
  EXPECT_EQ(0u, labels.get().size());

  EXPECT_TRUE(labels.add("hoge"));
  EXPECT_EQ(1u, labels.get().size());
  EXPECT_EQ(1u, labels.get().count("hoge"));
  EXPECT_EQ(0ull, labels.get()["hoge"]);

  EXPECT_TRUE(labels.erase("hoge"));
  EXPECT_EQ(0u, labels.get().size());

  labels.increment("hoge");
  EXPECT_EQ(1u, labels.get().size());
  EXPECT_EQ(1u, labels.get().count("hoge"));
  EXPECT_EQ(1ull, labels.get()["hoge"]);

  labels.increment("hoge");
  EXPECT_EQ(1u, labels.get().size());
  EXPECT_EQ(1u, labels.get().count("hoge"));
  EXPECT_EQ(2ull, labels.get()["hoge"]);

  labels.add("fuga");
  labels.increment("fuga");
  labels.increment("foo");
  EXPECT_EQ(3u, labels.get().size());
  EXPECT_EQ(1u, labels.get().count("hoge"));
  EXPECT_EQ(2ull, labels.get()["hoge"]);
  EXPECT_EQ(1u, labels.get().count("fuga"));
  EXPECT_EQ(1ull, labels.get()["fuga"]);
  EXPECT_EQ(1u, labels.get().count("foo"));
  EXPECT_EQ(1ull, labels.get()["foo"]);

  labels.clear();
  EXPECT_EQ(0u, labels.get().size());
}

TEST(labels, duplicate) {
  labels labels;

  EXPECT_TRUE(labels.add("hoge"));
  EXPECT_FALSE(labels.add("hoge"));
  EXPECT_EQ(1u, labels.get().size());
}

TEST(labels, pack_unpack) {
  labels l1, l2;

  l1.add("hoge");
  l1.increment("fuga");
  l1.increment("fuga");

  msgpack::sbuffer buf;
  {
    framework::stream_writer<msgpack::sbuffer> sw(buf);
    framework::jubatus_packer jp(sw);
    framework::packer packer(jp);
    l1.pack(packer);
  }

  {
    msgpack::unpacked unpacked;
    msgpack::unpack(&unpacked, buf.data(), buf.size());
    l2.unpack(unpacked.get());
  }

  EXPECT_EQ(2u, l2.get().size());
  EXPECT_EQ(1u, l2.get().count("hoge"));
  EXPECT_EQ(0ull, l2.get()["hoge"]);
  EXPECT_EQ(1u, l2.get().count("fuga"));
  EXPECT_EQ(2ull, l2.get()["fuga"]);
}

TEST(labels, get_diff_and_merge_and_put_diff) {
  labels l1, l2, l3;

  l1.increment("hoge");
  l1.increment("fuga");

  l2.increment("fuga");
  l2.increment("foo");
  l2.increment("bar");

  labels::data_t d1, d2, d3;

  l1.get_diff(d1);
  l2.get_diff(d2);
  l3.get_diff(d3);

  EXPECT_EQ(2u, d1.size());
  EXPECT_EQ(3u, d2.size());
  EXPECT_EQ(0u, d3.size());

  labels::data_t mixed;

  l1.mix(d1, mixed);
  EXPECT_EQ(2u, mixed.size());

  l1.mix(d2, mixed);
  EXPECT_EQ(4u, mixed.size());

  l1.mix(d3, mixed);
  EXPECT_EQ(4u, mixed.size());

  l1.increment("fuga");  // This data will be removed
  l2.increment("baz");  // This data will be removed
  l3.increment("qux");  // This data will be removed

  l1.put_diff(mixed);
  l2.put_diff(mixed);
  l3.put_diff(mixed);

  EXPECT_EQ(4u, l1.get().size());
  EXPECT_EQ(1ull, l1.get()["hoge"]);
  EXPECT_EQ(2ull, l1.get()["fuga"]);
  EXPECT_EQ(1ull, l1.get()["foo"]);
  EXPECT_EQ(1ull, l1.get()["bar"]);

  EXPECT_EQ(4u, l2.get().size());
  EXPECT_EQ(1ull, l2.get()["hoge"]);
  EXPECT_EQ(2ull, l2.get()["fuga"]);
  EXPECT_EQ(1ull, l2.get()["foo"]);
  EXPECT_EQ(1ull, l2.get()["bar"]);

  EXPECT_EQ(4u, l3.get().size());
  EXPECT_EQ(1ull, l3.get()["hoge"]);
  EXPECT_EQ(2ull, l3.get()["fuga"]);
  EXPECT_EQ(1ull, l3.get()["foo"]);
  EXPECT_EQ(1ull, l3.get()["bar"]);
}

}  // namespace storage
}  // namespace core
}  // namespace jubatus
