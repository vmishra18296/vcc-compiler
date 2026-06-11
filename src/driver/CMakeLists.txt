add_library(vcc_driver_lib STATIC
    Compiler.cpp
)

target_include_directories(vcc_driver_lib
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(vcc_driver_lib
    PUBLIC  vcc_codegen
    PUBLIC  vcc_ir
    PUBLIC  vcc_semantic
    PUBLIC  vcc_parser
    PUBLIC  vcc_lexer
    PUBLIC  vcc_ast
    PUBLIC  vcc_common
)

target_compile_features(vcc_driver_lib PUBLIC cxx_std_20)

# ─── vcc executable ──────────────────────────────────────────────────────────
add_executable(vcc main.cpp)
target_link_libraries(vcc PRIVATE vcc_driver_lib)
target_compile_features(vcc PRIVATE cxx_std_20)

install(TARGETS vcc DESTINATION bin)
