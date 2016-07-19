
set (CMAKE_C_COMPILER "${LLVM_ROOT}/bin/clang")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wshadow -Wextra")

add_bitcode(test1 test1.c)

#add_custom_command (OUTPUT test1.bc
  #COMMAND ${CMAKE_C_COMPILER} -emit-llvm ${CMAKE_C_FLAGS} -o test1.bc test1.c
#)
#
#add_custom_target (tests DEPENDS test1.bc)
