// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#include <map>
#include <string>

#include <gtest/gtest.h>

#include "assoc_vector.hpp"

namespace jubatus {
namespace core {
namespace common {

TEST(assoc_vector, trivial) {
  assoc_vector<int, int> m;
  m[10] = 5;
  m[4] = 2;
  m[12] = 6;
  m[10] = 4;

  EXPECT_EQ(3u, m.size());
  ASSERT_EQ(4, m[10]);
  ASSERT_EQ(2, m[4]);

  m.erase(4);
  ASSERT_EQ(0u, m.count(4));
}

TEST(assoc_vector, replace) {
  assoc_vector<int, int> m;
  m[1] = -1;
  m[1] = 3;
  ASSERT_EQ(1u, m.size());
  ASSERT_EQ(1u, m.count(1));
  EXPECT_EQ(3, m[1]);
}

TEST(assoc_vector, random) {
  std::map<int, int> map;
  assoc_vector<int, int> vec;

  srand(0);
  for (int i = 0; i < 1000; ++i) {
    int k = rand() % 1000 * (rand() % 2 == 0 ? 1 : -1);
    int v = rand() % 1000 * (rand() % 2 == 0 ? 1 : -1);
    map[k] = v;
    vec[k] = v;
  }

  EXPECT_EQ(map.size(), vec.size());
  for (std::map<int, int>::const_iterator it = map.begin();
       it != map.end(); ++it) {
    ASSERT_EQ(1u, vec.count(it->first));
    EXPECT_EQ(it->second, vec[it->first]);
  }
}

TEST(assoc_vector, pack) {
  // Pack format of assoc_vector must to be same as std::map

  assoc_vector<std::string, int> v;
  v["saitama"] = 1;

  msgpack::sbuffer buf;
  msgpack::pack(buf, v);

  msgpack::unpacked unpacked;
  msgpack::unpack(&unpacked, buf.data(), buf.size());
  msgpack::object obj = unpacked.get();

  std::map<std::string, int> m;
  m["saitama"] = 1;
  obj.convert(&m);

  ASSERT_EQ(1u, m.size());
  ASSERT_EQ(1u, m.count("saitama"));
  EXPECT_EQ(1, m["saitama"]);
}

TEST(assoc_vector, unpack) {
  // Pack format of assoc_vector must to be same as std::map

  std::map<std::string, int> m;
  m["saitama"] = 1;

  msgpack::sbuffer buf;
  msgpack::pack(buf, m);

  msgpack::unpacked unpacked;
  msgpack::unpack(&unpacked, buf.data(), buf.size());
  msgpack::object obj = unpacked.get();

  assoc_vector<std::string, int> v;
  obj.convert(&v);

  ASSERT_EQ(1u, v.size());
  ASSERT_EQ(1u, v.count("saitama"));
  EXPECT_EQ(1, v["saitama"]);
}

}  // namespace common
}  // namespace core
}  // namespace jubatus
