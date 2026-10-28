# Currently there is no OS layer implementation for baremetal"
has_os_layer = host_machine.system() != 'baremetal'

cli_src = files([
  'arena.c',
  'params_parse.c',
  'log.c',
  'file.c',
])
if has_os_layer
  cli_src += files('os.c')
endif

cli_c_args = ['-D_POSIX_C_SOURCE=200809L']
if host_machine.system() == 'darwin'
  cli_c_args += ['-D_DARWIN_C_SOURCE']
endif

cli_lib = static_library('airspace_cli',
  cli_src,
  include_directories: [inc_cmp],
  implicit_include_directories: false,
  # glibc hides prototypes (e.g., snprintf(3)) from <stdio.h> under strict C89,
  # see feature_test_macros(7)
  c_args: cli_c_args,
  install: false
)

airspacecli = disabler()
if has_os_layer
  airspacecli = executable(
    'airspace',
    'airspacecli.c',
    include_directories: inc_cmp,
    implicit_include_directories: false,
    link_with: [cli_lib, cmp_lib],
    c_args: cli_c_args,
    install: true,
  )
endif

bench_cmp = executable('bench_cmp',
  'bench_cmp.c', 'clock.c', 'arena.c',
  include_directories : inc_cmp,
  implicit_include_directories: false,
  link_with : [cmp_lib],
  c_args : cli_c_args,
  install : false)

if asciidoctor.found()
  custom_target(
    input : 'README.adoc',
    output : 'README.html',
    command : [asciidoctor, '--destination-dir=@OUTDIR@', '@INPUT@']
  )

  if has_os_layer
    custom_target('airspace-manpage',
      input : 'airspace.1.adoc',
      output : 'airspace.1',
      command : [asciidoctor, '-b', 'manpage', '-o', '@OUTPUT@', '@INPUT@'],
      install : true,
      install_dir : get_option('mandir') / 'man1',
      install_tag : 'doc',
    )
  endif
endif
