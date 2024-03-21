#ifndef _MMAP_FILE_OP_H_
#define _MMAP_FILE_OP_H_

#include "common.h"
#include "file_op.h"
#include "mmap_file.h"

namespace tbfs
{
    namespace largefile
    {
        class MMapFileOperation : public FileOperation
        {
        public:
            MMapFileOperation(const std::string& file_name, const int open_flags = O_CREAT | O_RDWR | O_LARGEFILE)
                : FileOperation(file_name, open_flags), map_file_(NULL), is_mapped_(false) {

            }

            ~MMapFileOperation() {
                if(map_file_) {
                    delete(map_file_);
                    map_file_ = NULL;
                    is_mapped_ = false;
                }
            }

            int pread_file(char* buf, const int32_t size, const int64_t offset);
            int pwrite_file(const char* buf, const int32_t size, const int64_t offset);

            int mmap_file(const MMapOption& mmap_operation);
            int munmap_file();

            void* get_map_data() const;
            int flush_file();

        private:
            MMapFile* map_file_;
            bool is_mapped_;
        };
    }
}


#endif