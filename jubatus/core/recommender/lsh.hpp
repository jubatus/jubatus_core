// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_RECOMMENDER_LSH_HPP_
#define JUBATUS_CORE_RECOMMENDER_LSH_HPP_

#include <string>
#include <utility>
#include <vector>
#include "jubatus/util/data/serialization.h"
#include "jubatus/util/data/optional.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "recommender_base.hpp"
#include "../unlearner/unlearner_base.hpp"
#include "../common/jsonconfig.hpp"

namespace jubatus {
namespace core {
namespace framework {
template <typename Model, typename Diff>
class linear_mixable_helper;
}  // namespace framework
namespace storage {
class bit_index_storage;
typedef framework::linear_mixable_helper<bit_index_storage, bit_table_t>
    mixable_bit_index_storage;
}  // namespace storage
namespace recommender {

class lsh : public recommender_base {
 public:
  struct config {
    config();

    int64_t hash_num;

    util::data::optional<std::string> unlearner;
    util::data::optional<core::common::jsonconfig::config> unlearner_parameter;

    template<typename Ar>
    void serialize(Ar& ar) {
      ar & JUBA_MEMBER(hash_num) &
          JUBA_MEMBER(unlearner) & JUBA_MEMBER(unlearner_parameter);
    }
  };

  explicit lsh(uint64_t hash_num);
  explicit lsh(const config& config);
  lsh();
  ~lsh();

  void similar_row(
      const common::sfv_t& query,
      std::vector<std::pair<std::string, float> >& ids,
      size_t ret_num) const;
  void neighbor_row(
      const common::sfv_t& query,
      std::vector<std::pair<std::string, float> >& ids,
      size_t ret_num) const;
  void clear();
  void clear_row(const std::string& id);
  void remove_row(const std::string& id);
  void update_row(const std::string& id, const sfv_diff_t& diff);
  void get_all_row_ids(std::vector<std::string>& ids) const;
  std::string type() const;

  framework::mixable* get_mixable() const;

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

 private:
  void calc_lsh_values(const common::sfv_t& sfv, storage::bit_vector& bv) const;
  void generate_column_base(const std::string& column);
  void generate_column_bases(const common::sfv_t& v);

  void initialize_model();

  // bases for lsh
  jubatus::util::data::unordered_map<std::string, std::vector<float> >
      column2baseval_;

  jubatus::util::lang::shared_ptr<storage::mixable_bit_index_storage>
      mixable_storage_;

  jubatus::util::lang::shared_ptr<unlearner::unlearner_base>
      unlearner_;

  const uint64_t hash_num_;
};

}  // namespace recommender
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_RECOMMENDER_LSH_HPP_
