#include "mmap_file.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define DEBUG 1

namespace tbfs
{
    namespace largefile
    {
        MMapFile::MMapFile() : size_(0), fd_(-1), data_(NULL) {

        }

        MMapFile::MMapFile(const int fd) : size_(0), fd_(fd), data_(NULL) {

        }

        MMapFile::MMapFile(const MMapOption& mmap_option, const int fd) : size_(0), fd_(fd), data_(NULL) {
            mmap_file_option_.max_mmap_size_ = mmap_option.max_mmap_size_;
            mmap_file_option_.first_mmap_size_ = mmap_option.first_mmap_size_;
            mmap_file_option_.per_mmap_size_ = mmap_option.per_mmap_size_;
        }

        MMapFile::~MMapFile() {
            if(data_) {
                if(DEBUG) fprintf(stdout, "mmap file destructed: fd=%d, size=%d, data=%p\n",fd_, size_, data_);
                //会强制将被映射区中发生的更改立即写入到磁盘，并且会一直等待直到写入操作完成。
                //这意味着，msync 函数会阻塞当前进程，直到所有的数据都被成功写入磁盘。
                msync(data_, size_, MS_SYNC);
                munmap(data_, size_);

                size_ = 0;
                fd_ = -1;
                data_ = NULL;

                mmap_file_option_.max_mmap_size_ = 0;
                mmap_file_option_.first_mmap_size_ = 0;
                mmap_file_option_.per_mmap_size_ = 0;
            }
        }

        bool MMapFile::sync_file() {
            if(NULL != data_ && size_ > 0) {
                return msync(data_, size_, MS_ASYNC) == 0;
            }
            return true;
        }

        bool MMapFile::map_file(const bool wirte) {
            if(fd_ < 0) return false;
            if(0 == mmap_file_option_.max_mmap_size_) return false;

            int flags = PROT_READ;
            if(write) flags |= PROT_WRITE;

            if(size_ < mmap_file_option_.max_mmap_size_) {
                size_ = mmap_file_option_.first_mmap_size_;
            } else {
                size_ = mmap_file_option_.max_mmap_size_;
            }

            if(!ensure_file_size(size_)) {
                return false;
            }

            data_ = mmap(NULL, size_, flags, MAP_SHARED, fd_, 0);
            if(MAP_FAILED == data_) {
                fprintf(stderr, "mmap file failed, %s\n", strerror(errno));

                size_ = 0;
                fd_ = -1;
                data_ = NULL;
                bzero(&mmap_file_option_, sizeof(mmap_file_option_));

                return false;
            }

            if(DEBUG) fprintf(stdout, "mmap file successed, fd=%d, size=%d, data=%p\n", fd_, size_, data_);

            return true;
        }

        void* MMapFile::get_data() const {
            return data_;
        }

        int32_t MMapFile::get_size() const {
            return size_;
        }

        bool MMapFile::munmap_file() {
            if(NULL == data_ || size_ < 0) return false;

            return munmap(data_, size_) == 0;
        }

        bool MMapFile::remap_file() {
            if(NULL == data_ || fd_ < 0) {
                fprintf(stderr, "mremap not mapped yet\n");
                return false;
            }

            if(size_ == mmap_file_option_.max_mmap_size_) {
                fprintf(stderr, "already mmaped max size, now size=%d, max=%d\n", size_, mmap_file_option_.max_mmap_size_);
                return false;
            }

            int32_t new_size = size_ + mmap_file_option_.per_mmap_size_;
            if(new_size > mmap_file_option_.max_mmap_size_) {
                new_size = mmap_file_option_.max_mmap_size_;
            }

            if(!ensure_file_size(new_size)) {
                return false;
            }   

            if(DEBUG) fprintf(stdout, "mremap start, fd=%d, now_size=%d, new_size=%d, data=%p\n", fd_, size_, new_size, data_);

            void* new_data = mremap(data_, size_, new_size, MREMAP_MAYMOVE);
            if(MAP_FAILED == new_data) {
                fprintf(stderr, "mremap error: fd=%d, new_size=%d, error: %s",fd_, new_size, strerror(errno));
                return false;
            }

            if(DEBUG) fprintf(stdout, "mremap file successed, fd=%d, old_size=%d, now_size:%d, data=%p\n", fd_, size_, new_size, new_data);
            
            size_ = new_size;
            data_ = new_data;
            return true;
        }

        // 如果文件大小小于 size，则拓展到 size 大小
        bool MMapFile::ensure_file_size(const int32_t size) {
            struct stat s;
            if(fstat(fd_, &s) < 0) {
                fprintf(stderr, "fstat error, error desc: %s\n", strerror(errno));
                return false;
            }

            if(s.st_size < size) {
                if(ftruncate(fd_, size) < 0) {
                    fprintf(stderr, "ftruncate error, error desc: %s\n", strerror(errno));
                    return false;
                }
            }

            return true;
        }
    }
}