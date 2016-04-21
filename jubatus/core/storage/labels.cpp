// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2016 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include "labels.hpp"

#include <algorithm>
#include <string>

#include "jubatus/util/concurrent/lock.h"

namespace jubatus {
namespace core {
namespace storage {

labels::labels() {
}

labels::~labels() {
}

bool labels::add(const std::string& label) {
  {
    util::concurrent::scoped_rlock lock(mutex_);
    if (master_.find(label) != master_.end()) {
      return false;
    }

    if (diff_.find(label) != diff_.end()) {
      return false;
    }
  }

  util::concurrent::scoped_wlock lock(mutex_);
  diff_[label] = 0;
  return true;
}

labels::data_t labels::get_labels() const {
  util::concurrent::scoped_rlock lock(mutex_);

  data_t result(master_);
  for (data_t::const_iterator it = diff_.begin();
      it != diff_.end(); ++it) {
    result[it->first] += it->second;
  }

  return result;
}

void labels::increment(const std::string& label) {
  util::concurrent::scoped_wlock lock(mutex_);
  diff_[label] += 1;
}

void labels::decrement(const std::string& label) {
  util::concurrent::scoped_wlock lock(mutex_);
  diff_[label] -= 1;
  if (diff_[label] <= 0) {
    diff_.erase(label);
  }
}

bool labels::erase(const std::string& label) {
  util::concurrent::scoped_wlock lock(mutex_);
  bool result = diff_.erase(label);
  result |= master_.erase(label);
  return result;
}

void labels::clear() {
  util::concurrent::scoped_wlock lock(mutex_);
  data_t().swap(diff_);
  data_t().swap(master_);
}

void labels::swap(data_t& labels) {
  util::concurrent::scoped_wlock lock(mutex_);
  labels.swap(diff_);
  data_t().swap(master_);
}

void labels::get_diff(data_t& diff) const {
  util::concurrent::scoped_rlock lock(mutex_);
  diff = diff_;
}

bool labels::put_diff(const data_t& mixed_diff) {
  util::concurrent::scoped_wlock lock(mutex_);

  for (data_t::const_iterator it = mixed_diff.begin();
      it != mixed_diff.end(); ++it) {
    master_[it->first] += it->second;
  }

  data_t().swap(diff_);
  version_.increment();

  return true;
}

void labels::mix(const data_t& lhs, data_t& rhs) const {
  for (data_t::const_iterator it = lhs.begin();
      it != lhs.end(); ++it) {
    rhs[it->first] += it->second;
  }
}

void labels::pack(framework::packer& packer) const {
  util::concurrent::scoped_rlock lock(mutex_);
  packer.pack(*this);
}

void labels::unpack(msgpack::object o) {
  util::concurrent::scoped_wlock lock(mutex_);
  o.convert(this);
}

}  // namespace storage
}  // namespace core
}  // namespace jubatus
