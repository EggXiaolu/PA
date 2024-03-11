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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

#define STACKLENGTH 1024

int stack[STACKLENGTH];
int top = STACKLENGTH;

/*判断栈是否为空*/
int empty() {
    if (top == STACKLENGTH)
        return 1;
    return 0;
}

/*进栈*/
void push(char ch) {
    top--;
    stack[top] = ch;
}

/*出栈*/
char pop() {
    char ch = stack[top];
    top++;
    return ch;
}

enum {
    TK_NOTYPE = 256,
    TK_NUM = 1,
    TK_EQ,
    /* TODO: Add more token types */

};

static struct rule {
    const char *regex;
    int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE},  // spaces
    {"==", TK_EQ},      // equal
    {"\\+", '+'},       // plus
    {"-", '-'},         // sub
    {"\\*", '*'},       // multiplication
    {"\\/", '/'},       // division
    {"\\(", '('},       // left-parenthesis
    {"\\)", ')'},       // right-parenthesis
    {"[0-9]+", TK_NUM}, // integer
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

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                    i, rules[i].regex, position, substr_len, substr_len,
                    substr_start);

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
    while (n < q) {
        int type = tokens[n].type;
        switch (type) {
        case '(':
            push(type); // 找到左括号 进栈
            break;
        case ')':
            if (empty()) {
                return 0;
            } else {
                pop(); // 出栈
                if (empty()) {
                    return 0;
                }
            }
            break;
        default:
            break;
        }
        n++;
    }
    pop();
    if (empty()) {
        return 1;
    }
    return 0;
}

u_int32_t eval(int p, int q) {
    top = STACKLENGTH;
    int ret_parentheses = check_parentheses(p, q);
    if (p > q) {
        return 0;
    } else if (p == q) {
        return atoi(tokens[p].str);
    } else if (ret_parentheses == 1) {
        return eval(p + 1, q - 1);
    } else if (ret_parentheses == -1) {
        printf("WRONG EXPRESSION.\n");
        assert(0);
    } else {
        int op = -1;
        top = STACKLENGTH;
        for (int i = p; i <= q; i++) {
            // 处理括号
            if (tokens[i].type == '(') {
                push(tokens[i].type);
            } else if (tokens[i].type == ')') {
                pop();
            }

            // 处理运算符
            if (!empty()) {
                continue;
            }
            if ((tokens[i].type == '*' || tokens[i].type == '/') &&
                (op == -1 || tokens[op].type == '*' ||
                 tokens[op].type == '/')) {
                op = i;
            } else if ((tokens[i].type == '+' || tokens[i].type == '-')) {
                op = i;
            }
        }
        if (op == -1) {
            assert(0);
        }
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
    *success = true;
    return eval(0, nr_token - 1);
}
