// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "jubatus/util/math/random.h"
#include "jubatus/util/lang/cast.h"
#include "jubatus/util/lang/shared_ptr.h"

#include "wordcount_factory.hpp"
#include "space_saving.hpp"
#include "../common/exception.hpp"
#include "../common/jsonconfig.hpp"

using std::pair;
using std::string;
using std::vector;
using std::make_pair;
using jubatus::util::text::json::to_json;
using jubatus::util::lang::lexical_cast;
using jubatus::util::lang::shared_ptr;
using jubatus::core::common::sfv_t;
using jubatus::util::data::unordered_map;

namespace jubatus {
namespace core {
namespace wordcount {

namespace {

const string charactors =
  "abcdefghijklmnopqrstuvwxyz"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "1234567890";

sfv_t generate_sfv(int length, int seed = 0) {
  jubatus::util::math::random::mtrand rand(seed);
  sfv_t ret;
  for (int i = 0; i < length; ++i) {
    string key = "       ";
    for (int j = 0; j < 6; ++j) {
      key[j] = charactors[rand() % charactors.size()];
    }
    ret.push_back(make_pair(key, 1));
  }
  return ret;
}

}  // anonymous namespace

template<typename T>
class wordcount_test : public testing::Test {
public:
  void SetUp() {
  }
  void TearDown() {
  }
};

TYPED_TEST_CASE_P(wordcount_test);

TYPED_TEST_P(wordcount_test, trivial) {
  wordcount_config conf;
  TypeParam wc(conf);
}

TYPED_TEST_P(wordcount_test, append) {
  wordcount_config conf;
  TypeParam wc(conf);
  for (int i = 0; i < 10; ++i) {
    wc.append("a", generate_sfv(10));
  }
}

TYPED_TEST_P(wordcount_test, get_ranking) {
  wordcount_config conf;
  TypeParam wc(conf);
  for (int i = 0; i < 10; ++i) {
    wc.append("a", generate_sfv(10));
  }
  vector<pair<std::string, size_t> > ret
    = wc.get_ranking("a", 100);
  ASSERT_EQ(10, ret.size());
}

TYPED_TEST_P(wordcount_test, count) {
  wordcount_config conf;
  TypeParam wc(conf);
  sfv_t sfv = generate_sfv(10, 0);
  sfv_t nsfv = generate_sfv(10, 1);  // different seed

  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < sfv.size(); ++j) {
      ASSERT_EQ(i, wc.count("a", sfv[j].first));
    }
    wc.append("a", sfv);
    for (int j = 0; j < sfv.size(); ++j) {
      // must be incremented
      ASSERT_EQ(i + 1, wc.count("a", sfv[j].first));
    }
  }
  for (int i = 0; i < sfv.size(); ++i) {
    ASSERT_EQ(10, wc.count("a", sfv[i].first));
    ASSERT_EQ(0, wc.count("a", nsfv[i].first));
  }
}

TYPED_TEST_P(wordcount_test, multibucket_count) {
  wordcount_config conf;
  TypeParam wc(conf);
  for (int j = 0; j < 10; ++j) {
    sfv_t sfv = generate_sfv(10, j);
    for (int i = 0; i < 10; ++i) {
      wc.append(lexical_cast<string>(j), sfv);
    }
    for (int i = 0; i < sfv.size(); ++i) {
      ASSERT_EQ(10, wc.count(lexical_cast<string>(j), sfv[i].first));
    }
  }
}

TYPED_TEST_P(wordcount_test, smallbucket) {
  wordcount_config conf;
  const size_t capacity = 20;
  conf.capacity = capacity;
  TypeParam wc(conf);
  sfv_t sample;
  sample.push_back(make_pair("b", 1));
  wc.append("a", sample);
  for (int i = 0; i < 100; ++i) {
    sfv_t sfv = generate_sfv(capacity, i);
    wc.append("a", sfv);
    ASSERT_EQ(0u, wc.count("a", "b"));
    for (int j = 0; j < sfv.size(); ++j) {
      ASSERT_LT(0, wc.count("a", sfv[j].first));
    }
  }
}

TYPED_TEST_P(wordcount_test, ranking) {
  wordcount_config conf;
  conf.capacity = 1000;
  TypeParam wc(conf);
  sfv_t sample;
  sample.push_back(make_pair("b", 1));
  wc.append("a", sample);
  for (int i = 0; i < 10; ++i) {
    sfv_t sfv = generate_sfv(10, i);
    for (int j = 0; j < i + 1; ++j) {
      wc.append("a", sfv);
    }
  }
  const std::vector<std::pair<std::string, size_t> > ret =
    wc.get_ranking("a", 100);
  ASSERT_EQ(100, ret.size());

  int idx = 0;
  for (int i = 0; i < 10; ++i) {
    sfv_t sfv = generate_sfv(10, 9 - i);
    for (size_t j = 0; j < sfv.size(); ++j) {
      ASSERT_EQ(10 - i, ret[idx].second);
      ++idx;
    }
  }
}

REGISTER_TYPED_TEST_CASE_P(
    wordcount_test,
    trivial,
    append,
    get_ranking,
    count,
    multibucket_count,
    smallbucket, ranking);

typedef testing::Types<space_saving> wordcount_types;

INSTANTIATE_TYPED_TEST_CASE_P(wc, wordcount_test, wordcount_types);

}  // namespace wordcount
}  // namespace core
}  // namespace jubatus
