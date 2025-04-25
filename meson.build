project('pynixf', 'cpp',
  default_options: [
    'cpp_std=gnu++20',
    'buildtype=release',
    'warning_level=3',
  ],
  version: '0.1.0'
)

# Dependencies
py_mod = import('python')
py = py_mod.find_installation('python3', required: true)
py_dep = py.dependency(required: true)
nixf_dep = dependency('nixf', required: true)

subdir('src')

py.install_sources(['nixf.pyi'])
