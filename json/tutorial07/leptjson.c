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
#include "stdio.h"
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
/**插入字符串，作用同上**/
#define PUTS(c,s,len) memcpy(lept_context_push(c,len),s,len)


typedef struct {
    const char * json;  /**存储要解析的字符串**/
    char * stack;       /**存储解析后的字符串**/
    size_t size,top;    /**size是整个stack的大小，top是最后一个字符串的结尾**/
}lept_context;


/***只移动指针，不向stack中添加内容，添加内容由PUTC进行**/
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


static int lept_parse_string_raw(lept_context * c,char  ** str,size_t * len){
    size_t head = c->top;
    unsigned u,u2;  /**代理项***/
    const char * p;
    EXPECT(c,'\"');/**记得转义**/
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                *len = c->top - head; /**c->top由PUTC也就是lept_context_push改变**/
                *str = lept_context_pop(c,*len);
                c->json = p;
                return LEPT_PARSE_OK;
            case '\\':
                switch (*p++) {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/' :  PUTC(c, '/' ); break;
                    case 'b' :  PUTC(c, '\b'); break;
                    case 'f' :  PUTC(c, '\f'); break;
                    case 'n' :  PUTC(c, '\n'); break;
                    case 'r' :  PUTC(c, '\r'); break;
                    case 't' :  PUTC(c, '\t'); break;
                    case 'u' :
                        if(!(p = lept_parse_hex4(p,&u)))
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                        if(u >= 0xD800 && u <= 0xDBFF){
                            if(*p++ != '\\'){
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            }
                            if(*p++ != 'u'){
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            }
                            if(!(p = lept_parse_hex4(p,&u2))){  /**return NULL的情况**/
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                            }
                            if(u2 < 0xDC00 || u2 > 0xDFFF){
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            }
                            u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                        }
                        lept_encode_utf8(c,u);
                        break;
                    default:
                        STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
                }
                break;  /**case后都会加的break**/
            case '\0':
                STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
                break;
            default:
                if((unsigned char)ch < 0x20)
                    STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
                PUTC(c,ch);
        }
    }
}

static int lept_parse_string(lept_context * c,lept_value * v){
    int ret;
    char * s; /**现在是空的，经过lept_parse_string_raw后  s被赋值**/
    size_t len;
    if((ret = lept_parse_string_raw(c,&s,&len)) == LEPT_PARSE_OK)
        lept_set_string(v,s,len);
    return ret;
}

/**提前声明***/
static int lept_parse_value(lept_context * c,lept_value * v);

static int lept_parse_object(lept_context * c,lept_value * v){
    size_t size;
    lept_member m;
    int ret;
    EXPECT(c,'{');
    lept_parse_whitespace(c);
    if(*c->json == '}'){  /**空对象的情况**/
        c->json++;
        v->type = LEPT_OBJECT;
        v->u.o.m = 0; /**因为是空对象，lept_value的menmber是指针，但是第二行的lept_member是临时对象，若取&m，m释放后，就成了悬空指针**/
        v->u.o.size = 0;
        return LEPT_PARSE_OK;
    }
    m.k = NULL;
    size = 0;
    for (;;) {
        char * str;
        lept_init(&m.v);
        if(*c->json != '"'){ /**因为key是字符串，第一个字符不是"那就说明不是字符串，也就没有key**/
            ret = LEPT_PARSE_MISS_KEY;
            break;
        }
        /**c和str经过lept_parse_string_raw被赋值**/
        if((ret = lept_parse_string_raw(c,&str,&m.klen)) != LEPT_PARSE_OK)
            break;
        /**将str的内容拷贝到m的k**/
        memcpy(m.k = (char *) malloc(m.klen+1),str,m.klen);
        m.k[m.klen] = '\0';
        lept_parse_whitespace(c);

        if(*c->json != ':'){
            ret = LEPT_PARSE_MISS_COLON;
            break;
        }
        c->json++;
        lept_parse_whitespace(c);

        if((ret = lept_parse_value(c,&m.v)) != LEPT_PARSE_OK)
            break;
        memcpy(lept_context_push(c,sizeof(lept_member)),&m, sizeof(lept_member));
        size++;
        m.k = NULL;
        lept_parse_whitespace(c);
        if(*c->json == ','){
            c->json++;
            lept_parse_whitespace(c);
        } else if(*c->json == '}'){
            size_t s = sizeof(lept_member) * size;
            c->json++;
            v->type = LEPT_OBJECT;
            v->u.o.size = size;
            memcpy(v->u.o.m = (lept_member *) malloc(s), lept_context_pop(c,s),s);
            return LEPT_PARSE_OK;
        } else{
            ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }
    /**最后，当 for (;;) 中遇到任何错误便会到达这第 5 步，要释放临时的 key 字符串及栈上的成员：**/
    free(m.k);
    for (int i = 0; i < size; ++i) {
        lept_member * m = lept_context_pop(c, sizeof(lept_member));
        free(m->k);
        lept_free(&m->v);
    }
    v->type = LEPT_NULL;

    return ret;
}

static int lept_parse_array(lept_context * c,lept_value * v){
    size_t i,size = 0;
    int ret;
    EXPECT(c,'[');
    lept_parse_whitespace(c);
    if(*c->json == ']'){  /**空数组的情况**/
        c->json++;
        v->type = LEPT_ARRAY;
        v->u.a.size = 0;
        v->u.a.e = NULL;
        return LEPT_PARSE_OK;
    }

    for (;;) {
        lept_value e;  /**暂存数组中的当前元素**/
        lept_init(&e);
        /**解析当前元素，并将当前元素解析至e**/
        if((ret = lept_parse_value(c,&e)) != LEPT_PARSE_OK)
            break;
        /**将当前lept_value压入stack**/
        /**你觉得是错的！！！！！     我觉得这个sizeof(lept_value)不太对，因为e的大小可能大于24
         * lept_value存的是指针，不是字符串或者数组本身啦！！！！！，大小就是24啦！！！！**/
        memcpy(lept_context_push(c,sizeof(lept_value)),&e, sizeof(lept_value));
        size++;/**元素个数+1 ***/
        lept_parse_whitespace(c);/**c在这里就是指针***/
        if(*c->json == ',') {
            c->json++;
            lept_parse_whitespace(c);
        }else if(*c->json == ']'){
            c->json++;
            v->type = LEPT_ARRAY;
            v->u.a.size = size;
            size *= sizeof(lept_value);
            memcpy(v->u.a.e = (lept_value*) malloc(size), lept_context_pop(c,size),size);
            return LEPT_PARSE_OK;
        } else{
            ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }
    /**中途解析失败，弹出之前压入stack的元素**/
    for (int j = 0; j < size; ++j) {
        lept_free((lept_value *) lept_context_pop(c, sizeof(lept_value)));
    }
    return ret;
}




static int lept_parse_value(lept_context * c,lept_value * v){
    switch (*c->json) {
        case 'n'  : return lept_parse_literal(c,v,"null",LEPT_NULL);
        case 't'  : return lept_parse_literal(c,v,"true",LEPT_TRUE);
        case 'f'  : return lept_parse_literal(c,v,"false",LEPT_FALSE);
        default   : return lept_parse_number(c,v);
        case '"'  : return lept_parse_string(c,v);
        case '['  : return lept_parse_array(c,v);
        case '{'  : return lept_parse_object(c,v);
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
    size_t i;
    assert(v != NULL);
    switch (v->type) {
        case LEPT_STRING:
            free(v->u.s.s);
            break;
        case LEPT_ARRAY:
            for (i = 0; i < v->u.a.size; ++i) {
                lept_free(&v->u.a.e[i]);
            }
            free(v->u.a.e);
            break;
        case LEPT_OBJECT:
            for (i = 0; i < v->u.o.size; ++i) {
                free(v->u.o.m[i].k);
                lept_free(&v->u.o.m[i].v);
            }
            free(v->u.o.m);
            break;
        default:
            break;
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

size_t lept_get_array_size(const lept_value * v){
    assert(v != NULL && v->type == LEPT_ARRAY);
    return v->u.a.size;
}

lept_value * lept_get_array_element(const lept_value * v,size_t index){
    assert(v != NULL && v->type == LEPT_ARRAY);
    assert(index < v->u.a.size);
    return &v->u.a.e[index];  /**在这里e应该是数组名的作用,e[index]是第index个元素，是lept_value类型，但返回值要求是 lept_value *，所以取地址**/
}

/**获取对象个数**/
size_t lept_get_object_size(const lept_value * v){
    assert(v != NULL && v->type == LEPT_OBJECT);
    return v->u.o.size;
}
/**获取次序在index的成员的key**/
const char * lept_get_object_key(const lept_value * v,size_t index){
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].k;
}

/**获取次序在index的成员的key的长度**/
size_t lept_get_object_key_length(const lept_value * v,size_t index){
    assert(v != NULL && v->type==LEPT_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].klen;
}

/**获取次序在index的成员的value**/
lept_value * lept_get_object_value(const lept_value * v,size_t index){
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert(index < v->u.o.size);
    return &v->u.o.m[index].v;
}

#ifndef LEPT_PARSE_STRINGIFY_INIT_SIZE
#define LEPT_PARSE_STRINGIFY_INIT_SIZE 256
#endif

#if 0

/**转义string类型数据**/
static void lept_stringify_string(lept_context * c,const char * s,size_t len){
    size_t i;
    assert(s != NULL);
    PUTC(c,'"');
    for (i = 0; i < len; ++i) {
        /**在C语言中，字符 char 的有符号性质是不确定的，它可能是有符号的也可能是无符号的，
         * 这取决于编译器和平台的实现。因此，如果你不明确将字符 s[i] 转换为 unsigned char，
         * 在某些情况下，编译器可能会发出警告，因为它可能会认为你在进行不安全的类型转换。**/
        unsigned char ch = (unsigned char)s[i];
        switch (ch) {
            case '\"' : PUTS(c,"\\\"",2); break;
            case '\\' : PUTS(c,"\\\\",2); break;
            case '\b' : PUTS(c,"\\b", 2); break;
            case '\f' : PUTS(c,"\\f", 2); break;
            case '\n' : PUTS(c,"\\n", 2); break;
            case '\r' : PUTS(c,"\\r", 2); break;
            case '\t' : PUTS(c,"\\t", 2); break;
            default:
                if (ch < 0x20){
                    char  buffer[7];
                    sprintf(buffer,"\\u%04X",ch);
                    PUTS(c,buffer,6);
                } else{
                    /**case和default都PUTS了转义后的字符串，之所以以上需要转义是因为<0x20和特殊字符，
                     * 到了else是合法字符且不特殊，就不需要转义了，直接插入原字符s[i]即可**/
                    PUTC(c,s[i]);
                }
        }
    }
    PUTC(c,'"');
}
#else
/**上面那个经常调用lept_context_push，比较耗内存，这个一次性创建一个足够大的空间来放置字符串，最后按
 * 实际的转义后的字符串的长度来调整c->top**/
static void lept_stringify_string(lept_context * c,const char * s,size_t len){
    static const char hex_digits[] = {'0', '1', '2', '3', '4',
                                      '5', '6', '7', '8', '9',
                                      'A', 'B', 'C', 'D', 'E', 'F'};
    size_t i,size;
    char * head,* p;
    assert(s != NULL);
    /**单个字符最长也就增大6倍，即不合法的情况"\\u%04X"，创建一个size = len * 6 + 2大小的空间即可保证
     * 一定能放下长度为len的string**/
    p = head = lept_context_push(c,size = len * 6 + 2);
    *p++ = '"';
    for (i = 0; i < len; ++i) {
        unsigned char ch = (unsigned char)s[i];
        switch (ch) {
            case '\"' : *p++ = '\\';*p++ = '\"';break;
            case '\\' : *p++ = '\\';*p++ = '\\';break;
            case '\b' : *p++ = '\\';*p++ = 'b';break;
            case '\f' : *p++ = '\\';*p++ = 'f';break;
            case '\n' : *p++ = '\\';*p++ = 'n';break;
            case '\r' : *p++ = '\\';*p++ = 'r';break;
            case '\t' : *p++ = '\\';*p++ = 't';break;
            default:
                if(ch < 0x20){
                    *p++ = '\\';*p++ = 'u';*p++ = '0';*p++ = '0';
                    *p++ = hex_digits[ch >> 4];
                    *p++ = hex_digits[ch & 15];
                } else{
                    *p++ = s[i];
                }
        }
    }
    *p++ = '"';
    /**重新修正c->top,因为
     * p = head = lept_context_push(c,size = len * 6 + 2);
     * 这一句修改了c->top，这个c->top不正确**/
    c->top -= size - (p-head);
}
#endif

/**根据不同的类型，采用不同的方式进行转义存入lept_context***/
static void lept_stringify_value(lept_context* c, const lept_value* v) {
    size_t i;
    switch (v->type) {
        case LEPT_NULL:   PUTS(c, "null",  4); break;
        case LEPT_FALSE:  PUTS(c, "false", 5); break;
        case LEPT_TRUE:   PUTS(c, "true",  4); break;
        case LEPT_NUMBER: c->top -= 32 - sprintf(lept_context_push(c, 32), "%.17g", v->u.n); break;
        case LEPT_STRING: lept_stringify_string(c, v->u.s.s, v->u.s.len); break;
        case LEPT_ARRAY:
            PUTC(c, '[');
            for (i = 0; i < v->u.a.size; i++) {
                if (i > 0)
                    PUTC(c, ',');
                lept_stringify_value(c, &v->u.a.e[i]);
            }
            PUTC(c, ']');
            break;
        case LEPT_OBJECT:
            PUTC(c, '{');
            for (i = 0; i < v->u.o.size; i++) {
                if (i > 0)
                    PUTC(c, ',');
                lept_stringify_string(c, v->u.o.m[i].k, v->u.o.m[i].klen);
                PUTC(c, ':');
                lept_stringify_value(c, &v->u.o.m[i].v);
            }
            PUTC(c, '}');
            break;
        default: assert(0 && "invalid type");
    }
}

/**生成json字符串，将v中的其七种类型的数据进行转义存入lept_context**/
char * lept_stringify(const lept_value * v,size_t * length){
    lept_context c;
    assert(v != NULL);
    c.stack = (char *) malloc(c.size = LEPT_PARSE_STRINGIFY_INIT_SIZE);
    c.top = 0;
    lept_stringify_value(&c,v);
    if(length)
        *length =c.top;
    PUTC(&c,'\0');
    return c.stack;
}