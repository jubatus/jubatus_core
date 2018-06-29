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

#ifndef JUBATUS_CORE_DRIVER_ANOMALY_HPP_
#define JUBATUS_CORE_DRIVER_ANOMALY_HPP_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "jubatus/util/lang/shared_ptr.h"
#include "../fv_converter/mixable_weight_manager.hpp"
#include "driver.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {
class datum;
class datum_to_fv_converter;
}  // namespace fv_converter
namespace anomaly {
class anomaly_base;
}  // namespace anomaly
namespace driver {

class anomaly : public driver_base {
 public:
  anomaly(
      jubatus::util::lang::shared_ptr<core::anomaly::anomaly_base>
          anomaly_method,
      jubatus::util::lang::shared_ptr<fv_converter::datum_to_fv_converter>
          converter);
  virtual ~anomaly();

  jubatus::core::anomaly::anomaly_base* get_model() const {
    return anomaly_.get();
  }

  bool is_updatable() const;

  void clear_row(const std::string& id);
  std::pair<std::string, double> add(
      const std::string& id,
      const fv_converter::datum& d);
  std::vector<std::string> add_bulk(
      const std::vector<std::pair<std::string, fv_converter::datum> >& data);

  double update(const std::string& id, const fv_converter::datum& d);
  double overwrite(const std::string& id, const fv_converter::datum& d);
  void get_status(std::map<std::string, std::string>& status) const;
  void clear();
  void pack(framework::packer& pk) const;
  void unpack(msgpack::object o);
  double calc_score(const fv_converter::datum& d) const;
  std::vector<std::string> get_all_rows() const;
  uint64_t find_max_int_id() const;

 private:
  jubatus::util::lang::shared_ptr<fv_converter::datum_to_fv_converter>
      converter_;
  jubatus::util::lang::shared_ptr<core::anomaly::anomaly_base> anomaly_;
  fv_converter::mixable_weight_manager wm_;
};

}  // namespace driver
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_DRIVER_ANOMALY_HPP_
