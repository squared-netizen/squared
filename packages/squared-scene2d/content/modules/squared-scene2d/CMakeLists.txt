if(NOT COMMAND squared_register_module)
    message(FATAL_ERROR
        "Squared Scene2D requires the squared_register_module hook"
    )
endif()
if(NOT TARGET squared_graphics2d)
    message(FATAL_ERROR
        "Squared Scene2D requires target: squared_graphics2d"
    )
endif()

file(GLOB_RECURSE SQUARED_SCENE2D_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
if(NOT SQUARED_SCENE2D_SOURCES)
    message(FATAL_ERROR "Squared Scene2D has no implementation sources")
endif()

add_library(squared_scene2d STATIC ${SQUARED_SCENE2D_SOURCES})
set_target_properties(squared_scene2d PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(squared_scene2d PUBLIC cxx_std_20)
target_compile_options(squared_scene2d PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
target_include_directories(squared_scene2d PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(squared_scene2d PUBLIC
    squared_graphics2d
)

squared_register_module(squared_scene2d)
