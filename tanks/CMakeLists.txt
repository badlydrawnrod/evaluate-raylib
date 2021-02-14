project(tanks)

if (MSVC)
    # Warning level 4 and all warnings as errors.
    add_compile_options(/W4 /WX)
else ()
    # Lots of warnings and all warnings as errors.
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif ()

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s USE_WEBGL2=1")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s ALLOW_MEMORY_GROWTH=1 --no-heap-copy")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/shell.html")
    if (IS_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${CMAKE_CURRENT_BINARY_DIR}/assets@assets/")
    endif ()
endif ()

add_executable(tanks tanks.c controls.c menu.c playing.c tanks.h bdr/loop.h)
target_link_libraries(tanks raylib)

set(tanks_assets)
file(GLOB assets assets/*)
list(APPEND tanks_assets ${assets})

file(COPY ${tanks_assets} DESTINATION "assets/")
