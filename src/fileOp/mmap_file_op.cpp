#include "mmap_file_op.h"

const static int debug = 1;

namespace tbfs
{
    namespace largefile
    {
        //如果已经映射并且所映射的区域大小足够的话，直接从映射区读取，否则直接依据 fd 直接从磁盘读取
        int MMapFileOperation::pread_file(char* buf, const int32_t size, const int64_t offset) {
            //1.文件已经映射到内存
            if(is_mapped_ && (offset + size) > map_file_->get_size()) {    //内存映射大小不够
                if(debug) fprintf(stdout, "MMapFileOperation: pread size=%d, map file size=%d, need remap\n", 
                size, map_file_->get_size());

                map_file_->remap_file();    //追加内存空间映射，之扩大一次后再次尝试
            }

            if(is_mapped_ && (offset + size) <= map_file_->get_size()) {
                memcpy(buf, (char*)map_file_->get_data() + offset, size);
                return TFS_SUCCESS;
            }

            //2.内存没有映射 或者 映射空降大小还是不够
            return FileOperation::pread_file(buf, size, offset);    //依据 fd 直接从磁盘文件读取
        }

        //如果已经映射并且所映射的区域大小足够的话，直接写入映射区，否则直接依据 fd 直接写入磁盘文件
        int MMapFileOperation::pwrite_file(const char* buf, const int32_t size, const int64_t offset) {
            //1.文件已经映射到内存
            if(is_mapped_ && (offset + size) > map_file_->get_size()) {    //内存映射大小不够
                if(debug) fprintf(stdout, "MMapFileOperation: pwrite size=%d, map file size=%d, need remap\n", 
                size, map_file_->get_size());

                map_file_->remap_file();    //追加内存空间映射，之扩大一次后再次尝试
            }

            if(is_mapped_ && (offset + size) <= map_file_->get_size()) {
                memcpy((char*)map_file_->get_data() + offset, buf, size);
                return TFS_SUCCESS;
            }

            //2.内存没有映射 或者 映射空降大小还是不够
            return FileOperation::pwrite_file(buf, size, offset);    //依据 fd 直接从写入磁盘文件
        }

        int MMapFileOperation::mmap_file(const MMapOption& mmap_operation) {
            if(mmap_operation.max_mmap_size_ < mmap_operation.first_mmap_size_ ||
                mmap_operation.max_mmap_size_ <= 0) {
                    return TFS_FAILED;
            }

            int fd = check_file();
            if(fd < 0) {
                fprintf(stderr, "MMapFileOperation::mmap_file checking file error\n");
                return TFS_FAILED;
            }

            if(!is_mapped_) {
                if(map_file_) delete map_file_;
                map_file_ = new MMapFile(mmap_operation, fd);
                is_mapped_ = map_file_->map_file(true);
            }

            if(!is_mapped_) return TFS_FAILED;
            
            return TFS_SUCCESS;
        }

        int MMapFileOperation::munmap_file() {
            if(is_mapped_ && map_file_ != NULL) {
                delete map_file_;   //通过 MMapFile 的析构函数来解除映射
                is_mapped_ = false;
                map_file_ = NULL;
            }

            return TFS_SUCCESS;
        }

        void* MMapFileOperation::get_map_data() const {
            if(is_mapped_) {
                return map_file_->get_data();
            }
            
            return NULL;
        }

        //如果映射到内存了，就 映射内存 同步，否则
        int MMapFileOperation::flush_file() {
            if(is_mapped_) {
                if(!map_file_->sync_file()) {
                    fprintf(stderr, "MMapFileOperation: flush file syncing error\n");
                    return TFS_FAILED;
                } else {
                    return TFS_SUCCESS;
                }
            }
            return FileOperation::flush_file();    //依据 fd 同步到磁盘
        }

    }
}