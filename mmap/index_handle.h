#ifndef _INDEX_HANDLE_H_
#define _INDEX_HANDLE_H_

#include "common.h"
#include "mmap_file_op.h"

namespace tbfs
{
    namespace largefile
    {
        struct IndexHeader
        {
            IndexHeader() {
                memset(this, 0, sizeof(IndexHeader));
            }

            BlockInfo block_info_;          //块信息
            int32_t bucket_size_;           //哈希桶大小
            int32_t data_file_offset_;      //未使用数据的起始偏移位置
            int32_t index_file_size_;       //索引文件当前偏移
            int32_t free_head_offset_;      //可重用空间的链表节点
        };

        class IndexHanle
        {
        public:
            IndexHanle(const std::string& base_path, const uint32_t main_block_id);
            ~IndexHanle();

            IndexHeader* index_header() {
                return reinterpret_cast<IndexHeader*>(file_op_->get_map_data());
            }

            BlockInfo* block_info() const {
                //return reinterpret_cast<IndexHeader*>(file_op_->get_map_data())->block_info;
                return reinterpret_cast<BlockInfo*>(file_op_->get_map_data());  //因为block_info_ 就在IndexHeader的首地址
            }

            int32_t bucket_size() const {
                return reinterpret_cast<IndexHeader*>(file_op_->get_map_data())->bucket_size_;
            }

            int32_t get_block_data_offset() const {
                return reinterpret_cast<IndexHeader*>(file_op_->get_map_data())->data_file_offset_;
            }

            int32_t get_free_offset() const {
                return reinterpret_cast<IndexHeader*>(file_op_->get_map_data())->free_head_offset_;
            }

            void commit_block_data_offset(const int64_t file_size) {
                reinterpret_cast<IndexHeader*>(file_op_->get_map_data())->data_file_offset_ += file_size;
            }

            int32_t* bucket_slot() {
                //起始位置 + 首部 就能得到哈希索引桶数组的起始位置
                return reinterpret_cast<int32_t*>(reinterpret_cast<char*>(file_op_->get_map_data()) + sizeof(IndexHeader));
            }

            int create(const uint32_t logic_block_id, const int32_t bucket_size, const MMapOption map_option);
            int load(const uint32_t logic_block_id, const int32_t bucket_size, const MMapOption map_option);

            int remove(const uint32_t logic_block_id);
            int flush();

            int hash_find(const uint64_t key, int32_t& current_offset, int32_t& previous_offset);
            int hash_insert(const uint64_t key, int32_t previous_offset, MetaInfo& meta);
            int write_segment(const uint64_t key, MetaInfo &meta);
            int32_t read_segment(const uint64_t key, MetaInfo& meta);
            int32_t del_segment(const uint64_t key);
            int update_block_info(const OperType oper_type, const uint32_t modify_size);

        private:
            bool hash_compare(const uint64_t key_left, const uint64_t key_right) {
                return key_left == key_right;
            }
        private:
            MMapFileOperation* file_op_;
            bool is_load_;
        };
    }
}

#endif