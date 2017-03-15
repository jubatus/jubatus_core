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

#ifndef JUBATUS_CORE_REGRESSION_INVERTED_INDEX_REGRESSION_HPP_
#define JUBATUS_CORE_REGRESSION_INVERTED_INDEX_REGRESSION_HPP_

#include <stdint.h>


#include <map>
#include <string>
#include <vector>
#include "jubatus/util/math/random.h"
#include "jubatus/util/concurrent/lock.h"
#include "jubatus/util/concurrent/mutex.h"
#include "jubatus/util/concurrent/rwmutex.h"
#include "jubatus/util/data/optional.h"
#include "../common/type.hpp"
#include "../framework/packer.hpp"
#include "../framework/mixable.hpp"
#include "../unlearner/unlearner_base.hpp"
#include "../common/jsonconfig.hpp"
#include "../storage/labels.hpp"
#include "../storage/inverted_index_storage.hpp"
#include "../framework/mixable_versioned_table.hpp"
#include "regression_base.hpp"

namespace jubatus {
namespace core {
namespace regression {

class inverted_index_regression : public regression_base {
 public:
  struct config {
    config();
    int nearest_neighbor_num;
    jubatus::util::data::optional<std::string> weight;

    template<typename Ar>
    void serialize(Ar& ar) {
      ar & JUBA_MEMBER(nearest_neighbor_num)
         & JUBA_MEMBER(weight);
    }
  };

  explicit inverted_index_regression(const config& config);
  void train(const common::sfv_t& fv, const float value);
  virtual float estimate(const common::sfv_t& fv) const = 0;
  std::string name() const;

  void get_status(std::map<std::string, std::string>& status) const;

  void pack(framework::packer& pk) const;
  void unpack(msgpack::object o);
  void clear();

  std::vector<framework::mixable*> get_mixables();

 protected:
  jubatus::util::lang::shared_ptr<storage::mixable_inverted_index_storage>
      mixable_storage_;
  jubatus::util::lang::shared_ptr<framework::mixable_versioned_table> values_;
  // A map from label to number of records that belongs to the label.
  config config_;
  mutable jubatus::util::concurrent::rw_mutex storage_mutex_;
  jubatus::util::concurrent::mutex rand_mutex_;
  jubatus::util::math::random::mtrand rand_;
};

}  // namespace regression
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_REGRESSION_INVERTED_INDEX_REGRESSION_HPP_
