#include "mmap_file_op.h"
#include <iostream>

using namespace tbfs;
const static largefile::MMapOption mmap_option = {10240000, 4096, 4096};

int main() {
    const char* filename = "mmap_file_op.txt";
    largefile::MMapFileOperation mmap(filename);

    int fd = mmap.open_file();
    if(fd < 0) {
        fprintf(stderr, "open %s failed, reason:%s\n", filename, strerror(errno));
        return -1;
    }

    int ret = mmap.mmap_file(mmap_option);
    if(ret < 0) {
        fprintf(stderr, "mmap_file failed, reason:%s\n", strerror(errno));
        mmap.close_file();
        exit(-1);
    }

    char buffer[128+1];
    memset(buffer, '6', 128);
    ret = mmap.pwrite_file(buffer, 128, 64);
    if(ret < 0) {
        fprintf(stderr, "write info to file error\n");
        return 0;
    }

    ret = mmap.flush_file();
    if(ret < 0) {
        fprintf(stderr, "write info to file error\n");
        return 0;
    }

    memset(buffer, 0, 128);
    ret = mmap.pread_file(buffer, 128, 64);
    if(ret < 0) {
        fprintf(stderr, "write info to file error\n");
        return 0;
    }
    buffer[128] = '\0';
    printf("read ifno:%s\n", buffer);

    memset(buffer, '6', 128);
    ret = mmap.pwrite_file(buffer, 128, 4000);
    if(ret < 0) {
        fprintf(stderr, "write info to file error\n");
        return 0;
    }

    mmap.munmap_file();
    mmap.close_file();

    return 0;
}