install_binary_artifact_z3() {
  mkdir -p "${Z3_INSTALL_PATH}"
  cd "${Z3_INSTALL_PATH}"
  wget "https://github.com/Z3Prover/z3/releases/download/z3-${Z3_VERSION}/z3-${Z3_VERSION}-arm64-osx-13.7.4.zip"
  unzip -o "z3-${Z3_VERSION}-arm64-osx-13.7.4.zip"
  rm "z3-${Z3_VERSION}-arm64-osx-13.7.4.zip"
}

#!/usr/bin/env bash
install_build_dependencies_z3() {
  # Install essential dependency
  # Ignore if already installed
  brew install wget || exit
}
