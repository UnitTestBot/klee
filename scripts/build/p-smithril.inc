# Build scripts for Smithril
# Variables that any artifact of this package might depend on
setup_build_variables_smithril() {
  SMITHRIL_BUILD_PATH="${BASE}/smithril-${SMITHRIL_VERSION}"
  SMITHRIL_INSTALL_PATH="${BASE}/smithril-${SMITHRIL_VERSION}-install"
  SMITHRIL_SYS_INSTALL_PATH="${BASE}/smithril-sys"
  smithril_url="https://github.com/smithril/smithril.git"

  return 0
}

download_smithril() {
  source "${DIR}/common-functions"
  # Download Smithril
  git_clone_or_update "${smithril_url}" "${BASE}/smithril-${SMITHRIL_VERSION}" "${SMITHRIL_VERSION}"
}

build_smithril() {
  pushd "${BASE}/smithril-${SMITHRIL_VERSION}"

  (
    rustup default nightly
    cargo cinstall --destdir="/" --prefix="${SMITHRIL_INSTALL_PATH}"
  )
  popd
  cd "${SMITHRIL_INSTALL_PATH}" || return 1
  touch "${SMITHRIL_INSTALL_PATH}/.smithril_installed"
}

install_smithril() {
  return 0
}

# Check if the binary artifact is installed
is_installed_smithril() {
  (
    setup_build_variables_smithril
    [[ -f "${SMITHRIL_INSTALL_PATH}/.smithril_installed" ]]
  ) || return 1
}

setup_artifact_variables_smithril() {
  setup_build_variables_smithril
}

get_build_artifacts_smithril() {
  (
    setup_build_variables_smithril
    echo "${SMITHRIL_INSTALL_PATH}"
  )
}

get_docker_config_id_smithril() {
  (
    source "${DIR}/common-functions"
    setup_build_variables_smithril

    smithril_remote_commit="$(get_git_hash "${smithril_url}" "${SMITHRIL_VERSION}")"
    echo "${smithril_remote_commit}"
  )
}
