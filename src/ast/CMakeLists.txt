add_library(vcc_ast STATIC
    ASTNode.cpp
    ASTDumper.cpp
    Expressions.cpp
)

target_include_directories(vcc_ast
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(vcc_ast
    PUBLIC vcc_common
)

target_compile_features(vcc_ast PUBLIC cxx_std_20)
