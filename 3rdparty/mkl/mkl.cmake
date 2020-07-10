include(ExternalProject)

set(MKL_URL_LINUX         "https://anaconda.org/anaconda/mkl/2020.1/download/linux-64/mkl-2020.1-217.tar.bz2")
set(MKL_INCLUDE_URL_LINUX "https://anaconda.org/anaconda/mkl-include/2020.1/download/linux-64/mkl-include-2020.1-217.tar.bz2")

ExternalProject_Add(ext_mkl
    PREFIX mkl
    URL ${MKL_URL_LINUX}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL TRUE
)

ExternalProject_Add(ext_mkl_include
    PREFIX mkl
    URL ${MKL_INCLUDE_URL_LINUX}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL TRUE
)

add_custom_target(ext_mkl_with_include DEPENDS ext_mkl ext_mkl_include)
