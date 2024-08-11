### 引入

##### 文件结构（Ext格式化分区）

> 文件 = 目录项 + 块文件数据 + Inode元信息
>
> ​		**目录项区** - 存放目录下文件的列表信息
>
> ​		**数据区**  - 存放文件数据
>
> ​		**inode区**（inode table） - 存放inode所包含的元信息，操作系统用 inode 号码来识别不同的文件。

##### 系统读取文件三部曲

> 1. 逐级遍历目录，查找文件，取得Inode number。
>
> 2. 根据 Inode number 查询 Inode table 取得 Inode 信息。
>
> 3. 根据 Inode 信息确定文件存储的位置（Blocks）。

##### 淘宝不适用小文件存储原因

> 1. 大规模的小文件存取，磁头需要频繁的寻道和换道，因此在读取上容易带来较长的延时。
>
> 2. 频繁的新增删除操作导致磁盘碎片，降低磁盘利用率和IO读写效率。
>
> 3. inode 占用了大量的磁盘空间，降低了缓存到内存的效果。将导致多次的磁盘内存swap。

### 大文件结构

##### 设计思路

> 1. 以 block 文件的形式存放数据文件，每个块都有唯一的一个整数编号，块在使用之前所用到的存储空间都会预先分配和初始化。(一般64M一个block)
> 2. 每一个块由一个索引文件、一个主块文件和若干个扩展块组成，“小文件”主要存放在主块中，扩展块主要用来存放溢出的数据。
> 3. 每个索引文件存放对应的 块信息 和 “小文件” 索引信息，索引文件会在服务启动时映射到内存，以便极大的提高文件检索速度。“小文件”索引信息采用在索引文件中的 数据结构哈希链表 来实现。
> 4. 每个文件有对应的文件编号，文件编号从1开始编号，依次递增，同时作为哈希查找算法的Key 来定位“小文件”在主块和扩展块中的偏移量。文件编号+块编号按某种算法可得到“小文件”对应的文件名。

##### 结构图

> 目录结构
>
> <img src="./assets/image-20231230151629088.png" alt="image-20231230151629088" style="zoom:29%;" />
>
> 大文件存储结构图
>
> <img src="./assets/image-20231230151001460.png" alt="image-20231230151001460" style="zoom: 25%;" />
>
> 块信息关键数据结构
>
> ```c++
> struct BlockInfo
> {
>       uint32_t block_id_;           	 //块编号   1 ......2^32-1  TFS = NameServer + DataServer
>       int32_t version_;             	 //块当前版本号
>       int32_t file_count_;            	 //当前已保存文件总数
>       int32_t size_;                     //当前已保存文件数据总大小
>       int32_t del_file_count_;           //已删除的文件数量
>       int32_t del_size_;              	 //已删除的文件数据总大小
>       uint32_t seq_no_;             	 //下一个可分配的文件编号  1 ...2^64-1    
> };
> ```
>
> 文件哈希链表实现图
>
> <img src="./assets/image-20231230192809730.png" alt="image-20231230192809730" style="zoom:25%;" />
>
> 索引元信息数据结构
>
> ```c++
> struct RawMeta {
>      struct
>      {
>         int32_t inner_offset_;       //文件在块内部的偏移量
>         int32_t size_;               //文件大小
>      } location_;
> };
> 
> struct MetaInfo{
>      uint64_t fileid_;                //文件编号
>      RawMeta raw_meta_;               //文件元数据
>      int32_t next_meta_offset_;       //当前哈希链下一个节点在索引文件中的偏移量
> };
> ```

##### 块初始化

> 1. 生成主块文件
>    1. 根据 id 创建主块文件
>    2. 预分配空间
> 2. 生成索引文件
>    1. 根据 id 生成索引文件
>    2. 头部信息初始化（块信息初始化 + 索引信息初始化）
>    3. 同步写入磁盘
>    4. 映射至内存访问

##### 块中写入文件

> 1. 加载索引文件
>    1. 映射索引文件到内存
> 2. 文件信息写入主块
>    1. 从索引文件中读取块数据偏移
>    2. 将文件写入主块对应的偏移位置中
> 3. 文件索引信息写入索引文件
>    1. 生成 MetaInfo 信息（包括文件在块中的fd）
>    2. 将 MetaInfo 写入索引文件
>    3. 跟新块信息 BlockInfo

##### 块中读取文件

> 1. 加载索引文件
>    1. 映射索引文件到内存
> 2. 从索引文件中获取文件 MetaInfo
>    1. 根据文件 id 从块索引内存映射的 Hash文件链表 中查找文件的 MetaInfo
> 3. 根据 MetaInfo 从主块中读取文件

##### 块中删除

> 1. 加载索引文件
>    1. 映射索引文件至内存
> 2. 从索引文件中获取文件 MetaInfo
>    1. 根据文件 id 从块索引内存映射的 Hash文件链表 中查找文件的 MetaInfo
> 3. 索引文件中删除 MetaInfo
>    1. 将当前文件对应 MetaInfo 在哈希链表中的上一个节点指向其后的节点
>    2. 当前 MetaInfo 加入可重用空闲节点链表中
>       1. 更新																																																																																																																																																																																														块信息

##### 设计类图

> <img src="./assets/image-20231230161937822.png" alt="image-20231230161937822" style="zoom: 33%;" />

具体实现

> * read 函数返回值处理
>
>   | **错误信息**  | **原因**                                         | **处理方法**                             |
>   | ------------- | ------------------------------------------------ | ---------------------------------------- |
>   | `EINTR`       | 系统调用被信号中断。                             | 重新调用 `read` 函数。                   |
>   | `EAGAIN`      | 文件描述符为非阻塞文件描述符，并且没有数据可读。 | 等待一段时间，然后重新调用 `read` 函数。 |
>  | `EWOULDBLOCK` | 与 `EAGAIN` 相同。                               | 等待一段时间，然后重新调用 `read` 函数。 |
>   | `EBADF`       | 文件描述符无效。                                 | 检查文件描述符是否有效。                 |
>  | `EINVAL`      | 传递给 `read` 函数的参数无效。                   | 检查参数是否有效。                       |
>   | `EFAULT`      | 传递给 `read` 函数的缓冲区地址无效。             | 检查缓冲区地址是否有效。                 |
>   | `ENOMEM`      | 内存不足。                                       | 释放一些内存，然后重新调用 `read` 函数。 |
>   | `ENOSPC`      | 没有足够的空间来写入数据。                       | 检查是否有足够的空间来写入数据。         |
> 
> 

> ```c++
> char *strdup(const char *s)
>  The  strdup()  function returns a pointer to a new string which is a duplicate of the string s.  Memory for the new string is obtained with malloc, and can be freed with free.
>     
> int msync(void *__addr, size_t __len, int __flags);
> 	flags: 
> 		MS_SYNC ：会强制将被映射区中发生的更改立即写入到磁盘，并且会一直等待直到写入操作完成。这意味着，msync 函数会阻塞当前进程，直到所有的数据都被成功写入磁盘。
>         MS_ASYNC ：将被内存映射区修改的数据标记为 "脏"，然后立即返回，而不会等待数据写入磁盘完成。这意味着 msync 函数不会阻塞当前进程，它只是将数据的写入操作放入到操作系统的写缓冲区，然后立即返回，使得程序可以继续执行后续操作。
>             
> void *mremap(void *__addr, size_t __old_len, size_t __new_len, int __flags, ...) noexcept(true);
> 	__addr：
>         指向要重新映射的内存区域的起始地址。
>     __old_len：
>         	重新映射区域的原始大小。
>     __new_len：
>         重新映射区域的新大小。
>     flags：
>         标志参数，用于指定重新映射操作的行为和选项。常用的标志有：
>         MREMAP_MAYMOVE：如果无法扩展原始映射区域，则创建一个新的区域并将其移动到新的地址范围。如果没有设置此标志，则原始映射区域必须已经包含足够的空间来进行大小调整。
>         MREMAP_FIXED：指定重新映射区域的新地址，而不是让操作系统自动选择一个地址。
> ```

### IndexHandle

* 构造函数 - IndexHandle

  * ```c
    1. 构造索引文件的路径和索引文件的块文件名
    2. 利用上面拼接的文件名构造 MMapFileOperatio 类型的成员 file_op_
    ```

* 创建索引文件 - create

  * ```c
    1. 如果索引文件已经加载，退出
    2. 获取索引文件大小，并分别处理
    	a) 文件大小 < 0，返回 TFS_FAILED
    	b) 文件大小 > 0，说明索引文件存在，并且索引数据信息也已经存在，无需创建，直接映射到内存即可
    	c) 文件大小 == 0 （文件已经创建，但是基本信息并未初始化）
    		1.初始化索引头部IndexHeader基本信息（快编号--传入、哈希桶大小--传入、下一个可分配文件编号--必须从1开始）
    		2.计算索引大小，即当前可使用索引位置的偏移量: sizeof(IndexHeader) + bucket_size * sizeof(int32_t)
             3.向索引文件写入头部IndexHeader信息（在前 sizeof(IndexHeader) 字节写入第 1 & 2 初始化的信息，同时将后面哈希桶大小的位置清 0）
             4.写入后，通过 fsync() 方法将信息同步到磁盘。
             5.将索引文件映射到内存（测试索引文件是否初始化成功）
    ```

* 加载索引文件 - load

  * ```c
    1.如果索引文件已经加载，退出
    2.检测索引文件大小（==0 - 未初始化，< 0 - 异常，都退出）
    3.依据传入的选项参数将索引文件映射到内存(选项参数的值不合理的话，会进行调整)
    4.将索引文件映射到内存，并逐一验证头部信息（块编号是否正确、哈希桶大小是否正确、索引文件实际大小是否大于理论大小--理论大小是指依据传入的哈希桶大小重新计算的索引文件大小）
    ```

* 删除文件 - remove

  * ```c
    1.解除映射
    2.删除文件 - unlink
    ```

* 查找小文件是否存在 - hash_find

  * ```c
    int IndexHanle::hash_find(const uint64_t key, int32_t& current_offset, int32_t& previous_offset);
    
    1. 确定 key 存放的桶（slot）的位置
        int32_t slot = static_cast<uint32_t>(key) % bucket_size();
    2. 读取哈希桶首节点存储的第一个节点的偏移量，如果偏移量为0，直接返回
    3. 根据偏移量读取存储的 metainfo
    4. 与key比较，相等则设置 current_offset 和 previous_offset，返回成功；否则执行步骤5
    5. 从meta中取得下一个节点在文件中的偏移量，如果偏移量为0，返回失败，否则执行步骤3
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
    ```

* 插入索引文件信息 - hash_insert

  * ```c
    int IndexHanle::hash_insert(const uint64_t key, int32_t previous_offset, MetaInfo& meta);
    
    1.确定 key 存放的桶(slot)的位置
    2.确定 meta 节点存储在文件中的偏移量(优先考虑可重用的空间节点，没有的话就选择 '索引文件当前偏移')
    3.将 meta 节点写入到所以文件中(插入失败的话，还需要进行节点回滚)
    4.将 meta 插入到哈希链表之中（尾插，前置节点要指向 meta）
    ```

* 写入节点元信息 - write_segment

  * ```c
    1.从文件哈希表中查找key是否存在 hash_find(key, current_offset, previous_offset);
    
    2.不存在就写入 meta 到文件哈希表中，hash_insert(key, previous_offset, meta);
    ```

* 读取指定节点信息 - read_segment

  * ```
    从文件哈希表中查找key是否存在 hash_find(key, current_offset, previous_offset); 存在则读取
    ```

* 删除指定节点信息 - del_segment

  * ```c
    1.从文件哈希表中查找key是否存在 hash_find(key, current_offset, previous_offset);
    2.存在则将该节点读取出来（存储到 meta 中）,通过前驱节点 previous_offset 判断要删除的节点是否为首节点
    	a) 要删除的索引节点是首节点：bucket_slot()[slot] = meta.get_next_meta_offset();
    	b) 读取要删除节点的前驱节点prev，将prev.next_meta_offset 指向要删除节点meta的下一个节点
    ```

* 更新块信息 - update_block_info

  * ```
    依据是删除节点还是插入节点做不同的处理，分别更新 版本号、已保存的文件总数、下一个可分配的文件编号、已删除的文件数量、已保存的文件总大小、已删除的文件数据总大小 索引等头部基本信息
    ```



### 写入调用

1. 加载索引文件

   ```c
   //1.指定索引块文件
   largefile::IndexHanle* index_handle = new largefile::IndexHanle(".", block_id); //索引文件句柄
   
   //2.加载索引文件到内存
   index_handle->load(block_id, bucket_size, mmap_option);
   ```

2. 写入数据到主块文件

   ```c
   //1.获取主块句柄
   largefile::FileOperation* mainblock = new largefile::FileOperation(mainblock_path, O_CREAT | O_RDWR | O_LARGEFILE);
   
   //2.将数据写入到主块文件
   mainblock->pwrite_file(buffer, sizeof(buffer), data_offset)
   ```

3. 索引文件写入 MetaInfo

   ```
   //初始化后 meta 的基本信息之后，写入 索引文件
   index_handle->write_segment(meta.get_key(), meta);
   ```

   



### 注意事项

##### 内存映射

* mmap 方法的 prot 和 flags 两个参数的顺序写反了的时候，文件会映射成功，但是文件映射的大小区域是0.后续对映射区域的写操作会导致 段错误。

##### 文件读写

* 当`write`和`read`操作的是同一个文件时，它们会共享文件偏移量。当你在不同的文件描述符上使用`write`和`read`函数时，它们的文件偏移量是相互独立的。
* `pread` 和 `pwrite` 用于带偏移量从文件中读取/写入数据，相当于顺序调用 lseek 和 read/write 方法，但**<font color="red">不更新文件的指针</font>**。