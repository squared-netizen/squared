if(NOT COMMAND squared_register_module)
    message(FATAL_ERROR
        "Squared Application requires the squared_register_module hook"
    )
endif()

add_library(squared_application INTERFACE)
target_compile_features(squared_application INTERFACE cxx_std_20)
target_include_directories(squared_application INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

squared_register_module(squared_application)
