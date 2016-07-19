cmake_minimum_required (VERSION 3.0)

set (LLVM_ROOT "" CACHE PATH "Root of LLVM install")

if ( NOT EXISTS ${LLVM_ROOT}/include/llvm )
  message(FATAL_ERROR "LLVM (${LLVM_ROOT}) not found.")
endif()

list (APPEND CMAKE_PREFIX_PATH "${LLVM_ROOT}/share/llvm/cmake")
find_package (LLVM REQUIRED CONFIG)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include (HandleLLVMOptions)
include (AddLLVM)

add_subdirectory(screen_pass)
