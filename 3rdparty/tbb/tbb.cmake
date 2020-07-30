include(ExternalProject)

ExternalProject_Add(
    ext_tbb
    PREFIX tbb
    GIT_REPOSITORY https://github.com/wjakob/tbb.git
    GIT_TAG 806df70ee69fc7b332fcf90a48651f6dbf0663ba # July 2020
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)

ExternalProject_Get_property(ext_tbb INSTALL_DIR)
set(TBB_INCLUDE_DIR "${INSTALL_DIR}/include/")
set(TBB_LIB_DIR "${INSTALL_DIR}/lib")
set(TBB_LIBRARIES tbb_static tbbmalloc_static)
