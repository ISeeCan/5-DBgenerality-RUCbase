# RUC 数据库系统概论

- [RUC 数据库系统概论](#ruc-数据库系统概论)
  - [前置](#前置)
  - [实验一](#实验一)
    - [实验说明](#实验说明)
      - [任务一 缓冲池管理器](#任务一-缓冲池管理器)
        - [任务1.0 模块逻辑说明](#任务10-模块逻辑说明)
        - [任务1.1 磁盘存储管理器](#任务11-磁盘存储管理器)
        - [任务1.2 缓冲池替换策略](#任务12-缓冲池替换策略)
        - [任务1.3 缓冲池管理器](#任务13-缓冲池管理器)
  - [实验二](#实验二)
    - [实验目标](#实验目标)

## 前置

具体项目安装配置见个人仓库[README](https://github.com/ISeeCan/5-DBgenerality-RUCbase)，官方项目仓库见[链接](https://github.com/ISeeCan/5-DBgenerality-RUCbase)

## 实验一

### 实验说明

[详细说明](https://github.com/ISeeCan/5-DBgenerality-RUCbase/blob/main/docs/Rucbase-Lab1%5B%E5%AD%98%E5%82%A8%E7%AE%A1%E7%90%86%E5%AE%9E%E9%AA%8C%E6%96%87%E6%A1%A3%5D.md)

#### 任务一 缓冲池管理器

##### 任务1.0 模块逻辑说明

- **BufferPoolManager** 是核心协调器，负责高效使用内存缓存。
- **DiskManager** 提供页面的磁盘读写接口，所有持久化操作依赖于它。
- **LRUReplacer** 提供替换策略，用于决定淘汰哪一页。

三者通过缓冲池（pages_）、页表（page_table_）和替换列表（free_list_）有机结合，共同实现数据库缓冲池的管理功能。

##### 任务1.1 磁盘存储管理器

需要完成[DiskManager](https://github.com/ISeeCan/5-DBgenerality-RUCbase/blob/main/src/storage/disk_manager.cpp)

|     Func     |      Todo      | Explaination                                                           |
| :-----------: | :------------: | :--------------------------------------------------------------------- |
|  write_page  |     写文件     | 根据文件大小lseek至偏移位置，写后进行大小检验                          |
|   read_page   |     读文件     | 根据文件大小lseek至偏移位置，读后进行大小检验                          |
| allocate_page |   分配新页号   | 采用单纯的自增策略                                                     |
|    is_file    | 是不是普通文件 | 调用 `stat(path.c_str(), &st)`，获取指定路径的文件信息并判断文件类型 |
|  create_file  |    创建文件    | 使用O_CREAT模式调用open()函数                                          |
| destroy_file |    删除文件    | 先关闭已打开的文件，再进行删除                                         |
|   open_file   |    打开文件    | 如果已经打开，直接返回文件描述符就行                                   |
|  close_file  |    关闭文件    | 先检查是否已经打开，不能关闭未打开者                                   |
| get_file_size |  获得文件大小  | 使用stat获得文件大小                                                   |
| get_file_name |   获得文件名   | 使用fd2path_哈希表获得名称                                             |
|  get_file_fd  |   获得文件id   | 使用path2fd_哈希表获得文件名称                                         |
|   read_log   |     读日志     | 使用名称打开日志，结合日志大小进行读操作                               |
|   write_log   |     写日志     | 使用名称打开日志，进行写操作                                           |

**注意**：

使用 std::atomic<page_id_t> 保证多线程环境下页面号分配的安全性

##### 任务1.2 缓冲池替换策略

需要完成[LeastRecentlyUsed_Replacer](https://github.com/ISeeCan/5-DBgenerality-RUCbase/blob/main/src/replacer/lru_replacer.cpp)

|  Func  |     Todo     | Explaination                                   |
| :----: | :----------: | ---------------------------------------------- |
| victim | 获取淘汰目标 | 从链表中获得最后一个元素，将其从LRUhash_内移除 |
|  pin  |   固定元素   | 通过id查找要固定的页面，不再追踪管理           |
| unpin |   取消固定   | 通过id查找要固定的页面，如果未被追踪则添加     |

Replacer类使用双向链表std::list维护页面的访问顺序，其链表头部是最近使用的页面，尾部是最久未使用的页面；并且使用无序哈希表std::unordered_map来实现高效的页面查找和链表操作。同时支持并发操作，利用 std::scoped_lock 确保线程安全。

##### 任务1.3 缓冲池管理器

需要完成[BufferPoolManager](https://github.com/ISeeCan/5-DBgenerality-RUCbase/blob/main/src/storage/buffer_pool_manager.cpp)

|       Func       |     Todo     | Explaination                                                                                        |
| :--------------: | :----------: | --------------------------------------------------------------------------------------------------- |
| find_victim_page |  寻找替换页  | 若缓冲池已满，则通过LRU选择被替换页，否则返回空页                                                   |
|   update_page   |   更新页面   | 写回脏页，删除后替换为新页                                                                          |
|    fetch_page    |   获取页面   | 若已位于缓冲区，则直接获取并计数，否则利用上述两个函数选择页面替换                                  |
|    unpin_page    | 取消固定页面 | 查找页面，减少 `pin_count_`，如果计数为 0（不再被任何进程使用），则通知**Replacer**解除固定 |
|    flush_page    |   写回磁盘   | 查找页面，调用**DiskManager**写回磁盘，重置藏标志                                             |
|     new_page     |  创建新页面  | 寻找空闲帧，调用**DiskManager** 分配新页面ID并update                                          |
|   delete_page   |   删除页面   | 查找页面，如果未被pin，则利用**DiskManager**删除                                              |
| flush_all_pages |   写回所有   | 遍历所有页面并逐个写回，重置脏标志                                                                  |

**BufferPoolManager**是调用对**DiskManager**和**LRUReplacer**的顶层调用，实现了更加宏观的各种操作。注意上锁。

## 实验二

### 实验目标

[详细说明](https://github.com/ISeeCan/5-DBgenerality-RUCbase/blob/main/docs/Rucbase-Lab2%5B%E7%B4%A2%E5%BC%95%E7%AE%A1%E7%90%86%E5%AE%9E%E9%AA%8C%E6%96%87%E6%A1%A3%5D.md)
