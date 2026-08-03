if(NOT COMMAND squared_register_module)
    message(FATAL_ERROR
        "Squared SDL2/OpenGL backend requires the squared_register_module hook"
    )
endif()
foreach(required_target SDL2 SDL2_image)
    if(NOT TARGET "${required_target}")
        message(FATAL_ERROR
            "Squared SDL2/OpenGL backend requires target: ${required_target}"
        )
    endif()
endforeach()
if(NOT DEFINED SDL2_INCLUDE_DIR OR
   NOT EXISTS "${SDL2_INCLUDE_DIR}/SDL.h")
    message(FATAL_ERROR
        "Squared SDL2/OpenGL backend requires SDL2_INCLUDE_DIR"
    )
endif()
if(NOT SQUARED_GLES2_LIBRARY)
    message(FATAL_ERROR
        "Squared SDL2/OpenGL backend requires an OpenGL ES 2 link library"
    )
endif()

file(GLOB_RECURSE SQUARED_BACKEND_SDL2_OPENGL_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
add_library(squared_backend_sdl2_opengl STATIC
    ${SQUARED_BACKEND_SDL2_OPENGL_SOURCES}
)
set_target_properties(squared_backend_sdl2_opengl PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(squared_backend_sdl2_opengl PUBLIC cxx_std_20)
target_compile_options(squared_backend_sdl2_opengl PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
target_include_directories(squared_backend_sdl2_opengl PRIVATE
    "${SDL2_INCLUDE_DIR}"
)
target_link_libraries(squared_backend_sdl2_opengl PUBLIC
    squared_graphics2d
    SDL2_image
    SDL2
    "${SQUARED_GLES2_LIBRARY}"
)

squared_register_module(squared_backend_sdl2_opengl)
