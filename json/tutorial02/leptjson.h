//
// Created by LZH on 2023/2/16.
//

#ifndef JSON_LEPTJSON_H
#define JSON_LEPTJSON_H

typedef enum {LEPT_NULL,LEPT_FALSE,LEPT_TRUE,LEPT_NUMBER,LEPT_STRING,LEPT_ARRAY,LEPT_OBJECT } lept_type;

typedef struct {
    lept_type type;
    double n;
}lept_value;




/**解析函数**/
int lept_parse(lept_value * v,const char * json);

/**获取类型**/
lept_type lept_get_type(const lept_value * v);

/**获取数字类型json的值**/
double lept_get_number(const lept_value * v);

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,  //若一个 JSON 只含有空白，传回 LEPT_PARSE_EXPECT_VALUE
    LEPT_PARSE_INVALID_VALUE,   //若值不是那三种字面值，传回 LEPT_PARSE_INVALID_VALUE。
    LEPT_PARSE_ROOT_NOT_SINGULAR,  //若一个值之后，在空白之后还有其他字符，传回 LEPT_PARSE_ROOT_NOT_SINGULAR。 比如  "null x"
    LEPT_PARSE_NUMBER_TOO_BIG  //数字类型太大
};

#endif //JSON_LEPTJSON_H
