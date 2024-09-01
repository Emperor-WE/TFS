set(
    LIB_LIB
    fileOp_lib
    pthread
    dl
)

add_executable(test_init block_init_test.cpp)
target_link_libraries(test_init ${LIB_LIB})

add_executable(test_read block_read_test.cpp)
target_link_libraries(test_read ${LIB_LIB})

add_executable(test_write block_write_test.cpp)
target_link_libraries(test_write ${LIB_LIB})

add_executable(test_del block_del_test.cpp)
target_link_libraries(test_del ${LIB_LIB})

