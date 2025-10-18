subdir('common')
subdir('compress')

xxhash_inc = subproject('xxhash').get_variable('inc')

inc_cmp = include_directories('.')

cmp_lib = static_library('cmp',
  src_common, src_compress,
  include_directories: [inc_cmp, xxhash_inc],
  implicit_include_directories: false,
  install: true)
