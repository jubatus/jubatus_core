// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011,2012 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#ifndef JUBATUS_CORE_FRAMEWORK_MIXABLE_HPP_
#define JUBATUS_CORE_FRAMEWORK_MIXABLE_HPP_

#include <iostream>
#include <string>
#include <vector>

#include <msgpack.hpp>
#include "jubatus/util/concurrent/rwmutex.h"
#include "jubatus/util/concurrent/lock.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/version.hpp"
#include "../common/exception.hpp"
#include "../common/byte_buffer.hpp"
#include "../common/assert.hpp"

// TODO(suma): Rename new_mixable.hpp to mixable.hpp when
// mixable0/deprecated_mixable/delegating_mixable deleted
#include "linear_mixable.hpp"
#include "new_mixable.hpp"
#include "model.hpp"

namespace jubatus {
namespace core {
namespace framework {

// TODO(unknown): split linear_mixable and random_miable
class mixable0 : public mixable, public model {
 public:
  mixable0() : mixable("TODO(linear and pushpull)") {
  }

  virtual ~mixable0() {
  }

  // interface for linear_mixer
  virtual common::byte_buffer get_diff() const = 0;
  virtual bool put_diff(const common::byte_buffer&) = 0;
  virtual void mix(const common::byte_buffer&,
                   const common::byte_buffer&,
                   common::byte_buffer&) const = 0;

  // interface for random_mixer
  virtual std::string get_pull_argument() const = 0;
  virtual std::string pull(const std::string&) const = 0;
  virtual void push(const std::string&) = 0;

  virtual void pack(msgpack::packer<msgpack::sbuffer>& packer) const = 0;
  virtual void unpack(msgpack::object o) = 0;
  virtual void clear() = 0;
};

// TODO(suma): separate other header
class mixable_holder {
 public:
  typedef std::vector<jubatus::util::lang::shared_ptr<mixable> > mixable_list;

  mixable_holder() {
  }

  virtual ~mixable_holder() {
  }

  void register_mixable(jubatus::util::lang::shared_ptr<mixable> m) {
    mixables_.push_back(m);
  }

  mixable_list get_mixables() const {
    return mixables_;
  }

  jubatus::util::concurrent::rw_mutex& rw_mutex() {
    return rw_mutex_;
  }

  std::vector<storage::version> get_versions() {
    jubatus::util::concurrent::scoped_rlock lk_read(rw_mutex_);
    std::vector<storage::version> ret;
    for (size_t i = 0; i < mixables_.size(); ++i) {
      ret.push_back(mixables_[i]->get_version());
    }
    return ret;
  }

  void pack(msgpack::packer<msgpack::sbuffer>& packer) const {
    packer.pack_array(mixables_.size());
    for (size_t i = 0; i < mixables_.size(); ++i) {
      // TODO(suma): extract model function from mixable0
      model* m = dynamic_cast<model*>(mixables_[i].get());
      if (m) {
        m->pack(packer);
      }
    }
  }

  void unpack(msgpack::object o) {
    if (o.type != msgpack::type::ARRAY ||
        o.via.array.size != mixables_.size()) {
      throw msgpack::type_error();
    }

    for (size_t i = 0; i < mixables_.size(); ++i) {
      // TODO(suma): extract model function from mixable0
      model* m = dynamic_cast<model*>(mixables_[i].get());
      if (m) {
        m->clear();
      }
    }

    msgpack::object* p = o.via.array.ptr;
    for (size_t i = 0; i < mixables_.size(); ++i) {
      // TODO(suma): extract model function from mixable0
      model* m = dynamic_cast<model*>(mixables_[i].get());
      if (m) {
        m->unpack(p[i]);
      }
    }
  }

  // for linear_mixable
  void mix(const std::vector<msgpack::object>& objs, packer& pk) {
    std::vector<linear_mixable*> mixables;
    for (size_t i = 0; i < mixables_.size(); i++) {
      linear_mixable* mixable = dynamic_cast<linear_mixable*>(mixables_[i].get());
      mixables.push_back(mixable);
    }

    std::vector<diff_object> diffs;
    for (size_t i = 0; i < objs.size(); i++) {
      if (objs[i].type != msgpack::type::ARRAY || objs[i].via.array.size != mixables.size()) {
        //throw msgpack::type_error();
        throw JUBATUS_EXCEPTION(common::exception::runtime_error("size: " + 
          jubatus::util::lang::lexical_cast<std::string>(objs[i].via.array.size) + " mixables: " +
          jubatus::util::lang::lexical_cast<std::string>(mixables.size())
          ));
      }
    }

    // convert internal raw object (front)
    for (size_t i = 0; i < mixables.size(); i++) {
      diffs.push_back(mixables[i]->convert_diff_object(objs[0].via.array.ptr[i]));
    }

    // do mix
    for (size_t i = 1; i < objs.size(); i++) {
      msgpack::object* o = objs[i].via.array.ptr;
      for (size_t j = 0; j < mixables.size(); j++) {
        mixables[i]->mix(o[j], diffs[j]);
      }
    }

    // output mixed buffer
    pk.pack_array(mixables.size());
    for (size_t i = 0; i < mixables.size(); i++) {
      diffs[i]->convert_binary(pk);
    }
  }

  void get_diff(packer& pk) const {
    pk.pack_array(mixables_.size());
    for (size_t i = 0; i < mixables_.size(); ++i) {
      linear_mixable* mixable = dynamic_cast<linear_mixable*>(mixables_[i].get());
      if (mixable) {
        mixable->get_diff(pk);
      }
    }
  }

 protected:
  jubatus::util::concurrent::rw_mutex rw_mutex_;
  std::vector<jubatus::util::lang::shared_ptr<mixable> > mixables_;
};

template<typename Model, typename Diff, typename PullArg = std::string>
class __attribute__ ((deprecated)) deprecated_mixable : public mixable0 {
 public:
  typedef Model model_type;
  typedef Diff diff_type;
  typedef jubatus::util::lang::shared_ptr<Model> model_ptr;

  virtual ~deprecated_mixable() {
  }

  virtual void clear() = 0;

  virtual Diff get_diff_impl() const = 0;
  virtual bool put_diff_impl(const Diff&) = 0;
  virtual void mix_impl(const Diff&, const Diff&, Diff&) const = 0;

  virtual PullArg get_pull_argument_impl() const {
    throw JUBATUS_EXCEPTION(common::unsupported_method(__func__));
  }
  virtual Diff pull_impl(const PullArg&) const {
    throw JUBATUS_EXCEPTION(common::unsupported_method(__func__));
  }

  virtual void push_impl(const Diff&) {
    throw JUBATUS_EXCEPTION(common::unsupported_method(__func__));
  }

  void set_model(model_ptr m) {
    model_ = m;
  }

  common::byte_buffer get_diff() const {
    if (model_) {
      common::byte_buffer buf;
      pack_(get_diff_impl(), buf);
      return buf;
    } else {
      throw JUBATUS_EXCEPTION(common::config_not_set());
    }
  }

  bool put_diff(const common::byte_buffer& d) {
    if (model_) {
      Diff diff;
      unpack_(d, diff);
      return put_diff_impl(diff);
    } else {
      throw JUBATUS_EXCEPTION(common::config_not_set());
    }
  }

  void mix(
      const common::byte_buffer& lhs,
      const common::byte_buffer& rhs,
      common::byte_buffer& mixed_buf) const {
    Diff left, right, mixed;
    unpack_(lhs, left);
    unpack_(rhs, right);
    mix_impl(left, right, mixed);
    pack_(mixed, mixed_buf);
  }

  std::string get_pull_argument() const {
    if (model_) {
      std::string buf;
      pack_(get_pull_argument_impl(), buf);
      return buf;
    } else {
      throw JUBATUS_EXCEPTION(common::config_not_set());
    }
  }

  std::string pull(const std::string& a) const {
    if (model_) {
      std::string buf;
      PullArg arg;
      unpack_(a, arg);
      pack_(pull_impl(arg), buf);
      return buf;
    } else {
      throw JUBATUS_EXCEPTION(common::config_not_set());
    }
  }

  void push(const std::string& d) {
    if (model_) {
      Diff diff;
      unpack_(d, diff);
      push_impl(diff);
    } else {
      throw JUBATUS_EXCEPTION(common::config_not_set());
    }
  }

  void pack(msgpack::packer<msgpack::sbuffer>& packer) const {
    model_->pack(packer);
  }

  void unpack(msgpack::object o) {
    model_->unpack(o);
  }

  model_ptr get_model() const {
    return model_;
  }

 private:
  void unpack_(const common::byte_buffer& buf, Diff& d) const {
    msgpack::unpacked msg;
    msgpack::unpack(&msg, buf.ptr(), buf.size());
    msg.get().convert(&d);
  }

  void pack_(const Diff& d, common::byte_buffer& buf) const {
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, d);
    buf.assign(sbuf.data(), sbuf.size());
  }

  template <class T>
  void unpack_(const std::string& buf, T& d) const {
    msgpack::unpacked msg;
    msgpack::unpack(&msg, buf.data(), buf.size());
    msg.get().convert(&d);
  }

  template <class T>
  void pack_(const T& d, std::string& buf) const {
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, d);
    buf.assign(sbuf.data(), sbuf.size());
  }

  model_ptr model_;
};

template<typename Model, typename Diff, typename PullArg = std::string>
class __attribute__((__deprecated__)) delegating_mixable : public deprecated_mixable<Model, Diff, PullArg> {
 public:
  Diff get_diff_impl() const {
    Diff diff;
    this->get_model()->get_diff(diff);
    return diff;
  }

  bool put_diff_impl(const Diff& diff) {
    return this->get_model()->set_mixed_and_clear_diff(diff);
  }

  storage::version get_version() const {
    return this->get_model()->get_version();
  }

  void mix_impl(const Diff& lhs, const Diff& rhs, Diff& mixed) const {
    mixed = lhs;
    this->get_model()->mix(rhs, mixed);
  }

  void clear() {
    this->get_model()->clear();
  }
};

}  // namespace framework
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_FRAMEWORK_MIXABLE_HPP_