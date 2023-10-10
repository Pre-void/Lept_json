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
#define EXPECT_EQ_DOUBLE(expect,actual) EXPECT_EQ_BASE((expect) == (actual),expect,actual,"%.17g")

/**这个宏是为了测试非法值，用于代替原本的test_parse_invaild_value()
 *                               test_parse_expect_value()
 *                               test_parse_root_not_singular()
 *                        以及新的test_parse_number_too_big()   **/
#define TEST_ERROR(error,json) \
    do{                        \
        lept_value v;          \
        v.type = LEPT_FALSE;   \
        EXPECT_EQ_INT(error,lept_parse(&v,json)); \
        EXPECT_EQ_INT(LEPT_NULL,lept_get_type(&v)); \
        }while(0)

/**第一个EXPECT_EQ_INT判断数字是否合法，并将字符串转为浮点数赋给v->n，这点在lept_parse_number中
 * 第二个EXPECT_EQ_INT判断是不是数字类型
 * EXPECT_EQ_DOUBLE判断字符串转成的浮点数和实际的数是否一致**/
#define TEST_NUMBER(expect,json) \
    do{                          \
        lept_value v;            \
        EXPECT_EQ_INT(LEPT_PARSE_OK,lept_parse(&v,json)); \
        EXPECT_EQ_INT(LEPT_NUMBER,lept_get_type(&v));     \
        EXPECT_EQ_DOUBLE(expect,lept_get_number(&v));\
    }while(0)


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
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE,"");
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE," ");

}
/** 测试  LEPT_PARSE_INVALID_VALUE**/
static void test_parse_invalid_value(){
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE,"nul");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE,"?");

    TEST_ERROR(LEPT_PARSE_INVALID_VALUE,"+0");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE,".123");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE,"1.");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE,"INF");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE,"inf");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE,"NAN");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE,"nan");
}
/**测试  LEPT_PARSE_ROOT_NOT_SINGULAR**/
static void test_parse_root_not_singular(){
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR,"null x");


    /**invalid number**/
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR,"0123");  /**0后面应该什么都没有或者是小数点**/
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR,"0x0");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR,"0x123");
}


static void test_parse_number(){
    TEST_NUMBER(0.0,"0");
    TEST_NUMBER(0.0,"-0");
    TEST_NUMBER(0.0,"-0.0");
    TEST_NUMBER(1.0,"1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

static void test_parse_number_too_big(){
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG,"1e309");
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG,"-1e309");
}

static void test_parse(){
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
}

int main(){
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n",test_pass,test_count,test_pass * 100.0 / test_count);

//    printf("************%d\n",LEPT_PARSE_EXPECT_VALUE);

    return main_ret;
}