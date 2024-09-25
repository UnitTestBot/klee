# Build dependencies
install_build_dependencies_klee() {
  if [[ $(to_bool "${ENABLE_DOXYGEN}") -eq 1 ]]; then
    brew install doxygen graphviz
  fi

  # Install `lit` required for testing
  brew install lit

  brew install pkg-config
  brew install coreutils

  # Install package in virtual environment
  python3 -m pip install "tabulate"
}
