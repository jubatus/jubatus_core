// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_ANOMALY_LOF_HPP_
#define JUBATUS_CORE_ANOMALY_LOF_HPP_

#include <string>
#include <vector>

#include "jubatus/util/lang/shared_ptr.h"

#include "../recommender/recommender_base.hpp"
#include "../unlearner/unlearner_base.hpp"
#include "anomaly_base.hpp"
#include "lof_storage.hpp"

namespace jubatus {
namespace core {
namespace anomaly {

class lof : public anomaly_base {
 public:
  lof(
      const lof_storage::config& config,
      jubatus::util::lang::shared_ptr<core::recommender::recommender_base>
          nn_engine);
  lof(
      const lof_storage::config& config,
      jubatus::util::lang::shared_ptr<core::recommender::recommender_base>
          nn_engine,
      jubatus::util::lang::shared_ptr<unlearner::unlearner_base> unlearner);
  ~lof();

  // return anomaly score of query
  float calc_anomaly_score(const common::sfv_t& query) const;
  float calc_anomaly_score(const std::string& id) const;
  float calc_anomaly_score(
      const std::string& id,
      const common::sfv_t& query) const;

  void clear();
  void clear_row(const std::string& id);
  void remove_row(const std::string& id);
  bool update_row(const std::string& id, const sfv_diff_t& diff);
  bool set_row(const std::string& id, const common::sfv_t& sfv);

  void get_all_row_ids(std::vector<std::string>& ids) const;
  std::string type() const;
  std::vector<framework::mixable*> get_mixables() const;

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

  bool is_updatable() const {
    return true;
  }

 private:
  jubatus::util::lang::shared_ptr<mixable_lof_storage> mixable_storage_;
  jubatus::util::lang::shared_ptr<recommender::recommender_base> nn_engine_;
  jubatus::util::lang::shared_ptr<unlearner::unlearner_base> unlearner_;
};

}  //  namespace anomaly
}  //  namespace core
}  //  namespace jubatus

#endif  // JUBATUS_CORE_ANOMALY_LOF_HPP_
