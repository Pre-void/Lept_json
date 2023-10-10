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

/**由于 lept_value 内使用了自身类型的指针，我们必须前向声明（forward declare）此类型。**/
typedef struct lept_value lept_value;
typedef struct lept_member lept_member;
struct lept_value{ /**一个json节点**/
    union{

        struct {  /**这个是对象 object**/
            lept_member * m; size_t size;
        }o;

        struct {  /**这个是数组 array**/
            lept_value * e; size_t size;
        }a;

        struct {  /**这个是字符串 string**/
            char * s;
            size_t len;
        }s;
        double n;
    }u;
    lept_type type;
};

/**成员结构 lept_member 是一个 lept_value 加上键的字符串。如同 JSON 字符串的值，
 * 我们也需要同时保留字符串的长度，因为字符串本身可能包含空字符 \u0000。**/
struct lept_member{ /**对象成员**/
    char * k;       /**成员的键 key**/
    size_t klen;    /**成员的键的长度***/
    lept_value v;   /**成员的键对应的属性 value***/
};

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

/**获取json数组的大小**/
size_t lept_get_array_size(const lept_value * v);
/**获取json数组中在index位次的的元素**/
lept_value * lept_get_array_element(const lept_value * v,size_t index);


size_t lept_get_object_size(const lept_value * v);
const char * lept_get_object_key(const lept_value * v,size_t index);
size_t  lept_get_object_key_length(const lept_value * v,size_t index);
lept_value  * lept_get_object_value(const lept_value * v,size_t index);

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,  //若一个 JSON 只含有空白，传回 LEPT_PARSE_EXPECT_VALUE
    LEPT_PARSE_INVALID_VALUE,   //若值不是那三种字面值，传回 LEPT_PARSE_INVALID_VALUE。
    LEPT_PARSE_ROOT_NOT_SINGULAR,  //若一个值之后，在空白之后还有其他字符，传回 LEPT_PARSE_ROOT_NOT_SINGULAR。 比如  "null x"
    LEPT_PARSE_NUMBER_TOO_BIG,  //数字类型太大
    LEPT_PARSE_MISS_QUOTATION_MARK,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR,
    LEPT_PARSE_INVALID_UNICODE_HEX,  /**应该是错误的16进制数**/
    LEPT_PARSE_INVALID_UNICODE_SURROGATE,   /***只有高代理欠缺低代理，或者低代理项不在合法码点范围内**/
    LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    LEPT_PARSE_MISS_KEY,
    LEPT_PARSE_MISS_COLON,
    LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

#endif //JSON_LEPTJSON_H
