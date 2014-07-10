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

#ifndef JUBATUS_CORE_DRIVER_BURST_HPP_
#define JUBATUS_CORE_DRIVER_BURST_HPP_

#include <string>

#include "../burst/burst.hpp"
#include "../framework/mixable.hpp"
#include "driver.hpp"

namespace jubatus {
namespace core {
namespace driver {

class burst : public driver_base {
 public:
  typedef jubatus::core::burst::burst model_t;
  typedef model_t::result_t result_t;
  typedef model_t::result_map result_map;
  typedef model_t::keyword_list keyword_list;

  explicit burst(jubatus::util::lang::shared_ptr<model_t> model) {
    init_(model);
  }
  explicit burst(model_t* model) {
    jubatus::util::lang::shared_ptr<model_t> p(model);
    init_(p);
  }

  model_t* get_model() const {
    return burst_.get();
  }

  bool add_keyword(
      const std::string& keyword, double scaling_param, double gamma);
  bool remove_keyword(const std::string& keyword);
  bool remove_all_keywords();
  keyword_list get_all_keywords() const;

  bool add_document(const std::string& str, double pos);
  result_t get_result(const std::string& keyword);
  result_t get_result_at(const std::string& keyword, double pos);
  result_map get_all_bursted_results();
  result_map get_all_bursted_results_at(double pos);

  void pack(framework::packer& pk) const;
  void unpack(msgpack::object o);
  void clear();

 private:
  jubatus::util::lang::shared_ptr<model_t> burst_;
  core::burst::mixable_burst mixable_burst_;

  void init_(jubatus::util::lang::shared_ptr<model_t> model);
};

}  // namespace driver
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_DRIVER_BURST_HPP_
