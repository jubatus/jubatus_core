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

#include <cmath>
#include <utility>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "../common/type.hpp"
#include "weight_manager.hpp"
#include "converter_config.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

namespace {

counter<std::string> make_counter(const std::string& key, double value) {
  counter<std::string> counter;
  counter[key] = value;
  return counter;
}

}  // namespace

TEST(weight_manager, trivial) {
  weight_manager m;

  splitter_weight_type bin_bin(FREQ_BINARY, TERM_BINARY);
  splitter_weight_type bin_idf(FREQ_BINARY, IDF);
  splitter_weight_type bin_weight(FREQ_BINARY, WITH_WEIGHT_FILE);

  {
    common::sfv_t fv;
    m.add_weight("/address$tokyo@str", 1.5);
    m.increment_document_count();

    // df = 1, |D| = 2

    m.increment_document_count();
    m.update_weight("/title", "space", bin_bin, make_counter("this", 1));
    m.update_weight("/title", "space", bin_idf, make_counter("this", 1));
    m.update_weight("/address", "str", bin_weight, make_counter("tokyo", 1));

    m.add_string_features(
        "/title", "space", bin_bin, make_counter("this", 1), fv);
    m.add_string_features(
        "/title", "space", bin_idf, make_counter("this", 1), fv);
    fv.push_back(std::make_pair("/age@bin", 1.0));
    m.add_string_features(
        "/address", "str", bin_weight, make_counter("tokyo", 1), fv);

    ASSERT_EQ(4u, fv.size());
    EXPECT_FLOAT_EQ(1.0, fv[0].second);
    EXPECT_FLOAT_EQ(1.0 * std::log((2.0 + 1) / (1.0 + 1)), fv[1].second);
    EXPECT_FLOAT_EQ(1.0, fv[2].second);
    EXPECT_FLOAT_EQ(1.5, fv[3].second);
  }

  versioned_weight_diff w;
  m.get_diff(w);
  EXPECT_EQ(2u, w.weights_.get_document_count());
  EXPECT_EQ(1u, w.weights_.get_document_frequency("/title$this@space#bin/idf"));
  EXPECT_EQ(1.5, w.weights_.get_user_weight("/address$tokyo@str"));

  {
    common::sfv_t fv;

    // df = 2, |D| = 3
    std::vector<std::string> doc;
    doc.push_back("/title$this@space#bin/idf");
    w.weights_.increment_document_count();
    w.weights_.increment_document_frequency(doc);
    w.weights_.add_weight("/title$hoge@str", 2.0);

    m.put_diff(w);

    m.get_diff(w);
    EXPECT_EQ(0u, w.weights_.get_document_count());

    // df = 3, |D| = 4
    m.increment_document_count();
    m.update_weight("/title", "space", bin_idf, make_counter("this", 1));

    m.add_string_features(
        "/title", "space", bin_idf, make_counter("this", 1), fv);
    EXPECT_EQ(1u, fv.size());
    EXPECT_FLOAT_EQ(1.0 * std::log((4.0 + 1) / (3.0 + 1)), fv[0].second);
  }
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
