project(spaceships)

if (MSVC)
    # Warning level 4 and all warnings as errors.
    add_compile_options(/W4 /WX)
else ()
    # Lots of warnings and all warnings as errors.
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif ()

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s USE_WEBGL2=1 -s ALLOW_MEMORY_GROWTH=1 --no-heap-copy --shell-file ${CMAKE_SOURCE_DIR}/shell.html")
    if (IS_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${CMAKE_CURRENT_BINARY_DIR}/assets@assets/")
    endif ()
endif ()

add_executable(spaceships spaceships.c controls.c menu.c playing.c spaceships.h)
target_include_directories(spaceships PRIVATE ${CMAKE_SOURCE_DIR} draw_text_rec)
target_link_libraries(spaceships raylib draw_text_rec)

add_executable(advanced_spaceships advanced_spaceships.c controls.c menu.c playing.c spaceships.h)
target_include_directories(advanced_spaceships PRIVATE ${CMAKE_SOURCE_DIR} draw_text_rec)
target_link_libraries(advanced_spaceships raylib draw_text_rec)

set(spaceships_assets)
file(GLOB assets ${CMAKE_SOURCE_DIR}/assets/*)
list(APPEND spaceships_assets ${assets})

file(COPY ${spaceships_assets} DESTINATION "assets/")
