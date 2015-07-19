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

#ifndef JUBATUS_CORE_UNLEARNER_LRU_UNLEARNER_HPP_
#define JUBATUS_CORE_UNLEARNER_LRU_UNLEARNER_HPP_

#include <stdint.h>
#include <list>
#include <string>
#include "jubatus/util/data/serialization.h"
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/data/unordered_set.h"
#include "jubatus/util/data/optional.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "unlearner_base.hpp"
#include "../common/export_model.hpp"
#include "../common/unordered_map.hpp"
#include "unlearner_config.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {
class key_matcher;
}  // namespace fv_converter
namespace unlearner {

// Unlearner based on Least Recently Used algorithm.
class lru_unlearner : public unlearner_base {
 public:
  struct lru_unlearner_config : public unlearner_config_base {
    int32_t max_size;
    jubatus::util::data::optional<std::string> sticky_pattern;

    template<typename Ar>
    void serialize(Ar& ar) {
      ar & JUBA_MEMBER(name)
          & JUBA_MEMBER(max_size)
          & JUBA_MEMBER(sticky_pattern);
    }
  };

  std::string type() const {
    return "lru_unlearner";
  }

  void clear() {
    lru_.clear();
    entry_map_.clear();
  }

  explicit lru_unlearner(const unlearner_config_base& conf);

  bool can_touch(const std::string& id);
  bool touch(const std::string& id);
  bool remove(const std::string& id);
  bool exists_in_memory(const std::string& id) const;

  void export_model(framework::packer& pk) const;
  void import_model(msgpack::object o);

 private:
  typedef std::list<std::string> lru;
  typedef jubatus::util::data::unordered_map<std::string, lru::iterator>
      entry_map;

  void rebuild_entry_map();

  lru lru_;
  entry_map entry_map_;
  jubatus::util::data::unordered_set<std::string> sticky_ids_;
  size_t max_size_;
  jubatus::util::lang::shared_ptr<jubatus::core::fv_converter::key_matcher>
      sticky_matcher_;
};

}  // namespace unlearner
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_UNLEARNER_LRU_UNLEARNER_HPP_
