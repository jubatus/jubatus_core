// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "jubatus/util/lang/shared_ptr.h"
#include "mixable_weight_manager.hpp"
#include "datum.hpp"

using std::make_pair;
using std::stringstream;
using std::sort;
using jubatus::util::lang::shared_ptr;
using jubatus::core::fv_converter::keyword_weights;
using jubatus::core::fv_converter::datum;

namespace jubatus {
namespace core {
namespace fv_converter {

class versioned_weight_diff_test : public ::testing::Test {
 public:
  void SetUp() {
    {
      std::vector<std::string> keys;
      keys.push_back("a");
      kw1.increment_document_count();
      kw1.increment_document_frequency(keys);
    }
    {
      std::vector<std::string> keys;
      keys.push_back("b");
      kw1.increment_document_count();
      kw1.increment_document_frequency(keys);
      kw1.increment_document_count();
      kw1.increment_document_frequency(keys);
    }
    {
      std::vector<std::string> keys;
      keys.push_back("b");
      kw2.increment_document_count();
      kw2.increment_document_frequency(keys);
      kw2.increment_document_count();
      kw2.increment_document_frequency(keys);
      kw2.increment_document_count();
      kw2.increment_document_frequency(keys);
      kw2.increment_document_count();
      kw2.increment_document_frequency(keys);
    }
    {
      std::vector<std::string> keys;
      keys.push_back("c");
      kw2.increment_document_count();
      kw2.increment_document_frequency(keys);
      kw2.increment_document_count();
      kw2.increment_document_frequency(keys);
      kw2.increment_document_count();
      kw2.increment_document_frequency(keys);
      kw2.increment_document_count();
      kw2.increment_document_frequency(keys);
      kw2.increment_document_count();
      kw2.increment_document_frequency(keys);
      kw2.increment_document_count();
      kw2.increment_document_frequency(keys);
      kw2.increment_document_count();
      kw2.increment_document_frequency(keys);
      kw2.increment_document_count();
      kw2.increment_document_frequency(keys);
    }
  }
  void TearDown() {
    kw1.clear();
    kw2.clear();
  }

  keyword_weights kw1;
  keyword_weights kw2;
};

TEST_F(versioned_weight_diff_test, fixture) {
  ASSERT_EQ(3U, kw1.get_document_count());
  ASSERT_EQ(1U, kw1.get_document_frequency("a"));
  ASSERT_EQ(2U, kw1.get_document_frequency("b"));

  ASSERT_EQ(12U, kw2.get_document_count());
  ASSERT_EQ(4U, kw2.get_document_frequency("b"));
  ASSERT_EQ(8U, kw2.get_document_frequency("c"));
}

TEST_F(versioned_weight_diff_test, merge_success) {
  versioned_weight_diff vw1(kw1);
  versioned_weight_diff vw2(kw2);

  vw1.version_.increment();
  vw2.version_.increment();
  vw1.merge(vw2);
  ASSERT_EQ(15U, vw1.weights_.get_document_count());
  ASSERT_EQ(1U, vw1.weights_.get_document_frequency("a"));
  ASSERT_EQ(6U, vw1.weights_.get_document_frequency("b"));
  ASSERT_EQ(8U, vw1.weights_.get_document_frequency("c"));
}

TEST_F(versioned_weight_diff_test, merge_vw1_win) {
  versioned_weight_diff vw1(kw1);
  versioned_weight_diff vw2(kw2);

  vw1.version_.increment();
  vw1.merge(vw2);
  ASSERT_EQ(3U, vw1.weights_.get_document_count());
  ASSERT_EQ(1U, vw1.weights_.get_document_frequency("a"));
  ASSERT_EQ(2U, vw1.weights_.get_document_frequency("b"));
  ASSERT_EQ(0U, vw1.weights_.get_document_frequency("c"));
}

TEST_F(versioned_weight_diff_test, merge_vw2_win) {
  versioned_weight_diff vw1(kw1);
  versioned_weight_diff vw2(kw2);

  vw2.version_.increment();
  vw1.merge(vw2);
  ASSERT_EQ(12U, vw1.weights_.get_document_count());
  ASSERT_EQ(0U, vw1.weights_.get_document_frequency("a"));
  ASSERT_EQ(4U, vw1.weights_.get_document_frequency("b"));
  ASSERT_EQ(8U, vw1.weights_.get_document_frequency("c"));
}

class mixable_weight_manager_test : public ::testing::Test {
 public:
  void SetUp() {
    shared_ptr<weight_manager> m(new weight_manager);

    {
      counter<std::string> counter;
      counter["K"] = 1;

      m->increment_document_count();
      m->update_weight("a", "T", tf_bin, counter);
      m->update_weight("b", "T", tf_bin, counter);
    }

    mw.reset(new mixable_weight_manager(m));
  }
  void TearDown() {
    mw.reset();
  }

  mixable_weight_manager_test() : tf_bin(TERM_FREQUENCY, TERM_BINARY) {}

  shared_ptr<mixable_weight_manager> mw;
  splitter_weight_type tf_bin;
};

TEST_F(mixable_weight_manager_test, fixture) {
  ASSERT_EQ(0U, mw->get_version().get_number());
}

TEST_F(mixable_weight_manager_test, get_diff) {
  versioned_weight_diff got;
  mw->get_model()->get_diff(got);
  ASSERT_EQ(0U, got.version_.get_number());
  ASSERT_EQ(1U, got.weights_.get_document_count());
  ASSERT_EQ(1U, got.weights_.get_document_frequency("a$K@T#tf/bin"));
  ASSERT_EQ(1U, got.weights_.get_document_frequency("b$K@T#tf/bin"));
}

TEST_F(mixable_weight_manager_test, put_diff) {
  keyword_weights w;
  {
    std::vector<std::string> keys;
    keys.push_back("a$K@T#tf/bin");
    keys.push_back("b$K@T#tf/bin");
    w.increment_document_count();
    w.increment_document_frequency(keys);
  }

  versioned_weight_diff appender(w);
  mw->get_model()->put_diff(appender);
  versioned_weight_diff got;
  mw->get_model()->get_diff(got);
  ASSERT_EQ(1U, got.version_.get_number());  // should be incremented

  ASSERT_EQ(0U, got.weights_.get_document_count());
  ASSERT_EQ(0U, got.weights_.get_document_frequency("a$K@T#tf/bin"));
  ASSERT_EQ(0U, got.weights_.get_document_frequency("b$K@T#tf/bin"));

  common::sfv_t result;
  counter<std::string> counter;
  counter["K"] = 2;
  mw->get_model()->add_string_features("a", "T", tf_bin, counter, result);
  counter["K"] = 3;
  mw->get_model()->add_string_features("b", "T", tf_bin, counter, result);

  ASSERT_EQ(2U, result.size());
  ASSERT_EQ("a$K@T#tf/bin", result[0].first);
  ASSERT_EQ(2, result[0].second);
  ASSERT_EQ("b$K@T#tf/bin", result[1].first);
  ASSERT_EQ(3, result[1].second);
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
