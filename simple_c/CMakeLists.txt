project(simple)

set(simple_sources)
file(GLOB sources *.c)
list(APPEND simple_sources ${sources})

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s USE_WEBGL2=1")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s ALLOW_MEMORY_GROWTH=1 --no-heap-copy")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/shell.html")
    if (IS_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${CMAKE_CURRENT_BINARY_DIR}/assets@assets/")
    endif ()
endif ()

foreach (simple_source ${simple_sources})
    get_filename_component(sample_name ${simple_source} NAME)
    add_executable(${sample_name} ${simple_source})
    target_link_libraries(${sample_name} raylib)
endforeach ()

if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assets)
    set(simple_assets)
    file(GLOB assets assets/*)
    list(APPEND simple_assets ${assets})

    file(COPY ${simple_assets} DESTINATION "assets/")
endif ()
