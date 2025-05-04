#!/usr/bin/env bash
install_binary_artifact_z3() {
  brew install z3
}

setup_artifact_variables_z3() {
  Z3_INSTALL_PATH="$(brew --cellar z3)/${Z3_VERSION}"
}

install_build_dependencies_z3() {
  return 0
}
