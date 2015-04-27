// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2015 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_BANDIT_EXP3_HPP_
#define JUBATUS_CORE_BANDIT_EXP3_HPP_

#include <string>
#include <vector>

#include "bandit_base.hpp"
#include "summation_storage.hpp"
#include "jubatus/util/math/random.h"

namespace jubatus {
namespace core {
namespace bandit {

class exp3 : public bandit_base {
 public:
  exp3(bool assume_unrewarded, double gamma);

  std::string select_arm(const std::string& player_id);

  bool register_arm(const std::string& arm_id);
  bool delete_arm(const std::string& arm_id);

  bool register_reward(const std::string& player_id,
                       const std::string& arm_id,
                       double reward);

  arm_info_map get_arm_info(const std::string& player_id) const;

  bool reset(const std::string& player_id);
  void clear();

  std::string name() const {
    return "exp3";
  }

  void pack(framework::packer& pk) const;
  void unpack(msgpack::object o);

  void get_diff(diff_t& diff) const;
  bool put_diff(const diff_t& diff);
  void mix(const diff_t& lhs, diff_t& rhs) const;
  storage::version get_version() const;

 private:
  double gamma_;
  jubatus::util::math::random::mtrand rand_;
  summation_storage s_;

  void calc_weights_(const std::string& player_id,
                     std::vector<double>& weights) const;
};

}  // namespace bandit
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BANDIT_EXP3_HPP_
