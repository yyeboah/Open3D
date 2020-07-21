include(ExternalProject)

ExternalProject_Add(
    ext_openblas
    PREFIX openblas
    GIT_REPOSITORY https://github.com/xianyi/OpenBLAS.git
    GIT_TAG v0.3.10
    INSTALL_COMMAND "" # Install command not compatible with powershell
    CMAKE_ARGS -DMSVC_STATIC_CRT=${STATIC_WINDOWS_RUNTIME}
)

ExternalProject_Get_property(ext_openblas SOURCE_DIR)
set(OPENBLAS_INCLUDE_DIR "${SOURCE_DIR}/") # The "/"" is critical, see import_3rdparty_library.
set(OPENBLAS_LIB_DIR "${SOURCE_DIR}")
set(OPENBLAS_LIBRARIES openblas)
message(STATUS "OPENBLAS_INCLUDE_DIR: ${OPENBLAS_INCLUDE_DIR}")
message(STATUS "OPENBLAS_LIB_DIR ${OPENBLAS_LIB_DIR}")
message(STATUS "OPENBLAS_LIBRARIES: ${OPENBLAS_LIBRARIES}")
