#!/usr/bin/env bash
install_build_dependencies_immer() {
  # Install essential dependency
  # Ignore if already installed
  brew install wget || /usr/bin/true
}
