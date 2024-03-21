#!/bin/bash
g++ block_init_test.cpp file_op.cpp index_handle.cpp  mmap_file_op.cpp mmap_file.cpp -o init

g++ block_write_test.cpp file_op.cpp index_handle.cpp  mmap_file_op.cpp mmap_file.cpp -o write

g++ block_read_test.cpp file_op.cpp index_handle.cpp  mmap_file_op.cpp mmap_file.cpp -o read

g++ block_del_test.cpp file_op.cpp index_handle.cpp  mmap_file_op.cpp mmap_file.cpp -o del