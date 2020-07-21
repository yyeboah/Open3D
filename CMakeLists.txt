cmake_minimum_required(VERSION 3.12.0)

project(BLASTEST LANGUAGES C CXX)

add_executable(blas_demo main.cpp)

target_include_directories(blas_demo PUBLIC "C:\\Users\\yixing\\Downloads\\OpenBLAS-0.3.10-x64\\include")
# target_link_libraries(blas_demo "C:\\Users\\yixing\\Downloads\\OpenBLAS-0.3.10-x64\\lib\\libopenblas.lib")
target_link_libraries(blas_demo "C:\\Users\\yixing\\repo\\OpenBLAS\\build\\lib\\DEBUG\\openblas.lib")
