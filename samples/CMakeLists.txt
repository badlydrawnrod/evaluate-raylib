project(samples)

set(samples_sources)
file(GLOB sources *.cpp)
list(APPEND samples_sources ${sources})

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
