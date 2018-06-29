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

#include <functional>
#include <utility>
#include <vector>
#include "bit_vector_ranking.hpp"
#include "../storage/abstract_column.hpp"
#include "../storage/fixed_size_heap.hpp"

using std::make_pair;
using std::pair;
using std::vector;
using jubatus::core::storage::bit_vector;
using jubatus::core::storage::const_bit_vector_column;

typedef jubatus::core::storage::fixed_size_heap<
  pair<uint32_t, uint64_t> > heap_t;

namespace jubatus {
namespace core {
namespace nearest_neighbor {

static heap_t ranking_hamming_bit_vectors_worker(
    const bit_vector *query, const_bit_vector_column *bvs,
    uint64_t ret_num, size_t off, size_t end) {
  heap_t heap(ret_num);
  for (uint64_t i = off; i < end; ++i) {
    const size_t dist = query->calc_hamming_distance_unsafe(
      bvs->get_data_at_unsafe(i));
    heap.push(make_pair(dist, i));
  }
  return heap;
}

void ranking_hamming_bit_vectors(
    const bit_vector& query,
    const const_bit_vector_column& bvs,
    vector<pair<uint64_t, double> >& ret,
    uint64_t ret_num, uint32_t threads) {
  ret.clear();
  if (bvs.size() == 0) {
    return;
  }
  heap_t heap(ret_num);
  jubatus::util::lang::function<heap_t(size_t, size_t)> f =
    jubatus::util::lang::bind(
      &ranking_hamming_bit_vectors_worker,
      &query, &bvs, ret_num, jubatus::util::lang::_1, jubatus::util::lang::_2);
  ranking_hamming_bit_vectors_internal(f, bvs.size(), threads, heap);

  vector<pair<uint32_t, uint64_t> > sorted;
  heap.get_sorted(sorted);

  const double denom = query.bit_num();
  for (size_t i = 0; i < sorted.size(); ++i) {
    ret.push_back(make_pair(sorted[i].second, sorted[i].first / denom));
  }
}

}  // namespace nearest_neighbor
}  // namespace core
}  // namespace jubatus
