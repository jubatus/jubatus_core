// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include <string>
#include <gtest/gtest.h>
#include "jubatus/util/lang/shared_ptr.h"

#include "minhash.hpp"
#include "../common/jsonconfig.hpp"


using jubatus::util::text::json::json;
using jubatus::util::text::json::json_object;
using jubatus::util::text::json::to_json;
using jubatus::core::common::jsonconfig::config_cast;

namespace jubatus {
namespace core {
namespace recommender {

TEST(minhash, config_validation_without_unlearner) {
  jubatus::util::lang::shared_ptr<minhash> r;
  minhash::config c;

  // 1 <= hash_num
  c.hash_num = 0;
  ASSERT_THROW(r.reset(new minhash(c)), common::invalid_parameter);
  c.hash_num = 1;
  ASSERT_NO_THROW(r.reset(new minhash(c)));
  c.hash_num = 2;
  ASSERT_NO_THROW(r.reset(new minhash(c)));
}

TEST(minhash, config_validation_with_unlearner) {
  jubatus::util::lang::shared_ptr<minhash> r;

  {
    json js(new json_object);
    js["hash_num"] = to_json(64);

    js["unlearner"] = to_json(std::string("lru"));
    js["unlearner_parameter"] = new json_object;
    js["unlearner_parameter"]["max_size"] = to_json(1);
    common::jsonconfig::config conf(js);

    ASSERT_NO_THROW(r.reset(new minhash(config_cast<minhash::config>(conf))));
  }

  {
    json js(new json_object);
    js["hash_num"] = to_json(64);

    js["unlearner"] = to_json(std::string("lru"));
    common::jsonconfig::config conf(js);

    ASSERT_THROW(
      r.reset(new minhash(config_cast<minhash::config>(conf))),
      common::config_exception);
  }
}

}  // namespace recommender
}  // namespace core
}  // namespace jubatus
