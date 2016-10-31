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

#include "linear_regression.hpp"

#include <map>
#include <string>
#include <vector>
#include "../storage/storage_base.hpp"

using std::string;

namespace jubatus {
namespace core {
namespace regression {

linear_regression::linear_regression(storage_ptr storage)
  : storage_(storage),
    mixable_storage_(storage) {
}

float linear_regression::estimate(const common::sfv_t& fv) const {
  storage::map_feature_val1_t ret;
  storage_->inp(fv, ret);
  return ret["+"];
}

void linear_regression::update(const common::sfv_t& fv, float coeff) {
  storage_->bulk_update(fv, coeff, "+", "");
}

void linear_regression::clear() {
  storage_->clear();
}

void linear_regression::get_status(std::map<string, string>& status)
    const {
  storage_->get_status(status);
  status["storage"] = storage_->type();
}

float linear_regression::calc_variance(const common::sfv_t& sfv) const {
    float var = 0.f;
    util::concurrent::scoped_rlock lk(storage_->get_lock());
    for (size_t i = 0; i < sfv.size(); ++i) {
      const string& feature = sfv[i].first;
      const float val = sfv[i].second;
      float covar = 1.f;
      storage::feature_val2_t weight_covars;
      storage_->get2_nolock(feature, weight_covars);

      if (weight_covars.size() > 0) {
        covar = weight_covars[0].second.v2;
      }
      var +=  covar * val * val;
    }
    return var;
}

void linear_regression::pack(framework::packer& pk) const {
  pk.pack_array(1);
  mixable_storage_.get_model()->pack(pk);
}

void linear_regression::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY || o.via.array.size != 1) {
    throw msgpack::type_error();
  }
  mixable_storage_.get_model()->unpack(o.via.array.ptr[0]);
}

std::vector<framework::mixable*> linear_regression::get_mixables() {
  std::vector<framework::mixable*> mixables;
  mixables.push_back(&mixable_storage_);
  return mixables;
}

}  // namespace regression
}  // namespace core
}  // namespace jubatus
