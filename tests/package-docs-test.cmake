if(NOT DEFINED SQUARED_SOURCE_DIR)
    message(FATAL_ERROR "SQUARED_SOURCE_DIR is required")
endif()

file(GLOB package_manifests
    LIST_DIRECTORIES false
    "${SQUARED_SOURCE_DIR}/packages/*/manifest.json"
)
list(LENGTH package_manifests package_count)
if(package_count EQUAL 0)
    message(FATAL_ERROR "No Squared package manifests were found")
endif()

foreach(manifest IN LISTS package_manifests)
    get_filename_component(package_dir "${manifest}" DIRECTORY)
    get_filename_component(package_name "${package_dir}" NAME)
    set(history "${package_dir}/history.md")
    set(todo "${package_dir}/TODO.md")

    if(NOT EXISTS "${history}")
        message(FATAL_ERROR "${package_name} is missing history.md")
    endif()
    if(NOT EXISTS "${todo}")
        message(FATAL_ERROR "${package_name} is missing TODO.md")
    endif()

    file(READ "${manifest}" manifest_contents)
    string(REGEX MATCH "\"version\"[ \t]*:[ \t]*\"([^\"]+)\""
        version_match "${manifest_contents}"
    )
    if(NOT CMAKE_MATCH_1)
        message(FATAL_ERROR "${package_name} manifest has no version")
    endif()
    set(package_version "${CMAKE_MATCH_1}")

    file(READ "${history}" history_contents)
    string(FIND "${history_contents}" "## ${package_version}" version_heading)
    if(version_heading EQUAL -1)
        message(FATAL_ERROR
            "${package_name}/history.md does not document ${package_version}"
        )
    endif()

    file(READ "${todo}" todo_contents)
    string(FIND "${todo_contents}" "## Next" next_heading)
    string(FIND "${todo_contents}" "- [ ]" unchecked_item)
    string(FIND "${todo_contents}" "history.md" maintenance_rule)
    if(next_heading EQUAL -1 OR unchecked_item EQUAL -1 OR
       maintenance_rule EQUAL -1)
        message(FATAL_ERROR
            "${package_name}/TODO.md must contain Next work, an unchecked item, and the history handoff rule"
        )
    endif()
endforeach()

message(STATUS
    "Verified package-owned history.md and TODO.md for ${package_count} packages"
)

