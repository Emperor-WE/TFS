#include "file_op.h"

using namespace tbfs;

int main(void) {
    const char* filename = "file_op.txt";
    largefile::FileOperation* fileOP = new largefile::FileOperation(filename);

    int fd = fileOP->open_file();
    if(fd < 0) {
        fprintf(stderr, "open file %s error: %s\n", filename, strerror(-fd));
        return -1;
    }

    char buffer[4094];
    memset(buffer, '8', 4096);
    int ret = fileOP->pwrite_file(buffer, 4096, 1024);
    //int ret = fileOP->write_file(buffer, 4096);
    if(ret != largefile::TFS_SUCCESS) {
        fprintf(stderr, "write to file with offset error:%s\n", strerror(-ret));
        return -1;
    }

    char buf[8192];
    ret = fileOP->pread_file(buf, 8, 0);
    if(ret != largefile::TFS_SUCCESS) {
        fprintf(stderr, "read file error\n");
        return -1;
    }

    fprintf(stdout, "read content:%s\n", buf);

    char temp[64];
    memset(temp, '1', 64);
    fileOP->write_file(temp, 64);


    fileOP->close_file();
}