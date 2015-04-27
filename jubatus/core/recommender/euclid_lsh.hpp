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

#ifndef JUBATUS_CORE_RECOMMENDER_EUCLID_LSH_HPP_
#define JUBATUS_CORE_RECOMMENDER_EUCLID_LSH_HPP_

#include <stdint.h>
#include <utility>
#include <string>
#include <vector>
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/concurrent/lock.h"
#include "jubatus/util/concurrent/mutex.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "jubatus/util/text/json.h"
#include "recommender_base.hpp"

namespace jubatus {
namespace core {
namespace framework {
template <typename Model, typename Diff>
class linear_mixable_helper;
}  // namespace framework
namespace storage {
class lsh_index_storage;
struct lsh_entry;
typedef jubatus::util::data::unordered_map<std::string, lsh_entry>
    lsh_master_table_t;
typedef framework::linear_mixable_helper<lsh_index_storage, lsh_master_table_t>
    mixable_lsh_index_storage;
}  // namespace storage
namespace recommender {

class euclid_lsh : public recommender_base {
 public:
  using recommender_base::similar_row;
  using recommender_base::neighbor_row;

  static const uint64_t DEFAULT_HASH_NUM;
  static const uint64_t DEFAULT_TABLE_NUM;
  static const float DEFAULT_BIN_WIDTH;
  static const uint32_t DEFAULT_NUM_PROBE;
  static const uint32_t DEFAULT_SEED;
  static const bool DEFAULT_RETAIN_PROJECTION;

  struct config {
    config();

    int64_t hash_num;
    int64_t table_num;
    float bin_width;
    int32_t probe_num;
    int32_t seed;
    bool retain_projection;

    template<typename Ar>
    void serialize(Ar& ar) {
      ar
          & JUBA_MEMBER(hash_num)
          & JUBA_MEMBER(table_num)
          & JUBA_MEMBER(bin_width)
          & JUBA_MEMBER(probe_num)
          & JUBA_MEMBER(seed)
          & JUBA_MEMBER(retain_projection);
    }
  };

  euclid_lsh();
  explicit euclid_lsh(const config& config);
  ~euclid_lsh();

  virtual void neighbor_row(
      const common::sfv_t& query,
      std::vector<std::pair<std::string, float> >& ids,
      size_t ret_num) const;
  virtual void neighbor_row(
      const std::string& id,
      std::vector<std::pair<std::string, float> >& ids,
      size_t ret_num) const;

  virtual void similar_row(
      const common::sfv_t& query,
      std::vector<std::pair<std::string, float> >& ids,
      size_t ret_num) const;
  virtual void similar_row(
      const std::string& id,
      std::vector<std::pair<std::string, float> >& ids,
      size_t ret_num) const;

  virtual void clear();
  virtual void clear_row(const std::string& id);
  virtual void update_row(const std::string& id, const sfv_diff_t& diff);
  virtual void get_all_row_ids(std::vector<std::string>& ids) const;

  virtual std::string type() const;

  framework::mixable* get_mixable() const;

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

 private:
  std::vector<float> calculate_lsh(const common::sfv_t& query) const;
  std::vector<float> get_projection(uint32_t seed) const;

  void initialize_model();

  jubatus::util::lang::shared_ptr<storage::mixable_lsh_index_storage>
    mixable_storage_;
  float bin_width_;
  uint32_t num_probe_;

  mutable jubatus::util::data::unordered_map<uint32_t, std::vector<float> >
      projection_cache_;
  mutable jubatus::util::concurrent::mutex cache_lock_;
  bool retain_projection_;
};

}  // namespace recommender
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_RECOMMENDER_EUCLID_LSH_HPP_
