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

}  // namespace common
}  // namespace core
}  // namespace jubatus
