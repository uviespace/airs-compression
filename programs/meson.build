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
  install: false)

airspacecli = executable('airspace',
  'airspacecli.c',
  include_directories : inc_cmp,
  implicit_include_directories: false,
  link_with : [cli_lib, cmp_lib],
  c_args : '-D_POSIX_C_SOURCE=1', # This is needed for some stdio.h function in Linux with ANSI C
  install: true)

if asciidoctor.found()
  custom_target(
    input : 'README.adoc',
    output : 'README.html',
    command : [asciidoctor, '--destination-dir=@OUTDIR@', '@INPUT@']
  )
endif
