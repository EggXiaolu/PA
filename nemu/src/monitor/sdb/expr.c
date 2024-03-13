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

#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

int top = 0;

/*判断栈是否为空*/
int empty() {
    if (top == 0)
        return 1;
    return 0;
}

/*进栈*/
void push() { top++; }

/*出栈*/
void pop() { top--; }

enum {
    TK_NOTYPE = 256,
    TK_EQ,
    TK_NEQ,
    TK_AND,
    TK_OR,
    TK_NO,
    TK_HEX,
    TK_REG,
    TK_NUM,
    TK_NEG,
    TK_DEREF
    /* TODO: Add more token types */

};

static struct rule {
    const char *regex;
    int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE}, // spaces
    {"==", TK_EQ},     // equal
    {"!=", TK_NEQ},    // not equal
    {"&&", TK_AND},    // and
    {"\\|\\|", TK_OR}, // or
    {"!", TK_NO},      // not
    {"\\+", '+'},      // plus
    {"\\-", '-'},      // sub
    {"\\*", '*'},      // multiplication
    {"\\/", '/'},      // division

    {"\\(", '('}, // left-parenthesis
    {"\\)", ')'}, // right-parenthesis

    {"0[xX][0-9a-fA-F]+", TK_HEX}, // memory
    {"\\$[a-z]*[0-9]*", TK_REG},   // register
    {"[0-9]+", TK_NUM},            // integer

};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
    int i;
    char error_msg[128];
    int ret;

    for (i = 0; i < NR_REGEX; i++) {
        ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
        if (ret != 0) {
            regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg,
                  rules[i].regex);
        }
    }
}

typedef struct token {
    int type;
    char str[32];
} Token;

static Token tokens[2048] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e) {
    int position = 0;
    int i;
    regmatch_t pmatch;

    nr_token = 0;
    while (e[position] != '\0') {
        /* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
                pmatch.rm_so == 0) {
                char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;

                // Log("match rules[%d] = \"%s\" at position %d with len %d:
                // %.*s",
                //     i, rules[i].regex, position, substr_len, substr_len,
                //     substr_start);

                position += substr_len;
                /* TODO: Now a new token is recognized with rules[i]. Add
                 * codes to record the token in the array `tokens'. For
                 * certain types of tokens, some extra actions should be
                 * performed.
                 */

                // 重置
                for (int j = 0; j < 32; j++) {
                    tokens[nr_token].str[j] = '\0';
                }

                switch (rules[i].token_type) {
                case TK_NOTYPE:
                    break;
                case '+':
                case '-':
                case '*':
                case '/':
                case '(':
                case ')':
                case TK_AND:
                case TK_OR:
                case TK_NO:
                case TK_EQ:
                case TK_NEQ:
                case TK_HEX:
                case TK_REG:
                case TK_NUM:
                    tokens[nr_token].type = rules[i].token_type;
                    strncpy(tokens[nr_token].str, substr_start, substr_len);
                    nr_token++;
                    break;
                default:
                    assert(0);
                }
                break;
            }
        }

        if (i == NR_REGEX) {
            printf("no match at position %d\n%s\n%*.s^\n", position, e,
                   position, "");
            return false;
        }
    }

    return true;
}

int check_parentheses(int p, int q) {
    if (tokens[p].type != '(' || tokens[q].type != ')') {
        return 0;
    }
    int n = p;
    while (n <= q) {
        int type = tokens[n].type;
        switch (type) {
        case '(':
            push(); // 找到左括号 进栈
            break;
        case ')':
            pop(); // 找到右括号 出栈
            break;
        default:
            break;
        }
        if (n != q && empty()) {
            // 未到结尾为空
            return 0;
        }
        n++;
    }
    if (empty()) {
        return 1;
    }
    return 0;
}

/*
定义主运算符优先级：
    1、or
    2、and
    3、== !=
    4、+ -
    5、* /
    6、negtive,point,not
*/

int primary_op(int p, int q) {
    int op = -1;
    int op_pri = 100;
    top = 0;
    for (int i = p; i <= q; i++) {
        // 处理括号
        if (tokens[i].type == '(') {
            push(tokens[i].type);
        } else if (tokens[i].type == ')') {
            pop();
        }
        if (!empty())
            continue;
        switch (tokens[i].type) {
        case TK_OR:
            if (op_pri >= 1) {
                op = i;
                op_pri = 1;
            }
            break;
        case TK_AND:
            if (op_pri >= 2) {
                op = i;
                op_pri = 2;
            }
            break;
        case TK_EQ:
        case TK_NEQ:
            if (op_pri >= 3) {
                op = i;
                op_pri = 3;
            }
            break;
        case '+':
        case '-':
            if (op_pri >= 4) {
                op = i;
                op_pri = 4;
            }
            break;
        case '*':
        case '/':
            if (op_pri >= 5) {
                op = i;
                op_pri = 5;
            }
        case TK_NEG:
        case TK_DEREF:
        case TK_NO:
            if (op_pri > 6) {
                op = i;
                op_pri = 6;
            }
        default:
            break;
        }
    }
    return op;
}
u_int32_t eval(int p, int q) {
    top = 0;
    int ret_parentheses = check_parentheses(p, q);
    if (p > q) {
        return 0;
    } else if (p == q) {
        switch (tokens[p].type) {
        case TK_NUM:
            return atoi(tokens[p].str);
        case TK_HEX:
            u_int32_t res_hex = 0;
            for (int i = 2; tokens[p].str[i] != '\0'; i++) {
                res_hex *= 16;
                res_hex += tokens[p].str[i] <= '9'
                               ? tokens[p].str[i] - '0'
                               : tokens[p].str[i] - 'a' + 10;
            }
            return res_hex;
        case TK_REG:
            bool *success = malloc(sizeof(bool));
            u_int32_t res_reg = isa_reg_str2val(tokens[p].str + 1, success);
            if (success) {
                return res_reg;
            } else {
                assert(0);
            }
        default:
            assert(0);
        }
    } else if (ret_parentheses == 1) {
        return eval(p + 1, q - 1);
    } else if (ret_parentheses == -1) {
        printf("WRONG EXPRESSION.\n");
        assert(0);
    } else {
        int op = primary_op(p, q);
        if (op == -1) {
            assert(0);
        }
        // printf("%d %c\n", op, tokens[op].type);
        u_int32_t val1 = eval(p, op - 1);
        u_int32_t val2 = eval(op + 1, q);
        switch (tokens[op].type) {
        case '+':
            return val1 + val2;
        case '-':
            return val1 - val2;
        case '*':
            return val1 * val2;
        case '/':
            if (val2 == 0) {
                printf("/0 EROOR!!\n");
                assert(0);
            }
            return val1 / val2;
        case TK_NEG:
            return -val2;
        case TK_DEREF:
            return *guest_to_host(val2);
        case TK_AND:
            return val1 && val2;
        case TK_OR:
            return val1 || val2;
        case TK_EQ:
            return val1 == val2;
        case TK_NEQ:
            return val1 != val2;
        case TK_NO:
            return !val2;
        default:
            assert(0);
        }
    }
}

word_t expr(char *e, bool *success) {
    if (!make_token(e)) {
        *success = false;
        return 0;
    }

    // 处理负数
    for (int i = 0; i < nr_token; i++) {
        if (tokens[i].type == '-' &&
            (i == 0 ||
             !(tokens[i - 1].type == TK_HEX || tokens[i - 1].type == TK_REG ||
               tokens[i - 1].type == TK_NUM || tokens[i - 1].type == '(' ||
               tokens[i - 1].type == ')'))) {
            tokens[i].type = TK_NEG;
        }
    }

    // 处理指针解引用
    for (int i = 0; i < nr_token; i++) {
        if (tokens[i].type == '*' &&
            (i == 0 ||
             !(tokens[i - 1].type == TK_HEX || tokens[i - 1].type == TK_REG ||
               tokens[i - 1].type == TK_NUM || tokens[i - 1].type == '(' ||
               tokens[i - 1].type == ')'))) {
            tokens[i].type = TK_DEREF;
        }
    }

    *success = true;
    return eval(0, nr_token - 1);
}
