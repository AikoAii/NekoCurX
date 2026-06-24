add_library(nkx_lib STATIC
    core/types.cpp
    theme/scanner.cpp
    theme/detector.cpp
    theme/builder.cpp
    theme/installer.cpp
    ani/riff.cpp
    ani/ico_decoder.cpp
    ani/reader.cpp
    xcursor/writer.cpp
    cli/app.cpp
)

target_include_directories(nkx_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(nkx_lib PUBLIC 
    CLI11::CLI11 
    nlohmann_json::nlohmann_json
    tl::expected
    ${XCURSOR_LIBRARIES}
    ${LIBPNG_LIBRARIES}
)

add_executable(nkx main.cpp)
target_link_libraries(nkx PRIVATE nkx_lib)
