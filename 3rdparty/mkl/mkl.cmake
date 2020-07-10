include(ExternalProject)

set(MKL_URL_LINUX "https://anaconda.org/anaconda/mkl/2020.1/download/linux-64/mkl-2020.1-217.tar.bz2")

ExternalProject_Add(ext_mkl
    URL ${MKL_URL_LINUX}
    CONFIGURE_COMMAND ""
    EXCLUDE_FROM_ALL TRUE
)
