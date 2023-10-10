//
// Created by LZH on 2023/2/16.
//

//#ifdef _WINDOWS
//#define _CRTDBG_MAP_ALLOC
//#include <crtdbg.h>
//#endif

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
#define EXPECT_EQ_STRING(expect,actual,alength) \
    EXPECT_EQ_BASE(sizeof(expect)-1==alength && memcmp(expect,actual,alength) == 0,expect,actual,"%s")
/**我觉得下面这两个的倒数第二个参数应该是actual，但是EXPECT_EQ_BASE只看equality也就是第一个参数，无所谓了**/
#define EXPECT_TRUE(actual)  EXPECT_EQ_BASE((actual) != 0,"true","false","%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0,"false","true","%s")

/**C 语言的数组大小应该使用 size_t 类型。因为我们要验证
 lept_get_array_size() 返回值是否正确，所以再为单元测试框架添加一个宏 EXPECT_EQ_SIZE_T
 ANSI C（C89）并没有的 size_t 打印方法，在 C99 则加入了 "%zu"，但 VS2015 中才有，之前的
 VC 版本使用非标准的 "%Iu"。因此，上面的代码使用条件编译去区分 VC 和其他编译器。虽然这部分不跨平台也不是
 ANSI C 标准，但它只在测试程序中，不太影响程序库的跨平台性。**/
#if defined(_MSC_VER)
#define EXPECT_EQ_SIZE_T(expect,actual) EXPECT_EQ_BASE((expect)==(actual),(size_t)expect,(size_t)actual,"%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect,actual) EXPECT_EQ_BASE((expect)==(actual),(size_t)expect,(size_t)actual,"%zu")
#endif


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
        lept_init(&v);           \
        EXPECT_EQ_INT(LEPT_PARSE_OK,lept_parse(&v,json)); \
        EXPECT_EQ_INT(LEPT_NUMBER,lept_get_type(&v));     \
        EXPECT_EQ_DOUBLE(expect,lept_get_number(&v));     \
        lept_free(&v);                             \
    }while(0)


#define TEST_STRING(expect,json) \
    do{                          \
        lept_value v;            \
        lept_init(&v);           \
        EXPECT_EQ_INT(LEPT_PARSE_OK,lept_parse(&v,json)); \
        EXPECT_EQ_INT(LEPT_STRING,lept_get_type(&v));     \
        EXPECT_EQ_STRING(expect,lept_get_string(&v),lept_get_string_length(&v)); \
        lept_free(&v);\
    }while(0)
static void test_parse_null(){
    lept_value v;
    lept_init(&v);
    lept_set_boolean(&v,0);  /**这个是LEPT_NULL类型，这里设置boolean是修改lept_init设置的LEPT_NULL类型，
                                        下一行lept_parse会修改回LEPT_NULL * **/
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,"null"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));   /**形参是指针，实参写地址**/
    lept_free(&v);
}
static void test_parse_true(){
    lept_value v;
    lept_init(&v);
    lept_set_boolean(&v,0);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,"true"));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
    lept_free(&v);
}

static void test_parse_false(){
    lept_value v;
    lept_init(&v);
    lept_set_boolean(&v,1);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,"false"));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
    lept_free(&v);
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
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE,"+1");
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

static void test_parse_string(){
    TEST_STRING("","\"\"");
    TEST_STRING("Hello","\"Hello\"");
    TEST_STRING("Hello\nWorld","\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t","\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("Hello\0World","\"Hello\\u0000World\"");
    TEST_STRING("\x24","\"\\u0024\"");
    TEST_STRING("\xC2\xA2","\"\\u00A2\"");
    TEST_STRING("\xE2\x82\xAC","\"\\u20AC\"");
    TEST_STRING("\xF0\x9D\x84\x9E","\"\\uD834\\uDD1E\"");
    TEST_STRING("\xF0\x9D\x84\x9E","\"\\ud834\\udd1e\"");
}

static void test_parse_number_too_big(){
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG,"1e309");
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG,"-1e309");
}


/**应该是缺失配对的\"**/
static void test_parse_missing_quotation_mark(){
    TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK,"\"");
    TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK,"\"abc");
}
/**对其他不合法的字符返回LEPT_PARSE_INVALID_STRING_ESCAPE**/
static void test_parse_invalid_string_escape(){
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE,"\"\\v\"");  /**这个应该是\v不合法**/
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE,"\"\\'\"");  /**这个应该是\'不合法**/
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE,"\"\\0\"");  /**这个应该是\0不合法**/
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE,"\"\\x12\""); /**这个应该是\x12不合法**/
}
/**处理不合法字符
 * unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
 * 当中空缺的 %x22 是双引号，%x5C 是反斜线，都已经处理。所以不合法的字符是 %x00 至 %x1F。**/
static void test_parse_invalid_string_char(){
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR,"\"\x01\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR,"\"\x1F\"");
}


static void test_parse_array() {
    size_t i, j;
    lept_value v;

    /* ... */

    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(5, lept_get_array_size(&v));
    EXPECT_EQ_INT(LEPT_NULL,   lept_get_type(lept_get_array_element(&v, 0)));
    EXPECT_EQ_INT(LEPT_FALSE,  lept_get_type(lept_get_array_element(&v, 1)));
    EXPECT_EQ_INT(LEPT_TRUE,   lept_get_type(lept_get_array_element(&v, 2)));
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_array_element(&v, 3)));
    EXPECT_EQ_INT(LEPT_STRING, lept_get_type(lept_get_array_element(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_array_element(&v, 3)));
    EXPECT_EQ_STRING("abc", lept_get_string(lept_get_array_element(&v, 4)), lept_get_string_length(lept_get_array_element(&v, 4)));
    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v));
    for (i = 0; i < 4; i++) {
        lept_value* a = lept_get_array_element(&v, i);
        EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(a));
        EXPECT_EQ_SIZE_T(i, lept_get_array_size(a));
        for (j = 0; j < i; j++) {
            lept_value* e = lept_get_array_element(a, j);
            EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
            EXPECT_EQ_DOUBLE((double)j, lept_get_number(e));
        }
    }
    lept_free(&v);
}


/***以下的单元测试和解析没有关系**/

/**单元测试boolean**/
static void test_access_boolean(){
    lept_value v;   /**声明**/
    lept_init(&v);   /**初始化**/
    lept_set_string(&v,"a",1);   /**设置为字符串，不懂为什么这么做**/
    lept_set_boolean(&v,1);          /**设置为boolean TRUE**/
    EXPECT_TRUE(lept_get_boolean(&v));  /**检验是否是TRUE**/
    lept_set_boolean(&v,0);          /**设置为boolean False**/
    EXPECT_FALSE(lept_get_boolean(&v)); /**检验是否为 False**/
    lept_free(&v);                      /**释放内存**/
}
/***单元测试null**/
static void test_access_null(){
    lept_value v;
    lept_init(&v);
    lept_set_string(&v,"a",1);
    lept_set_null(&v);
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
    lept_free(&v);
}

/**单元测试number**/
static void test_access_number(){
    lept_value v;
    lept_init(&v);
    lept_set_string(&v,"a",1);
    lept_set_number(&v,1234.5);
    EXPECT_EQ_DOUBLE(1234.5, lept_get_number(&v));
    lept_free(&v);
}

static void test_access_string(){
    lept_value v;
    lept_init(&v);
    lept_set_string(&v,"",0);
    EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v));
    lept_set_string(&v,"Hello",5);
    EXPECT_EQ_STRING("Hello", lept_get_string(&v), lept_get_string_length(&v));
    lept_free(&v);
}



static void test_parse(){
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_array();


    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}

int main(){

//#ifdef _WINDOWS
//    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//#endif

    test_parse();

    /**随便试试  创建一个string类型的json节点v，如果字符串内容为hhh,sizeof(lept_value)和sizeof(v)大小一样
    size_t size1 = sizeof(lept_value);
    lept_value v;
    lept_value * vv = &v;
//    char * str = "hhh";
    char * str = "This is a string with more than 24 charactershhhhhhhhhhhhhhhhhhhhhhhhhh";
    lept_set_string(vv,str,84);
    size_t size2 = sizeof(v);
    size_t size3 = sizeof(*v.u.s.s);
    printf("%zu %zu %zu",size1,size2,size3);    **/
    printf("%d/%d (%3.2f%%) passed\n",test_pass,test_count,test_pass * 100.0 / test_count);


    return main_ret;
}