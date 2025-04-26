{
  pkg-config,
  python,
  pybind11,
  meson-python,
  meson,
  nixf,
  buildPythonPackage,
  nlohmann_json,
  boost,
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
    nixf
    nlohmann_json
    boost
  ];
}
