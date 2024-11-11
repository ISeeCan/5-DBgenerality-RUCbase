[原readme在这里](Ori-README.md)

在此简要记录我的实验过程：

首先根据docs文件夹下的[环境配置文档](docs/Rucbase环境配置文档.md)，[使用文档](docs/Rucbase使用文档.md)进行安装配置

#### 实验一

[实验说明](docs/Rucbase-Lab1[存储管理实验文档].md)

任务：补全src/storage/disk_manager.cpp中的函数

    任务1.1 磁盘存储管理器  需要补全的函数：

    ``` c
    void write_page(int fd, page_id_t page_no, const char *offset, int num_bytes);
    void read_page(int fd, page_id_t page_no, char *offset, int num_bytes);
    page_id_t allocate_page(int fd);
    bool is_file(const std::string &path);
    void create_file(const std::string &path);
    int open_file(const std::string &path);
    void close_file(int fd);
    void destroy_file(const std::string &path);
    ```
