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

    //2.读取文件的mate info
    uint64_t file_id = 1;
    file_id = static_cast<uint64_t>(std::stoi(argv[2]));
    largefile::MetaInfo meta;

    ret = index_handle->read_segment(file_id, meta);
    if(ret != largefile::TFS_SUCCESS) {
        fprintf(stderr, "read_segment_meta error. file_id:%ld, ret:%d\n", file_id, ret);
        //exit(-3);
    }

    //3.根据 meta_info 读取文件
    std::stringstream tmp_stream;
    tmp_stream << "." << largefile::MAINBLOCK_DIR_PREFIX << block_id;
    tmp_stream >> mainblock_path;

    largefile::FileOperation* mainblock = new largefile::FileOperation(mainblock_path, O_RDWR);
    char* buffer = new char[meta.get_size() + 1];
    ret = mainblock->pread_file(buffer, meta.get_size(), meta.get_offset());
    if(largefile::TFS_SUCCESS != ret) {
        fprintf(stderr, "read from main block failed. ret:%d, reason:%s\n", ret, strerror(errno));
        delete mainblock;
        delete index_handle;
        exit(-3);
    }

    buffer[meta.get_size()] = '\0';
    printf("read info size %d: %s\n", meta.get_size(), buffer);

    
    //释放资源
    mainblock->close_file();
    delete mainblock;
    delete index_handle;

    return 0;
}