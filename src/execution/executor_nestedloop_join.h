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

class NestedLoopJoinExecutor : public AbstractExecutor {    //嵌套链接模块
   private:
    std::unique_ptr<AbstractExecutor> left_;    // 左儿子节点（需要join的表）
    std::unique_ptr<AbstractExecutor> right_;   // 右儿子节点（需要join的表）
    size_t len_;                                // join后获得的每条记录的长度
    std::vector<ColMeta> cols_;                 // join后获得的记录的字段

    std::vector<Condition> fed_conds_;          // join条件
    bool isend;

   public:
    NestedLoopJoinExecutor(std::unique_ptr<AbstractExecutor> left, std::unique_ptr<AbstractExecutor> right, 
                            std::vector<Condition> conds) {
        left_ = std::move(left);
        right_ = std::move(right);
        len_ = left_->tupleLen() + right_->tupleLen();
        cols_ = left_->cols();
        auto right_cols = right_->cols();
        for (auto &col : right_cols) {
            col.offset += left_->tupleLen();
        }

        cols_.insert(cols_.end(), right_cols.begin(), right_cols.end());    //将左表和右表的列合并成一个字段列表,右表在左表后
        isend = false;
        fed_conds_ = std::move(conds);

        // ATTENTION ------ -------- -------- ------- -------- ------ add Lock?
    }

    //New Add
    bool is_end() const override { return left_->is_end(); }    //检查左表是否已经遍历完（是否已经全部结束）
    size_t tupleLen() const override { return len_; }   //返回连接后每条记录的长度（左表和右表记录长度的总和）
    const std::vector<ColMeta> &cols() const override { return cols_; } //返回连接后的字段列表（包括左表和右表的所有列）

    void beginTuple() override {
        //Need to do
        left_->beginTuple();
        if (left_->is_end()) {
            return;
        }
        right_->beginTuple();
        while (!is_end()) { //如果满足连接条件（eval_conds），则跳出循环。
            if (eval_conds(cols_, fed_conds_, left_->Next().get(), right_->Next().get())) {
                break;
            }
            right_->nextTuple();
            if (right_->is_end()) {
                left_->nextTuple();
                right_->beginTuple();
            }
            //如果右表的记录遍历完，则开始遍历下一条左表记录，并重新从头开始遍历右表
        }
    }

    void nextTuple() override {
        //Need to do
        //right_->nextTuple()：让右表移动到下一条记录。
        //如果右表的记录已经遍历完，则移动到左表的下一条记录，并重新开始遍历右表。
        assert(!is_end());
        right_->nextTuple();
        if (right_->is_end()) {
            left_->nextTuple();
            right_->beginTuple();
        }
        while (!is_end()) {
            if (eval_conds(cols_, fed_conds_, left_->Next().get(), right_->Next().get())) {
                break;
            }
            right_->nextTuple();
            if (right_->is_end()) {
                left_->nextTuple();
                right_->beginTuple();
            }
        }
    }

    std::unique_ptr<RmRecord> Next() override {
        //Need to do
        //递归连接到一块
        assert(!is_end());
        auto record = std::make_unique<RmRecord>(len_);
        auto left_record = left_->Next();
        auto right_record = right_->Next();
        memcpy(record->data, left_record->data, left_record->size);
        memcpy(record->data + left_record->size, right_record->data, right_record->size);
        return record;
        return nullptr;
    }


    bool eval_cond(const std::vector<ColMeta> &rec_cols, const Condition &cond, const RmRecord *lrec,
                   const RmRecord *rrec) {
        //NEW ADD
        
        auto lhs_col = get_col(rec_cols, cond.lhs_col); //获取左表条件的列元数据
        char *lhs = lrec->data + lhs_col->offset;   //获得参与比较的左值
        char *rhs;
        ColType rhs_type;
        if (cond.is_rhs_val) {  //是常量则直接获取常量值并设置它的类型
            rhs_type = cond.rhs_val.type;
            rhs = cond.rhs_val.raw->data;   //常量右值直接获得
        } else {
            // rhs is a column
            auto rhs_col = get_col(rec_cols, cond.rhs_col);
            rhs_type = rhs_col->type;       //右表的字段位置相对于整个连接结果的记录布局
            rhs = rrec->data + rhs_col->offset - left_->tupleLen();
        }
        assert(rhs_type == lhs_col->type);  //确保左右列的类型一致
        int cmp = ix_compare(lhs, rhs, rhs_type, lhs_col->len);
        if (cond.op == OP_EQ) {
            return cmp == 0;
        } else if (cond.op == OP_NE) {
            return cmp != 0;
        } else if (cond.op == OP_LT) {
            return cmp < 0;
        } else if (cond.op == OP_GT) {
            return cmp > 0;
        } else if (cond.op == OP_LE) {
            return cmp <= 0;
        } else if (cond.op == OP_GE) {
            return cmp >= 0;
        } else {
            throw InternalError("Unexpected op type");
        }
    }

    bool eval_conds(const std::vector<ColMeta> &rec_cols, const std::vector<Condition> &conds, const RmRecord *lrec,
                    const RmRecord *rrec) {
        //NEW ADD
        //将所有连接条件逐一应用于左表和右表的记录。如果所有条件都满足，返回 true，表示这对记录可以连接
        return std::all_of(conds.begin(), conds.end(),
                           [&](const Condition &cond) { return eval_cond(rec_cols, cond, lrec, rrec); });
    }

    Rid &rid() override { return _abstract_rid; }
};