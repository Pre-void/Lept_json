//
// Created by LZH on 2023/2/16.
//
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "leptjson.h"

static int main_ret   = 0;
static int test_count = 0;  //测试总数
static int test_pass  = 0;  //测试通过数


/**
 * 使用do{...}while(0)构造后的宏定义不会受到大括号、分号等的影响，总是会按你期望的方式调用运行。
 * https://www.qb5200.com/article/316194.html
 * https://blog.csdn.net/mijichui2153/article/details/85070135**/

#define EXPECT_EQ_BASE(equality,expect,actual,format) \
    do{                                               \
        test_count++;                                 \
        if(equality)                                  \
            test_pass++;                              \
        else{                                         \
            fprintf(stderr,"%s:%d: expect: " format " actual: " format "\n",__FILE__,__LINE__,expect,actual); \
            /**printf("%d----%d",expect,actual);
            * /home/lzh/CPP/json/json/test.c:37: expect: 0 actual: 2   expect为0是因为LEPT_NULL在枚举中序号为0
            *                                                          actual为2是因为LEPT_TRUE在枚举中序号为2
            **/                                        \
            main_ret = 1;\
        }    \
    }while(0)

#define EXPECT_EQ_INT(expect,actual) EXPECT_EQ_BASE((expect) == (actual),expect,actual,"%d")

static void test_parse_null(){
    lept_value v;
    v.type = LEPT_TRUE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,"null"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));   /**形参是指针，实参写地址**/
}
static void test_parse_true(){
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,"true"));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
}

static void test_parse_false(){
    lept_value v;
    v.type = LEPT_NULL;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,"false"));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
}


/** 测试返回值 LEPT_PARSE_EXPECT_VALUE **/
static void test_parse_expect_value(){
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v,""));  /**lept_parse比较""是否和v是同一类型，返回的结果查看和LEPT_PARSE_INVALID_VALUE是否是同一类型**/
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));

    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v," "));   /**同上**/
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));

}
/** 测试  LEPT_PARSE_INVALID_VALUE**/
static void test_parse_invalid_value(){
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v,"nul"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));

    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v,"?"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}
/**测试  LEPT_PARSE_ROOT_NOT_SINGULAR**/
static void test_parse_root_not_singular(){
    lept_value v;
    v.type = LEPT_FALSE;

    EXPECT_EQ_INT(LEPT_PARSE_ROOT_NOT_SINGULAR, lept_parse(&v,"true x"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse(){
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
}

int main(){
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n",test_pass,test_count,test_pass * 100.0 / test_count);

    printf("************%d\n",LEPT_PARSE_EXPECT_VALUE);

    return main_ret;
}