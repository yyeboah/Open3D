include(FetchContent)

if(WIN32)
    message(FATAL_ERROR "TODO")
elseif(APPLE)
    set(MKL_INCLUDE_DIR "/Users/ylao/repo/mkl-releaser/build/mkl_include/src/ext_mkl_include/include/")
    set(MKL_LIB_DIR "/Users/ylao/repo/mkl-releaser/build/mkl/src/ext_mkl/lib")
    set(MKL_LIBRARIES mkl_intel_ilp64 mkl_tbb_thread mkl_core tbb_static)
else()
    FetchContent_Declare(
        ext_mkl
        URL "https://storage.googleapis.com/isl-datasets/open3d-dev/mkl-release-2020.1.zip"
    )
    FetchContent_GetProperties(ext_mkl)
    if(NOT ext_mkl_POPULATED)
        message(STATUS "Downloading MKL, this may take a while...")
        FetchContent_Populate(ext_mkl)
        message(STATUS "ext_mkl_SOURCE_DIR: ${ext_mkl_SOURCE_DIR}")
        set(MKL_INCLUDE_DIR "${ext_mkl_SOURCE_DIR}/linux-64/include/")
        set(MKL_LIB_DIR "${ext_mkl_SOURCE_DIR}/linux-64/lib")
        set(MKL_LIBRARIES mkl_merged)
    endif()
endif()
