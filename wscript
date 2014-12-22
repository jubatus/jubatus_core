# -*- python -*-
import Options
from waflib.Errors import TaskNotReady
from functools import partial
import os
import sys

VERSION = '0.0.7'
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
  conf.define('JUBATUS_CORE_VERSION', VERSION)
  conf.define('JUBATUS_CORE_APPNAME', APPNAME)
  conf.write_config_header('jubatus/core_config.hpp', guard="JUBATUS_CORE_CONFIG_HPP_", remove=False)

  # Version constants
  conf.env.VERSION = VERSION
  conf.env.ABI_VERSION = ABI_VERSION

  conf.check_cxx(lib = 'msgpack')

  if Options.options.debug:
    conf.define('_GLIBCXX_DEBUG', 1)
  else:
    conf.define('NDEBUG', 1)
    conf.define('JUBATUS_DISABLE_ASSERTIONS', 1)

  if Options.options.gcov:
    conf.env.append_value('CXXFLAGS', '-fprofile-arcs')
    conf.env.append_value('CXXFLAGS', '-ftest-coverage')
    conf.env.append_value('LINKFLAGS', '-lgcov')

  sanitizer_names = Options.options.fsanitize
  if len(sanitizer_names) > 0:
    conf.env.append_unique('CXXFLAGS', '-fsanitize=' + sanitizer_names)
    conf.env.append_unique('LINKFLAGS', '-fsanitize=' + sanitizer_names)

  conf.define('BUILD_DIR',  conf.bldnode.abspath())

  conf.env.USE_EIGEN = not Options.options.disable_eigen
  if conf.env.USE_EIGEN:
    conf.define('JUBATUS_USE_EIGEN', 1)

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
  import fnmatch, tempfile
  cpplint = ctx.path.find_node('tools/codestyle/cpplint/cpplint.py')
  src_dir = ctx.path.find_node('jubatus')
  file_list = []
  excludes = ['jubatus/core/third_party/*',
              'jubatus/util/*.h',
              'jubatus/util/*.cpp',
              'jubatus/util/*/*.h',
              'jubatus/util/*/*.cpp',
              'jubatus/util/*/*/*.h',
              'jubatus/util/*/*/*.cpp']
  for file in src_dir.ant_glob('**/*.cpp **/*.cc **/*.hpp **/*.h'):
    file_list += [file.path_from(ctx.path)]
  for exclude in excludes:
    file_list = [f for f in file_list if not fnmatch.fnmatch(f, exclude)]
  tmp_file = tempfile.NamedTemporaryFile(delete=True);
  tmp_file.write("\n".join(file_list));
  tmp_file.flush()
  ctx.exec_command('cat ' + tmp_file.name + ' | xargs "' + cpplint.abspath() + '" --filter=-runtime/references,-runtime/rtti 2>&1')
  tmp_file.close()

def check_cmath(ctx):
  ctx.cmd_and_log('tools/codestyle/cmath_finder.sh')
