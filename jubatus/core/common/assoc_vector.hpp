// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_COMMON_ASSOC_VECTOR_HPP_
#define JUBATUS_CORE_COMMON_ASSOC_VECTOR_HPP_

#include <algorithm>
#include <utility>
#include <vector>
#include <msgpack.hpp>

namespace jubatus {
namespace core {
namespace common {

template <typename K, typename V>
class assoc_vector {
 public:
  typedef std::vector<std::pair<K, V> > data_type;
  typedef typename data_type::const_iterator const_iterator;
  typedef typename data_type::iterator iterator;

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

  size_t size() const {
    return data_.size();
  }

  bool empty() const {
    return data_.empty();
  }

  const_iterator find(const K& key) const {
    const_iterator it = std::lower_bound(begin(),
                                         end(),
                                         key,
                                         less_pair_and_key());
    if (it != end() && it->first == key) {
      return it;
    } else {
      return end();
    }
  }

  iterator find(const K& key) {
    const_iterator it = const_cast<const assoc_vector&>(*this).find(key);
    return begin() + (it - begin());
  }

  size_t count(const K& key) const {
    if (find(key) == end()) {
      return 0;
    } else {
      return 1;
    }
  }

  iterator erase(const K& key) {
    iterator it = find(key);
    if (it != data_.end()) {
      return erase(it);
    } else {
      return end();
    }
  }

  iterator erase(iterator it) {
    return data_.erase(it);
  }

  V& operator[](const K& key) {
    iterator it = std::lower_bound(begin(), end(), key, less_pair_and_key());
    if (it != data_.end() && it->first == key) {
      return it->second;
    } else {
      it = data_.insert(it, std::make_pair(key, V()));
      return it->second;
    }
  }

  template <typename Packer>
  void msgpack_pack(Packer& pk) const {
    pk.pack_map(data_.size());
    for (std::size_t i = 0; i < data_.size(); ++i) {
      pk.pack(data_[i].first);
      pk.pack(data_[i].second);
    }
  }

  void msgpack_unpack(msgpack::object o) {
    if (o.type != msgpack::type::MAP) {
      throw msgpack::type_error();
    }
    std::vector<std::pair<K, V> > data(o.via.map.size);
    for (std::size_t i = 0; i < data.size(); ++i) {
      o.via.map.ptr[i].key.convert(&data[i].first);
      o.via.map.ptr[i].val.convert(&data[i].second);
    }
    data.swap(data_);
  }

 private:
  struct less_pair_and_key {
    template <class Pair, class Key>
    bool operator()(Pair const& p, Key const& k) const {
      return p.first < k;
    }
  };

  std::vector<std::pair<K, V> > data_;
};

}  // namespace common
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_COMMON_ASSOC_VECTOR_HPP_
