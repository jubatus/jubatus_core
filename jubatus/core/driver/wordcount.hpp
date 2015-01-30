// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2015 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_DRIVER_WORDCOUNT_HPP_
#define JUBATUS_CORE_DRIVER_WORDCOUNT_HPP_

#include <string>
#include <utility>
#include <vector>
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "../fv_converter/datum.hpp"
#include "../wordcount/wordcount_base.hpp"
#include "../framework/mixable.hpp"
#include "../framework/diffv.hpp"
#include "../fv_converter/mixable_weight_manager.hpp"
#include "../fv_converter/datum_to_fv_converter.hpp"
#include "driver.hpp"

namespace jubatus {
namespace core {
namespace driver {

class wordcount : public driver_base {
 public:
  wordcount(
      jubatus::util::lang::shared_ptr<core::wordcount::wordcount_base>
          method,
      jubatus::util::lang::shared_ptr<fv_converter::datum_to_fv_converter>
          converter);
  virtual ~wordcount() {}

  bool append(const std::string& bucket, const fv_converter::datum& datum);

  size_t count(const std::string& bucket, const std::string& word) const;

  std::vector<std::pair<std::string, size_t> >
      get_ranking(const std::string& bucket, size_t size) const;

  std::vector<std::pair<std::string, size_t> >
      split_test(const fv_converter::datum& datum) const;

  bool clear_bucket(const std::string& bucket);

  void clear();

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object obj);

private:
  jubatus::util::lang::shared_ptr<fv_converter::datum_to_fv_converter>
    converter_;
  jubatus::util::lang::shared_ptr<core::wordcount::wordcount_base>
    wordcount_;
};

}  // namespace driver
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_DRIVER_WORDCOUNT_HPP_
