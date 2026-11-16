# Currently there is no OS layer implementation for baremetal"
has_os_layer = host_machine.system() != 'baremetal'

cli_src = files([
  '../lib/common/arena.c',
  'params_parse.c',
  'log.c',
  'file.c',
])
if has_os_layer
  if host_machine.system() == 'windows'
    cli_src += files('os_windows.c')
  else
    cli_src += files('os_posix.c')
  endif
endif

cli_lib = static_library('airspace_cli',
  cli_src,
  include_directories: [inc_cmp],
  implicit_include_directories: false,
  install: false
)

airspacecli = disabler()
if has_os_layer
  cli_deps = []
  if host_machine.system() == 'windows'
    cli_deps += compiler.find_library('shlwapi')
  endif
  airspacecli = executable(
    'airspace',
    'airspacecli.c',
    include_directories: inc_cmp,
    implicit_include_directories: false,
    link_with: [cli_lib, cmp_lib],
    dependencies: cli_deps,
    install: true,
  )
endif

bench_cmp = executable('bench_cmp',
  'bench_cmp.c', 'clock.c', '../lib/common/arena.c',
  include_directories : inc_cmp,
  implicit_include_directories: false,
  link_with : [cmp_lib],
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
