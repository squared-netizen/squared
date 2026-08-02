if(NOT COMMAND squared_register_module)
    message(FATAL_ERROR
        "Squared Graphics2D requires the squared_register_module hook"
    )
endif()
foreach(required_target SDL2 SDL2_image)
    if(NOT TARGET "${required_target}")
        message(FATAL_ERROR
            "Squared Graphics2D requires target: ${required_target}"
        )
    endif()
endforeach()
if(NOT DEFINED SDL2_INCLUDE_DIR OR
   NOT EXISTS "${SDL2_INCLUDE_DIR}/SDL.h")
    message(FATAL_ERROR
        "Squared Graphics2D requires SDL2_INCLUDE_DIR from the selected template"
    )
endif()
if(NOT SQUARED_GLES2_LIBRARY)
    message(FATAL_ERROR
        "Squared Graphics2D requires an OpenGL ES 2 link library"
    )
endif()

file(GLOB_RECURSE SQUARED_GRAPHICS2D_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
if(NOT SQUARED_GRAPHICS2D_SOURCES)
    message(FATAL_ERROR "Squared Graphics2D has no implementation sources")
endif()

add_library(squared_graphics2d STATIC ${SQUARED_GRAPHICS2D_SOURCES})
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
    "${SDL2_INCLUDE_DIR}"
)
target_link_libraries(squared_graphics2d PUBLIC
    squared_graphics
    squared_math
    SDL2_image
    SDL2
    "${SQUARED_GLES2_LIBRARY}"
)

squared_register_module(squared_graphics2d)
