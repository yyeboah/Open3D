include(ExternalProject)

set(MKL_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/mkl_install)

# We need to put TBB right next to MKL in the link flags. So instead of creating
# a new tbb.cmake, it is also put here.
ExternalProject_Add(
    ext_tbb
    PREFIX tbb
    GIT_REPOSITORY https://github.com/wjakob/tbb.git
    GIT_TAG 806df70ee69fc7b332fcf90a48651f6dbf0663ba # July 2020
    UPDATE_COMMAND ""
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${MKL_INSTALL_PREFIX}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
)

if(WIN32)
    set(MKL_INCLUDE_URL "https://anaconda.org/intel/mkl-include/2020.1/download/win-64/mkl-include-2020.1-intel_216.tar.bz2")
    set(MKL_URL         "https://anaconda.org/intel/mkl-static/2020.1/download/win-64/mkl-static-2020.1-intel_216.tar.bz2")
elseif(APPLE)
    set(MKL_INCLUDE_URL   "https://anaconda.org/intel/mkl-include/2020.1/download/osx-64/mkl-include-2020.1-intel_216.tar.bz2")
    set(MKL_URL           "https://anaconda.org/intel/mkl-static/2020.1/download/osx-64/mkl-static-2020.1-intel_216.tar.bz2")
else()
    set(MKL_INCLUDE_URL   "https://anaconda.org/intel/mkl-include/2020.1/download/linux-64/mkl-include-2020.1-intel_217.tar.bz2")
    set(MKL_URL           "https://anaconda.org/intel/mkl-static/2020.1/download/linux-64/mkl-static-2020.1-intel_217.tar.bz2")
endif()

if(WIN32)
    message(FATAL_ERROR "TODO")
elseif(APPLE)
    ExternalProject_Add(
        ext_mkl_include
        PREFIX mkl_include
        URL ${MKL_INCLUDE_URL}
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include ${MKL_INSTALL_PREFIX}/include
    )
    ExternalProject_Add(
        ext_mkl
        PREFIX mkl
        URL ${MKL_URL}
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/lib ${MKL_INSTALL_PREFIX}/lib
    )
    set(MKL_INCLUDE_DIR "${MKL_INSTALL_PREFIX}/include/")
    set(MKL_LIB_DIR "${MKL_INSTALL_PREFIX}/lib")
    set(MKL_LIBRARIES mkl_intel_ilp64 mkl_tbb_thread mkl_core tbb_static)
else()
    # Resolving static library circular dependencies.
    # - Approach 1: Add `-Wl,--start-group` `-Wl,--end-group` around, but this
    #               is not friendly with CMake.
    # - Approach 2: Set LINK_INTERFACE_MULTIPLICITY to 3. However this does not
    #               work directly with interface library, and requires big
    #               changes to the build system. See discussions in:
    #               - https://gitlab.kitware.com/cmake/cmake/-/issues/17964
    #               - https://gitlab.kitware.com/cmake/cmake/-/issues/18415
    #               - https://stackoverflow.com/q/50166553/1255535
    # - Approach 3: Merge libmkl_intel_ilp64.a, libmkl_tbb_thread.a and
    #               libmkl_core.a into libmkl_merged.a. This is the most simple
    #               approach to integrate with the build system. However, extra
    #               time is required to merge the libraries and the merged
    #               library size can be large. We choose to use approach 3.
    ExternalProject_Add(
        ext_mkl_include
        PREFIX mkl_include
        URL ${MKL_INCLUDE_URL}
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include ${MKL_INSTALL_PREFIX}/include
    )
    ExternalProject_Add(
        ext_mkl
        PREFIX mkl
        URL ${MKL_URL}
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE ON
        BUILD_COMMAND echo "Extracting static libs..."
        COMMAND ar x lib/libmkl_intel_ilp64.a
        COMMAND ar x lib/libmkl_tbb_thread.a
        COMMAND ar x lib/libmkl_core.a
        COMMAND echo "Merging static libs..."
        COMMAND bash -c "ar -qc lib/libmkl_merged.a *.o"
        COMMAND echo "Cleaning up *.o files..."
        COMMAND bash -c "rm *.o"
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy lib/libmkl_merged.a ${MKL_INSTALL_PREFIX}/lib/libmkl_merged.a
    )
    set(MKL_INCLUDE_DIR "${MKL_INSTALL_PREFIX}/include/")
    set(MKL_LIB_DIR "${MKL_INSTALL_PREFIX}/lib")
    set(MKL_LIBRARIES mkl_merged tbb_static)
endif()

add_dependencies(ext_mkl ext_mkl_include)
add_dependencies(ext_mkl_include ext_tbb)
