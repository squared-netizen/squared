if(NOT COMMAND squared_register_module)
    message(FATAL_ERROR
        "Squared Messaging requires the squared_register_module hook"
    )
endif()

file(GLOB_RECURSE SQUARED_MESSAGING_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
if(NOT SQUARED_MESSAGING_SOURCES)
    message(FATAL_ERROR "Squared Messaging has no implementation sources")
endif()

add_library(squared_messaging STATIC ${SQUARED_MESSAGING_SOURCES})
set_target_properties(squared_messaging PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(squared_messaging PUBLIC cxx_std_20)
target_compile_options(squared_messaging PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
target_include_directories(squared_messaging PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(squared_messaging PUBLIC
    squared_data
    squared_time
)

squared_register_module(squared_messaging)
