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

#include "input_window.hpp"

#include <gtest/gtest.h>

namespace jubatus {
namespace core {
namespace burst {

TEST(construct, input_window) {
  ASSERT_THROW({input_window w(0,  0, 10);},
               common::invalid_parameter);
  ASSERT_THROW({input_window w(0, -1, 10);},
               common::invalid_parameter);
  ASSERT_THROW({input_window w(0, 10, -1);},
               common::invalid_parameter);
  ASSERT_THROW({input_window w(0, -1, -1);},
               common::invalid_parameter);

  input_window w(1, 2, 5);
  ASSERT_DOUBLE_EQ(1, w.get_start_pos());
  ASSERT_DOUBLE_EQ(11, w.get_end_pos());
  ASSERT_DOUBLE_EQ(10, w.get_all_interval());
  ASSERT_EQ(5, w.get_batch_size());
  ASSERT_DOUBLE_EQ(2, w.get_batch_interval());

  ASSERT_TRUE(w.contains(1));
  ASSERT_TRUE(w.contains(5));
  ASSERT_FALSE(w.contains(11));
  ASSERT_FALSE(w.contains(12));
}

TEST(default_constructed, input_window) {
  input_window w;

  ASSERT_DOUBLE_EQ(0, w.get_start_pos());
  ASSERT_DOUBLE_EQ(0, w.get_end_pos());
  ASSERT_DOUBLE_EQ(0, w.get_all_interval());
  ASSERT_EQ(0, w.get_batch_size());
  ASSERT_LT(0, w.get_batch_interval());

  ASSERT_FALSE(w.add_document(1, 1, 1));  // fails anywhere
}

TEST(add_document, input_window) {
  input_window w(1, 2, 5);

  ASSERT_FALSE(w.add_document(2, 1, 0));  // fails if out-of-range
  ASSERT_EQ(0, w.get_batches()[0].d);
  ASSERT_EQ(0, w.get_batches()[0].r);

  ASSERT_TRUE(w.add_document(2, 1, 1));
  ASSERT_EQ(2, w.get_batches()[0].d);
  ASSERT_EQ(1, w.get_batches()[0].r);

  ASSERT_TRUE(w.add_document(2, 1, 2));
  ASSERT_EQ(4, w.get_batches()[0].d);
  ASSERT_EQ(2, w.get_batches()[0].r);

  ASSERT_TRUE(w.add_document(2, 1, 3));
  ASSERT_EQ(4, w.get_batches()[0].d);
  ASSERT_EQ(2, w.get_batches()[0].r);
  ASSERT_EQ(2, w.get_batches()[1].d);
  ASSERT_EQ(1, w.get_batches()[1].r);

  ASSERT_TRUE(w.add_document(2, 1, 10));
  ASSERT_EQ(2, w.get_batches()[4].d);
  ASSERT_EQ(1, w.get_batches()[4].r);

  ASSERT_FALSE(w.add_document(2, 1, 11));  // ouf-of-range
  ASSERT_EQ(2, w.get_batches()[4].d);
  ASSERT_EQ(1, w.get_batches()[4].r);
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
