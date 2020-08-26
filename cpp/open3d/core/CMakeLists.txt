set(CORE_SRC
    Dtype.cpp
)

set(ALL_CORE_SRC
    ${CORE_SRC}
)

# Create object library
add_library(core OBJECT ${ALL_CORE_SRC})
open3d_set_global_properties(core)
open3d_link_3rdparty_libraries(core)
open3d_show_and_abort_on_warning(core)
