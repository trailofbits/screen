
include_directories(../include)

add_library(01_annotated_function SHARED 01_annotated_function.c)
add_executable(02_annotated_region_simple 02_annotated_region_simple.c)
add_library(03_interprocedural_basic  03_interprocedural_basic.c)
