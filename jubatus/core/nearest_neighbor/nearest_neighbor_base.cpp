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

#include <string>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "nearest_neighbor_base.hpp"
#include "../storage/column_table.hpp"
#include "jubatus/util/concurrent/rwmutex.h"

using std::pair;
using std::string;
using std::vector;
using jubatus::util::lang::shared_ptr;

namespace jubatus {
namespace core {
namespace nearest_neighbor {

nearest_neighbor_base::nearest_neighbor_base(
    shared_ptr<storage::column_table> table,
    const std::string& id)
    : my_id_(id),
      mixable_table_(new framework::mixable_versioned_table) {
  mixable_table_->set_model(table);
}

void nearest_neighbor_base::get_all_row_ids(vector<string>& ids) const {
  vector<string> ret;
  shared_ptr<const storage::column_table> table = get_const_table();
  util::concurrent::scoped_rlock lk(table->get_mutex());

  ret.reserve(table->size());
  for (size_t i = 0; i < table->size(); ++i) {
    ret.push_back(table->get_key_nolock(i));
  }
  ret.swap(ids);
}

void nearest_neighbor_base::clear() {
  mixable_table_->get_model()->clear();  // lock acquired inside
}

void nearest_neighbor_base::similar_row(
    const common::sfv_t& query,
    vector<pair<string, float> >& ids,
    uint64_t ret_num) const {
  neighbor_row(query, ids, ret_num);  // lock acquired inside
  for (size_t i = 0; i < ids.size(); ++i) {
    ids[i].second = calc_similarity(ids[i].second);
  }
}

void nearest_neighbor_base::similar_row(
    const string& query_id,
    vector<pair<string, float> >& ids,
    uint64_t ret_num) const {
  neighbor_row(query_id, ids, ret_num);  // lock acquired inside
  for (size_t i = 0; i < ids.size(); ++i) {
    ids[i].second = calc_similarity(ids[i].second);
  }
}

void nearest_neighbor_base::pack(framework::packer& packer) const {
  get_const_table()->pack(packer);  // lock acquired inside
}

void nearest_neighbor_base::unpack(msgpack::object o) {
  get_table()->unpack(o);  // lock acquired inside
}

framework::mixable* nearest_neighbor_base::get_mixable() const {
  return mixable_table_.get();
}

}  // namespace nearest_neighbor
}  // namespcae core
}  // namespace jubatus
