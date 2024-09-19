project(
  'tiny_thread_pool',
  'cpp',
  version : '0.1',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17'
  ]
)

tiny_thread_pool = static_library(
  'tiny_thread_pool',
  ['src/tiny_thread_pool.cpp'],
  cpp_args: ['-fno-exceptions']
)

tiny_thread_pool_dep = declare_dependency(
  link_with: tiny_thread_pool,
  include_directories: include_directories('src')
)

test_tiny_thread_pool = executable(
  'test_tiny_thread_pool',
  ['tests/test_tiny_thread_pool.cpp'],
  include_directories: include_directories('src'),
  link_with: [tiny_thread_pool]
)

test('test_tiny_thread_pool', test_tiny_thread_pool)
