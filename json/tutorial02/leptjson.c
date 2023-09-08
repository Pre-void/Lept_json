//
// Created by LZH on 2023/2/16.
//
#include "leptjson.h"
#include "assert.h"
#include "stdlib.h"
#include "errno.h"
#include "math.h"
/** ->  优先级比  *   高**/
#define EXPECT(c,ch) \
            do{                         \
             assert(*c->json == (ch));  \
             c->json++;                 \
             }while(0)
#define ISDIGIT(ch)     ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char * json;
}lept_context;

/** ws = *(%x20 / %x09 / %x0A / %x0D)
 * 用一个指针p指向当前lept_context指针的内容，判断是否属于空格，若是空格，指针后移，若不是，p返回给lept_context指针的内容
 * **/
static void lept_parse_whitespace(lept_context * c){
    const char * p = c->json;
    while (*p==' '||*p=='\t'||*p=='\n'||*p=='\r'){
        p++;
    }
    c->json = p;
}

/**使用lept_parse_literal代替原本的lept_parse_null()
 *                              lept_parse_true()
 *                              lept_parse_false()**/
static int lept_parse_literal(lept_context * c,lept_value * v,const char * literal,lept_type type){
    size_t i;     //记录TRUE FALSE NULL字符个数
    EXPECT(c,literal[0]);
    for (i = 0;literal[i+1];i++){  //这里是literal[i+1]的原因是EXPECT(c,literal[0])将c->json+1了
        if(c->json[i]!=literal[i+1]){
            return LEPT_PARSE_INVALID_VALUE;
        }
    }
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context * c,lept_value * v){
    const char * p = c->json;
    if(*p == '-') p++;
    if(*p == '0') p++;   /**整数部分如果第一个是0那只能只有一个0**/
    else{                /**处理整数**/
        if(!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;   /**第一个整数一定要是1-9**/
        for (p++;ISDIGIT(*p);p++);      /**后续的整数**/
    }
    if (*p == '.'){   /**小数点**/
        p++;
        if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;  /**小数点后续的数**/
        for (p++; ISDIGIT(*p);p++);
    }
    if(*p == 'e' || *p == 'E'){  /**指数符号**/
        p++;
        if(*p == '+' || *p == '-') p++;  /**指数的正负**/
        if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p);p++);
    }
    errno = 0;
    /***错误码  https://zhuanlan.zhihu.com/p/538783160
     * ERANGE：Error Range，范围错误，计算结果取值过大超出范围，例如对0求对数，其值是负无穷大，超出范围。
     * HUGE_VAL 太大，无法表示**/
    v->n = strtod(c->json,NULL);  /**将字符串转换成浮点数，后一个参数是若遇见不合法的字符返回不合法的字符串，例如1234.567qwer,第二个参数返回qwer**/
    if(errno == ERANGE && (v->n == HUGE_VAL || v->n ==-HUGE_VAL)){
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    v->type = LEPT_NUMBER;
    c->json = p;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context * c,lept_value * v){
    switch (*c->json) {
        case 'n'  : return lept_parse_literal(c,v,"null",LEPT_NULL);
        case 't'  : return lept_parse_literal(c,v,"true",LEPT_TRUE);
        case 'f'  : return lept_parse_literal(c,v,"false",LEPT_FALSE);
        default   : return lept_parse_number(c,v);
        case '\0' : return LEPT_PARSE_EXPECT_VALUE;
    }
}

/****  json文本格式如下 JSON-text = ws value ws
 *   该版本只处理了前面两个部分，即  ws value 没有处理后一个ws
 * @param v
 * @param json
 * @return
int lept_parse(lept_value * v,const char * json){
//    return LEPT_PARSE_OK;

    lept_context c;
    assert(v != NULL);  //是否是空指针
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    return lept_parse_value(&c,v);

}
**/
/** 比前面那个版本更新的后的 **/
int lept_parse(lept_value * v,const char * json){
//    return LEPT_PARSE_OK;

    lept_context c;
    int ret;  //返回值
    assert(v != NULL);  //是否是空指针
    c.json = json;
    v->type = LEPT_NULL;   //虽然lept_parse_value里的null，true，false设置了v->type，但在这里提前设置LEPT_NULL的目的是测试错误情况，即enum的后三种
    lept_parse_whitespace(&c);
    if((ret = lept_parse_value(&c,v)) == LEPT_PARSE_OK){   //解析完value，将lept_parse_value的值返回给ret，并比较与LEPT_PARSE_OK是否一致，若一致就接着解析下一个ws，否则直接返回ret
        lept_parse_whitespace(&c);
        if(*c.json != '\0'){  //若检查完第二个ws后当前指针指向不是"\0"即字符串结尾，说明json文本还没有结束，报错
            v->type = LEPT_NULL;   /**因为检查任意类型时，若出现LEPT_PARSE_ROOT_NOT_SINGULAR错误，此时v->type是当前类型，不一定是LEPT_NULL，要转成LEPT_NULL，否则下一部检查type时会出错**/
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;

}

lept_type lept_get_type(const lept_value * v){
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value * v){
    //assert(v != NULL && v->type != LEPT_NUMBER);    // ==啊sb
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}