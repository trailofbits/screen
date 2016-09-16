SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

file(GLOB TOP_SRC *.cc)

if(${STATIC_LINK})
  # probably not the cleanest way to do (-static won't exist for all
  # compilers). Using find_library and putting .a at the front of
  # CMAKE_FIND_LIBRARY_SUFFIXES should also do the trick if ever
  # needed.
  set(CMAKE_EXE_LINKER_FLAGS " ${CMAKE_EXE_LINKER_FLAGS} -static -fno-rtti -fPIC")
endif()

if(${PRINT_DEBUG})
  add_definitions(-DPRINT_DEBUG)
endif()
if(${ENABLE_PPL})
  add_definitions(-DPPL_ENABLED)
endif()
if(${ENABLE_OPT_OCT})
  add_definitions(-DOPT_OCT_ENABLED)
endif()

if(${PRINT_DEBUG_SMT})
  add_definitions(-DPRINT_DEBUG_SMT)
endif()

add_definitions(-DLLVM_INSTALL_PATH="${LLVM_PATH}")

find_package(Boost COMPONENTS program_options REQUIRED)

# TODO: we could do configure-time checks with find_file(FOO NAMES foo.h PATH ...)
include_directories(
  ${YICES_PATH}/include 
  ${Z3_PATH}/include
  ${GMP_PREFIX}/include
  ${MPFR_PREFIX}/include
  ${APRON_PREFIX}/include 
  ${CUDD_PATH}/include
  ${Boost_INCLUDE_DIR}
)

# TODO: If we don't add /usr/lib at the front, then the pagai executable
# tries to link against libgmp.so.3 which doesn't exist. It seems to
# be linked to the version of libutils chosen by the linker.
# set(CMAKE_EXE_LINKER_FLAGS "-L/usr/lib -lboost_system ${CMAKE_EXE_LINKER_FLAGS}")
link_directories(
  ${GMP_PREFIX}/lib
  ${MPFR_PREFIX}/lib 
  #${PPL_PREFIX}/lib
)

find_package(BISON)
find_package(FLEX)
BISON_TARGET(SMTlib2parser SMTlib2parser.yy
	${CMAKE_CURRENT_BINARY_DIR}/SMTlib2parser.cc
        COMPILE_FLAGS --report=state)
FLEX_TARGET(SMTlib2lexer SMTlib2lexer.lex
	${CMAKE_CURRENT_BINARY_DIR}/SMTlib2lexer.cc
        COMPILE_FLAGS --always-interactive)
ADD_FLEX_BISON_DEPENDENCY(SMTlib2lexer SMTlib2parser)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


add_executable(pagai ${TOP_SRC} 
	${BISON_SMTlib2parser_OUTPUTS}
	${FLEX_SMTlib2lexer_OUTPUTS})
TARGET_LINK_LIBRARIES(pagai ${Boost_LIBRARIES})

configure_llvm(pagai bitreader selectiondag all)

# Apron and dependencies
add_apron_library(pagai polkaMPQ)
add_apron_library(pagai octD)
add_apron_library(pagai boxMPFR)
if(${ENABLE_PPL})
	add_apron_library(pagai ap_pkgrid)
	add_apron_library(pagai ap_ppl)
	add_ppl_library(pagai ppl)
endif()
if(${ENABLE_OPT_OCT})
        add_apron_library(pagai optoct)
endif()

add_apron_library(pagai apron)

# SMT solvers
if(${ENABLE_YICES})
	add_yices_library(pagai)
	add_definitions(-DHAS_YICES)
endif()

if(${ENABLE_Z3})
	add_z3_library(pagai)
	add_definitions(-DHAS_Z3)
endif()


target_link_libraries(pagai mpfr gmpxx gmp ${Boost_LIBRARIES} )

if(${ENABLE_Z3})
	if (${STATIC_LINK})
		target_link_libraries(pagai rt gomp pthread)
	endif()
endif()

# CUDD (BDD library)
# TODO: There is a confusion between /lib/libutil and CUDD's libutil,
# hence we need to add CUDD one by its full name (If we don't do this,
# it doesn't link on Julien's machine). While we're there, make all
# CUDD library paths absolute and unambiguous.
add_cudd_library(pagai obj)
add_cudd_library(pagai cudd)
add_cudd_library(pagai util)
add_cudd_library(pagai mtr)
add_cudd_library(pagai st)
add_cudd_library(pagai epd)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/DOXYFILE.in ${CMAKE_CURRENT_BINARY_DIR}/DOXYFILE @ONLY)
ADD_CUSTOM_TARGET(doc
  COMMAND doxygen ${CMAKE_CURRENT_BINARY_DIR}/DOXYFILE
  )


# for LLVM (shouldn't it be automatic?)
find_package( ZLIB REQUIRED )
if ( ZLIB_FOUND )
    include_directories( ${ZLIB_INCLUDE_DIRS} )
    target_link_libraries(pagai ${ZLIB_LIBRARIES} )
endif( ZLIB_FOUND )
