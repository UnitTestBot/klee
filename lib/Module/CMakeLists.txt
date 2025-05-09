#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
set(KLEE_MODULE_COMPONENT_SRCS
  Annotation.cpp
  CallSplitter.cpp
  CallRemover.cpp
  Checks.cpp
  CodeGraphInfo.cpp
  FunctionAlias.cpp
  LocationInfo.cpp
  InstructionOperandTypeCheckPass.cpp
  IntrinsicCleaner.cpp
  FreezeLower.cpp
  KInstruction.cpp
  KModule.cpp
  KValue.cpp
  LocalVarDeclarationFinderPass.cpp
  LowerSwitch.cpp
  ModuleUtil.cpp
  OptNone.cpp
  PhiCleaner.cpp
  RaiseAsm.cpp
  ReturnLocationFinderPass.cpp
  ReturnSplitter.cpp
  SarifReport.cpp
  Target.cpp
  TargetHash.cpp
  TargetForest.cpp
)

if ("${LLVM_VERSION_MAJOR}" LESS 17)
  LIST(APPEND KLEE_MODULE_COMPONENT_SRCS
          InstrumentLegacy.cpp
          OptimizeLegacy.cpp
  )
else ()
  LIST(APPEND KLEE_MODULE_COMPONENT_SRCS
          Instrument.cpp
          Optimize.cpp
  )
endif ()


add_library(kleeModule
  ${KLEE_MODULE_COMPONENT_SRCS}
)

llvm_config(kleeModule "${USE_LLVM_SHARED}"
  bitreader
  bitwriter
  codegen
  ipo
  irreader
  linker
  support
  scalaropts
  instcombine
  transformutils
  analysis
  object
  mc
  binaryformat
  )

target_link_libraries(kleeModule PRIVATE
  kleeSupport
)

target_include_directories(kleeModule SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_include_directories(kleeModule PRIVATE ${KLEE_INCLUDE_DIRS})
target_compile_options(kleeModule PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(kleeModule PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
