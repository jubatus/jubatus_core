// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2015 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <map>

#include <gtest/gtest.h>

#include "jubatus/util/lang/cast.h"
#include "jubatus/util/text/json.h"
#include "jubatus/util/math/random.h"

#include "../wordcount/wordcount_factory.hpp"
#include "../wordcount/wordcount_base.hpp"
#include "../wordcount/space_saving.hpp"
#include "../fv_converter/datum_to_fv_converter.hpp"
#include "../fv_converter/datum.hpp"
#include "../fv_converter/string_feature_factory.hpp"
#include "../framework/stream_writer.hpp"
#include "wordcount.hpp"

#include "test_util.hpp"

using std::string;
using std::vector;
using std::pair;
using std::map;
using std::make_pair;
using std::isfinite;
using std::numeric_limits;
using std::cout;
using std::endl;

using jubatus::util::lang::lexical_cast;
using jubatus::util::lang::shared_ptr;
using jubatus::util::data::optional;
using jubatus::core::fv_converter::datum;
using jubatus::core::fv_converter::datum_to_fv_converter;
using jubatus::core::wordcount::wordcount_base;

namespace jubatus {
namespace core {
namespace driver {

namespace {

shared_ptr<datum_to_fv_converter>  // NOLINT
  make_ngram_converter(int n) {

  fv_converter::converter_config cc;
  {
    fv_converter::param_t unigram_conf;
    unigram_conf.insert(make_pair("method", "ngram"));
    unigram_conf.insert(make_pair("char_num", lexical_cast<string>(n)));
    cc.string_types = map<string, fv_converter::param_t>();
    cc.string_types->insert(make_pair("unigram", unigram_conf));
  }

  {
    fv_converter::string_rule sr;
    sr.key = "*";
    sr.type = "unigram";
    sr.sample_weight = "tf";
    sr.global_weight = "bin";
    cc.string_rules = vector<fv_converter::string_rule>();
    cc.string_rules->push_back(sr);
  }

  return make_fv_converter(cc, NULL);
}

}

class wordcount_test
  : public ::testing::TestWithParam<shared_ptr<wordcount_base> > {
protected:
  void SetUp() {
    wordcount_.reset(new driver::wordcount(GetParam(),
                                           make_ngram_converter(1)));
  }

  void TearDown() {
    wordcount_.reset();
  }

  shared_ptr<core::driver::wordcount> wordcount_;
};

TEST_P(wordcount_test, simple) {
  datum d;
  d.string_values_.push_back(make_pair("split", "abcdefghijk"));
  wordcount_->append("hoge", d);
}

TEST_P(wordcount_test, split_count) {
  datum d;
  d.string_values_.push_back(make_pair("split", "abc"));
  vector<pair<string, size_t> > ret = wordcount_->split_test(d);
  ASSERT_EQ(3, ret.size());
}

TEST_P(wordcount_test, split) {
  datum d;
  d.string_values_.push_back(make_pair("split", "abcdefghijk"));
  vector<pair<string, size_t> > ret = wordcount_->split_test(d);

  for (size_t i = 0; i < ret.size(); ++i) {
    EXPECT_EQ(1, ret[i].first.size());
    EXPECT_LE('a', ret[i].first[0]);
    EXPECT_GE('a' + ret.size(), ret[i].first[0]);
  }
}

TEST_P(wordcount_test, ranking) {
  datum d1;
  d1.string_values_.push_back(make_pair("split", "abcdefghij"));
  wordcount_->append("a", d1);
  {
    datum d2;
    d1.string_values_.push_back(make_pair("split", "klmnopqrstu"));
    for (int i = 0; i < 100; ++i) {
      wordcount_->append("a", d2);
    }
  }
  vector<pair<string, size_t> > ret = wordcount_->get_ranking("a", 10);
  for (size_t i = 0; i < ret.size(); ++i) {
    EXPECT_EQ(1, ret[i].first.size());
    EXPECT_LE('a', ret[i].first[0]);
    EXPECT_GE('a' + ret.size(), ret[i].first[0]);
  }

}

vector<shared_ptr<wordcount_base> > create_wordcounts() {
  vector<shared_ptr<wordcount_base> > method;
  core::wordcount::wordcount_config config;
  config.capacity = 10;

  method.push_back(shared_ptr<wordcount_base>(
      new core::wordcount::space_saving(config)));

  return method;
}

INSTANTIATE_TEST_CASE_P(wordcount_test_instance,
    wordcount_test,
    testing::ValuesIn(create_wordcounts()));


}  // driver namespace
}  // core namespace
}  // jubatus namespace
