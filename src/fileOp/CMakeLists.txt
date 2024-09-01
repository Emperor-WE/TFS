# 获取当前源文件所在目录的绝对路径
get_filename_component(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# 将该目录下所有源文件添加到 SRC_LIST 变量中
aux_source_directory(${SRC_DIR} SRC_LIST)

#添加库
add_library(fileOp_lib ${SRC_LIST})
#链接库
target_link_libraries(fileOp_lib -ldl)
#设置变量
set(src_fileOp ${SRC_LIST} CACHE INTERNAL "Description of the variable")
