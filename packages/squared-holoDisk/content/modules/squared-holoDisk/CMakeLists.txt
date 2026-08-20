set(SQUARED_HOLODISK_MINIZ_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/miniz-3.1.2"
)

add_library(squared_holodisk STATIC
    src/asset_manager.cpp
    src/holodrive.cpp
    "${SQUARED_HOLODISK_MINIZ_ROOT}/miniz.c"
    "${SQUARED_HOLODISK_MINIZ_ROOT}/miniz_tdef.c"
    "${SQUARED_HOLODISK_MINIZ_ROOT}/miniz_tinfl.c"
    "${SQUARED_HOLODISK_MINIZ_ROOT}/miniz_zip.c"
)
set_target_properties(squared_holodisk PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(squared_holodisk PUBLIC cxx_std_20)
target_compile_options(squared_holodisk PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic>
)
target_include_directories(squared_holodisk
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${SQUARED_HOLODISK_MINIZ_ROOT}"
)

squared_register_module(squared_holodisk)
