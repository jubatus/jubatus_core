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
#include <queue>
#include "bit_vector_ranking.hpp"
#include "../storage/abstract_column.hpp"
#include "../storage/fixed_size_heap.hpp"
#include "jubatus/util/concurrent/thread.h"
#include "jubatus/util/concurrent/lock.h"
#include "jubatus/util/concurrent/condition.h"
#include "jubatus/util/lang/bind.h"
#include "jubatus/util/lang/function.h"
#include "jubatus/util/lang/shared_ptr.h"

using std::make_pair;
using std::pair;
using std::vector;
using jubatus::core::storage::bit_vector;
using jubatus::core::storage::const_bit_vector_column;

namespace jubatus {
namespace core {
namespace nearest_neighbor {
namespace detail {
inline void yield() {
#ifdef __linux__
  sched_yield();
#else
  (void)sleep(0);
#endif
}
}  // namespace detail

struct bvs_task {
  const bit_vector& query;
  const_bit_vector_column& bvs;
  const uint64_t start;
  const uint64_t finish;
  volatile bool finished;
  bool is_finished() const {
    return finished;
  }
  storage::fixed_size_heap<pair<uint32_t, uint64_t> > result;
  util::lang::function<void(util::lang::shared_ptr<bvs_task>)> task;
  bvs_task(const bit_vector& q,
           const_bit_vector_column& b,
           uint64_t s,
           uint64_t f,
           uint64_t l)
    : query(q),
      bvs(b),
      start(s),
      finish(f),
      finished(false),
      result(l),
      task(bvs_work)
  {}
};

void bvs_work(util::lang::shared_ptr<bvs_task> desc) {
  const bit_vector& query = desc->query;
  const_bit_vector_column& bvs = desc->bvs;
  const uint64_t start = desc->start;
  const uint64_t finish = desc->finish;

  for (uint64_t i = start; i < finish; ++i) {
    const size_t dist = query.calc_hamming_distance(bvs[i]);
    desc->result.push(make_pair(dist, i));
  }
  desc->finished = true;
}

void bit_vector_ranker::ranking_hamming_bit_vectors(
    const bit_vector& query,
    const const_bit_vector_column& bvs,
    vector<pair<uint64_t, float> >& ret,
    uint64_t ret_num) {
  std::vector<util::lang::shared_ptr<bvs_task> > tasks;
  for (uint64_t i = 0; i < bvs.size(); i += 10000) {
    uint64_t til = std::min(bvs.size(), i + 10000);
    util::lang::shared_ptr<bvs_task> new_task
      (new bvs_task(query, bvs, i, til, ret_num));
    tasks.push_back(new_task);
    workers_.add_task(new_task, &bvs_work);
  }

  storage::fixed_size_heap<pair<uint32_t, uint64_t> > heap(ret_num);
  std::vector<pair<uint32_t, uint64_t> > result;
  for (size_t i = 0; i < tasks.size(); ++i) {
    __sync_synchronize();
    if (!tasks[i]->is_finished()) {
      --i;
      detail::yield();
      continue;
    }
    tasks[i]->result.get_sorted(result);
    for (size_t i = 0; i < result.size(); ++i) {
      heap.push(result[i]);
    }
  }

  vector<pair<uint32_t, uint64_t> > sorted;
  heap.get_sorted(sorted);

  ret.clear();
  ret.reserve(sorted.size());
  const float denom = query.bit_num();
  for (size_t i = 0; i < sorted.size(); ++i) {
    ret.push_back(make_pair(sorted[i].second, sorted[i].first / denom));
  }
}

}  // namespace nearest_neighbor
}  // namespace core
}  // namespace jubatus
