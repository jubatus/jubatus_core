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

#ifndef JUBATUS_CORE_UNLEARNER_UNLEARNER_BASE_HPP_
#define JUBATUS_CORE_UNLEARNER_UNLEARNER_BASE_HPP_

#include <string>
#include "jubatus/util/lang/function.h"
#include "jubatus/core/framework/packer.hpp"

namespace msgpack {
class object;
}

namespace jubatus {
namespace core {
namespace unlearner {

typedef jubatus::util::lang::function<void (std::string)> unlearning_callback;

// Base class of unlearners.
//
// Unlearning is a functionality to keep the model size up to constant.
// Unlearner is a strategy of unlearning that decides which part of the model to
// be removed. User should register a callback that actually removes something
// on unlearning.
class unlearner_base {
 public:
  virtual ~unlearner_base() {}

  void set_callback(const unlearning_callback& callback) {
    callback_ = callback;
  }

  virtual std::string type() const = 0;
  virtual void clear() = 0;

  // Tests if the given id can be touched.
  //
  // This function returns true if the given |id| can be touched (i.e., the
  // number of registered unique ids is less than the limit, or the number of
  // registered unique ids reached the limit but there are at least one ID that
  // is available for unlearning), and false otherwise.
  virtual bool can_touch(const std::string& id) = 0;

  // Informs that the item of given id is updated in the model.
  //
  // If the number of unique ids reaches to the predefined upper bound, one or
  // more ids are unlearned. In that case, when there are no IDs available for
  // unlearning, this function returns false. On unlearning an id X, a callback
  // given by |set_callback| is called by passing X as an argument.
  virtual bool touch(const std::string& id) = 0;

  // Informs that the item of given id is removed from the model.
  //
  // This function returns true when succeed to remove |id|. If |id| does not
  // exist in memory, it returns false.
  virtual bool remove(const std::string& id) = 0;

  // Checks that a given id is still begin in memory.
  //
  // If |id| has not been touched, or touched but unlearned at some point and
  // not been touched since then, this function returns false. If |id| has been
  // touched and not unlearned since then, it returns true.
  virtual bool exists_in_memory(const std::string& id) const = 0;

  virtual void export_model(framework::packer& pk) const = 0;
  virtual void import_model(msgpack::object o) = 0;

 protected:
  void unlearn(const std::string& id) const {
    callback_(id);
  }

 private:
  unlearning_callback callback_;
};

}  // namespace unlearner
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_UNLEARNER_UNLEARNER_BASE_HPP_
