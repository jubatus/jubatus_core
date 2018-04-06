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

#ifndef JUBATUS_CORE_CLUSTERING_COMPRESSOR_HPP_
#define JUBATUS_CORE_CLUSTERING_COMPRESSOR_HPP_

#include <msgpack.hpp>
#include "types.hpp"
#include "util.hpp"
#include "../../util/lang/function.h"

namespace jubatus {
namespace core {
namespace clustering {
namespace compressor {

class compressor {
 public:
  explicit compressor() {}
  virtual ~compressor() {}

  virtual void compress(
      const wplist& src,
      size_t bsize,
      size_t dstsize,
      wplist& dst) = 0;
  //  MSGPACK_DEFINE();
  util::lang::function<double (const common::sfv_t&, const common::sfv_t&)> sfv_dist_;
  util::lang::function<double (const weighted_point&, const weighted_point&)> point_dist_;

};

}  // namespace compressor
}  // namespace clustering
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_CLUSTERING_COMPRESSOR_HPP_
