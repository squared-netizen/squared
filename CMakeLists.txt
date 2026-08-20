cmake_minimum_required(VERSION 3.20)

project(squared VERSION 0.6.0 LANGUAGES C CXX)

include(CTest)

function(squared_test_target target)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_compile_options(${target}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
    set_target_properties(${target}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endfunction()

set(SQUARED_APPLICATION_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-application/content/modules/squared-application"
)
set(SQUARED_DATA_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-data/content/modules/squared-data"
)
set(SQUARED_DATA_YYJSON_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-data/content/third_party/yyjson-0.12.0"
)
set(SQUARED_GRAPHICS_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-graphics/content/modules/squared-graphics"
)
set(SQUARED_GRAPHICS2D_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-graphics2d/content/modules/squared-graphics2d"
)
set(SQUARED_MATH_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-math/content/modules/squared-math"
)
set(SQUARED_MESSAGING_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-messaging/content/modules/squared-messaging"
)
set(SQUARED_SCENE2D_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-scene2d/content/modules/squared-scene2d"
)
set(SQUARED_TIME_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-time/content/modules/squared-time"
)
set(SQUARED_HOLODISK_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-holoDisk/content/modules/squared-holoDisk"
)
set(SQUARED_HOLODISK_MINIZ_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-holoDisk/content/third_party/miniz-3.1.2"
)
set(SQUARED_GUI_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-gui/content/modules/squared-gui"
)

add_library(squared_test_yyjson STATIC
    "${SQUARED_DATA_YYJSON_ROOT}/src/yyjson.c"
)
set_target_properties(squared_test_yyjson
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)
target_include_directories(squared_test_yyjson
    PUBLIC
        "${SQUARED_DATA_YYJSON_ROOT}/src"
)

add_executable(squared_application_boundary_test
    tests/application-boundary-test.cpp
)
squared_test_target(squared_application_boundary_test)
target_include_directories(squared_application_boundary_test
    PRIVATE
        "${SQUARED_APPLICATION_ROOT}/include"
)

add_executable(squared_data_json_test
    tests/json-test.cpp
    "${SQUARED_DATA_ROOT}/src/json.cpp"
)
squared_test_target(squared_data_json_test)
target_include_directories(squared_data_json_test
    PRIVATE
        "${SQUARED_DATA_ROOT}/include"
)
target_link_libraries(squared_data_json_test
    PRIVATE
        squared_test_yyjson
)

add_executable(squared_graphics_test
    tests/graphics-test.cpp
)
squared_test_target(squared_graphics_test)
target_include_directories(squared_graphics_test
    PRIVATE
        "${SQUARED_GRAPHICS_ROOT}/include"
)

add_executable(squared_math_test
    tests/math-test.cpp
    "${SQUARED_MATH_ROOT}/src/matrix4.cpp"
)
squared_test_target(squared_math_test)
target_include_directories(squared_math_test
    PRIVATE
        "${SQUARED_MATH_ROOT}/include"
)

add_executable(squared_graphics2d_portable_test
    tests/graphics2d-portable-test.cpp
    "${SQUARED_GRAPHICS2D_ROOT}/src/bitmap_font.cpp"
    "${SQUARED_GRAPHICS2D_ROOT}/src/orthographic_camera.cpp"
    "${SQUARED_MATH_ROOT}/src/matrix4.cpp"
)
squared_test_target(squared_graphics2d_portable_test)
target_include_directories(squared_graphics2d_portable_test
    PRIVATE
        "${SQUARED_GRAPHICS_ROOT}/include"
        "${SQUARED_GRAPHICS2D_ROOT}/include"
        "${SQUARED_MATH_ROOT}/include"
)

add_executable(squared_scene2d_test
    tests/scene2d-test.cpp
    "${SQUARED_SCENE2D_ROOT}/src/actor.cpp"
    "${SQUARED_SCENE2D_ROOT}/src/group.cpp"
    "${SQUARED_SCENE2D_ROOT}/src/stage.cpp"
)
squared_test_target(squared_scene2d_test)
target_include_directories(squared_scene2d_test
    PRIVATE
        "${SQUARED_SCENE2D_ROOT}/include"
)

add_executable(squared_time_test
    tests/time-test.cpp
    "${SQUARED_TIME_ROOT}/src/timepiece.cpp"
)
squared_test_target(squared_time_test)
target_include_directories(squared_time_test
    PRIVATE
        "${SQUARED_TIME_ROOT}/include"
)

add_executable(squared_messaging_test
    tests/messaging-test.cpp
    "${SQUARED_MESSAGING_ROOT}/src/message_dispatcher.cpp"
    "${SQUARED_DATA_ROOT}/src/json.cpp"
    "${SQUARED_TIME_ROOT}/src/timepiece.cpp"
)
squared_test_target(squared_messaging_test)
target_include_directories(squared_messaging_test
    PRIVATE
        "${SQUARED_MESSAGING_ROOT}/include"
        "${SQUARED_DATA_ROOT}/include"
        "${SQUARED_TIME_ROOT}/include"
)
target_link_libraries(squared_messaging_test
    PRIVATE
        squared_test_yyjson
)

add_executable(squared_holodisk_test
    tests/holodisk-test.cpp
    "${SQUARED_HOLODISK_ROOT}/src/asset_manager.cpp"
    "${SQUARED_HOLODISK_ROOT}/src/holodrive.cpp"
    "${SQUARED_HOLODISK_MINIZ_ROOT}/miniz.c"
    "${SQUARED_HOLODISK_MINIZ_ROOT}/miniz_tdef.c"
    "${SQUARED_HOLODISK_MINIZ_ROOT}/miniz_tinfl.c"
    "${SQUARED_HOLODISK_MINIZ_ROOT}/miniz_zip.c"
)

add_executable(squared_gui_test
    tests/gui-test.cpp
    "${SQUARED_GRAPHICS2D_ROOT}/src/bitmap_font.cpp"
    "${SQUARED_GUI_ROOT}/src/gui.cpp"
    "${SQUARED_GUI_ROOT}/src/skin_loader.cpp"
    "${SQUARED_DATA_ROOT}/src/json.cpp"
    "${SQUARED_SCENE2D_ROOT}/src/actor.cpp"
    "${SQUARED_SCENE2D_ROOT}/src/group.cpp"
    "${SQUARED_SCENE2D_ROOT}/src/stage.cpp"
)
squared_test_target(squared_gui_test)
target_include_directories(squared_gui_test
    PRIVATE
        "${SQUARED_APPLICATION_ROOT}/include"
        "${SQUARED_DATA_ROOT}/include"
        "${SQUARED_GUI_ROOT}/include"
        "${SQUARED_SCENE2D_ROOT}/include"
        "${SQUARED_GRAPHICS_ROOT}/include"
        "${SQUARED_GRAPHICS2D_ROOT}/include"
)
target_link_libraries(squared_gui_test PRIVATE squared_test_yyjson)
set(SQUARED_GDX_SKINS_ASSET_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-gui/content/app/src/main/assets/gui/gdx-skins"
)
if(NOT EXISTS "${SQUARED_GDX_SKINS_ASSET_DIR}/PIN.sha256")
    message(FATAL_ERROR
        "Pinned gdx-skins asset is missing; run tools/vendor-gdx-skins.fish"
    )
endif()
if(NOT EXISTS "${SQUARED_GDX_SKINS_ASSET_DIR}/gdx-skins.zip")
    message(FATAL_ERROR "Pinned gdx-skins archive is missing")
endif()
file(STRINGS
    "${SQUARED_GDX_SKINS_ASSET_DIR}/PIN.sha256"
    SQUARED_GDX_SKINS_PIN_LINE
    LIMIT_COUNT 1
)
string(REGEX MATCH "^[0-9a-fA-F]+"
    SQUARED_GDX_SKINS_EXPECTED_SHA
    "${SQUARED_GDX_SKINS_PIN_LINE}"
)
string(LENGTH "${SQUARED_GDX_SKINS_EXPECTED_SHA}"
    SQUARED_GDX_SKINS_SHA_LENGTH
)
if(NOT SQUARED_GDX_SKINS_SHA_LENGTH EQUAL 64)
    message(FATAL_ERROR "Pinned gdx-skins SHA-256 record is malformed")
endif()
file(SHA256
    "${SQUARED_GDX_SKINS_ASSET_DIR}/gdx-skins.zip"
    SQUARED_GDX_SKINS_ACTUAL_SHA
)
if(NOT SQUARED_GDX_SKINS_ACTUAL_SHA STREQUAL SQUARED_GDX_SKINS_EXPECTED_SHA)
    message(FATAL_ERROR "Pinned gdx-skins archive SHA-256 does not match PIN.sha256")
endif()
target_compile_definitions(squared_gui_test
    PRIVATE
        SQUARED_GUI_TEST_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/packages/squared-gui/content/app/src/main/assets/gui/kenney-test"
        SQUARED_GDX_SKINS_ASSET_DIR="${SQUARED_GDX_SKINS_ASSET_DIR}"
        SQUARED_GDX_SKINS_EXPECTED_SHA="${SQUARED_GDX_SKINS_EXPECTED_SHA}"
)

add_executable(squared_gui_showcase_test
    tests/gui-showcase-test.cpp
    examples/gui-showcase/src/showcase.cpp
    "${SQUARED_GRAPHICS2D_ROOT}/src/bitmap_font.cpp"
    "${SQUARED_GUI_ROOT}/src/gui.cpp"
    "${SQUARED_SCENE2D_ROOT}/src/actor.cpp"
    "${SQUARED_SCENE2D_ROOT}/src/group.cpp"
    "${SQUARED_SCENE2D_ROOT}/src/stage.cpp"
)
squared_test_target(squared_gui_showcase_test)
target_include_directories(squared_gui_showcase_test
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/gui-showcase/include"
        "${SQUARED_APPLICATION_ROOT}/include"
        "${SQUARED_GUI_ROOT}/include"
        "${SQUARED_SCENE2D_ROOT}/include"
        "${SQUARED_GRAPHICS_ROOT}/include"
        "${SQUARED_GRAPHICS2D_ROOT}/include"
)
squared_test_target(squared_holodisk_test)
target_include_directories(squared_holodisk_test
    PRIVATE
        "${SQUARED_HOLODISK_ROOT}/include"
        "${SQUARED_HOLODISK_MINIZ_ROOT}"
)

if(BUILD_TESTING)
    add_test(
        NAME squared_package_documentation
        COMMAND "${CMAKE_COMMAND}"
            -DSQUARED_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/package-docs-test.cmake
    )
    add_test(
        NAME squared_application_boundary
        COMMAND "$<TARGET_FILE:squared_application_boundary_test>"
    )
    add_test(
        NAME squared_data_strict_json
        COMMAND "$<TARGET_FILE:squared_data_json_test>"
    )
    add_test(
        NAME squared_graphics_color
        COMMAND "$<TARGET_FILE:squared_graphics_test>"
    )
    add_test(
        NAME squared_math_primitives
        COMMAND "$<TARGET_FILE:squared_math_test>"
    )
    add_test(
        NAME squared_graphics2d_portable_boundary
        COMMAND "$<TARGET_FILE:squared_graphics2d_portable_test>"
    )
    add_test(
        NAME squared_scene2d_hierarchy
        COMMAND "$<TARGET_FILE:squared_scene2d_test>"
    )
    add_test(
        NAME squared_time_domains
        COMMAND "$<TARGET_FILE:squared_time_test>"
    )
    add_test(
        NAME squared_telegram_messaging
        COMMAND "$<TARGET_FILE:squared_messaging_test>"
    )
    add_test(
        NAME squared_holodisk_drive
        COMMAND "$<TARGET_FILE:squared_holodisk_test>"
    )
    add_test(
        NAME squared_gui_portable_widgets
        COMMAND "$<TARGET_FILE:squared_gui_test>"
    )
    add_test(
        NAME squared_gui_complete_showcase
        COMMAND "$<TARGET_FILE:squared_gui_showcase_test>"
    )
endif()
