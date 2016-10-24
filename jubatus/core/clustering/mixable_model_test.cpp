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

#include <string>
#include <gtest/gtest.h>
#include "jubatus/util/lang/scoped_ptr.h"
#include "clustering.hpp"
#include "clustering_factory.hpp"
#include "testutil.hpp"
#include "../common/jsonconfig.hpp"

using jubatus::util::text::json::json;
using jubatus::util::text::json::json_object;
using jubatus::util::text::json::to_json;

namespace jubatus {
namespace core {
namespace clustering {

class mixable_model_test : public ::testing::Test {
  protected:
  const std::string name_;
  mixable_model_test() : name_("name") {
    json js(new json_object);
    js["parameter"] = new json_object;
    js["parameter"]["k"] = to_json(3);
    js["parameter"]["seed"] = to_json(0);
    js["compressor_parameter"] = new json_object;
    js["compressor_parameter"]["bucket_size"] = to_json(10000);
    js["compressor_parameter"]["bucket_length"] = to_json(2);
    js["compressor_parameter"]["compressed_bucket_size"] = to_json(400);
    js["compressor_parameter"]["bicriteria_base_size"] = to_json(10);
    js["compressor_parameter"]["forgetting_factor"] = to_json(2);
    js["compressor_parameter"]["forgetting_threshold"] = to_json(0.05);
    js["compressor_parameter"]["seed"] = to_json(0);
    common::jsonconfig::config conf(js);

    model_ = clustering_factory::create(
              name_,
              "kmeans",
              "compressive",
              conf["parameter"],
              conf["compressor_parameter"]);
    storage_ = model_->get_storage();
  }
  jubatus::util::lang::shared_ptr<clustering> model_;
  jubatus::util::lang::shared_ptr<storage> storage_;
};

TEST_F(mixable_model_test, get_diff) {
  static const size_t N = 200;
  static const size_t D = 2;
  model_->push(get_points(N, D));
  diff_t df;
  storage_->get_diff(df);
  ASSERT_EQ(df.size(), (size_t)1);
  ASSERT_EQ(df[0].first, name_);
  ASSERT_EQ(df[0].second.size(), N);
}

TEST_F(mixable_model_test, reduce) {
  static const size_t N = 200;
  static const size_t D = 2;
  model_->push(get_points(N, D));
  diff_t df;
  storage_->get_diff(df);
  diff_t df2;
  storage_->mix(df, df2);
  ASSERT_EQ(df2.size(), (size_t)1);
  ASSERT_EQ(df2[0].first, name_);
  ASSERT_EQ(df2[0].second.size(), N);

  storage_->get_diff(df2);
  storage_->mix(df, df2);
  ASSERT_EQ(df2.size(), (size_t)1);
  ASSERT_EQ(df2[0].first, name_);
  ASSERT_EQ(df2[0].second.size(), 2*N);

  storage_->get_diff(df2);
  df2[0].first += "2";
  storage_->mix(df, df2);
  ASSERT_EQ(df2.size(), (size_t)2);
  ASSERT_EQ(df2[0].first, name_);
  ASSERT_EQ(df2[0].second.size(), N);
  ASSERT_EQ(df2[1].first, name_+"2");
  ASSERT_EQ(df2[1].second.size(), N);
}

TEST_F(mixable_model_test, put_diff) {
  static const size_t N = 200;
  static const size_t D = 2;
  model_->push(get_points(N, D));
  diff_t df, df2;
  storage_->get_diff(df);
  storage_->get_diff(df2);
  df2[0].first = "name2";
  storage_->mix(df, df2);
  storage_->put_diff(df2);

  wplist coreset = model_->get_coreset();
  ASSERT_EQ(coreset.size(), 2*N);
  storage_->get_diff(df);
  ASSERT_EQ(df.size(), (size_t)1);
  ASSERT_EQ(df[0].second.size(), N);
}

}  // namespace clustering
}  // namespace core
}  // namespace jubatus
