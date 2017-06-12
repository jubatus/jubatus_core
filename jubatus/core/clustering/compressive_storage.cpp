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

#include "compressive_storage.hpp"

#include <string>
#include <vector>
#include "compressor.hpp"
#include "gmm_compressor.hpp"
#include "kmeans_compressor.hpp"

namespace jubatus {
namespace core {
namespace clustering {

compressive_storage::compressive_storage(
    const std::string& name,
    const int bucket_size,
    const int bucket_length,
    const int compressed_bucket_size,
    const int bicriteria_base_size,
    const double forgetting_factor,
    const double forgetting_threshold)
  : storage(name),
    bucket_size_(bucket_size),
    bucket_length_(bucket_length),
    compressed_bucket_size_(compressed_bucket_size),
    bicriteria_base_size_(bicriteria_base_size),
    forgetting_factor_(forgetting_factor),
    forgetting_threshold_(forgetting_threshold),
    status_(0) {
    if (!(1 <= bucket_size)) {
      throw JUBATUS_EXCEPTION(
          common::invalid_parameter("1 <= bucket_size"));
    }

    if (!(2 <= bucket_length)) {
      throw JUBATUS_EXCEPTION(
          common::invalid_parameter("2 <= bucket_length"));
    }

    if (!(1 <= bicriteria_base_size &&
          bicriteria_base_size <= compressed_bucket_size)) {
      throw JUBATUS_EXCEPTION(common::invalid_parameter(
          "1 <= bicriteria_base_size <= compressed_bucket_size"));
    }

    if (!(compressed_bucket_size <= bucket_size)) {
      throw JUBATUS_EXCEPTION(
          common::invalid_parameter("compressed_bucket_size <= bucket_size"));
    }

    if (!(0.0 <= forgetting_factor)) {
      throw JUBATUS_EXCEPTION(
          common::invalid_parameter("0.0 <= forgetting_factor"));
    }

    if (!(0.0 <= forgetting_threshold &&
          forgetting_threshold <= 1.0)) {
      throw JUBATUS_EXCEPTION(common::invalid_parameter(
          "0.0 <= forgetting_threshold <= 1.0"));
    }

  mine_.push_back(wplist());
}

void compressive_storage::set_compressor(
    jubatus::util::lang::shared_ptr<compressor::compressor> compressor) {
  compressor_ = compressor;
}

void compressive_storage::add(const weighted_point& point) {
  wplist& c0 = mine_[0];
  c0.push_back(point);
  if (c0.size() >= static_cast<size_t>(bucket_size_)) {
    wplist cr;
    compressor_->compress(
        c0, bicriteria_base_size_, compressed_bucket_size_, cr);
    c0.swap(cr);
    status_ += 1;
    carry_up(0);

    increment_revision();
  }
}

wplist compressive_storage::get_mine() const {
  wplist ret;
  for (std::vector<wplist>::const_iterator it = mine_.begin();
      it != mine_.end(); ++it) {
    concat(*it, ret);
  }
  return ret;
}

void compressive_storage::forget_weight(wplist& points) {
  double factor = std::exp(-forgetting_factor_);
  typedef wplist::iterator iter;
  for (iter it = points.begin(); it != points.end(); ++it) {
    it->weight *= factor;
  }
}

bool compressive_storage::reach_forgetting_threshold(size_t bucket_number) {
  double C = forgetting_threshold_;
  double lam = forgetting_factor_;
  if (std::exp(-lam * bucket_number) < C) {
    return true;
  }
  return false;
}

bool compressive_storage::is_next_bucket_full(size_t bucket_number) {
  return digit(status_ - 1, bucket_number, bucket_length_) ==
      bucket_length_ - 1;
}

void compressive_storage::carry_up(size_t r) {
  if (r >= mine_.size() - 1) {
    mine_.push_back(wplist());
  }
  forget_weight(mine_[r]);
  if (!is_next_bucket_full(r)) {
    if (!reach_forgetting_threshold(r + 1) ||
        mine_[r].size() == get_mine().size()) {
      concat(mine_[r], mine_[r + 1]);
      mine_[r].clear();
    } else {
      mine_[r + 1].swap(mine_[r]);
      mine_[r].clear();
    }
  } else {
    wplist cr = mine_[r];
    wplist crr = mine_[r + 1];
    mine_[r].clear();
    mine_[r + 1].clear();
    concat(cr, crr);
    size_t dstsize = (r == 0) ? compressed_bucket_size_ :
        2 * r * r * compressed_bucket_size_;
    compressor_->compress(crr, bicriteria_base_size_,
                          dstsize, mine_[r + 1]);
    carry_up(r + 1);
  }
}

void compressive_storage::pack_impl_(framework::packer& packer) const {
  packer.pack_array(3);
  storage::pack_impl_(packer);
  packer.pack(mine_);
  packer.pack(status_);
}

void compressive_storage::unpack_impl_(msgpack::object o) {
  std::vector<msgpack::object> mems;
  o.convert(mems);
  if (mems.size() != 3) {
    throw msgpack::type_error();
  }
  storage::unpack_impl_(mems[0]);
  mems[1].convert(mine_);
  mems[2].convert(status_);
}

void compressive_storage::clear_impl_() {
  storage::clear_impl_();
  mine_.clear();
  mine_.push_back(wplist());
  status_ = 0;
}

}  // namespace clustering
}  // namespace core
}  // namespace jubatus
