include(GoogleTest)

# ─── Test executables ─────────────────────────────────────────────────────────

add_executable(test_lexer    test_lexer.cpp)
add_executable(test_parser   test_parser.cpp)
add_executable(test_semantic test_semantic.cpp)
add_executable(test_ir       test_ir.cpp)

foreach(target test_lexer test_parser test_semantic test_ir)
    target_link_libraries(${target}
        PRIVATE vcc_driver_lib
        PRIVATE GTest::gtest
        PRIVATE GTest::gtest_main
    )
    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_features(${target} PRIVATE cxx_std_20)
    gtest_discover_tests(${target})
endforeach()
