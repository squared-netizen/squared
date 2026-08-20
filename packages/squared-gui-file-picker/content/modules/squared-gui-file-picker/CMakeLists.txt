if(NOT COMMAND squared_register_module)
    message(FATAL_ERROR
        "Squared GUI File Picker requires the squared_register_module hook"
    )
endif()

add_library(squared_gui_file_picker STATIC
    src/file_picker.cpp
)
set_target_properties(squared_gui_file_picker PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_features(squared_gui_file_picker PUBLIC cxx_std_20)
target_compile_options(squared_gui_file_picker PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
target_include_directories(squared_gui_file_picker PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

cmake_language(DEFER
    DIRECTORY "${CMAKE_SOURCE_DIR}"
    CALL add_dependencies
        squared_gui_file_picker
        squared_gui
        squared_holodisk
)
cmake_language(DEFER
    DIRECTORY "${CMAKE_SOURCE_DIR}"
    CALL target_link_libraries
        squared_gui_file_picker
        PUBLIC
        squared_gui
        squared_holodisk
)

squared_register_module(squared_gui_file_picker)
