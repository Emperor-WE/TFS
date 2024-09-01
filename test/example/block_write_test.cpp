#include "common.h"
#include "file_op.h"
#include "index_handle.h"
#include <sstream>

using namespace tbfs;

const static largefile::MMapOption mmap_option = {1024*1024, 4096, 4096};   //内存映射参数
const static uint32_t main_blocksize = 1024*1024*64;    //主块文件大小
const static uint32_t bucket_size = 1000;   //哈希桶大小

int32_t block_id = 1;

const static int32_t debug = 1;

int main(int argc, char* argv[]) {
    std::string mainblock_path;
    std::string index_path;

    block_id = std::stoi(argv[1]);

    std::stringstream tmp_stream;
    tmp_stream << "." << largefile::MAINBLOCK_DIR_PREFIX << block_id;
    tmp_stream >> mainblock_path;
    
    //1.加载索引文件
    largefile::IndexHanle* index_handle = new largefile::IndexHanle(".", block_id); //索引文件句柄

    if(debug) printf("Load index...\n");
    
    int ret = index_handle->load(block_id, bucket_size, mmap_option);
    if(ret != largefile::TFS_SUCCESS) {
        fprintf(stderr, "load index %d failed.\n", block_id);
        index_handle->remove(block_id);
        delete index_handle;
        exit(-3);
    }

    //1.写入文件到主块文件
    largefile::FileOperation* mainblock = new largefile::FileOperation(mainblock_path, O_CREAT | O_RDWR | O_LARGEFILE);

    char buffer[4096];
    memset(buffer, '6', sizeof(buffer));

    int32_t data_offset = index_handle->get_block_data_offset();
    int32_t file_no = index_handle->block_info()->seq_no_;

    if((ret = mainblock->pwrite_file(buffer, sizeof(buffer), data_offset)) != largefile::TFS_SUCCESS) {
        fprintf(stderr, "write to main block failed. ret:%d, reason:%s\n", ret, strerror(errno));
        mainblock->close_file();

        delete mainblock;
        delete index_handle;
        exit(-3);
    }

    //数据必须写入到主块，同时索引文件更新成功才是写入成功
    //3.索引文件写入 MetaInfo
    largefile::MetaInfo meta;
    meta.set_file_id(file_no);
    meta.set_offset(data_offset);
    meta.set_size(sizeof(buffer));

    ret = index_handle->write_segment(meta.get_key(), meta);
    if(ret == largefile::TFS_SUCCESS) {
        //1.更新索引头部信息
        index_handle->commit_block_data_offset(sizeof(buffer));

        //2.更新块信息
        index_handle->update_block_info(largefile::C_OPER_INSERT, sizeof(buffer));

        ret = index_handle->flush();
        if(ret != largefile::TFS_SUCCESS) {
            fprintf(stderr, "flush mainblock %d failed. file no:%u\n", block_id, file_no);
        }
    } 

    if(ret != largefile::TFS_SUCCESS) {
        fprintf(stderr, "write to mainblock %d failed. file no:%u\n", block_id, file_no);
    } else {
        if(debug) printf("write to mainblock %d successfully. file no:%u\n", block_id, file_no);
    }

    //释放资源
    mainblock->close_file();

    delete mainblock;
    delete index_handle;

    return 0;
}