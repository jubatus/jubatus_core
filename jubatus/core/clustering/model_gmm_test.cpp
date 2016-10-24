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

#include <vector>
#include <gtest/gtest.h>
#include "jubatus/util/lang/scoped_ptr.h"
#include "../common/type.hpp"
#include "clustering.hpp"
#include "types.hpp"
#include "testutil.hpp"
#include "../common/jsonconfig.hpp"
#include "clustering_factory.hpp"

using std::vector;
using jubatus::util::text::json::json;
using jubatus::util::text::json::json_object;
using jubatus::util::text::json::to_json;

namespace jubatus {
namespace core {
namespace clustering {

class model_gmm_test : public ::testing::Test {
 protected:
  static const size_t k_ = 2;

  model_gmm_test() {
    json js(new json_object);
    js["parameter"] = new json_object;
    js["parameter"]["k"] = to_json(3);
    js["parameter"]["seed"] = to_json(0);
    js["compressor_parameter"] = new json_object;
    js["compressor_parameter"]["bucket_size"] = to_json(100);
    js["compressor_parameter"]["bucket_length"] = to_json(4);
    js["compressor_parameter"]["compressed_bucket_size"] = to_json(5);
    js["compressor_parameter"]["bicriteria_base_size"] = to_json(1);
    js["compressor_parameter"]["forgetting_factor"] = to_json(2);
    js["compressor_parameter"]["forgetting_threshold"] = to_json(0.05);
    js["compressor_parameter"]["seed"] = to_json(0);
    common::jsonconfig::config conf(js);
    model_ = clustering_factory::create("test",
                                        "gmm",
                                        "compressive",
                                        conf["parameter"],
                                        conf["compressor_parameter"]);
  }
  jubatus::util::lang::shared_ptr<clustering> model_;
};

TEST_F(model_gmm_test, initial_centers) {
  EXPECT_THROW(model_->get_k_center(), not_performed);
}

TEST_F(model_gmm_test, push_small) {
  static const size_t N = 99;
  static const size_t D = 2;
  model_->push(get_points(N, D));
  vector<weighted_point> coreset = model_->get_coreset();
  ASSERT_EQ(coreset.size(), N);
  ASSERT_EQ(coreset.front().data.size(), D);

  EXPECT_THROW(model_->get_k_center(), not_performed);
}

TEST_F(model_gmm_test, compression_and_clusteringing) {
  static const size_t N = 100;  // bucket_size
  static const size_t D = 2;
  std::cout << "total adding points : " << N << std::endl;
  model_->push(get_points(N, D));
  vector<weighted_point> coreset = model_->get_coreset();
  ASSERT_EQ(
            coreset.size(), static_cast<size_t>(5));  // compressed_bucket_size

  vector<common::sfv_t> centers = model_->get_k_center();
  ASSERT_EQ(centers.size(), static_cast<size_t>(3));  // k

  vector<wplist> core_members = model_->get_core_members();
  ASSERT_GT(core_members.size(), 0ul);

  wplist nearest_members = model_->get_nearest_members(get_point(D).data);
  ASSERT_GT(nearest_members.size(), 0ul);

  common::sfv_t nearest_center = model_->get_nearest_center(get_point(D).data);
}


TEST_F(model_gmm_test, bucket_management_and_forgetting) {
  static const size_t N = 100* 4;  // bucket_size * 4
  static const size_t D = 2;
  model_->push(get_points(N, D));
  vector<weighted_point> coreset = model_->get_coreset();
  ASSERT_EQ(
            coreset.size(), static_cast<size_t>(5));  // compressed_bucket_size

  model_->push(get_points(N, D));
  coreset = model_->get_coreset();
  ASSERT_EQ(
            coreset.size(), static_cast<size_t>(5));  // compressed_bucket_size
}

}  // namespace clustering
}  // namespace core
}  // namespace jubatus
