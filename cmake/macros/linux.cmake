# linux specific macros

# GEN_WAYLAND: args = `filename`
macro(GEN_WAYLAND wayland_directory subdirectory filename)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated-src)

    # Resolve the input protocol XML to an absolute path. CMAKE_SOURCE_DIR
    # may be relative (e.g. when cmake is invoked with a relative -S), and
    # wayland-scanner resolves paths against the build directory's working
    # directory -- a relative path there would fail with the cryptic
    # "Could not open input file: No such file or directory".
    set(WAYLAND_INPUT_XML "${wayland_directory}/${subdirectory}/${filename}.xml")
    get_filename_component(WAYLAND_INPUT_XML "${WAYLAND_INPUT_XML}" ABSOLUTE)
    get_filename_component(WAYLAND_OUTPUT_C "${CMAKE_BINARY_DIR}/generated-src/${filename}.c" ABSOLUTE)
    get_filename_component(WAYLAND_OUTPUT_H "${CMAKE_BINARY_DIR}/generated-src/${filename}.h" ABSOLUTE)

    if(NOT EXISTS "${WAYLAND_INPUT_XML}")
        message(FATAL_ERROR
                "wayland-scanner input protocol not found: ${WAYLAND_INPUT_XML}\n"
                "The Wayland protocol XML is missing -- usually because the "
                "'third-party/wayland-protocols' (or 'third-party/wlr-protocols') "
                "git submodule was not initialised. Run:\n"
                "    git submodule update --init --recursive\n"
                "then re-run the build.")
    endif()

    message("wayland-scanner private-code ${WAYLAND_INPUT_XML} ${WAYLAND_OUTPUT_C}")
    message("wayland-scanner client-header ${WAYLAND_INPUT_XML} ${WAYLAND_OUTPUT_H}")
    execute_process(
            COMMAND wayland-scanner private-code
            ${WAYLAND_INPUT_XML}
            ${WAYLAND_OUTPUT_C}
            COMMAND wayland-scanner client-header
            ${WAYLAND_INPUT_XML}
            ${WAYLAND_OUTPUT_H}

            RESULT_VARIABLE EXIT_INT
    )

    if(NOT ${EXIT_INT} EQUAL 0)
        message(FATAL_ERROR "wayland-scanner failed for ${WAYLAND_INPUT_XML}")
    endif()

    list(APPEND PLATFORM_TARGET_FILES
            ${WAYLAND_OUTPUT_C}
            ${WAYLAND_OUTPUT_H})
endmacro()
