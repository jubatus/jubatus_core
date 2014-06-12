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

#ifndef JUBATUS_CORE_DRIVER_CLASSIFIER_HPP_
#define JUBATUS_CORE_DRIVER_CLASSIFIER_HPP_

#include <map>
#include <string>
#include <vector>
#include "jubatus/util/text/json.h"
#include "jubatus/util/data/serialization.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/jsonconfig.hpp"
#include "../common/byte_buffer.hpp"
#include "../classifier/classifier_type.hpp"
#include "../classifier/classifier_config.hpp"
#include "../framework/mixable.hpp"
#include "../fv_converter/mixable_weight_manager.hpp"
#include "../fv_converter/converter_config.hpp"
#include "driver.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {
class datum;
class datum_to_fv_converter;
}  // namespace fv_converter
namespace classifier {
class classifier_base;
}  // namespace classifier
namespace driver {

struct classifier_driver_config {
  std::string method;
  jubatus::util::data::optional<core::classifier::classifier_config> parameter;
  core::fv_converter::converter_config converter;

  void swap(classifier_driver_config& other) {
    method.swap(other.method);
    parameter.swap(other.parameter);
    converter.swap(other.converter);
  }

  template<typename Ar>
  void serialize(Ar& ar) {
    ar & JUBA_MEMBER(method) & JUBA_MEMBER(parameter) & JUBA_MEMBER(converter);
  }
};

class classifier : public driver_base {
 public:
  typedef core::classifier::classifier_base classifier_base;

  // TODO(suma): where is the owner of model, mixer, and converter?
  classifier(const classifier_driver_config& conf);
  virtual ~classifier();

  void train(const std::string&, const fv_converter::datum&);

  jubatus::core::classifier::classify_result classify(
      const fv_converter::datum& data) const;

  void get_status(std::map<std::string, std::string>& status) const;
  bool delete_label(const std::string& name);

  void clear();
  void pack(framework::packer& pk) const;
  void unpack(msgpack::object o);

  void import_model(common::byte_buffer& src);
  common::byte_buffer export_model() const;

  std::vector<std::string> get_labels() const;
  bool set_label(const std::string& label);

  void swap(classifier& other);

 private:
  classifier_driver_config conf_;
  jubatus::util::lang::shared_ptr<fv_converter::factory_extender> extender_;
  jubatus::util::lang::shared_ptr<fv_converter::datum_to_fv_converter>
      converter_;
  jubatus::util::lang::shared_ptr<classifier_base> classifier_;
  fv_converter::mixable_weight_manager wm_;
};

}  // namespace driver
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_DRIVER_CLASSIFIER_HPP_
