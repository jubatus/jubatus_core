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

#include "burst.hpp"

#include <string>
#include <vector>
#include <map>

#include "../framework/mixable.hpp"


namespace jubatus {
namespace core {
namespace driver {

namespace {

std::string keywords_to_string(const burst::keyword_list& keywords) {
  size_t n = keywords.size();

  if (n == 0) {
    return "";
  }

  std::string result = keywords[0].keyword;

  for (size_t i = 1; i < n; ++i) {
    result += ',';
    result += keywords[i].keyword;
  }

  return result;
}

}  // namespace

void burst::init_() {
  holder_ = mixable_holder();  // just to be safe

  register_mixable(&mixable_burst_);
}

bool burst::add_keyword(const std::string& keyword,
                        const keyword_params& params,
                        bool processed_in_this_server) {
  return burst_->add_keyword(keyword, params, processed_in_this_server);
}
bool burst::remove_keyword(const std::string& keyword) {
  return burst_->remove_keyword(keyword);
}
bool burst::remove_all_keywords() {
  return burst_->remove_all_keywords();
}
burst::keyword_list burst::get_all_keywords() const {
  return burst_->get_all_keywords();
}

bool burst::add_document(const std::string& str, double pos) {
  return burst_->add_document(str, pos);
}
void burst::calculate_results() {
  burst_->calculate_results();
}
burst::result_t burst::get_result(const std::string& keyword) const {
  return burst_->get_result(keyword);
}
burst::result_t burst::get_result_at(
    const std::string& keyword, double pos) const {
  return burst_->get_result_at(keyword, pos);
}
burst::result_map burst::get_all_bursted_results() const {
  return burst_->get_all_bursted_results();
}
burst::result_map burst::get_all_bursted_results_at(double pos) const {
  return burst_->get_all_bursted_results_at(pos);
}

void burst::get_status(std::map<std::string, std::string>& status) const {
  status["all_keywords"] =
      keywords_to_string(burst_->get_all_keywords());
  status["processed_keywords"] =
      keywords_to_string(burst_->get_processed_keywords());
}

bool burst::has_been_mixed() const {
  return burst_->has_been_mixed();
}
void burst::set_processed_keywords(const std::vector<std::string>& keywords) {
  return burst_->set_processed_keywords(keywords);
}

void burst::pack(framework::packer& pk) const {
  burst_->pack(pk);
}
void burst::unpack(msgpack::object o) {
  burst_->unpack(o);
}
void burst::clear() {
  burst_->clear();
}

}  // namespace driver
}  // namespace core
}  // namespace jubatus
