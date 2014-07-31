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

#ifndef JUBATUS_CORE_BURST_BURST_HPP_
#define JUBATUS_CORE_BURST_BURST_HPP_

#include <string>
#include <vector>
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/lang/scoped_ptr.h"

#include "../framework/mixable_helper.hpp"
#include "burst_result.hpp"

namespace jubatus {
namespace core {
namespace burst {

struct burst_options {
  int window_batch_size;
  double batch_interval;
  int result_window_rotate_size;
  int max_reuse_batch_num;
  double costcut_threshold;

  MSGPACK_DEFINE(
      window_batch_size,
      batch_interval,
      result_window_rotate_size,
      max_reuse_batch_num,
      costcut_threshold);
};

struct keyword_params {
  double scaling_param;
  double gamma;

  MSGPACK_DEFINE(scaling_param, gamma);
};

struct keyword_with_params {
  std::string keyword;
  double scaling_param;
  double gamma;
};

class burst {
 public:
  typedef burst_result result_t;
  typedef jubatus::util::data::unordered_map<std::string, result_t> result_map;
  typedef std::vector<keyword_with_params> keyword_list;

  explicit burst(const burst_options& options);
  ~burst();

  bool add_keyword(const std::string& keyword,
                   const keyword_params& params,
                   bool processed_in_this_server);
  bool remove_keyword(const std::string& keyword);
  bool remove_all_keywords();
  keyword_list get_all_keywords() const;

  bool add_document(const std::string& str, double pos);

  void calculate_results();

  result_t get_result(const std::string& keyword) const;
  result_t get_result_at(const std::string& keyword, double pos) const;
  result_map get_all_bursted_results() const;
  result_map get_all_bursted_results_at(double pos) const;

  class diff_t {
    friend class burst;
    class impl_;
    jubatus::util::lang::shared_ptr<const impl_> p_;
   public:
    diff_t() {}
    void mix(const diff_t& mixed);
    void msgpack_pack(framework::packer& packer) const;
    void msgpack_unpack(msgpack::object o);
  };
  void get_diff(diff_t& ret) const;
  bool put_diff(const diff_t&);
  static void mix(const diff_t& lhs, diff_t& ret) {
    ret.mix(lhs);
  }

  void clear();
  storage::version get_version() const;
  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

 private:
  class impl_;
  jubatus::util::lang::scoped_ptr<impl_> p_;
};

typedef framework::linear_mixable_helper<burst, burst::diff_t>
    mixable_burst;

}  // namespace burst
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_BURST_BURST_HPP_
