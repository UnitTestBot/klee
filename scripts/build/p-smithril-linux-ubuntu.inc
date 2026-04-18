# Build dependencies Smithril
install_build_dependencies_smithril() {
  source "${DIR}/common-functions"
  with_sudo apt update -y

  dependencies=(
    build-essential

    ninja-build
    python3
    python3-pip
    cmake
    curl
    git
    wget
    ca-certificates
    libclang-dev
    libssl-dev
  )


  #Install essential dependencies
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"

  with_sudo apt-get update -y
  with_sudo apt-get -y --no-install-recommends install pkg-config cmake-data m4

  pip3 install --user meson
  base_path="$(python3 -m site --user-base)"
  export PATH="$PATH:${base_path}/bin"

  curl https://sh.rustup.rs -sSf | sh -s -- -y
  . "$HOME/.cargo/env"
  cargo install cargo-c
}
