project(samples)

set(samples_sources)
file(GLOB sources *.cpp)
list(APPEND samples_sources ${sources})

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s USE_WEBGL2=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_MEMORY_GROWTH=1 --no-heap-copy")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/shell.html")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${CMAKE_CURRENT_BINARY_DIR}/assets@assets/")
endif ()

foreach (samples_source ${samples_sources})
    get_filename_component(sample_name ${samples_source} NAME)
    string(REPLACE ".cpp" "${OUTPUT_EXT}" sample_name ${sample_name})

    add_executable(${sample_name} ${samples_source} raygui.c menu.cpp menu.h controllers.h screens.h screens.cpp config.cpp config.h playing.cpp playing.h)

    target_include_directories(${sample_name} INTERFACE raygui)
    target_link_libraries(${sample_name} raylib raygui)
endforeach ()


set(samples_assets)
file(GLOB assets assets/*)
list(APPEND samples_assets ${assets})

file(COPY ${samples_assets} DESTINATION "assets/")
