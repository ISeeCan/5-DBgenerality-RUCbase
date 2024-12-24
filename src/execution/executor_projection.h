/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once
#include "execution_defs.h"
#include "execution_manager.h"
#include "executor_abstract.h"
#include "index/ix.h"
#include "system/sm.h"

class ProjectionExecutor : public AbstractExecutor {
   private:
    std::unique_ptr<AbstractExecutor> prev_;        // 投影节点的儿子节点
    std::vector<ColMeta> cols_;                     // 需要投影的字段
    size_t len_;                                    // 字段总长度
    std::vector<size_t> sel_idxs_;                  //存储选择列在原始表格中的位置。这个索引用于在投影操作中提取正确的列

   public:
    ProjectionExecutor(std::unique_ptr<AbstractExecutor> prev, const std::vector<TabCol> &sel_cols) {
        prev_ = std::move(prev);

        size_t curr_offset = 0;
        auto &prev_cols = prev_->cols();
        for (auto &sel_col : sel_cols) {
            auto pos = get_col(prev_cols, sel_col); //根据列名或列描述符返回该列在 prev_cols 中的位置
            sel_idxs_.push_back(pos - prev_cols.begin());   //设置sel_idxs_
            auto col = *pos;
            col.offset = curr_offset;
            curr_offset += col.len;
            cols_.push_back(col);
        }
        len_ = curr_offset;

        // ATTENTION ------ -------- -------- ------- -------- ------ add Lock?
    }

    //new add
    bool is_end() const override { return prev_->is_end(); }
    size_t tupleLen() const override { return len_; }
    const std::vector<ColMeta> &cols() const override { return cols_; }

    void beginTuple() override {
        //Need to do
        prev_->beginTuple();
    }

    void nextTuple() override {
        //Need to do
        assert(!prev_->is_end());
        prev_->nextTuple();
    }

    std::unique_ptr<RmRecord> Next() override {
        assert(!is_end());
        auto &prev_cols = prev_->cols();
        auto prev_rec = prev_->Next();
        auto &proj_cols = cols_;
        auto proj_rec = std::make_unique<RmRecord>(len_);   //创建一条新的记录 proj_rec，它的长度是投影后的长度
        for (size_t proj_idx = 0; proj_idx < proj_cols.size(); proj_idx++) {
            size_t prev_idx = sel_idxs_[proj_idx];
            auto &prev_col = prev_cols[prev_idx];   //获取原始记录中与当前投影列对应的列元数据
            auto &proj_col = proj_cols[proj_idx];   //获取当前投影列的列元数据
            memcpy(proj_rec->data + proj_col.offset, prev_rec->data + prev_col.offset, prev_col.len);   //将原始记录中的列数据拷贝到投影结果记录中的正确位置
        }
        return proj_rec;
        return nullptr;
    }

    Rid &rid() override { return _abstract_rid; }
};