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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301

#include "wordcount.hpp"
#include "jubatus/util/lang/shared_ptr.h"
#include "../wordcount/wordcount_factory.hpp"
#include "wordcount_util.hpp"

using std::vector;
using std::pair;
using std::string;
using msgpack::object;
using jubatus::util::lang::shared_ptr;
using jubatus::core::wordcount::wordcount_base;
using jubatus::core::fv_converter::datum;
using jubatus::core::common::sfv_t;
using jubatus::core::fv_converter::datum_to_fv_converter;
using jubatus::core::framework::packer;

namespace jubatus {
namespace core {
namespace driver {

wordcount::wordcount(
    shared_ptr<wordcount_base> wordcount,
    shared_ptr<datum_to_fv_converter>
    converter)
  : converter_(converter),
    wordcount_(wordcount) {
}

bool
wordcount::append(const string& bucket, const datum& datum) {
  sfv_t counts;
  converter_->convert(datum, counts);
  for (size_t i = 0; i < counts.size(); ++i) {
    counts[i].first = util::revert_string_feature(counts[i].first);
  }
  return wordcount_->append(bucket, counts);
}

size_t wordcount::count(const string& bucket, const string& word) const {
  return wordcount_->count(bucket, word);
}

vector<pair<string, size_t> >
wordcount::get_ranking(const string& bucket, size_t size) const {
  return wordcount_->get_ranking(bucket, size);
}

vector<pair<string, size_t> > wordcount::split_test(
    const datum& datum) const {
  sfv_t counts;
  converter_->convert(datum, counts);
  vector<pair<string, size_t> > ret;
  ret.reserve(counts.size());
  std::cerr << "counts_size:" << counts.size() << std::endl;
  for (size_t i = 0; i < counts.size(); ++i) {
    ret.push_back(
        make_pair(util::revert_string_feature(counts[i].first),
                  static_cast<size_t>(counts[i].second)));
  }
  return ret;
}

bool wordcount::clear_bucket(const string& bucket) {
  return wordcount_->clear_bucket(bucket);
}

void wordcount::clear() {
  wordcount_->clear();
}


void wordcount::pack(packer& packer) const {
  wordcount_->pack(packer);
}
void wordcount::unpack(object obj) {
  wordcount_->unpack(obj);
}


}  // namespace driver
}  // namespace core
}  // namespace jubatus
