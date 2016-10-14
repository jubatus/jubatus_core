// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2013 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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
#include <vector>
#include <string>
#include <gtest/gtest.h>
#include "jubatus/util/lang/scoped_ptr.h"
#include "../common/type.hpp"
#include "../framework/stream_writer.hpp"
#include "storage.hpp"
#include "storage_factory.hpp"
#include "testutil.hpp"

namespace jubatus {
namespace core {
namespace clustering {

namespace {

class make_case_type {
 public:
  make_case_type& operator()(const string& key, const string& value) {
    cases_.insert(make_pair(key, value));
    return *this;
  }

  std::map<string, string> operator()() {
    std::map<string, string> ret;
    ret.swap(cases_);
    return ret;
  }

 private:
  std::map<string, string> cases_;
} make_case;

}  // namespace

class storage_test
    : public testing::TestWithParam<std::map<std::string, std::string> > {
 protected:
  typedef jubatus::util::lang::shared_ptr<storage> storage_ptr;
  std::string name;
  std::string method;
  clustering_config conf;

  void SetUp() {
    name = "test";
    std::map<std::string, std::string> param = GetParam();
    method = param["method"];
    conf.compressor_method = param["compressor_method"];
  }
};

TEST_P(storage_test, pack_unpack) {
  storage_ptr s = storage_factory::create(name, method, conf);
  ASSERT_TRUE(s != NULL);
  for (size_t i = 0; i < 10; ++i) {
    s->add(get_point(3));
  }

  // pack
  msgpack::sbuffer buf;
  {
    framework::stream_writer<msgpack::sbuffer> st(buf);
    framework::jubatus_packer jp(st);
    framework::packer packer(jp);
    s->pack(packer);
  }

  // unpack
  storage_ptr s2 = storage_factory::create(name, method, conf);
  ASSERT_TRUE(s2 != NULL);
  {
    msgpack::unpacked unpacked;
    msgpack::unpack(&unpacked, buf.data(), buf.size());
    s2->unpack(unpacked.get());
  }

  EXPECT_EQ(s->get_revision(), s2->get_revision());

  {
    wplist all1 = s->get_all(), all2 = s2->get_all();

    ASSERT_EQ(all1.size(), all2.size());
    for (size_t i = 0; i < all1.size(); ++i) {
      EXPECT_EQ(all1[i].weight, all2[i].weight);
      EXPECT_EQ(all1[i].data, all2[i].data);
      // EXPECT_EQ(all1[i].original, all2[i].original);
    }
  }
}

const std::map<std::string, std::string> test_cases[] = {
#ifdef JUBATUS_USE_EIGEN
  make_case("method", "gmm")
    ("compressor_method", "compressive")
    ("result", "true")(),
  make_case("method", "gmm")
    ("compressor_method", "simple")
    ("result", "true")(),
#endif
  make_case("method", "kmeans")
    ("compressor_method", "compressive")
    ("result", "true")(),
  make_case("method", "kmeans")
    ("compressor_method", "simple")
    ("result", "true")()
};

INSTANTIATE_TEST_CASE_P(
    storage_test_instance,
    storage_test,
    ::testing::ValuesIn(test_cases));

}  // namespace clustering
}  // namespace core
}  // namespace jubatus
