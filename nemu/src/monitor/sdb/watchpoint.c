/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
    int NO;
    struct watchpoint *next;
    char expression[1024];
    uint32_t res;
    /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
    int i;
    for (i = 0; i < NR_WP; i++) {
        wp_pool[i].NO = i;
        wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    }

    head = NULL;
    free_ = wp_pool;
}

WP *new_wp() {
    if (free_ == NULL) {
        assert(0);
    }
    WP *node = free_;
    free_ = free_->next;
    node->next = NULL;
    if (!head) {
        head = node;
    } else {
        WP *tmp = head;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = node;
    }
    return node;
}

void free_wp(WP *wp) {
    if (!wp) {
        assert(0);
    }
    memset(wp->expression, 0, sizeof(1024));
    // 删除head节点
    if (head == wp) {
        // head头节点为wp
        head = head->next;
    } else {
        WP *tmp = head;
        while (tmp->next != NULL && tmp->next != wp) {
            tmp = tmp->next;
        }
        tmp->next = tmp->next->next;
    }
    wp->next = NULL;
    // 插回原处
    if (!free_) {
        free_ = wp;
        return;
    }
    WP *pre = free_;
    WP *next = free_->next;
    while (wp->NO > next->NO && next != NULL) {
        pre = next;
        next = next->next;
    }
    wp->next = pre->next;
    pre->next = wp;
}

int set_wp(char *expression) {
    bool *success = malloc(sizeof(bool));
    WP *wp = new_wp();
    strncpy(wp->expression, expression, sizeof(wp->expression) - 1);
    wp->res = expr(wp->expression, success);
    if (success)
        return 1;
    return 0;
}

int delete_wp(int n) {
    WP *node;
    for (node = head; node; node = node->next) {
        if (node->NO == n) {
            free_wp(node);
            return 1;
        }
    }
    return 0;
}

bool if_expr_change() {
    WP *wp;
    bool flag = false;
    for (wp = head; wp != NULL; wp = wp->next) {
        bool *success = malloc(sizeof(bool));
        uint32_t res = expr(wp->expression, success);
        if (res != wp->res) {
            flag = true;
            printf("Piont %d has changed:\n", wp->NO);
            // printf("  Address: %08x\n", cpu.pc);
            printf("  Expression: %s\n", wp->expression);
            printf("  Old value: %u\n", wp->res);
            printf("  New value: %u\n\n", res);
            wp->res = res;
        }
    }
    return flag;
}

void print_wp() {
    WP *wp = head;
    while (wp != NULL) {
        printf("Point %d:\n", wp->NO);
        printf("  Expression: %s\n", wp->expression);
        printf("  value: %u\n\n", wp->res);
        wp = wp->next;
    }
}
/* TODO: Implement the functionality of watchpoint */
