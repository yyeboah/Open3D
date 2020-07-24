include(FetchContent)

set(MKL_URL_LINUX           "https://anaconda.org/anaconda/mkl/2020.1/download/linux-64/mkl-2020.1-217.tar.bz2")
set(MKL_URL_MACOS           "https://anaconda.org/anaconda/mkl/2020.1/download/osx-64/mkl-2020.1-216.tar.bz2")
set(MKL_URL_WINDOWS         "https://anaconda.org/anaconda/mkl/2020.1/download/win-64/mkl-2020.1-216.tar.bz2")
set(MKL_INCLUDE_URL_LINUX   "https://anaconda.org/anaconda/mkl-include/2020.1/download/linux-64/mkl-include-2020.1-217.tar.bz2")
set(MKL_INCLUDE_URL_MACOS   "https://anaconda.org/anaconda/mkl-include/2020.1/download/osx-64/mkl-include-2020.1-216.tar.bz2")
set(MKL_INCLUDE_URL_WINDOWS "https://anaconda.org/anaconda/mkl-include/2020.1/download/win-64/mkl-include-2020.1-216.tar.bz2")

if(WIN32)
    set(MKL_URL ${MKL_URL_WINDOWS})
    set(MKL_INCLUDE_URL ${MKL_INCLUDE_URL_WINDOWS})
elseif(APPLE)
    set(MKL_URL ${MKL_URL_MACOS})
    set(MKL_INCLUDE_URL ${MKL_INCLUDE_URL_MACOS})
elseif(UNIX)
    set(MKL_URL ${MKL_URL_LINUX})
    set(MKL_INCLUDE_URL ${MKL_INCLUDE_URL_LINUX})
else()
    message("FATAL_ERROR: unknown platform.")
endif()
message(STATUS "MKL_URL: ${MKL_URL}")
message(STATUS "MKL_INCLUDE_URL: ${MKL_INCLUDE_URL}")

FetchContent_Declare(
    ext_mkl
    URL ${MKL_URL}
)

FetchContent_Declare(
    ext_mkl_include
    URL ${MKL_INCLUDE_URL}
)

FetchContent_GetProperties(ext_mkl)
if(NOT ext_mkl_POPULATED)
    message(STATUS "Downloading MKL, this may take a while...")
    FetchContent_Populate(ext_mkl)
    message(STATUS "ext_mkl_SOURCE_DIR: ${ext_mkl_SOURCE_DIR}")
endif()

FetchContent_GetProperties(ext_mkl_include)
if(NOT ext_mkl_include_POPULATED)
    message(STATUS "Downloading MKL-INCLUDE, this may take a while...")
    FetchContent_Populate(ext_mkl_include)
    message(STATUS "ext_mkl_include_SOURCE_DIR: ${ext_mkl_include_SOURCE_DIR}")
endif()

# Call FindMKL, after that, we have:
# - MKL_INCLUDE_DIR
# - MKL_LIBRARIES: full paths to
#   - MKL libraries, e.g.
#     - libmkl_intel_lp64.so
#     - libmkl_intel_thread.so
#     - libmkl_core.so
#   - System library used by MKL, e.g.
#     - libpthread.so
#     - libm.so
#     - libdl.so
set(CMAKE_PREFIX_PATH
    ${CMAKE_PREFIX_PATH}
    ${ext_mkl_SOURCE_DIR}
    ${ext_mkl_include_SOURCE_DIR}
)
include(${CMAKE_MODULE_PATH}/FindMKL.cmake)
