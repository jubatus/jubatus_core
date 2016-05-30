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
#include "keyword_weights.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

TEST(keyword_weights, trivial) {
  keyword_weights m, m2;
  {
    std::vector<std::string> keys;

    m.increment_document_count();

    keys.push_back("key1");
    keys.push_back("key2");
    m.increment_document_count();
    m.increment_document_frequency(keys);

    m.add_weight("key3", 2.0);

    EXPECT_EQ(2u, m.get_document_count());
    EXPECT_EQ(1u, m.get_document_frequency("key1"));
    EXPECT_EQ(0u, m.get_document_frequency("unknown"));
  }

  {
    std::vector<std::string> keys;
    m2.increment_document_count();

    keys.push_back("key1");
    keys.push_back("key2");
    m2.increment_document_count();
    m2.increment_document_frequency(keys);

    m2.add_weight("key3", 3.0);

    m.merge(m2);

    EXPECT_EQ(4u, m.get_document_count());
    EXPECT_EQ(2u, m.get_document_frequency("key1"));
    EXPECT_EQ(3.0, m.get_user_weight("key3"));
    EXPECT_EQ(0u, m.get_document_frequency("unknown"));
  }

  {
    m.clear();
    EXPECT_EQ(0u, m.get_document_count());
    EXPECT_EQ(0u, m.get_document_frequency("key1"));
    EXPECT_EQ(0.0, m.get_user_weight("key3"));
  }
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
