// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2015 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_NEAREST_NEIGHBOR_LSH_HASH_WORKER_HPP_
#define JUBATUS_CORE_NEAREST_NEIGHBOR_LSH_HASH_WORKER_HPP_

#include <stdint.h>
#include <utility>
#include <vector>
#include "../common/type.hpp"
#include "../storage/thread_pool.hpp"

namespace jubatus {
namespace core {
namespace storage {
template <typename bit_base> class bit_vector_base;
typedef bit_vector_base<uint64_t> bit_vector;

template <typename T>
class typed_column;
typedef typed_column<bit_vector> bit_vector_column;
typedef const bit_vector_column const_bit_vector_column;
}
namespace nearest_neighbor {

struct hash_task;
void hash_work(util::lang::shared_ptr<hash_task> desc);

class lsh_hash_worker {
 public:
  lsh_hash_worker(uint32_t threads)
    : threads_(threads),
      workers_(threads)
  {}
  void hash(
    const common::sfv_t& sfv,
    uint32_t hash_num,
    storage::bit_vector& result);
 private:
  uint32_t threads_;
  storage::thread_pool<util::lang::shared_ptr<hash_task> > workers_;
};

}  // namespace nearest_neighbor
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_NEAREST_NEIGHBOR_LSH_HASH_WORKER_HPP_
