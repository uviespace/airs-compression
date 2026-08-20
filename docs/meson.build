doxygen = find_program('doxygen', required: false)
dot = find_program('dot', required: false)
pandoc = find_program('pandoc', required: false)

# We rely on the file.full_path() feature, available since Meson 1.4.0.
if meson.version().version_compare('<1.4')
    message('Meson >= 1.4.0 is required to build documentation; skipping documentation.')
    subdir_done()
endif

if doxygen.found()
    src_root = meson.project_source_root()

    # Doxygen INPUT files. Declared as File objects (not plain path strings) so
    # that Meson tracks them as inputs and rebuilds the docs when any of them
    # changes. The Doxyfile below needs absolute paths, so they are resolved
    # via full_path(): Doxygen runs from the build directory, where relative
    # paths would not point at the sources.
    doxygen_api_input = files(
        '../lib/cmp.h',
        '../lib/cmp_errors.h',
        '../lib/cmp_header.h',
        '../examples/simple_compression.c',
    )
    doxygen_page_input = files(
        'design.md',
        'format.md',
        'mainpage.md',
        'topics.dox',
    )
    doxygen_input = doxygen_api_input + doxygen_page_input

    # Doxygen does not understand AsciiDoc, so convert it to markdown with pandoc.
    guide_pages = []
    if pandoc.found()
        pandoc_command = [
            pandoc,
            '--from=asciidoc',
            '--to=gfm', # GitHub flavour md
            '--standalone',
            '--shift-heading-level-by=1',
            '--lua-filter', files('rewrite-page-links.lua'),
            '--template', files('pandoc-page.template'),
        ]

        install_md = custom_target(
            input: files('../INSTALL.adoc'),
            output: 'INSTALL.md',
            command: pandoc_command + [
                '--output=@OUTPUT@',
                '@INPUT@',
            ],
        )
        cli_md = custom_target(
            input: files('../programs/README.adoc'),
            output: 'CLI.md',
            command: pandoc_command + [
                '--output=@OUTPUT@',
                '@INPUT@',
            ],
        )

        guide_pages = [install_md, cli_md]
    else
        message('Pandoc not found. Excluding INSTALL and CLI guides from documentation.')
    endif

    doxygen_awesome = subproject('doxygen-awesome-css')
    stylesheet_files = doxygen_awesome.get_variable('doxygen_awesome_css')
    stylesheet_files += doxygen_awesome.get_variable(
        'doxygen_awesome_sidebar_only_css',
    )
    icon_file = files('AIRSPACE_Icon.svg')
    logo_file = files('AIRSPACE_Logo.svg')

    # Doxyfile substitution needs plain string paths, so resolve the File
    # objects with full_path().
    doxygen_input_paths = []
    foreach f : doxygen_input
        doxygen_input_paths += f.full_path()
    endforeach
    guide_page_paths = []
    foreach f : guide_pages
        guide_page_paths += f.full_path()
    endforeach
    stylesheet_full_paths = []
    foreach f : stylesheet_files
        stylesheet_full_paths += f.full_path()
    endforeach

    config = configuration_data()
    config.set('VERSION', meson.project_version())
    config.set('PROJECT_NAME', meson.project_name())
    config.set('PROJECT_BRIEF', 'Compression Engine')
    config.set('OUTPUT_DIRECTORY', meson.current_build_dir())
    config.set('INPUT', ' '.join(doxygen_input_paths + guide_page_paths))
    config.set('PROJECT_LOGO', logo_file[0].full_path())
    config.set('PROJECT_ICON', icon_file[0].full_path())
    config.set('STRIP_FROM_PATH', src_root + '/')
    config.set('STRIP_FROM_INC_PATH', src_root / 'lib')
    config.set('EXAMPLE_PATH', src_root / 'examples')
    config.set('HTML_EXTRA_STYLESHEET', ' '.join(stylesheet_full_paths))
    config.set('HTML_EXTRA_FILES', ' '.join([
        icon_file[0].full_path(),
        logo_file[0].full_path(),
    ]))
    config.set('HAVE_DOT', dot.found() ? 'YES' : 'NO')

    doxyfile = configure_file(
        input: files('Doxyfile.in'),
        output: 'Doxyfile',
        configuration: config,
        install: false,
    )

    custom_target(
        'docs',
        input: [
            doxyfile,
            doxygen_input,
            guide_pages,
            stylesheet_files,
            icon_file,
            logo_file,
        ],
        output: 'html',
        command: [doxygen, doxyfile],
        install: true,
        install_dir: get_option('datadir') / 'docs',
        install_tag: 'doc',
    )
endif
