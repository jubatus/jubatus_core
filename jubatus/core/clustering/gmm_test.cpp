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

#include <gtest/gtest.h>
#include "jubatus/util/lang/scoped_ptr.h"
#include "jubatus/util/math/random.h"
#include "gmm.hpp"
#include "gmm_types.hpp"
#include "../common/exception.hpp"

namespace jubatus {
namespace core {
namespace clustering {

class gmm_test : public ::testing::Test {
 protected:
  static const size_t k_ = 2;
  static const size_t d_ = 2;
  static const double epsilon_;

  virtual void SetUp() {
    gmm_.reset(new gmm);
  }

  void do_batch(size_t num) {
    eigen_wsvec_list_t vs;
    for (size_t i = 0; i < num; ++i) {
      eigen_wsvec_t wsvec_a, wsvec_b;
      wsvec_a.data = eigen_svec_t(2);
      wsvec_b.data = eigen_svec_t(2);
      wsvec_a.data.coeffRef(0) = r_.next_gaussian() + 2.0;
      wsvec_a.data.coeffRef(1) = r_.next_gaussian() + 2.0;
      wsvec_b.data.coeffRef(0) = r_.next_gaussian() - 2.0;
      wsvec_b.data.coeffRef(1) = r_.next_gaussian() - 2.0;
      wsvec_a.weight = 1.0;
      wsvec_b.weight = 1.0;
      vs.push_back(wsvec_a);
      vs.push_back(wsvec_b);
    }
    gmm_->batch(vs, k_, d_);
  }

  jubatus::util::math::random::mtrand r_;
  jubatus::util::lang::scoped_ptr<gmm> gmm_;
};

const double gmm_test::epsilon_ = 0.3;

TEST_F(gmm_test, centers_and_covs) {
  do_batch(500);

  eigen_svec_list_t centers = gmm_->get_centers();
  double diff0 = std::abs(centers[0].coeffRef(0) - centers[1].coeffRef(0));
  double diff1 = std::abs(centers[0].coeffRef(1) - centers[1].coeffRef(1));
  double sum0 = std::abs(centers[0].coeffRef(0) + centers[1].coeffRef(0));
  double sum1 = std::abs(centers[0].coeffRef(1) + centers[1].coeffRef(1));
  EXPECT_NEAR(diff0, 4.0, epsilon_);
  EXPECT_NEAR(diff1, 4.0, epsilon_);
  EXPECT_NEAR(sum0, 0.0, epsilon_);
  EXPECT_NEAR(sum1, 0.0, epsilon_);

  eigen_smat_list_t covs = gmm_->get_covs();
  EXPECT_NEAR(covs[0].coeffRef(0, 0), 1.0, epsilon_);
  EXPECT_NEAR(covs[0].coeffRef(1, 1), 1.0, epsilon_);
  EXPECT_NEAR(covs[0].coeffRef(0, 1), 0.0, epsilon_);
  EXPECT_NEAR(covs[0].coeffRef(1, 0), 0.0, epsilon_);
  EXPECT_NEAR(covs[1].coeffRef(0, 0), 1.0, epsilon_);
  EXPECT_NEAR(covs[1].coeffRef(1, 1), 1.0, epsilon_);
  EXPECT_NEAR(covs[1].coeffRef(0, 1), 0.0, epsilon_);
  EXPECT_NEAR(covs[1].coeffRef(1, 0), 0.0, epsilon_);
}

TEST_F(gmm_test, nearest_center) {
  do_batch(100);
  eigen_svec_t svec_a(2), svec_b(2);
  svec_a.coeffRef(0) = 2.0;
  svec_a.coeffRef(1) = 2.0;
  svec_b.coeffRef(0) = -2.0;
  svec_b.coeffRef(1) = -2.0;
  eigen_svec_t nc_a = gmm_->get_nearest_center(svec_a);
  eigen_svec_t nc_b = gmm_->get_nearest_center(svec_b);
  int64_t nc_index_a = gmm_->get_nearest_center_index(svec_a);
  int64_t nc_index_b = gmm_->get_nearest_center_index(svec_b);

  EXPECT_NEAR(nc_a.coeffRef(0), 2.0, epsilon_);
  EXPECT_NEAR(nc_a.coeffRef(1), 2.0, epsilon_);
  EXPECT_NEAR(nc_b.coeffRef(0), -2.0, epsilon_);
  EXPECT_NEAR(nc_b.coeffRef(1), -2.0, epsilon_);
  EXPECT_TRUE(nc_index_a != nc_index_b);
}

TEST_F(gmm_test, clear_with_empty_list) {
  do_batch(100);
  eigen_svec_t svec_a(2);
  svec_a.coeffRef(0) = 2.0;
  svec_a.coeffRef(1) = 2.0;
  eigen_svec_t nc_a = gmm_->get_nearest_center(svec_a);
  EXPECT_NEAR(nc_a.coeffRef(0), 2.0, epsilon_);
  EXPECT_NEAR(nc_a.coeffRef(1), 2.0, epsilon_);
  do_batch(0);  // batch update with empty list
  EXPECT_THROW(
    eigen_svec_t nc_b = gmm_->get_nearest_center(svec_a),
    common::exception::runtime_error);
}

}  // namespace clustering
}  // namespace core
}  // namespace jubatus
