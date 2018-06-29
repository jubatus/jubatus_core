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

#include "euclid_lsh.hpp"

#include <map>
#include <string>
#include <vector>
#include <utility>
#include <cmath>
#include "jubatus/util/lang/cast.h"
#include "../storage/fixed_size_heap.hpp"
#include "../storage/column_table.hpp"
#include "lsh_function.hpp"
#include "bit_vector_ranking.hpp"

using std::map;
using std::pair;
using std::make_pair;
using std::string;
using std::vector;
using jubatus::util::lang::lexical_cast;
using jubatus::core::storage::column_table;
using jubatus::core::storage::column_type;
using jubatus::core::storage::owner;
using jubatus::core::storage::bit_vector;
using jubatus::core::storage::const_bit_vector_column;
using jubatus::core::storage::const_double_column;

typedef jubatus::core::storage::fixed_size_heap<pair<double, size_t> > heap_t;

namespace jubatus {
namespace core {
namespace nearest_neighbor {
namespace {

double squared_l2norm(const common::sfv_t& sfv) {
  double sqnorm = 0;
  for (size_t i = 0; i < sfv.size(); ++i) {
    sqnorm += sfv[i].second * sfv[i].second;
  }
  return sqnorm;
}

double l2norm(const common::sfv_t& sfv) {
  return std::sqrt(squared_l2norm(sfv));
}

}  // namespace

euclid_lsh::euclid_lsh(
    const config& conf,
    jubatus::util::lang::shared_ptr<column_table> table,
    const std::string& id)
    : nearest_neighbor_base(table, id) {
  set_config(conf);

  vector<column_type> schema;
  fill_schema(schema);
  get_table()->init(schema);
}

euclid_lsh::euclid_lsh(
    const config& conf,
    jubatus::util::lang::shared_ptr<column_table> table,
    vector<column_type>& schema,
    const std::string& id)
    : nearest_neighbor_base(table, id) {
  set_config(conf);
  fill_schema(schema);
}

void euclid_lsh::set_row(const string& id, const common::sfv_t& sfv) {
  // TODO(beam2d): support nested algorithm, e.g. when used by lof and then we
  // cannot suppose that the first two columns are assigned to euclid_lsh.
  get_table()->add(id, owner(my_id_),
                   cosine_lsh(sfv, hash_num_, threads_, cache_), l2norm(sfv));
}

void euclid_lsh::neighbor_row(
    const common::sfv_t& query,
    vector<pair<string, double> >& ids,
    uint64_t ret_num) const {
  util::concurrent::scoped_rlock lk(get_const_table()->get_mutex());

  /* table lock acquired; all subsequent table operations must be nolock */

  neighbor_row_from_hash(
      cosine_lsh(query, hash_num_, threads_, cache_),
      l2norm(query),
      ids,
      ret_num);
}

void euclid_lsh::neighbor_row(
    const std::string& query_id,
    vector<pair<string, double> >& ids,
    uint64_t ret_num) const {
  util::concurrent::scoped_rlock lk(get_const_table()->get_mutex());

  /* table lock acquired; all subsequent table operations must be nolock */

  const pair<bool, uint64_t> maybe_index =
      get_const_table()->exact_match_nolock(query_id);
  if (!maybe_index.first) {
    ids.clear();
    return;
  }

  const bit_vector bv = lsh_column()[maybe_index.second];
  const double norm = norm_column()[maybe_index.second];
  neighbor_row_from_hash(bv, norm, ids, ret_num);
}

void euclid_lsh::set_config(const config& conf) {
  if (!(1 <= conf.hash_num)) {
    throw JUBATUS_EXCEPTION(
        common::invalid_parameter("1 <= hash_num"));
  }
  hash_num_ = conf.hash_num;
  threads_ = read_threads_config(conf.threads);
  init_cache_from_config(cache_, conf.cache_size);
}

void euclid_lsh::fill_schema(vector<column_type>& schema) {
  first_column_id_ = schema.size();
  schema.push_back(column_type(column_type::bit_vector_type, hash_num_));
  schema.push_back(column_type(column_type::double_type));
}

const_bit_vector_column& euclid_lsh::lsh_column() const {
  return get_const_table()->get_bit_vector_column(first_column_id_);
}

const_double_column& euclid_lsh::norm_column() const {
  return get_const_table()->get_double_column(first_column_id_ + 1);
}

static heap_t ranking_hamming_bit_vectors_worker(
    const bit_vector *bv, const_bit_vector_column *bv_col,
    const_double_column *norm_col, double denom, double norm,
    uint64_t ret_num, size_t off, size_t end) {
  heap_t heap(ret_num);
  for (size_t i = off; i < end; ++i) {
    const size_t hamm_dist =
      bv->calc_hamming_distance_unsafe(bv_col->get_data_at_unsafe(i));
    const double norm_i = (*norm_col)[i];
    double score;
    if (hamm_dist == 0) {
      score = std::fabs(norm - norm_i);
    } else {
      const double theta = hamm_dist * M_PI / denom;
      score = std::sqrt(
          norm * norm + norm_i * norm_i - 2 * norm * norm_i * std::cos(theta));
    }
    heap.push(make_pair(score, i));
  }
  return heap;
}

void euclid_lsh::neighbor_row_from_hash(
    const bit_vector& bv,
    double norm,
    vector<pair<string, double> >& ids,
    uint64_t ret_num) const {
  // This function is not thread safe.
  // Take lock out of this function.
  jubatus::util::lang::shared_ptr<const column_table> table =
    get_const_table();
  ids.clear();
  if (table->size_nolock() == 0) {
    return;
  }
  const_bit_vector_column& bv_col = lsh_column();
  const_double_column& norm_col = norm_column();
  const double denom = bv.bit_num();
  heap_t heap(ret_num);
  jubatus::util::lang::function<heap_t(size_t, size_t)> f =
    jubatus::util::lang::bind(
      &ranking_hamming_bit_vectors_worker, &bv, &bv_col, &norm_col,
      denom, norm, ret_num,
      jubatus::util::lang::_1, jubatus::util::lang::_2);
  ranking_hamming_bit_vectors_internal(
      f, table->size_nolock(), threads_, heap);

  vector<pair<double, size_t> > sorted;
  heap.get_sorted(sorted);

  for (size_t i = 0; i < sorted.size(); ++i) {
    ids.push_back(make_pair(
      table->get_key_nolock(sorted[i].second), sorted[i].first));
  }
}

}  // namespace nearest_neighbor
}  // namespace core
}  // namespace jubatus
