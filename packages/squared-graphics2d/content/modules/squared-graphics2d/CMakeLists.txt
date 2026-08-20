if(NOT COMMAND squared_register_module)
    message(FATAL_ERROR
        "Squared Graphics2D requires the squared_register_module hook"
    )
endif()
add_library(squared_graphics2d STATIC
    src/bitmap_font.cpp
    src/orthographic_camera.cpp
)
set_target_properties(squared_graphics2d PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(squared_graphics2d PUBLIC cxx_std_20)
target_compile_options(squared_graphics2d PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
target_include_directories(squared_graphics2d PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(squared_graphics2d PUBLIC
    squared_graphics
    squared_math
)

squared_register_module(squared_graphics2d)
