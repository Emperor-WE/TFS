#include "mmap_file.h"
//#include "file_op.h"
#include "common.h"

using namespace tbfs;

static const mode_t OPEN_MODE = 0644;
const static largefile::MMapOption mmap_option = {10240000, 4096, 4096};

int open_file(const char* file_name, int open_flags) {
    int fd = open(file_name, open_flags, OPEN_MODE);
    if(fd < 0) {
        return -errno;
    }
    return fd;
}

int main() {
    const char* filename = "./mmap_file.txt";

    int fd = open_file(filename, O_RDWR | O_CREAT | O_LARGEFILE);
    if(fd < 0) {
        fprintf(stderr, "open file failed, filename:%s, error desc: %s\n", filename, strerror(-fd));
        return -1;
    }

    largefile::MMapFile* map_file = new largefile::MMapFile(mmap_option, fd);

    if(map_file->map_file(true)) {
        map_file->remap_file();
        memset(map_file->get_data(), '8', map_file->get_size());
        map_file->sync_file();
        map_file->munmap_file();
    } else {
        fprintf(stderr, "map file failed\n");
        return -1;
    }

    close(fd);

    return 0;
}