// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
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
#include "jubatus/util/data/string/utility.h"
#include "jubatus/util/lang/cast.h"
#include "recommender_mock_util.hpp"

using std::pair;
using std::string;
using std::vector;
using jubatus::util::data::string::split;

namespace jubatus {
namespace core {
namespace recommender {

common::sfv_t make_sfv(const string& repr) {
  vector<string> elems = split(repr, ' ');
  common::sfv_t sfv(elems.size());
  for (size_t i = 0; i < elems.size(); ++i) {
    vector<string> parts = split(elems[i], ':');
    sfv[i] = make_pair(
        parts[0],
        jubatus::util::lang::lexical_cast<double>(parts[1]));
  }
  return sfv;
}

vector<pair<string, double> > make_ids(const string& repr) {
  return make_sfv(repr);
}

}  // namespace recommender
}  // namespace core
}  // namespace jubatus
