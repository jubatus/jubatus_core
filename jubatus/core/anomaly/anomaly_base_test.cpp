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
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "jubatus/util/lang/shared_ptr.h"

#include "../storage/column_table.hpp"
#include "anomaly_base.hpp"

namespace jubatus {
namespace core {
namespace anomaly {

class anomaly_impl : public anomaly_base {
 public:
  anomaly_impl() : anomaly_base() {
  }

  float calc_anomaly_score(
      const common::sfv_t& query) const {
    return 1.0;
  }

  float calc_anomaly_score(const std::string& id) const {
    return 1.0;
  }

  float calc_anomaly_score(
      const std::string& id,
      const common::sfv_t& query) const {
    return 1.0;
  }

  void clear() {
  }

  void clear_row(const std::string& id) {
  }

  bool update_row(const std::string& id, const sfv_diff_t& diff) {
    return true;
  }

  bool set_row(const std::string& id, const common::sfv_t& sfv) {
    return true;
  }

  void get_all_row_ids(std::vector<std::string>& ids) const {
    ids.clear();
    ids.push_back("100");
    ids.push_back("99.0");
    ids.push_back("A300");
  }

  std::string type() const {
    return std::string("anomaly_impl");
  }

  std::vector<framework::mixable*> get_mixables() const {
    return std::vector<framework::mixable*>();
  }

  void pack(framework::packer& packer) const {
  }
  void unpack(msgpack::object o) {
  }

  bool is_updatable() const {
    return true;
  }
};

TEST(anomaly_base, find_max_int_id) {
  anomaly_impl a;
  std::vector<std::string> ids;
  a.get_all_row_ids(ids);
  uint64_t max_id = a.find_max_int_id();
  EXPECT_EQ("100", ids[0]);
  EXPECT_EQ("99.0", ids[1]);
  EXPECT_EQ("A300", ids[2]);
  EXPECT_EQ(100u, max_id);
}

}  // namespace anomaly
}  // namespace core
}  // namespace jubatus
