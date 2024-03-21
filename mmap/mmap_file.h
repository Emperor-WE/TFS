#ifndef _MMAP_FILE_H_
#define _MMAP_FILE_H_

#include <unistd.h>
#include <cstdint>
#include <sys/mman.h>
#include "common.h"

namespace tbfs
{
    namespace largefile
    {
        class MMapFile
        {
        public:
            MMapFile();

            explicit MMapFile(const int fd);
            MMapFile(const MMapOption& mmap_option, const int fd);

            ~MMapFile();

            bool sync_file();   //同步文件
            bool map_file(const bool wirte = false);    //设置映射内存的访问权限，并将文件映射到内存
            void* get_data() const;     //获取映射到内存数据的首地址
            int32_t get_size() const;   //获取映射数据的大小

            bool munmap_file();     //解除文件映射
            bool remap_file();      //重新执行映射

        private:
            bool ensure_file_size(const int32_t size);  //映射内存区域大小的扩展

        private:
            int32_t size_;  //映射区域的大小
            int fd_;    //所要映射的文件
            void* data_; //映射到内存后，对应的映射内存首地址

            struct MMapOption mmap_file_option_;    //映射选项
        };
    }
}

#endif