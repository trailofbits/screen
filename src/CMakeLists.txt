
add_definitions (${LLVM_DEFINITIONS})
include_directories (${LLVM_INCLUDE_DIRS})

set (LLVM_RUNTIME_OUTPUT_INTDIR "${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INT_DIR}")
set (LLVM_LIBRARY_OUTPUT_INTDIR "${CMAKE_BINARY_DIR}/lib/${CMAKE_CFG_INT_DIR}")

add_llvm_loadable_module(screen screen.cpp traverse.cpp)
