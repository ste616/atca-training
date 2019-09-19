cmake_minimum_required (VERSION 3.5)
project (ATCATraining LANGUAGES C Fortran)

add_library(atrpfits STATIC src/rpfits/reader.c)
target_include_directories(atrpfits PUBLIC src/rpfits)

add_executable(summariser src/apps/summariser/summariser.c)
set_target_properties(summariser PROPERTIES LINKER_LANGUAGE Fortran)
target_link_libraries(summariser PUBLIC atrpfits rpfits)


