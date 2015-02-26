cmake_minimum_required (VERSION 2.8)
project (libSoneFiveFS)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99 -D_XOPEN_SOURCE=500 -Wall")

include_directories(${CMAKE_CURRENT_DIRECTORY}/include)

#add_library(SoneFiveFS SHARED src/S15Filesystem.c)

#install(TARGETS SoneFiveFS DESTINATION lib)
#install(FILES include/S15Filesystem.h DESTINATION include)

#target_link_libraries(SoneFiveFS vbs)

add_executable (test_S15FS src/test_S15FS.c)
#target_link_libraries (test_S15FS SoneFiveFS)


# Automatic API Documentation Generate
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile @ONLY)
add_custom_target(doc
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)



