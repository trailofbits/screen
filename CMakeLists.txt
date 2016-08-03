cmake_minimum_required (VERSION 3.1)

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

######
# Use the compiler we just got to build screen as well
# set (CMAKE_CXX_COMPILER "${LLVM_ROOT}/bin/clang++")
# set (CMAKE_C_COMPILER "${LLVM_ROOT}/bin/clang")

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pedantic")

# set (LIBCPP "-stdlib=libc++ -lc++abi")
# set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LIBCPP}")
# set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LIBCPP}")
# set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LIBCPP}")
# set (CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} ${LIBCPP}")

add_subdirectory(tests)
add_subdirectory(src)

if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif ()
