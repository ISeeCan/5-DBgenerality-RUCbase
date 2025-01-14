**本仓库代替原fork仓库，成为了个人的私有不公开仓库以便核验，项目完成后将取代另一仓库**:

# RUC 数据库系统概论

本仓库作为中国人民大学2024年秋季学期数据库系统概论的荣誉课程代码仓库，保存了（当时可行的）课程实验相关内容，目前进度：

- [实验一](#实验一)已完成
- [实验二](#实验二)已完成
- [实验三](#实验三)已完成
- [实验四](#实验四)已完成
- [附加实验](#附加实验40分)已完成

部分实验说明文档可能存在更新，最新版请详见[实验发布地址](https://github.com/ruc-deke/rucbase-lab)

[原readme在这里](Ori-README.md)

在此简要记录我的实验过程与各个实验的实验目标：

首先根据docs文件夹下的[环境配置文档](docs/Rucbase环境配置文档.md)进行环境配置，根据[使用文档](docs/Rucbase使用文档.md)进行编译安装

在我的实验环境下我选择了如下代码进行编译：

``` bash
#均位于根目录开始执行
#服务端
mkdir build
cd build 
cmake .. -DCMAKE_BUILD_TYPE=Debug
make rmdb -j4
#客户端
cd rucbase_client
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j3
```

#### 实验一

[实验说明](docs/Rucbase-Lab1[存储管理实验文档].md)

##### 任务1.1 磁盘存储管理器

补全[src/storage/disk_manager.cpp](src/storage/disk_manager.cpp)中的函数

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

lseek(fd, offset, whence)：lseek() 是一个文件操作系统调用，用于在文件中移动文件指针。它的三个参数是：

fd：文件描述符，指向打开的文件。通常由 open 函数返回。

offset：偏移量，以字节为单位。这里传入的是 page_no * PAGE_SIZE，表示目标页在文件中的位置。通过这个计算，我们可以跳转到特定页。

whence：参照位置，SEEK_SET 表示从文件开头开始偏移 offset 个字节，直接定位到指定页的位置。假设 PAGE_SIZE 为 4096 字节（典型数据库页大小），如果 page_no = 2，lseek 将把文件指针定位到 2 * 4096 = 8192 字节处，相当于文件的第三个页位置。

lseek 的返回值是新文件指针的位置（即新的偏移量），如果返回值是 -1，说明定位失败，通常会抛出 UnixError。

##### 任务1.2 缓冲池替换策略

补全`Replacer`类，其负责跟踪缓冲池中每个页面所在帧的使用情况。当缓冲池没有空闲页面时，需要使用该类提供的替换策略选择一个页面进行淘汰。要求实现的替换策略为最近最少使用（LRU）算法。

需要补全的文件位于：[src/replacer/lru_replacer.cpp](src/replacer/lru_replacer.cpp)

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

文件位置：[src/storage/buffer_pool_manager.cpp](src/storage/buffer_pool_manager.cpp)

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

每个`RMFileHandle`对应一个记录文件，当`RMManager`执行打开文件操作时，便会创建一个指向`RMFileHandle`的指针。文件位于：[src/record/rm_file_handle.cpp](src/record/rm_file_handle.cpp)

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

文件位置：[src/record/rm_scan.cpp](src/record/rm_scan.cpp)

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

#### 实验二

##### 任务1 B+树的查找

文件位置：[src/index/ix_index_handle.cpp](src/index/ix_index_handle.cpp)

###### 节点内查找

需要实现：

``` plaintext
class IxNodeHandle
- int lower_bound(const char *target) const;
用于在当前结点中查找第一个大于或等于target的key的位置。

- int upper_bound(const char *target) const;
用于在当前结点中查找第一个大于target的key的位置。

- bool leaf_lookup(const char *key, Rid **value);
​ 用于叶子结点根据key来查找该结点中的键值对。值value作为传出参数，函数返回是否查找成功。

- page_id_t internal_lookup(const char *key);
​ 用于内部结点根据key来查找该key所在的孩子结点（子树）。
```

###### B+树查找

``` plaintext
class IxIndexHandle
- std::pair<IxNodeHandle *, bool> find_leaf_page(const char *key, Operation operation, Transaction *transaction, bool find_first = false);
​用于查找指定键所在的叶子结点。
​从根结点开始，不断向下查找孩子结点，直到找到包含该key的叶子结点

- bool get_value(const char *key, std::vector<Rid> *result, Transaction *transaction);
用于查找指定键在叶子结点中的对应的值result。
提示：可以调用find_leaf_page()和leaf_lookup()函数。
```

##### 任务2 B+树的插入

###### 结点内的插入

需要实现：

``` plaintext
class IxNodeHandle
- void insert_pairs(int pos, const char *key, const Rid *rid, int n);
-   int insert(const char *key, const Rid &value);
```

###### B+树的插入

``` plaintext
class IxIndexHandle
    // B+树的插入
- page_id_t insert_entry(const char *key, const Rid &value, Transaction *transaction);
    IxNodeHandle *split(IxNodeHandle *node);
- void insert_into_parent(IxNodeHandle *old_node, const char *key, IxNodeHandle *new_node, Transaction *transaction);
```

##### 任务3 B+树的删除

###### 结点内的删除

``` plaintext
class IxNodeHandle
    // 结点内的删除
- void erase_pair(int pos);
- int remove(const char *key);
###### B+树的删除
```

###### B+树的删除

``` plaintext
class IxIndexHandle
    // B+树的删除
- bool delete_entry(const char *key, Transaction *transaction);
- bool coalesce_or_redistribute(IxNodeHandle *node, Transaction *transaction = nullptr,bool *root_is_latched = nullptr);
- bool coalesce(IxNodeHandle **neighbor_node, IxNodeHandle **node, IxNodeHandle **parent, int index,Transaction *transaction, bool *root_is_latched);
- void redistribute(IxNodeHandle *neighbor_node, IxNodeHandle *node, IxNodeHandle *parent, int index);
- bool adjust_root(IxNodeHandle *old_root_node);
```

##### 任务4 B+树索引并发控制

本任务要求修改IxIndexHandle类的原实现逻辑，让其支持对B+树索引的并发查找、插入、删除操作。
学生可以选择实现并发的粒度，选择下面两种并发粒度的任意一种进行实现即可。

##### 测试

编译生成可执行文件进行测试：

``` bash
cd build

make b_plus_tree_insert_test
./bin/b_plus_tree_insert_test

make b_plus_tree_delete_test
./bin/b_plus_tree_delete_test

make b_plus_tree_concurrent_test
./bin/b_plus_tree_concurrent_test
```

注意：

在本实验中的所有测试只调用get_value()、insert_entry()、delete_entry()这三个函数。学生可以自行添加和修改辅助函数，但不能修改以上三个函数的声明。
进行测试前，学生还需自行完成src/system/sm_manager.cpp中的SmManager::create_index()函数，方可进行测试。

#### 实验三

我们有[指导文档](docs/Rucbase-Lab3[查询执行实验指导].md)

Rucbase查询执行模块采用的是火山模型(Volcano Model),你可以通过[链接](https://www.computer.org/csdl/journal/tk/1994/01/k0120/13rRUwI5TRe)获取相应论文阅读以理解火山模型的基本概念，并结合项目结构文档理解系统查询模块的整体架构和处理流程。

P.S.: 另外一个[论文链接](https://paperhub.s3.amazonaws.com/dace52a42c07f7f8348b08dc2b186061.pdf)

在本测试中，要求把select语句的输出写入到指定文件中，写入逻辑已经在select_from函数中给出，不要修改写入格式。 对于执行错误的SQL语句，需要打印failure到output.txt文件中。

##### 实验一：元数据管理和DDL语句 (25分)

完成[src/system/sm_manager.cpp](src/system/sm_manager.cpp)中的接口，使得系统能够支持DDL语句，具体包括create table、drop table、create index和drop index语句。

需要实现SmManager类的:

``` c
open_db(...)：系统通过调用该接口打开数据库
close_db(...)：系统通过调用该接口关闭数据库
drop_table(...)：删除表
create_index(...)：创建索引
drop_index(...)：删除索引
```

进行测试：

``` bash
cd src/test/query
python query_unit_test.py basic_query_test1.sql # 25分
python query_unit_test.py basic_query_test{i}.sql  # replace {i} with the desired test file index
```

实际测试：

``` bash
cd src/test/query
python query_unit_test.py basic_query_test1.sql # 25分
```

##### 实验二：DML语句实现（75分）

完成src/execution文件夹下执行算子中的空缺函数，使得系统能够支持增删改查四种DML语句。

相关代码位于src/execution文件夹下，其中需要完成的文件包括[executor_delete.h](src/execution/executor_delete.h)、[executor_nestedloop_join.h](src/execution/executor_nestedloop_join.h)、[executor_projection.h](src/execution/executor_projection.h)、[executor_seq_scan.h](src/execution/executor_seq_scan.h)、[executor_update.h](src/execution/executor_update.h)，已经实现的文件包括executor_insert.h和execution_manager.cpp。

需要仿照insert算子实现如下算子：

`SeqScan`算子：你需要实现该算子的Next()、beginTuple()和nextTuple()接口，用于表的扫描，你需要调用RmScan中的相关函数辅助实现（这些接口在lab1中已经实现）；
`Projection`算子：你需要实现该算子的Next()、beginTuple()和nextTuple()接口，该算子用于投影操作的实现；
`NestedLoopJoin`算子：你需要实现该算子的Next()、beginTuple()和nextTuple()接口，该算子用于连接操作，在本实验中，你只需要支持两表连接；
`Delete`算子：你需要实现该算子的Next()接口，该算子用于删除操作；
`Update`算子：你需要实现该算子的Next()接口，该算子用于更新操作。
所有算子都继承了抽象算子类execuotr_abstract，它给出了各个算子继承的基类抽象算子的声明和相应方法。

*在本系统中，默认规定用的连接方式是连接算子作为右孩子*，需要补充完成LoopJoin算子中的以下3个方法：

``` c
void beginTuple() override {}
void nextTuple() override {}
std::unique_ptr<RmRecord> Next() override{}
```

进行测试：

``` bash
cd src/test/query
python query_test_basic.py  # 100分
```

#### 实验四

##### 前置

在完成本实验之前，需要取消[`rmdb.cpp::client_handler()`](./src/rmdb.cpp)函数中对如下语句的注释：

第121行：`SetTransaction(&txn_id, context);`

第183～186行：

``` cpp
if(context->txn_->get_txn_mode() == false)
{
    txn_manager->commit(context->txn_, context->log_mgr_);
}
```

##### 实验四·一 事务管理器实验（40分）

- 在本实验中，你需要实现系统中的事务管理器，即TransactionManager类。

相关数据结构包括Transaction类、WriteRecord类等，分别位于txn_def.h和transaction.h文件中。

本实验要求完成事务管理器中的三个接口：事务的开始、提交和终止方法。

- 你需要完成[src/transaction/transaction_manager.cpp](src/transaction/transaction_manager.cpp)文件中的以下接口：

begin(Transaction*, LogManager*)：该接口提供事务的开始方法。

提示：如果是新事务，需要创建一个Transaction对象，并把该对象的指针加入到全局事务表中。

commit(Transaction*, LogManager*)：该接口提供事务的提交方法。

提示：如果并发控制算法需要申请和释放锁，那么你需要在提交阶段完成锁的释放。

abort(Transaction*, LogManager*)：该接口提供事务的终止方法。

在事务的终止方法中，你需要对需要对事务的所有写操作进行撤销，事务的写操作都存储在Transaction类的write_set_中，因此，你可以通过修改存储层或执行层的相关代码来维护write_set_，并在终止阶段遍历write_set_，撤销所有的写操作。

提示：需要对事务的所有写操作进行撤销，如果并发控制算法需要申请和释放锁，那么你需要在终止阶段完成锁的释放。

思考：在回滚删除操作的时候，是否必须插入到record的原位置，如果没有插入到原位置，会存在哪些问题？

- 测试点及分数

``` bash
cd src/test/transaction
python transaction_test.py
```

也可以通过单元测试文件来进行单个测试点的测试，使用方法如下：

``` bash
cd src/test/transaction
python transaction_unit_test.py <test_case_name>
# The <test_case_name> should be one of the following options from the TESTS array:
# 'commit_test', 'abort_test', 'commit_index_test', 'abort_index_test'
# Replace <test_case_name> with the desired test case name to run that specific test.
```

##### 实验四·二 并发控制实验（60分）

###### 任务1：锁管理器实现

首先要求完成锁管理器LockManager类。相关数据结构包括LockDataId、TransactionAbortException、LockRequest、LockRequestQueue等，位于txn_def.h和Lockanager.h文件中。

你需要完成[src/transaction/concurrency/lock_manager.cpp](src/transaction/concurrency/lock_manager.cpp)文件中的以下接口：

（1）行级锁加锁
lock_shared_on_record(Transaction *, const Rid, int)：用于申请指定元组上的读锁。

事务要对表中的某个指定元组申请行级读锁，该操作需要被阻塞直到申请成功或失败，如果申请成功则返回true，否则返回false。

lock_exclusive_on_record(Transaction *, const Rid, int)：用于申请指定元组上的写锁。

事务要对表中的某个指定元组申请行级写锁，该操作需要被阻塞直到申请成功或失败，如果申请成功则返回true，否则返回false。

（2）表级锁加锁
lock_shared_on_table(Transaction *txn, int tab_fd)：用于申请指定表上的读锁。

事务要对表中的某个指定元组申请表级读锁，该操作需要被阻塞直到申请成功或失败，如果申请成功则返回true，否则返回false。

lock_exclusive_on_table(Transaction *txn, int tab_fd)：用于申请指定表上的写锁。

事务要对表中的某个指定元组申请表级写锁，该操作需要被阻塞直到申请成功或失败，如果申请成功则返回true，否则返回false。

lock_IS_on_table(Transaction *txn, int tab_fd)：用于申请指定表上的意向读锁。

事务要对表中的某个指定元组申请表级意向读锁，该操作需要被阻塞直到申请成功或失败，如果申请成功则返回true，否则返回false。

lock_IX_on_table(Transaction *txn, int tab_fd)：用于申请指定表上的意向写锁。

事务要对表中的某个指定元组申请表级意向写锁，该操作需要被阻塞直到申请成功或失败，如果申请成功则返回true，否则返回false。

（3）解锁
unlock(Transaction *, LockDataId)：解锁操作。
需要更新锁表，如果解锁成功则返回true，否则返回false。

###### 任务2：两阶段封锁协议的实现

需要调用任务一中锁管理器通过的加锁解锁接口，在合适的地方申请行级锁和意向锁，并在合适的地方释放事务的锁。

你需要修改[src/record/rm_file_handle.cpp](src/record/rm_file_handle.cpp)中的以下接口：

``` cpp
get_record(const Rid, Context *)：在该接口中，你需要申请对应元组上的行级锁。
delete_record(const Rid, Context *)：在该接口中，你需要申请对应元组上的行级锁。
update_record(const Rid, Context *)：在该接口中，你需要申请对应元组上的行级锁。
```

同时还需要修改src/system/sm_manager.cpp和executor_manager.cpp中的相关接口，在合适的地方申请行级锁和意向锁。主要涉及以下接口：

``` cpp
create_table(const std::string, const std::vector<ColDef>, Context *)
drop_table(const std::string, Context *)
create_index(const std::string, const std::string, Context *)
drop_index(const std::string, const std::string, Context *)
```

提示：除了事务锁的申请，还需要考虑txn_map等共享数据结构。

- 测试点及分数

``` bash
cd src/test/concurrency
python concurrency_test.py
```

也可以通过单元测试文件来进行针对上述六个测试点的单独测试，使用方法如下：

``` bash
cd src/test/concurrency
python concurrency_unit_test.py <test_case_name>
# Run the unit test script with a specific test case name
# The <test_case_name> should be one of the following options from the TESTS dictionary:
# 'concurrency_read_test', 'dirty_write_test', 'dirty_read_test', 
# 'lost_update_test', 'unrepeatable_read_test', 'unrepeatable_read_test_hard', 
# 'phantom_read_test_1', 'phantom_read_test_2', 'phantom_read_test_3', 'phantom_read_test_4'
# Each test case has an associated check method and score as defined in the TESTS dictionary.
# Replace <test_case_name> with the desired test case name to run that specific test.
```

##### 附加实验（40分）

在上述两个实验中，均未涉及到索引操作，在附加实验中，需要在表上存在索引时，依然能够完成事务的提交、回滚，保证事务的可串行化。

###### 任务1: 事务的提交与回滚（20分）

在本任务中，需要实现如下功能：

实现lab3中没有要求实现index_scan算子，支持索引扫描；
支持联合索引的创建，即如下语法：create index orders (o_w_id, o_d_id, o_id);
在插入删除数据时，需要同步对索引进行更新。

测试点及分数

``` bash
cd src/test/transaction
python transaction_test_bonus.py
```

本测试包含两个测试点，分别对事务的提交和回滚进行测试，测试点分数设置如下：

###### 任务2：幻读数据异常（20分）

在实验二中，没有对幻读数据异常进行测试，在本任务中，你需要规避幻读数据异常。可以通过表锁来规避幻读数据异常，但是会降低系统的并发度，因此，最合理的做法是通过间隙锁来规避幻读数据异常。

测试点及分数

``` bash
cd src/test/concurrency
python concurrency_test_bonus.py
```

本测试中包含四个测试点，每个分数点为5分，如果通过表锁的方式规避幻读数据异常，则最终得分为(通过测试点数量)*5/2，如果通过间隙锁的方式规避幻读数据异常，则最终得分为(通过测试点数量)*5

提示：当查询语句的条件符合索引扫描的条件时，系统会自动选择索引扫描，因此输出顺序是固定的，在幻读数据异常测试时，你的输出顺序需要和答案的输出顺序一致才可以得分。如果在某些测试点中，你发现系统没有选择索引扫描，那么你需要修改optimizer中的匹配规则，让符合索引查询条件的SQL语句使用索引扫描算子。
