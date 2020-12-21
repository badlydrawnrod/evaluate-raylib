cmake_minimum_required(VERSION 3.17)
project(0x37_evaluate_raylib)

set(CMAKE_C_STANDARD 99)

include(FetchContent)

# Search for a raylib-config.cmake. Fetch it and build it if we can't find one.
find_package(raylib 3.0 QUIET)
if (NOT raylib_FOUND)

    FetchContent_Declare(
            raylib
            #            URL https://github.com/raysan5/raylib/archive/master.tar.gz
            If master is broken for some reason then uncomment this.
            GIT_REPOSITORY ${CMAKE_SOURCE_DIR}/../3rdparty/raylib
            GIT_TAG rod2
    )
    FetchContent_GetProperties(raylib)

    # Download and build raylib.
    if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
        set(FETCHCONTENT_QUIET NO)
        FetchContent_Populate(raylib)

        # Don't build the raylib examples.
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

        # Build raylib
        add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
    endif ()
endif ()


# Obtain raygui if we don't already have it.
find_package(raygui QUIET)
if (NOT raygui_FOUND)

    FetchContent_Declare(
            raygui
            # If master is broken for some reason then uncomment this.
            GIT_REPOSITORY ${CMAKE_SOURCE_DIR}/../3rdparty/raygui
            GIT_SHALLOW ON
            GIT_TAG fix-sscanf
            #            GIT_REPOSITORY https://github.com/raysan5/raygui.git
            #            GIT_SHALLOW ON
            #            GIT_TAG master
    )

    FetchContent_GetProperties(raygui)

    # Download and build raygui.
    if (NOT raygui_POPULATED)
        set(FETCHCONTENT_QUIET NO)
        FetchContent_Populate(raygui)

        # Don't build the raygui examples.
        set(BUILD_RAYGUI_EXAMPLES OFF CACHE BOOL "" FORCE)

        # Build raygui.
        add_subdirectory(${raygui_SOURCE_DIR}/projects/CMake ${raygui_BINARY_DIR})
    endif ()
endif ()

set(raylib_VERBOSE 1)

add_subdirectory(simple_c)
add_subdirectory(spaceships_c)
