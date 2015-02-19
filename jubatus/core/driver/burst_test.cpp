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

#include "burst.hpp"

#include <cfloat>
#include <string>
#include <vector>
#include <set>
#include <gtest/gtest.h>

#include "../burst/burst_result.hpp"

namespace jubatus {
namespace core {
namespace driver {

TEST(burst, simple) {
  core::burst::burst_options options = {
    10,       // window_batch_size
    1.0,      // batch_interval
    10,       // result_window_rotate_size
    5,        // max_reuse_batch_num
    DBL_MAX   // costcut_threshold
  };
  core::burst::keyword_params params = {
    2.0,  // scaling_param
    1.0   // gamma
  };


  burst tested(new burst::model_t(options));

  tested.calculate_results();  // do nothing because no keywords added
  {
    burst::result_map r = tested.get_all_bursted_results();
    ASSERT_EQ(0u, r.size());
  }

  // add keywords
  {
    std::vector<std::string> keywords;
    keywords.push_back("scala");
    keywords.push_back("ocaml");
    keywords.push_back("haskell");
    for (size_t i = 0; i < keywords.size(); ++i) {
      bool success = tested.add_keyword(keywords[i], params, true);
      ASSERT_TRUE(success);
    }

    // already added
    ASSERT_FALSE(tested.add_keyword(keywords[0], params, true));

    burst::keyword_list r = tested.get_all_keywords();
    ASSERT_EQ(keywords.size(), r.size());
    std::set<std::string> keyword_set(keywords.begin(), keywords.end());
    for (size_t i = 0; i < r.size(); ++i) {
      ASSERT_EQ(1u, keyword_set.count(r[i].keyword));
      ASSERT_EQ(params.scaling_param, r[i].scaling_param);
      ASSERT_EQ(params.gamma, r[i].gamma);
    }
  }

  tested.calculate_results();  // do nothing because no documents added
  {
    burst::result_map r = tested.get_all_bursted_results();
    ASSERT_EQ(0u, r.size());
  }

  // add documents
  {
    std::vector<std::string> documents;
    documents.push_back("hoge");
    documents.push_back("hoge fuga");
    documents.push_back("hogehoge");
    documents.push_back("piyo");
    documents.push_back("haha");
    documents.push_back("haskell");
    documents.push_back("hogefuga");
    documents.push_back("I like haskell");
    documents.push_back("hoge haskell");
    documents.push_back("piyo");
    documents.push_back("a haskell");
    documents.push_back("haskell fuga");
    documents.push_back("haskell");

    double pos = options.batch_interval/2;
    for (size_t i = 0; i < documents.size(); ++i) {
      bool added = tested.add_document(documents[i], pos);
      ASSERT_TRUE(added);
      pos += options.batch_interval;
    }
  }

  tested.calculate_results();
  {
    burst::result_map r = tested.get_all_bursted_results();
    ASSERT_EQ(1u, r.size());
    ASSERT_EQ(1u, r.count("haskell"));
  }
}

}  // driver namespace
}  // core namespace
}  // jubatus namespace
