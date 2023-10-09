add_library(kleeRunner
        run_klee.cpp
        )

set(KLEE_LIBS
        kleeCore
        )

set(RUN_KLEE_INCLUDE_DEST "include/run_klee")

target_link_libraries(kleeRunner ${KLEE_LIBS})
target_include_directories(kleeRunner PUBLIC ${KLEE_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_compile_options(kleeRunner PUBLIC ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(kleeRunner PUBLIC ${KLEE_COMPONENT_CXX_DEFINES})
set(run_klee_header_path ${PROJECT_SOURCE_DIR}/include/klee/Runner)
set(run_klee_headers ${run_klee_header_path}/run_klee.h)

set_target_properties(kleeRunner PROPERTIES PUBLIC_HEADER "${run_klee_headers}")

install(TARGETS kleeRunner
        EXPORT run_klee DESTINATION "${RUN_KLEE_LIB_DEST}"
        PUBLIC_HEADER DESTINATION "${RUN_KLEE_INCLUDE_DEST}"
        )


# The KLEE depends on the runtimes
add_dependencies(kleeRunner BuildKLEERuntimes)
