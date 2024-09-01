#include "file_op.h"
#include "common.h"

#define DEBUG 1

namespace tbfs{
    namespace largefile
    {
        FileOperation::FileOperation(const std::string& file_name, const int open_flags)
            : fd_(-1), open_flags_(open_flags) {
            file_name_ = strdup(file_name.c_str());
        }
        
        FileOperation::~FileOperation() {
            if(fd_ > 0) {
                ::close(fd_);
            }
            if(NULL != file_name_) {
                free(file_name_);
                file_name_ = NULL;
            }
        }

        int FileOperation::open_file() {
            if(fd_ > 0) {
                ::close(fd_);
                fd_ = -1;
            }

            fd_ = ::open(file_name_, open_flags_, OPEN_MODE);
            if(fd_ < 0) {
                return -errno;
            }

            return fd_;
        }
        
        void FileOperation::close_file() {
            if(fd_ < 0) {
                return;
            }

            ::close(fd_);
            fd_ = -1;
        }
        
        int FileOperation::flush_file() {
            if(open_flags_ & O_SYNC) return 0;  //同步的方式，当数据写到磁盘时，才会返回，无需再次同步
            int fd = check_file();
            if(fd < 0) return fd;

            return fsync(fd);
        }
        
        int FileOperation::unlink_file() {
            close_file();

            return ::unlink(file_name_);
        }

        int FileOperation::pread_file(char* buf, const int32_t nbytes, const int64_t offset) {
            int32_t left = nbytes;  //剩余的字节数
            int64_t read_offset = offset;   //起始位置的偏移量
            int32_t read_len = 0;   //已经读取的字节数
            char* p_tmp = buf;      //读取的内容

            int i = 0;  //记录读取尝试的次数
            while(left > 0) {
                ++i;
                if(i > MAX_DISK_TIMES) break;
                
                if(check_file() < 0) return -errno;

                read_len = ::pread64(fd_, p_tmp, left, read_offset);
                if(read_len < 0) {
                    if(errno == EINTR || EAGAIN == errno) {
                        continue;;
                    } else if(EBADE == errno){
                        fd_ = -1;
                        return errno;
                    } else {
                        return read_len;
                    }
                } else if(read_len == 0) {
                    break;
                }

                left -= read_len;
                p_tmp += read_len;
                read_offset =+ read_len;
            }

            if(0 != left && read_len != 0) {
                return EXIT_DISK_INCOMPLETE;
            }
            return TFS_SUCCESS;
        }

        int FileOperation::pwrite_file(const char* buf, const int32_t nbytes, const int64_t offset) {
            int32_t left = nbytes;  //剩余的字节数
            int64_t write_offset = offset;   //起始位置的偏移量
            int32_t written_len = 0;   //已经写的字节数
            const char* p_tmp = buf;      //要写的内容

            int i = 0;  //记录写入尝试的次数
            while(left > 0) {
                ++i;
                if(i > MAX_DISK_TIMES) break;
                
                if(check_file() < 0) return -errno;

                written_len = ::pwrite64(fd_, p_tmp, left, write_offset);
                if(written_len < 0) {
                    if(errno == EINTR || EAGAIN == errno) {
                        continue;;
                    } else if(EBADE == errno){
                        fd_ = -1;
                        return errno;
                    } else {
                        return written_len;
                    }
                }

                left -= written_len;
                p_tmp += written_len;
                write_offset =+ written_len;
            }

            if(0 != left) {
                return EXIT_DISK_INCOMPLETE;
            }
            return TFS_SUCCESS;
        }

        int FileOperation::write_file(const char* buf, const int32_t nbytes) {
            int32_t left = nbytes;  //剩余的字节数
            int32_t written_len = 0;   //已经写的字节数
            const char* p_tmp = buf;      //要写的内容

            int i = 0;  //记录写入尝试的次数
            while(left > 0) {
                ++i;
                if(i > MAX_DISK_TIMES) break;
                
                if(check_file() < 0) return -errno;

                written_len = ::write(fd_, p_tmp, left);
                if(written_len < 0) {
                    if(errno == EINTR || EAGAIN == errno) {
                        continue;;
                    } else if(EBADE == errno){
                        fd_ = -1;
                        return errno;
                    } else {
                        return written_len; 
                    }
                }

                left -= written_len;
                p_tmp += written_len;
            }

            if(0 != left) {
                return EXIT_DISK_INCOMPLETE;
            }
            return TFS_SUCCESS;
        }

        int FileOperation::ftruncate_file(const int64_t length) {
            int fd = check_file();
            if(fd < 0) return fd;

            return ftruncate(fd, length);
        }

        int FileOperation::seek_file(const int64_t offset) {
            int fd = check_file();
            if(fd < 0) return fd;

            return lseek(fd, offset, SEEK_SET);
        }

        int64_t FileOperation::get_file_size() {
            struct stat s;
            int fd = check_file();
            if(fd < 0) return -1;

            if(fstat(fd, &s) != 0) {
                return -1;
            }

            return s.st_size;
        }

        int FileOperation::get_fd() const {
            return fd_;
        }

        int FileOperation::check_file(){
            if(fd_ < 0) {
                return open_file();
            }

            return fd_;
        }
    }
}