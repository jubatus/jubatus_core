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

#ifndef JUBATUS_CORE_DRIVER_RECOMMENDER_HPP_
#define JUBATUS_CORE_DRIVER_RECOMMENDER_HPP_

#include <string>
#include <utility>
#include <vector>
#include "jubatus/util/lang/shared_ptr.h"
#include "../recommender/recommender_base.hpp"
#include "../framework/mixable.hpp"
#include "../framework/diffv.hpp"
#include "../fv_converter/mixable_weight_manager.hpp"
#include "../fv_converter/datum_to_fv_converter.hpp"
#include "driver.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {
class datum;
class datum_to_fv_converter;
}  // namespace fv_converter
namespace recommender {
class recommender_base;
}  // namespace recommender
namespace driver {

class recommender : public driver_base {
 public:
  recommender(
      jubatus::util::lang::shared_ptr<core::recommender::recommender_base>
          method,
      jubatus::util::lang::shared_ptr<fv_converter::datum_to_fv_converter>
          converter);
  virtual ~recommender();

  void clear_row(const std::string& id);
  void update_row(const std::string& id, const fv_converter::datum& dat);
  void clear();
  void pack(framework::packer& pk) const;
  void unpack(msgpack::object o);

  fv_converter::datum complete_row_from_id(const std::string& id);
  fv_converter::datum complete_row_from_datum(const fv_converter::datum& dat);
  std::vector<std::pair<std::string, float> > similar_row_from_id(
      const std::string& id,
      size_t ret_num);
  std::vector<std::pair<std::string, float> > similar_row_from_datum(
      const fv_converter::datum& data,
      size_t size);

  float calc_similarity(
      const fv_converter::datum& l,
      const fv_converter::datum& r);
  float calc_l2norm(const fv_converter::datum& q);

  fv_converter::datum decode_row(const std::string& id);
  std::vector<std::string> get_all_rows();

 private:
  jubatus::util::lang::shared_ptr<fv_converter::datum_to_fv_converter>
    converter_;
  jubatus::util::lang::shared_ptr<core::recommender::recommender_base>
    recommender_;
  fv_converter::mixable_weight_manager wm_;
};

}  // namespace driver
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_DRIVER_RECOMMENDER_HPP_
