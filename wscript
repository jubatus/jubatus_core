# -*- python -*-
from waflib import Options
from waflib.Errors import TaskNotReady
from functools import partial
import os
import sys

VERSION = '1.0.1'
ABI_VERSION = VERSION
APPNAME = 'jubatus_core'

top = '.'
out = 'build'
subdirs = ['jubatus']

def options(opt):
  opt.load('compiler_cxx')
  opt.load('unittest_gtest')
  opt.load('gnu_dirs')

  opt.add_option('--enable-debug',
                 action='store_true', default=False,
                 dest='debug', help='build for debug')

  opt.add_option('--enable-gcov',
                 action='store_true', default=False,
                 dest='gcov', help='only for debug')

  opt.add_option('--disable-eigen',
                 action='store_true', default=False,
                 dest='disable_eigen', help='disable internal Eigen and algorithms using it')

  opt.add_option('--fsanitize',
                 action='store', default="",
                 dest='fsanitize', help='specify sanitizer')

  opt.recurse(subdirs)

def configure(conf):
  env = conf.env

  env.append_unique('CXXFLAGS', ['-O2', '-Wall', '-g', '-pipe', '-fno-omit-frame-pointer', '-pthread'])

  conf.load('compiler_cxx')
  conf.load('unittest_gtest')
  conf.load('gnu_dirs')

  ver = env.CC_VERSION
  if env.COMPILER_CXX != 'g++' or int(ver[0]) < 4 or (int(ver[0]) == 4 and int(ver[1]) < 6):
    env.append_unique('CXXFLAGS', '-D_FORTIFY_SOURCE=1')

  conf.check_cxx(lib = 'pthread')

  # Generate config.hpp
  conf.env.JUBATUS_PLUGIN_DIR = conf.env['LIBDIR'] + '/jubatus/plugin'
  conf.define('JUBATUS_CORE_VERSION', VERSION)
  conf.define('JUBATUS_CORE_APPNAME', APPNAME)
  conf.define('JUBATUS_PLUGIN_DIR', conf.env.JUBATUS_PLUGIN_DIR)
  conf.write_config_header('jubatus/core_config.hpp', guard="JUBATUS_CORE_CONFIG_HPP_", remove=False)

  # Version constants
  conf.env.VERSION = VERSION
  conf.env.ABI_VERSION = ABI_VERSION

  conf.check_cxx(lib = 'msgpackc')

  if Options.options.debug:
    """
    You can compile "debug-enabled" version of Jubatus Core by ``./waf configure --enable-debug ...``.
    In debug-enabled Jubatus Core, assertions are enabled.

    In addition, if you want to enable Glibc C++ debugging feature (useful to debug STL-related
    issues), you can uncomment the following line.  Note that dependency libraries (log4cxx) must
    must be recompiled with this option.
    """
    # conf.define('_GLIBCXX_DEBUG', 1)

    """
    The following flag enables sanity check to detect double acquision of a read lock of pthread_rwlock
    in the same thread.  More precisely, it raises assertion if the thread that already owns read lock
    tries to take a read/write lock.
    """
    conf.define('JUBATUS_UTIL_CONCURRENT_RWMUTEX_ERRORCHECK', 1)
  else:
    # Disable standard assertions
    conf.define('NDEBUG', 1)

    # Disable Jubatus specific assertions
    conf.define('JUBATUS_DISABLE_ASSERTIONS', 1)

  if Options.options.gcov:
    conf.env.append_value('CXXFLAGS', '-fprofile-arcs')
    conf.env.append_value('CXXFLAGS', '-ftest-coverage')
    conf.env.append_value('LINKFLAGS', '-lgcov')
    conf.env.append_value('LINKFLAGS', '--coverage')

  sanitizer_names = Options.options.fsanitize
  if len(sanitizer_names) > 0:
    conf.env.append_unique('CXXFLAGS', '-fsanitize=' + sanitizer_names)
    conf.env.append_unique('LINKFLAGS', '-fsanitize=' + sanitizer_names)

  conf.define('BUILD_DIR',  conf.bldnode.abspath())

  conf.env.USE_EIGEN = not Options.options.disable_eigen
  if conf.env.USE_EIGEN:
    conf.define('JUBATUS_USE_EIGEN', 1)

  func_multiver_test_code = '''#include <immintrin.h>
__attribute__((target("default"))) void test() {}
__attribute__((target("sse2"))) void test() { __m128i x; _mm_xor_si128(x,x); }
__attribute__((target("avx2"))) void test() { __m256i x; _mm256_xor_si256(x,x); }
int main() { test(); }
'''
  func_multiver_enabled = conf.check_cxx(
    fragment=func_multiver_test_code,
    msg='Checking for function multiversioning',
    execute=True,
    mandatory=False,
    define_name='JUBATUS_USE_FMV')
  if not func_multiver_enabled:
    sse2_test_code = '#ifdef __SSE2__\nint main() {}\n#else\n#error\n#endif'
    conf.check_cxx(fragment=sse2_test_code, msg='Checking for sse2', mandatory=False)

  conf.recurse(subdirs)

def build(bld):

  bld(name = 'core_headers', export_includes = './')

  def add_prefix(bld, paths):
    prefix_dir = os.path.dirname(bld.cur_script.relpath())
    return [os.path.join(prefix_dir, str(path)) for path in paths]

  bld.add_prefix = partial(add_prefix, bld)

  bld.core_sources = []
  bld.core_headers = []
  bld.core_use = []

  bld.recurse(subdirs)

  # core
  bld.shlib(source=list(set(bld.core_sources)), target='jubatus_core', use=list(set(bld.core_use)), vnum = ABI_VERSION)
  bld.install_files('${PREFIX}/include/', list(set(bld.core_headers)), relative_trick=True)


  bld(source = 'jubatus_core.pc.in',
      prefix = bld.env['PREFIX'],
      exec_prefix = '${prefix}',
      libdir = bld.env['LIBDIR'],
      includedir = '${prefix}/include',
      PACKAGE = APPNAME,
      VERSION = VERSION)

def cpplint(ctx):
  sys.stderr.write('Running cpplint...\n')
  ctx.cmd_and_log(['tools/codestyle/run_cpplint.sh'] + subdirs)

def check_cmath(ctx):
  ctx.cmd_and_log('tools/codestyle/cmath_finder.sh')
