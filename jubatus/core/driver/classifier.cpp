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

#include "classifier.hpp"

#include <map>
#include <string>
#include <sstream>
#include <utility>
#include <vector>
#include <algorithm>

#include "jubatus/util/text/json.h"
#include "jubatus/util/lang/cast.h"
#include "../classifier/classifier_factory.hpp"
#include "../classifier/classifier_base.hpp"
#include "../common/vector_util.hpp"
#include "../common/jsonconfig.hpp"
#include "../common/byte_buffer.hpp"
#include "../framework/stream_writer.hpp"
#include "../framework/packer.hpp"
#include "../fv_converter/datum.hpp"
#include "../fv_converter/datum_to_fv_converter.hpp"
#include "../fv_converter/converter_config.hpp"
#include "../storage/storage_factory.hpp"
#include "../fv_converter/factory.hpp"
#include "../fv_converter/so_factory.hpp"

using std::string;
using std::vector;
using std::make_pair;
using jubatus::util::text::json::json;
using jubatus::util::lang::shared_ptr;
using jubatus::util::lang::lexical_cast;
using jubatus::core::fv_converter::weight_manager;
using jubatus::core::common::jsonconfig::config_cast_check;
using jubatus::core::fv_converter::mixable_weight_manager;

namespace jubatus {
namespace core {
namespace driver {

shared_ptr<fv_converter::datum_to_fv_converter>
generate_fv_converter(const classifier_driver_config& conf,
                      const fv_converter::factory_extender* extender) {
  return fv_converter::make_fv_converter(conf.converter, extender);
}

classifier::classifier(const classifier_driver_config& conf)
    : conf_(conf),
      extender_(new fv_converter::so_factory()),
      converter_(fv_converter::make_fv_converter(conf.converter,
                                                 extender_.get())),
      classifier_(
          core::classifier::classifier_factory::create_classifier(
              *conf.parameter)),
      wm_(mixable_weight_manager::model_ptr(new weight_manager)) {
  register_mixable(classifier_->get_mixable());
  register_mixable(&wm_);
  converter_->set_weight_manager(wm_.get_model());
}

classifier::~classifier() {
}

void classifier::train(const string& label, const fv_converter::datum& data) {
  common::sfv_t v;
  converter_->convert_and_update_weight(data, v);
  common::sort_and_merge(v);
  classifier_->train(v, label);
}

jubatus::core::classifier::classify_result classifier::classify(
    const fv_converter::datum& data) const {
  common::sfv_t v;
  converter_->convert(data, v);

  jubatus::core::classifier::classify_result scores;
  classifier_->classify_with_scores(v, scores);
  return scores;
}

void classifier::get_status(std::map<string, string>& status) const {
  classifier_->get_status(status);
}

bool classifier::delete_label(const std::string& label) {
  return classifier_->delete_label(label);
}

void classifier::clear() {
  classifier_->clear();
  converter_->clear_weights();
}

std::vector<std::string> classifier::get_labels() const {
  return classifier_->get_labels();
}
bool classifier::set_label(const std::string& label) {
  return classifier_->set_label(label);
}

void classifier::pack(framework::packer& pk) const {
  pk.pack_array(2);
  classifier_->pack(pk);
  wm_.get_model()->pack(pk);
}

void classifier::unpack(msgpack::object o) {
  if (o.type != msgpack::type::ARRAY || o.via.array.size != 2) {
    throw msgpack::type_error();
  }

  // clear before load
  classifier_->clear();
  converter_->clear_weights();
  classifier_->unpack(o.via.array.ptr[0]);
  wm_.get_model()->unpack(o.via.array.ptr[1]);
}

struct versioned_model {
  // this struct is version compatible
  std::string version;
  common::byte_buffer buffer;
  versioned_model(const std::string& v, const common::byte_buffer& b)
      : version(v), buffer(b) {
  }
  MSGPACK_DEFINE(version, buffer);
};

void classifier::swap(driver::classifier& other) {
  conf_.swap(other.conf_);
  extender_.swap(other.extender_);
  converter_.swap(other.converter_);
  classifier_.swap(other.classifier_);
  wm_.swap(other.wm_);
}

void classifier::import_model(common::byte_buffer& src) {
  msgpack::unpacked packed;
  msgpack::unpack(&packed, src.ptr(), src.size());
  msgpack::object o = packed.get();

  if (o.type != msgpack::type::ARRAY || o.via.array.size != 2) {
    throw msgpack::type_error();
  }

  // config_file
  common::jsonconfig::config
      jsonconf(lexical_cast<json>(o.via.array.ptr[0].as<string>()));
  classifier_driver_config new_config(
      config_cast_check<classifier_driver_config>(jsonconf));
  classifier new_classifier(new_config);
  this->swap(new_classifier);

  // config_data
  std::string model_payload(o.via.array.ptr[1].as<string>());
  msgpack::unpacked packed_data;
  msgpack::unpack(&packed_data, &model_payload[0], model_payload.size());
  msgpack::object model_obj;
  classifier_->import_model(model_obj);
  wm_.get_model()->import_model(model_obj);

  // TODO: re-register mixer and re-register weight_manager to fvconveter is needed here  // NOLINT
}

common::byte_buffer classifier::export_model() const {
  common::byte_buffer result;
  framework::stream_writer<common::byte_buffer> writer(result);
  framework::jubatus_packer jp(writer);
  core::framework::packer dst(jp);
  dst.pack_array(2);  // [config_file, model_data]

  {  // config_file
    std::string config_json;
    classifier_driver_config nonconst_conf(
        *const_cast<classifier_driver_config*>(&conf_));
    nonconst_conf.serialize(config_json);
    dst.pack(config_json);
  }

  {  // model_data
    dst.pack_array(2);  // [classifier_, wm_]
    classifier_->export_model(dst);
    wm_.get_model()->export_model(dst);
  }
  return result;
}


}  // namespace driver
}  // namespace core
}  // namespace jubatus
