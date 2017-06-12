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

#include "storage.hpp"

#include <algorithm>
#include <iterator>
#include <string>
#include "util.hpp"

namespace jubatus {
namespace core {
namespace clustering {

storage::storage(const std::string& name)
    : revision_(0),
      name_(name) {
}

wplist storage::get_all() const {
  wplist ret = get_mine();
  wplist common = get_common();
  concat(common, ret);
  return ret;
}

wplist storage::get_common() const {
  wplist ret;
  for (diff_t::const_iterator it = common_.begin();
      it != common_.end(); ++it) {
    concat(it->second, ret);
  }
  return ret;
}

void storage::get_diff(diff_t& d) const {
  d.clear();
  wplist coreset = get_mine();
  d.push_back(make_pair(name_, coreset));
}

bool storage::put_diff(const diff_t& diff) {
  common_.clear();
  for (diff_t::const_iterator it = diff.begin(); it != diff.end(); ++it) {
    // Exclude data originated from me.
    if (it->first != name_) {
      common_.push_back(*it);
    }
  }
  increment_revision();

  // TODO(kumagi): return false if we want to reject the diff
  return true;
}

void storage::mix(const diff_t& lhs, diff_t& ret) {
  diff_t::const_iterator lb = lhs.begin(), le =  lhs.end();
  diff_t::iterator b = ret.begin(), e = ret.end();
  while (lb != le && b != e) {
    if ((*lb).first < (*b).first) {
      b = ret.insert(b, (*lb));
      ++lb;
    } else if ((*lb).first > (*b).first) {
      ++b;
    } else {
      std::copy((*lb).second.begin(), (*lb).second.end(),
          std::back_inserter((*b).second));
      ++b;
      ++lb;
    }
  }
  std::copy(lb, le, std::back_inserter(ret));
};

void storage::pack(framework::packer& packer) const {
  pack_impl_(packer);
}

void storage::unpack(msgpack::object o) {
  unpack_impl_(o);
  dispatch(REVISION_CHANGE, get_all());
}

void storage::clear() {
  revision_ = 0;
  clear_impl_();
  dispatch(REVISION_CHANGE, get_all());
}

size_t storage::get_revision() {
  return revision_;
}

void storage::increment_revision() {
  ++revision_;
  dispatch(REVISION_CHANGE, get_all());
}

void storage::pack_impl_(framework::packer& packer) const {
  packer.pack(*this);
}
void storage::unpack_impl_(msgpack::object o) {
  o.convert(*this);
}
void storage::clear_impl_() {
  common_.clear();
}


}  // namespace clustering
}  // namespace core
}  // namespace jubatus
