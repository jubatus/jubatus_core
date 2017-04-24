// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2016 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_REGRESSION_NEAREST_NEIGHBOR_REGRESSION_HPP_
#define JUBATUS_CORE_REGRESSION_NEAREST_NEIGHBOR_REGRESSION_HPP_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>
#include "jubatus/util/math/random.h"
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/concurrent/lock.h"
#include "jubatus/util/concurrent/mutex.h"
#include "../common/jsonconfig.hpp"
#include "jubatus/util/data/optional.h"
#include "../framework/mixable_versioned_table.hpp"

#include "../common/type.hpp"
#include "../nearest_neighbor/nearest_neighbor_base.hpp"
#include "../storage/labels.hpp"
#include "../unlearner/unlearner_base.hpp"
#include "regression_base.hpp"

namespace jubatus {
namespace core {
namespace regression {

class nearest_neighbor_regression : public regression_base {
 public:
  struct unlearner_config {
    jubatus::util::data::optional<std::string> unlearner;
    jubatus::util::data::optional<common::jsonconfig::config>
        unlearner_parameter;

    template<typename Ar>
    void serialize(Ar& ar) {
      ar & JUBA_MEMBER(unlearner) & JUBA_MEMBER(unlearner_parameter);
    }
  };

  struct config
    : public unlearner_config {
    std::string method;
    common::jsonconfig::config parameter;
    int nearest_neighbor_num;
    jubatus::util::data::optional<std::string> weight;

    template<typename Ar>
    void serialize(Ar& ar) {
      ar & JUBA_MEMBER(method)
        & JUBA_MEMBER(parameter)
        & JUBA_MEMBER(nearest_neighbor_num)
        & JUBA_MEMBER(weight);
      unlearner_config::serialize(ar);
    }
  };

  nearest_neighbor_regression(
      jubatus::util::lang::shared_ptr<nearest_neighbor::nearest_neighbor_base>
        nearest_neighbor_engine,
      const config& config);

  void train(const common::sfv_t& fv, const float value);
  float estimate(const common::sfv_t& fv) const;
  void clear();

  std::string name() const;

  void get_status(std::map<std::string, std::string>& status) const;

  void pack(framework::packer& pk) const;
  void unpack(msgpack::object o);

  std::vector<framework::mixable*> get_mixables();

  void set_unlearner(
      jubatus::util::lang::shared_ptr<unlearner::unlearner_base>
          unlearner);
  jubatus::util::lang::shared_ptr<unlearner::unlearner_base>
  create_unlearner(const unlearner_config& conf);

 private:
  jubatus::util::lang::shared_ptr<nearest_neighbor::nearest_neighbor_base>
      nearest_neighbor_engine_;

  config config_;
  jubatus::util::concurrent::mutex unlearner_mutex_;
  jubatus::util::lang::shared_ptr<unlearner::unlearner_base> unlearner_;
  jubatus::util::lang::shared_ptr<framework::mixable_versioned_table> values_;
  jubatus::util::concurrent::mutex rand_mutex_;
  jubatus::util::math::random::mtrand rand_;

  class unlearning_callback;
  void unlearn_id(const std::string& id);
};

}  // namespace regression
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_REGRESSION_NEAREST_NEIGHBOR_REGRESSION_HPP_
