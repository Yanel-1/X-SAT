#pragma once

#include <string>
#include <cstring>
#include <unordered_map>
#include <cassert>

#include "libs/cmdline.h"

class Options
{
public:
//        name             , type   , sname,must-need, default ,low, high, comments
#define PARAS \
    PARA( reduce_limit_inc , int    , '\0' , false,  1024   , 0   , 1e8    , "reduce_limit_inc") \
    PARA( reduce_per       , int    , '\0' , false,  50     , 0   , 100    , "reduce_per") \
    PARA( branchMode       , int    , 'b'  , false,  1      , 0   , 2      , "0 for vmtf, 1 for VSIDS, 2 for JFRONTIER") \
    PARA( max_lut_input    , int    , 'l'  , false,  7      , 2   , 10     , "LUT input limit") \
    PARA( enable_elim      , int    , 'e'  , false,  1      , 0   , 1     , "enable elimination") \
    PARA( enable_xvsids    , int    , 'x'  , false,  1      , 0   , 1     , "enable xor bump for VSIDS")

#define STR_PARAS \
    STR_PARA( instance   , 'i'   ,  true    , "" , ".aig/.aag format instance") \
    STR_PARA( pre_out   , 'p'   ,  false    , "" , "enable preprocess and write cnf to dist")

#define PARA(N, T, S, M, D, L, H, C) \
    T N = D;
    PARAS 
#undef PARA

#define STR_PARA(N, S, M, D, C) \
    std::string N = D;
    STR_PARAS
#undef STR_PARA

void parse_args(int argc, char *argv[]) {
    cmdline::parser parser;

    #define STR_PARA(N, S, M, D, C) \
    parser.add<std::string>(#N, S, C, M, D);
    STR_PARAS
    #undef STR_PARA

    #define PARA(N, T, S, M, D, L, H, C) \
    if (!strcmp(#T, "int")) parser.add<int>(#N, S, C, M, D, cmdline::range((int)L, (int)H)); \
    else parser.add<double>(#N, S, C, M, D, cmdline::range((double)L, (double)H));
    PARAS
    #undef PARA

    parser.parse_check(argc, argv);

    #define STR_PARA(N, S, M, D, C) \
    N = parser.get<std::string>(#N);
    STR_PARAS
    #undef STR_PARA

    #define PARA(N, T, S, M, D, L, H, C) \
    if (!strcmp(#T, "int")) N = parser.get<int>(#N); \
    else N = parser.get<double>(#N);
    PARAS
    #undef PARA
}
void print_change() {
    printf("c ------------------- Paras list -------------------\n");
    printf("c %-20s\t %-10s\t %-10s\t %-10s\t %s\n",
           "Name", "Type", "Now", "Default", "Comment");

#define PARA(N, T, S, M, D, L, H, C) \
    if (strcmp(#T, "int") == 0) printf("c %-20s\t %-10s\t %-10s\t %-10s\t %s\n", (#N), (#T), (#N), (#D), (C)); \
    else if(strcmp(#T, "double") == 0) printf("c %-20s\t %-10s\t %-10s\t %-10s\t %s\n", (#N), (#T), (#N), (#D), (C)); \
    else assert(false);
    PARAS
#undef PARA

#define STR_PARA(N, S, M, D, C) \
    printf("c %-20s\t string\t\t %-10s\t %-10s\t %s\n", (#N), N.c_str(), (#D), (C));
    STR_PARAS
#undef STR_PARA
    printf("c --------------------------------------------------\n");
}
};

inline Options __gloabal_options;

#define INIT_ARGS(argc, argv) __gloabal_options.parse_args(argc, argv);

#define OPT(N) (__gloabal_options.N)