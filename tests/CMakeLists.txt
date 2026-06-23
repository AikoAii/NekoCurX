list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)

add_executable(nkx_tests
    test_scanner.cpp
    test_detector.cpp
    test_riff.cpp
    test_ani_reader.cpp
    test_xcursor_writer.cpp
    test_theme_builder.cpp
)

target_include_directories(nkx_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(nkx_tests PRIVATE nkx_lib Catch2::Catch2WithMain)

catch_discover_tests(nkx_tests)
