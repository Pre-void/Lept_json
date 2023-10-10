//
// Created by LZH on 2023/2/16.
//
#include "leptjson.h"
#include "assert.h"
#include "stdlib.h"

/** ->  优先级比  *   高**/
#define EXPECT(c,ch) \
            do{                         \
             assert(*c->json == (ch));  \
             c->json++;                 \
             }while(0)


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
/** null  = "null"  **/
/**  'n'应该是当前指针指到了"null"的第一个 若首字母不是n说明不是null，直接报错 **/
/**若第一个是n，因为EXPECT已经将指针+1，
* 所以0 1 2分别应该是  'u' 'l'  'l'
* 若错误 则返回LEPT_PARSE_INVALID_VALUE**/
static int lept_parse_null(lept_context * c,lept_value * v){
    EXPECT(c, 'n');
    if(c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l'){
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += 3;   /** +3 到下个字符串**/
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;   /**返回解析成功**/
}

static int lept_parse_true(lept_context * c,lept_value * v){
    EXPECT(c, 't');
    if(c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e'){
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json+=3;
    v->type = LEPT_TRUE;
    return LEPT_PARSE_OK;
}


static int lept_parse_false(lept_context * c,lept_value * v){
    EXPECT(c,'f');
    if(c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e'){
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json+=4;
    v->type = LEPT_FALSE;
    return  LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context * c,lept_value * v){
    switch (*c->json) {
        case 'n'  : return lept_parse_null(c,v);
        case 't'  : return lept_parse_true(c,v);
        case 'f'  : return lept_parse_false(c,v);
        case '\0' : return LEPT_PARSE_EXPECT_VALUE;
        default   : return LEPT_PARSE_INVALID_VALUE;
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
    v->type = LEPT_NULL;   //虽然lept_parse_value里的null，true，false设置了v->type，但在这里提前设置LEPT_NULL的目的是测试错误情况，即enum的后三种,若不设置，后面lept_get_type时无法得到LEPT_NULL
    lept_parse_whitespace(&c);
    if((ret = lept_parse_value(&c,v)) == LEPT_PARSE_OK){   //解析完value，将lept_parse_value的值返回给ret，并比较与LEPT_PARSE_OK是否一致，若一致就接着解析下一个ws，否则直接返回ret
        lept_parse_whitespace(&c);
        if(*c.json != '\0'){  //若检查完第二个ws后当前指针指向不是"\0"即字符串结尾，说明json文本还没有结束，报错
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;

}

lept_type lept_get_type(const lept_value * v){

    return v->type;
}
