add_library(vcc_ir STATIC
    IRInstruction.cpp
    IRModule.cpp
    IRPrinter.cpp
    IRBuilder.cpp
    IRGen.cpp
)

target_include_directories(vcc_ir
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(vcc_ir
    PUBLIC vcc_ast
    PUBLIC vcc_common
)

target_compile_features(vcc_ir PUBLIC cxx_std_20)
