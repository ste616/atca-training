cmake_minimum_required (VERSION 3.5)
project (ATCATraining LANGUAGES C Fortran)
set(CMAKE_BUILD_TYPE Debug)

add_library(atrpfits STATIC src/rpfits/reader.c)
target_include_directories(atrpfits PUBLIC src/rpfits src/include)

add_executable(summariser src/apps/summariser/summariser.c)
set_target_properties(summariser PROPERTIES LINKER_LANGUAGE Fortran)
target_link_libraries(summariser PUBLIC atrpfits rpfits)

add_executable(visplot src/apps/visplot/visplot.c)
set_target_properties(visplot PROPERTIES LINKER_LANGUAGE Fortran)
target_link_libraries(visplot PUBLIC atrpfits rpfits cpgplot pgplot png z m X11)

