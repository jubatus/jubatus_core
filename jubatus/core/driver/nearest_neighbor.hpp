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

#ifndef JUBATUS_CORE_DRIVER_NEAREST_NEIGHBOR_HPP_
#define JUBATUS_CORE_DRIVER_NEAREST_NEIGHBOR_HPP_

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>
#include "jubatus/util/lang/shared_ptr.h"

#include "../framework/mixable.hpp"
#include "../fv_converter/mixable_weight_manager.hpp"
#include "../unlearner/unlearner_base.hpp"
#include "driver.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {
class datum;
class datum_to_fv_converter;
}  // namespace fv_converter
namespace storage {
class column_table;
}  // namespace storage
namespace nearest_neighbor {
class nearest_neighbor_base;
}  // namespace nearest_neighbor
namespace driver {

class nearest_neighbor : public driver_base {
 public:
  nearest_neighbor(
    jubatus::util::lang::shared_ptr<
        core::nearest_neighbor::nearest_neighbor_base> nn,
    jubatus::util::lang::shared_ptr<
        fv_converter::datum_to_fv_converter> converter);

  nearest_neighbor(
    jubatus::util::lang::shared_ptr<
        core::nearest_neighbor::nearest_neighbor_base> nn,
    jubatus::util::lang::shared_ptr<
        fv_converter::datum_to_fv_converter> converter,
    jubatus::util::lang::shared_ptr<unlearner::unlearner_base> unlearner);

  jubatus::util::lang::shared_ptr<storage::column_table> get_table();
  jubatus::util::lang::shared_ptr<const storage::column_table>
  get_const_table() const;

  void set_row(const std::string& id, const fv_converter::datum& datum);

  std::vector<std::pair<std::string, double> >
  neighbor_row_from_id(const std::string& id, size_t size);

  std::vector<std::pair<std::string, double> >
  neighbor_row_from_datum(const fv_converter::datum& datum, size_t size);

  std::vector<std::pair<std::string, double> >
  similar_row(const std::string& id, size_t ret_num);

  std::vector<std::pair<std::string, double> >
  similar_row(const core::fv_converter::datum& datum, size_t ret_num);

  std::vector<std::string> get_all_rows();

  void clear();
  void pack(framework::packer& pk) const;
  void unpack(msgpack::object o);

 private:
  jubatus::util::lang::shared_ptr<fv_converter::datum_to_fv_converter>
      converter_;
  jubatus::util::lang::shared_ptr<core::nearest_neighbor::nearest_neighbor_base>
      nn_;
  jubatus::util::lang::shared_ptr<unlearner::unlearner_base> unlearner_;
  fv_converter::mixable_weight_manager wm_;
};

}  // namespace driver
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_DRIVER_NEAREST_NEIGHBOR_HPP_
