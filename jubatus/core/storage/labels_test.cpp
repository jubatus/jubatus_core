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

  EXPECT_EQ(0u, labels.size());
  EXPECT_EQ(0u, labels.erase("hoge"));

  EXPECT_TRUE(labels.insert("hoge").second);

  EXPECT_EQ(1u, labels.size());
  EXPECT_EQ(1u, labels.erase("hoge"));
  EXPECT_EQ(0u, labels.size());

  labels.insert("fuga");
  labels.insert("foo");

  EXPECT_EQ(2u, labels.size());

  labels.clear();

  EXPECT_EQ(0u, labels.size());
}

TEST(labels, duplicate) {
  labels labels;

  labels.insert("hoge");

  EXPECT_FALSE(labels.insert("hoge").second);
  EXPECT_EQ(1u, labels.size());
}

TEST(labels, pack_unpack) {
  labels l1, l2;

  l1.insert("hoge");

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

  EXPECT_EQ(1u, l2.size());
  EXPECT_EQ(1u, l2.erase("hoge"));
}

TEST(labels, get_put_merge) {
  labels l1, l2, l3;

  l1.insert("hoge");
  l1.insert("fuga");

  l2.insert("fuga");
  l2.insert("foo");
  l2.insert("bar");

  data_t d1, d2, d3;

  d1 = l1.get();
  d2 = l2.get();
  d3 = l3.get();

  EXPECT_EQ(2u, d1.size());
  EXPECT_EQ(3u, d2.size());
  EXPECT_EQ(0u, d3.size());

  data_t merged;

  l1.merge(d1, merged);
  EXPECT_EQ(2u, merged.size());

  l1.merge(d2, merged);
  EXPECT_EQ(4u, merged.size());

  l1.merge(d3, merged);
  EXPECT_EQ(4u, merged.size());

  l1.put(merged);
  l2.put(merged);
  l3.put(merged);

  EXPECT_EQ(4u, l1.size());
  EXPECT_EQ(4u, l2.size());
  EXPECT_EQ(4u, l3.size());
}

TEST(mixable_labels, linear_mix) {
  mixable_labels::model_ptr m1, m2, m3;
  m1.reset(new labels());
  m2.reset(new labels());
  m3.reset(new labels());

  std::vector<mixable_labels> mixables;

  mixable_labels l1(m1), l2(m2), l3(m3);

  l1.get_model()->insert("hoge");
  l1.get_model()->insert("fuga");
  mixables.push_back(l1);

  l2.get_model()->insert("fuga");
  l2.get_model()->insert("foo");
  l2.get_model()->insert("bar");
  mixables.push_back(l2);

  mixables.push_back(l3);

  msgpack::sbuffer buf;
  framework::stream_writer<msgpack::sbuffer> sw(buf);
  framework::jubatus_packer jp(sw);
  framework::packer packer(jp);
  packer.pack_array(mixables.size());
  for (size_t i = 0; i < mixables.size(); i++) {
    mixables[i].get_diff(packer);
  }

  msgpack::unpacked unpacked;
  msgpack::unpack(&unpacked, buf.data(), buf.size());
  msgpack::object obj = unpacked.get();

  framework::diff_object mixed;
  mixed = mixables[0].convert_diff_object(obj.via.array.ptr[0]);
  for (size_t i = 1; i < mixables.size(); i++) {
    mixables[i].mix(obj.via.array.ptr[i], mixed);
  }

  for (size_t i = 0; i < mixables.size(); i++) {
    mixables[i].put_diff(mixed);
  }

  EXPECT_EQ(4u, l1.get_model()->size());
  EXPECT_EQ(4u, l2.get_model()->size());
  EXPECT_EQ(4u, l3.get_model()->size());
}

TEST(mixable_labels, push_mix) {
  mixable_labels::model_ptr m1, m2;
  m1.reset(new labels());
  m2.reset(new labels());

  mixable_labels l1(m1), l2(m2);

  l1.get_model()->insert("hoge");
  l1.get_model()->insert("fuga");

  l2.get_model()->insert("fuga");
  l2.get_model()->insert("foo");
  l2.get_model()->insert("bar");

  msgpack::sbuffer buf1, buf2, buf3, buf4;

  {
    framework::stream_writer<msgpack::sbuffer> sw(buf1);
    framework::jubatus_packer jp(sw);
    framework::packer packer(jp);
    l1.get_argument(packer);
  }

  {
    msgpack::unpacked unpacked;
    msgpack::unpack(&unpacked, buf1.data(), buf1.size());
    msgpack::object obj = unpacked.get();
    framework::stream_writer<msgpack::sbuffer> sw(buf2);
    framework::jubatus_packer jp(sw);
    framework::packer packer(jp);
    l2.pull(obj, packer);
  }

  {
    framework::stream_writer<msgpack::sbuffer> sw(buf3);
    framework::jubatus_packer jp(sw);
    framework::packer packer(jp);
    l2.get_argument(packer);
  }

  {
    msgpack::unpacked unpacked;
    msgpack::unpack(&unpacked, buf3.data(), buf3.size());
    msgpack::object obj = unpacked.get();
    framework::stream_writer<msgpack::sbuffer> sw(buf4);
    framework::jubatus_packer jp(sw);
    framework::packer packer(jp);
    l1.pull(obj, packer);
  }

  {
    msgpack::unpacked unpacked;
    msgpack::unpack(&unpacked, buf4.data(), buf4.size());
    msgpack::object obj = unpacked.get();
    l2.push(obj);
  }

  {
    msgpack::unpacked unpacked;
    msgpack::unpack(&unpacked, buf2.data(), buf2.size());
    msgpack::object obj = unpacked.get();
    l1.push(obj);
  }

  EXPECT_EQ(4u, l1.get_model()->size());
  EXPECT_EQ(4u, l2.get_model()->size());
}

}  // namespace storage
}  // namespace core
}  // namespace jubatus
