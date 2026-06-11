add_executable(vcc-dump-ast main.cpp)

target_include_directories(vcc-dump-ast
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(vcc-dump-ast
    PRIVATE vcc_driver_lib
)

target_compile_features(vcc-dump-ast PRIVATE cxx_std_20)
