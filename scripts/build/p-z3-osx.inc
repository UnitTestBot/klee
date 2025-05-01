install_binary_artifact_z3() {
  local Z3_SUFFIX="${SANITIZER_SUFFIX}"
  local Z3_INSTALL_PATH="${BASE}/z3-${Z3_VERSION}-install${Z3_SUFFIX}"
  mkdir -p "${Z3_INSTALL_PATH}"
  cd "${Z3_INSTALL_PATH}"
  wget "https://github.com/Z3Prover/z3/releases/download/z3-${Z3_VERSION}/z3-${Z3_VERSION}-arm64-osx-13.7.4.zip"
  unzip -o "z3-${Z3_VERSION}-arm64-osx-13.7.4.zip"
  rm "z3-${Z3_VERSION}-arm64-osx-13.7.4.zip"
}

setup_artifact_variables_llvm() {
  Z3_SUFFIX="${SANITIZER_SUFFIX}"
  Z3_INSTALL_PATH="${BASE}/z3-${Z3_VERSION}-install${Z3_SUFFIX}"
}

#!/usr/bin/env bash
install_build_dependencies_z3() {
  # Install essential dependency
  # Ignore if already installed
  brew install wget || exit
}
