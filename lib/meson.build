subdir('common')
subdir('compress')

xxhash_inc = subproject(
  'xxhash',
  default_options: ['cli=false','default_library=static']
).get_variable('inc')

inc_cmp = include_directories('.')
install_headers('cmp.h', 'cmp_errors.h', 'cmp_header.h')

cmp_lib = static_library('cmp',
  src_common, src_compress,
  include_directories: [inc_cmp, xxhash_inc],
  implicit_include_directories: false,
  install: true)

pkg = import('pkgconfig')
pkg.generate(cmp_lib, description: 'AIRSPACE compression static library')
