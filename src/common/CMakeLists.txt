add_library(vcc_common STATIC
    SourceLocation.cpp
    Diagnostics.cpp
    Token.cpp
    CompilerContext.cpp
)

target_include_directories(vcc_common
    PUBLIC  ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(vcc_common PUBLIC cxx_std_20)
