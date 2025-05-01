download_z3() {
  mkdir -p "${Z3_SRC_PATH}"
  cd "${Z3_SRC_PATH}"
  wget "https://github.com/Z3Prover/z3/archive/z3-${Z3_VERSION}-arm64-osx.zip"
  unzip -o "z3-${Z3_VERSION}-arm64-osx.zip"
  rm "z3-${Z3_VERSION}-arm64-osx.zip"
}
