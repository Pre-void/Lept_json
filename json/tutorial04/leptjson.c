//
// Created by LZH on 2023/2/16.
//

//#ifdef _WINDOWS
//#define _CRTDBG_MAP_ALLOC
//#include <crtdbg.h>
//#endif

#include "leptjson.h"
#include "assert.h"
#include "stdlib.h"
#include "errno.h"
#include "math.h"
#include "string.h"

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

/** ->
 * 优先级比  *   高**/
#define EXPECT(c,ch) \
            do{                         \
             assert(*c->json == (ch));  \
             c->json++;                 \
             }while(0)
#define ISDIGIT(ch)     ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

/**lept_context_push返回的是新的要插入的字符串在stack中的地址，因为是void* 类型的，要转成char*类型，
 * 然后将再* 就是将这个地址的内容取出，设置为ch**/
#define PUTC(c,ch)  do{ *(char*)lept_context_push(c,sizeof(char)) = (ch); }while(0)

typedef struct {
    const char * json;
    char * stack;
    size_t size,top;   /**size是整个stack的大小，top是最后一个字符串的结尾**/
}lept_context;

static void * lept_context_push(lept_context * c,size_t size){
    void * ret;
    assert(size > 0);
    if(c->top + size >= c->size){
        if(c->size == 0){
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        }
        while (c->top + size >= c->size){
            c->size += c->size >> 1;  /** c->size * 1.5 **/
        }
        c->stack = (char*) realloc(c->stack,c->size);  /**返回重新申请的stack的首地址**/
    }
    ret = c->stack + c->top;  /**返回的应该是插入的字符串的起始位置**/
    c->top += size;           /**新的c->top应该是插入的字符串的末尾，也就是下一个的头部**/
    return ret;
}

static void * lept_context_pop(lept_context * c,size_t size){
    assert(c->top >= size);        /**栈顶是否大于size**/
    return c->stack + (c->top -= size);  /**返回的是栈的起始地址加上新的栈顶，新的栈顶就是原来的栈顶减去size**/
}


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
    if(*p == '0') p++;
    else{
        if(!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++;ISDIGIT(*p);p++);
    }
    if (*p == '.'){
        p++;
        if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p);p++);
    }
    if(*p == 'e' || *p == 'E'){
        p++;
        if(*p == '+' || *p == '-') p++;
        if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p);p++);
    }
    errno = 0;
    v->u.n = strtod(c->json,NULL);  /**将字符串转换成浮点数，后一个参数是若遇见不合法的字符返回不合法的字符串，例如1234.567qwer,第二个参数返回qwer**/
    if(errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n ==-HUGE_VAL)){
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    v->type = LEPT_NUMBER;
    c->json = p;
    return LEPT_PARSE_OK;
}
/**读取四位16进制数字
 * eg.   \u0024   ------>   0000 0000 0010 0100***/
static const char * lept_parse_hex4(const char * p,unsigned * u){
    *u = 0;
    for (int i = 0; i < 4; ++i) {
        char ch = *p++;
        *u <<= 4;
        if (ch >= '0' && ch <= '9')  *u |= ch - '0';
        else if(ch >= 'A' && ch <= 'F') *u |= ch - ('A' - 10);  /** +10是因为A到F是10到15**/
        else if(ch >= 'a' && ch <= 'f') *u |= ch - ('a' - 10);  /**同上**/
        else return NULL;
    }
    return p;
}

static void lept_encode_utf8(lept_context * c,unsigned u){
    /****
    码点范围 	码点位数 	字节1 	字节2 	字节3 	字节4
    U+0000 ~ U+007F 	7 	0xxxxxxx    **/
    if(u <= 0x7F)
        PUTC(c,u & 0xFF);
    else if(u <= 0x7FF){
        /**U+0080 ~ U+07FF 	11 	110xxxxx 	10xxxxxx***/
        PUTC(c,0xC0 | ((u >> 6) & 0xFF));
        PUTC(c,0x80 | ( u       & 0x3F));
    }
    else if(u <= 0xFFFF){
        /**U+0800 ~ U+FFFF 	16 	1110xxxx 	10xxxxxx 	10xxxxxx
    **/
        PUTC(c,0xE0 | ((u >> 12) & 0xFF));
        PUTC(c,0x80 | ((u >> 6)  & 0x3F));
        PUTC(c,0x80 | ( u        & 0x3F));
    }else{
        /**U+10000 ~ U+10FFFF 	21 	11110xxx 	10xxxxxx 	10xxxxxx 	10xxxxxx**/
        PUTC(c,0xF0 | ((u >> 18) & 0xFF));
        PUTC(c,0x80 | ((u >> 12) & 0x3F));
        PUTC(c,0x80 | ((u >> 6)  & 0x3F));
        PUTC(c,0x80 | ( u        & 0x3F));
    }


}

#define STRING_ERROR(ret) do { c->top = head; return ret; }while(0)

static int lept_parse_string(lept_context * c,lept_value * v){
    size_t head = c->top,len;  /**len和head是并列的，不是赋值给head**/
    unsigned u,u2;
    const char * p;
    EXPECT(c,'\"');

//    char temp = *c->json;
//    assert(*c->json == ('\"'));
//    c->json++;

    p = c->json;
    for (;;) {
        char ch = *p++;  /**先*再++**/
        switch (ch) {
            case '\"':   /**这个是第二个"也就是字符串结尾的"，到这里字符串就结束了**/
                len = c->top - head;
                lept_set_string(v,(const char *) lept_context_pop(c,len),len);
                c->json = p;
                return LEPT_PARSE_OK;
            case '\\':
                switch (*p++) {
                    case '\"' : PUTC(c,'\"'); break;  /**\"转义后是一个"**/
                    case '\\' : PUTC(c,'\\'); break;  /**\\转义后是一个\**/
                    case '/' :  PUTC(c,'/');  break;
                    case 'b' :  PUTC(c,'\b'); break;   /**退格(BS) ，将当前位置移到前一列**/
                    case 'f' :  PUTC(c,'\f'); break;   /**换页(FF)，将当前位置移到下页开头**/
                    case 'n' :  PUTC(c,'\n'); break;   /**换行(LF) ，将当前位置移到下一行开头**/
                    case 'r' :  PUTC(c,'\r'); break;   /**回车(CR) ，将当前位置移到本行开头**/
                    case 't' :  PUTC(c,'\t'); break;   /**水平制表(HT) （跳到下一个TAB位置）**/
                    case 'u' :
                        if(!(p = lept_parse_hex4(p,&u)))
                            /**解析不成功的话，说明这个4位16进制数不合法***/
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                        /**遇见高代理项**/
                        if(u >= 0xD800 && u <= 0xD8FF){
                            if(*p++ != '\\')
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            if(*p++ != 'u')
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            if(!(p = lept_parse_hex4(p,&u2)))
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                            if(u2 < 0xDC00 || u2 > 0XDFFF)
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            /**codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)**/
                            u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                        }
                        lept_encode_utf8(c,u);
                        break;
                    default  :
                        /**对其他不合法的字符返回LEPT_PARSE_INVALID_STRING_ESCAPE**/
                        STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
                }
                break;
            case '\0':   /**应该是缺失配对的\"**/
                STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
                break;
            default:
                if((unsigned  char) ch < 0x20){
                    /**处理不合法字符
                        * unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
                        * 当中空缺的 %x22 是双引号，%x5C 是反斜线，都已经处理。所以不合法的字符是 %x00 至 %x1F。**/
                    STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
                }
                PUTC(c,ch);
        }
    }
}


static int lept_parse_value(lept_context * c,lept_value * v){
    switch (*c->json) {
        case 'n'  : return lept_parse_literal(c,v,"null",LEPT_NULL);
        case 't'  : return lept_parse_literal(c,v,"true",LEPT_TRUE);
        case 'f'  : return lept_parse_literal(c,v,"false",LEPT_FALSE);
        default   : return lept_parse_number(c,v);
        case '"'  : return lept_parse_string(c,v);
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
    c.stack = NULL;
    c.size = c.top = 0;
    lept_init(v);  /**代替了下面注释的这行代码,初始化lept_value的type为LEPT_NULL**/
    //v->type = LEPT_NULL;   //虽然lept_parse_value里的null，true，false设置了v->type，但在这里提前设置LEPT_NULL的目的是测试错误情况，即enum的后三种
    lept_parse_whitespace(&c);
    if((ret = lept_parse_value(&c,v)) == LEPT_PARSE_OK){   //解析完value，将lept_parse_value的值返回给ret，并比较与LEPT_PARSE_OK是否一致，若一致就接着解析下一个ws，否则直接返回ret
        lept_parse_whitespace(&c);
        if(*c.json != '\0'){  //若检查完第二个ws后当前指针指向不是"\0"即字符串结尾，说明json文本还没有结束，报错
            v->type = LEPT_NULL;   /**因为检查任意类型时，若出现LEPT_PARSE_ROOT_NOT_SINGULAR错误，此时v->type是当前类型，不一定是LEPT_NULL，要转成LEPT_NULL，否则下一部检查type时会出错**/
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);
    free(c.stack);
    return ret;

}

/**释放字符串类型的内存空间**/
void lept_free(lept_value * v){
    assert(v != NULL);
    if(v->type == LEPT_STRING){
        free(v->u.s.s);
    }
    v->type = LEPT_NULL;
}

int lept_get_boolean(const lept_value * v){
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value * v,int b){
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}



/**获取lept_value的类型**/
lept_type lept_get_type(const lept_value * v){
    assert(v != NULL);
    return v->type;
}
/**获取数字类型的json数据的值**/
double lept_get_number(const lept_value * v){
    //assert(v != NULL && v->type != LEPT_NUMBER);    // ==啊sb
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value * v,double n){
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}

const char * lept_get_string(const lept_value * v){
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}

size_t lept_get_string_length(const lept_value * v){
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

void lept_set_string(lept_value * v,const char * s,size_t len){
    assert(v != NULL && (s != NULL || len == 0));  /** v不能是NULL，s若不是NULL，那么长度要是0**/
    lept_free(v);
    v->u.s.s = (char *) malloc(len+1);
    memcpy(v->u.s.s,s,len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}