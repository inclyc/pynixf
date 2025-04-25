# pybind11 detection
pybind11_dep = dependency('pybind11', required: true)

# Create Python extension module
py.extension_module(
  'nixf',
  'nixf_python.cpp',
  dependencies: [py_dep, nixf_dep, pybind11_dep],
  install: true,
  install_dir: py.get_install_dir(pure: false),
)
