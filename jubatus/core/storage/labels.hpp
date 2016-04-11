// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2016 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_STORAGE_LABELS_HPP_
#define JUBATUS_CORE_STORAGE_LABELS_HPP_

#include <string>
#include <utility>
#include <msgpack.hpp>

#include "jubatus/util/data/unordered_set.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/unordered_set.hpp"
#include "../common/version.hpp"
#include "../framework/model.hpp"
#include "../framework/packer.hpp"
#include "../framework/linear_mixable.hpp"
#include "../framework/push_mixable.hpp"

namespace jubatus {
namespace core {
namespace storage {

typedef jubatus::util::data::unordered_set<std::string> data_t;

class labels {
 public:
  typedef data_t::iterator iterator;
  typedef data_t::const_iterator const_iterator;

  labels();
  ~labels();

  std::pair<iterator, bool> insert(std::string label) {
    return master_.insert(label);
  }

  size_t erase(const std::string label) {
    return master_.erase(label);
  }

  void clear() {
    master_.clear();
  }

  size_t size() const {
    return master_.size();
  }

  const_iterator begin() const {
    return master_.begin();
  }

  const_iterator end() const {
    return master_.end();
  }

  iterator begin() {
    return master_.begin();
  }

  iterator end() {
    return master_.end();
  }

  data_t get() const;
  void put(const data_t& data);
  void merge(const data_t& lhs, data_t& rhs) const;

  version get_version() const {
    return version_;
  }

  std::string name() const {
    return std::string("labels");
  }

  void pack(framework::packer& packer) const;
  void unpack(msgpack::object o);

  MSGPACK_DEFINE(master_, version_);

 private:
  // Not manage diffs between MIX.
  data_t master_;
  version version_;
};

class mixable_labels : public framework::push_mixable,
  public framework::linear_mixable {
 public:
  typedef jubatus::util::lang::shared_ptr<labels> model_ptr;

  explicit mixable_labels(model_ptr model);
  ~mixable_labels();

  model_ptr get_model() const {
    return model_;
  }

  // linear mixable
  framework::diff_object convert_diff_object(
      const msgpack::object& obj) const;
  void mix(const msgpack::object& obj, framework::diff_object dff) const;
  void get_diff(framework::packer& packer) const;
  bool put_diff(const framework::diff_object& diff);

  // push mixable
  void get_argument(framework::packer& packer) const;
  void pull(const msgpack::object& obj, framework::packer& packer) const;
  void push(const msgpack::object& obj);

  storage::version get_version() const {
    return model_->get_version();
  }

 private:
  struct internal_diff_object : framework::diff_object_raw {
    void convert_binary(framework::packer& packer) const {
      packer.pack(diff_);
    }

    data_t diff_;
  };

  model_ptr model_;
};

}  // namespace storage
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_STORAGE_LABELS_HPP_
