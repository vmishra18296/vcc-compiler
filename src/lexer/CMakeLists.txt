add_library(vcc_lexer STATIC
    Lexer.cpp
)

target_include_directories(vcc_lexer
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(vcc_lexer
    PUBLIC vcc_common
)

target_compile_features(vcc_lexer PUBLIC cxx_std_20)
