// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2017 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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
    jubatus::util::data::unordered_set<V, H, P, A>& v) {
  if (o.type != type::ARRAY) {
    throw type_error();
  }
  object* p = o.via.array.ptr + o.via.array.size;
  object* const pbegin = o.via.array.ptr;
  while (p > pbegin) {
    --p;
    v.insert(p->as<V>());
  }
  return v;
}

template<typename Stream,
         typename V,
         typename H,
         typename P,
         typename A>
inline packer<Stream>& operator<<(
    packer<Stream>& o,
    const jubatus::util::data::unordered_set<V, H, P, A>& v) {
  o.pack_array(v.size());
  typedef typename
    jubatus::util::data::unordered_set<V, H, P, A>::const_iterator
    iter_t;
  for (iter_t it = v.begin(); it != v.end(); ++it) {
    o.pack(*it);
  }
  return o;
}

}  // namespace msgpack

#endif  // JUBATUS_CORE_COMMON_UNORDERED_SET_HPP_
