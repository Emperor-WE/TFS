#include "common.h"
#include "file_op.h"
#include "index_handle.h"
#include <sstream>

using namespace tbfs;

const static largefile::MMapOption mmap_option = {1024*1024, 4096, 4096};   //内存映射参数
const static uint32_t main_blocksize = 1024*1024*64;    //主块文件大小
const static uint32_t bucket_size = 1000;   //哈希桶大小

int32_t block_id = 1;

int directory_exists(const char *path);

const static int32_t debug = 1;

int main(int argc, char* argv[]) {
    std::string mainblock_path;
    std::string index_path;

    block_id = std::stoi(argv[1]);

    //1.生成主块文件
    std::stringstream tmp_stream;
    tmp_stream << "." << largefile::MAINBLOCK_DIR_PREFIX << block_id;
    tmp_stream >> mainblock_path;

    // 检查目录是否存在
    if (!directory_exists("./mainblock")) {
        // 创建目录
        if (mkdir("./mainblock", 0755) == -1) {
            perror("mkdir");
            return 1;
        }
    }

    largefile::FileOperation* mainblock = new largefile::FileOperation(mainblock_path, O_CREAT | O_RDWR | O_LARGEFILE);

    int32_t ret = mainblock->ftruncate_file(main_blocksize);
    if(ret != 0) {
        fprintf(stderr, "Create main block %s failed. reason:%s\n", mainblock_path.c_str(), strerror(errno));
        mainblock->unlink_file();
        delete mainblock;
        exit(-2);
    }

    //2.生成索引文件
    // 检查目录是否存在
    if (!directory_exists("index")) {
        // 创建目录
        if (mkdir("index", 0755) == -1) {
            perror("mkdir");
            return 1;
        }
    }
    
    largefile::IndexHanle* index_handle = new largefile::IndexHanle(".", block_id); //索引文件句柄

    if(debug) printf("Init index...\n");
    
    ret = index_handle->create(block_id, bucket_size, mmap_option);
    if(ret != largefile::TFS_SUCCESS) {
        fprintf(stderr, "create index %d failed.\n", block_id);
        index_handle->remove(block_id);
        mainblock->unlink_file();
        delete index_handle;
        delete mainblock;
        exit(-3);
    }

    //释放资源
    mainblock->flush_file();
    mainblock->close_file();
    index_handle->flush();

    delete mainblock;
    delete index_handle;

    return 0;
}

// 检查目录是否存在
int directory_exists(const char *path) {
    struct stat info;

    if (stat(path, &info) != 0) {
        return 0;  // 目录不存在
    } else if (info.st_mode & S_IFDIR) {
        return 1;  // 是目录
    } else {
        return 0;  // 不是目录
    }
}