add_library(vcc_semantic STATIC
    SymbolTable.cpp
    TypeChecker.cpp
    SemanticAnalyzer.cpp
)

target_include_directories(vcc_semantic
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(vcc_semantic
    PUBLIC vcc_ast
    PUBLIC vcc_common
)

target_compile_features(vcc_semantic PUBLIC cxx_std_20)
