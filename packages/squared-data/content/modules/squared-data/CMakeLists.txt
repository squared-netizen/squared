if(NOT COMMAND squared_register_module)
    message(FATAL_ERROR
        "Squared Data requires the squared_register_module hook"
    )
endif()

set(SQUARED_DATA_YYJSON_ROOT
    "${PROJECT_ROOT}/third_party/yyjson-0.12.0"
)
if(NOT EXISTS "${SQUARED_DATA_YYJSON_ROOT}/src/yyjson.c" OR
   NOT EXISTS "${SQUARED_DATA_YYJSON_ROOT}/src/yyjson.h")
    message(FATAL_ERROR
        "Squared Data private yyjson sources are missing: "
        "${SQUARED_DATA_YYJSON_ROOT}"
    )
endif()

add_library(squared_data_yyjson STATIC
    "${SQUARED_DATA_YYJSON_ROOT}/src/yyjson.c"
)
set_target_properties(squared_data_yyjson PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_include_directories(squared_data_yyjson PUBLIC
    "${SQUARED_DATA_YYJSON_ROOT}/src"
)

file(GLOB_RECURSE SQUARED_DATA_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
if(NOT SQUARED_DATA_SOURCES)
    message(FATAL_ERROR "Squared Data has no implementation sources")
endif()

add_library(squared_data STATIC ${SQUARED_DATA_SOURCES})
set_target_properties(squared_data PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(squared_data PUBLIC cxx_std_20)
target_compile_options(squared_data PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
target_include_directories(squared_data PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(squared_data PRIVATE
    squared_data_yyjson
)

squared_register_module(squared_data)
