cmake_minimum_required(VERSION 3.17)
project(0x37_evaluate_raylib)

set(CMAKE_CXX_STANDARD 20)

# Search for a raylib-config.cmake. Fetch it and build it if we can't find one.
if (NOT EMSCRIPTEN)
    find_package(raylib 3.0 QUIET)
    if (NOT raylib_FOUND)
        include(FetchContent)

        FetchContent_Declare(
                raylib
                URL https://github.com/raysan5/raylib/archive/master.tar.gz
                # If master is broken for some reason then uncomment this.
                #            GIT_REPOSITORY ${CMAKE_SOURCE_DIR}/../3rdparty/raylib
                #            GIT_TAG rod
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
endif ()

if (EMSCRIPTEN)
    # I haven't been able to build Raylib with Emscripten using CMake, so this is currently, "it works on my machine."
    add_library(raylib STATIC IMPORTED)
    set_property(TARGET raylib PROPERTY IMPORTED_LOCATION "D:\\Users\\Rod\\Projects\\3rdparty\\raylib\\build\\raylib-dev_webassembly\\lib\\libraylib.a")
    include_directories(D:\\Users\\Rod\\Projects\\3rdparty\\raylib\\build\\raylib-dev_webassembly\\include)
endif ()

set(raylib_VERBOSE 1)

add_subdirectory(samples)
