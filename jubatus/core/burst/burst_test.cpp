// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2014 Preferred Infrastructure and Nippon Telegraph and Telephone Corporation.
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

#include "../framework/stream_writer.hpp"
#include "burst_result.hpp"
#include "jubatus/util/lang/bind.h"

using jubatus::util::lang::bind;
using jubatus::util::lang::_1;

namespace jubatus {
namespace core {
namespace burst {

const burst_options default_burst_options = {
  10,       // window_batch_size
  1.0,      // batch_interval
  10,       // result_window_rotate_size
  5,        // max_reuse_batch_num
  DBL_MAX   // costcut_threshold
};

const keyword_params default_keyword_params = {
  2.0,  // scaling_param
  1.0   // gamma
};

inline void copy_by_pack_and_unpack(const burst& src, burst& dst) {
  msgpack::sbuffer buf;
  framework::stream_writer<msgpack::sbuffer> st(buf);
  framework::jubatus_packer jp(st);
  framework::packer packer(jp);
  src.pack(packer);

  msgpack::unpacked unpacked;
  msgpack::unpack(&unpacked, buf.data(), buf.size());
  dst.unpack(unpacked.get());
}

template<class F>
void test_burst_and_its_copy(burst& tested, F f) {
  burst copied(default_burst_options);
  copy_by_pack_and_unpack(tested, copied);

  f(tested);
  f(copied);
}

TEST(burst, simple) {
  burst tested(default_burst_options);

  tested.calculate_results();  // do nothing because no keywords added
  {
    struct test {
      static void invoke(burst& tested) {
        burst::result_map r = tested.get_all_bursted_results();
        ASSERT_EQ(0u, r.size());
      }
    };
    test_burst_and_its_copy(tested, &test::invoke);
  }

  // add keywords
  {
    keyword_params params = default_keyword_params;

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

    struct test {
      static void invoke(burst& tested,
                         const std::vector<std::string>& keywords,
                         const keyword_params& params) {
        burst::keyword_list r = tested.get_all_keywords();
        ASSERT_EQ(keywords.size(), r.size());
        std::set<std::string> keyword_set(keywords.begin(), keywords.end());
        for (size_t i = 0; i < r.size(); ++i) {
          ASSERT_EQ(1u, keyword_set.count(r[i].keyword));
          ASSERT_EQ(params.scaling_param, r[i].scaling_param);
          ASSERT_EQ(params.gamma, r[i].gamma);
        }
      }
    };
    test_burst_and_its_copy(tested, bind(&test::invoke, _1, keywords, params));
  }

  tested.calculate_results();  // do nothing because no documents added
  {
    struct test {
      static void invoke(burst& tested) {
        burst::result_map r = tested.get_all_bursted_results();
        ASSERT_EQ(0u, r.size());
      }
    };
    test_burst_and_its_copy(tested, &test::invoke);
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

    double batch_interval = default_burst_options.batch_interval;
    double pos = batch_interval/2;
    for (size_t i = 0; i < documents.size(); ++i) {
      bool added = tested.add_document(documents[i], pos);
      ASSERT_TRUE(added);
      pos += batch_interval;
    }
  }

  {
    // before calculated
    struct test {
      static void invoke(burst& tested) {
        burst::result_map r = tested.get_all_bursted_results();
        ASSERT_EQ(0u, r.size());
      }
    };
    test_burst_and_its_copy(tested, &test::invoke);
  }

  tested.calculate_results();
  {
    struct test {
      static void invoke(burst& tested) {
        burst::result_map r = tested.get_all_bursted_results();
        ASSERT_EQ(1u, r.size());
        ASSERT_EQ(1u, r.count("haskell"));
      }
    };
    test_burst_and_its_copy(tested, &test::invoke);
  }

  {
    burst::diff_t diff;
    tested.get_diff(diff);
    tested.put_diff(diff);

    struct test {
      static void invoke(burst& tested) {
        burst::result_map r = tested.get_all_bursted_results();
        ASSERT_EQ(1u, r.size());
        ASSERT_EQ(1u, r.count("haskell"));
      }
    };
    test_burst_and_its_copy(tested, &test::invoke);
  }
}

}  // namespace burst
}  // namespace core
}  // namespace jubatus
