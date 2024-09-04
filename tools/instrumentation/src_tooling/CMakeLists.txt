################################
# 
################################

# message(Clang: "${CLANG_CMAKE_PATH}")

# find_package(Clang REQUIRED HINTS "${CLANG_CMAKE_PATH}")
# include(AddLLVM)

# find_package(LLVM REQUIRED HINTS "${LLVM_CMAKE_PATH}")
# include("/usr/lib64/cmake/clang/AddClang.cmake")

################################

set(LLVM_LINK_COMPONENTS support)

add_clang_executable(label-adder
    LabelerMain.cpp

    LabelerASTConsumer.cpp
    LabelerASTVisitor.cpp
    LabelerFrontendAction.cpp
    LabelerFrontendActionFactory.cpp

    LabelerASTConsumer.hpp
    LabelerASTVisitor.hpp
    LabelerFrontendAction.hpp
    LabelerFrontendActionFactory.hpp
  )

target_include_directories(label-adder SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_directories(label-adder PRIVATE ${LLVM_LIBRARY_DIRS})

target_link_libraries(label-adder
  PRIVATE
  clang-cpp
  )

install(TARGETS label-adder RUNTIME DESTINATION bin)
