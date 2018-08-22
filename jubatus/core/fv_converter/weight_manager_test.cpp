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

#include <iostream>
#include <cmath>
#include <utility>
#include <gtest/gtest.h>
#include "../common/type.hpp"
#include "weight_manager.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

TEST(weight_manager, trivial) {
  weight_manager m;

  {
    common::sfv_t fv;
    m.add_weight("/address$tokyo@str", 1.5);

    m.update_weight(fv, true, true);  // |D| = 1
    m.get_weight(fv);
    ASSERT_EQ(0, fv.size());

    m.update_weight(fv, true, true);  // |D| = 2
    m.get_weight(fv);
    ASSERT_EQ(0, fv.size());

    fv.push_back(std::make_pair("/title$this@space#bin/bin", 1.0));
    fv.push_back(std::make_pair("/title$this@space#bin/idf", 1.0));
    fv.push_back(std::make_pair("/age@bin", 1.0));
    fv.push_back(std::make_pair("/address$tokyo@str#bin/weight", 1.0));
    fv.push_back(std::make_pair("/profile$hello@space#tf/bm25", 5.0));
    fv.push_back(std::make_pair("/profile$world@space#tf/bm25", 4.0));
    fv.push_back(std::make_pair("/title$this@space#bin/idf1", 1.0));
    m.update_weight(fv, true, true);  // |D| = 3
    m.get_weight(fv);

    ASSERT_EQ(7u, fv.size());

    // String features without weighting
    EXPECT_DOUBLE_EQ(1.0, fv[0].second);

    // String features weighted by bin-idf
    EXPECT_DOUBLE_EQ(1.0 * std::log((3.0 + 1) / (1.0 + 1)), fv[1].second);

    // Non-string features
    EXPECT_DOUBLE_EQ(1.0, fv[2].second);

    // String features weighted by bin-weight
    EXPECT_DOUBLE_EQ(1.5, fv[3].second);

    // String features weighted by tf-bm25
    EXPECT_DOUBLE_EQ(
        std::log((3.0 - 1.0 + 0.5) / (1.0 + 0.5)) *
        ((/* tf = */ 5.0 * (1.2 + 1)) /
        (/* tf = */ 5.0 + 1.2 * (1 - 0.75 + 0.75 *
        (9.0 / 9.0)))), fv[4].second);
    EXPECT_DOUBLE_EQ(
        std::log((3.0 - 1.0 + 0.5) / (1.0 + 0.5)) *
        ((/* tf = */ 4.0 * (1.2 + 1)) /
        (/* tf = */ 4.0 + 1.2 * (1 - 0.75 + 0.75 *
        (9.0 / 9.0)))), fv[5].second);

    // String features weighted by bin-idf1
    EXPECT_DOUBLE_EQ(
        1.0 * (std::log((3.0 + 1) / (1.0 + 1)) + 1.0), fv[6].second);
  }

  versioned_weight_diff w;
  m.get_diff(w);
  EXPECT_EQ(3u, w.weights_.get_document_count());
  EXPECT_EQ(1u, w.weights_.get_document_frequency("/title$this@space#bin/idf"));
  EXPECT_EQ(1.5, w.weights_.get_user_weight("/address$tokyo@str"));

  {
    common::sfv_t fv;
    fv.push_back(std::make_pair("/title$this@space#bin/idf", 1.0));

    // df = 2, |D| = 4
    w.weights_.update_document_frequency(fv, false);
    w.weights_.add_weight("/title$hoge@str", 2.0);

    m.put_diff(w);

    m.get_diff(w);
    EXPECT_EQ(0u, w.weights_.get_document_count());

    // df = 3, |D| = 5
    m.update_weight(fv, true, false);
    m.get_weight(fv);
    EXPECT_EQ(1u, fv.size());
    EXPECT_DOUBLE_EQ(1.0 * std::log((5.0 + 1) / (3.0 + 1)), fv[0].second);
  }
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
