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

template <typename T>
class blocking_queue : public util::lang::noncopyable {
 public:
  blocking_queue() {}
  void enqueue(const T& item) {
    {
      util::concurrent::scoped_lock lk(lk_);
      const bool was_empty = queue_.empty();
      queue_.push(item);
      if (was_empty) {
        empty_wait_.notify_all();  // notify_one() may be suitable?
      }
    }
  }

  T dequeue() {
    while (true) {
      util::concurrent::scoped_lock lk(lk_);
      if (queue_.empty()) {  // if empty
        empty_wait_.wait(lk_);  // unlock
        //  relock
      }
      if (queue_.empty()) {
        continue;
      }

      T result = queue_.front();
      queue_.pop();
      return result;
    }
  }

  size_t size() const {
    util::concurrent::scoped_lock lk(lk_);
    return queue_.size();
  }

 private:
  mutable util::concurrent::mutex lk_;
  mutable util::concurrent::condition empty_wait_;
  std::queue<T> queue_;
};

template <typename Arg>
void task(int worker_id, blocking_queue<Arg>* queue, volatile bool& terminate) {
  while (!terminate) {
    Arg a = queue->dequeue();
    a->task(a);
    a->finished = true;
  }
}

template <typename Arg>
class thread_pool {
 public:
  thread_pool(int num)
    : threads_(), terminate_(false)
  {
    threads_.reserve(num);
    for (int i = 0; i < num; ++i) {
      threads_.push_back(util::lang::shared_ptr<util::concurrent::thread>(
                             new util::concurrent::thread(
                               util::lang::bind(task<Arg>, i, &queue_, terminate_)
                             )));
      threads_.back()->start();
    }
  }
  void add_task(Arg& arg) {
    queue_.enqueue(arg);
  }
  ~thread_pool() {
  }
 private:
  blocking_queue<Arg> queue_;
  std::vector<util::lang::shared_ptr<util::concurrent::thread> > threads_;
  volatile bool terminate_;
};

}  // namespace detail

struct bvs_task;
void bvs_work(util::lang::shared_ptr<bvs_task> desc);

struct bvs_task {
  const bit_vector& query;
  const_bit_vector_column& bvs;
  const uint64_t start;
  const uint64_t finish;
  volatile bool finished;
  storage::fixed_size_heap<pair<uint32_t, uint64_t> > result;
  util::lang::function<void(util::lang::shared_ptr<bvs_task>)> task;
  bool is_finished() const {
    return finished;
  }
  bvs_task(const bit_vector& q,
           const_bit_vector_column& b,
           uint64_t s,
           uint64_t f,
           uint64_t len)
    : query(q),
      bvs(b),
      start(s),
      finish(f),
      finished(false),
      result(len),
      task(bvs_work)
    {}
};

void bvs_work(util::lang::shared_ptr<bvs_task> desc) {
  const bit_vector& query = desc->query;
  const_bit_vector_column& bvs = desc->bvs;
  const uint64_t start = desc->start;
  const uint64_t finish = desc->finish;
  storage::fixed_size_heap<pair<uint32_t, uint64_t> >& result = desc->result;

  for (uint64_t i = start; i < finish; ++i) {
    const size_t dist = query.calc_hamming_distance(bvs[i]);
    result.push(make_pair(dist, i));
  }
}

void ranking_hamming_bit_vectors(
    const bit_vector& query,
    const const_bit_vector_column& bvs,
    vector<pair<uint64_t, float> >& ret,
    uint64_t ret_num) {
  const int thread_num = 8;
  static detail::thread_pool<util::lang::shared_ptr<bvs_task> >
    threads(thread_num);

  std::vector<util::lang::shared_ptr<bvs_task> > tasks;
  for (uint64_t i = 0; i < bvs.size(); i += 10000) {
    uint64_t til = std::min(bvs.size(), i + 10000);
    util::lang::shared_ptr<bvs_task> new_task
      (new bvs_task(query, bvs, i, til, ret_num));
    tasks.push_back(new_task);
    threads.add_task(new_task);
  }

  storage::fixed_size_heap<pair<uint32_t, uint64_t> > heap(ret_num);
  std::vector<pair<uint32_t, uint64_t> > result;
  for (size_t i = 0; i < tasks.size(); ++i) {
    // need memory fence
    if (!tasks[i]->is_finished()) {
      --i;
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
  const float denom = query.bit_num();
  for (size_t i = 0; i < sorted.size(); ++i) {
    ret.push_back(make_pair(sorted[i].second, sorted[i].first / denom));
  }
}

}  // namespace nearest_neighbor
}  // namespace core
}  // namespace jubatus
