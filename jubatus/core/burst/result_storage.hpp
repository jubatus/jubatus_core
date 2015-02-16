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

#ifndef JUBATUS_CORE_BURST_RESULT_STORAGE_HPP_
#define JUBATUS_CORE_BURST_RESULT_STORAGE_HPP_

#include <vector>
#include <msgpack.hpp>
#include "jubatus/util/lang/scoped_ptr.h"

#include "burst_result.hpp"
#include "../framework/mixable_helper.hpp"

namespace jubatus {
namespace core {
namespace burst {

class result_storage {
 public:
  typedef burst_result result_t;

  explicit result_storage(int stored_results_max);
  ~result_storage();

  void store(const result_t& result);

  result_t get_latest_result() const;
  result_t get_result_at(double pos) const;

  typedef std::vector<result_t> diff_t;
  diff_t get_diff() const;
  void put_diff(const diff_t& diff);

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

 private:
  class impl_;
  jubatus::util::lang::scoped_ptr<impl_> p_;
};

}  // namespace burst
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BURST_RESULT_STORAGE_HPP_
