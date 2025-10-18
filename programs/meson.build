cli_src = files([
  'params_parse.c',
  'log.c',
  'file.c',
  'util.c'
])

cli_lib = static_library('airspace_cli',
  cli_src,
  include_directories: [inc_cmp],
  implicit_include_directories: false,
  # glibc hides prototypes (e.g., snprintf(3)) from <stdio.h> under strict C89,
  # see feature_test_macros(7)
  c_args: ['-D_POSIX_C_SOURCE=200809L'],
  install: false)

airspacecli = executable('airspace',
  'airspacecli.c',
  include_directories : inc_cmp,
  implicit_include_directories: false,
  link_with : [cli_lib, cmp_lib],
  # glibc hides prototypes (e.g., snprintf(3)) from <stdio.h> under strict C89,
  # see feature_test_macros(7)
  c_args : ['-D_POSIX_C_SOURCE=200809L'],
  install: true)

if asciidoctor.found()
  custom_target(
    input : 'README.adoc',
    output : 'README.html',
    command : [asciidoctor, '--destination-dir=@OUTDIR@', '@INPUT@']
  )
endif
