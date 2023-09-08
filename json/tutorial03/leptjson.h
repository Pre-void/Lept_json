//
// Created by LZH on 2023/2/16.
//

#ifndef JSON_LEPTJSON_H
#define JSON_LEPTJSON_H

#include "stddef.h"   //  size_t

typedef enum {LEPT_NULL,LEPT_FALSE,LEPT_TRUE,LEPT_NUMBER,LEPT_STRING,LEPT_ARRAY,LEPT_OBJECT } lept_type;


/**因为一个值不可能同时是数字和字符串，使用c语言的union来节省内存
 * union内的字段共用一块内存，同时只能有一个成员变量可以存在，也就是说各个字段的使用是互斥的，任意时刻只能使用一个
 * https://blog.csdn.net/m0_53263647/article/details/126800359**/
typedef struct {
    union{
        struct {
            char * s;
            size_t len;
        }s;
        double n;
    }u;
    lept_type type;
}lept_value;

#define lept_init(v)  \
            do{ (v)->type = LEPT_NULL;           \
            }while(0)

/**解析函数**/
int lept_parse(lept_value * v,const char * json);

/**清空当前json内存**/
void lept_free(lept_value * v);

/**这个API的作用是将当前json类型设置为null，lept_free最后会将v的类型设置为LEPT_NULL类型，直接用lept_free就可以了**/
#define lept_set_null(v)  lept_free(v)

/**获取类型**/
lept_type lept_get_type(const lept_value * v);

/**获取true和false**/
int lept_get_boolean(const lept_value * v);
/**设置true和false**/
void lept_set_boolean(lept_value * v,int b);

/**获取数字类型json的值**/
double lept_get_number(const lept_value * v);
/**设置数字类型json的值**/
void lept_set_number(lept_value * v,double n);


/**获取字符串**/
const char * lept_get_string(const lept_value * v);
/**获取字符串长度**/
size_t lept_get_string_length(const lept_value * v);
/**设置字符串**/
void lept_set_string(lept_value * v,const char * s,size_t len);

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,  //若一个 JSON 只含有空白，传回 LEPT_PARSE_EXPECT_VALUE
    LEPT_PARSE_INVALID_VALUE,   //若值不是那三种字面值，传回 LEPT_PARSE_INVALID_VALUE。
    LEPT_PARSE_ROOT_NOT_SINGULAR,  //若一个值之后，在空白之后还有其他字符，传回 LEPT_PARSE_ROOT_NOT_SINGULAR。 比如  "null x"
    LEPT_PARSE_NUMBER_TOO_BIG,  //数字类型太大
    LEPT_PARSE_MISS_QUOTATION_MARK,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR
};

#endif //JSON_LEPTJSON_H
