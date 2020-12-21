project(spaceships_c)

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s USE_WEBGL2=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_MEMORY_GROWTH=1 --no-heap-copy")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/shell.html")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${CMAKE_CURRENT_BINARY_DIR}/assets@assets/")
endif ()

add_executable(${PROJECT_NAME} spaceships.c raygui.c controller_selection.c controller_selection.h menu.c menu.h playing.c playing.h controllers.h)
target_include_directories(${PROJECT_NAME} INTERFACE raygui)
target_link_libraries(${PROJECT_NAME} raylib raygui)

set(spaceships_assets)
file(GLOB assets assets/*)
list(APPEND spaceships_assets ${assets})

file(COPY ${spaceships_assets} DESTINATION "assets/")
