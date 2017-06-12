// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011,2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_COMMON_UNORDERED_MAP_HPP_
#define JUBATUS_CORE_COMMON_UNORDERED_MAP_HPP_

#include <msgpack.hpp>
#include "jubatus/util/data/unordered_map.h"

// to make util::data::unordered_map serializable

namespace msgpack {

template<typename K, typename V, typename H, typename E, typename A>
inline jubatus::util::data::unordered_map<K, V, H, E, A> operator>>(
    object o,
    jubatus::util::data::unordered_map<K, V, H, E, A>& v) {
  if (o.type != type::MAP) {
    throw type_error();
  }
  object_kv* const p_end = o.via.map.ptr + o.via.map.size;
  for (object_kv* p = o.via.map.ptr; p != p_end; ++p) {
    K key;
    p->key.convert(&key);
    p->val.convert(&v[key]);
  }
  return v;
}


template<typename Stream,
         typename K,
         typename V,
         typename H,
         typename E,
         typename A>
inline packer<Stream>& operator<<(
    packer<Stream>& o,
    const jubatus::util::data::unordered_map<K, V, H, E, A>& v) {
  o.pack_map(v.size());
  typedef typename
    jubatus::util::data::unordered_map<K, V, H, E, A>::const_iterator
    iter_t;
  for (iter_t it = v.begin(); it != v.end(); ++it) {
    o.pack(it->first);
    o.pack(it->second);
  }
  return o;
}

MSGPACK_API_VERSION_NAMESPACE(v1) {

namespace adaptor {

template <typename K, typename V, typename Hash, typename Pred, typename Alloc>
struct convert<jubatus::util::data::unordered_map<K, V, Hash, Pred, Alloc> > {
  msgpack::object const& operator()(msgpack::object const& o, jubatus::util::data::unordered_map<K, V, Hash, Pred, Alloc>& v) const {
        if(o.type != msgpack::type::MAP) { throw msgpack::type_error(); }
        msgpack::object_kv* p(o.via.map.ptr);
        msgpack::object_kv* const pend(o.via.map.ptr + o.via.map.size);
        jubatus::util::data::unordered_map<K, V, Hash, Pred, Alloc> tmp;
        for(; p != pend; ++p) {
            K key;
            p->key.convert(key);
            p->val.convert(tmp[key]);
        }
        tmp.swap(v);
        return o;
    }
};

template <typename K, typename V, typename Hash, typename Pred, typename Alloc>
struct pack<jubatus::util::data::unordered_map<K, V, Hash, Pred, Alloc> > {
    template <typename Stream>
    msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o, const jubatus::util::data::unordered_map<K, V, Hash, Pred, Alloc>& v) const {
        uint32_t size = checked_get_container_size(v.size());
        o.pack_map(size);
        for(typename jubatus::util::data::unordered_map<K, V, Hash, Pred, Alloc>::const_iterator it(v.begin()), it_end(v.end());
            it != it_end; ++it) {
            o.pack(it->first);
            o.pack(it->second);
        }
        return o;
    }
};

template <typename K, typename V, typename Hash, typename Pred, typename Alloc>
struct object_with_zone<jubatus::util::data::unordered_map<K, V, Hash, Pred, Alloc> > {
    void operator()(msgpack::object::with_zone& o, const jubatus::util::data::unordered_map<K, V, Hash, Pred, Alloc>& v) const {
        o.type = msgpack::type::MAP;
        if(v.empty()) {
            o.via.map.ptr  = MSGPACK_NULLPTR;
            o.via.map.size = 0;
        } else {
            uint32_t size = checked_get_container_size(v.size());
            msgpack::object_kv* p = static_cast<msgpack::object_kv*>(o.zone.allocate_align(sizeof(msgpack::object_kv)*size));
            msgpack::object_kv* const pend = p + size;
            o.via.map.ptr  = p;
            o.via.map.size = size;
            typename jubatus::util::data::unordered_map<K, V, Hash, Pred, Alloc>::const_iterator it(v.begin());
            do {
                p->key = msgpack::object(it->first, o.zone);
                p->val = msgpack::object(it->second, o.zone);
                ++p;
                ++it;
            } while(p < pend);
        }
    }
};


}  // MSGPACK_API_VERSION_NAMESPACE(v1)
}  // namespace adaptor

}  // namespace msgpack

#endif  // JUBATUS_CORE_COMMON_UNORDERED_MAP_HPP_
