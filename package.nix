{
  pkg-config,
  python,
  pybind11,
  meson-python,
  meson,
  pkgsStatic,
  buildPythonPackage,
}:

buildPythonPackage {
  pname = "pynixf";
  version = "0.1.0";

  src = ./.;

  pyproject = true;

  build-system = [
    meson-python
    meson
    pkg-config
  ];

  buildInputs = [
    python
    pybind11
    meson-python
    pkgsStatic.nixf
  ];
}
