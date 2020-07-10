include(ExternalProject)

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

ExternalProject_Add(ext_mkl
    PREFIX mkl
    URL ${MKL_URL}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL TRUE
)

ExternalProject_Add(ext_mkl_include
    PREFIX mkl
    URL ${MKL_INCLUDE_URL}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL TRUE
)

add_custom_target(ext_mkl_with_include DEPENDS ext_mkl ext_mkl_include)
