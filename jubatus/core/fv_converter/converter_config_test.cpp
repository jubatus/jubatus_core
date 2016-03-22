// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2011 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "jubatus/util/text/json.h"
#include "converter_config.hpp"
#include "datum.hpp"
#include "datum_to_fv_converter.hpp"
#include "exception.hpp"

using std::map;
using std::string;
using jubatus::util::data::optional;

namespace jubatus {
namespace core {
namespace fv_converter {

#if defined(HAVE_RE2) || defined(HAVE_ONIGURUMA)
TEST(converter_config, config) {
  try {
    std::ifstream ifs("./test_input/config.json");
    converter_config config;
    ifs >> jubatus::util::text::json::via_json(config);

    datum_to_fv_converter conv;
    initialize_converter(config, conv);

    datum d;
    d.string_values_.push_back(std::make_pair("user/name", "Taro Yamada"));
    d.string_values_.push_back(std::make_pair("user/text", "hoge fuga <foo>"));

    d.num_values_.push_back(std::make_pair("user/id", 1000));
    d.num_values_.push_back(std::make_pair("user/age", 20));

    common::sfv_t f;
    conv.convert(d, f);

    common::sfv_t exp;
    exp.push_back(std::make_pair("user/name$Taro Yamada@str#bin/bin", 1.));

    // detagging filter
    exp.push_back(
        std::make_pair("user/text-detagged$hoge fuga @str#bin/bin", 1.));

    exp.push_back(std::make_pair("user/id@str$1000", 1.));
    exp.push_back(std::make_pair("user/age@num", 20.));
    exp.push_back(std::make_pair("user/age@log", std::log(20.)));
    exp.push_back(std::make_pair("user/age+1@num", 21.));

    std::sort(f.begin(), f.end());
    std::sort(exp.begin(), exp.end());
    ASSERT_EQ(exp, f);
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    throw;
  } catch (const std::string& e) {
    std::cout << e << std::endl;
    throw;
  }
}
#endif

TEST(converter_config, hash) {
  converter_config config;
  num_rule r = {"*", optional<string>(), "str"};
  config.num_rules = std::vector<num_rule>();
  config.num_rules->push_back(r);
  config.hash_max_size = 1;

  datum_to_fv_converter conv;
  initialize_converter(config, conv);

  datum d;
  d.num_values_.push_back(std::make_pair("age", 10));

  common::sfv_t f;
  conv.convert(d, f);

  EXPECT_EQ("0", f[0].first);
}

TEST(converter_config, hash_negative) {
  converter_config config;
  config.hash_max_size = 0;
  datum_to_fv_converter conv;

  EXPECT_THROW(initialize_converter(config, conv), converter_exception);
}

TEST(converter_config, combination) {
  converter_config config;

  num_rule nr = {"*", optional<string>(), "num"};
  config.num_rules = std::vector<num_rule>();
  config.num_rules->push_back(nr);

  combination_rule cr = {"*@num", "*@num", optional<string>("b*"), optional<string>("b*"), "add"};
  config.combination_rules = std::vector<combination_rule>();
  config.combination_rules->push_back(cr);

  datum_to_fv_converter conv;
  initialize_converter(config, conv);
  
  datum d;
  d.num_values_.push_back(std::make_pair("a1", 1.0));
  d.num_values_.push_back(std::make_pair("a2", 2.0));
  d.num_values_.push_back(std::make_pair("b", 300.0));
  
  common::sfv_t f;
  conv.convert(d, f);
  
  common::sfv_t exp;
  exp.push_back(std::make_pair("a1@num", 1.0));
  exp.push_back(std::make_pair("a2@num", 2.0));
  exp.push_back(std::make_pair("b@num", 300.0));
  exp.push_back(std::make_pair("a1@num&a2@num/add", 3.0));

  std::sort(f.begin(), f.end());
  std::sort(exp.begin(), exp.end());
  ASSERT_EQ(exp, f);
  ASSERT_EQ(4u, f.size());
}

TEST(make_fv_converter, empty_config) {
  jubatus::util::text::json::json
    js(new jubatus::util::text::json::json_object);
  converter_config config =
    jubatus::util::text::json::json_cast<converter_config>(js);
  datum_to_fv_converter conv;
  initialize_converter(config, conv);

  datum d;
  common::sfv_t f;
  conv.convert(d, f);

  EXPECT_EQ(0u, f.size());
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
