# RUC 数据库系统概论

- [RUC 数据库系统概论](#ruc-数据库系统概论)
  - [前置](#前置)
  - [实验一](#实验一)
    - [实验说明](#实验说明)
      - [任务一 缓冲池管理器](#任务一-缓冲池管理器)
        - [任务1.1 磁盘存储管理器](#任务11-磁盘存储管理器)
  - [实验二](#实验二)
    - [实验目标](#实验目标)

## 前置

具体项目安装配置见个人仓库[README](https://github.com/ISeeCan/5-DBgenerality-RUCbase)，官方项目仓库见[链接](https://github.com/ISeeCan/5-DBgenerality-RUCbase)

## 实验一

### 实验说明

[详细说明](https://github.com/ISeeCan/5-DBgenerality-RUCbase/blob/main/docs/Rucbase-Lab1%5B%E5%AD%98%E5%82%A8%E7%AE%A1%E7%90%86%E5%AE%9E%E9%AA%8C%E6%96%87%E6%A1%A3%5D.md)

#### 任务一 缓冲池管理器

##### 任务1.1 磁盘存储管理器

需要完成[DiskManager](https://github.com/ISeeCan/5-DBgenerality-RUCbase/blob/main/src/storage/disk_manager.cpp)

|     Func     |      Todo      |                                Explain                                |
| :-----------: | :------------: | :--------------------------------------------------------------------: |
|  write_page  |     写文件     |             根据文件大小lseek至偏移位置，写后进行大小检验             |
|   read_page   |     读文件     |             根据文件大小lseek至偏移位置，读后进行大小检验             |
| allocate_page |   分配新页号   |                           采用单纯的自增策略                           |
|    is_file    | 是不是普通文件 | 调用 `stat(path.c_str(), &st)`，获取指定路径的文件信息并判断文件类型 |
|  create_file  |    创建文件    |                     使用O_CREAT模式调用open()函数                     |
| destroy_file |    删除文件    |                     先关闭已打开的文件，再进行删除                     |
|   open_file   |    打开文件    |                  如果已经打开，直接返回文件描述符就行                  |
|  close_file  |    关闭文件    |                  先检查是否已经打开，不能关闭未打开者                  |
| get_file_size |  获得文件大小  |                          使用stat获得文件大小                          |
| get_file_name |   获得文件名   |                       使用fd2path_哈希表获得名称                       |
|  get_file_fd  |   获得文件id   |                     使用path2fd_哈希表获得文件名称                     |
|   read_log   |     读日志     |                使用名称打开日志，结合日志大小进行读操作                |
|   write_log   |     写日志     |                      使用名称打开日志，进行写操作                      |

**注意**：

使用 std::atomic<page_id_t> 保证多线程环境下页面号分配的安全性

## 实验二

### 实验目标

[详细说明](https://github.com/ISeeCan/5-DBgenerality-RUCbase/blob/main/docs/Rucbase-Lab2%5B%E7%B4%A2%E5%BC%95%E7%AE%A1%E7%90%86%E5%AE%9E%E9%AA%8C%E6%96%87%E6%A1%A3%5D.md)
