if(NOT COMMAND squared_register_module)
    message(FATAL_ERROR
        "Squared Graphics requires the squared_register_module hook"
    )
endif()
if(NOT TARGET SDL2)
    message(FATAL_ERROR
        "Squared Graphics requires an SDL2 target from the selected template"
    )
endif()
if(NOT DEFINED SDL2_INCLUDE_DIR OR
   NOT EXISTS "${SDL2_INCLUDE_DIR}/SDL.h")
    message(FATAL_ERROR
        "Squared Graphics requires SDL2_INCLUDE_DIR from the selected template"
    )
endif()
if(NOT SQUARED_GLES2_LIBRARY)
    message(FATAL_ERROR
        "Squared Graphics requires an OpenGL ES 2 link library"
    )
endif()

file(GLOB_RECURSE SQUARED_GRAPHICS_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
if(NOT SQUARED_GRAPHICS_SOURCES)
    message(FATAL_ERROR "Squared Graphics has no implementation sources")
endif()

add_library(squared_graphics STATIC ${SQUARED_GRAPHICS_SOURCES})
set_target_properties(squared_graphics PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(squared_graphics PUBLIC cxx_std_20)
target_compile_options(squared_graphics PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
target_include_directories(squared_graphics PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${SDL2_INCLUDE_DIR}"
)
target_link_libraries(squared_graphics PUBLIC
    SDL2
    "${SQUARED_GLES2_LIBRARY}"
)

squared_register_module(squared_graphics)
