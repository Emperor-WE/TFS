#ifndef _COMMON_H_
#define _COMMON_H_

#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

namespace tbfs
{
    namespace largefile
    {
        const int32_t TFS_SUCCESS = 0;  //success
        const int32_t TFS_FAILED = -1;  //failed 
        const int32_t EXIT_DISK_INCOMPLETE = -8012; //read or write length is less than required
        const int32_t EXIT_INDEX_ALREADY_LOADED_ERROR = -8013;  //index is loaded when create or load
        const int32_t EXIT_META_UNEXPECT_FOUND_ERROR = -8014;   //meta found in index when insert
        const int32_t EXIT_INDEX_CORRUPT_ERROR = -8015;     //index is corrupt
        const int32_t EXIT_BLOCKID_CONFLICT_ERROR = -8016;
        const int32_t EXIT_BUCKET_CONFLICT_ERROR = -8017;
        const int32_t EXIT_META_NOT_FOUND_ERROR = -8018;
        const int32_t EXIT_BLOCKID_ZERO_ERROR = -8019;

        static const std::string MAINBLOCK_DIR_PREFIX = "/mainblock/";
        static const std::string INDEX_DIR_PREFIX = "/index/";
        static const mode_t DIR_MODE = 0755;

        struct MMapOption
        {
            int32_t max_mmap_size_;     //最大映射大小
            int32_t first_mmap_size_;   //初始映射大小
            int32_t per_mmap_size_;     //每一次追加时，所追加的映射大小
        };

        typedef enum OperType
        {
            C_OPER_INSERT = 1,
            C_OPER_DELETE
        }OperType;

        struct BlockInfo
        {
            uint32_t block_id_;         //块编号
            int32_t version_;           //块当前版本号
            int32_t file_count_;        //当前已保存文件总数
            int32_t size_t_;            //当前已保存文件数据总大小
            int32_t del_file_count_;    //已删除的文件数量
            int32_t del_size_;          //已删除的文件数据总大小
            uint32_t seq_no_;           //下一个可分配的文件编号

            BlockInfo() {
                memset(this, 0, sizeof(BlockInfo));
            }

            inline bool operator==(const BlockInfo& rhs) const {
                return block_id_ == rhs.block_id_ && version_ == rhs.version_
                    && file_count_ == rhs.file_count_ && size_t_ == rhs.size_t_
                    && del_file_count_ == rhs.del_file_count_ 
                    && del_size_ == rhs.del_size_ && seq_no_ == rhs.seq_no_;
            }
        };

        struct MetaInfo
        {
        public:
            MetaInfo() {
                init();
            }

            MetaInfo(const uint64_t file_id, const int32_t inner_offset, const int32_t size, const int32_t next_mate_offset) {
                file_id_ = file_id;
                location_.inner_offset_ = inner_offset;
                location_.size_ = size;
                next_meta_offset_ = next_mate_offset;
            }

            MetaInfo(const  MetaInfo& rhs) {
                memcpy(this, &rhs, sizeof(MetaInfo));
            }

            MetaInfo& operator=(const MetaInfo& rhs) {
                if(this == &rhs) return *this;
                
                file_id_ = rhs.file_id_;
                location_.inner_offset_ = rhs.location_.inner_offset_;
                location_.size_ = rhs.location_.size_;
                next_meta_offset_ = rhs.next_meta_offset_;
                
                return *this;
            }

            MetaInfo& clone(const MetaInfo& rhs) {
                assert(this != &rhs);
                
                file_id_ = rhs.file_id_;
                location_.inner_offset_ = rhs.location_.inner_offset_;
                location_.size_ = rhs.location_.size_;
                next_meta_offset_ = rhs.next_meta_offset_;
                
                return *this;
            }

            bool operator==(const MetaInfo& rhs) {
                return file_id_ == rhs.file_id_ && location_.inner_offset_ == rhs.location_.inner_offset_
                    && location_.size_ == rhs.location_.size_ && next_meta_offset_ == rhs.next_meta_offset_;
            }

            uint64_t get_key() const {
                return file_id_;
            }

            void set_key(const uint64_t key) {
                file_id_ = key;
            }

            uint64_t get_file_id() const {
                return file_id_;
            }

            void set_file_id(const uint64_t key) {
                file_id_ = key;
            }

            int32_t get_offset() const {
                return location_.inner_offset_;
            }

            void set_offset(const int32_t offset) {
                location_.inner_offset_ = offset;
            }

            int32_t get_size() const {
                return location_.size_;
            }

            void set_size(const int32_t size) {
                location_.size_ = size;
            }

            int32_t get_next_meta_offset() const {
                return next_meta_offset_;
            }

            void set_next_meta_offset(const int32_t offset) {
                next_meta_offset_ = offset;
            }

        private:
            uint64_t file_id_;          //文件编号

            struct {                    //文件元数据
                int32_t inner_offset_;  //文件在块内部的偏移量
                int32_t size_;          //文件大小
            } location_;

            int32_t next_meta_offset_;  //当前哈希链下一个节点再索引文件中的偏移量
        
        private:
            void init() {
                memset(this, 0, sizeof(MetaInfo));
            }
        };
    }
}

#endif