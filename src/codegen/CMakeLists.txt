add_library(vcc_codegen STATIC
    CodeGen.cpp
    LLVMBackend.cpp
    LLVMCodeGenerator.cpp
)

target_include_directories(vcc_codegen
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(vcc_codegen
    PUBLIC vcc_ir
    PUBLIC vcc_common
)

if(VCC_ENABLE_LLVM)
    llvm_map_components_to_libnames(llvm_libs core support analysis)
    target_link_libraries(vcc_codegen PUBLIC ${llvm_libs})
    target_compile_definitions(vcc_codegen PUBLIC VCC_ENABLE_LLVM)
endif()

target_compile_features(vcc_codegen PUBLIC cxx_std_20)
