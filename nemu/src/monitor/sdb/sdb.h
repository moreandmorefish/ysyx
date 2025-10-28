/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>

word_t expr(char *e, bool *success);

// 初始化监视点池
void init_wp_pool();

// 创建监视点：传入表达式字符串和初始值
void wp_watch(char *expr, word_t res);

// 删除指定编号的监视点
void wp_remove(int no);

// 遍历并打印所有监视点（供 info w 调用）
void wp_iterate();

// 检测所有监视点的表达式值变化（供 trace_and_difftest 调用）
void wp_difftest();

#endif
