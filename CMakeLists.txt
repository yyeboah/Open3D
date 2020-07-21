cmake_minimum_required(VERSION 3.12.0)

project(BLASTEST LANGUAGES C CXX)

include(openblas.cmake)
add_executable(blas_demo main.cpp)
add_dependencies(blas_demo ext_openblas)

target_include_directories(blas_demo PUBLIC ${OPENBLAS_INCLUDE_DIR})
target_link_libraries(blas_demo ${OPENBLAS_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${OPENBLAS_LIBRARIES}${CMAKE_STATIC_LIBRARY_SUFFIX})
