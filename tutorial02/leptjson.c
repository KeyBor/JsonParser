#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <string.h>
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}



//通过一个函数集合true、false、null的解析
static int lept_parse_literal(lept_context* c, lept_value* v,const char* literal,lept_type type) {
    EXPECT(c, literal[0]);
    size_t i;
    for (i = 0; literal[i + 1] != '\0'; i++)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    //解决原本代码出现字符串比期望字符串长并且非空格的情况
    if(*(c->json) != ' ' && *(c->json) != '\0') return LEPT_PARSE_INVALID_VALUE;
    v->type = type;
    return LEPT_PARSE_OK;
}
//static int lept_parse_true(lept_context* c, lept_value* v) {
//    EXPECT(c, 't');
//    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
//        return LEPT_PARSE_INVALID_VALUE;
//    c->json += 3;
//    v->type = LEPT_TRUE;
//    return LEPT_PARSE_OK;
//}
//
//static int lept_parse_false(lept_context* c, lept_value* v) {
//    EXPECT(c, 'f');
//    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
//        return LEPT_PARSE_INVALID_VALUE;
//    c->json += 4;
//    v->type = LEPT_FALSE;
//    return LEPT_PARSE_OK;
//}
//
//static int lept_parse_null(lept_context* c, lept_value* v) {
//    EXPECT(c, 'n');
//    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
//        return LEPT_PARSE_INVALID_VALUE;
//    c->json += 3;
//    v->type = LEPT_NULL;
//    return LEPT_PARSE_OK;
//}



static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    if (!ISDIGIT(*(c->json)) && *(c->json) != '-') {
        return LEPT_PARSE_INVALID_VALUE;
    }
    char* temp = c->json;
    //是负号直接跳过
    if (*temp == '-') ++temp;
    //如果不是数字直接报错
    if (!ISDIGIT(*temp)) return LEPT_PARSE_INVALID_VALUE;
    //如果是0,看后面是否是点或者e,E,就继续
    if (!ISDIGIT1TO9(*temp)) {
        ++temp;
    }
    else {
        for (temp; ISDIGIT(*temp); ++temp);
    }
    //如果结束,符合要求,如果是点,进入逻辑判断
    if (*temp == '.') {
        //指针前移,如果点后无数字,报错
        ++temp;
        if (*(temp++) == '\0') return LEPT_PARSE_INVALID_VALUE;
        //点后有数字,指针往前移,跳过数字部分
        for (temp; ISDIGIT(*temp); temp++);
    }
    //如果数字部分后面是e或E,进入逻辑
    if (*temp == 'e' || *temp == 'E') {
        ++temp;
        //看e后面是不是'-'或者'+'
        if (*temp == '-' || *temp == '+') ++temp;
        if (!ISDIGIT(*temp)) return LEPT_PARSE_INVALID_VALUE;
        for (temp; ISDIGIT(*temp); ++temp);
        if (*temp != '\0') return LEPT_PARSE_INVALID_VALUE;
    }

 
    /* \TODO validate number */
    //str -- 要转换为双精度浮点数的字符串。
    //endptr -- 对类型为 char* 的对象的引用，其值由函数设置为 str 中数值后的下一个字符,没有执行有效转换则返回0
    //stdtod可以直接转换科学计数法
    //如果校验成功就用q指针替换end,校验成功一定是到了字符串末尾,'\0'
    errno = 0;
    v->n = strtod(c->json, NULL);
    //数字过大处理
    if (errno == ERANGE && v->n == HUGE_VAL) return LEPT_PARSE_NUMBER_TOO_BIG;
    //这个地方是针对字符串校验是否到末尾,到末尾了如果不是'\0',则交由上一级函数,返回not_singular的error
    c->json = temp;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}



static int lept_parse_value(lept_context* c, lept_value* v) {
    //解引用运算的优先级，低于箭头运算符与点运算符
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
