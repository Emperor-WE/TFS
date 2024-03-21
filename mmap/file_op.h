#ifndef _FILE_OP_H_
#define _FILE_OP_H_

#include "common.h"

namespace tbfs
{
    namespace largefile
    {
        class FileOperation
        {
        public:
            FileOperation(const std::string& file_name, const int open_flags = O_RDWR | O_CREAT | O_LARGEFILE);
            ~FileOperation();

            int open_file();
            void close_file();
            virtual int flush_file();   //将文件缓存写入磁盘
            int unlink_file();  //删除文件

            /*读取 nbytes 字节得文件内容到 buf 中，文件的其实偏移位置为 offset*/
            virtual int pread_file(char* buf, const int32_t nbytes, const int64_t offset);
            virtual int pwrite_file(const char* buf, const int32_t nbytes, const int64_t offset);

            int write_file(const char* buf, const int32_t nbytes);  //没有偏移位置的写文件

            int ftruncate_file(const int64_t length);   //将文件长度截断为 length
            int seek_file(const int64_t offset);    //移动文件指针

            int64_t get_file_size();
            int get_fd() const;

        protected:
            int check_file();
        protected:
            int fd_;
            int open_flags_;
            char* file_name_;
        protected:
            static const mode_t OPEN_MODE = 0644;
            static const int MAX_DISK_TIMES = 5;    //最大磁盘读取次数，因为有时候可能读取失败 
        };
    }
}

#endif