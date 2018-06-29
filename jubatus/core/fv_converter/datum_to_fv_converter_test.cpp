// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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
#include <cmath>
#include <string>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include "jubatus/util/lang/shared_ptr.h"
#include "jubatus/util/text/json.h"
#include "binary_feature.hpp"
#include "combination_feature_impl.hpp"
#include "character_ngram.hpp"
#include "converter_config.hpp"
#include "datum_to_fv_converter.hpp"
#include "datum.hpp"
#include "exact_match.hpp"
#include "exception.hpp"
#include "match_all.hpp"
#include "num_feature_impl.hpp"
#include "num_filter_impl.hpp"
#include "regexp_filter.hpp"
#include "prefix_match.hpp"
#include "space_splitter.hpp"
#include "weight_manager.hpp"
#include "without_split.hpp"

using jubatus::util::lang::lexical_cast;
using jubatus::util::lang::shared_ptr;

namespace jubatus {
namespace core {
namespace fv_converter {

TEST(datum_to_fv_converter, trivial) {
  datum_to_fv_converter conv;
}

// export GTEST_ALSO_RUN_DISABLED_TESTS=1 to run this
TEST(datum_to_fv_converter, DISABLED_benchmark) {
  size_t key_count = 100;  // number of keys in Datum
  size_t bench_count = 1000;  // number of `convert_and_update_weight` calls
  term_weight_type global_weight = TERM_BINARY;  // type of global_weight

  const std::string& sentence = "あいうえおかきくけこ"
                                "さしすせそたちつてと"
                                "なにぬねのはひふへほ"
                                "ABCD";  // 100 bytes

  shared_ptr<key_matcher> match(new match_all());
  shared_ptr<word_splitter> s(new character_ngram(3));
  std::vector<splitter_weight_type> p;
  p.push_back(splitter_weight_type(FREQ_BINARY, global_weight));

  datum_to_fv_converter conv;
  conv.register_string_rule("trigram", match, s, p);

  datum datum;
  for (size_t i = 0; i < key_count; ++i) {
    datum.string_values_.push_back(std::make_pair(
        std::string("text") + lexical_cast<std::string>(i), sentence));
  }

  for (size_t i = 0; i < bench_count; ++i) {
    std::vector<std::pair<std::string, double> > feature;
    conv.convert_and_update_weight(datum, feature);
  }
}

TEST(datum_to_fv_converter, num_feature) {
  datum datum;
  datum.num_values_.push_back(std::make_pair("/val1", 1.1));
  datum.num_values_.push_back(std::make_pair("/val2", 0.));

  datum_to_fv_converter conv;
  typedef shared_ptr<num_feature> num_feature_t;
  shared_ptr<key_matcher> a(new match_all());

  conv.register_num_rule("num", a, num_feature_t(new num_value_feature()));
  conv.register_num_rule("log", a, num_feature_t(new num_log_feature()));
  std::vector<std::pair<std::string, double> > feature;
  conv.convert(datum, feature);

  std::vector<std::pair<std::string, double> > expected;
  expected.push_back(std::make_pair("/val1@num", 1.1));
  expected.push_back(std::make_pair("/val1@log", std::log(1.1)));
  // elements with zero are removed
  // expected.push_back(std::make_pair("/val2@num", 0.));
  // expected.push_back(std::make_pair("/val2@log", std::log(1.)));

  ASSERT_EQ(expected, feature);
}

TEST(datum_to_fv_converter, string_feature) {
  shared_ptr<key_matcher> match(new match_all());
  typedef shared_ptr<word_splitter> splitter_t;

  datum_to_fv_converter conv;
  {
    shared_ptr<word_splitter> s(new space_splitter());
    std::vector<splitter_weight_type> p;
    p.push_back(splitter_weight_type(FREQ_BINARY, TERM_BINARY));
    p.push_back(splitter_weight_type(TERM_FREQUENCY, IDF));
    p.push_back(splitter_weight_type(LOG_TERM_FREQUENCY, IDF));
    conv.register_string_rule("space", match, s, p);
  }

  /*  {
   std::vector<std::string> keywords;
   keywords.push_back("is");
   //shared_ptr<word_splitter> s(new ux_splitter(keywords));
   shared_ptr<word_splitter> s(new dynamic_splitter(
   "../plugin/fv_converter/libux_splitter.so",
   "create", map<std::string, std::string>()));
   std::vector<splitter_weight_type> p;
   p.push_back(splitter_weight_type(TERM_FREQUENCY, TERM_BINARY));
   conv.register_string_rule("ux", match, s, p);
   }
   */
  {
    std::vector<splitter_weight_type> p;
    p.push_back(splitter_weight_type(FREQ_BINARY, TERM_BINARY));
    conv.register_string_rule("str", match, splitter_t(new without_split()), p);
  }

  std::vector<std::pair<std::string, double> > feature;
  {
    datum datum;
    datum.string_values_.push_back(std::make_pair("/name", "doc0"));
    datum.string_values_.push_back(std::make_pair("/title", " this is "));
    conv.convert_and_update_weight(datum, feature);
  }
  {
    datum datum;
    datum.string_values_.push_back(std::make_pair("/name", "doc1"));
    datum.string_values_.push_back(
        std::make_pair("/title", " this is it . it is it ."));
    conv.convert_and_update_weight(datum, feature);
  }

  std::vector<std::pair<std::string, double> > expected;
  expected.push_back(std::make_pair("/name$doc1@str#bin/bin", 1.));
  expected.push_back(
      std::make_pair("/title$ this is it . it is it .@str#bin/bin", 1.));

  expected.push_back(std::make_pair("/name$doc1@space#bin/bin", 1.));
  expected.push_back(std::make_pair("/title$this@space#bin/bin", 1.));
  expected.push_back(std::make_pair("/title$is@space#bin/bin", 1.));
  expected.push_back(std::make_pair("/title$it@space#bin/bin", 1.));
  expected.push_back(std::make_pair("/title$.@space#bin/bin", 1.));

  double idf1 = std::log((2. + 1) / (1. + 1));
  // double idf2 = std::log(2. / 2.);
  expected.push_back(std::make_pair("/name$doc1@space#tf/idf", 1. * idf1));
  // expected.push_back(std::make_pair("/title$this@space#tf/idf", 1. * idf2));
  // expected.push_back(std::make_pair("/title$is@space#tf/idf",   2. * idf2));
  expected.push_back(std::make_pair("/title$it@space#tf/idf", 3. * idf1));
  expected.push_back(std::make_pair("/title$.@space#tf/idf", 2. * idf1));

  expected.push_back(
      std::make_pair("/name$doc1@space#log_tf/idf", std::log(2.) * idf1));
  // expected.push_back(
  //     std::make_pair("/title$this@space#log_tf/idf", std::log(2.) * idf2));
  // expected.push_back(
  //     std::make_pair("/title$is@space#log_tf/idf",   std::log(3.) * idf2));
  expected.push_back(
      std::make_pair("/title$it@space#log_tf/idf", std::log(4.) * idf1));
  expected.push_back(
      std::make_pair("/title$.@space#log_tf/idf", std::log(3.) * idf1));

  // expected.push_back(std::make_pair("/title$is@ux#tf/bin", 3.));

  std::sort(feature.begin(), feature.end());
  std::sort(expected.begin(), expected.end());

  ASSERT_EQ(expected, feature);
}

TEST(datum_to_fv_converter, weight) {
  datum_to_fv_converter conv;
  {
    shared_ptr<key_matcher> match(new match_all());
    shared_ptr<word_splitter> s(new space_splitter());
    std::vector<splitter_weight_type> p;
    p.push_back(splitter_weight_type(FREQ_BINARY, WITH_WEIGHT_FILE));
    conv.register_string_rule("space", match, s, p);
  }
  conv.add_weight("/id$a@space", 3.f);

  datum datum;
  datum.string_values_.push_back(std::make_pair("/id", "a b"));

  std::vector<std::pair<std::string, double> > feature;
  conv.convert_and_update_weight(datum, feature);

  ASSERT_EQ(1u, feature.size());
  ASSERT_EQ("/id$a@space#bin/weight", feature[0].first);
  ASSERT_EQ(3., feature[0].second);
}

TEST(datum_to_fv_converter, register_string_rule) {
  datum_to_fv_converter conv;
  initialize_converter(converter_config(), conv);

  std::vector<splitter_weight_type> p;
  p.push_back(splitter_weight_type(FREQ_BINARY, TERM_BINARY));
  shared_ptr<word_splitter> s(new character_ngram(1));
  shared_ptr<key_matcher> a(new match_all());
  conv.register_string_rule("1gram", a, s, p);

  datum datum;
  datum.string_values_.push_back(std::make_pair("/id", "a b"));

  std::vector<std::pair<std::string, double> > feature;
  conv.convert(datum, feature);

  std::vector<std::pair<std::string, double> > exp;
  exp.push_back(std::make_pair("/id$a@1gram#bin/bin", 1.));
  exp.push_back(std::make_pair("/id$ @1gram#bin/bin", 1.));
  exp.push_back(std::make_pair("/id$b@1gram#bin/bin", 1.));

  std::sort(feature.begin(), feature.end());
  std::sort(exp.begin(), exp.end());
  ASSERT_EQ(exp, feature);
}

TEST(datum_to_fv_converter, register_num_rule) {
  datum_to_fv_converter conv;

  datum datum;
  datum.num_values_.push_back(std::make_pair("/age", 20));

  {
    std::vector<std::pair<std::string, double> > feature;
    conv.convert(datum, feature);
    EXPECT_EQ(0u, feature.size());
  }

  shared_ptr<num_feature> f(new num_string_feature());
  shared_ptr<key_matcher> a(new match_all());
  conv.register_num_rule("str", a, f);

  {
    std::vector<std::pair<std::string, double> > feature;
    conv.convert(datum, feature);
    EXPECT_EQ(1u, feature.size());

    std::vector<std::pair<std::string, double> > exp;
    exp.push_back(std::make_pair("/age@str$20", 1.));

    std::sort(feature.begin(), feature.end());
    std::sort(exp.begin(), exp.end());
    ASSERT_EQ(exp, feature);
  }
}

namespace {

class binary_length_feature : public binary_feature {
 public:
  void add_feature(
      const std::string& key,
      const std::string& value,
      std::vector<std::pair<std::string, double> >& ret_fv) const {
    ret_fv.push_back(std::make_pair(key, value.size()));
  }
};

}  // namespace

TEST(datum_to_fv_converter, register_binary_rule) {
  datum_to_fv_converter conv;

  datum datum;
  datum.binary_values_.push_back(std::make_pair("/bin", "0101"));

  {
    std::vector<std::pair<std::string, double> > feature;
    conv.convert(datum, feature);
    EXPECT_EQ(0u, feature.size());
  }

  shared_ptr<binary_feature> f(new binary_length_feature());
  shared_ptr<key_matcher> a(new match_all());
  conv.register_binary_rule("len", a, f);

  {
    std::vector<std::pair<std::string, double> > feature;
    conv.convert(datum, feature);
    EXPECT_EQ(1u, feature.size());

    std::vector<std::pair<std::string, double> > exp;
    exp.push_back(std::make_pair("/bin@len", 4.));

    std::sort(feature.begin(), feature.end());
    std::sort(exp.begin(), exp.end());
    ASSERT_EQ(exp, feature);
  }
}

TEST(datum_to_fv_converter, register_string_filter) {
  datum_to_fv_converter conv;

  datum datum;
  datum.string_values_.push_back(std::make_pair("/text", "<tag>aaa</tag>"));

  std::vector<splitter_weight_type> p;
  p.push_back(splitter_weight_type(FREQ_BINARY, TERM_BINARY));
  conv.register_string_rule("str",
      shared_ptr<key_matcher>(new match_all()),
      shared_ptr<word_splitter>(new without_split()),
      p);
  {
    std::vector<std::pair<std::string, double> > feature;
    conv.convert(datum, feature);
    EXPECT_EQ(1u, feature.size());
  }

#if defined(HAVE_RE2) || defined(HAVE_ONIGURUMA)
  conv.register_string_filter(shared_ptr<key_matcher>(new match_all()),
      shared_ptr<string_filter>(new regexp_filter("<[^>]*>", "")),
      "_filtered");

  {
    std::vector<std::pair<std::string, double> > feature;
    conv.convert(datum, feature);
    EXPECT_EQ(2u, feature.size());
    EXPECT_EQ("/text_filtered$aaa@str#bin/bin", feature[1].first);
  }
#endif
}

TEST(datum_to_fv_converter, register_num_filter) {
  datum_to_fv_converter conv;

  datum datum;
  datum.num_values_.push_back(std::make_pair("/age", 20));

  conv.register_num_rule("str",
      shared_ptr<key_matcher>(new match_all()),
      shared_ptr<num_feature>(new num_string_feature()));

  conv.register_num_filter(
      shared_ptr<key_matcher>(new match_all()),
      shared_ptr<num_filter>(new add_filter(5)),
      "+5");

  std::vector<std::pair<std::string, double> > feature;
  conv.convert(datum, feature);

  EXPECT_EQ(2u, feature.size());
  EXPECT_EQ("/age+5@str$25", feature[1].first);
}

TEST(datum_to_fv_converter, recursive_filter) {
  datum_to_fv_converter conv;
  datum datum;
  datum.num_values_.push_back(std::make_pair("/age", 20));

  conv.register_num_rule("str",
      shared_ptr<key_matcher>(new match_all()),
      shared_ptr<num_feature>(new num_string_feature()));

  conv.register_num_filter(
      shared_ptr<key_matcher>(new match_all()),
      shared_ptr<num_filter>(new add_filter(5)),
      "+5");
  conv.register_num_filter(
      shared_ptr<key_matcher>(new match_all()),
      shared_ptr<num_filter>(new add_filter(2)),
      "+2");

  std::vector<std::pair<std::string, double> > feature;
  conv.convert(datum, feature);

  EXPECT_EQ(4u, feature.size());
  EXPECT_EQ("/age+5@str$25", feature[1].first);
  EXPECT_EQ("/age+2@str$22", feature[2].first);
  EXPECT_EQ("/age+5+2@str$27", feature[3].first);
}

TEST(datum_to_fv_converter, hasher) {
  datum_to_fv_converter conv;
  conv.set_hash_max_size(1);
  conv.register_num_rule("str",
      shared_ptr<key_matcher>(new match_all()),
      shared_ptr<num_feature>(new num_string_feature()));
  datum d;
  for (int i = 0; i < 10; ++i)
  d.num_values_.push_back(std::make_pair("age", i));

  std::vector<std::pair<std::string, double> > feature;
  conv.convert(d, feature);

  for (size_t i = 0; i < feature.size(); ++i)
  EXPECT_EQ("0", feature[i].first);
}

TEST(datum_to_fv_converter, check_datum_key_in_string) {
  datum_to_fv_converter conv;

  {
    shared_ptr<key_matcher> match(new match_all());
    shared_ptr<word_splitter> s(new space_splitter());
    std::vector<splitter_weight_type> p;
    p.push_back(splitter_weight_type(FREQ_BINARY, TERM_BINARY));
    p.push_back(splitter_weight_type(TERM_FREQUENCY, IDF));
    p.push_back(splitter_weight_type(LOG_TERM_FREQUENCY, IDF));
    conv.register_string_rule("space", match, s, p);
  }

  datum datum;
  datum.string_values_.push_back(std::make_pair("bad$key", "doc0"));
  std::vector<std::pair<std::string, double> > feature;
  ASSERT_THROW(conv.convert(datum, feature), converter_exception);
}

TEST(datum_to_fv_converter, check_datum_key_in_number) {
  datum_to_fv_converter conv;

  shared_ptr<num_feature> f(new num_string_feature());
  shared_ptr<key_matcher> a(new match_all());
  conv.register_num_rule("str", a, f);

  datum datum;
  datum.num_values_.push_back(std::make_pair("bad$key", 20));

  std::vector<std::pair<std::string, double> > feature;
  ASSERT_THROW(conv.convert(datum, feature), converter_exception);
}

TEST(datum_to_fv_converter, check_datum_key_in_binary) {
  datum_to_fv_converter conv;

  shared_ptr<binary_feature> f(new binary_length_feature());
  shared_ptr<key_matcher> a(new match_all());
  conv.register_binary_rule("len", a, f);

  datum datum;
  datum.binary_values_.push_back(std::make_pair("bad$key", "0101"));
  std::vector<std::pair<std::string, double> > feature;
  ASSERT_THROW(conv.convert(datum, feature), converter_exception);
}

TEST(datum_to_fv_converter, combination_feature_num) {
  datum d;
  d.num_values_.push_back(std::make_pair("val1", 1.0));
  d.num_values_.push_back(std::make_pair("val2", 1.1));

  datum_to_fv_converter conv;
  typedef shared_ptr<combination_feature> combination_feature_t;
  typedef shared_ptr<num_feature> num_feature_t;
  shared_ptr<key_matcher> all_matcher(new match_all());

  conv.register_num_rule(
      "num",
      all_matcher,
      num_feature_t(new num_value_feature()));
  conv.register_combination_rule(
      "add",
      all_matcher,
      all_matcher,
      combination_feature_t(new combination_add_feature()));
  conv.register_combination_rule(
      "mul",
      all_matcher,
      all_matcher,
      combination_feature_t(new combination_mul_feature()));
  std::vector<std::pair<std::string, double> > feature;
  conv.convert(d, feature);

  std::vector<std::pair<std::string, double> > expected;
  expected.push_back(std::make_pair("val1@num", 1.0));
  expected.push_back(std::make_pair("val2@num", 1.1));
  expected.push_back(std::make_pair("val1@num&val2@num/add", 2.1));
  expected.push_back(std::make_pair("val1@num&val2@num/mul", 1.1));

  ASSERT_EQ(expected, feature);

  // test for empty datum
  datum empty_datum;
  feature.clear();
  expected.clear();

  conv.convert(empty_datum, feature);
  ASSERT_EQ(expected, feature);
}

namespace {

class combination_div_feature : public combination_feature {
 public:
  void add_feature(const std::string& key,
                   double value_left,
                   double value_right,
                   common::sfv_t& ret_fv) const {
    ret_fv.push_back(
        std::make_pair(key, static_cast<double>(value_left / value_right)));
  }

  bool is_commutative() const {
    return false;
  }
};

}  // namespace

TEST(datum_to_fv_converter, combination_feature_num_non_commutative) {
  datum d;
  d.num_values_.push_back(std::make_pair("val1", 2.0));
  d.num_values_.push_back(std::make_pair("val2", 0.5));

  datum_to_fv_converter conv;
  typedef shared_ptr<combination_feature> combination_feature_t;
  typedef shared_ptr<num_feature> num_feature_t;
  shared_ptr<key_matcher> all_matcher(new match_all());

  conv.register_num_rule(
      "num",
      all_matcher,
      num_feature_t(new num_value_feature()));
  conv.register_combination_rule(
      "div",
      all_matcher,
      all_matcher,
      combination_feature_t(new combination_div_feature()));
  std::vector<std::pair<std::string, double> > feature;
  conv.convert(d, feature);

  std::vector<std::pair<std::string, double> > expected;
  expected.push_back(std::make_pair("val1@num", 2.0));
  expected.push_back(std::make_pair("val2@num", 0.5));
  expected.push_back(std::make_pair("val1@num&val2@num/div", 4.0));
  expected.push_back(std::make_pair("val2@num&val1@num/div", 0.25));

  ASSERT_EQ(expected, feature);
}

TEST(datum_to_fv_converter, combination_feature_string) {
  datum datum;
  datum.string_values_.push_back(std::make_pair("name", "abc xyz"));
  datum.string_values_.push_back(std::make_pair("title", "foo bar"));

  datum_to_fv_converter conv;
  {
    shared_ptr<key_matcher> all_matcher(new match_all());
    shared_ptr<word_splitter> s(new space_splitter());
    std::vector<splitter_weight_type> p;
    p.push_back(splitter_weight_type(FREQ_BINARY, TERM_BINARY));
    p.push_back(splitter_weight_type(TERM_FREQUENCY, IDF));
    p.push_back(splitter_weight_type(LOG_TERM_FREQUENCY, IDF));
    conv.register_string_rule("space", all_matcher, s, p);
  }

  typedef shared_ptr<combination_feature> combination_feature_t;
  shared_ptr<key_matcher> name_matcher(new prefix_match("name"));
  shared_ptr<key_matcher> title_matcher(new prefix_match("title"));

  conv.register_combination_rule(
      "add",
      name_matcher,
      title_matcher,
      combination_feature_t(new combination_add_feature()));
  conv.register_combination_rule(
      "mul",
      title_matcher,
      title_matcher,
      combination_feature_t(new combination_mul_feature()));
  std::vector<std::pair<std::string, double> > feature;
  conv.convert(datum, feature);

  std::vector<std::pair<std::string, double> > expected;
  expected.push_back(std::make_pair("name$xyz@space#bin/bin", 1.0));
  expected.push_back(std::make_pair("name$abc@space#bin/bin", 1.0));
  expected.push_back(std::make_pair("title$bar@space#bin/bin", 1.0));
  expected.push_back(std::make_pair("title$foo@space#bin/bin", 1.0));
  expected.push_back(std::make_pair(
      "name$xyz@space#bin/bin&title$bar@space#bin/bin/add",
      2.0));
  expected.push_back(std::make_pair(
      "name$xyz@space#bin/bin&title$foo@space#bin/bin/add",
      2.0));
  expected.push_back(std::make_pair(
      "name$abc@space#bin/bin&title$bar@space#bin/bin/add",
      2.0));
  expected.push_back(std::make_pair(
      "name$abc@space#bin/bin&title$foo@space#bin/bin/add",
      2.0));
  expected.push_back(std::make_pair(
      "title$bar@space#bin/bin&title$foo@space#bin/bin/mul",
      1.0));

  ASSERT_EQ(expected, feature);
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
