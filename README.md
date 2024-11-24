**本仓库代替原fork仓库，成为了个人的私有不公开仓库以便核验，项目完成后将取代另一仓库**

部分文档可能存在更新，最新版请详见[实验发布地址](https://github.com/ruc-deke/rucbase-lab)

[原readme在这里](Ori-README.md)

在此简要记录我的实验过程：

首先根据docs文件夹下的[环境配置文档](docs/Rucbase环境配置文档.md)，[使用文档](docs/Rucbase使用文档.md)进行安装配置

#### 实验一

[实验说明](docs/Rucbase-Lab1[存储管理实验文档].md)

##### 任务1.1 磁盘存储管理器：

补全src/storage/disk_manager.cpp中的函数

[链接](src/storage/disk_manager.cpp)

依赖的函数如下，需要手动补全的有前缀-：

``` cpp
    - void write_page(int fd, page_id_t page_no, const char *offset, int num_bytes);
    - void read_page(int fd, page_id_t page_no, char *offset, int num_bytes);
    page_id_t allocate_page(int fd);
    bool is_file(const std::string &path);
    - void create_file(const std::string &path);
    - int open_file(const std::string &path);
    - void close_file(int fd);
    - void destroy_file(const std::string &path);
```

相关知识及系统函数：

lseek(fd, offset, whence)
lseek() 是一个文件操作系统调用，用于在文件中移动文件指针。它的三个参数是：

fd：文件描述符，指向打开的文件。通常由 open 函数返回。

offset：偏移量，以字节为单位。这里传入的是 page_no * PAGE_SIZE，表示目标页在文件中的位置。通过这个计算，我们可以跳转到特定页。

whence：参照位置，SEEK_SET 表示从文件开头开始偏移 offset 个字节，直接定位到指定页的位置。假设 PAGE_SIZE 为 4096 字节（典型数据库页大小），如果 page_no = 2，lseek 将把文件指针定位到 2 * 4096 = 8192 字节处，相当于文件的第三个页位置。

lseek 的返回值是新文件指针的位置（即新的偏移量），如果返回值是 -1，说明定位失败，通常会抛出 UnixError。
    

##### 任务1.2 缓冲池替换策略

补全`Replacer`类，其负责跟踪缓冲池中每个页面所在帧的使用情况。当缓冲池没有空闲页面时，需要使用该类提供的替换策略选择一个页面进行淘汰。要求实现的替换策略为最近最少使用（LRU）算法。

需要补全的文件位于：src/replacer/lru_replacer.cpp

[链接](src/replacer/lru_replacer.cpp)

``` cpp
    bool LRUReplacer::victim(frame_id_t* frame_id)
    void LRUReplacer::pin(frame_id_t frame_id) 
    void LRUReplacer::unpin(frame_id_t frame_id)
```

知识点：std::scoped_lock

功能：std::scoped_lock 是 C++17 引入的一种锁，能够实现作用域内自动加锁和解锁功能。std::scoped_lock 会在构造时自动上锁，在作用域结束或遇到异常退出时自动解锁，避免因未手动解锁而造成的死锁问题。
    
锁对象 latch_：latch_ 是一个互斥锁（std::mutex），保护函数的关键区域，确保只有一个线程能够进入并执行操作。其他线程只能在加锁结束后才能继续访问。

##### 任务1.3 缓冲池管理器

补全`BufferPoolManager`类，其负责管理缓冲池中的页面与磁盘文件中的页面之间的来回移动。

文件位置：src/storage/buffer_pool_manager.cpp

[链接](src/storage/buffer_pool_manager.cpp)

需补全函数：
    
``` cpp
    public:
        BufferPoolManager(size_t pool_size, DiskManager *disk_manager);
        ~BufferPoolManager();
        - Page *new_page(PageId *page_id);
        - Page *fetch_page(PageId page_id);
        - bool unpin_page(PageId page_id, bool is_dirty);
        - bool delete_page(PageId page_id);
        - bool flush_page(PageId page_id);
        - void flush_all_pages(int fd);
    private:
        // 辅助函数
        - bool find_victim_page(frame_id_t *frame_id);
        - void update_page(Page *page, PageId new_page_id, frame_id_t new_frame_id);
```

值得注意的是，删除过程中需要判断页面有没有被访问：

如果 pin_count_ 大于 0，表示当前页面正在被访问或使用，无法删除，因此返回 false。pin_count 为零的页面说明没有其他线程或操作正在使用它，因此可以安全地将其删除。

##### 任务2.1 记录操作

补全`RMFileHandle`类，其负责对文件中的记录进行操作。

每个`RMFileHandle`对应一个记录文件，当`RMManager`执行打开文件操作时，便会创建一个指向`RMFileHandle`的指针。文件位于：src/record/rm_file_handle.cpp

[链接](src/record/rm_file_handle.cpp)

`RMFileHandle`类的接口如下：

``` cpp
    public:
    - RmFileHandle(DiskManager *disk_manager, BufferPoolManager *buffer_pool_manager, int fd);
    // 不考虑事务的记录操作（事务将在后续实验使用）
    - std::unique_ptr<RmRecord> get_record(const Rid &rid, Context *context) const;
    - Rid insert_record(char *buf, Context *context);
    - void delete_record(const Rid &rid, Context *context);
    - void update_record(const Rid &rid, char *buf, Context *context);
    // 辅助函数
    - RmPageHandle create_new_page_handle();
    - RmPageHandle fetch_page_handle(int page_no) const;
    - RmPageHandle create_page_handle();
    - void release_page_handle(RmPageHandle &page_handle);
```

##### 任务2.2 记录迭代器

本任务要求补全`RmScan`类，其用于遍历文件中存放的记录。

文件位置：src/record/rm_scan.cpp

[链接](src/record/rm_scan.cpp)

`RmScan`类继承于`RecScan`类，它们的接口如下：

```cpp
    class RecScan {
    public:
        virtual ~RecScan() = default;
        virtual void next() = 0;
        virtual bool is_end() const = 0;
        virtual Rid rid() const = 0;
    };

    class RmScan : public RecScan {
    public:
        - RmScan(const RmFileHandle *file_handle);
        - void next() override;
        - bool is_end() const override;
        Rid rid() const override;
    };
```

##### 进行测试

先编译项目后在根目录运行以下指令，编译方法见使用文档：

```bash
cd build

make disk_manager_test
./bin/disk_manager_test

make lru_replacer_test
./bin/lru_replacer_test

make buffer_pool_manager_test
./bin/buffer_pool_manager_test

make record_manager_test
./bin/record_manager_test
```

```bash
cd build
./bin/disk_manager_test
./bin/lru_replacer_test
./bin/buffer_pool_manager_test
./bin/record_manager_test
```

#### 实验一

##### 任务1 B+树的查找

文件位置：[src/index/ix_index_handle.cpp](src/index/ix_index_handle.cpp)

需要实现：

int lower_bound(const char *target) const;
用于在当前结点中查找第一个大于或等于target的key的位置。

int upper_bound(const char *target) const;
用于在当前结点中查找第一个大于target的key的位置。