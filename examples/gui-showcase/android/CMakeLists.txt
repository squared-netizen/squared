file(GLOB_RECURSE PROJECT_APPLICATION_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

if(NOT PROJECT_APPLICATION_SOURCES)
    message(FATAL_ERROR "GUI showcase has no application sources")
endif()

add_library(project_application STATIC ${PROJECT_APPLICATION_SOURCES})
set_target_properties(project_application PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(project_application PUBLIC cxx_std_20)
target_compile_options(project_application PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
target_include_directories(project_application PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${SDL2_INCLUDE_DIR}"
)
target_link_libraries(project_application PUBLIC
    squared_application
    squared_gui
    squared_scene2d
    squared_backend_sdl2_opengl
    SDL2_ttf
    SDL2
)
