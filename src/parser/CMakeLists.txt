add_library(vcc_parser STATIC
    Parser.cpp
)

target_include_directories(vcc_parser
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(vcc_parser
    PUBLIC vcc_ast
    PUBLIC vcc_common
)

target_compile_features(vcc_parser PUBLIC cxx_std_20)
