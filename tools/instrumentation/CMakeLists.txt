add_subdirectory(src_tooling)
# install(PROGRAMS instrumenter.py RUNTIME DESTINATION bin)
file(COPY src_tooling_runner DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/")
configure_file(instrumenter.py "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/instrumenter" COPYONLY)