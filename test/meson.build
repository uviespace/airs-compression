# Set environment variable for Sanitizers
# Options were copied from oss-fuzz and adapted
# see: https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
test_env = environment()

if get_option('b_sanitize').contains('address')
  test_env.set('ASAN_OPTIONS',
    'abort_on_error=1',
    'allocator_may_return_null=1',
    'allocator_release_to_os_interval_ms=500',
    'detect_container_overflow=1',
    'detect_stack_use_after_return=1',
    'fast_unwind_on_fatal=0','handle_abort=1',
    'handle_segv=1',
    'handle_sigill=1',
    'max_uar_stack_size_log=16',
    'print_scariness=1',
    'quarantine_size_mb=10',
    'strict_memcmp=1',
    'symbolize=1',
    'use_sigaltstack=1',
    'dedup_token_length=3')

  if compiler.has_argument('-fsanitize=leak')
    test_env.append('ASAN_OPTIONS', 'detect_leaks=1')
    # workaround for https://github.com/llvm/llvm-project/issues/115992
    if target_machine.system() == 'darwin' and target_machine.cpu_family() == 'aarch64'
      test_env.set('LSAN_OPTIONS',
        'suppressions=' + meson.current_source_dir()+'/.leak.suppress',
        'print_suppressions=0')
    endif
  endif
endif

if get_option('b_sanitize').contains('undefined')
  test_env.set('UBSAN_OPTIONS',
    'halt_on_error=1',
    'print_stacktrace=1',
    'print_summary=1',
    'symbolize=1',
    'dedup_token_length=3')
endif

if get_option('b_sanitize').contains('memory')
  test_env.set('MSAN_OPTIONS',
    'abort_on_error=1',
    'print_stats=1',
    'symbolize=1',
    'dedup_token_length=3')
endif

# Unity test runner generates implementation-only sources without headers,
# so we suppress prototype/declaration warnings for unit tests
unit_testing_flags = [
    '-Wno-missing-declarations',
    '-Wno-missing-prototypes',
    '-Wno-redundant-decls',
    '-DCMP_MESON_BUILD_ROOT="' + meson.project_build_root() + '/"',
]

# setup the Unity Test framework
ruby = find_program('ruby', required : false)
if ruby.found()
  unity_dep = dependency('unity', fallback : ['unity', 'unity_dep'])

  # use the test runner generator script
  gen_test_runner = subproject('unity').get_variable('gen_test_runner')
  test_lib = static_library('test_common_lib',
    files(['test_common.c']),
    dependencies : [unity_dep])

  unit_test_src = files([
    'test_initialisation.c',
    'test_cmp.c',
    'test_header.c',
    'test_cmp_errors.c',
    'test_preprocessing.c',
    'test_encoder.c',
    'test_params_parse.c',
    'test_buildsetup.c'])

  foreach test_file : unit_test_src
    test_name = fs.name(test_file).split('.')[0]

    test_runner = gen_test_runner.process(test_file)
    test_exe = executable(test_name, test_file, test_runner,
      include_directories : inc_cmp,
      c_args : unit_testing_flags,
      link_with : [test_lib, cmp_lib, cli_lib],
      dependencies : [unity_dep])

    test(test_name.replace('test_', '') + ' units tests',
      test_exe,
      env : test_env)
  endforeach

else
   message('Ruby not found. Tests that require Ruby will be disabled.')
endif


cli_tests = files([
  'cli_basic_test.py',
  'cli_compression_test.py'
  ])

foreach test : cli_tests
  test_name = fs.name(test).split('.')[0]

  # test('CLI Basic Test',
  test(test_name,
    find_program(test),
    args : ['--cli', airspacecli.full_path(),
            '--test-root',  meson.current_build_dir()],
    depends : airspacecli,
    env : test_env)
endforeach
