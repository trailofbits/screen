cmake_minimum_required(VERSION 2.8)

project(PAGAI CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# configuration
option(STATIC_LINK "Whether to build executables as static" OFF)
option(CHECK_EXTERNALS "Whether to check that external dependancies are OK before compiling" ON)
option(ENABLE_PROFILING "Whether to build using -pg" OFF)
option(ENABLE_DEBUGGING "Whether to build using -g" OFF)
option(ENABLE_YICES "Whether to enable Yices" ON)
option(ENABLE_Z3 "Whether to enable Z3 through api" ON)
option(ENABLE_PPL "Whether APRON has been compiled with PPL enabled" OFF)
option(ENABLE_OPT_OCT "Whether APRON has been compiled with the optimized octagons from ETHZ" OFF)

# Path to various libraries
set(EXTERNAL_PATH "${CMAKE_BINARY_DIR}/external/" CACHE PATH "Path to auto-installed external dependencies")

set(GMP_PREFIX "/usr" CACHE PATH "Path to the GMP multiprecision library")
set(MPFR_PREFIX "/usr" CACHE PATH "Path to the MPFR library")
set(LLVM_PATH "${EXTERNAL_PATH}/llvm" CACHE PATH "Path to LLVM installation, used to find llvm-config.")
set(APRON_PREFIX "${EXTERNAL_PATH}/apron" CACHE PATH "Path to the APRON abstract domains library")
set(PPL_PREFIX "${EXTERNAL_PATH}/ppl" CACHE PATH "Path to the PPL Polyhedra library")
set(CAMLIDL_PREFIX "/usr" CACHE PATH "Path to the CAMLIDL library")
set(YICES_PATH "${EXTERNAL_PATH}/yices" CACHE PATH "Path to the YICES SMT solver library")
set(Z3_PATH "${EXTERNAL_PATH}/z3" CACHE PATH "Path to the Z3 SMT solver library")
set(CUDD_PATH "${EXTERNAL_PATH}/cudd" CACHE PATH "Path to the CUDD BDD library")

if(NOT DEFINED ENV{LIB})
  # CMake considers that the system environment variable for libraries
  # is $LIB, but most people just have $LD_LIBRARY_PATH set for this.
  set(ENV{LIB} $ENV{LD_LIBRARY_PATH})
endif()

macro(configure_llvm PROG)
  # In theory, we could use the package discovery like this:
  # SET(CMAKE_MODULE_PATH "${LLVM_DIR}")
  # find_package(LLVM)
  # (see http://llvm.org/docs/CMake.html#embedding )
  # In practice, I didn't manage to get llvm_map_components_to_libraries
  # to work, hence we do it by calling llvm-config ourselves instead.
  find_program(LLVM_CONFIG_PROG llvm-config NAMES llvm-config HINTS "${LLVM_PATH}/bin")
  execute_process(COMMAND "${LLVM_CONFIG_PROG}" --version
    OUTPUT_VARIABLE LLVM_VERSION)
  STRING(REGEX REPLACE "\n" "" LLVM_VERSION "${LLVM_VERSION}")
  
  if("${LLVM_CONFIG_PROG}" STREQUAL LLVM_CONFIG_PROG-NOTFOUND)
    message(WARNING "llvm-config not found. Please set \$LLVM_PATH "
      "so that I can find it in \$LLVM_PATH/bin/")
    set(ERROR_OCCURED ON PARENT_SCOPE)
  elseif("${LLVM_VERSION}" STRLESS "3.3")
    message(WARNING "LLVM version ${LLVM_VERSION} found, while version 3.3 is needed for ${PROG}.")
    unset(LLVM_CONFIG_PROG CACHE)
    set(ERROR_OCCURED ON PARENT_SCOPE)
  endif()

  #execute_process(COMMAND "${LLVM_CONFIG_PROG}" --cxxflags ${ARGN} OUTPUT_VARIABLE LLVM_CXXFLAGS)
  execute_process(COMMAND "${LLVM_CONFIG_PROG}" --cxxflags OUTPUT_VARIABLE LLVM_CXXFLAGS)
  # CMake doesn't like newline in CXXFLAGS, and there's at least a
  # trailing \n in llvm-config's output.
  STRING(REGEX REPLACE "\n" " " LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
  # llvm-config has a -fno-rtti -fno-exception by default, but we use dynamic_cast<>
  STRING(REGEX REPLACE "-fno-rtti" " " LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
  STRING(REGEX REPLACE "-fno-exceptions" " " LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
  # when compiling with -pg, -fomit-frame-pointer is not allowed
  if(${ENABLE_PROFILING})
    STRING(REGEX REPLACE "-fomit-frame-pointer" " " LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
  endif()

  execute_process(COMMAND "${LLVM_CONFIG_PROG}" --ldflags
    OUTPUT_VARIABLE LLVM_LDFLAGS)
  STRING(REGEX REPLACE "\n" " " LLVM_LDFLAGS "${LLVM_LDFLAGS}")

  execute_process(COMMAND "${LLVM_CONFIG_PROG}" --libs ${ARGN}
    OUTPUT_VARIABLE LLVM_LIBS)

  # turn -lfoo -lbar into foo;bar, so that CMake considers it as a list
  STRING(REGEX REPLACE "\n" "" LLVM_LIBS "${LLVM_LIBS}")
  STRING(REGEX REPLACE "^-l" "" LLVM_LIBS "${LLVM_LIBS}")
  STRING(REGEX REPLACE " -l" ";" LLVM_LIBS "${LLVM_LIBS}")

  #set(LLVM_LIBS "-lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization -lclangCodeGen -lclangParse -lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis -lclangARCMigrate -lclangRewrite -lclangEdit -lclangAST -lclangLex -lclangBasic -l${LLVM_LIBS}")
  set(LLVM_LIBS "-lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization -lclangCodeGen -lclangParse -lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis -lclangARCMigrate -lclangEdit -lclangAST -lclangLex -lclangBasic -l${LLVM_LIBS}")
  target_link_libraries(pagai ${LLVM_LIBS})

  # llvm-config --ldflags was hardcoding -lfoo, but the order of
  # libraries is important, and we don't have control on the ordering
  # between LDFLAGS and CMake use of target_link_libraries => remove
  # -lfoo and use target_link_libraries() instead.
  STRING(REGEX REPLACE " *-lpthread *" " " LLVM_LDFLAGS "${LLVM_LDFLAGS}")
  STRING(REGEX REPLACE " *-ldl *" " " LLVM_LDFLAGS "${LLVM_LDFLAGS}")
  target_link_libraries("${PROG}" dl)
  target_link_libraries("${PROG}" pthread)
  target_link_libraries("${PROG}" ncurses)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CXXFLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LLVM_LDFLAGS}")

  if(${ENABLE_DEBUGGING})
    set(CMAKE_CXX_FLAGS " -g ${CMAKE_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS " -g ${CMAKE_EXE_LINKER_FLAGS}")
  endif()
  if(${ENABLE_PROFILING})
    set(CMAKE_CXX_FLAGS " -pg ${CMAKE_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS " -pg ${CMAKE_EXE_LINKER_FLAGS}")
  endif()
endmacro()

macro(add_library_maybe_names PROG LIB NAMES OPTS MSG)
  find_library(TEMP_LIB_${LIB} 
    NAMES ${NAMES}
    HINTS ${ARGN}
    ${OPTS})
  if("${TEMP_LIB_${LIB}}" STREQUAL TEMP_LIB_${LIB}-NOTFOUND)
    message(WARNING "${MSG}")
    set(ERROR_OCCURED ON PARENT_SCOPE)
    # We didn't find the absolute path, but if the user wants to try
    # anyway, we can try with just the library name.
    target_link_libraries("${PROG}" ${LIB})
  else()
    target_link_libraries("${PROG}" ${TEMP_LIB_${LIB}})
  endif()
  unset(TEMP_LIB_${LIB} CACHE)
endmacro()

macro(add_library_maybe PROG LIB MSG)
  add_library_maybe_names("${PROG}" "${LIB}" "${LIB}" "" "${MSG}" ${ARGN})
endmacro()

macro(add_cudd_library PROG LIB)
  add_library_maybe_names("${PROG}" "${LIB}" "${LIB}" "NO_DEFAULT_PATH"
    "Required CUDD library '${LIB}' (i.e lib${LIB}) not found
(I've searched in ${CUDD_PATH} and ${CUDD_PATH}/${LIB}).
Please, install CUDD if needed and set \$CUDD_PATH accordingly."
    "${CUDD_PATH}" "${CUDD_PATH}/${LIB}" "${CUDD_PATH}/lib"
    )
endmacro()

macro(add_yices_library PROG)
  add_library_maybe("${PROG}" yices
    "Required Yices library (i.e libyices) not found
(I've searched in ${YICES_PATH} and ${YICES_PATH}/lib).
Please, install Yices if needed and set \$YICES_PATH accordingly."
    "${YICES_PATH}/lib" "${YICES_PATH}"
    )
endmacro()

macro(add_z3_library PROG)
  # Preferably link against z3-gmp to avoid library version issues
  # with gmp, but fall-back to z3 if z3-gmp is not found (this is the
  # case on 64bits distributions of z3).
  add_library_maybe_names("${PROG}" z3 "z3-gmp;z3" ""
    "Required Z3 library 'z3-gmp' (i.e libz3-gmp) not found
(I've searched in ${Z3_PATH} and ${Z3_PATH}/lib).
Please, install Z3 if needed and set \$Z3_PATH accordingly."
    "${Z3_PATH}/lib" "${Z3_PATH}"
    )
endmacro()

macro(add_apron_library PROG LIB)
  add_library_maybe("${PROG}" "${LIB}"
    "Required APRON library '${LIB}' (i.e lib${LIB}) not found
(I've searched in ${APRON_PREFIX} and ${APRON_PREFIX}/lib).
Please, install APRON if needed (it is included in Debian
and Ubuntu Universe, so if you are using one of these systems,
just install the packages libapron libapron-dev libapron-ocaml-dev)
and set \$APRON_PREFIX accordingly."
    "${APRON_PREFIX}/lib" "${APRON_PREFIX}"
    "${CAMLIDL_PREFIX}/lib/ocaml/apron"
    )
endmacro()

macro(add_ppl_library PROG LIB)
  add_library_maybe("${PROG}" "${LIB}"
    "Required PPL library '${LIB}' (i.e lib${LIB}) not found
(I've searched in ${PPL_PREFIX} and ${PPL_PREFIX}/lib).
Please, install PPL if needed (it is included in Debian
and Ubuntu Universe, so if you are using one of these systems,
just install the package libppl-dev) and set \$PPL_PREFIX accordingly."
    "${PPL_PREFIX}/lib" "${PPL_PREFIX}"
    )
endmacro()

# Compiler choice: clang++ by default
find_program(CLANG_PATH "clang++")
if("${CLANG_PATH}" STREQUAL CLANG_PATH-NOTFOUND)
  message(WARNING "clang++ executable not found, falling back to g++")
  find_program(CLANG_PATH "g++")
endif()

if("${CLANG_PATH}" STREQUAL CLANG_PATH-NOTFOUND)
  message(WARNING "g++ executable not found either. Falling back to CMake's default")
else()
  set(CMAKE_CXX_COMPILER "${CLANG_PATH}")
endif()

find_package(BISON 3.0)
find_package(FLEX 2.5.35)

#set(Boost_USE_STATIC_LIBS OFF) 
#set(Boost_USE_MULTITHREADED OFF)
#set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED COMPONENTS program_options)

option(PRINT_DEBUG "Activate debug traces" OFF)
option(PRINT_DEBUG_SMT "Print the SMT formulae during the analysis" OFF)

add_subdirectory(src)

if(${CHECK_EXTERNALS})
  if(${ERROR_OCCURED})
    message(FATAL_ERROR
      "Some errors occured during configuration (see above). Aborting.  "
      "Many errors about missing dependencies can be solved by typing "
      "simply \"make\" from the external/ directory."
      )
  endif()
endif()
