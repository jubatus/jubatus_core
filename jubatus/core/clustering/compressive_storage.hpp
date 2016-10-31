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

#ifndef JUBATUS_CORE_CLUSTERING_COMPRESSIVE_STORAGE_HPP_
#define JUBATUS_CORE_CLUSTERING_COMPRESSIVE_STORAGE_HPP_

#include <string>
#include <vector>
#include <msgpack.hpp>
#include "storage.hpp"

namespace jubatus {
namespace core {
namespace clustering {
namespace compressor {
class compressor;
}  // namespace compressor
class compressive_storage : public storage {
 public:
  struct config {
    config()
      : bucket_size(10000),
        bucket_length(2),
        compressed_bucket_size(200),
        bicriteria_base_size(10),
        forgetting_factor(2.0),
        forgetting_threshold(0.05),
        seed(0) {
    }
    int bucket_size;
    int bucket_length;
    int compressed_bucket_size;
    int bicriteria_base_size;
    double forgetting_factor;
    double forgetting_threshold;
    int seed;

    MSGPACK_DEFINE(bucket_size,
                   bucket_length,
                   compressed_bucket_size,
                   bicriteria_base_size,
                   forgetting_factor,
                   forgetting_threshold,
                   seed);

    template<typename Ar>
    void serialize(Ar& ar) {
      ar & JUBA_MEMBER(bucket_size)
        & JUBA_MEMBER(bucket_length)
        & JUBA_MEMBER(compressed_bucket_size)
        & JUBA_MEMBER(bicriteria_base_size)
        & JUBA_MEMBER(forgetting_factor)
        & JUBA_MEMBER(forgetting_threshold)
        & JUBA_MEMBER(seed);
    }
  };

  compressive_storage(
      const std::string& name,
      const int bucket_size,
      const int bucket_length,
      const int compressed_bucket_size,
      const int bicriteria_base_size,
      const double forgetting_factor,
      const double forgetting_threshold);

  void add(const weighted_point& point);
  wplist get_mine() const;
  void set_compressor(
      jubatus::util::lang::shared_ptr<compressor::compressor> compressor);

 private:
  void carry_up(size_t r);
  bool is_next_bucket_full(size_t bucket_number);
  bool reach_forgetting_threshold(size_t bucket_number);
  void forget_weight(wplist& points);

  void pack_impl_(framework::packer& packer) const;
  void unpack_impl_(msgpack::object o);
  void clear_impl_();

  std::vector<wplist> mine_;
  jubatus::util::lang::shared_ptr<compressor::compressor> compressor_;
  int bucket_size_;
  int bucket_length_;
  int compressed_bucket_size_;
  int bicriteria_base_size_;
  double forgetting_factor_;
  double forgetting_threshold_;
  uint64_t status_;
};

}  // namespace clustering
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_CLUSTERING_COMPRESSIVE_STORAGE_HPP_
