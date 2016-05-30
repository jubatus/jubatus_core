#!/bin/sh

find ${@} '(' \
      -name "*.cpp" -or \
      -name "*.cc" -or \
      -name "*.hpp" -or \
      -name "*.h" \
    ')' -and -not '(' \
      -path 'jubatus/core/third_party/*' -or \
      -path 'jubatus/util/*' \
    ')' \
| xargs "$(dirname ${0})/cpplint/cpplint.py" --filter=-runtime/references,-runtime/rtti 2>&1 \
| grep -v '^Done processing '

"$(dirname ${0})/check_include_guard.sh" $(find ${@} -name "*.hpp")
