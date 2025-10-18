project('AIRSPACE', 'c',
  meson_version : '>=0.63.0',
  version : run_command(find_program('python3'),
    'test/get_library_version.py', 'lib/cmp.h',
     check : true).stdout().strip(),
  license : 'GPL-2.0',
  default_options : [
    'c_std=c89',
    'warning_level=3',
    'pkgconfig.relocatable=true',
  ]
)

fs = import('fs')

compiler = meson.get_compiler('c')

# Built-in options
use_debug = get_option('debug')

# Compiler flags
add_global_arguments('-Wno-long-long', language: 'c')

cc_flags = []
if use_debug
  debug_flags = [
    # more information at https://github.com/Cyan4973/Writing_Safer_C_code/blob/master/4_compiler_warnings.md
    '-Wconversion',
    '-Wcast-qual' ,
    '-Wstrict-aliasing=1',
    '-Wcast-align',
    '-Wpointer-arith',
    '-Winit-self',
    '-Wshadow',
    '-Wswitch-enum',
    '-Wstrict-prototypes',
    '-Wformat=2',
    '-Wfloat-equal',
    '-Wundef',
    '-Wvla',
    '-Wdeclaration-after-statement',
    '-Wwrite-strings',
    '-Wold-style-definition',
    '-Wmissing-include-dirs',
    '-Wundef',
    '-Wdouble-promotion',
    '-Wstrict-overflow=2',
    '-Wformat-truncation',
    '-Wformat-security',
    '-Woverflow',
    '-Wdocumentation',
    '-Wmissing-declarations',
    '-Wmissing-prototypes',
    '-Wredundant-decls'
  ]
  cc_flags += compiler.get_supported_arguments(debug_flags)
endif
add_project_arguments(cc_flags, language : 'c')

if ['windows', 'cygwin'].contains(host_machine.system()) and compiler.get_id() == 'gcc'
  # by default, MinGW on win32 behaves as if it ignores __attribute__((packed)),
  # you need to add -mno-ms-bitfields to make it work as expected.
  # See: https://wintermade.it/blog/posts/__attribute__packed-on-windows-is-ignored-with-mingw.html
  add_project_arguments('-mno-ms-bitfields', language : 'c')
  add_global_link_arguments('-static', language: 'c')
endif


asciidoctor = find_program('asciidoctor', required: false)
asciidoc_files = files([
  'README.adoc',
  'INSTALL.adoc'
])

if asciidoctor.found()
  html_files = []
  foreach f : asciidoc_files
    html_files += fs.replace_suffix(f, '.html')
  endforeach
  custom_target(
    input : asciidoc_files,
    output : html_files,
    command : [asciidoctor, '--destination-dir=@OUTDIR@', '--attribute', 'imagesdir=@SOURCE_ROOT@/docs', '@INPUT@']
  )
endif


subdir('lib')
subdir('programs')
subdir('examples')
subdir('test')
subdir('docs')

summary({
  'Ruby found' : ruby.found(),
  'Asciidoctor found' : asciidoctor.found(),
  'Doxygen found' : doxygen.found(),
  'dot found' : dot.found(),
}, section : 'Auto-detected features')
