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

#include <cfloat>
#include <utility>
#include <string>
#include "jubatus/util/lang/noncopyable.h"
#include "jubatus/util/data/unordered_map.h"

#include "../common/assert.hpp"
#include "../common/exception.hpp"
#include "aggregator.hpp"

using std::string;
using jubatus::util::lang::shared_ptr;
using jubatus::util::data::unordered_map;

namespace jubatus {
namespace core {
namespace burst {

class burst::diff_t::impl_ {
 public:
  impl_() {
  }

  impl_(const impl_& x, const impl_& y) {
    // do mix (unimplemented)
    throw JUBATUS_EXCEPTION(common::unsupported_method(
        "sorry, unimplemented: mixing burst"));
  }

  explicit impl_(msgpack::object o) {
    // unpack (unimplemented)
    throw JUBATUS_EXCEPTION(common::unsupported_method(
        "sorry, unimplemented: mixing burst"));
  }

  void msgpack_pack(framework::packer& /*packer*/) const {
    throw JUBATUS_EXCEPTION(common::unsupported_method(
        "sorry, unimplemented: mixing burst"));
  }
  void msgpack_unpack(msgpack::object /*o*/) {
    throw JUBATUS_EXCEPTION(common::unsupported_method(
        "sorry, unimplemented: mixing burst"));
  }

 private:
  // unimplemented
};

class burst::impl_ : jubatus::util::lang::noncopyable {
  class storage_ {
   public:
    storage_(
        const burst_options& options, const keyword_params& params)
        : s_(new result_storage(options.result_window_rotate_size)),
          params_(params) {
    }

    const shared_ptr<result_storage>& get_storage() const {
      return s_;
    }

    const keyword_params& get_params() const {
      return params_;
    }

   private:
    shared_ptr<result_storage> s_;
    keyword_params params_;
  };

  class aggregate_helper_ {
   public:
    aggregate_helper_(const burst_options& options,
                      const storage_& s)
        : a_(new aggregator(options.window_batch_size,
                            options.batch_interval,
                            options.result_window_rotate_size)),
          s_(s.get_storage()),
          params_(s.get_params()) {
    }

    bool add_document(int d, int r, double pos) const {
      return a_->add_document(d, r, pos);
    }

    void calculate_result(const burst_options& options) const {
      a_->flush_results(params_.scaling_param,
                        params_.gamma,
                        options.costcut_threshold,
                        options.max_reuse_batch_num,
                        *s_);
    }

    const shared_ptr<aggregator>& get_aggregator() const {
      return a_;
    }

   private:
    shared_ptr<aggregator> a_;
    shared_ptr<result_storage> s_;
    keyword_params params_;
  };

  typedef unordered_map<string, storage_> storages_t;
  typedef unordered_map<string, aggregate_helper_> aggregators_t;

 public:
  explicit impl_(const burst_options& options)
      : options_(options) {
    if (options_.window_batch_size <= 0) {
      throw JUBATUS_EXCEPTION(common::invalid_parameter(
          "window_batch_size should > 0"));
    }
    if (options_.batch_interval <= 0) {
      throw JUBATUS_EXCEPTION(common::invalid_parameter(
          "batch_interval should > 0"));
    }
    if (options_.result_window_rotate_size <= 0) {
      throw JUBATUS_EXCEPTION(common::invalid_parameter(
          "result_window_rotate_size should > 0"));
    }
    if (options_.max_reuse_batch_num < 0) {
      throw JUBATUS_EXCEPTION(common::invalid_parameter(
          "max_reuse_batch_num should >= 0"));
    }
    if (options_.costcut_threshold <= 0) {
      options_.costcut_threshold = DBL_MAX;
    }
  }

  bool add_keyword(const string& keyword,
                   const keyword_params& params,
                   bool processed_in_this_server) {
    if (storages_.count(keyword) > 0 || aggregators_.count(keyword) > 0) {
      return false;
    }

    std::pair<storages_t::iterator, bool> r =
        storages_.insert(std::make_pair(keyword, storage_(options_, params)));

    JUBATUS_ASSERT_EQ(true, r.second, "");

    if (processed_in_this_server) {
      aggregators_.insert(
          std::make_pair(keyword,
                         aggregate_helper_(options_, r.first->second)));
    }

    return true;
  }

  bool remove_keyword(const string& keyword) {
    aggregators_.erase(keyword);
    return storages_.erase(keyword) > 0;
  }

  bool remove_all_keywords() {
    clear();
    return true;
  }

  keyword_list get_all_keywords() const {
    keyword_list result;
    result.reserve(storages_.size());

    for (storages_t::const_iterator iter = storages_.begin();
         iter != storages_.end(); ++iter) {
      const string& keyword = iter->first;
      const keyword_params& params = iter->second.get_params();

      result.push_back(keyword_with_params());
      keyword_with_params& x = result.back();
      x.keyword = keyword;
      x.scaling_param = params.scaling_param;
      x.gamma = params.gamma;
    }

    return result;
  }

  bool add_document(const string& str, double pos) {
    bool result = true;
    for (aggregators_t::iterator iter = aggregators_.begin();
         iter != aggregators_.end(); ++iter) {
      const string& keyword = iter->first;
      aggregate_helper_& a = iter->second;
      int r = str.find(keyword) != str.npos ? 1 : 0;
      result = a.add_document(1, r, pos) && result;
    }
    return result;
  }

  void calculate_results() {
    for (aggregators_t::iterator iter = aggregators_.begin();
         iter != aggregators_.end(); ++iter) {
      iter->second.calculate_result(options_);
    }
  }

  result_t get_result(const string& keyword) const {
    const result_storage* s = get_storage_(keyword);
    if (s == NULL) {
      return result_t();
    }
    return s->get_latest_result();
  }

  result_t get_result_at(const string& keyword, double pos) const {
    const result_storage* s = get_storage_(keyword);
    if (s == NULL) {
      return result_t();
    }
    return s->get_result_at(pos);
  }

  result_map get_all_bursted_results() const {
    result_map results;
    for (storages_t::const_iterator iter = storages_.begin();
         iter != storages_.end(); ++iter) {
      const result_storage& s = *iter->second.get_storage();
      result_t result = s.get_latest_result();
      if (result.is_bursted_at_latest_batch()) {
        results.insert(std::make_pair(iter->first, result));
      }
    }
    return results;
  }

  result_map get_all_bursted_results_at(double pos) const {
    result_map results;
    for (storages_t::const_iterator iter = storages_.begin();
         iter != storages_.end(); ++iter) {
      const result_storage& s = *iter->second.get_storage();
      result_t result = s.get_result_at(pos);
      if (result.is_bursted_at(pos)) {
        results.insert(std::make_pair(iter->first, result));
      }
    }
    return results;
  }

  void get_diff(diff_t& /*ret*/) const {
    throw JUBATUS_EXCEPTION(common::unsupported_method(
        "sorry, unimplemented: mixing burst"));
  }
  bool put_diff(const diff_t&) {
    throw JUBATUS_EXCEPTION(common::unsupported_method(
        "sorry, unimplemented: mixing burst"));
  }

  void clear() {
    aggregators_t().swap(aggregators_);
    storages_t().swap(storages_);
  }
  storage::version get_version() const {
    return storage::version();
  }

  void pack(framework::packer& packer) const {
    packer.pack_array(3);

    packer.pack(options_);

    packer.pack_map(storages_.size());
    for (storages_t::const_iterator iter = storages_.begin();
         iter != storages_.end(); ++iter) {
      packer.pack(iter->first);
      packer.pack_array(2);
      packer.pack(iter->second.get_params());
      iter->second.get_storage()->pack(packer);
    }

    packer.pack_map(aggregators_.size());
    for (aggregators_t::const_iterator iter = aggregators_.begin();
         iter != aggregators_.end(); ++iter) {
      packer.pack(iter->first);
      iter->second.get_aggregator()->pack(packer);
    }
  }

  void unpack(msgpack::object o) {
    burst_options unpacked_options = options_;
    aggregators_t unpacked_aggregators;
    storages_t unpacked_storages;

    unpack_impl_(o, unpacked_options, unpacked_aggregators, unpacked_storages);

    // assign
    options_ = unpacked_options;
    storages_.swap(unpacked_storages);
    aggregators_.swap(unpacked_aggregators);
  }

 private:
  burst_options options_;
  aggregators_t aggregators_;
  storages_t storages_;

  const result_storage* get_storage_(const string& keyword) const {
    storages_t::const_iterator iter = storages_.find(keyword);
    if (iter == storages_.end()) {
      return NULL;
    }
    return iter->second.get_storage().get();
  }

  static void unpack_impl_(msgpack::object o,
                           burst_options& unpacked_options,
                           aggregators_t& unpacked_aggregators,
                           storages_t& unpacked_storages) {
    if (o.type != msgpack::type::ARRAY || o.via.array.size != 3) {
      throw msgpack::type_error();
    }

    o.via.array.ptr[0].convert(&unpacked_options);

    {
      const msgpack::object& m = o.via.array.ptr[1];
      if (m.type != msgpack::type::MAP) {
        throw msgpack::type_error();
      }
      size_t n = m.via.map.size;
      for (size_t i = 0; i < n; ++i) {
        string keyword;
        m.via.map.ptr[i].key.convert(&keyword);

        std::pair<keyword_params, msgpack::object> val;
        m.via.map.ptr[i].val.convert(&val);
        storage_ s(unpacked_options, val.first);
        s.get_storage()->unpack(val.second);

        unpacked_storages.insert(std::make_pair(keyword, s));
      }
    }

    {
      const msgpack::object& m = o.via.array.ptr[2];
      if (m.type != msgpack::type::MAP) {
        throw msgpack::type_error();
      }
      size_t n = m.via.map.size;
      for (size_t i = 0; i < n; ++i) {
        string keyword;
        m.via.map.ptr[i].key.convert(&keyword);

        storages_t::const_iterator iter = unpacked_storages.find(keyword);
        if (iter == unpacked_storages.end()) {
          throw msgpack::type_error();
        }

        aggregate_helper_ a(unpacked_options, iter->second);
        a.get_aggregator()->unpack(m.via.map.ptr[i].val);

        unpacked_aggregators.insert(std::make_pair(keyword, a));
      }
    }
  }
};

burst::burst(const burst_options& options)
    : p_(new impl_(options)) {
}

burst::~burst() {
}

bool burst::add_keyword(const string& keyword,
                        const keyword_params& params,
                        bool processed_in_this_server) {
  JUBATUS_ASSERT(p_);
  return p_->add_keyword(keyword, params, processed_in_this_server);
}

bool burst::remove_keyword(const string& keyword) {
  JUBATUS_ASSERT(p_);
  return p_->remove_keyword(keyword);
}

bool burst::remove_all_keywords() {
  JUBATUS_ASSERT(p_);
  return p_->remove_all_keywords();
}

burst::keyword_list burst::get_all_keywords() const {
  JUBATUS_ASSERT(p_);
  return p_->get_all_keywords();
}

bool burst::add_document(const string& str, double pos) {
  JUBATUS_ASSERT(p_);
  return p_->add_document(str, pos);
}

void burst::calculate_results() {
  JUBATUS_ASSERT(p_);
  p_->calculate_results();
}

burst::result_t burst::get_result(const std::string& keyword) const {
  JUBATUS_ASSERT(p_);
  return p_->get_result(keyword);
}
burst::result_t burst::get_result_at(
    const std::string& keyword, double pos) const {
  JUBATUS_ASSERT(p_);
  return p_->get_result_at(keyword, pos);
}
burst::result_map burst::get_all_bursted_results() const {
  JUBATUS_ASSERT(p_);
  return p_->get_all_bursted_results();
}
burst::result_map burst::get_all_bursted_results_at(double pos) const {
  JUBATUS_ASSERT(p_);
  return p_->get_all_bursted_results_at(pos);
}

void burst::diff_t::mix(const diff_t& mixed) {
  if (!p_) {
    p_ = mixed.p_;
  } else if (!mixed.p_) {
    // do nothing
  } else {
    p_.reset(new impl_(*p_, *mixed.p_));
  }
}
void burst::diff_t::msgpack_pack(framework::packer& packer) const {
  if (!p_) {
    packer.pack_array(0);
  } else {
    p_->msgpack_pack(packer);
  }
}
void burst::diff_t::msgpack_unpack(msgpack::object o) {
  p_.reset(new impl_(o));
}
void burst::get_diff(diff_t& ret) const {
  JUBATUS_ASSERT(p_);
  p_->get_diff(ret);
}
bool burst::put_diff(const diff_t& diff) {
  JUBATUS_ASSERT(p_);
  return p_->put_diff(diff);
}

void burst::clear() {
  JUBATUS_ASSERT(p_);
  p_->clear();
}
storage::version burst::get_version() const {
  JUBATUS_ASSERT(p_);
  return p_->get_version();
}
void burst::pack(framework::packer& packer) const {
  JUBATUS_ASSERT(p_);
  p_->pack(packer);
}
void burst::unpack(msgpack::object o) {
  JUBATUS_ASSERT(p_);
  p_->unpack(o);
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
