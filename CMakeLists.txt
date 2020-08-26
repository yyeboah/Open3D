cmake_minimum_required(VERSION 3.12.0)
# CMake 3.12+ is required to allow linking with OBJECT libraries
# and to prevent erroneous -gencode option deduplication with CUDA.
# If you're using Ubuntu 18.04 or older, we suggest you install
# a backported CMake from https://apt.kitware.com/

set (CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")

# Open3D build options
option(BUILD_SHARED_LIBS          "Build shared libraries"                   OFF)
option(BUILD_PYTHON_MODULE        "Build the python module"                  ON )
option(WITH_OPENMP                "Use OpenMP multi-threading"               ON )
option(STATIC_WINDOWS_RUNTIME     "Use static (MT/MTd) Windows runtime"      ON )
option(GLIBCXX_USE_CXX11_ABI      "Set -D_GLIBCXX_USE_CXX11_ABI=1"           OFF)
option(USE_SYSTEM_PYBIND11        "Use system pre-installed pybind11"        OFF)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No CMAKE_BUILD_TYPE specified, default to RelWithDebInfo")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

# Set policy for hidden symbol visibility
if(POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()
# Set policy to enable MSVC_RUNTIME_LIBRARY property
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

# Catch a few incompatible build options
if((UNIX AND NOT APPLE) AND BUILD_LIBREALSENSE)
    message(STATUS "Disabling RealSense since it is not fully supported on Linux.")
    set(BUILD_LIBREALSENSE OFF)
endif()
if (APPLE AND ENABLE_HEADLESS_RENDERING)
    message(STATUS "Headless rendering is not supported on Mac OS")
    set(ENABLE_HEADLESS_RENDERING OFF)
endif()
if(ENABLE_HEADLESS_RENDERING AND (USE_SYSTEM_GLEW OR USE_SYSTEM_GLFW))
    message(STATUS "Headless rendering requires customized GLEW and GLFW builds")
    set(USE_SYSTEM_GLEW OFF)
    set(USE_SYSTEM_GLFW OFF)
endif()
if(BUILD_RPC_INTERFACE AND WIN32)
    message(STATUS "The RPC interface is currently unsupported on Windows.")
    set(BUILD_RPC_INTERFACE OFF)
endif()

# Parse Open3D version number
file(STRINGS "cpp/open3d/version.txt" OPEN3D_VERSION_READ)
foreach(ver ${OPEN3D_VERSION_READ})
    if (ver MATCHES "OPEN3D_VERSION_(MAJOR|MINOR|PATCH|TWEAK) +([^ ]+)$")
        set(OPEN3D_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif()
endforeach()
string(CONCAT OPEN3D_VERSION
    "${OPEN3D_VERSION_MAJOR}"
    ".${OPEN3D_VERSION_MINOR}"
    ".${OPEN3D_VERSION_PATCH}"
    ".${OPEN3D_VERSION_TWEAK}"
)

project(Open3D VERSION ${OPEN3D_VERSION} LANGUAGES C CXX)
message(STATUS "Open3D ${PROJECT_VERSION}")

# CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/3rdparty/CMake)

# setup PYTHON_EXECUTABLE if not set
find_package(PythonExecutable REQUIRED) # invokes the module in 3rdparty/CMake

# npm version has to be MAJOR.MINOR.PATCH
string(CONCAT PROJECT_VERSION_THREE_NUMBER "${OPEN3D_VERSION_MAJOR}"
                                           ".${OPEN3D_VERSION_MINOR}"
                                           ".${OPEN3D_VERSION_PATCH}")

# PyPI package name controls specifies the repository name on PyPI. The default
# name is "open3d". In the past, for historical reasons, we've used the
# following names for PyPI, while they are now deprecated:
# - open3d-python
# - py3d
# - open3d-original
# - open3d-official
# - open-3d
if(NOT DEFINED PYPI_PACKAGE_NAME)
    set(PYPI_PACKAGE_NAME "open3d")
endif()

# Set additional info
set(PROJECT_EMAIL   "info@open3d.org")
set(PROJECT_HOME    "http://www.open3d.org")
set(PROJECT_DOCS    "http://www.open3d.org/docs")
set(PROJECT_CODE    "https://github.com/intel-isl/Open3D")
set(PROJECT_ISSUES  "https://github.com/intel-isl/Open3D/issues")

# Set installation paths
if(UNIX OR CYGWIN)
    include(GNUInstallDirs)
    set(Open3D_INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}")
    set(Open3D_INSTALL_BIN_DIR "${CMAKE_INSTALL_BINDIR}")
    set(Open3D_INSTALL_LIB_DIR "${CMAKE_INSTALL_LIBDIR}")
    set(Open3D_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
else()
    set(Open3D_INSTALL_INCLUDE_DIR include)
    set(Open3D_INSTALL_BIN_DIR bin)
    set(Open3D_INSTALL_LIB_DIR lib)
    set(Open3D_INSTALL_CMAKE_DIR CMake)
endif()

# Put build results in some predictable places
# The $<CONFIG> generator expression makes sure that XCode or Visual Studio do not
# append additional path components, as we need to know *exactly* where the build results
# end up.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/$<CONFIG>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# OS specific settings
if(WIN32)
    # Windows defaults to hidden symbol visibility, override that
    # TODO: It would be better to explictly export symbols.
    #       Then, we could use -fvisibility=hidden for Linux as well
    SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    if(MSVC)
        # Make sure we don't hit the 65535 object member limit with MSVC
        #
        # /bigobj allows object files with more than 65535 members
        # /Ob2 enables function inlining, because MSVC is particularly
        # verbose with inline members
        #
        # See: https://github.com/tensorflow/tensorflow/pull/10962
        add_compile_options(/bigobj /Ob2)
    endif()
    if (STATIC_WINDOWS_RUNTIME)
        # This needs cmake_policy(SET CMP0091 NEW)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

# Folder view for project files
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Convenience function to link against all third-party libraries
# We need this because we create a lot of object libraries to assemble
# the main Open3D library
function(open3d_link_3rdparty_libraries target)
    target_link_libraries(${target} PRIVATE ${Open3D_3RDPARTY_PRIVATE_TARGETS})
    target_link_libraries(${target} PUBLIC ${Open3D_3RDPARTY_PUBLIC_TARGETS})
    foreach(dep IN LISTS Open3D_3RDPARTY_HEADER_TARGETS)
        if(TARGET ${dep})
            get_property(inc TARGET ${dep} PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
            if(inc)
                set_property(TARGET ${target} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${inc})
            endif()
            get_property(inc TARGET ${dep} PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
            if(inc)
                set_property(TARGET ${target} APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${inc})
            endif()
            get_property(def TARGET ${dep} PROPERTY INTERFACE_COMPILE_DEFINITIONS)
            if(def)
                set_property(TARGET ${target} APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS ${def})
            endif()
        endif()
    endforeach()
endfunction()

# Check if the compiler defines the _GLIBCXX_USE_CXX11_ABI macro
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("#include <cxxabi.h>
int main() { return _GLIBCXX_USE_CXX11_ABI; }" HAS_GLIBCXX_USE_CXX11_ABI)

# Convenience function to set important target properties
function(open3d_set_global_properties target)
    # Libraries need to be compiled with position independent code
    get_target_property(target_type ${target} TYPE)
    if (NOT target_type STREQUAL "EXECUTABLE")
        set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()
    # Tell CMake we want a compiler that supports C++14 features
    target_compile_features(${target} PUBLIC
        cxx_variadic_templates
        cxx_constexpr
        cxx_override
        cxx_static_assert
        cxx_trailing_return_types
        cxx_return_type_deduction
    )
    target_include_directories(${target} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/cpp>
        $<INSTALL_INTERFACE:${Open3D_INSTALL_INCLUDE_DIR}>
    )
    if(BUILD_RPC_INTERFACE)
        target_compile_definitions(${target} PRIVATE BUILD_RPC_INTERFACE)
    endif()
    if(GLIBCXX_USE_CXX11_ABI)
        target_compile_definitions(${target} PUBLIC _GLIBCXX_USE_CXX11_ABI=1)
    else()
        target_compile_definitions(${target} PUBLIC _GLIBCXX_USE_CXX11_ABI=0)
    endif()
    if(ENABLE_HEADLESS_RENDERING)
        target_compile_definitions(${target} PRIVATE HEADLESS_RENDERING)
    endif()
    if(WIN32)
        target_compile_definitions(${target} PRIVATE
            WINDOWS
            _CRT_SECURE_NO_DEPRECATE
            _CRT_NONSTDC_NO_DEPRECATE
            _SCL_SECURE_NO_WARNINGS
        )
        if(MSVC)
            target_compile_definitions(${target} PRIVATE NOMINMAX _USE_MATH_DEFINES _ENABLE_EXTENDED_ALIGNED_STORAGE)
            target_compile_options(${target} PRIVATE /EHsc /wd4522 /wd4190 /wd4819 /Wv:18 /WX)
            # Multi-thread compile, two ways to enable
            # Option 1, at build time: cmake --build . --parallel %NUMBER_OF_PROCESSORS%
            # https://stackoverflow.com/questions/36633074/set-the-number-of-threads-in-a-cmake-build
            # Option 2, at configure time: add /MP flag, no need to use Option 1
            # https://docs.microsoft.com/en-us/cpp/build/reference/mp-build-with-multiple-processes?view=vs-2019
            target_compile_options(${target} PRIVATE /MP)
            if(COMMAND target_link_options) # target_link_options add in 3.13
                # The examples' .pdb files use up a lot of space and cause us to run
                # out of space on Github Actions. Compressing gives us another couple of GB.
                target_link_options(${target} PRIVATE /pdbcompress)
            endif()
        endif()
    elseif(APPLE)
        target_compile_definitions(${target} PRIVATE UNIX APPLE)
        target_compile_options(${target} PRIVATE "-Wno-deprecated-declarations")
    elseif(UNIX)
        target_compile_definitions(${target} PRIVATE UNIX)
        target_compile_options(${target} PRIVATE "-Wno-deprecated-declarations" "$<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-result>")
    endif()
endfunction()

macro(add_source_group module_name)
    file(GLOB MODULE_HEADER_FILES "${module_name}/*.h")
    source_group("Header Files\\${module_name}" FILES ${MODULE_HEADER_FILES})
    file(GLOB MODULE_SOURCE_FILES "${module_name}/*.cpp")
    source_group("Source Files\\${module_name}" FILES ${MODULE_SOURCE_FILES})
endmacro()

# Enumerate all third-party libraries which we need later
# This creates the necessary targets and sets the
# Open3D_3RDPARTY_*_TARGETS variables we use in open3d_link_3rdparty_libraries
include(3rdparty/find_dependencies.cmake)

# Open3D library
add_subdirectory(cpp)

# Install CMake configuration files
install(EXPORT ${PROJECT_NAME}Targets NAMESPACE ${PROJECT_NAME}:: DESTINATION ${Open3D_INSTALL_CMAKE_DIR})
export(EXPORT ${PROJECT_NAME}Targets NAMESPACE ${PROJECT_NAME}::)
