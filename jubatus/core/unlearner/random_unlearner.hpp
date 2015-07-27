// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2013 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_UNLEARNER_RANDOM_UNLEARNER_HPP_
#define JUBATUS_CORE_UNLEARNER_RANDOM_UNLEARNER_HPP_

#include <string>
#include <vector>
#include <map>
#include "jubatus/util/data/optional.h"
#include "jubatus/util/data/serialization.h"
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/math/random.h"
#include "unlearner_base.hpp"

namespace jubatus {
namespace core {
namespace unlearner {

// Unlearner that chooses an item to be removed by uniformly random sampling.
class random_unlearner : public unlearner_base {
 public:
  struct config {
    int32_t max_size;
    jubatus::util::data::optional<int64_t> seed;

    template<typename Ar>
    void serialize(Ar& ar) {
      ar & JUBA_MEMBER(max_size) & JUBA_MEMBER(seed);
    }
  };

  std::string type() const {
    return "random_unlearner";
  }

  void clear() {
    id_map_.clear();
    ids_.clear();
  }

  explicit random_unlearner(const config& conf);

  bool can_touch(const std::string& id);
  bool touch(const std::string& id);
  bool remove(const std::string& id);
  bool exists_in_memory(const std::string& id) const;
  void get_status(std::map<std::string, std::string>& status) const;

 private:
  /**
   * Map of ID and its position in ids_.
   */
  jubatus::util::data::unordered_map<std::string, size_t> id_map_;

  /**
   * Unlearner ID set.
   */
  std::vector<std::string> ids_;

  /**
   * Maximum size to be hold.
   */
  size_t max_size_;

  /**
   * Random number generator.
   */
  jubatus::util::math::random::mtrand mtr_;
};

}  // namespace unlearner
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_UNLEARNER_RANDOM_UNLEARNER_HPP_

