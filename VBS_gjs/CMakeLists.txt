cmake_minimum_required (VERSION 2.8)
project (libVBS)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99 -D_XOPEN_SOURCE=500 -Wall")

include_directories(${CMAKE_CURRENT_DIRECTORY}/include)

add_library(vbs SHARED src/VirtualBlockStorage.c src/bitmap.c)
# add_library(vbs STATIC src/VirtualBlockStorage.c src/bitmap.c)

install(TARGETS vbs DESTINATION lib)
install(FILES include/VirtualBlockStorage.h include/bitmap.h DESTINATION include)
	
target_link_libraries(vbs)

add_executable (test_vbs src/VBS_test.c)
add_executable (test_vbs_storage src/VBS_test_storage.c)

target_link_libraries (test_vbs vbs)
target_link_libraries (test_vbs_storage vbs)


# Automatic API Documentation Generate
#find_package(Doxygen)
#if(DOXYGEN_FOUND)
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile @ONLY)
#add_custom_target(doc
#${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile
#WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#COMMENT "Generating API documentation with Doxygen" VERBATIM
#)
#endif(DOXYGEN_FOUND)

