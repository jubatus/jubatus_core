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

#ifndef JUBATUS_CORE_FV_CONVERTER_COUNTER_HPP_
#define JUBATUS_CORE_FV_CONVERTER_COUNTER_HPP_

#include <ostream>
#include <sstream>

#include "jubatus/util/data/unordered_map.h"
#include "../common/unordered_map.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

template<class T>
class counter {
 public:
  typedef jubatus::util::data::unordered_map<T, double> map_t;
  typedef typename jubatus::util::data::unordered_map<T, double>
      ::const_iterator const_iterator;
  typedef typename jubatus::util::data::unordered_map<T, double>
      ::iterator iterator;

  bool contains(const T& key) const {
    return data_.count(key) != 0;
  }

  double sum() const {
    double sum = 0.0;
    for (const_iterator it = data_.begin(); it != data_.end(); ++it) {
      sum = it->second;
    }
    return sum;
  }

  double operator[](const T& key) const {
    const_iterator it = data_.find(key);
    if (it == data_.end()) {
      return 0;
    } else {
      return it->second;
    }
  }

  double& operator[](const T& key) {
    if (data_.count(key) == 0) {
      data_[key] = 0;
    }
    return data_[key];
  }

  const_iterator begin() const {
    return data_.begin();
  }

  iterator begin() {
    return data_.begin();
  }

  const_iterator end() const {
    return data_.end();
  }

  iterator end() {
    return data_.end();
  }

  void clear() {
    jubatus::util::data::unordered_map<T, double>().swap(data_);
  }

  void add(const counter<T>& counts) {
    for (const_iterator it = counts.begin(); it != counts.end(); ++it) {
      (*this)[it->first] += it->second;
    }
  }

  size_t size() const {
    return data_.size();
  }

  MSGPACK_DEFINE(data_);

  friend std::ostream& operator<<(std::ostream& os, const counter<T>& c) {
    os << "{";
    for (typename
             jubatus::util::data::unordered_map<T, double>::const_iterator it
                 = c.data_.begin();
         it != c.data_.end();
         ++it) {
      os << it->first << ":" << it->second << ", ";
    }
    os << "}";
    return os;
  }

 private:
  jubatus::util::data::unordered_map<T, double> data_;
};

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FV_CONVERTER_COUNTER_HPP_
