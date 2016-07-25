// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2016 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include "char_splitter.hpp"

namespace jubatus {
namespace core {
namespace fv_converter {

namespace {

std::vector<std::pair<size_t, size_t> > make_pairs(int*xs) {
    std::vector<std::pair<size_t, size_t> > v;
    for (int* x  = xs; *x != -1; x += 2) {
        v.push_back(std::make_pair(x[0], x[1]));
    }
    return v;
}

class char_splitter_test : public ::testing::Test {
};

}  // namespace

TEST_F(char_splitter_test, empty) {
    std::string separators = "";
    char_splitter splitter = char_splitter(separators);

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split("" , bs);
        ASSERT_EQ(0u , bs.size());
    }

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split("STRING", bs);
        int exp[] = {0, 6, -1};
        ASSERT_EQ(make_pairs(exp), bs);
    }
}

TEST_F(char_splitter_test, with_one_charactor) {
    std::string separators = ",";
    char_splitter splitter = char_splitter(separators);

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split("HELLO,WORLD" , bs);
        int exp[] = {0, 5, 6, 5, -1};
        ASSERT_EQ(make_pairs(exp), bs);
    }

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split(",,aaa", bs);
        int exp[] = {2, 3, -1};
        ASSERT_EQ(make_pairs(exp), bs);
    }

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split(",,", bs);
        int exp[] = {-1};
        ASSERT_EQ(make_pairs(exp), bs);
    }

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split("", bs);
        int exp[] = {-1};
        ASSERT_EQ(make_pairs(exp), bs);
    }

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split("日本語,English" , bs);
        int exp[] = {0, 9, 10, 7, -1};
        ASSERT_EQ(make_pairs(exp), bs);
    }
}


TEST_F(char_splitter_test, with_two_charactors) {
    std::string separators = " ,";
    char_splitter splitter = char_splitter(separators);

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split("HELLO,WORLD. Good morning.", bs);
        int exp[] = {0, 5, 6, 6, 13, 4, 18, 8, -1};
        ASSERT_EQ(make_pairs(exp), bs);
    }
}

TEST_F(char_splitter_test, with_abc) {
    std::string separators = "abc";
    char_splitter splitter = char_splitter(separators);

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split("zzzabxxccyyy", bs);
        int exp[] = {0, 3, 5, 2, 9, 3, -1};
        ASSERT_EQ(make_pairs(exp), bs);
    }
}


TEST_F(char_splitter_test, with_multibyte) {
    std::string separators = "：m";
    char_splitter splitter = char_splitter(separators);

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split("zzzabxx：ccyyy", bs);
        int exp[] = {0, 7, 10, 5, -1};
        ASSERT_EQ(make_pairs(exp), bs);
    }

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split("年齢：ゼロ", bs);
        int exp[] = {0, 6, 9, 6, -1};
        ASSERT_EQ(make_pairs(exp), bs);
    }

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split("日本語：English", bs);
        int exp[] = {0, 9, 12, 7, -1};
        ASSERT_EQ(make_pairs(exp), bs);
    }
}

TEST_F(char_splitter_test, with_multibyte_multiseparators) {
    std::string separators = "、。";
    char_splitter splitter = char_splitter(separators);

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split("日本語文章です。こんにちは、世界。", bs);
        int exp[] = {0, 21, 24, 15, 42, 6, -1};
        ASSERT_EQ(make_pairs(exp), bs);
    }

    {
        std::vector<std::pair<size_t, size_t> > bs;
        splitter.split("全半角混じりの文章です。123ＡＢＣ", bs);
        int exp[] = {0, 33, 36, 12, -1};
        ASSERT_EQ(make_pairs(exp), bs);
    }
}

}  // namespace fv_converter
}  // namespace core
}  // namespace jubatus
