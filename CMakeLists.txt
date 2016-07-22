cmake_minimum_required (VERSION 3.0)

#####
# LLVM includes
set (LLVM_ROOT "" CACHE PATH "Root of LLVM install")

if ( NOT EXISTS ${LLVM_ROOT}/include/llvm )
  message(FATAL_ERROR "LLVM (${LLVM_ROOT}) not found.")
endif()

list (APPEND CMAKE_PREFIX_PATH "${LLVM_ROOT}/share/llvm/cmake")
find_package (LLVM REQUIRED CONFIG)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include (HandleLLVMOptions)
include (AddLLVM)

######
# LLVM local CMake modules
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include (LLVMUtils)


# add_subdirectory(tests)
add_subdirectory(screen_pass)
