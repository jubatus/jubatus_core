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

#ifndef JUBATUS_CORE_COMMON_UNORDERED_SET_HPP_
#define JUBATUS_CORE_COMMON_UNORDERED_SET_HPP_

#include <msgpack.hpp>
#include "jubatus/util/data/unordered_set.h"

// to make util::data::unordered_set serializable

namespace msgpack {

template<typename V, typename H, typename P, typename A>
inline jubatus::util::data::unordered_set<V, H, P, A> operator>>(
    object o,
    jubatus::util::data::unordered_set<V, H, P, A>& s) {
  if (o.type != type::ARRAY) {
    throw type_error();
  }
  for (size_t i = 0; i < o.via.array.size; ++i) {
    V v;
    o.via.array.ptr[i].convert(&v);
    s.insert(v);
  }
  return s;
}

template<typename Stream,
         typename V,
         typename H,
         typename P,
         typename A>
inline packer<Stream>& operator<<(
    packer<Stream>& o,
    const jubatus::util::data::unordered_set<V, H, P, A>& s) {
  o.pack_array(s.size());
  typedef typename
    jubatus::util::data::unordered_set<V, H, P, A>::const_iterator
    iter_t;
  for (iter_t it = s.begin(); it != s.end(); ++it) {
    o.pack(*it);
  }
  return o;
}

}  // namespace msgpack

#endif  // JUBATUS_CORE_COMMON_UNORDERED_SET_HPP_
