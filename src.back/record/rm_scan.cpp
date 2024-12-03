/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "rm_scan.h"
#include "rm_file_handle.h"

/**
 * @brief 初始化file_handle和rid
 * @param file_handle
 */
RmScan::RmScan(const RmFileHandle *file_handle) : file_handle_(file_handle) {
    // Todo:
    // 初始化file_handle和rid（指向第一个存放了记录的位置）
    int maxpage = file_handle_->file_hdr_.num_pages;// 获取记录文件的最大页面数
    int pageno = 1;
    if(maxpage > 1)// 如果文件包含多个页面
    {
        for(pageno = 1; pageno < maxpage; pageno++)// 遍历页面，查找包含记录的第一页
        {
            if(file_handle_->fetch_page_handle(pageno).page_hdr->num_records > 0)
            { 
                int i = Bitmap::first_bit(1, file_handle_->fetch_page_handle(pageno).bitmap, file_handle_->file_hdr_.num_records_per_page);// 找到第一个包含记录的插槽
                // 设置rid_为找到的页面和插槽编号
                rid_.page_no = pageno; 
                rid_.slot_no = i;
                return;
            }
        }
    }
    rid_=Rid{-1,-1};// 如果没有找到包含记录的页面，则将rid_设置为无效值
}

/**
 * @brief 找到文件中下一个存放了记录的位置
 */
void RmScan::next() {
    // Todo:
    // 找到文件中下一个存放了记录的非空闲位置，用rid_来指向这个位置
    int maxpage = file_handle_->file_hdr_.num_pages;// 获取记录文件的最大页面数
    int pageno = rid_.page_no;
    int slotno = rid_.slot_no;
    for(;pageno < maxpage; pageno++)// 遍历页面，查找下一个包含记录的页面和插槽
    {
        int i = Bitmap::next_bit(1, file_handle_->fetch_page_handle(pageno).bitmap, file_handle_->file_hdr_.num_records_per_page, slotno);// 查找下一个包含记录的插槽
        if(i == file_handle_->file_hdr_.num_records_per_page)// 如果i等于页面的最大插槽数，说明没有找到下一个包含记录的插槽，将slotno设置为-1并继续下一轮查找
        {   
            slotno = -1;
            continue;  
        }
        else// 找到下一个包含记录的插槽，将rid_设置为找到的页面和插槽编号
        {
            rid_.page_no = pageno;
            rid_.slot_no = i;
            return;
        }
    }
    rid_=Rid{-1,-1};// 如果没有找到下一个包含记录的位置，将rid_设置为无效值，表示扫描结束
}

/**
 * @brief ​ 判断是否到达文件末尾
 */
bool RmScan::is_end() const {
    // Todo: 修改返回值
    return rid_.page_no == RM_NO_PAGE;// 如果rid_的page_no等于RM_NO_PAGE，表示到达了文件末尾，返回true，否则返回false
    return false;
}

/**
 * @brief RmScan内部存放的rid
 */
Rid RmScan::rid() const {
    return rid_;
}