// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#include "bandit.hpp"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../bandit/bandit_factory.hpp"

using jubatus::util::lang::shared_ptr;
using jubatus::core::bandit::bandit_factory;

namespace jubatus {
namespace core {
namespace driver {

bandit::bandit(const std::string& method_name,
               const common::jsonconfig::config& param) {
  shared_ptr<bandit_storage> s(new bandit_storage());
  bandit_ = bandit_factory::create(method_name, param, s);
  mixable_storage_.set_model(s);

  register_mixable(&mixable_storage_);
}

void bandit::clear() {
  bandit_->clear();
}
void bandit::pack(framework::packer& pk) const {
  bandit_->pack(pk);
}
void bandit::unpack(msgpack::object o) {
  bandit_->unpack(o);
}

}  // namespace driver
}  // namespace core
}  // namespace jubatus
