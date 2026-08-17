if(NOT COMMAND squared_register_module)
    message(FATAL_ERROR
        "Squared GUI requires the squared_register_module hook"
    )
endif()
file(GLOB_RECURSE SQUARED_GUI_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
if(NOT SQUARED_GUI_SOURCES)
    message(FATAL_ERROR "Squared GUI has no implementation sources")
endif()

add_library(squared_gui STATIC ${SQUARED_GUI_SOURCES})
set_target_properties(squared_gui PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(squared_gui PUBLIC cxx_std_20)
target_compile_options(squared_gui PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
target_include_directories(squared_gui PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Generated projects discover module directories alphabetically. Defer this
# dependency edge to the top-level directory so later module directories have
# registered their targets before GUI validates and links them.
cmake_language(DEFER
    DIRECTORY "${CMAKE_SOURCE_DIR}"
    CALL add_dependencies
        squared_gui
        squared_application
        squared_data
        squared_graphics2d
        squared_scene2d
)
cmake_language(DEFER
    DIRECTORY "${CMAKE_SOURCE_DIR}"
    CALL target_link_libraries
        squared_gui
        PUBLIC
        squared_application
        squared_data
        squared_graphics2d
        squared_scene2d
)

squared_register_module(squared_gui)
