cmake_minimum_required(VERSION 3.5.1)
project(Json)

set(CMAKE_CXX_STANDARD 14)

include_directories(json-tutorial-master/tutorial01)
include_directories(json/tutorial01)


add_executable(leptjson json-tutorial-master/tutorial01/leptjson.h
                        json-tutorial-master/tutorial01/leptjson.c
                        json-tutorial-master/tutorial01/test.c)
add_executable(main main.cpp)

add_executable(leptjson_myself_01 json/tutorial01/leptjson.h
                                  json/tutorial01/leptjson.c
                                  json/tutorial01/test.c
        )

add_executable(leptjson_myself_02 json/tutorial02/leptjson.h
                                  json/tutorial02/leptjson.c
                                  json/tutorial02/test.c)



add_executable(leptjson_myself_03 json/tutorial03/leptjson.h
                        json/tutorial03/leptjson.c
                        json/tutorial03/test.c)

add_executable(leptjson_myself_04 json/tutorial04/leptjson.h
                                  json/tutorial04/leptjson.c
                                  json/tutorial04/test.c)


add_executable(leptjson_myself_05 json/tutorial05/leptjson.h
                                  json/tutorial05/leptjson.c
                                  json/tutorial05/test.c)

add_executable(leptjson_myself_06 json/tutorial06/leptjson.h
                                  json/tutorial06/leptjson.c
                                  json/tutorial06/test.c)

add_executable(leptjson_myself_07 json/tutorial07/leptjson.h
                                  json/tutorial07/leptjson.c
                                  json/tutorial07/test.c)