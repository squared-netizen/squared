file(GLOB_RECURSE SQUARED_TIME_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

if(NOT SQUARED_TIME_SOURCES)
    message(FATAL_ERROR "Squared Time has no source files")
endif()

add_library(squared_time STATIC
    ${SQUARED_TIME_SOURCES}
)
set_target_properties(squared_time PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(squared_time PUBLIC cxx_std_20)
target_compile_options(squared_time PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
target_include_directories(squared_time PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

squared_register_module(squared_time)
