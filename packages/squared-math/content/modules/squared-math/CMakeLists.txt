if(NOT COMMAND squared_register_module)
    message(FATAL_ERROR
        "Squared Math requires the squared_register_module hook"
    )
endif()

file(GLOB_RECURSE SQUARED_MATH_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
if(NOT SQUARED_MATH_SOURCES)
    message(FATAL_ERROR "Squared Math has no implementation sources")
endif()

add_library(squared_math STATIC ${SQUARED_MATH_SOURCES})
set_target_properties(squared_math PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(squared_math PUBLIC cxx_std_20)
target_compile_options(squared_math PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
target_include_directories(squared_math PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

squared_register_module(squared_math)
