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

#ifndef JUBATUS_CORE_NEAREST_NEIGHBOR_BIT_VECTOR_RANKING_HPP_
#define JUBATUS_CORE_NEAREST_NEIGHBOR_BIT_VECTOR_RANKING_HPP_

#include <stdint.h>
#include <utility>
#include <vector>

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

void ranking_hamming_bit_vectors(
    const storage::bit_vector& query,
    const storage::const_bit_vector_column& bvs,
    std::vector<std::pair<uint64_t, float> >& ret,
    uint64_t ret_num);

}  // namespace nearest_neighbor
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_NEAREST_NEIGHBOR_BIT_VECTOR_RANKING_HPP_
