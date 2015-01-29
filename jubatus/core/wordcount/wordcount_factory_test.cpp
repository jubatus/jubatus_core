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

#include <string>
#include <gtest/gtest.h>
#include <iostream>

#include "wordcount_factory.hpp"
#include "wordcount_base.hpp"
#include "../common/jsonconfig.hpp"

using jubatus::util::text::json::json;
using jubatus::util::text::json::json_null;
using jubatus::util::text::json::json_object;
using jubatus::util::text::json::to_json;

namespace jubatus {
namespace core {
namespace wordcount {

TEST(wordcount_factory, create) {
  json js(new json_object);
  js["capacity"] = to_json(1000);
  common::jsonconfig::config conf(js);
  try {
    wordcount_factory::create_wordcount("space_saving", conf);
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
  }
  EXPECT_NO_THROW(wordcount_factory::create_wordcount("space_saving", conf));
}

// --- validation test ---

TEST(wordcount_factory, invalid_config) {
  json js(new json_object);
  js["capacity"] = to_json(1000);
  common::jsonconfig::config conf(js);

  EXPECT_THROW(wordcount_factory::create_wordcount("perceptron", conf),
               common::unsupported_method);
}

}  // namespace wordcount
}  // namespace core
}  // namespace jubatus
