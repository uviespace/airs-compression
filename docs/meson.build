doxygen = find_program('doxygen', required : false)
dot = find_program('dot', required : false)
if doxygen.found()
  cdata = configuration_data()
  cdata.set('VERSION', meson.project_version())
  cdata.set('PROJECT_NAME', meson.project_name())
  cdata.set('PROJECT_BRIEF', 'AIRS Portable Compression Engine')
  cdata.set('TOP_SRCDIR', meson.project_source_root())
  cdata.set('INPUT', meson.project_source_root())
  cdata.set('OUTPUT_DIRECTORY', join_paths(meson.project_build_root(), 'docs'))
  exclude_path = [
    join_paths(meson.project_source_root(), 'test'),
    join_paths(meson.project_source_root(), 'subprojects'),
    meson.project_build_root()
  ]
  cdata.set('EXCLUDE', ' '.join(exclude_path))
  cdata.set('EXCLUDE_PATTERNS', '*/*build*/*')

  if dot.found()
    cdata.set('HAVE_DOT', 'YES')
  else
    cdata.set('HAVE_DOT', 'NO')
  endif

  doxyfile = configure_file(input : files('Doxyfile.in'),
                            output : 'Doxyfile',
                            configuration : cdata,
                            install : false)

  html_target = custom_target('docs',
    input : [doxyfile, src_common, src_compress, cli_src, asciidoc_files],
    output : ['html'],
    command : [doxygen, doxyfile],
    depends : [cmp_lib, airspacecli],
    install : true,
    install_dir : join_paths(get_option('datadir'), 'docs'),
    install_tag : 'doc')
endif
