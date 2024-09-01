#include "index_handle.h"
#include <sstream>

static const int32_t debug = 1;

namespace tbfs
{
    namespace largefile
    {
        //初始化索引文件名和路径
        IndexHanle::IndexHanle(const std::string& base_path, const uint32_t main_block_id) {
            std::stringstream tmp_stream;
            tmp_stream << base_path << INDEX_DIR_PREFIX << main_block_id;

            std::string index_path;
            tmp_stream >> index_path;

            file_op_ = new MMapFileOperation(index_path, O_CREAT | O_RDWR | O_LARGEFILE);
            is_load_ = false;
        }

        IndexHanle::~IndexHanle() {
            if(file_op_) {
                delete file_op_;
                file_op_ = nullptr;
                is_load_ = false;
            }
        }

        //创建索引文件，初始化索引文件的基本信息并同步写入索引文件
        int IndexHanle::create(const uint32_t logic_block_id, const int32_t bucket_size, const MMapOption map_option) {
            if(debug) printf("create index, block id:%d, bucket size:%d, max_mmap_size:%d, first mmap size:%d, per mmap size:%d\n",
                logic_block_id, bucket_size, map_option.max_mmap_size_, map_option.first_mmap_size_, map_option.per_mmap_size_);

            if(is_load_) return EXIT_INDEX_ALREADY_LOADED_ERROR;

            int64_t file_size = file_op_->get_file_size();
            if(file_size < 0) {
                return TFS_FAILED;
            } else if(file_size == 0) {
                IndexHeader i_header;
                i_header.block_info_.block_id_ = logic_block_id;
                i_header.block_info_.seq_no_ = 1;   //下一个可用文件编号必须从 1 开始
                i_header.bucket_size_ = bucket_size;

                i_header.index_file_size_ = sizeof(IndexHeader) + bucket_size * sizeof(int32_t);    //计算索引大小
                //i_header.index_file_size_ = sizeof(IndexHeader) + bucket_size * sizeof(MetaInfo);    //计算索引大小

                //这时候索引文件大小为0，需要初始化好索引信息，并将其写入文件
                char* init_data = new char[i_header.index_file_size_];  //index header + total bucket
                memcpy(init_data, &i_header, sizeof(i_header));
                memset(init_data + sizeof(IndexHeader), 0, i_header.index_file_size_ - sizeof(IndexHeader));

                //write index header info into index file
                int ret = file_op_->pwrite_file(init_data, i_header.index_file_size_, 0);

                delete[] init_data;
                init_data = nullptr;

                if(ret != TFS_SUCCESS) {
                    fprintf(stderr, "create-pwrite_file error\n");
                    return ret;
                }

                ret = file_op_->flush_file();
                if(ret != TFS_SUCCESS) {
                    fprintf(stderr, "create-flush_file error\n");
                    return ret;
                }
            } else {    //file_size > 0 即索引文件已经存在，并存在索引数据
                return EXIT_META_UNEXPECT_FOUND_ERROR;
            }

            int ret = file_op_->mmap_file(map_option);
            if(ret != TFS_SUCCESS) {
                return ret;
            }

            is_load_ = true;

            if(debug) printf("create blockid:%d index successful. data file offset:%d, index file size:%d, bucket size:%d, free head offset:%d, seqno:%d, file_count:%d, del file size:%d, del file count:%d\n",
                logic_block_id, index_header()->data_file_offset_, index_header()->index_file_size_, 
                index_header()->bucket_size_, index_header()->free_head_offset_, block_info()->seq_no_,
                block_info()->file_count_, block_info()->del_size_, block_info()->del_file_count_);

            return TFS_SUCCESS;
        }

        //加载索引文件 - 将索引文件映射到内存
        int IndexHanle::load(const uint32_t logic_block_id, const int32_t bucket_size, const MMapOption map_option) {
            if(is_load_) return EXIT_INDEX_ALREADY_LOADED_ERROR;

            int64_t file_size = file_op_->get_file_size();
            if(file_size < 0) {
                return file_size;
            } else if(file_size == 0) {
                return EXIT_INDEX_CORRUPT_ERROR;
            }

            MMapOption tmp_map_option = map_option; //临时选项，用于修正传入的选项参数
            if(file_size > tmp_map_option.first_mmap_size_ && file_size <= tmp_map_option.max_mmap_size_) {
                tmp_map_option.first_mmap_size_ = file_size;    //将文件全部映射到内存
            }

            int ret = file_op_->mmap_file(tmp_map_option);
            if(ret != TFS_SUCCESS) {
                return ret;
            }

            if(0 == this->bucket_size() || 0 == block_info()->block_id_) {
                fprintf(stderr, "Index corrupt error. block id:%u, bucket size:%d\n", block_info()->block_id_, this->bucket_size());
                return EXIT_INDEX_CORRUPT_ERROR;
            }

            //check file size
            int32_t index_file_size = sizeof(IndexHeader) + this->bucket_size() * sizeof(int32_t);
            //int32_t index_file_size = sizeof(IndexHeader) + this->bucket_size() * sizeof(MetaInfo);
            if(file_size < index_file_size) {
                fprintf(stderr, "Index corrupt error, block id:%u, bucket size:%d, file_size:%ld, index file size:%d\n", block_info()->block_id_, this->bucket_size(), file_size, index_file_size);
                return EXIT_INDEX_CORRUPT_ERROR;
            }

            if(logic_block_id != block_info()->block_id_) {
                fprintf(stderr, "block id conflict. block id:%u, index block id:%d\n", logic_block_id, block_info()->block_id_);
                return EXIT_BLOCKID_CONFLICT_ERROR;
            }

            if(bucket_size != this->bucket_size()) {
                fprintf(stderr, "Index configure error, old bucket size:%d, new bucket size:%d\n", this->bucket_size(), bucket_size);
                return EXIT_BUCKET_CONFLICT_ERROR;
            }

            is_load_ = true;
            if(debug) printf("load blockid:%d index successful. data file offset:%d, index file size:%d, bucket size:%d, free head offset:%d, seqno:%d, file_count:%d, del file size:%d, del file count:%d, file_size:%d\n",
                logic_block_id, index_header()->data_file_offset_, index_header()->index_file_size_, 
                index_header()->bucket_size_, index_header()->free_head_offset_, block_info()->seq_no_,
                block_info()->file_count_, block_info()->del_size_, block_info()->del_file_count_, block_info()->size_t_);

            return TFS_SUCCESS;
        }

        int IndexHanle::remove(const uint32_t logic_block_id) {
            if(is_load_) {
                if(logic_block_id != block_info()->block_id_) {
                    fprintf(stderr, "block id conflict. block id:%d, index block id:%d\n", logic_block_id, block_info()->block_id_);
                    return EXIT_BLOCKID_CONFLICT_ERROR;
                }
            }

            int ret = file_op_->munmap_file();
            if(ret != largefile::TFS_SUCCESS) {
                return ret;
            }

            ret = file_op_->unlink_file();
            return ret;
        }

        int IndexHanle::flush() {
            int ret = file_op_->flush_file();
            if(TFS_SUCCESS != ret) {
                fprintf(stderr, "index flush fail, ret:%d, error desc:%s\n", ret, strerror(errno));
                return ret;
            }
            return ret;
        }

        int IndexHanle::hash_find(const uint64_t key, int32_t& current_offset, int32_t& previous_offset) {
            current_offset = 0;
            previous_offset = 0;
            MetaInfo meta_info;

            //1.确定 key 存放的桶（slot）的位置
            int32_t slot = static_cast<uint32_t>(key) % bucket_size();

            //2.读取哈希桶首节点存储的第一个节点的偏移量，如果偏移量为0，直接返回
            //3.根据偏移量读取存储的 metainfo
            //4.与key比较，相等则设置 current_offset 和 previous_offset，返回成功；否则执行步骤5
            //5.从meta中取得下一个节点在文件中的偏移量，如果偏移量为0，返回失败，否则执行步骤3
            int32_t pos = bucket_slot()[slot];

            while(pos != 0) {
                int ret = file_op_->pread_file(reinterpret_cast<char*>(&meta_info), sizeof(MetaInfo), pos);
                if(ret != TFS_SUCCESS) {
                    return ret;
                }

                if(hash_compare(key, meta_info.get_key())) {
                    current_offset = pos;
                    return TFS_SUCCESS;
                }

                previous_offset = pos;
                pos = meta_info.get_next_meta_offset();
            }

            return EXIT_META_NOT_FOUND_ERROR;
        }

        int IndexHanle::hash_insert(const uint64_t key, int32_t previous_offset, MetaInfo& meta) {
            int ret = TFS_SUCCESS;
            MetaInfo tmp_meta_info;
            int32_t current_offset = 0;

            //1.确定 key 存放的桶(slot)的位置
            int32_t slot = static_cast<uint32_t>(key) % bucket_size();

            //2.确定 meta 节点存储在文件中的偏移量
            if(get_free_offset() != 0) {
                ret = file_op_->pread_file(reinterpret_cast<char*>(&tmp_meta_info), sizeof(MetaInfo), get_free_offset());
                if(ret != TFS_SUCCESS) {
                    return ret;
                }
                current_offset = get_free_offset();
                if(debug) printf("reuse meta info, current_offset:%d\n", current_offset);
                index_header()->free_head_offset_ = tmp_meta_info.get_next_meta_offset();
            } else {
                current_offset = index_header()->index_file_size_;  //获取当前以写入数据最后位置的偏移量
                index_header()->index_file_size_ += sizeof(MetaInfo);
            }

            //3.将 meta 节点写入到所以文件中
            meta.set_next_meta_offset(0);   //指向下一个节点的偏移量置为 0

            ret = file_op_->pwrite_file(reinterpret_cast<const char*>(&meta), sizeof(MetaInfo), current_offset);
            if(ret != TFS_SUCCESS) {
                index_header()->index_file_size_ -= sizeof(MetaInfo);   //写入失败，回退末尾节点偏移量
                return ret;
            }

            //4.将 meta 插入到哈希链表之中
            if(0 != previous_offset) {  //存储前置节点
                ret = file_op_->pread_file(reinterpret_cast<char*>(&tmp_meta_info), sizeof(MetaInfo), previous_offset);
                if(ret != TFS_SUCCESS) {
                    index_header()->index_file_size_ -= sizeof(MetaInfo);
                    return ret;
                }

                tmp_meta_info.set_next_meta_offset(current_offset);
                ret = file_op_->pwrite_file(reinterpret_cast<const char*>(&tmp_meta_info), sizeof(MetaInfo), previous_offset);
                if(ret != TFS_SUCCESS) {
                    index_header()->index_file_size_ -= sizeof(MetaInfo);   //写入失败，回退末尾节点偏移量
                    return ret;
                }
            } else {    //该键值还没有节点，即当前插入的节点为第一个节点
                bucket_slot()[slot] = current_offset;   //数据已经写入，更新指向即可
            }

            return TFS_SUCCESS;
        }

        int IndexHanle::write_segment(const uint64_t key, MetaInfo &meta) {
            int32_t current_offset = 0, previous_offset = 0;
            
            //1.从文件哈希表中查找key是否存在 hash_find(key, current_offset, previous_offset);
            int ret = hash_find(key, current_offset, previous_offset);
            if(TFS_SUCCESS == ret) {
                return EXIT_META_UNEXPECT_FOUND_ERROR;
            } else if(EXIT_META_NOT_FOUND_ERROR != ret) {
                return ret;
            }

            //2.不存在就写入 meta 到文件哈希表中，hash_insert(key, previous_offset, meta);
            ret = hash_insert(key, previous_offset, meta);
            return ret;
        }

        int32_t IndexHanle::read_segment(const uint64_t key, MetaInfo& meta) {
            int32_t current_offset = 0, previous_offset = 0;
             
            //从文件哈希表中查找key是否存在 hash_find(key, current_offset, previous_offset);
            int32_t ret = hash_find(key, current_offset, previous_offset);
            if(TFS_SUCCESS == ret) {
                ret = file_op_->pread_file(reinterpret_cast<char*>(&meta), sizeof(MetaInfo), current_offset);
                return ret;
            } 
            
            return ret;
        }

        int32_t IndexHanle::del_segment(const uint64_t key) {
            int32_t current_offset = 0, previous_offset = 0;
             
            //从文件哈希表中查找key是否存在 hash_find(key, current_offset, previous_offset);
            int32_t ret = hash_find(key, current_offset, previous_offset);
            if(TFS_SUCCESS != ret) {
                return ret;
            }

            MetaInfo meta;
            ret = file_op_->pread_file(reinterpret_cast<char*>(&meta), sizeof(MetaInfo), current_offset);
            if(TFS_SUCCESS != ret) {
                return ret;
            }

            int32_t next_pos = meta.get_next_meta_offset();
            if(previous_offset == 0) {  //要删除的索引节点是首节点
                int32_t slot = static_cast<uint32_t>(key) % bucket_size();
                bucket_slot()[slot] = next_pos;
            } else {
                MetaInfo prev_meta;
                ret = file_op_->pread_file(reinterpret_cast<char*>(&prev_meta), sizeof(MetaInfo), previous_offset);
                if(TFS_SUCCESS != ret) {
                    return ret;
                }

                prev_meta.set_next_meta_offset(next_pos);
                ret = file_op_->pwrite_file(reinterpret_cast<const char*>(&prev_meta), sizeof(MetaInfo), previous_offset);
                if(ret != TFS_SUCCESS) {
                    return ret;
                }
            }

            //设置可重用节点
            meta.set_next_meta_offset(get_free_offset());
            ret = file_op_->pread_file(reinterpret_cast<char*>(&meta), sizeof(MetaInfo), current_offset);
            if(TFS_SUCCESS != ret) {
                return ret;
            }

            index_header()->free_head_offset_ = current_offset;
            if(debug) printf("del_segment--reuse meta info, current_offset:%d\n", current_offset);

            ret = update_block_info(C_OPER_DELETE, meta.get_size());
            return ret;
        }

        int IndexHanle::update_block_info(const OperType oper_type, const uint32_t modify_size) {
            if(block_info()->block_id_ == 0) return EXIT_BLOCKID_ZERO_ERROR;

            if(oper_type == C_OPER_INSERT) {
                ++block_info()->version_;
                ++block_info()->file_count_;
                ++block_info()->seq_no_;
                block_info()->size_t_ += modify_size;
            } else if(oper_type == C_OPER_DELETE) {
                ++block_info()->version_;
                --block_info()->file_count_;
                block_info()->size_t_ -= modify_size;
                ++block_info()->del_file_count_;
                block_info()->del_size_ += modify_size;
            }

            if(debug) printf("update block info. blockid:%d, version:%d, file_count:%d, size:%d, del_fel_count:%d, del_size:%d, seq_no:%d\n",
                block_info()->block_id_, block_info()->version_, block_info()->file_count_, block_info()->size_t_,
                block_info()->del_file_count_, block_info()->del_size_, block_info()->seq_no_);

            return TFS_SUCCESS;
        }
    }
}