#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_executable(klee
  klee.cpp
)

target_link_libraries(klee PUBLIC kleeRunner)

# Copy run script
configure_file(${CMAKE_SOURCE_DIR}/scripts/kleef ${CMAKE_BINARY_DIR}/bin/kleef COPYONLY)

install(TARGETS klee RUNTIME DESTINATION bin)

# The KLEE binary depends on the runtimes
add_dependencies(klee BuildKLEERuntimes)
