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



//ͨ��һ����������true��false��null�Ľ���
static int lept_parse_literal(lept_context* c, lept_value* v,const char* literal,lept_type type) {
    EXPECT(c, literal[0]);
    size_t i;
    for (i = 0; literal[i + 1] != '\0'; i++)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    //���ԭ����������ַ����������ַ��������ҷǿո�����
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
    //�Ǹ���ֱ������
    if (*temp == '-') ++temp;
    //�����������ֱ�ӱ���
    if (!ISDIGIT(*temp)) return LEPT_PARSE_INVALID_VALUE;
    //�����0,�������Ƿ��ǵ����e,E,�ͼ���
    if (!ISDIGIT1TO9(*temp)) {
        ++temp;
    }
    else {
        for (temp; ISDIGIT(*temp); ++temp);
    }
    //�������,����Ҫ��,����ǵ�,�����߼��ж�
    if (*temp == '.') {
        //ָ��ǰ��,������������,����
        ++temp;
        if (*(temp++) == '\0') return LEPT_PARSE_INVALID_VALUE;
        //���������,ָ����ǰ��,�������ֲ���
        for (temp; ISDIGIT(*temp); temp++);
    }
    //������ֲ��ֺ�����e��E,�����߼�
    if (*temp == 'e' || *temp == 'E') {
        ++temp;
        //��e�����ǲ���'-'����'+'
        if (*temp == '-' || *temp == '+') ++temp;
        if (!ISDIGIT(*temp)) return LEPT_PARSE_INVALID_VALUE;
        for (temp; ISDIGIT(*temp); ++temp);
        if (*temp != '\0') return LEPT_PARSE_INVALID_VALUE;
    }

 
    /* \TODO validate number */
    //str -- Ҫת��Ϊ˫���ȸ��������ַ�����
    //endptr -- ������Ϊ char* �Ķ�������ã���ֵ�ɺ�������Ϊ str ����ֵ�����һ���ַ�,û��ִ����Чת���򷵻�0
    //stdtod����ֱ��ת����ѧ������
    //���У��ɹ�����qָ���滻end,У��ɹ�һ���ǵ����ַ���ĩβ,'\0'
    errno = 0;
    v->n = strtod(c->json, NULL);
    //���ֹ�����
    if (errno == ERANGE && v->n == HUGE_VAL) return LEPT_PARSE_NUMBER_TOO_BIG;
    //����ط�������ַ���У���Ƿ�ĩβ,��ĩβ���������'\0',������һ������,����not_singular��error
    c->json = temp;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}



static int lept_parse_value(lept_context* c, lept_value* v) {
    //��������������ȼ������ڼ�ͷ�������������
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
