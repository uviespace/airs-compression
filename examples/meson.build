simple_compression_exe = executable('simple_compression',
  'simple_compression.c',
  link_with : cmp_lib,
  include_directories : inc_cmp,
  implicit_include_directories: false,
  c_args : non_testing_flags
)

test('Run simple compression example', simple_compression_exe)
