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

#ifndef JUBATUS_CORE_BANDIT_BANDIT_BASE_HPP_
#define JUBATUS_CORE_BANDIT_BANDIT_BASE_HPP_

#include <string>

#include "arm_info.hpp"

namespace msgpack {
template <typename T>
class packer;
}  // namespace msgpack
namespace jubatus {
namespace core {
namespace framework {
class jubatus_packer;
typedef msgpack::packer<jubatus_packer> packer;
}  // namespace framework
namespace storage {
class version;
}  // namespace storage
namespace bandit {

class bandit_base {
 public:
  bandit_base() {
  }
  virtual ~bandit_base() {
  }

  virtual bool register_arm(const std::string& arm_id) = 0;
  virtual bool delete_arm(const std::string& arm_id) = 0;

  virtual std::string select_arm(const std::string& player_id) = 0;

  virtual bool register_reward(const std::string& player_id,
                               const std::string& arm_id,
                               double reward) = 0;

  virtual arm_info_map get_arm_info(const std::string& player_id) const = 0;

  virtual bool reset(const std::string& player_id) = 0;
  virtual void clear() = 0;

  virtual std::string name() const = 0;

  virtual void pack(framework::packer& pk) const = 0;
  virtual void unpack(msgpack::object o) = 0;

  typedef jubatus::util::data::unordered_map<std::string,
    counted_arm_info_map> diff_t;

  virtual void get_diff(diff_t& diff) const = 0;
  virtual bool put_diff(const diff_t& diff) = 0;
  virtual void mix(const diff_t& lhs, diff_t& rhs) const = 0;

  virtual core::storage::version get_version() const = 0;
};

}  // namespace bandit
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BANDIT_BANDIT_BASE_HPP_
