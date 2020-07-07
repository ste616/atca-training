cmake_minimum_required (VERSION 3.5)
project (ATCATraining LANGUAGES C Fortran)
set(CMAKE_BUILD_TYPE Debug)

add_library(applib STATIC src/library/common.c src/library/packing.c extern/cmp/cmp.c src/library/atnetworking.c extern/cmp_mem_access/cmp_mem_access.c)
target_include_directories(applib PUBLIC src/library src/rpfits src/include extern/cmp extern/cmp_mem_access)
target_compile_options(applib PRIVATE -Werror -Wall -Wextra)

add_library(atrpfits STATIC src/rpfits/reader.c src/rpfits/compute.c
  src/rpfits/atrpfits.c)
target_include_directories(atrpfits PUBLIC src/rpfits src/include)
target_compile_options(atrpfits PRIVATE -Werror -Wall -Wextra)

add_executable(summariser src/apps/summariser/summariser.c)
set_target_properties(summariser PROPERTIES LINKER_LANGUAGE Fortran)
target_link_libraries(summariser PUBLIC atrpfits rpfits)
target_compile_options(summariser PRIVATE -Werror -Wall -Wextra -Wno-missing-field-initializers)

#add_executable(visplot src/apps/visplot/visplot.c)
#set_target_properties(visplot PROPERTIES LINKER_LANGUAGE Fortran)
#target_link_libraries(visplot PUBLIC atrpfits rpfits applib cpgplot pgplot png z m X11 readline)
#target_compile_options(visplot PRIVATE -Werror -Wall -Wextra -Wno-missing-field-initializers)

add_executable(rpfitsfile_server src/apps/rpfitsfile_server/rpfitsfile_server.c)
set_target_properties(rpfitsfile_server PROPERTIES LINKER_LANGUAGE Fortran)
target_link_libraries(rpfitsfile_server PUBLIC applib atrpfits rpfits m)
target_include_directories(rpfitsfile_server PUBLIC src/rpfits src/include extern/cmp extern/cmp_mem_access src/library)
target_compile_options(rpfitsfile_server PRIVATE -Werror -Wall -Wextra -Wno-missing-field-initializers)

add_executable(nspd src/apps/nspd/nspd.c)
set_target_properties(nspd PROPERTIES LINKER_LANGUAGE Fortran)
target_link_libraries(nspd PUBLIC applib atrpfits rpfits m cpgplot pgplot png z X11 readline)
target_include_directories(nspd PUBLIC src/rpfits src/include extern/cmp extern/cmp_mem_access src/library)
target_compile_options(nspd PRIVATE -Werror -Wall -Wextra -Wno-missing-field-initializers)

add_executable(nvis src/apps/nvis/nvis.c)
set_target_properties(nvis PROPERTIES LINKER_LANGUAGE Fortran)
target_link_libraries(nvis PUBLIC applib atrpfits rpfits m cpgplot pgplot png z X11 readline)
target_include_directories(nvis PUBLIC src/rpfits src/include extern/cmp extern/cmp_mem_access src/library)
target_compile_options(nvis PRIVATE -Werror -Wall -Wextra -Wno-missing-field-initializers)
