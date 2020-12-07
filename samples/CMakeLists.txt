project(samples)

set(samples_sources)
file(GLOB sources *.cpp)
list(APPEND samples_sources ${sources})

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_MEMORY_GROWTH=1 --no-heap-copy")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/shell.html")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${CMAKE_SOURCE_DIR}/samples/assets@/assets/")
endif ()

foreach (samples_source ${samples_sources})
    get_filename_component(sample_name ${samples_source} NAME)
    string(REPLACE ".cpp" "${OUTPUT_EXT}" sample_name ${sample_name})

    add_executable(${sample_name} ${samples_source})

    target_link_libraries(${sample_name} raylib)
endforeach ()


set(samples_assets)
file(GLOB assets assets/*)
list(APPEND samples_assets ${assets})

file(COPY ${samples_assets} DESTINATION "assets/")
