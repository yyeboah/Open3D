# Configure a header file to pass the version settings to the source code
configure_file("${PROJECT_SOURCE_DIR}/cpp/open3d/Open3D.h.in"
               "${PROJECT_SOURCE_DIR}/cpp/open3d/Open3D.h")
configure_file("${PROJECT_SOURCE_DIR}/cpp/open3d/Open3DConfig.h.in"
               "${PROJECT_SOURCE_DIR}/cpp/open3d/Open3DConfig.h")

# Subdirectories
add_subdirectory(core)

# Source group for Visual Studio
add_source_group(core)

# note: adding at least one real source file to any target that references
# reference: https://cmake.org/cmake/help/v3.12/command/add_library.html#object-libraries
add_library(${CMAKE_PROJECT_NAME}
    Open3DConfig.cpp
    $<TARGET_OBJECTS:core>
    ${GUI_OBJECTS}
)
open3d_show_and_abort_on_warning(${PROJECT_NAME})
open3d_set_global_properties(${PROJECT_NAME})
open3d_link_3rdparty_libraries(${PROJECT_NAME})
if(X11_TARGET)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_TARGET})
endif()
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

include(CMakePackageConfigHelpers)

# find_package Open3D
configure_package_config_file(Open3DConfig.cmake.in
                              "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/Open3DConfig.cmake"
                              INSTALL_DESTINATION ${Open3D_INSTALL_CMAKE_DIR}
                              PATH_VARS Open3D_INSTALL_INCLUDE_DIR
                              NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

# find_package Open3D Version
write_basic_package_version_file("${PROJECT_BINARY_DIR}/Open3DConfigVersion.cmake"
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY ExactVersion)

# Installation
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets
        RUNTIME DESTINATION ${Open3D_INSTALL_BIN_DIR}
        LIBRARY DESTINATION ${Open3D_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${Open3D_INSTALL_LIB_DIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${Open3D_INSTALL_INCLUDE_DIR}
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.cuh"
)

