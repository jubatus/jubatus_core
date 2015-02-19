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

#ifndef JUBATUS_CORE_BURST_AGGREGATOR_HPP_
#define JUBATUS_CORE_BURST_AGGREGATOR_HPP_

#include <vector>
#include "jubatus/util/lang/scoped_ptr.h"

#include "result_storage.hpp"
#include "../framework/mixable_helper.hpp"

namespace jubatus {
namespace core {
namespace burst {

class aggregator {
 public:
  aggregator(int window_batch_size, double batch_interval, int max_stored);
  ~aggregator();

  // store data into internal windows
  // returns: false if pos is too late to store
  bool add_document(int d, int r, double pos);

  // calculate windows and store into result_storage
  //   then erase stored input which has no intersection to latest window
  // returns: count of windows erased
  int flush_results(double scaling_param,
                    double gamma,
                    double costcut_threshold,
                    int max_reuse_batches,
                    result_storage& stored);

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

 private:
  class impl_;
  jubatus::util::lang::scoped_ptr<impl_> p_;
};

}  // namespace burst
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BURST_AGGREGATOR_HPP_
