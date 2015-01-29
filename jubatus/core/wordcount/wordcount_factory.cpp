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

#include "wordcount_factory.hpp"
#include <string>
#include "wordcount_base.hpp"
#include <iostream>

#include "space_saving.hpp"
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/jsonconfig.hpp"
#include "../common/exception.hpp"

using std::string;
using jubatus::core::common::jsonconfig::config_cast_check;

namespace jubatus {
namespace core {
namespace wordcount {

jubatus::util::lang::shared_ptr<wordcount_base>
wordcount_factory::create_wordcount(const string& name,
                                    const common::jsonconfig::config& param) {
  try {
    wordcount_config conf = config_cast_check<wordcount_config>(param);
    if (name == "space_saving") {
      return jubatus::util::lang::shared_ptr<wordcount_base>(new space_saving(conf));
    } else {
      throw JUBATUS_EXCEPTION(common::unsupported_method("wordcount(" + name + ")"));
    }
  } catch (const common::jsonconfig::cast_check_error& e) {
    std::vector<jubatus::util::lang::shared_ptr<common::jsonconfig::config_error> > errors = e.errors();
    for (size_t i = 0; i < errors.size(); ++i) {
      std::cout << errors[i]->what() << std::endl;
    }
    throw e;
  }
}


}  // namespace wordcount
}  // namespace core
}  // namespace jubatus

