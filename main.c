
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

/*
SPDX-License-Identifier: MIT

arcane
https://github.com/kgabis/arcane
Copyright (c) 2023 Krzysztof Gabis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#if 0
    #define TMPSTATIC static inline  __attribute__((always_inline))
    #define TMPSEMISTATIC static inline
#else
    #define TMPSTATIC
    #define TMPSEMISTATIC
#endif


#ifdef _MSC_VER
    #define __attribute__(x)
#endif


#define IS_NULLSTR(str) ((str) == NULL || (str)[0] == '\0')

/*
#define MC_CONF_OBJECT_PATTERN 0xfff8000000000000
#define MC_CONF_OBJECT_HEADERMASK 0xffff000000000000
#define MC_CONF_OBJECT_HDRALLOCATED 0xfffc000000000000
#define MC_CONF_OBJECT_HDRBOOL 0xfff9000000000000
#define MC_CONF_OBJECT_NULLPATTERN 0xfffa000000000000
*/

enum mcerrtype_t
{
    ARCANE_ERROR_NONE = 0,
    ARCANE_ERROR_PARSING,
    ARCANE_ERROR_COMPILATION,
    ARCANE_ERROR_RUNTIME,
    ARCANE_ERROR_TIMEOUT,
    ARCANE_ERROR_ALLOCATION,
    ARCANE_ERROR_USER,
};

enum mcobjtype_t
{
    ARCANE_OBJECT_NONE = 0,
    ARCANE_OBJECT_ERROR = 1 << 0,
    ARCANE_OBJECT_NUMBER = 1 << 1,
    ARCANE_OBJECT_BOOL = 1 << 2,
    ARCANE_OBJECT_STRING = 1 << 3,
    ARCANE_OBJECT_NULL = 1 << 4,
    ARCANE_OBJECT_NATIVE_FUNCTION = 1 << 5,
    ARCANE_OBJECT_ARRAY = 1 << 6,
    ARCANE_OBJECT_MAP = 1 << 7,
    ARCANE_OBJECT_FUNCTION = 1 << 8,
    ARCANE_OBJECT_EXTERNAL = 1 << 9,
    ARCANE_OBJECT_FREED = 1 << 10,
    /* for checking types with & */
    ARCANE_OBJECT_ANY = 0xffff,
};

enum mcasttoktype_t
{
    TOKEN_INVALID = 0,
    TOKEN_EOF,

    /* Operators */
    TOKEN_ASSIGN,

    TOKEN_PLUS_ASSIGN,
    TOKEN_MINUS_ASSIGN,
    TOKEN_ASTERISK_ASSIGN,
    TOKEN_SLASH_ASSIGN,
    TOKEN_PERCENT_ASSIGN,
    TOKEN_BIT_AND_ASSIGN,
    TOKEN_BIT_OR_ASSIGN,
    TOKEN_BIT_XOR_ASSIGN,
    TOKEN_LSHIFT_ASSIGN,
    TOKEN_RSHIFT_ASSIGN,

    TOKEN_QUESTION,

    TOKEN_PLUS,
    TOKEN_PLUS_PLUS,
    TOKEN_MINUS,
    TOKEN_MINUS_MINUS,
    TOKEN_BANG,
    TOKEN_ASTERISK,
    TOKEN_SLASH,

    TOKEN_LT,
    TOKEN_LTE,
    TOKEN_GT,
    TOKEN_GTE,

    TOKEN_EQ,
    TOKEN_NOT_EQ,

    TOKEN_AND,
    TOKEN_OR,

    TOKEN_BIT_AND,
    TOKEN_BIT_OR,
    TOKEN_BIT_XOR,
    TOKEN_LSHIFT,
    TOKEN_RSHIFT,

    /* Delimiters */
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_DOT,
    TOKEN_PERCENT,

    /* Keywords */
    TOKEN_FUNCTION,
    TOKEN_CONST,
    TOKEN_VAR,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_RETURN,
    TOKEN_WHILE,
    TOKEN_BREAK,
    TOKEN_FOR,
    TOKEN_IN,
    TOKEN_CONTINUE,
    TOKEN_NULL,
    TOKEN_IMPORT,
    TOKEN_RECOVER,

    /* Identifiers and literals */
    TOKEN_IDENT,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_TEMPLATE_STRING,

    TOKEN_TYPE_MAX
};

enum mcastmathoptype_t
{
    OPERATOR_NONE,
    OPERATOR_ASSIGN,
    OPERATOR_PLUS,
    OPERATOR_MINUS,
    OPERATOR_BANG,
    OPERATOR_ASTERISK,
    OPERATOR_SLASH,
    OPERATOR_LT,
    OPERATOR_LTE,
    OPERATOR_GT,
    OPERATOR_GTE,
    OPERATOR_EQ,
    OPERATOR_NOT_EQ,
    OPERATOR_MODULUS,
    OPERATOR_LOGICAL_AND,
    OPERATOR_LOGICAL_OR,
    OPERATOR_BIT_AND,
    OPERATOR_BIT_OR,
    OPERATOR_BIT_XOR,
    OPERATOR_LSHIFT,
    OPERATOR_RSHIFT,
};


enum mcastexprtype_t
{
    MC_EXPR_NONE,
    MC_EXPR_IDENT,
    MC_EXPR_NUMBERLITERAL,
    MC_EXPR_BOOLLITERAL,
    MC_EXPR_STRINGLITERAL,
    MC_EXPR_NULLLITERAL,
    MC_EXPR_ARRAYLITERAL,
    MC_EXPR_MAPLITERAL,
    MC_EXPR_PREFIX,
    MC_EXPR_INFIX,
    MC_EXPR_FUNCTIONLITERAL,
    MC_EXPR_CALL,
    MC_EXPR_INDEX,
    MC_EXPR_ASSIGN,
    MC_EXPR_LOGICAL,
    MC_EXPR_TERNARY,

    MC_EXPR_STMTDEFINE,
    MC_EXPR_STMTIF,
    MC_EXPR_STMTRETURN,
    MC_EXPR_STMTEXPRESSION,
    MC_EXPR_STMTLOOPWHILE,
    MC_EXPR_STMTBREAK,
    MC_EXPR_STMTCONTINUE,
    MC_EXPR_STMTLOOPFOREACH,
    MC_EXPR_STMTLOOPFORCLASSIC,
    MC_EXPR_STMTBLOCK,
    MC_EXPR_STMTIMPORT,
    MC_EXPR_STMTRECOVER,
};

enum mcopcode_t
{
    OPCODE_NONE = 0,
    OPCODE_CONSTANT,
    OPCODE_ADD,
    OPCODE_POP,
    OPCODE_SUB,
    OPCODE_MUL,
    OPCODE_DIV,
    OPCODE_MOD,
    OPCODE_TRUE,
    OPCODE_FALSE,
    OPCODE_COMPARE,
    OPCODE_COMPARE_EQ,
    OPCODE_EQUAL,
    OPCODE_NOT_EQUAL,
    OPCODE_GREATER_THAN,
    OPCODE_GREATER_THAN_EQUAL,
    OPCODE_MINUS,
    OPCODE_BANG,
    OPCODE_JUMP,
    OPCODE_JUMP_IF_FALSE,
    OPCODE_JUMP_IF_TRUE,
    OPCODE_NULL,
    OPCODE_GET_MODULE_GLOBAL,
    OPCODE_SET_MODULE_GLOBAL,
    OPCODE_DEFINE_MODULE_GLOBAL,
    OPCODE_ARRAY,
    OPCODE_MAP_START,
    OPCODE_MAP_END,
    OPCODE_GET_THIS,
    OPCODE_GET_INDEX,
    OPCODE_SET_INDEX,
    OPCODE_GET_VALUE_AT,
    OPCODE_CALL,
    OPCODE_RETURN_VALUE,
    OPCODE_RETURN,
    OPCODE_GET_LOCAL,
    OPCODE_DEFINE_LOCAL,
    OPCODE_SET_LOCAL,
    OPCODE_GET_ARCANE_GLOBAL,
    OPCODE_FUNCTION,
    OPCODE_GET_FREE,
    OPCODE_SET_FREE,
    OPCODE_CURRENT_FUNCTION,
    OPCODE_DUP,
    OPCODE_NUMBER,
    OPCODE_LEN,
    OPCODE_SET_RECOVER,
    OPCODE_OR,
    OPCODE_XOR,
    OPCODE_AND,
    OPCODE_LSHIFT,
    OPCODE_RSHIFT,
    OPCODE_MAX,
};


enum mcastsymtype_t
{
    SYMBOL_NONE = 0,
    SYMBOL_MODULE_GLOBAL,
    SYMBOL_LOCAL,
    SYMBOL_ARCANE_GLOBAL,
    SYMBOL_FREE,
    SYMBOL_FUNCTION,
    SYMBOL_THIS,
};

enum mcastprecedence_t
{
    PRECEDENCE_LOWEST = 0,
    PRECEDENCE_ASSIGN,
    /* a = b */
    PRECEDENCE_TERNARY,
    /* a ? b : c */
    PRECEDENCE_LOGICAL_OR,
    /* || */
    PRECEDENCE_LOGICAL_AND,
    /* && */
    PRECEDENCE_BIT_OR,
    /* | */
    PRECEDENCE_BIT_XOR,
    /* ^ */
    PRECEDENCE_BIT_AND,
    /* & */
    PRECEDENCE_EQUALS,
    /* == != */
    PRECEDENCE_LESSGREATER,
    /* >, >=, <, <= */
    PRECEDENCE_SHIFT,
    /* << >> */
    PRECEDENCE_SUM,
    /* + - */
    PRECEDENCE_PRODUCT,
    /* * / % */
    PRECEDENCE_PREFIX,
    /* -x !x ++x --x */
    PRECEDENCE_INCDEC,
    /* x++ x-- */
    PRECEDENCE_POSTFIX,
    /* myFunction(x) x["foo"] x.foo */
    PRECEDENCE_HIGHEST
};

typedef uint8_t opcode_t;


typedef enum mcerrtype_t mcerrtype_t;
typedef enum mcobjtype_t mcobjtype_t;
typedef enum mcasttoktype_t mcasttoktype_t;
typedef enum mcastmathoptype_t mcastmathoptype_t;
typedef enum mcastexprtype_t mcastexprtype_t;
typedef enum mcopcode_t mcopcode_t;
typedef enum mcastsymtype_t mcastsymtype_t;
typedef enum mcastprecedence_t mcastprecedence_t;

typedef struct mcgenericdict_t mcgenericdict_t;
typedef struct mcvaldict_t mcvaldict_t;
typedef struct mcbasicarray_t mcbasicarray_t;
typedef struct mcptrarray_t mcptrarray_t;
typedef struct mcprintstate_t mcprintstate_t;

typedef struct mcerror_t mcerror_t;
typedef struct mccompiledprogram_t mccompiledprogram_t;
typedef struct mctraceback_t mctraceback_t;
typedef struct mcastcompiledfile_t mcastcompiledfile_t;

typedef struct mcastexpression_t mcastexpression_t;
typedef struct compilation_result_t compilation_result_t;
typedef struct mctraceback_t mctraceback_t;
typedef struct mcstate_t mcstate_t;
typedef struct mcgcmemory_t mcgcmemory_t;
typedef struct mcglobalstore_t mcglobalstore_t;
typedef struct mcobjdata_t mcobjdata_t;
typedef struct mcerrlist_t mcerrlist_t;
typedef struct mcastparser_t mcastparser_t;
typedef struct mcconfig_t mcconfig_t;
typedef struct mcastsymtable_t mcastsymtable_t;
typedef struct mcastcompiler_t mcastcompiler_t;
typedef struct mcastsymbol_t mcastsymbol_t;
typedef struct mcvalue_t mcvalue_t;
typedef struct mcastlocation_t mcastlocation_t;
typedef struct mctimer_t mctimer_t;

typedef struct mcasttoken_t mcasttoken_t;
typedef struct code_block_t code_block_t;
typedef struct map_literal_t map_literal_t;

typedef struct prefix_t prefix_expression_t;
typedef struct infix_t infix_expression_t;
typedef struct if_case_t if_case_t;
typedef struct fn_literal_t fn_literal_t;
typedef struct call_expression_t call_expression_t;
typedef struct index_expression_t index_expression_t;
typedef struct assign_expression_t assign_expression_t;
typedef struct logical_expression_t logical_expression_t;
typedef struct ternary_expression_t ternary_expression_t;
typedef struct ident_t ident_t;

typedef struct define_statement_t define_statement_t;
typedef struct if_statement_t if_statement_t;
typedef struct while_loop_statement_t while_loop_statement_t;
typedef struct foreach_statement_t foreach_statement_t;
typedef struct for_loop_statement_t for_loop_statement_t;
typedef struct import_statement_t import_statement_t;
typedef struct recover_statement_t recover_statement_t;

typedef struct mcobjfuncscript_t mcobjfuncscript_t;
typedef struct mcobjfuncnative_t mcobjfuncnative_t;
typedef struct mcobjexternal_t mcobjexternal_t;
typedef struct mcobjerror_t mcobjerror_t;
typedef struct mcobjstring_t mcobjstring_t;


typedef struct mcopdefinition_t mcopdefinition_t;

typedef struct mcastscopeblock_t mcastscopeblock_t;
typedef struct mcastscopefile_t mcastscopefile_t;
typedef struct mcastscopecomp_t mcastscopecomp_t;

typedef struct mcgcobjdatapool_t mcgcobjdatapool_t;

typedef struct mcerror_t mcerror_t;
typedef struct mcastlexer_t mcastlexer_t;
typedef struct mcvmframe_t mcvmframe_t;
typedef struct mctraceitem_t mctraceitem_t;
typedef struct module_t module_t;
typedef struct mcnatfnbox_t mcnatfnbox_t;

typedef mcvalue_t (*arcanenativefn)(mcstate_t* state, void* data, int argc, mcvalue_t* args);
typedef void* (*arcanemallocfn)(void* ctx, size_t size);
typedef void (*arcanefreefn)(void* ctx, void* ptr);
typedef void (*arcanedatadestroyfn)(void* data);
typedef void* (*arcanedatacopyfn)(void* data);
typedef size_t (*arcanestdoutwritefn)(void* context, const void* data, size_t datasize);
typedef char* (*arcanereadfilefn)(void* context, const char* path);
typedef size_t (*arcanewritefilefn)(void* context, const char* path, const char* string, size_t stringsize);
typedef unsigned long (*collectionshashfn)(void* val);
typedef bool (*collectionsequalsfn)(void* a, void* b);
typedef void (*dictitemdestroyfn)(void* item);
typedef void* (*dictitemcopyfn)(void* item);
typedef void (*arrayitemdeinitfn)(void* item);
typedef void (*ptrarrayitemdestroyfn)(void* item);
typedef void* (*ptrarrayitemcopyfn)(void* item);
typedef mcvalue_t (*nativefn)(mcstate_t* state, void* data, int argc, mcvalue_t* args);
typedef void (*externaldatadestroyfn)(void* data);
typedef void* (*externaldatacopyfn)(void* data);
typedef mcastexpression_t* (*rightassocparsefn)(mcastparser_t* p);
typedef mcastexpression_t* (*leftassocparsefn)(mcastparser_t* p, mcastexpression_t* expr);



/**
 * \brief The execution environment for an instance of the script engine.
 */

struct mcvalue_t
{
    mcobjtype_t type;
    bool isallocated;
    union
    {
        mcobjdata_t* objdatahandle;
        double valnumber;
        bool valbool;
    };
};


#define ARCANE_STREQ(a, b) (strcmp((a), (b)) == 0)
#define ARCANE_STRNEQ(a, b, n) (strncmp((a), (b), (n)) == 0)
#define ARCANE_ARRAY_LEN(array) ((int)(sizeof(array) / sizeof(array[0])))
#define ARCANE_DBLEQ(a, b) (fabs((a) - (b)) < DBL_EPSILON)

#ifdef ARCANE_DEBUG
    #define ARCANE_ASSERT(x) assert((x))
    #define ARCANE_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#else
    #define ARCANE_ASSERT(x) ((void)0)
#endif


struct mcastlocation_t
{
    mcastcompiledfile_t* file;
    int line;
    int column;
};

struct mcconfig_t
{
    struct
    {
        struct
        {
            arcanestdoutwritefn fnprintto;
            void* context;
        } write;
    } stdio;
    struct
    {
        struct
        {
            arcanereadfilefn read_file;
            void* context;
        } read_file;
        struct
        {
            arcanewritefilefn write_file;
            void* context;
        } write_file;
    } fileio;
    /* allows redefinition of symbols */
    bool repl_mode;
    double max_execution_time_ms;
    bool max_execution_time_set;
};

struct mctimer_t
{
    int64_t start_offset;
    double start_time_ms;
};


struct mcasttoken_t
{
    mcasttoktype_t type;
    const char* literal;
    int len;
    mcastlocation_t pos;
};

struct code_block_t
{
    mcstate_t* pstate;
    mcptrarray_t* statements;
};

struct map_literal_t
{
    mcptrarray_t* keys;
    mcptrarray_t* values;
};

struct prefix_t
{
    mcastmathoptype_t op;
    mcastexpression_t* right;
};

struct infix_t
{
    mcastmathoptype_t op;
    mcastexpression_t* left;
    mcastexpression_t* right;
};

struct if_case_t
{
    mcstate_t* pstate;
    mcastexpression_t* test;
    code_block_t* consequence;
};

struct fn_literal_t
{
    char* name;
    mcptrarray_t* params;
    code_block_t* body;
};

struct call_expression_t
{
    mcastexpression_t* function;
    mcptrarray_t* args;
};

struct index_expression_t
{
    mcastexpression_t* left;
    mcastexpression_t* index;
};

struct assign_expression_t
{
    mcastexpression_t* dest;
    mcastexpression_t* source;
    bool is_postfix;
};

struct logical_expression_t
{
    mcastmathoptype_t op;
    mcastexpression_t* left;
    mcastexpression_t* right;
};

struct ternary_expression_t
{
    mcastexpression_t* test;
    mcastexpression_t* if_true;
    mcastexpression_t* if_false;
};


struct ident_t
{
    mcstate_t* pstate;
    char* value;
    mcastlocation_t pos;
};

struct define_statement_t
{
    ident_t* name;
    mcastexpression_t* value;
    bool assignable;
};

struct if_statement_t
{
    mcptrarray_t* cases;
    code_block_t* alternative;
};

struct while_loop_statement_t
{
    mcastexpression_t* test;
    code_block_t* body;
};

struct foreach_statement_t
{
    ident_t* iterator;
    mcastexpression_t* source;
    code_block_t* body;
};

struct for_loop_statement_t
{
    mcastexpression_t* init;
    mcastexpression_t* test;
    mcastexpression_t* update;
    code_block_t* body;
};

struct import_statement_t
{
    char* path;
};

struct recover_statement_t
{
    ident_t* error_ident;
    code_block_t* body;
};



struct mcastexpression_t
{
    mcstate_t* pstate;
    mcastexprtype_t type;

    union
    {
        ident_t* ident;
        double number_literal;
        bool bool_literal;
        char* string_literal;
        mcptrarray_t* array;
        map_literal_t map;
        prefix_expression_t prefix;
        infix_expression_t infix;
        fn_literal_t fn_literal;
        call_expression_t call_expr;
        index_expression_t index_expr;
        assign_expression_t assign;
        logical_expression_t logical;
        ternary_expression_t ternary;



        define_statement_t define;
        if_statement_t if_statement;
        mcastexpression_t* return_value;
        mcastexpression_t* expression;
        while_loop_statement_t while_loop;
        foreach_statement_t foreach;
        for_loop_statement_t for_loop;
        code_block_t* block;
        import_statement_t import;
        recover_statement_t recover;
    };

    mcastlocation_t pos;
};

#define MC_CONF_OBJECT_STRING_BUF_SIZE 24


struct mcobjfuncscript_t
{
    union
    {
        mcvalue_t* free_vals_allocated;
        mcvalue_t free_vals_buf[2];
    };

    union
    {
        char* name;
        const char* const_name;
    };

    compilation_result_t* comp_result;
    int num_locals;
    int num_args;
    int free_vals_count;
    bool owns_data;
};

#define NATIVE_FN_MAX_DATA_LEN 24

struct mcobjfuncnative_t
{
    char* name;
    nativefn fn;
    uint8_t data[NATIVE_FN_MAX_DATA_LEN];
    int data_len;
};

struct mcobjexternal_t
{
    void* data;
    externaldatadestroyfn data_destroy_fn;
    externaldatacopyfn data_copy_fn;
};

struct mcobjerror_t
{
    char* message;
    mctraceback_t* traceback;
};

struct mcobjstring_t
{
    union
    {
        char* actualallocated;
        char actualonstack[MC_CONF_OBJECT_STRING_BUF_SIZE];
    };
    unsigned long hash;
    bool is_allocated;
    int capacity;
    int length;
    const char* data;
};

struct mcobjdata_t
{
    mcstate_t* pstate;
    mcgcmemory_t* mem;
    mcobjtype_t type;
    union
    {
        mcobjstring_t string;
        mcobjerror_t error;
        mcbasicarray_t* array;
        mcvaldict_t* map;
        mcobjfuncscript_t function;
        mcobjfuncnative_t native_function;
        mcobjexternal_t external;
    };

    bool gcmark;
};

struct mcopdefinition_t
{
    const char* name;
    int num_operands;
    int operand_widths[2];
};


struct mcastsymbol_t
{
    mcstate_t* pstate;
    mcastsymtype_t type;
    char* name;
    int index;
    bool assignable;
};

struct mcastscopeblock_t
{
    mcstate_t* pstate;
    mcgenericdict_t* store;
    int offset;
    int num_definitions;
};

struct mcastsymtable_t
{
    mcstate_t* pstate;
    mcastsymtable_t* outer;
    mcglobalstore_t* global_store;
    mcptrarray_t* block_scopes;
    mcptrarray_t* free_symbols;
    mcptrarray_t* module_global_symbols;
    int max_num_definitions;
    int module_global_offset;
};


#define GCMEM_POOL_SIZE 2048
#define GCMEM_POOLS_NUM 3
#define GCMEM_SWEEP_INTERVAL 128

struct mcgcobjdatapool_t
{
    mcobjdata_t* data[GCMEM_POOL_SIZE];
    int count;
};

struct mcgcmemory_t
{
    mcstate_t* pstate;
    int allocssincesweep;
    mcptrarray_t* gcobjlist;
    mcptrarray_t* gcobjlistback;
    mcbasicarray_t* gcobjlistremains;
    mcgcobjdatapool_t onlydatapool;
    mcgcobjdatapool_t mempools[GCMEM_POOLS_NUM];
};

struct compilation_result_t
{
    mcstate_t* pstate;
    uint8_t* bytecode;
    mcastlocation_t* src_positions;
    int count;
};

struct mcastscopecomp_t
{
    mcstate_t* pstate;
    mcastscopecomp_t* outer;
    mcbasicarray_t* bytecode;
    mcbasicarray_t* src_positions;
    mcbasicarray_t* break_ip_stack;
    mcbasicarray_t* continue_ip_stack;
    opcode_t last_opcode;
};


struct mcastcompiledfile_t
{
    mcstate_t* pstate;
    char* dir_path;
    char* path;
    mcptrarray_t* lines;
};

#define MC_CONF_ERROR_MAXERRORCOUNT 16
#define MC_CONF_ERROR_MSGMAXLENGTH 255

struct mcerror_t
{
    mcerrtype_t type;
    char message[MC_CONF_ERROR_MSGMAXLENGTH];
    mcastlocation_t pos;
    mctraceback_t* traceback;
};

struct mcerrlist_t
{
    mcerror_t errors[MC_CONF_ERROR_MAXERRORCOUNT];
    int count;
};

struct mcastlexer_t
{
    mcstate_t* pstate;
    mcerrlist_t* errors;
    const char* input;
    int input_len;
    int position;
    int next_position;
    char ch;
    int line;
    int column;
    mcastcompiledfile_t* file;
    bool failed;
    bool continue_template_string;

    struct
    {
        int position;
        int next_position;
        char ch;
        int line;
        int column;
    } prev_token_state;

    mcasttoken_t prev_token;
    mcasttoken_t cur_token;
    mcasttoken_t peek_token;
};

struct mcastparser_t
{
    mcstate_t* pstate;
    mcconfig_t* config;
    mcastlexer_t lexer;
    mcerrlist_t* errors;

    rightassocparsefn right_assoc_parse_fns[TOKEN_TYPE_MAX];
    leftassocparsefn left_assoc_parse_fns[TOKEN_TYPE_MAX];

    int depth;
};

struct mcvmframe_t
{
    mcvalue_t function;
    int ip;
    int base_pointer;
    mcastlocation_t* src_positions;
    uint8_t* bytecode;
    int src_ip;
    int bytecode_size;
    int recover_ip;
    bool is_recovering;
};

struct mctraceitem_t
{
    char* function_name;
    mcastlocation_t pos;
};

struct mctraceback_t
{
    mcstate_t* pstate;
    mcbasicarray_t* items;
};


#define VM_STACK_SIZE 2048
#define VM_MAX_GLOBALS 2048
#define VM_MAX_FRAMES 2048
#define VM_THIS_STACK_SIZE 2048

struct mcstate_t
{
    mcconfig_t config;
    mcerrlist_t errors;
    mcgcmemory_t* mem;
    mcglobalstore_t* global_store;
    mcvalue_t globals[VM_MAX_GLOBALS];
    int globals_count;
    mcvalue_t stack[VM_STACK_SIZE];
    int sp;
    mcvalue_t this_stack[VM_THIS_STACK_SIZE];
    int this_sp;
    mcvmframe_t frames[VM_MAX_FRAMES];
    int frames_count;
    mcvalue_t last_popped;
    mcvmframe_t* current_frame;
    bool running;
    mcvalue_t operator_oveload_keys[OPCODE_MAX];
    mcptrarray_t* files;
    mcastcompiler_t* compiler;
};

struct mcglobalstore_t
{
    mcstate_t* pstate;
    mcgenericdict_t* symbols;
    mcbasicarray_t* objects;
};

struct mcprintstate_t
{
    mcstate_t* pstate;
    bool failed;
    bool onstack;
    FILE* destfile;
    char* data;
    size_t capacity;
    size_t len;
};

struct mcbasicarray_t
{
    mcstate_t* pstate;
    unsigned char* data;
    unsigned char* data_allocated;
    unsigned int count;
    unsigned int capacity;
    size_t element_size;
    bool lock_capacity;
};

struct mcptrarray_t
{
    mcstate_t* pstate;
    mcbasicarray_t arr;
};

struct mcvaldict_t
{
    mcstate_t* pstate;
    size_t key_size;
    size_t val_size;
    unsigned int* cells;
    unsigned long* hashes;
    void* keys;
    void* values;
    unsigned int* cell_ixs;
    unsigned int count;
    unsigned int item_capacity;
    unsigned int cell_capacity;
    collectionshashfn _hash_key;
    collectionsequalsfn _keys_equals;
};

struct mcgenericdict_t
{
    mcstate_t* pstate;
    unsigned int* cells;
    unsigned long* hashes;
    char** keys;
    void** values;
    unsigned int* cell_ixs;
    unsigned int count;
    unsigned int item_capacity;
    unsigned int cell_capacity;
    dictitemcopyfn copy_fn;
    dictitemdestroyfn destroy_fn;
};


struct module_t
{
    mcstate_t* pstate;
    char* name;
    mcptrarray_t* symbols;
};

struct mcastscopefile_t
{
    mcstate_t* pstate;
    mcastparser_t* parser;
    mcastsymtable_t* symbol_table;
    mcastcompiledfile_t* file;
    mcptrarray_t* loaded_module_names;
};

struct mcastcompiler_t
{
    mcstate_t* pstate;
    mcconfig_t* config;
    mcgcmemory_t* mem;
    mcerrlist_t* errors;
    mcptrarray_t* files;
    mcglobalstore_t* global_store;
    mcbasicarray_t* constants;
    mcastscopecomp_t* compilation_scope;
    mcptrarray_t* file_scopes;
    mcbasicarray_t* src_positions_stack;
    mcgenericdict_t* modules;
    mcgenericdict_t* string_constants_positions;
};

struct mcnatfnbox_t
{
    arcanenativefn fn;
    mcstate_t* pstate;
    void* data;
};

struct mccompiledprogram_t
{
    mcstate_t* pstate;
    compilation_result_t* comp_res;
};

#include "prot.inc"

/* endheader */

static mcastlocation_t srcposinvalid = { NULL, -1, -1 };

TMPSTATIC mcastlocation_t src_pos_make(mcastcompiledfile_t* file, int line, int column)
{
    return (mcastlocation_t){
        .file = file,
        .line = line,
        .column = column,
    };
}

TMPSEMISTATIC char* arcane_stringf(mcstate_t* state, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int towrite = vsnprintf(NULL, 0, format, args);
    va_end(args);
    va_start(args, format);
    char* res = (char*)allocator_malloc(state, towrite + 1);
    if(!res)
    {
        return NULL;
    }
    int written = vsprintf(res, format, args);
    va_end(args);
    (void)written;
    ARCANE_ASSERT(written == towrite);
    return res;
}


TMPSTATIC char* arcane_strndup(mcstate_t* state, const char* string, size_t n)
{
    char* outputstring = (char*)allocator_malloc(state, n + 1);
    if(!outputstring)
    {
        return NULL;
    }
    outputstring[n] = '\0';
    memcpy(outputstring, string, n);
    return outputstring;
}

TMPSTATIC char* arcane_strdup(mcstate_t* state, const char* string)
{
    if(!string)
    {
        return NULL;
    }
    return arcane_strndup(state, string, strlen(string));
}

TMPSTATIC uint64_t arcane_double_to_uint64(double val)
{
    union
    {
        uint64_t val_uint64;
        double val_double;
    } temp = { .val_double = val };

    return temp.val_uint64;
}

TMPSTATIC double arcane_uint64_to_double(uint64_t val)
{
    union
    {
        uint64_t val_uint64;
        double val_double;
    } temp = { .val_uint64 = val };

    return temp.val_double;
}

TMPSTATIC bool arcane_timer_platform_supported()
{
    return true;
}

TMPSTATIC mctimer_t arcane_timer_start()
{
    mctimer_t timer;
    memset(&timer, 0, sizeof(mctimer_t));
    /* At some point it should be replaced with more accurate per-platform timers */
    struct timeval starttime;
    gettimeofday(&starttime, NULL);
    timer.start_offset = starttime.tv_sec;
    timer.start_time_ms = starttime.tv_usec / 1000.0;

    return timer;
}

TMPSTATIC double arcane_timer_get_elapsed_ms(mctimer_t* timer)
{
    struct timeval currenttime;
    gettimeofday(&currenttime, NULL);
    int times = (int)((int64_t)currenttime.tv_sec - timer->start_offset);
    double currenttimems = (times * 1000) + (currenttime.tv_usec / 1000.0);
    return currenttimems - timer->start_time_ms;
}

TMPSTATIC mcastscopecomp_t* compilation_scope_make(mcstate_t* state, mcastscopecomp_t* outer)
{
    mcastscopecomp_t* scope = (mcastscopecomp_t*)allocator_malloc(state, sizeof(mcastscopecomp_t));
    if(!scope)
    {
        return NULL;
    }
    memset(scope, 0, sizeof(mcastscopecomp_t));
    scope->pstate = state;
    scope->outer = outer;
    scope->bytecode = array_make(state, sizeof(uint8_t));
    if(!scope->bytecode)
    {
        goto err;
    }
    scope->src_positions = array_make(state, sizeof(mcastlocation_t));
    if(!scope->src_positions)
    {
        goto err;
    }
    scope->break_ip_stack = array_make(state, sizeof(int));
    if(!scope->break_ip_stack)
    {
        goto err;
    }
    scope->continue_ip_stack = array_make(state, sizeof(int));
    if(!scope->continue_ip_stack)
    {
        goto err;
    }
    return scope;
err:
    compilation_scope_destroy(scope);
    return NULL;
}

TMPSTATIC void compilation_scope_destroy(mcastscopecomp_t* scope)
{
    mcstate_t* state;
    state = scope->pstate;
    array_destroy(scope->continue_ip_stack);
    array_destroy(scope->break_ip_stack);
    array_destroy(scope->bytecode);
    array_destroy(scope->src_positions);
    allocator_free(state, scope);
}

TMPSTATIC compilation_result_t* compilation_scope_orphan_result(mcastscopecomp_t* scope)
{
    compilation_result_t* res = compilation_result_make(scope->pstate, array_data(scope->bytecode), array_data(scope->src_positions), array_count(scope->bytecode));
    if(!res)
    {
        return NULL;
    }
    array_orphan_data(scope->bytecode);
    array_orphan_data(scope->src_positions);
    return res;
}

TMPSTATIC compilation_result_t* compilation_result_make(mcstate_t* state, uint8_t* bytecode, mcastlocation_t* src_positions, int count)
{
    compilation_result_t* res = (compilation_result_t*)allocator_malloc(state, sizeof(compilation_result_t));
    if(!res)
    {
        return NULL;
    }
    memset(res, 0, sizeof(compilation_result_t));
    res->pstate = state;
    res->bytecode = bytecode;
    res->src_positions = src_positions;
    res->count = count;
    return res;
}

TMPSTATIC void compilation_result_destroy(compilation_result_t* res)
{
    mcstate_t* state;
    if(!res)
    {
        return;
    }
    state = res->pstate;
    allocator_free(state, res->bytecode);
    allocator_free(state, res->src_positions);
    allocator_free(state, res);
}

TMPSTATIC bool mc_lexer_init(mcastlexer_t* lex, mcstate_t* state, mcerrlist_t* errs, const char* input, mcastcompiledfile_t* file)
{
    bool ok;
    lex->pstate = state;
    lex->errors = errs;
    lex->input = input;
    lex->input_len = (int)strlen(input);
    lex->position = 0;
    lex->next_position = 0;
    lex->ch = '\0';
    if(file)
    {
        lex->line = ptrarray_count(file->lines);
    }
    else
    {
        lex->line = 0;
    }
    lex->column = -1;
    lex->file = file;
    ok = mc_lexer_addline(lex, 0);
    if(!ok)
    {
        return false;
    }
    ok = mc_lexer_readchar(lex);
    if(!ok)
    {
        return false;
    }
    lex->failed = false;
    lex->continue_template_string = false;

    memset(&lex->prev_token_state, 0, sizeof(lex->prev_token_state));
    token_init(&lex->prev_token, TOKEN_INVALID, NULL, 0);
    token_init(&lex->cur_token, TOKEN_INVALID, NULL, 0);
    token_init(&lex->peek_token, TOKEN_INVALID, NULL, 0);

    return true;
}

TMPSTATIC bool mc_lexer_failed(mcastlexer_t* lex)
{
    return lex->failed;
}

TMPSTATIC void mc_lexer_conttplstring(mcastlexer_t* lex)
{
    lex->continue_template_string = true;
}

TMPSTATIC bool mc_lexer_currtokenis(mcastlexer_t* lex, mcasttoktype_t type)
{
    return lex->cur_token.type == type;
}

TMPSTATIC bool mc_lexer_peektokenis(mcastlexer_t* lex, mcasttoktype_t type)
{
    return lex->peek_token.type == type;
}

TMPSTATIC bool mc_lexer_nexttoken(mcastlexer_t* lex)
{
    lex->prev_token = lex->cur_token;
    lex->cur_token = lex->peek_token;
    lex->peek_token = mc_lexer_nexttokinternal(lex);
    return !lex->failed;
}

TMPSTATIC bool mc_lexer_previoustoken(mcastlexer_t* lex)
{
    if(lex->prev_token.type == TOKEN_INVALID)
    {
        return false;
    }

    lex->peek_token = lex->cur_token;
    lex->cur_token = lex->prev_token;
    token_init(&lex->prev_token, TOKEN_INVALID, NULL, 0);

    lex->ch = lex->prev_token_state.ch;
    lex->column = lex->prev_token_state.column;
    lex->line = lex->prev_token_state.line;
    lex->position = lex->prev_token_state.position;
    lex->next_position = lex->prev_token_state.next_position;

    return true;
}

TMPSTATIC mcasttoken_t mc_lexer_nexttokinternal(mcastlexer_t* lex)
{
    lex->prev_token_state.ch = lex->ch;
    lex->prev_token_state.column = lex->column;
    lex->prev_token_state.line = lex->line;
    lex->prev_token_state.position = lex->position;
    lex->prev_token_state.next_position = lex->next_position;

    while(true)
    {
        if(!lex->continue_template_string)
        {
            mc_lexer_skipspace(lex);
        }

        mcasttoken_t outtok;
        outtok.type = TOKEN_INVALID;
        outtok.literal = lex->input + lex->position;
        outtok.len = 1;
        outtok.pos = src_pos_make(lex->file, lex->line, lex->column);

        char c = lex->continue_template_string ? '`' : lex->ch;

        switch(c)
        {
            case '\0':
                token_init(&outtok, TOKEN_EOF, "EOF", 3);
                break;
            case '=':
            {
                if(mc_lexer_peekchar(lex) == '=')
                {
                    token_init(&outtok, TOKEN_EQ, "==", 2);
                    mc_lexer_readchar(lex);
                }
                else
                {
                    token_init(&outtok, TOKEN_ASSIGN, "=", 1);
                }
                break;
            }
            case '&':
            {
                if(mc_lexer_peekchar(lex) == '&')
                {
                    token_init(&outtok, TOKEN_AND, "&&", 2);
                    mc_lexer_readchar(lex);
                }
                else if(mc_lexer_peekchar(lex) == '=')
                {
                    token_init(&outtok, TOKEN_BIT_AND_ASSIGN, "&=", 2);
                    mc_lexer_readchar(lex);
                }
                else
                {
                    token_init(&outtok, TOKEN_BIT_AND, "&", 1);
                }
                break;
            }
            case '|':
            {
                if(mc_lexer_peekchar(lex) == '|')
                {
                    token_init(&outtok, TOKEN_OR, "||", 2);
                    mc_lexer_readchar(lex);
                }
                else if(mc_lexer_peekchar(lex) == '=')
                {
                    token_init(&outtok, TOKEN_BIT_OR_ASSIGN, "|=", 2);
                    mc_lexer_readchar(lex);
                }
                else
                {
                    token_init(&outtok, TOKEN_BIT_OR, "|", 1);
                }
                break;
            }
            case '^':
            {
                if(mc_lexer_peekchar(lex) == '=')
                {
                    token_init(&outtok, TOKEN_BIT_XOR_ASSIGN, "^=", 2);
                    mc_lexer_readchar(lex);
                }
                else
                {
                    token_init(&outtok, TOKEN_BIT_XOR, "^", 1);
                    break;
                }
                break;
            }
            case '+':
            {
                if(mc_lexer_peekchar(lex) == '=')
                {
                    token_init(&outtok, TOKEN_PLUS_ASSIGN, "+=", 2);
                    mc_lexer_readchar(lex);
                }
                else if(mc_lexer_peekchar(lex) == '+')
                {
                    token_init(&outtok, TOKEN_PLUS_PLUS, "++", 2);
                    mc_lexer_readchar(lex);
                }
                else
                {
                    token_init(&outtok, TOKEN_PLUS, "+", 1);
                    break;
                }
                break;
            }
            case '-':
            {
                if(mc_lexer_peekchar(lex) == '=')
                {
                    token_init(&outtok, TOKEN_MINUS_ASSIGN, "-=", 2);
                    mc_lexer_readchar(lex);
                }
                else if(mc_lexer_peekchar(lex) == '-')
                {
                    token_init(&outtok, TOKEN_MINUS_MINUS, "--", 2);
                    mc_lexer_readchar(lex);
                }
                else
                {
                    token_init(&outtok, TOKEN_MINUS, "-", 1);
                    break;
                }
                break;
            }
            case '!':
            {
                if(mc_lexer_peekchar(lex) == '=')
                {
                    token_init(&outtok, TOKEN_NOT_EQ, "!=", 2);
                    mc_lexer_readchar(lex);
                }
                else
                {
                    token_init(&outtok, TOKEN_BANG, "!", 1);
                }
                break;
            }
            case '*':
            {
                if(mc_lexer_peekchar(lex) == '=')
                {
                    token_init(&outtok, TOKEN_ASTERISK_ASSIGN, "*=", 2);
                    mc_lexer_readchar(lex);
                }
                else
                {
                    token_init(&outtok, TOKEN_ASTERISK, "*", 1);
                    break;
                }
                break;
            }
            case '/':
            {
                if(mc_lexer_peekchar(lex) == '/')
                {
                    mc_lexer_readchar(lex);
                    while(lex->ch != '\n' && lex->ch != '\0')
                    {
                        mc_lexer_readchar(lex);
                    }
                    continue;
                }
                if(mc_lexer_peekchar(lex) == '=')
                {
                    token_init(&outtok, TOKEN_SLASH_ASSIGN, "/=", 2);
                    mc_lexer_readchar(lex);
                }
                else
                {
                    token_init(&outtok, TOKEN_SLASH, "/", 1);
                    break;
                }
                break;
            }
            case '<':
            {
                if(mc_lexer_peekchar(lex) == '=')
                {
                    token_init(&outtok, TOKEN_LTE, "<=", 2);
                    mc_lexer_readchar(lex);
                }
                else if(mc_lexer_peekchar(lex) == '<')
                {
                    mc_lexer_readchar(lex);
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        token_init(&outtok, TOKEN_LSHIFT_ASSIGN, "<<=", 3);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        token_init(&outtok, TOKEN_LSHIFT, "<<", 2);
                    }
                }
                else
                {
                    token_init(&outtok, TOKEN_LT, "<", 1);
                    break;
                }
                break;
            }
            case '>':
            {
                if(mc_lexer_peekchar(lex) == '=')
                {
                    token_init(&outtok, TOKEN_GTE, ">=", 2);
                    mc_lexer_readchar(lex);
                }
                else if(mc_lexer_peekchar(lex) == '>')
                {
                    mc_lexer_readchar(lex);
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        token_init(&outtok, TOKEN_RSHIFT_ASSIGN, ">>=", 3);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        token_init(&outtok, TOKEN_RSHIFT, ">>", 2);
                    }
                }
                else
                {
                    token_init(&outtok, TOKEN_GT, ">", 1);
                }
                break;
            }
            case ',':
                token_init(&outtok, TOKEN_COMMA, ",", 1);
                break;
            case ';':
                token_init(&outtok, TOKEN_SEMICOLON, ";", 1);
                break;
            case ':':
                token_init(&outtok, TOKEN_COLON, ":", 1);
                break;
            case '(':
                token_init(&outtok, TOKEN_LPAREN, "(", 1);
                break;
            case ')':
                token_init(&outtok, TOKEN_RPAREN, ")", 1);
                break;
            case '{':
                token_init(&outtok, TOKEN_LBRACE, "{", 1);
                break;
            case '}':
                token_init(&outtok, TOKEN_RBRACE, "}", 1);
                break;
            case '[':
                token_init(&outtok, TOKEN_LBRACKET, "[", 1);
                break;
            case ']':
                token_init(&outtok, TOKEN_RBRACKET, "]", 1);
                break;
            case '.':
                token_init(&outtok, TOKEN_DOT, ".", 1);
                break;
            case '?':
                token_init(&outtok, TOKEN_QUESTION, "?", 1);
                break;
            case '%':
            {
                if(mc_lexer_peekchar(lex) == '=')
                {
                    token_init(&outtok, TOKEN_PERCENT_ASSIGN, "%=", 2);
                    mc_lexer_readchar(lex);
                }
                else
                {
                    token_init(&outtok, TOKEN_PERCENT, "%", 1);
                    break;
                }
                break;
            }
            case '"':
            {
                mc_lexer_readchar(lex);
                int len;
                const char* str = mc_lexer_scanstring(lex, '"', false, NULL, &len);
                if(str)
                {
                    token_init(&outtok, TOKEN_STRING, str, len);
                }
                else
                {
                    token_init(&outtok, TOKEN_INVALID, NULL, 0);
                }
                break;
            }
            case '\'':
            {
                mc_lexer_readchar(lex);
                int len;
                const char* str = mc_lexer_scanstring(lex, '\'', false, NULL, &len);
                if(str)
                {
                    token_init(&outtok, TOKEN_STRING, str, len);
                }
                else
                {
                    token_init(&outtok, TOKEN_INVALID, NULL, 0);
                }
                break;
            }
            case '`':
            {
                if(!lex->continue_template_string)
                {
                    mc_lexer_readchar(lex);
                }
                int len;
                bool templatefound = false;
                const char* str = mc_lexer_scanstring(lex, '`', true, &templatefound, &len);
                if(str)
                {
                    if(templatefound)
                    {
                        token_init(&outtok, TOKEN_TEMPLATE_STRING, str, len);
                    }
                    else
                    {
                        token_init(&outtok, TOKEN_STRING, str, len);
                    }
                }
                else
                {
                    token_init(&outtok, TOKEN_INVALID, NULL, 0);
                }
                break;
            }
            default:
            {
                if(mc_lexer_charisletter(lex->ch))
                {
                    int identlen = 0;
                    const char* ident = mc_lexer_scanident(lex, &identlen);
                    mcasttoktype_t type = mc_lexer_lookupident(ident, identlen);
                    token_init(&outtok, type, ident, identlen);
                    return outtok;
                }
                if(mc_lexer_charisdigit(lex->ch))
                {
                    int numberlen = 0;
                    const char* number = mc_lexer_scannumber(lex, &numberlen);
                    token_init(&outtok, TOKEN_NUMBER, number, numberlen);
                    return outtok;
                }
                break;
            }
        }
        mc_lexer_readchar(lex);
        if(mc_lexer_failed(lex))
        {
            token_init(&outtok, TOKEN_INVALID, NULL, 0);
        }
        lex->continue_template_string = false;
        return outtok;
    }
}

TMPSTATIC bool mc_lexer_expectcurrent(mcastlexer_t* lex, mcasttoktype_t type)
{
    if(mc_lexer_failed(lex))
    {
        return false;
    }

    if(!mc_lexer_currtokenis(lex, type))
    {
        const char* expectedtypestr = token_type_to_string(type);
        const char* actualtypestr = token_type_to_string(lex->cur_token.type);
        errors_add_errorf(lex->errors, ARCANE_ERROR_PARSING, lex->cur_token.pos, "Expected current token to be \"%s\", got \"%s\" instead", expectedtypestr, actualtypestr);
        return false;
    }
    return true;
}

TMPSTATIC bool mc_lexer_readchar(mcastlexer_t* lex)
{
    bool ok; 
    if(lex->next_position >= lex->input_len)
    {
        lex->ch = '\0';
    }
    else
    {
        lex->ch = lex->input[lex->next_position];
    }
    lex->position = lex->next_position;
    lex->next_position++;

    if(lex->ch == '\n')
    {
        lex->line++;
        lex->column = -1;
        ok = mc_lexer_addline(lex, lex->next_position);
        if(!ok)
        {
            lex->failed = true;
            return false;
        }
    }
    else
    {
        lex->column++;
    }
    return true;
}

TMPSTATIC char mc_lexer_peekchar(mcastlexer_t* lex)
{
    if(lex->next_position >= lex->input_len)
    {
        return '\0';
    }
    return lex->input[lex->next_position];
}

TMPSTATIC bool mc_lexer_charisletter(char ch)
{
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

TMPSTATIC bool mc_lexer_charisdigit(char ch)
{
    return ch >= '0' && ch <= '9';
}

TMPSTATIC bool mc_lexer_charisoneof(char ch, const char* allowed, int allowedlen)
{
    for(int i = 0; i < allowedlen; i++)
    {
        if(ch == allowed[i])
        {
            return true;
        }
    }
    return false;
}

TMPSTATIC const char* mc_lexer_scanident(mcastlexer_t* lex, int* outlen)
{
    int position = lex->position;
    int len = 0;
    while(mc_lexer_charisdigit(lex->ch) || mc_lexer_charisletter(lex->ch) || lex->ch == ':')
    {
        if(lex->ch == ':')
        {
            if(mc_lexer_peekchar(lex) != ':')
            {
                goto end;
            }
            mc_lexer_readchar(lex);
        }
        mc_lexer_readchar(lex);
    }
end:
    len = lex->position - position;
    *outlen = len;
    return lex->input + position;
}

TMPSTATIC const char* mc_lexer_scannumber(mcastlexer_t* lex, int* outlen)
{
    char allowed[] = ".xXaAbBcCdDeEfF";
    int position = lex->position;
    while(mc_lexer_charisdigit(lex->ch) || mc_lexer_charisoneof(lex->ch, allowed, ARCANE_ARRAY_LEN(allowed) - 1))
    {
        mc_lexer_readchar(lex);
    }
    int len = lex->position - position;
    *outlen = len;
    return lex->input + position;
}

TMPSTATIC const char* mc_lexer_scanstring(mcastlexer_t* lex, char delimiter, bool istemplate, bool* outtemplatefound, int* outlen)
{
    *outlen = 0;

    bool escarcaned = false;
    int position = lex->position;

    while(true)
    {
        if(lex->ch == '\0')
        {
            return NULL;
        }
        if(lex->ch == delimiter && !escarcaned)
        {
            break;
        }
        if(istemplate && !escarcaned && lex->ch == '$' && mc_lexer_peekchar(lex) == '{')
        {
            *outtemplatefound = true;
            break;
        }
        escarcaned = false;
        if(lex->ch == '\\')
        {
            escarcaned = true;
        }
        mc_lexer_readchar(lex);
    }
    int len = lex->position - position;
    *outlen = len;
    return lex->input + position;
}

TMPSTATIC mcasttoktype_t mc_lexer_lookupident(const char* ident, int len)
{
    static struct
    {
        const char* value;
        int len;
        mcasttoktype_t type;
    } keywords[] = {
        { "fn", 2, TOKEN_FUNCTION },       { "const", 5, TOKEN_CONST }, { "var", 3, TOKEN_VAR },       { "true", 4, TOKEN_TRUE },
        { "false", 5, TOKEN_FALSE },       { "if", 2, TOKEN_IF },       { "else", 4, TOKEN_ELSE },     { "return", 6, TOKEN_RETURN },
        { "while", 5, TOKEN_WHILE },       { "break", 5, TOKEN_BREAK }, { "for", 3, TOKEN_FOR },       { "in", 2, TOKEN_IN },
        { "continue", 8, TOKEN_CONTINUE }, { "null", 4, TOKEN_NULL },   { "import", 6, TOKEN_IMPORT }, { "recover", 7, TOKEN_RECOVER },
    };

    for(int i = 0; i < ARCANE_ARRAY_LEN(keywords); i++)
    {
        if(keywords[i].len == len && ARCANE_STRNEQ(ident, keywords[i].value, len))
        {
            return keywords[i].type;
        }
    }

    return TOKEN_IDENT;
}

TMPSTATIC void mc_lexer_skipspace(mcastlexer_t* lex)
{
    char ch = lex->ch;
    while(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
    {
        mc_lexer_readchar(lex);
        ch = lex->ch;
    }
}

TMPSTATIC bool mc_lexer_addline(mcastlexer_t* lex, int offset)
{
    bool ok;
    if(!lex->file)
    {
        return true;
    }

    if(lex->line < ptrarray_count(lex->file->lines))
    {
        return true;
    }

    const char* linestart = lex->input + offset;
    const char* newlineptr = strchr(linestart, '\n');
    char* line = NULL;
    if(!newlineptr)
    {
        line = arcane_strdup(lex->pstate, linestart);
    }
    else
    {
        size_t linelen = newlineptr - linestart;
        line = arcane_strndup(lex->pstate, linestart, linelen);
    }
    if(!line)
    {
        lex->failed = true;
        return false;
    }
    ok = ptrarray_push(lex->file->lines, line);
    if(!ok)
    {
        lex->failed = true;
        allocator_free(lex->pstate, line);
        return false;
    }
    return true;
}

TMPSTATIC mcastcompiledfile_t* mc_compiledfile_make(mcstate_t* state, const char* path)
{
    mcastcompiledfile_t* file = (mcastcompiledfile_t*)allocator_malloc(state, sizeof(mcastcompiledfile_t));
    if(!file)
    {
        return NULL;
    }
    memset(file, 0, sizeof(mcastcompiledfile_t));
    file->pstate = state;
    const char* lastslashpos = strrchr(path, '/');
    if(lastslashpos)
    {
        size_t len = lastslashpos - path + 1;
        file->dir_path = arcane_strndup(state, path, len);
    }
    else
    {
        file->dir_path = arcane_strdup(state, "");
    }
    if(!file->dir_path)
    {
        goto error;
    }
    file->path = arcane_strdup(state, path);
    if(!file->path)
    {
        goto error;
    }
    file->lines = ptrarray_make(state);
    if(!file->lines)
    {
        goto error;
    }
    return file;
error:
    mc_compiledfile_destroy(file);
    return NULL;
}

TMPSTATIC void mc_compiledfile_destroy(mcastcompiledfile_t* file)
{
    mcstate_t* state;
    if(!file)
    {
        return;
    }
    state = file->pstate;
    for(int i = 0; i < ptrarray_count(file->lines); i++)
    {
        void* item = ptrarray_get(file->lines, i);
        allocator_free(state, item);
    }
    ptrarray_destroy(file->lines);
    allocator_free(state, file->dir_path);
    allocator_free(state, file->path);
    allocator_free(state, file);
}

TMPSTATIC mcastcompiler_t* mc_compiler_make(mcstate_t* state, mcconfig_t* config, mcgcmemory_t* mem, mcerrlist_t* errors, mcptrarray_t* files, mcglobalstore_t* global_store)
{
    bool ok;
    mcastcompiler_t* comp = (mcastcompiler_t*)allocator_malloc(state, sizeof(mcastcompiler_t));
    if(!comp)
    {
        return NULL;
    }
    ok = mc_compiler_init(comp, state, config, mem, errors, files, global_store);
    if(!ok)
    {
        allocator_free(state, comp);
        return NULL;
    }
    comp->pstate = state; 
    return comp;
}

TMPSTATIC void mc_compiler_destroy(mcastcompiler_t* comp)
{
    mcstate_t* state;
    if(!comp)
    {
        return;
    }
    state = comp->pstate;
    mc_compiler_deinit(comp);
    allocator_free(state, comp);
}

TMPSTATIC compilation_result_t* mc_compiler_compilesource(mcastcompiler_t* comp, const char* code)
{
    bool ok;
    mcastscopecomp_t* compilation_scope = mc_compiler_getcompilationscope(comp);

    ARCANE_ASSERT(array_count(comp->src_positions_stack) == 0);
    ARCANE_ASSERT(array_count(compilation_scope->bytecode) == 0);
    ARCANE_ASSERT(array_count(compilation_scope->break_ip_stack) == 0);
    ARCANE_ASSERT(array_count(compilation_scope->continue_ip_stack) == 0);

    array_clear(comp->src_positions_stack);
    array_clear(compilation_scope->bytecode);
    array_clear(compilation_scope->src_positions);
    array_clear(compilation_scope->break_ip_stack);
    array_clear(compilation_scope->continue_ip_stack);

    mcastcompiler_t compshallowcopy;
    ok = mc_compiler_initshallowcopy(&compshallowcopy, comp);
    if(!ok)
    {
        return NULL;
    }

    ok = mc_compiler_docompilesource(comp, code);
    if(!ok)
    {
        goto err;
    }
    /* might've changed */
    compilation_scope = mc_compiler_getcompilationscope(comp);
    ARCANE_ASSERT(compilation_scope->outer == NULL);
    compilation_scope = mc_compiler_getcompilationscope(comp);
    compilation_result_t* res = compilation_scope_orphan_result(compilation_scope);
    if(!res)
    {
        goto err;
    }
    mc_compiler_deinit(&compshallowcopy);
    return res;
err:
    mc_compiler_deinit(comp);
    *comp = compshallowcopy;
    return NULL;
}


TMPSTATIC mcastsymtable_t* mc_compiler_getsymtable(mcastcompiler_t* comp)
{
    mcastscopefile_t* filescope = ptrarray_top(comp->file_scopes);
    if(!filescope)
    {
        ARCANE_ASSERT(false);
        return NULL;
    }
    return filescope->symbol_table;
}

TMPSTATIC void mc_compiler_setsymtable(mcastcompiler_t* comp, mcastsymtable_t* table)
{
    mcastscopefile_t* filescope = ptrarray_top(comp->file_scopes);
    if(!filescope)
    {
        ARCANE_ASSERT(false);
        return;
    }
    filescope->symbol_table = table;
}

TMPSTATIC mcbasicarray_t* mc_compiler_getconstants(mcastcompiler_t* comp)
{
    return comp->constants;
}

TMPSTATIC bool mc_compiler_init(mcastcompiler_t* comp, mcstate_t* state, mcconfig_t* config, mcgcmemory_t* mem, mcerrlist_t* errors, mcptrarray_t* files, mcglobalstore_t* global_store)
{
    bool ok;
    memset(comp, 0, sizeof(mcastcompiler_t));
    comp->pstate = state;
    comp->config = config;
    comp->mem = mem;
    comp->errors = errors;
    comp->files = files;
    comp->global_store = global_store;

    comp->file_scopes = ptrarray_make(state);
    if(!comp->file_scopes)
    {
        goto err;
    }
    comp->constants = array_make(state, sizeof(mcvalue_t));
    if(!comp->constants)
    {
        goto err;
    }
    comp->src_positions_stack = array_make(state, sizeof(mcastlocation_t));
    if(!comp->src_positions_stack)
    {
        goto err;
    }
    comp->modules = dict_make_(state, (dictitemcopyfn)mc_module_copy, (dictitemdestroyfn)mc_module_destroy);
    if(!comp->modules)
    {
        goto err;
    }
    ok = mc_compiler_pushcompilationscope(comp);
    if(!ok)
    {
        goto err;
    }
    ok = mc_compiler_filescopepush(comp, "none");
    if(!ok)
    {
        goto err;
    }
    comp->string_constants_positions = dict_make_(comp->pstate, NULL, NULL);
    if(!comp->string_constants_positions)
    {
        goto err;
    }

    return true;
err:
    mc_compiler_deinit(comp);
    return false;
}

TMPSTATIC void mc_compiler_deinit(mcastcompiler_t* comp)
{
    if(!comp)
    {
        return;
    }
    for(int i = 0; i < dict_count(comp->string_constants_positions); i++)
    {
        int* val = dict_get_value_at(comp->string_constants_positions, i);
        allocator_free(comp->pstate, val);
    }
    dict_destroy(comp->string_constants_positions);

    while(ptrarray_count(comp->file_scopes) > 0)
    {
        mc_compiler_filescopepop(comp);
    }
    while(mc_compiler_getcompilationscope(comp))
    {
        mc_compiler_popcompilationscope(comp);
    }
    dict_destroy_with_items(comp->modules);
    array_destroy(comp->src_positions_stack);

    array_destroy(comp->constants);
    ptrarray_destroy(comp->file_scopes);
    memset(comp, 0, sizeof(mcastcompiler_t));
}

TMPSTATIC bool mc_compiler_initshallowcopy(mcastcompiler_t* copy, mcastcompiler_t* src)
{
    bool ok;
    ok = mc_compiler_init(copy, src->pstate, src->config, src->mem, src->errors, src->files, src->global_store);
    if(!ok)
    {
        return false;
    }

    mcastsymtable_t* srcst = mc_compiler_getsymtable(src);
    ARCANE_ASSERT(ptrarray_count(src->file_scopes) == 1);
    ARCANE_ASSERT(srcst->outer == NULL);
    mcastsymtable_t* srcstcopy = symbol_table_copy(srcst);
    if(!srcstcopy)
    {
        goto err;
    }
    mcastsymtable_t* copyst = mc_compiler_getsymtable(copy);
    symbol_table_destroy(copyst);
    copyst = NULL;
    mc_compiler_setsymtable(copy, srcstcopy);

    mcgenericdict_t* modulescopy = dict_copy_with_items(src->modules);
    if(!modulescopy)
    {
        goto err;
    }
    dict_destroy_with_items(copy->modules);
    copy->modules = modulescopy;

    mcbasicarray_t* constantscopy = array_copy(src->constants);
    if(!constantscopy)
    {
        goto err;
    }
    array_destroy(copy->constants);
    copy->constants = constantscopy;

    for(int i = 0; i < dict_count(src->string_constants_positions); i++)
    {
        const char* key = dict_get_key_at(src->string_constants_positions, i);
        int* val = dict_get_value_at(src->string_constants_positions, i);
        int* valcopy = (int*)allocator_malloc(src->pstate, sizeof(int));
        if(!valcopy)
        {
            goto err;
        }
        *valcopy = *val;
        ok = dict_set(copy->string_constants_positions, key, valcopy);
        if(!ok)
        {
            allocator_free(src->pstate, valcopy);
            goto err;
        }
    }

    mcastscopefile_t* srcfilescope = ptrarray_top(src->file_scopes);
    mcastscopefile_t* copyfilescope = ptrarray_top(copy->file_scopes);

    mcptrarray_t* srcloadedmodulenames = srcfilescope->loaded_module_names;
    mcptrarray_t* copyloadedmodulenames = copyfilescope->loaded_module_names;

    for(int i = 0; i < ptrarray_count(srcloadedmodulenames); i++)
    {
        const char* loadedname = ptrarray_get(srcloadedmodulenames, i);
        char* loadednamecopy = arcane_strdup(copy->pstate, loadedname);
        if(!loadednamecopy)
        {
            goto err;
        }
        ok = ptrarray_push(copyloadedmodulenames, loadednamecopy);
        if(!ok)
        {
            allocator_free(copy->pstate, loadednamecopy);
            goto err;
        }
    }

    return true;
err:
    mc_compiler_deinit(copy);
    return false;
}


static mcopdefinition_t g_definitions[OPCODE_MAX + 1] = {
    { "NONE", 0, { 0 } },
    { "CONSTANT", 1, { 2 } },
    { "ADD", 0, { 0 } },
    { "POP", 0, { 0 } },
    { "SUB", 0, { 0 } },
    { "MUL", 0, { 0 } },
    { "DIV", 0, { 0 } },
    { "MOD", 0, { 0 } },
    { "TRUE", 0, { 0 } },
    { "FALSE", 0, { 0 } },
    { "COMPARE", 0, { 0 } },
    { "COMPARE_EQ", 0, { 0 } },
    { "EQUAL", 0, { 0 } },
    { "NOT_EQUAL", 0, { 0 } },
    { "GREATER_THAN", 0, { 0 } },
    { "GREATER_THAN_EQUAL", 0, { 0 } },
    { "MINUS", 0, { 0 } },
    { "BANG", 0, { 0 } },
    { "JUMP", 1, { 2 } },
    { "JUMP_IF_FALSE", 1, { 2 } },
    { "JUMP_IF_TRUE", 1, { 2 } },
    { "NULL", 0, { 0 } },
    { "GET_MODULE_GLOBAL", 1, { 2 } },
    { "SET_MODULE_GLOBAL", 1, { 2 } },
    { "DEFINE_MODULE_GLOBAL", 1, { 2 } },
    { "ARRAY", 1, { 2 } },
    { "MAP_START", 1, { 2 } },
    { "MAP_END", 1, { 2 } },
    { "GET_THIS", 0, { 0 } },
    { "GET_INDEX", 0, { 0 } },
    { "SET_INDEX", 0, { 0 } },
    { "GET_VALUE_AT", 0, { 0 } },
    { "CALL", 1, { 1 } },
    { "RETURN_VALUE", 0, { 0 } },
    { "RETURN", 0, { 0 } },
    { "GET_LOCAL", 1, { 1 } },
    { "DEFINE_LOCAL", 1, { 1 } },
    { "SET_LOCAL", 1, { 1 } },
    { "GET_ARCANE_GLOBAL", 1, { 2 } },
    { "FUNCTION", 2, { 2, 1 } },
    { "GET_FREE", 1, { 1 } },
    { "SET_FREE", 1, { 1 } },
    { "CURRENT_FUNCTION", 0, { 0 } },
    { "DUP", 0, { 0 } },
    { "NUMBER", 1, { 8 } },
    { "LEN", 0, { 0 } },
    { "SET_RECOVER", 1, { 2 } },
    { "OR", 0, { 0 } },
    { "XOR", 0, { 0 } },
    { "AND", 0, { 0 } },
    { "LSHIFT", 0, { 0 } },
    { "RSHIFT", 0, { 0 } },
    { "INVALID_MAX", 0, { 0 } },
};

mcopdefinition_t* opcode_lookup(opcode_t op)
{
    if(op <= OPCODE_NONE || op >= OPCODE_MAX)
    {
        return NULL;
    }
    return &g_definitions[op];
}

const char* opcode_get_name(opcode_t op)
{
    if(op <= OPCODE_NONE || op >= OPCODE_MAX)
    {
        return NULL;
    }
    return g_definitions[op].name;
}

int mc_compiler_gencode(opcode_t op, int operandscount, uint64_t* operands, mcbasicarray_t* res)
{
    bool ok;
    mcopdefinition_t* def = opcode_lookup(op);
    if(!def)
    {
        return 0;
    }

    int instrlen = 1;
    for(int i = 0; i < def->num_operands; i++)
    {
        instrlen += def->operand_widths[i];
    }

    uint8_t val = op;
    ok = false;

    ok = array_add(res, &val);
    if(!ok)
    {
        return 0;
    }

#define APPEND_BYTE(n)                           \
    do                                           \
    {                                            \
        val = (uint8_t)(operands[i] >> (n * 8)); \
        ok = array_add(res, &val);               \
        if(!ok)                                  \
        {                                        \
            return 0;                            \
        }                                        \
    } while(0)

    for(int i = 0; i < operandscount; i++)
    {
        int width = def->operand_widths[i];
        switch(width)
        {
            case 1:
            {
                APPEND_BYTE(0);
                break;
            }
            case 2:
            {
                APPEND_BYTE(1);
                APPEND_BYTE(0);
                break;
            }
            case 4:
            {
                APPEND_BYTE(3);
                APPEND_BYTE(2);
                APPEND_BYTE(1);
                APPEND_BYTE(0);
                break;
            }
            case 8:
            {
                APPEND_BYTE(7);
                APPEND_BYTE(6);
                APPEND_BYTE(5);
                APPEND_BYTE(4);
                APPEND_BYTE(3);
                APPEND_BYTE(2);
                APPEND_BYTE(1);
                APPEND_BYTE(0);
                break;
            }
            default:
            {
                ARCANE_ASSERT(false);
                break;
            }
        }
    }
#undef APPEND_BYTE
    return instrlen;
}


TMPSTATIC int mc_compiler_emit(mcastcompiler_t* comp, opcode_t op, int operandscount, uint64_t* operands)
{
    bool ok;
    int ip = mc_compiler_getip(comp);
    int len = mc_compiler_gencode(op, operandscount, operands, mc_compiler_getbytecode(comp));
    if(len == 0)
    {
        return -1;
    }
    for(int i = 0; i < len; i++)
    {
        mcastlocation_t* srcpos = array_top(comp->src_positions_stack);
        ARCANE_ASSERT(srcpos->line >= 0);
        ARCANE_ASSERT(srcpos->column >= 0);
        ok = array_add(mc_compiler_getsrcpositions(comp), srcpos);
        if(!ok)
        {
            return -1;
        }
    }
    mcastscopecomp_t* compilation_scope = mc_compiler_getcompilationscope(comp);
    compilation_scope->last_opcode = op;
    return ip;
}

TMPSTATIC mcastscopecomp_t* mc_compiler_getcompilationscope(mcastcompiler_t* comp)
{
    return comp->compilation_scope;
}

TMPSTATIC bool mc_compiler_pushcompilationscope(mcastcompiler_t* comp)
{
    mcastscopecomp_t* currentscope = mc_compiler_getcompilationscope(comp);
    mcastscopecomp_t* newscope = compilation_scope_make(comp->pstate, currentscope);
    if(!newscope)
    {
        return false;
    }
    mc_compiler_setcompilationscope(comp, newscope);
    return true;
}

TMPSTATIC void mc_compiler_popcompilationscope(mcastcompiler_t* comp)
{
    mcastscopecomp_t* currentscope = mc_compiler_getcompilationscope(comp);
    ARCANE_ASSERT(currentscope);
    mc_compiler_setcompilationscope(comp, currentscope->outer);
    compilation_scope_destroy(currentscope);
}

TMPSTATIC bool mc_compiler_pushsymtable(mcastcompiler_t* comp, int globaloffset)
{
    mcastscopefile_t* filescope = ptrarray_top(comp->file_scopes);
    if(!filescope)
    {
        ARCANE_ASSERT(false);
        return false;
    }
    mcastsymtable_t* currenttable = filescope->symbol_table;
    filescope->symbol_table = symbol_table_make(comp->pstate, currenttable, comp->global_store, globaloffset);
    if(!filescope->symbol_table)
    {
        filescope->symbol_table = currenttable;
        return false;
    }
    return true;
}

TMPSTATIC void mc_compiler_popsymtable(mcastcompiler_t* comp)
{
    mcastscopefile_t* filescope = ptrarray_top(comp->file_scopes);
    if(!filescope)
    {
        ARCANE_ASSERT(false);
        return;
    }
    mcastsymtable_t* currenttable = filescope->symbol_table;
    if(!currenttable)
    {
        ARCANE_ASSERT(false);
        return;
    }
    filescope->symbol_table = currenttable->outer;
    symbol_table_destroy(currenttable);
}

TMPSTATIC opcode_t mc_compiler_getlastopcode(mcastcompiler_t* comp)
{
    mcastscopecomp_t* currentscope = mc_compiler_getcompilationscope(comp);
    return currentscope->last_opcode;
}

TMPSTATIC bool mc_compiler_docompilesource(mcastcompiler_t* comp, const char* code)
{
    bool ok;
    mcastscopefile_t* filescope = ptrarray_top(comp->file_scopes);
    ARCANE_ASSERT(filescope);

    mcptrarray_t* statements = parser_parse_all(filescope->parser, code, filescope->file);
    if(!statements)
    {
        /* errors are added by parser */
        return false;
    }

    ok = mc_compiler_compilestmtlist(comp, statements);

    ptrarray_destroy_with_items_(statements, (ptrarrayitemdestroyfn)statement_destroy);
    /*
    * //Left for debugging purposes
    if (ok)
    {
        mcprintstate_t *buf = mc_printer_make(NULL, NULL);
        code_to_string(
            array_data(comp->compilation_scope->bytecode),
            array_data(comp->compilation_scope->src_positions),
            array_count(comp->compilation_scope->bytecode), buf);
        puts(mc_printer_getstring(buf));
        mc_printer_destroy(buf);
    }
    */

    return ok;
}

TMPSEMISTATIC bool mc_compiler_compilestmtlist(mcastcompiler_t* comp, mcptrarray_t* statements)
{
    bool ok;
    ok = true;
    for(int i = 0; i < ptrarray_count(statements); i++)
    {
        mcastexpression_t* stmt = ptrarray_get(statements, i);
        ok = mc_compiler_compilestatement(comp, stmt);
        if(!ok)
        {
            break;
        }
    }
    return ok;
}

TMPSTATIC bool mc_compiler_compileimport(mcastcompiler_t* comp, mcastexpression_t* importstmt)
{
    bool ok;
    /* todo: split into smaller functions */
    bool result = false;
    char* filepath = NULL;
    char* code = NULL;
    mcastscopefile_t* filescope = ptrarray_top(comp->file_scopes);
    const char* modulepath = importstmt->import.path;
    const char* modulename = mc_util_getmodulename(modulepath);
    for(int i = 0; i < ptrarray_count(filescope->loaded_module_names); i++)
    {
        const char* loadedname = ptrarray_get(filescope->loaded_module_names, i);
        if(kg_streq(loadedname, modulename))
        {
            errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, importstmt->pos, "Module \"%s\" was already imported", modulename);
            result = false;
            goto end;
        }
    }
    mcprintstate_t* filepathbuf = mc_printer_make(comp->pstate, NULL);
    if(!filepathbuf)
    {
        result = false;
        goto end;
    }
    if(kg_is_path_absolute(modulepath))
    {
        mc_printer_appendf(filepathbuf, "%s.arcane", modulepath);
    }
    else
    {
        mc_printer_appendf(filepathbuf, "%s%s.arcane", filescope->file->dir_path, modulepath);
    }

    if(mc_printer_failed(filepathbuf))
    {
        mc_printer_destroy(filepathbuf);
        result = false;
        goto end;
    }

    const char* filepathnoncanonicalised = mc_printer_getstring(filepathbuf);
    filepath = kg_canonicalise_path(comp->pstate, filepathnoncanonicalised);
    mc_printer_destroy(filepathbuf);
    if(!filepath)
    {
        result = false;
        goto end;
    }

    mcastsymtable_t* symbol_table = mc_compiler_getsymtable(comp);
    if(symbol_table->outer != NULL || ptrarray_count(symbol_table->block_scopes) > 1)
    {
        errors_add_error(comp->errors, ARCANE_ERROR_COMPILATION, importstmt->pos, "Modules can only be imported in global scope");
        result = false;
        goto end;
    }

    for(int i = 0; i < ptrarray_count(comp->file_scopes); i++)
    {
        mcastscopefile_t* fs = ptrarray_get(comp->file_scopes, i);
        if(ARCANE_STREQ(fs->file->path, filepath))
        {
            errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, importstmt->pos, "Cyclic reference of file \"%s\"", filepath);
            result = false;
            goto end;
        }
    }

    module_t* module = dict_get(comp->modules, filepath);
    if(!module)
    {
        /* todo: create new module function */
        if(!comp->config->fileio.read_file.read_file)
        {
            errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, importstmt->pos, "Cannot import module \"%s\", file read function not configured", filepath);
            result = false;
            goto end;
        }

        code = comp->config->fileio.read_file.read_file(comp->config->fileio.read_file.context, filepath);
        if(!code)
        {
            errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, importstmt->pos, "Reading module file \"%s\" failed", filepath);
            result = false;
            goto end;
        }

        module = mc_module_make(comp->pstate, modulename);
        if(!module)
        {
            result = false;
            goto end;
        }

        ok = mc_compiler_filescopepush(comp, filepath);
        if(!ok)
        {
            mc_module_destroy(module);
            result = false;
            goto end;
        }

        ok = mc_compiler_docompilesource(comp, code);
        if(!ok)
        {
            mc_module_destroy(module);
            result = false;
            goto end;
        }

        mcastsymtable_t* st = mc_compiler_getsymtable(comp);
        for(int i = 0; i < symbol_table_get_module_global_symbol_count(st); i++)
        {
            mcastsymbol_t* symbol = symbol_table_get_module_global_symbol_at(st, i);
            mc_module_addsymbol(module, symbol);
        }

        mc_compiler_filescopepop(comp);

        ok = dict_set(comp->modules, filepath, module);
        if(!ok)
        {
            mc_module_destroy(module);
            result = false;
            goto end;
        }
    }

    for(int i = 0; i < ptrarray_count(module->symbols); i++)
    {
        mcastsymbol_t* symbol = ptrarray_get(module->symbols, i);
        ok = symbol_table_add_module_symbol(symbol_table, symbol);
        if(!ok)
        {
            result = false;
            goto end;
        }
    }

    char* namecopy = arcane_strdup(comp->pstate, modulename);
    if(!namecopy)
    {
        result = false;
        goto end;
    }

    ok = ptrarray_push(filescope->loaded_module_names, namecopy);
    if(!ok)
    {
        allocator_free(comp->pstate, namecopy);
        result = false;
        goto end;
    }

    result = true;

end:
    allocator_free(comp->pstate, filepath);
    allocator_free(comp->pstate, code);
    return result;
}

TMPSEMISTATIC bool mc_compiler_compilestatement(mcastcompiler_t* comp, mcastexpression_t* stmt)
{
    bool ok;
    ok = false;
    int ip = -1;

    ok = array_push(comp->src_positions_stack, &stmt->pos);
    if(!ok)
    {
        return false;
    }

    mcastscopecomp_t* compilation_scope = mc_compiler_getcompilationscope(comp);
    mcastsymtable_t* symbol_table = mc_compiler_getsymtable(comp);
    switch(stmt->type)
    {
        case MC_EXPR_STMTEXPRESSION:
        {
            ok = mc_compiler_compileexpression(comp, stmt->expression);
            if(!ok)
            {
                return false;
            }
            ip = mc_compiler_emit(comp, OPCODE_POP, 0, NULL);
            if(ip < 0)
            {
                return false;
            }
            break;
        }
        case MC_EXPR_STMTDEFINE:
        {
            ok = mc_compiler_compileexpression(comp, stmt->define.value);
            if(!ok)
            {
                return false;
            }

            mcastsymbol_t* symbol = mc_compiler_defsymbol(comp, stmt->define.name->pos, stmt->define.name->value, stmt->define.assignable, false);
            if(!symbol)
            {
                return false;
            }

            ok = mc_compiler_storesymbol(comp, symbol, true);
            if(!ok)
            {
                return false;
            }

            break;
        }
        case MC_EXPR_STMTIF:
        {
            if_statement_t* ifstmt = &stmt->if_statement;

            mcbasicarray_t* jumptoendips = array_make(comp->pstate, sizeof(int));
            if(!jumptoendips)
            {
                goto statementiferror;
            }

            for(int i = 0; i < ptrarray_count(ifstmt->cases); i++)
            {
                if_case_t* ifcase = ptrarray_get(ifstmt->cases, i);

                ok = mc_compiler_compileexpression(comp, ifcase->test);
                if(!ok)
                {
                    goto statementiferror;
                }

                int nextcasejumpip = mc_compiler_emit(comp, OPCODE_JUMP_IF_FALSE, 1, (uint64_t[]){ 0xbeef });

                ok = mc_compiler_compilecodeblock(comp, ifcase->consequence);
                if(!ok)
                {
                    goto statementiferror;
                }

                /* don't emit jump for the last statement */
                if(i < (ptrarray_count(ifstmt->cases) - 1) || ifstmt->alternative)
                {
                    int jumptoendip = mc_compiler_emit(comp, OPCODE_JUMP, 1, (uint64_t[]){ 0xbeef });
                    ok = array_add(jumptoendips, &jumptoendip);
                    if(!ok)
                    {
                        goto statementiferror;
                    }
                }

                int afterelifip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, nextcasejumpip + 1, afterelifip);
            }

            if(ifstmt->alternative)
            {
                ok = mc_compiler_compilecodeblock(comp, ifstmt->alternative);
                if(!ok)
                {
                    goto statementiferror;
                }
            }

            int afteraltip = mc_compiler_getip(comp);

            for(int i = 0; i < array_count(jumptoendips); i++)
            {
                int* pos = array_get(jumptoendips, i);
                mc_compiler_changeuint16operand(comp, *pos + 1, afteraltip);
            }

            array_destroy(jumptoendips);

            break;
        statementiferror:
            array_destroy(jumptoendips);
            return false;
        }
        case MC_EXPR_STMTRETURN:
        {
            if(compilation_scope->outer == NULL)
            {
                errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, stmt->pos, "Nothing to return from");
                return false;
            }
            ip = -1;
            if(stmt->return_value)
            {
                ok = mc_compiler_compileexpression(comp, stmt->return_value);
                if(!ok)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, OPCODE_RETURN_VALUE, 0, NULL);
            }
            else
            {
                ip = mc_compiler_emit(comp, OPCODE_RETURN, 0, NULL);
            }
            if(ip < 0)
            {
                return false;
            }
            break;
        }
        case MC_EXPR_STMTLOOPWHILE:
        {
            while_loop_statement_t* loop = &stmt->while_loop;

            int beforetestip = mc_compiler_getip(comp);

            ok = mc_compiler_compileexpression(comp, loop->test);
            if(!ok)
            {
                return false;
            }

            int aftertestip = mc_compiler_getip(comp);
            ip = mc_compiler_emit(comp, OPCODE_JUMP_IF_TRUE, 1, (uint64_t[]){ aftertestip + 6 });
            if(ip < 0)
            {
                return false;
            }

            int jumptoafterbodyip = mc_compiler_emit(comp, OPCODE_JUMP, 1, (uint64_t[]){ 0xdead });
            if(jumptoafterbodyip < 0)
            {
                return false;
            }

            ok = mc_compiler_pushcontinueip(comp, beforetestip);
            if(!ok)
            {
                return false;
            }

            ok = mc_compiler_pushbreakip(comp, jumptoafterbodyip);
            if(!ok)
            {
                return false;
            }

            ok = mc_compiler_compilecodeblock(comp, loop->body);
            if(!ok)
            {
                return false;
            }

            mc_compiler_popbreakip(comp);
            mc_compiler_popcontinueip(comp);

            ip = mc_compiler_emit(comp, OPCODE_JUMP, 1, (uint64_t[]){ beforetestip });
            if(ip < 0)
            {
                return false;
            }

            int afterbodyip = mc_compiler_getip(comp);
            mc_compiler_changeuint16operand(comp, jumptoafterbodyip + 1, afterbodyip);

            break;
        }
        case MC_EXPR_STMTBREAK:
        {
            int breakip = mc_compiler_getbreakip(comp);
            if(breakip < 0)
            {
                errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, stmt->pos, "Nothing to break from.");
                return false;
            }
            ip = mc_compiler_emit(comp, OPCODE_JUMP, 1, (uint64_t[]){ breakip });
            if(ip < 0)
            {
                return false;
            }
            break;
        }
        case MC_EXPR_STMTCONTINUE:
        {
            int continueip = mc_compiler_getcontinueip(comp);
            if(continueip < 0)
            {
                errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, stmt->pos, "Nothing to continue from.");
                return false;
            }
            ip = mc_compiler_emit(comp, OPCODE_JUMP, 1, (uint64_t[]){ continueip });
            if(ip < 0)
            {
                return false;
            }
            break;
        }
        case MC_EXPR_STMTLOOPFOREACH:
        {
            foreach_statement_t* foreach = &stmt->foreach;
            ok = symbol_table_push_block_scope(symbol_table);
            if(!ok)
            {
                return false;
            }

            /* Init */
            mcastsymbol_t* indexsymbol = mc_compiler_defsymbol(comp, stmt->pos, "@i", false, true);
            if(!indexsymbol)
            {
                return false;
            }

            ip = mc_compiler_emit(comp, OPCODE_NUMBER, 1, (uint64_t[]){ 0 });
            if(ip < 0)
            {
                return false;
            }

            ok = mc_compiler_storesymbol(comp, indexsymbol, true);
            if(!ok)
            {
                return false;
            }

            mcastsymbol_t* sourcesymbol = NULL;
            if(foreach->source->type == MC_EXPR_IDENT)
            {
                sourcesymbol = symbol_table_resolve(symbol_table, foreach->source->ident->value);
                if(!sourcesymbol)
                {
                    errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, foreach->source->pos, "Symbol \"%s\" could not be resolved", foreach->source->ident->value);
                    return false;
                }
            }
            else
            {
                ok = mc_compiler_compileexpression(comp, foreach->source);
                if(!ok)
                {
                    return false;
                }
                sourcesymbol = mc_compiler_defsymbol(comp, foreach->source->pos, "@source", false, true);
                if(!sourcesymbol)
                {
                    return false;
                }
                ok = mc_compiler_storesymbol(comp, sourcesymbol, true);
                if(!ok)
                {
                    return false;
                }
            }

            /* Update */
            int jumptoafterupdateip = mc_compiler_emit(comp, OPCODE_JUMP, 1, (uint64_t[]){ 0xbeef });
            if(jumptoafterupdateip < 0)
            {
                return false;
            }

            int updateip = mc_compiler_getip(comp);
            ok = mc_compiler_readsymbol(comp, indexsymbol);
            if(!ok)
            {
                return false;
            }

            ip = mc_compiler_emit(comp, OPCODE_NUMBER, 1, (uint64_t[]){ arcane_double_to_uint64(1) });
            if(ip < 0)
            {
                return false;
            }

            ip = mc_compiler_emit(comp, OPCODE_ADD, 0, NULL);
            if(ip < 0)
            {
                return false;
            }

            ok = mc_compiler_storesymbol(comp, indexsymbol, false);
            if(!ok)
            {
                return false;
            }

            int afterupdateip = mc_compiler_getip(comp);
            mc_compiler_changeuint16operand(comp, jumptoafterupdateip + 1, afterupdateip);

            /* Test */
            ok = array_push(comp->src_positions_stack, &foreach->source->pos);
            if(!ok)
            {
                return false;
            }

            ok = mc_compiler_readsymbol(comp, sourcesymbol);
            if(!ok)
            {
                return false;
            }

            ip = mc_compiler_emit(comp, OPCODE_LEN, 0, NULL);
            if(ip < 0)
            {
                return false;
            }

            array_pop(comp->src_positions_stack, NULL);
            ok = mc_compiler_readsymbol(comp, indexsymbol);
            if(!ok)
            {
                return false;
            }

            ip = mc_compiler_emit(comp, OPCODE_COMPARE, 0, NULL);
            if(ip < 0)
            {
                return false;
            }

            ip = mc_compiler_emit(comp, OPCODE_EQUAL, 0, NULL);
            if(ip < 0)
            {
                return false;
            }

            int aftertestip = mc_compiler_getip(comp);
            ip = mc_compiler_emit(comp, OPCODE_JUMP_IF_FALSE, 1, (uint64_t[]){ aftertestip + 6 });
            if(ip < 0)
            {
                return false;
            }

            int jumptoafterbodyip = mc_compiler_emit(comp, OPCODE_JUMP, 1, (uint64_t[]){ 0xdead });
            if(jumptoafterbodyip < 0)
            {
                return false;
            }

            ok = mc_compiler_readsymbol(comp, sourcesymbol);
            if(!ok)
            {
                return false;
            }

            ok = mc_compiler_readsymbol(comp, indexsymbol);
            if(!ok)
            {
                return false;
            }

            ip = mc_compiler_emit(comp, OPCODE_GET_VALUE_AT, 0, NULL);
            if(ip < 0)
            {
                return false;
            }

            mcastsymbol_t* itersymbol = mc_compiler_defsymbol(comp, foreach->iterator->pos, foreach->iterator->value, false, false);
            if(!itersymbol)
            {
                return false;
            }

            ok = mc_compiler_storesymbol(comp, itersymbol, true);
            if(!ok)
            {
                return false;
            }

            /* Body */
            ok = mc_compiler_pushcontinueip(comp, updateip);
            if(!ok)
            {
                return false;
            }

            ok = mc_compiler_pushbreakip(comp, jumptoafterbodyip);
            if(!ok)
            {
                return false;
            }

            ok = mc_compiler_compilecodeblock(comp, foreach->body);
            if(!ok)
            {
                return false;
            }

            mc_compiler_popbreakip(comp);
            mc_compiler_popcontinueip(comp);

            ip = mc_compiler_emit(comp, OPCODE_JUMP, 1, (uint64_t[]){ updateip });
            if(ip < 0)
            {
                return false;
            }

            int afterbodyip = mc_compiler_getip(comp);
            mc_compiler_changeuint16operand(comp, jumptoafterbodyip + 1, afterbodyip);

            symbol_table_pop_block_scope(symbol_table);
            break;
        }
        case MC_EXPR_STMTLOOPFORCLASSIC:
        {
            for_loop_statement_t* loop = &stmt->for_loop;

            ok = symbol_table_push_block_scope(symbol_table);
            if(!ok)
            {
                return false;
            }

            /* Init */
            int jumptoafterupdateip = 0;
            ok = false;
            if(loop->init)
            {
                ok = mc_compiler_compilestatement(comp, loop->init);
                if(!ok)
                {
                    return false;
                }
                jumptoafterupdateip = mc_compiler_emit(comp, OPCODE_JUMP, 1, (uint64_t[]){ 0xbeef });
                if(jumptoafterupdateip < 0)
                {
                    return false;
                }
            }

            /* Update */
            int updateip = mc_compiler_getip(comp);
            if(loop->update)
            {
                ok = mc_compiler_compileexpression(comp, loop->update);
                if(!ok)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, OPCODE_POP, 0, NULL);
                if(ip < 0)
                {
                    return false;
                }
            }

            if(loop->init)
            {
                int afterupdateip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, jumptoafterupdateip + 1, afterupdateip);
            }

            /* Test */
            if(loop->test)
            {
                ok = mc_compiler_compileexpression(comp, loop->test);
                if(!ok)
                {
                    return false;
                }
            }
            else
            {
                ip = mc_compiler_emit(comp, OPCODE_TRUE, 0, NULL);
                if(ip < 0)
                {
                    return false;
                }
            }
            int aftertestip = mc_compiler_getip(comp);

            ip = mc_compiler_emit(comp, OPCODE_JUMP_IF_TRUE, 1, (uint64_t[]){ aftertestip + 6 });
            if(ip < 0)
            {
                return false;
            }
            int jmptoafterbodyip = mc_compiler_emit(comp, OPCODE_JUMP, 1, (uint64_t[]){ 0xdead });
            if(jmptoafterbodyip < 0)
            {
                return false;
            }
            /* Body */
            ok = mc_compiler_pushcontinueip(comp, updateip);
            if(!ok)
            {
                return false;
            }

            ok = mc_compiler_pushbreakip(comp, jmptoafterbodyip);
            if(!ok)
            {
                return false;
            }

            ok = mc_compiler_compilecodeblock(comp, loop->body);
            if(!ok)
            {
                return false;
            }

            mc_compiler_popbreakip(comp);
            mc_compiler_popcontinueip(comp);

            ip = mc_compiler_emit(comp, OPCODE_JUMP, 1, (uint64_t[]){ updateip });
            if(ip < 0)
            {
                return false;
            }

            int afterbodyip = mc_compiler_getip(comp);
            mc_compiler_changeuint16operand(comp, jmptoafterbodyip + 1, afterbodyip);

            symbol_table_pop_block_scope(symbol_table);
            break;
        }
        case MC_EXPR_STMTBLOCK:
        {
            ok = mc_compiler_compilecodeblock(comp, stmt->block);
            if(!ok)
            {
                return false;
            }
            break;
        }
        case MC_EXPR_STMTIMPORT:
        {
            ok = mc_compiler_compileimport(comp, stmt);
            if(!ok)
            {
                return false;
            }
            break;
        }
        case MC_EXPR_STMTRECOVER:
        {
            recover_statement_t* recover = &stmt->recover;

            if(symbol_table_is_module_global_scope(symbol_table))
            {
                errors_add_error(comp->errors, ARCANE_ERROR_COMPILATION, stmt->pos, "Recover statement cannot be defined in global scope");
                return false;
            }

            if(!symbol_table_is_top_block_scope(symbol_table))
            {
                errors_add_error(comp->errors, ARCANE_ERROR_COMPILATION, stmt->pos, "Recover statement cannot be defined within other statements");
                return false;
            }

            int recover_ip = mc_compiler_emit(comp, OPCODE_SET_RECOVER, 1, (uint64_t[]){ 0xbeef });
            if(recover_ip < 0)
            {
                return false;
            }

            int jumptoafterrecoverip = mc_compiler_emit(comp, OPCODE_JUMP, 1, (uint64_t[]){ 0xbeef });
            if(jumptoafterrecoverip < 0)
            {
                return false;
            }

            int afterjumptorecoverip = mc_compiler_getip(comp);
            mc_compiler_changeuint16operand(comp, recover_ip + 1, afterjumptorecoverip);

            ok = symbol_table_push_block_scope(symbol_table);
            if(!ok)
            {
                return false;
            }

            mcastsymbol_t* errorsymbol = mc_compiler_defsymbol(comp, recover->error_ident->pos, recover->error_ident->value, false, false);
            if(!errorsymbol)
            {
                return false;
            }

            ok = mc_compiler_storesymbol(comp, errorsymbol, true);
            if(!ok)
            {
                return false;
            }

            ok = mc_compiler_compilecodeblock(comp, recover->body);
            if(!ok)
            {
                return false;
            }

            if(!mc_compiler_lastopcodeis(comp, OPCODE_RETURN) && !mc_compiler_lastopcodeis(comp, OPCODE_RETURN_VALUE))
            {
                errors_add_error(comp->errors, ARCANE_ERROR_COMPILATION, stmt->pos, "Recover body must end with a return statement");
                return false;
            }

            symbol_table_pop_block_scope(symbol_table);

            int afterrecoverip = mc_compiler_getip(comp);
            mc_compiler_changeuint16operand(comp, jumptoafterrecoverip + 1, afterrecoverip);

            break;
        }
        default:
        {
            ARCANE_ASSERT(false);
            return false;
        }
    }
    array_pop(comp->src_positions_stack, NULL);
    return true;
}

TMPSEMISTATIC bool mc_compiler_compileexpression(mcastcompiler_t* comp, mcastexpression_t* expr)
{
    bool ok;
    ok = false;
    int ip = -1;

    mcastexpression_t* exproptimised = optimise_expression(expr);
    if(exproptimised)
    {
        expr = exproptimised;
    }

    ok = array_push(comp->src_positions_stack, &expr->pos);
    if(!ok)
    {
        return false;
    }

    mcastscopecomp_t* compilation_scope = mc_compiler_getcompilationscope(comp);
    mcastsymtable_t* symbol_table = mc_compiler_getsymtable(comp);

    bool res = false;

    switch(expr->type)
    {
        case MC_EXPR_INFIX:
        {
            bool rearrange = false;

            opcode_t op = OPCODE_NONE;
            switch(expr->infix.op)
            {
                case OPERATOR_PLUS:
                    op = OPCODE_ADD;
                    break;
                case OPERATOR_MINUS:
                    op = OPCODE_SUB;
                    break;
                case OPERATOR_ASTERISK:
                    op = OPCODE_MUL;
                    break;
                case OPERATOR_SLASH:
                    op = OPCODE_DIV;
                    break;
                case OPERATOR_MODULUS:
                    op = OPCODE_MOD;
                    break;
                case OPERATOR_EQ:
                    op = OPCODE_EQUAL;
                    break;
                case OPERATOR_NOT_EQ:
                    op = OPCODE_NOT_EQUAL;
                    break;
                case OPERATOR_GT:
                    op = OPCODE_GREATER_THAN;
                    break;
                case OPERATOR_GTE:
                    op = OPCODE_GREATER_THAN_EQUAL;
                    break;
                case OPERATOR_LT:
                    op = OPCODE_GREATER_THAN;
                    rearrange = true;
                    break;
                case OPERATOR_LTE:
                    op = OPCODE_GREATER_THAN_EQUAL;
                    rearrange = true;
                    break;
                case OPERATOR_BIT_OR:
                    op = OPCODE_OR;
                    break;
                case OPERATOR_BIT_XOR:
                    op = OPCODE_XOR;
                    break;
                case OPERATOR_BIT_AND:
                    op = OPCODE_AND;
                    break;
                case OPERATOR_LSHIFT:
                    op = OPCODE_LSHIFT;
                    break;
                case OPERATOR_RSHIFT:
                    op = OPCODE_RSHIFT;
                    break;
                default:
                {
                    errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, expr->pos, "Unknown infix operator");
                    goto error;
                }
            }

            mcastexpression_t* left = rearrange ? expr->infix.right : expr->infix.left;
            mcastexpression_t* right = rearrange ? expr->infix.left : expr->infix.right;

            ok = mc_compiler_compileexpression(comp, left);
            if(!ok)
            {
                goto error;
            }

            ok = mc_compiler_compileexpression(comp, right);
            if(!ok)
            {
                goto error;
            }

            switch(expr->infix.op)
            {
                case OPERATOR_EQ:
                case OPERATOR_NOT_EQ:
                {
                    ip = mc_compiler_emit(comp, OPCODE_COMPARE_EQ, 0, NULL);
                    if(ip < 0)
                    {
                        goto error;
                    }
                    break;
                }
                case OPERATOR_GT:
                case OPERATOR_GTE:
                case OPERATOR_LT:
                case OPERATOR_LTE:
                {
                    ip = mc_compiler_emit(comp, OPCODE_COMPARE, 0, NULL);
                    if(ip < 0)
                    {
                        goto error;
                    }
                    break;
                }
                default:
                    break;
            }

            ip = mc_compiler_emit(comp, op, 0, NULL);
            if(ip < 0)
            {
                goto error;
            }

            break;
        }
        case MC_EXPR_NUMBERLITERAL:
        {
            double number = expr->number_literal;
            ip = mc_compiler_emit(comp, OPCODE_NUMBER, 1, (uint64_t[]){ arcane_double_to_uint64(number) });
            if(ip < 0)
            {
                goto error;
            }

            break;
        }
        case MC_EXPR_STRINGLITERAL:
        {
            int pos = 0;
            int* currentpos = dict_get(comp->string_constants_positions, expr->string_literal);
            if(currentpos)
            {
                pos = *currentpos;
            }
            else
            {
                mcvalue_t obj = mc_value_makestring(comp->pstate, expr->string_literal);
                if(object_is_null(obj))
                {
                    goto error;
                }

                pos = mc_compiler_addconstant(comp, obj);
                if(pos < 0)
                {
                    goto error;
                }

                int* posval = (int*)allocator_malloc(comp->pstate, sizeof(int));
                if(!posval)
                {
                    goto error;
                }

                *posval = pos;
                ok = dict_set(comp->string_constants_positions, expr->string_literal, posval);
                if(!ok)
                {
                    allocator_free(comp->pstate, posval);
                    goto error;
                }
            }

            ip = mc_compiler_emit(comp, OPCODE_CONSTANT, 1, (uint64_t[]){ pos });
            if(ip < 0)
            {
                goto error;
            }

            break;
        }
        case MC_EXPR_NULLLITERAL:
        {
            ip = mc_compiler_emit(comp, OPCODE_NULL, 0, NULL);
            if(ip < 0)
            {
                goto error;
            }
            break;
        }
        case MC_EXPR_BOOLLITERAL:
        {
            ip = mc_compiler_emit(comp, expr->bool_literal ? OPCODE_TRUE : OPCODE_FALSE, 0, NULL);
            if(ip < 0)
            {
                goto error;
            }
            break;
        }
        case MC_EXPR_ARRAYLITERAL:
        {
            for(int i = 0; i < ptrarray_count(expr->array); i++)
            {
                ok = mc_compiler_compileexpression(comp, ptrarray_get(expr->array, i));
                if(!ok)
                {
                    goto error;
                }
            }
            ip = mc_compiler_emit(comp, OPCODE_ARRAY, 1, (uint64_t[]){ ptrarray_count(expr->array) });
            if(ip < 0)
            {
                goto error;
            }
            break;
        }
        case MC_EXPR_MAPLITERAL:
        {
            map_literal_t* map = &expr->map;
            int len = ptrarray_count(map->keys);
            ip = mc_compiler_emit(comp, OPCODE_MAP_START, 1, (uint64_t[]){ len });
            if(ip < 0)
            {
                goto error;
            }

            for(int i = 0; i < len; i++)
            {
                mcastexpression_t* key = ptrarray_get(map->keys, i);
                mcastexpression_t* val = ptrarray_get(map->values, i);

                ok = mc_compiler_compileexpression(comp, key);
                if(!ok)
                {
                    goto error;
                }

                ok = mc_compiler_compileexpression(comp, val);
                if(!ok)
                {
                    goto error;
                }
            }

            ip = mc_compiler_emit(comp, OPCODE_MAP_END, 1, (uint64_t[]){ len });
            if(ip < 0)
            {
                goto error;
            }

            break;
        }
        case MC_EXPR_PREFIX:
        {
            ok = mc_compiler_compileexpression(comp, expr->prefix.right);
            if(!ok)
            {
                goto error;
            }

            opcode_t op = OPCODE_NONE;
            switch(expr->prefix.op)
            {
                case OPERATOR_MINUS:
                    op = OPCODE_MINUS;
                    break;
                case OPERATOR_BANG:
                    op = OPCODE_BANG;
                    break;
                default:
                {
                    errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, expr->pos, "Unknown prefix operator.");
                    goto error;
                }
            }
            ip = mc_compiler_emit(comp, op, 0, NULL);
            if(ip < 0)
            {
                goto error;
            }

            break;
        }
        case MC_EXPR_IDENT:
        {
            ident_t* ident = expr->ident;
            mcastsymbol_t* symbol = symbol_table_resolve(symbol_table, ident->value);
            if(!symbol)
            {
                errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, ident->pos, "Symbol \"%s\" could not be resolved", ident->value);
                goto error;
            }
            ok = mc_compiler_readsymbol(comp, symbol);
            if(!ok)
            {
                goto error;
            }

            break;
        }
        case MC_EXPR_INDEX:
        {
            index_expression_t* index = &expr->index_expr;
            ok = mc_compiler_compileexpression(comp, index->left);
            if(!ok)
            {
                goto error;
            }
            ok = mc_compiler_compileexpression(comp, index->index);
            if(!ok)
            {
                goto error;
            }
            ip = mc_compiler_emit(comp, OPCODE_GET_INDEX, 0, NULL);
            if(ip < 0)
            {
                goto error;
            }

            break;
        }
        case MC_EXPR_FUNCTIONLITERAL:
        {
            fn_literal_t* fn = &expr->fn_literal;

            ok = mc_compiler_pushcompilationscope(comp);
            if(!ok)
            {
                goto error;
            }

            ok = mc_compiler_pushsymtable(comp, 0);
            if(!ok)
            {
                goto error;
            }

            compilation_scope = mc_compiler_getcompilationscope(comp);
            symbol_table = mc_compiler_getsymtable(comp);

            if(fn->name)
            {
                mcastsymbol_t* fnsymbol = symbol_table_define_function_name(symbol_table, fn->name, false);
                if(!fnsymbol)
                {
                    errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, expr->pos, "Cannot define symbol \"%s\"", fn->name);
                    goto error;
                }
            }

            mcastsymbol_t* thissymbol = symbol_table_define_this(symbol_table);
            if(!thissymbol)
            {
                errors_add_error(comp->errors, ARCANE_ERROR_COMPILATION, expr->pos, "Cannot define \"this\" symbol");
                goto error;
            }

            for(int i = 0; i < ptrarray_count(expr->fn_literal.params); i++)
            {
                ident_t* param = ptrarray_get(expr->fn_literal.params, i);
                mcastsymbol_t* paramsymbol = mc_compiler_defsymbol(comp, param->pos, param->value, true, false);
                if(!paramsymbol)
                {
                    goto error;
                }
            }

            ok = mc_compiler_compilestmtlist(comp, fn->body->statements);
            if(!ok)
            {
                goto error;
            }

            if(!mc_compiler_lastopcodeis(comp, OPCODE_RETURN_VALUE) && !mc_compiler_lastopcodeis(comp, OPCODE_RETURN))
            {
                ip = mc_compiler_emit(comp, OPCODE_RETURN, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
            }

            mcptrarray_t* free_symbols = symbol_table->free_symbols;
            /* because it gets destroyed with compiler_pop_compilation_scope() */
            symbol_table->free_symbols = NULL;

            int num_locals = symbol_table->max_num_definitions;

            compilation_result_t* comp_res = compilation_scope_orphan_result(compilation_scope);
            if(!comp_res)
            {
                ptrarray_destroy_with_items_(free_symbols, (ptrarrayitemdestroyfn)symbol_destroy);
                goto error;
            }
            mc_compiler_popsymtable(comp);
            mc_compiler_popcompilationscope(comp);
            compilation_scope = mc_compiler_getcompilationscope(comp);
            symbol_table = mc_compiler_getsymtable(comp);

            mcvalue_t obj = object_make_function(comp->pstate, fn->name, comp_res, true, num_locals, ptrarray_count(fn->params), 0);

            if(object_is_null(obj))
            {
                ptrarray_destroy_with_items_(free_symbols, (ptrarrayitemdestroyfn)symbol_destroy);
                compilation_result_destroy(comp_res);
                goto error;
            }

            for(int i = 0; i < ptrarray_count(free_symbols); i++)
            {
                mcastsymbol_t* symbol = ptrarray_get(free_symbols, i);
                ok = mc_compiler_readsymbol(comp, symbol);
                if(!ok)
                {
                    ptrarray_destroy_with_items_(free_symbols, (ptrarrayitemdestroyfn)symbol_destroy);
                    goto error;
                }
            }

            int pos = mc_compiler_addconstant(comp, obj);
            if(pos < 0)
            {
                ptrarray_destroy_with_items_(free_symbols, (ptrarrayitemdestroyfn)symbol_destroy);
                goto error;
            }

            ip = mc_compiler_emit(comp, OPCODE_FUNCTION, 2, (uint64_t[]){ pos, ptrarray_count(free_symbols) });
            if(ip < 0)
            {
                ptrarray_destroy_with_items_(free_symbols, (ptrarrayitemdestroyfn)symbol_destroy);
                goto error;
            }

            ptrarray_destroy_with_items_(free_symbols, (ptrarrayitemdestroyfn)symbol_destroy);

            break;
        }
        case MC_EXPR_CALL:
        {
            ok = mc_compiler_compileexpression(comp, expr->call_expr.function);
            if(!ok)
            {
                goto error;
            }

            for(int i = 0; i < ptrarray_count(expr->call_expr.args); i++)
            {
                mcastexpression_t* argexpr = ptrarray_get(expr->call_expr.args, i);
                ok = mc_compiler_compileexpression(comp, argexpr);
                if(!ok)
                {
                    goto error;
                }
            }

            ip = mc_compiler_emit(comp, OPCODE_CALL, 1, (uint64_t[]){ ptrarray_count(expr->call_expr.args) });
            if(ip < 0)
            {
                goto error;
            }

            break;
        }
        case MC_EXPR_ASSIGN:
        {
            assign_expression_t* assign = &expr->assign;
            if(assign->dest->type != MC_EXPR_IDENT && assign->dest->type != MC_EXPR_INDEX)
            {
                errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, assign->dest->pos, "Expression is not assignable.");
                goto error;
            }

            if(assign->is_postfix)
            {
                ok = mc_compiler_compileexpression(comp, assign->dest);
                if(!ok)
                {
                    goto error;
                }
            }

            ok = mc_compiler_compileexpression(comp, assign->source);
            if(!ok)
            {
                goto error;
            }

            ip = mc_compiler_emit(comp, OPCODE_DUP, 0, NULL);
            if(ip < 0)
            {
                goto error;
            }

            ok = array_push(comp->src_positions_stack, &assign->dest->pos);
            if(!ok)
            {
                goto error;
            }

            if(assign->dest->type == MC_EXPR_IDENT)
            {
                ident_t* ident = assign->dest->ident;
                mcastsymbol_t* symbol = symbol_table_resolve(symbol_table, ident->value);
                if(!symbol)
                {
                    errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, assign->dest->pos, "Symbol \"%s\" could not be resolved", ident->value);
                    goto error;
                }
                if(!symbol->assignable)
                {
                    errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, assign->dest->pos, "Symbol \"%s\" is not assignable", ident->value);
                    goto error;
                }
                ok = mc_compiler_storesymbol(comp, symbol, false);
                if(!ok)
                {
                    goto error;
                }
            }
            else if(assign->dest->type == MC_EXPR_INDEX)
            {
                index_expression_t* index = &assign->dest->index_expr;
                ok = mc_compiler_compileexpression(comp, index->left);
                if(!ok)
                {
                    goto error;
                }
                ok = mc_compiler_compileexpression(comp, index->index);
                if(!ok)
                {
                    goto error;
                }
                ip = mc_compiler_emit(comp, OPCODE_SET_INDEX, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
            }

            if(assign->is_postfix)
            {
                ip = mc_compiler_emit(comp, OPCODE_POP, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
            }

            array_pop(comp->src_positions_stack, NULL);
            break;
        }
        case MC_EXPR_LOGICAL:
        {
            logical_expression_t* logi = &expr->logical;

            ok = mc_compiler_compileexpression(comp, logi->left);
            if(!ok)
            {
                goto error;
            }

            ip = mc_compiler_emit(comp, OPCODE_DUP, 0, NULL);
            if(ip < 0)
            {
                goto error;
            }

            int afterleftjumpip = 0;
            if(logi->op == OPERATOR_LOGICAL_AND)
            {
                afterleftjumpip = mc_compiler_emit(comp, OPCODE_JUMP_IF_FALSE, 1, (uint64_t[]){ 0xbeef });
            }
            else
            {
                afterleftjumpip = mc_compiler_emit(comp, OPCODE_JUMP_IF_TRUE, 1, (uint64_t[]){ 0xbeef });
            }

            if(afterleftjumpip < 0)
            {
                goto error;
            }

            ip = mc_compiler_emit(comp, OPCODE_POP, 0, NULL);
            if(ip < 0)
            {
                goto error;
            }

            ok = mc_compiler_compileexpression(comp, logi->right);
            if(!ok)
            {
                goto error;
            }

            int afterrightip = mc_compiler_getip(comp);
            mc_compiler_changeuint16operand(comp, afterleftjumpip + 1, afterrightip);

            break;
        }
        case MC_EXPR_TERNARY:
        {
            ternary_expression_t* ternary = &expr->ternary;

            ok = mc_compiler_compileexpression(comp, ternary->test);
            if(!ok)
            {
                goto error;
            }

            int elsejumpip = mc_compiler_emit(comp, OPCODE_JUMP_IF_FALSE, 1, (uint64_t[]){ 0xbeef });

            ok = mc_compiler_compileexpression(comp, ternary->if_true);
            if(!ok)
            {
                goto error;
            }

            int endjumpip = mc_compiler_emit(comp, OPCODE_JUMP, 1, (uint64_t[]){ 0xbeef });

            int elseip = mc_compiler_getip(comp);
            mc_compiler_changeuint16operand(comp, elsejumpip + 1, elseip);

            ok = mc_compiler_compileexpression(comp, ternary->if_false);
            if(!ok)
            {
                goto error;
            }

            int endip = mc_compiler_getip(comp);
            mc_compiler_changeuint16operand(comp, endjumpip + 1, endip);

            break;
        }
        default:
        {
            ARCANE_ASSERT(false);
            break;
        }
    }
    res = true;
    goto end;
error:
    res = false;
end:
    array_pop(comp->src_positions_stack, NULL);
    expression_destroy(exproptimised);
    return res;
}

TMPSEMISTATIC bool mc_compiler_compilecodeblock(mcastcompiler_t* comp, code_block_t* block)
{
    bool ok;
    mcastsymtable_t* symbol_table = mc_compiler_getsymtable(comp);
    if(!symbol_table)
    {
        return false;
    }

    ok = symbol_table_push_block_scope(symbol_table);
    if(!ok)
    {
        return false;
    }

    if(ptrarray_count(block->statements) == 0)
    {
        int ip = mc_compiler_emit(comp, OPCODE_NULL, 0, NULL);
        if(ip < 0)
        {
            return false;
        }
        ip = mc_compiler_emit(comp, OPCODE_POP, 0, NULL);
        if(ip < 0)
        {
            return false;
        }
    }

    for(int i = 0; i < ptrarray_count(block->statements); i++)
    {
        mcastexpression_t* stmt = ptrarray_get(block->statements, i);
        ok = mc_compiler_compilestatement(comp, stmt);
        if(!ok)
        {
            return false;
        }
    }
    symbol_table_pop_block_scope(symbol_table);
    return true;
}

TMPSTATIC int mc_compiler_addconstant(mcastcompiler_t* comp, mcvalue_t obj)
{
    bool ok;
    ok = array_add(comp->constants, &obj);
    if(!ok)
    {
        return -1;
    }
    int pos = array_count(comp->constants) - 1;
    return pos;
}

TMPSTATIC void mc_compiler_changeuint16operand(mcastcompiler_t* comp, int ip, uint16_t operand)
{
    mcbasicarray_t* bytecode = mc_compiler_getbytecode(comp);
    if((ip + 1) >= array_count(bytecode))
    {
        ARCANE_ASSERT(false);
        return;
    }
    uint8_t hi = operand >> 8;
    array_set(bytecode, ip, &hi);
    uint8_t lo = operand;
    array_set(bytecode, ip + 1, &lo);
}

TMPSTATIC bool mc_compiler_lastopcodeis(mcastcompiler_t* comp, opcode_t op)
{
    opcode_t last_opcode = mc_compiler_getlastopcode(comp);
    return last_opcode == op;
}

TMPSTATIC bool mc_compiler_readsymbol(mcastcompiler_t* comp, mcastsymbol_t* symbol)
{
    int ip = -1;
    if(symbol->type == SYMBOL_MODULE_GLOBAL)
    {
        ip = mc_compiler_emit(comp, OPCODE_GET_MODULE_GLOBAL, 1, (uint64_t[]){ symbol->index });
    }
    else if(symbol->type == SYMBOL_ARCANE_GLOBAL)
    {
        ip = mc_compiler_emit(comp, OPCODE_GET_ARCANE_GLOBAL, 1, (uint64_t[]){ symbol->index });
    }
    else if(symbol->type == SYMBOL_LOCAL)
    {
        ip = mc_compiler_emit(comp, OPCODE_GET_LOCAL, 1, (uint64_t[]){ symbol->index });
    }
    else if(symbol->type == SYMBOL_FREE)
    {
        ip = mc_compiler_emit(comp, OPCODE_GET_FREE, 1, (uint64_t[]){ symbol->index });
    }
    else if(symbol->type == SYMBOL_FUNCTION)
    {
        ip = mc_compiler_emit(comp, OPCODE_CURRENT_FUNCTION, 0, NULL);
    }
    else if(symbol->type == SYMBOL_THIS)
    {
        ip = mc_compiler_emit(comp, OPCODE_GET_THIS, 0, NULL);
    }
    return ip >= 0;
}

TMPSTATIC bool mc_compiler_storesymbol(mcastcompiler_t* comp, mcastsymbol_t* symbol, bool define)
{
    int ip = -1;
    if(symbol->type == SYMBOL_MODULE_GLOBAL)
    {
        if(define)
        {
            ip = mc_compiler_emit(comp, OPCODE_DEFINE_MODULE_GLOBAL, 1, (uint64_t[]){ symbol->index });
        }
        else
        {
            ip = mc_compiler_emit(comp, OPCODE_SET_MODULE_GLOBAL, 1, (uint64_t[]){ symbol->index });
        }
    }
    else if(symbol->type == SYMBOL_LOCAL)
    {
        if(define)
        {
            ip = mc_compiler_emit(comp, OPCODE_DEFINE_LOCAL, 1, (uint64_t[]){ symbol->index });
        }
        else
        {
            ip = mc_compiler_emit(comp, OPCODE_SET_LOCAL, 1, (uint64_t[]){ symbol->index });
        }
    }
    else if(symbol->type == SYMBOL_FREE)
    {
        ip = mc_compiler_emit(comp, OPCODE_SET_FREE, 1, (uint64_t[]){ symbol->index });
    }
    return ip >= 0;
}

TMPSTATIC bool mc_compiler_pushbreakip(mcastcompiler_t* comp, int ip)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    return array_push(compscope->break_ip_stack, &ip);
}

TMPSTATIC void mc_compiler_popbreakip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    if(array_count(compscope->break_ip_stack) == 0)
    {
        ARCANE_ASSERT(false);
        return;
    }
    array_pop(compscope->break_ip_stack, NULL);
}

TMPSTATIC int mc_compiler_getbreakip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    if(array_count(compscope->break_ip_stack) == 0)
    {
        return -1;
    }
    int* res = array_top(compscope->break_ip_stack);
    return *res;
}

TMPSTATIC bool mc_compiler_pushcontinueip(mcastcompiler_t* comp, int ip)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    return array_push(compscope->continue_ip_stack, &ip);
}

TMPSTATIC void mc_compiler_popcontinueip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    if(array_count(compscope->continue_ip_stack) == 0)
    {
        ARCANE_ASSERT(false);
        return;
    }
    array_pop(compscope->continue_ip_stack, NULL);
}

TMPSTATIC int mc_compiler_getcontinueip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    if(array_count(compscope->continue_ip_stack) == 0)
    {
        ARCANE_ASSERT(false);
        return -1;
    }
    int* res = array_top(compscope->continue_ip_stack);
    return *res;
}

TMPSTATIC int mc_compiler_getip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compilation_scope = mc_compiler_getcompilationscope(comp);
    return array_count(compilation_scope->bytecode);
}

TMPSTATIC mcbasicarray_t* mc_compiler_getsrcpositions(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compilation_scope = mc_compiler_getcompilationscope(comp);
    return compilation_scope->src_positions;
}

TMPSTATIC mcbasicarray_t* mc_compiler_getbytecode(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compilation_scope = mc_compiler_getcompilationscope(comp);
    return compilation_scope->bytecode;
}

TMPSTATIC mcastscopefile_t* mc_compiler_filescopemake(mcastcompiler_t* comp, mcastcompiledfile_t* file)
{
    mcastscopefile_t* filescope = (mcastscopefile_t*)allocator_malloc(comp->pstate, sizeof(mcastscopefile_t));
    if(!filescope)
    {
        return NULL;
    }
    memset(filescope, 0, sizeof(mcastscopefile_t));
    filescope->pstate = comp->pstate;
    filescope->parser = parser_make(comp->pstate, comp->config, comp->errors);
    if(!filescope->parser)
    {
        goto err;
    }
    filescope->symbol_table = NULL;
    filescope->file = file;
    filescope->loaded_module_names = ptrarray_make(comp->pstate);
    if(!filescope->loaded_module_names)
    {
        goto err;
    }
    return filescope;
err:
    mc_compiler_filescopedestroy(filescope);
    return NULL;
}

TMPSTATIC void mc_compiler_filescopedestroy(mcastscopefile_t* scope)
{
    for(int i = 0; i < ptrarray_count(scope->loaded_module_names); i++)
    {
        void* name = ptrarray_get(scope->loaded_module_names, i);
        allocator_free(scope->pstate, name);
    }
    ptrarray_destroy(scope->loaded_module_names);
    parser_destroy(scope->parser);
    allocator_free(scope->pstate, scope);
}

TMPSTATIC bool mc_compiler_filescopepush(mcastcompiler_t* comp, const char* filepath)
{
    bool ok;
    mcastsymtable_t* prevst = NULL;
    if(ptrarray_count(comp->file_scopes) > 0)
    {
        prevst = mc_compiler_getsymtable(comp);
    }

    mcastcompiledfile_t* file = mc_compiledfile_make(comp->pstate, filepath);
    if(!file)
    {
        return false;
    }

    ok = ptrarray_push(comp->files, file);
    if(!ok)
    {
        mc_compiledfile_destroy(file);
        return false;
    }

    mcastscopefile_t* filescope = mc_compiler_filescopemake(comp, file);
    if(!filescope)
    {
        return false;
    }

    ok = ptrarray_push(comp->file_scopes, filescope);
    if(!ok)
    {
        mc_compiler_filescopedestroy(filescope);
        return false;
    }

    int globaloffset = 0;
    if(prevst)
    {
        mcastscopeblock_t* prevsttopscope = symbol_table_get_block_scope(prevst);
        globaloffset = prevsttopscope->offset + prevsttopscope->num_definitions;
    }

    ok = mc_compiler_pushsymtable(comp, globaloffset);
    if(!ok)
    {
        ptrarray_pop(comp->file_scopes);
        mc_compiler_filescopedestroy(filescope);
        return false;
    }

    return true;
}

TMPSTATIC void mc_compiler_filescopepop(mcastcompiler_t* comp)
{
    mcastsymtable_t* poppedst = mc_compiler_getsymtable(comp);
    mcastscopeblock_t* poppedsttopscope = symbol_table_get_block_scope(poppedst);
    int poppednumdefs = poppedsttopscope->num_definitions;

    while(mc_compiler_getsymtable(comp))
    {
        mc_compiler_popsymtable(comp);
    }
    mcastscopefile_t* scope = ptrarray_top(comp->file_scopes);
    if(!scope)
    {
        ARCANE_ASSERT(false);
        return;
    }
    mc_compiler_filescopedestroy(scope);

    ptrarray_pop(comp->file_scopes);

    if(ptrarray_count(comp->file_scopes) > 0)
    {
        mcastsymtable_t* currentst = mc_compiler_getsymtable(comp);
        mcastscopeblock_t* currentsttopscope = symbol_table_get_block_scope(currentst);
        currentsttopscope->num_definitions += poppednumdefs;
    }
}

TMPSTATIC void mc_compiler_setcompilationscope(mcastcompiler_t* comp, mcastscopecomp_t* scope)
{
    comp->compilation_scope = scope;
}

TMPSTATIC module_t* mc_module_make(mcstate_t* state, const char* name)
{
    module_t* module = (module_t*)allocator_malloc(state, sizeof(module_t));
    if(!module)
    {
        return NULL;
    }
    memset(module, 0, sizeof(module_t));
    module->pstate = state;
    module->name = arcane_strdup(state, name);
    if(!module->name)
    {
        mc_module_destroy(module);
        return NULL;
    }
    module->symbols = ptrarray_make(state);
    if(!module->symbols)
    {
        mc_module_destroy(module);
        return NULL;
    }
    return module;
}

TMPSTATIC void mc_module_destroy(module_t* module)
{
    if(!module)
    {
        return;
    }
    allocator_free(module->pstate, module->name);
    ptrarray_destroy_with_items_(module->symbols, (ptrarrayitemdestroyfn)symbol_destroy);
    allocator_free(module->pstate, module);
}

TMPSTATIC module_t* mc_module_copy(module_t* src)
{
    module_t* copy = (module_t*)allocator_malloc(src->pstate, sizeof(module_t));
    if(!copy)
    {
        return NULL;
    }
    memset(copy, 0, sizeof(module_t));
    copy->pstate = src->pstate;
    copy->name = arcane_strdup(copy->pstate, src->name);
    if(!copy->name)
    {
        mc_module_destroy(copy);
        return NULL;
    }
    copy->symbols = ptrarray_copy_with_items_(src->symbols, (ptrarrayitemcopyfn)symbol_copy, (ptrarrayitemdestroyfn)symbol_destroy);
    if(!copy->symbols)
    {
        mc_module_destroy(copy);
        return NULL;
    }
    return copy;
}

TMPSTATIC const char* mc_util_getmodulename(const char* path)
{
    const char* lastslashpos = strrchr(path, '/');
    if(lastslashpos)
    {
        return lastslashpos + 1;
    }
    return path;
}

TMPSTATIC bool mc_module_addsymbol(module_t* module, mcastsymbol_t* symbol)
{
    bool ok;
    mcprintstate_t* namebuf = mc_printer_make(module->pstate, NULL);
    if(!namebuf)
    {
        return false;
    }
    ok = mc_printer_appendf(namebuf, "%s::%s", module->name, symbol->name);
    if(!ok)
    {
        mc_printer_destroy(namebuf);
        return false;
    }
    mcastsymbol_t* modulesymbol = symbol_make(module->pstate, mc_printer_getstring(namebuf), SYMBOL_MODULE_GLOBAL, symbol->index, false);
    mc_printer_destroy(namebuf);
    if(!modulesymbol)
    {
        return false;
    }
    ok = ptrarray_push(module->symbols, modulesymbol);
    if(!ok)
    {
        symbol_destroy(modulesymbol);
        return false;
    }
    return true;
}

TMPSTATIC mcastsymbol_t* mc_compiler_defsymbol(mcastcompiler_t* comp, mcastlocation_t pos, const char* name, bool assignable, bool canshadow)
{
    mcastsymtable_t* symbol_table = mc_compiler_getsymtable(comp);
    if(!canshadow && !symbol_table_is_top_global_scope(symbol_table))
    {
        mcastsymbol_t* currentsymbol = symbol_table_resolve(symbol_table, name);
        if(currentsymbol)
        {
            errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, pos, "Symbol \"%s\" is already defined", name);
            return NULL;
        }
    }

    mcastsymbol_t* symbol = symbol_table_define(symbol_table, name, assignable);
    if(!symbol)
    {
        errors_add_errorf(comp->errors, ARCANE_ERROR_COMPILATION, pos, "Cannot define symbol \"%s\"", name);
        return false;
    }

    return symbol;
}

TMPSTATIC mcastparser_t* parser_make(mcstate_t* state, mcconfig_t* config, mcerrlist_t* errors)
{
    mcastparser_t* parser = (mcastparser_t*)allocator_malloc(state, sizeof(mcastparser_t));
    if(!parser)
    {
        return NULL;
    }
    memset(parser, 0, sizeof(mcastparser_t));

    parser->pstate = state;
    parser->config = config;
    parser->errors = errors;

    parser->right_assoc_parse_fns[TOKEN_IDENT] = parse_identifier;
    parser->right_assoc_parse_fns[TOKEN_NUMBER] = parse_number_literal;
    parser->right_assoc_parse_fns[TOKEN_TRUE] = parse_bool_literal;
    parser->right_assoc_parse_fns[TOKEN_FALSE] = parse_bool_literal;
    parser->right_assoc_parse_fns[TOKEN_STRING] = parse_string_literal;
    parser->right_assoc_parse_fns[TOKEN_TEMPLATE_STRING] = parse_template_string_literal;
    parser->right_assoc_parse_fns[TOKEN_NULL] = parse_null_literal;
    parser->right_assoc_parse_fns[TOKEN_BANG] = parse_prefix_expression;
    parser->right_assoc_parse_fns[TOKEN_MINUS] = parse_prefix_expression;
    parser->right_assoc_parse_fns[TOKEN_LPAREN] = parse_grouped_expression;
    parser->right_assoc_parse_fns[TOKEN_FUNCTION] = parse_function_literal;
    parser->right_assoc_parse_fns[TOKEN_LBRACKET] = parse_array_literal;
    parser->right_assoc_parse_fns[TOKEN_LBRACE] = parse_map_literal;
    parser->right_assoc_parse_fns[TOKEN_PLUS_PLUS] = parse_incdec_prefix_expression;
    parser->right_assoc_parse_fns[TOKEN_MINUS_MINUS] = parse_incdec_prefix_expression;

    parser->left_assoc_parse_fns[TOKEN_PLUS] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_MINUS] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_SLASH] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_ASTERISK] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_PERCENT] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_EQ] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_NOT_EQ] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_LT] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_LTE] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_GT] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_GTE] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_LPAREN] = parse_call_expression;
    parser->left_assoc_parse_fns[TOKEN_LBRACKET] = parse_index_expression;
    parser->left_assoc_parse_fns[TOKEN_ASSIGN] = parse_assign_expression;
    parser->left_assoc_parse_fns[TOKEN_PLUS_ASSIGN] = parse_assign_expression;
    parser->left_assoc_parse_fns[TOKEN_MINUS_ASSIGN] = parse_assign_expression;
    parser->left_assoc_parse_fns[TOKEN_SLASH_ASSIGN] = parse_assign_expression;
    parser->left_assoc_parse_fns[TOKEN_ASTERISK_ASSIGN] = parse_assign_expression;
    parser->left_assoc_parse_fns[TOKEN_PERCENT_ASSIGN] = parse_assign_expression;
    parser->left_assoc_parse_fns[TOKEN_BIT_AND_ASSIGN] = parse_assign_expression;
    parser->left_assoc_parse_fns[TOKEN_BIT_OR_ASSIGN] = parse_assign_expression;
    parser->left_assoc_parse_fns[TOKEN_BIT_XOR_ASSIGN] = parse_assign_expression;
    parser->left_assoc_parse_fns[TOKEN_LSHIFT_ASSIGN] = parse_assign_expression;
    parser->left_assoc_parse_fns[TOKEN_RSHIFT_ASSIGN] = parse_assign_expression;
    parser->left_assoc_parse_fns[TOKEN_DOT] = parse_dot_expression;
    parser->left_assoc_parse_fns[TOKEN_AND] = parse_logical_expression;
    parser->left_assoc_parse_fns[TOKEN_OR] = parse_logical_expression;
    parser->left_assoc_parse_fns[TOKEN_BIT_AND] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_BIT_OR] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_BIT_XOR] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_LSHIFT] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_RSHIFT] = parse_infix_expression;
    parser->left_assoc_parse_fns[TOKEN_QUESTION] = parse_ternary_expression;
    parser->left_assoc_parse_fns[TOKEN_PLUS_PLUS] = parse_incdec_postfix_expression;
    parser->left_assoc_parse_fns[TOKEN_MINUS_MINUS] = parse_incdec_postfix_expression;

    parser->depth = 0;

    return parser;
}

TMPSTATIC void parser_destroy(mcastparser_t* parser)
{
    if(!parser)
    {
        return;
    }
    allocator_free(parser->pstate, parser);
}

TMPSTATIC mcptrarray_t* parser_parse_all(mcastparser_t* parser, const char* input, mcastcompiledfile_t* file)
{
    bool ok;
    parser->depth = 0;

    ok = mc_lexer_init(&parser->lexer, parser->pstate, parser->errors, input, file);
    if(!ok)
    {
        return NULL;
    }

    mc_lexer_nexttoken(&parser->lexer);
    mc_lexer_nexttoken(&parser->lexer);

    mcptrarray_t* statements = ptrarray_make(parser->pstate);
    if(!statements)
    {
        return NULL;
    }

    while(!mc_lexer_currtokenis(&parser->lexer, TOKEN_EOF))
    {
        if(mc_lexer_currtokenis(&parser->lexer, TOKEN_SEMICOLON))
        {
            mc_lexer_nexttoken(&parser->lexer);
            continue;
        }
        mcastexpression_t* stmt = parse_statement(parser);
        if(!stmt)
        {
            goto err;
        }
        ok = ptrarray_push(statements, stmt);
        if(!ok)
        {
            statement_destroy(stmt);
            goto err;
        }
    }

    if(errors_get_count(parser->errors) > 0)
    {
        goto err;
    }

    return statements;
err:
    ptrarray_destroy_with_items_(statements, (ptrarrayitemdestroyfn)statement_destroy);
    return NULL;
}

TMPSTATIC mcastexpression_t* parse_statement(mcastparser_t* p)
{
    mcastlocation_t pos = p->lexer.cur_token.pos;

    mcastexpression_t* res = NULL;
    switch(p->lexer.cur_token.type)
    {
        case TOKEN_VAR:
        case TOKEN_CONST:
        {
            res = parse_define_statement(p);
            break;
        }
        case TOKEN_IF:
        {
            res = parse_if_statement(p);
            break;
        }
        case TOKEN_RETURN:
        {
            res = parse_return_statement(p);
            break;
        }
        case TOKEN_WHILE:
        {
            res = parse_while_loop_statement(p);
            break;
        }
        case TOKEN_BREAK:
        {
            res = parse_break_statement(p);
            break;
        }
        case TOKEN_FOR:
        {
            res = parse_for_loop_statement(p);
            break;
        }
        case TOKEN_FUNCTION:
        {
            if(mc_lexer_peektokenis(&p->lexer, TOKEN_IDENT))
            {
                res = parse_function_statement(p);
            }
            else
            {
                res = parse_expression_statement(p);
            }
            break;
        }
        case TOKEN_LBRACE:
        {
            if(p->config->repl_mode && p->depth == 0)
            {
                res = parse_expression_statement(p);
            }
            else
            {
                res = parse_block_statement(p);
            }
            break;
        }
        case TOKEN_CONTINUE:
        {
            res = parse_continue_statement(p);
            break;
        }
        case TOKEN_IMPORT:
        {
            res = parse_import_statement(p);
            break;
        }
        case TOKEN_RECOVER:
        {
            res = parse_recover_statement(p);
            break;
        }
        default:
        {
            res = parse_expression_statement(p);
            break;
        }
    }
    if(res)
    {
        res->pos = pos;
    }
    return res;
}

TMPSEMISTATIC mcastexpression_t* parse_define_statement(mcastparser_t* p)
{
    ident_t* nameident = NULL;
    mcastexpression_t* value = NULL;

    bool assignable = mc_lexer_currtokenis(&p->lexer, TOKEN_VAR);

    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_IDENT))
    {
        goto err;
    }

    nameident = ident_make(p->pstate, p->lexer.cur_token);
    if(!nameident)
    {
        goto err;
    }

    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_ASSIGN))
    {
        goto err;
    }

    mc_lexer_nexttoken(&p->lexer);

    value = parse_expression(p, PRECEDENCE_LOWEST);
    if(!value)
    {
        goto err;
    }

    if(value->type == MC_EXPR_FUNCTIONLITERAL)
    {
        value->fn_literal.name = arcane_strdup(p->pstate, nameident->value);
        if(!value->fn_literal.name)
        {
            goto err;
        }
    }

    mcastexpression_t* res = statement_make_define(p->pstate, nameident, value, assignable);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    expression_destroy(value);
    ident_destroy(nameident);
    return NULL;
}

TMPSEMISTATIC mcastexpression_t* parse_if_statement(mcastparser_t* p)
{
    bool ok;
    mcptrarray_t* cases = NULL;
    code_block_t* alternative = NULL;

    cases = ptrarray_make(p->pstate);
    if(!cases)
    {
        goto err;
    }

    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_LPAREN))
    {
        goto err;
    }

    mc_lexer_nexttoken(&p->lexer);

    if_case_t* cond = if_case_make(p->pstate, NULL, NULL);
    if(!cond)
    {
        goto err;
    }

    ok = ptrarray_push(cases, cond);
    if(!ok)
    {
        if_case_destroy(cond);
        goto err;
    }

    cond->test = parse_expression(p, PRECEDENCE_LOWEST);
    if(!cond->test)
    {
        goto err;
    }

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_RPAREN))
    {
        goto err;
    }

    mc_lexer_nexttoken(&p->lexer);

    cond->consequence = parse_code_block(p);
    if(!cond->consequence)
    {
        goto err;
    }

    while(mc_lexer_currtokenis(&p->lexer, TOKEN_ELSE))
    {
        mc_lexer_nexttoken(&p->lexer);

        if(mc_lexer_currtokenis(&p->lexer, TOKEN_IF))
        {
            mc_lexer_nexttoken(&p->lexer);

            if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_LPAREN))
            {
                goto err;
            }

            mc_lexer_nexttoken(&p->lexer);

            if_case_t* elif = if_case_make(p->pstate, NULL, NULL);
            if(!elif)
            {
                goto err;
            }

            ok = ptrarray_push(cases, elif);
            if(!ok)
            {
                if_case_destroy(elif);
                goto err;
            }

            elif->test = parse_expression(p, PRECEDENCE_LOWEST);
            if(!elif->test)
            {
                goto err;
            }

            if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_RPAREN))
            {
                goto err;
            }

            mc_lexer_nexttoken(&p->lexer);

            elif->consequence = parse_code_block(p);
            if(!elif->consequence)
            {
                goto err;
            }
        }
        else
        {
            alternative = parse_code_block(p);
            if(!alternative)
            {
                goto err;
            }
        }
    }

    mcastexpression_t* res = statement_make_if(p->pstate, cases, alternative);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    ptrarray_destroy_with_items_(cases, (ptrarrayitemdestroyfn)if_case_destroy);
    code_block_destroy(alternative);
    return NULL;
}

TMPSEMISTATIC mcastexpression_t* parse_return_statement(mcastparser_t* p)
{
    mcastexpression_t* expr = NULL;

    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_currtokenis(&p->lexer, TOKEN_SEMICOLON) && !mc_lexer_currtokenis(&p->lexer, TOKEN_RBRACE) && !mc_lexer_currtokenis(&p->lexer, TOKEN_EOF))
    {
        expr = parse_expression(p, PRECEDENCE_LOWEST);
        if(!expr)
        {
            return NULL;
        }
    }

    mcastexpression_t* res = statement_make_return(p->pstate, expr);
    if(!res)
    {
        expression_destroy(expr);
        return NULL;
    }
    return res;
}

TMPSEMISTATIC mcastexpression_t* parse_expression_statement(mcastparser_t* p)
{
    mcastexpression_t* expr = parse_expression(p, PRECEDENCE_LOWEST);
    if(!expr)
    {
        return NULL;
    }

    if(expr && (!p->config->repl_mode || p->depth > 0))
    {
        if(expr->type != MC_EXPR_ASSIGN && expr->type != MC_EXPR_CALL)
        {
            errors_add_errorf(p->errors, ARCANE_ERROR_PARSING, expr->pos, "Only assignments and function calls can be expression statements");
            expression_destroy(expr);
            return NULL;
        }
    }

    mcastexpression_t* res = statement_make_expression(p->pstate, expr);
    if(!res)
    {
        expression_destroy(expr);
        return NULL;
    }
    return res;
}

TMPSEMISTATIC mcastexpression_t* parse_while_loop_statement(mcastparser_t* p)
{
    mcastexpression_t* test = NULL;
    code_block_t* body = NULL;

    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_LPAREN))
    {
        goto err;
    }

    mc_lexer_nexttoken(&p->lexer);

    test = parse_expression(p, PRECEDENCE_LOWEST);
    if(!test)
    {
        goto err;
    }

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_RPAREN))
    {
        goto err;
    }

    mc_lexer_nexttoken(&p->lexer);

    body = parse_code_block(p);
    if(!body)
    {
        goto err;
    }

    mcastexpression_t* res = statement_make_while_loop(p->pstate, test, body);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    code_block_destroy(body);
    expression_destroy(test);
    return NULL;
}

TMPSEMISTATIC mcastexpression_t* parse_break_statement(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    return statement_make_break(p->pstate);
}

TMPSEMISTATIC mcastexpression_t* parse_continue_statement(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    return statement_make_continue(p->pstate);
}

TMPSEMISTATIC mcastexpression_t* parse_block_statement(mcastparser_t* p)
{
    code_block_t* block = parse_code_block(p);
    if(!block)
    {
        return NULL;
    }
    mcastexpression_t* res = statement_make_block(p->pstate, block);
    if(!res)
    {
        code_block_destroy(block);
        return NULL;
    }
    return res;
}

TMPSEMISTATIC mcastexpression_t* parse_import_statement(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_STRING))
    {
        return NULL;
    }

    char* processedname = process_and_copy_string(p->pstate, p->lexer.cur_token.literal, p->lexer.cur_token.len);
    if(!processedname)
    {
        errors_add_error(p->errors, ARCANE_ERROR_PARSING, p->lexer.cur_token.pos, "Error when parsing module name");
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);

    mcastexpression_t* res = statement_make_import(p->pstate, processedname);
    if(!res)
    {
        allocator_free(p->pstate, processedname);
        return NULL;
    }
    return res;
}

TMPSEMISTATIC mcastexpression_t* parse_recover_statement(mcastparser_t* p)
{
    ident_t* error_ident = NULL;
    code_block_t* body = NULL;

    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_LPAREN))
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_IDENT))
    {
        return NULL;
    }

    error_ident = ident_make(p->pstate, p->lexer.cur_token);
    if(!error_ident)
    {
        return NULL;
    }

    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_RPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);

    body = parse_code_block(p);
    if(!body)
    {
        goto err;
    }

    mcastexpression_t* res = statement_make_recover(p->pstate, error_ident, body);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    code_block_destroy(body);
    ident_destroy(error_ident);
    return NULL;
}

TMPSEMISTATIC mcastexpression_t* parse_for_loop_statement(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_LPAREN))
    {
        return NULL;
    }

    mc_lexer_nexttoken(&p->lexer);

    if(mc_lexer_currtokenis(&p->lexer, TOKEN_IDENT) && mc_lexer_peektokenis(&p->lexer, TOKEN_IN))
    {
        return parse_foreach(p);
    }
    return parse_classic_for_loop(p);
}

TMPSTATIC mcastexpression_t* parse_foreach(mcastparser_t* p)
{
    mcastexpression_t* source = NULL;
    code_block_t* body = NULL;
    ident_t* iteratorident = NULL;

    iteratorident = ident_make(p->pstate, p->lexer.cur_token);
    if(!iteratorident)
    {
        goto err;
    }

    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_IN))
    {
        goto err;
    }

    mc_lexer_nexttoken(&p->lexer);

    source = parse_expression(p, PRECEDENCE_LOWEST);
    if(!source)
    {
        goto err;
    }

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_RPAREN))
    {
        goto err;
    }

    mc_lexer_nexttoken(&p->lexer);

    body = parse_code_block(p);
    if(!body)
    {
        goto err;
    }

    mcastexpression_t* res = statement_make_foreach(p->pstate, iteratorident, source, body);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    code_block_destroy(body);
    ident_destroy(iteratorident);
    expression_destroy(source);
    return NULL;
}

TMPSTATIC mcastexpression_t* parse_classic_for_loop(mcastparser_t* p)
{
    mcastexpression_t* init = NULL;
    mcastexpression_t* test = NULL;
    mcastexpression_t* update = NULL;
    code_block_t* body = NULL;

    if(!mc_lexer_currtokenis(&p->lexer, TOKEN_SEMICOLON))
    {
        init = parse_statement(p);
        if(!init)
        {
            goto err;
        }
        if(init->type != MC_EXPR_STMTDEFINE && init->type != MC_EXPR_STMTEXPRESSION)
        {
            errors_add_errorf(p->errors, ARCANE_ERROR_PARSING, init->pos, "for loop's init clause should be a define statement or an expression");
            goto err;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_SEMICOLON))
        {
            goto err;
        }
    }

    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_currtokenis(&p->lexer, TOKEN_SEMICOLON))
    {
        test = parse_expression(p, PRECEDENCE_LOWEST);
        if(!test)
        {
            goto err;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_SEMICOLON))
        {
            goto err;
        }
    }

    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_currtokenis(&p->lexer, TOKEN_RPAREN))
    {
        update = parse_expression(p, PRECEDENCE_LOWEST);
        if(!update)
        {
            goto err;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_RPAREN))
        {
            goto err;
        }
    }

    mc_lexer_nexttoken(&p->lexer);

    body = parse_code_block(p);
    if(!body)
    {
        goto err;
    }

    mcastexpression_t* res = statement_make_for_loop(p->pstate, init, test, update, body);
    if(!res)
    {
        goto err;
    }

    return res;
err:
    statement_destroy(init);
    expression_destroy(test);
    expression_destroy(update);
    code_block_destroy(body);
    return NULL;
}

TMPSEMISTATIC mcastexpression_t* parse_function_statement(mcastparser_t* p)
{
    ident_t* nameident = NULL;
    mcastexpression_t* value = NULL;

    mcastlocation_t pos = p->lexer.cur_token.pos;

    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_IDENT))
    {
        goto err;
    }

    nameident = ident_make(p->pstate, p->lexer.cur_token);
    if(!nameident)
    {
        goto err;
    }

    mc_lexer_nexttoken(&p->lexer);

    value = parse_function_literal(p);
    if(!value)
    {
        goto err;
    }

    value->pos = pos;
    value->fn_literal.name = arcane_strdup(p->pstate, nameident->value);

    if(!value->fn_literal.name)
    {
        goto err;
    }

    mcastexpression_t* res = statement_make_define(p->pstate, nameident, value, false);
    if(!res)
    {
        goto err;
    }
    return res;

err:
    expression_destroy(value);
    ident_destroy(nameident);
    return NULL;
}

TMPSTATIC code_block_t* parse_code_block(mcastparser_t* p)
{
    bool ok;
    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_LBRACE))
    {
        return NULL;
    }

    mc_lexer_nexttoken(&p->lexer);
    p->depth++;

    mcptrarray_t* statements = ptrarray_make(p->pstate);
    if(!statements)
    {
        goto err;
    }

    while(!mc_lexer_currtokenis(&p->lexer, TOKEN_RBRACE))
    {
        if(mc_lexer_currtokenis(&p->lexer, TOKEN_EOF))
        {
            errors_add_error(p->errors, ARCANE_ERROR_PARSING, p->lexer.cur_token.pos, "Unexpected EOF");
            goto err;
        }
        if(mc_lexer_currtokenis(&p->lexer, TOKEN_SEMICOLON))
        {
            mc_lexer_nexttoken(&p->lexer);
            continue;
        }
        mcastexpression_t* stmt = parse_statement(p);
        if(!stmt)
        {
            goto err;
        }
        ok = ptrarray_push(statements, stmt);
        if(!ok)
        {
            statement_destroy(stmt);
            goto err;
        }
    }

    mc_lexer_nexttoken(&p->lexer);

    p->depth--;

    code_block_t* res = code_block_make(p->pstate, statements);
    if(!res)
    {
        goto err;
    }
    return res;

err:
    p->depth--;
    ptrarray_destroy_with_items_(statements, (ptrarrayitemdestroyfn)statement_destroy);
    return NULL;
}

TMPSTATIC mcastexpression_t* parse_expression(mcastparser_t* p, mcastprecedence_t prec)
{
    mcastlocation_t pos = p->lexer.cur_token.pos;

    if(p->lexer.cur_token.type == TOKEN_INVALID)
    {
        errors_add_error(p->errors, ARCANE_ERROR_PARSING, p->lexer.cur_token.pos, "Illegal token");
        return NULL;
    }

    rightassocparsefn parse_right_assoc = p->right_assoc_parse_fns[p->lexer.cur_token.type];
    if(!parse_right_assoc)
    {
        char* literal = token_duplicate_literal(p->pstate, &p->lexer.cur_token);
        errors_add_errorf(p->errors, ARCANE_ERROR_PARSING, p->lexer.cur_token.pos, "No prefix parse function for \"%s\" found", literal);
        allocator_free(p->pstate, literal);
        return NULL;
    }

    mcastexpression_t* leftexpr = parse_right_assoc(p);
    if(!leftexpr)
    {
        return NULL;
    }
    leftexpr->pos = pos;

    while(!mc_lexer_currtokenis(&p->lexer, TOKEN_SEMICOLON) && prec < get_precedence(p->lexer.cur_token.type))
    {
        leftassocparsefn parse_left_assoc = p->left_assoc_parse_fns[p->lexer.cur_token.type];
        if(!parse_left_assoc)
        {
            return leftexpr;
        }
        pos = p->lexer.cur_token.pos;
        mcastexpression_t* newleftexpr = parse_left_assoc(p, leftexpr);
        if(!newleftexpr)
        {
            expression_destroy(leftexpr);
            return NULL;
        }
        newleftexpr->pos = pos;
        leftexpr = newleftexpr;
    }

    return leftexpr;
}

TMPSTATIC mcastexpression_t* parse_identifier(mcastparser_t* p)
{
    ident_t* ident = ident_make(p->pstate, p->lexer.cur_token);
    if(!ident)
    {
        return NULL;
    }
    mcastexpression_t* res = expression_make_ident(p->pstate, ident);
    if(!res)
    {
        ident_destroy(ident);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    return res;
}

TMPSTATIC mcastexpression_t* parse_number_literal(mcastparser_t* p)
{
    char* end;
    double number = 0;
    errno = 0;
    number = strtod(p->lexer.cur_token.literal, &end);
    long parsedlen = end - p->lexer.cur_token.literal;
    if(errno || parsedlen != p->lexer.cur_token.len)
    {
        char* literal = token_duplicate_literal(p->pstate, &p->lexer.cur_token);
        errors_add_errorf(p->errors, ARCANE_ERROR_PARSING, p->lexer.cur_token.pos, "Parsing number literal \"%s\" failed", literal);
        allocator_free(p->pstate, literal);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    return expression_make_number_literal(p->pstate, number);
}

TMPSTATIC mcastexpression_t* parse_bool_literal(mcastparser_t* p)
{
    mcastexpression_t* res = expression_make_bool_literal(p->pstate, p->lexer.cur_token.type == TOKEN_TRUE);
    mc_lexer_nexttoken(&p->lexer);
    return res;
}

TMPSTATIC mcastexpression_t* parse_string_literal(mcastparser_t* p)
{
    char* processedliteral = process_and_copy_string(p->pstate, p->lexer.cur_token.literal, p->lexer.cur_token.len);
    if(!processedliteral)
    {
        errors_add_error(p->errors, ARCANE_ERROR_PARSING, p->lexer.cur_token.pos, "Error when parsing string literal");
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    mcastexpression_t* res = expression_make_string_literal(p->pstate, processedliteral);
    if(!res)
    {
        allocator_free(p->pstate, processedliteral);
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* parse_template_string_literal(mcastparser_t* p)
{
    char* processedliteral = NULL;
    mcastexpression_t* leftstringexpr = NULL;
    mcastexpression_t* templateexpr = NULL;
    mcastexpression_t* tostrcallexpr = NULL;
    mcastexpression_t* leftaddexpr = NULL;
    mcastexpression_t* rightexpr = NULL;
    mcastexpression_t* rightaddexpr = NULL;

    processedliteral = process_and_copy_string(p->pstate, p->lexer.cur_token.literal, p->lexer.cur_token.len);
    if(!processedliteral)
    {
        errors_add_error(p->errors, ARCANE_ERROR_PARSING, p->lexer.cur_token.pos, "Error when parsing string literal");
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_LBRACE))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);

    mcastlocation_t pos = p->lexer.cur_token.pos;

    leftstringexpr = expression_make_string_literal(p->pstate, processedliteral);
    if(!leftstringexpr)
    {
        goto err;
    }
    leftstringexpr->pos = pos;
    processedliteral = NULL;

    pos = p->lexer.cur_token.pos;
    templateexpr = parse_expression(p, PRECEDENCE_LOWEST);
    if(!templateexpr)
    {
        goto err;
    }

    tostrcallexpr = wrap_expression_in_function_call(p->pstate, templateexpr, "tostring");
    if(!tostrcallexpr)
    {
        goto err;
    }
    tostrcallexpr->pos = pos;
    templateexpr = NULL;

    leftaddexpr = expression_make_infix(p->pstate, OPERATOR_PLUS, leftstringexpr, tostrcallexpr);
    if(!leftaddexpr)
    {
        goto err;
    }
    leftaddexpr->pos = pos;
    leftstringexpr = NULL;
    tostrcallexpr = NULL;

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_RBRACE))
    {
        goto err;
    }
    mc_lexer_previoustoken(&p->lexer);
    mc_lexer_conttplstring(&p->lexer);
    mc_lexer_nexttoken(&p->lexer);
    mc_lexer_nexttoken(&p->lexer);

    pos = p->lexer.cur_token.pos;

    rightexpr = parse_expression(p, PRECEDENCE_HIGHEST);
    if(!rightexpr)
    {
        goto err;
    }

    rightaddexpr = expression_make_infix(p->pstate, OPERATOR_PLUS, leftaddexpr, rightexpr);
    if(!rightaddexpr)
    {
        goto err;
    }
    rightaddexpr->pos = pos;
    leftaddexpr = NULL;
    rightexpr = NULL;

    return rightaddexpr;
err:
    expression_destroy(rightaddexpr);
    expression_destroy(rightexpr);
    expression_destroy(leftaddexpr);
    expression_destroy(tostrcallexpr);
    expression_destroy(templateexpr);
    expression_destroy(leftstringexpr);
    allocator_free(p->pstate, processedliteral);
    return NULL;
}

TMPSTATIC mcastexpression_t* parse_null_literal(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    return expression_make_null_literal(p->pstate);
}

TMPSTATIC mcastexpression_t* parse_array_literal(mcastparser_t* p)
{
    mcptrarray_t* array = parse_expression_list(p, TOKEN_LBRACKET, TOKEN_RBRACKET, true);
    if(!array)
    {
        return NULL;
    }
    mcastexpression_t* res = expression_make_array_literal(p->pstate, array);
    if(!res)
    {
        ptrarray_destroy_with_items_(array, (ptrarrayitemdestroyfn)expression_destroy);
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* parse_map_literal(mcastparser_t* p)
{
    bool ok;
    mcptrarray_t* keys = ptrarray_make(p->pstate);
    mcptrarray_t* values = ptrarray_make(p->pstate);

    if(!keys || !values)
    {
        goto err;
    }

    mc_lexer_nexttoken(&p->lexer);

    while(!mc_lexer_currtokenis(&p->lexer, TOKEN_RBRACE))
    {
        mcastexpression_t* key = NULL;
        if(mc_lexer_currtokenis(&p->lexer, TOKEN_IDENT))
        {
            char* str = token_duplicate_literal(p->pstate, &p->lexer.cur_token);
            key = expression_make_string_literal(p->pstate, str);
            if(!key)
            {
                allocator_free(p->pstate, str);
                goto err;
            }
            key->pos = p->lexer.cur_token.pos;
            mc_lexer_nexttoken(&p->lexer);
        }
        else
        {
            key = parse_expression(p, PRECEDENCE_LOWEST);
            if(!key)
            {
                goto err;
            }
            switch(key->type)
            {
                case MC_EXPR_STRINGLITERAL:
                case MC_EXPR_NUMBERLITERAL:
                case MC_EXPR_BOOLLITERAL:
                {
                    break;
                }
                default:
                {
                    errors_add_errorf(p->errors, ARCANE_ERROR_PARSING, key->pos, "Invalid map literal key type");
                    expression_destroy(key);
                    goto err;
                }
            }
        }

        ok = ptrarray_push(keys, key);
        if(!ok)
        {
            expression_destroy(key);
            goto err;
        }

        if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_COLON))
        {
            goto err;
        }

        mc_lexer_nexttoken(&p->lexer);

        mcastexpression_t* value = parse_expression(p, PRECEDENCE_LOWEST);
        if(!value)
        {
            goto err;
        }
        ok = ptrarray_push(values, value);
        if(!ok)
        {
            expression_destroy(value);
            goto err;
        }

        if(mc_lexer_currtokenis(&p->lexer, TOKEN_RBRACE))
        {
            break;
        }

        if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_COMMA))
        {
            goto err;
        }

        mc_lexer_nexttoken(&p->lexer);
    }

    mc_lexer_nexttoken(&p->lexer);

    mcastexpression_t* res = expression_make_map_literal(p->pstate, keys, values);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    ptrarray_destroy_with_items_(keys, (ptrarrayitemdestroyfn)expression_destroy);
    ptrarray_destroy_with_items_(values, (ptrarrayitemdestroyfn)expression_destroy);
    return NULL;
}

TMPSTATIC mcastexpression_t* parse_prefix_expression(mcastparser_t* p)
{
    mcastmathoptype_t op = token_to_operator(p->lexer.cur_token.type);
    mc_lexer_nexttoken(&p->lexer);
    mcastexpression_t* right = parse_expression(p, PRECEDENCE_PREFIX);
    if(!right)
    {
        return NULL;
    }
    mcastexpression_t* res = expression_make_prefix(p->pstate, op, right);
    if(!res)
    {
        expression_destroy(right);
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* parse_infix_expression(mcastparser_t* p, mcastexpression_t* left)
{
    mcastmathoptype_t op = token_to_operator(p->lexer.cur_token.type);
    mcastprecedence_t prec = get_precedence(p->lexer.cur_token.type);
    mc_lexer_nexttoken(&p->lexer);
    mcastexpression_t* right = parse_expression(p, prec);
    if(!right)
    {
        return NULL;
    }
    mcastexpression_t* res = expression_make_infix(p->pstate, op, left, right);
    if(!res)
    {
        expression_destroy(right);
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* parse_grouped_expression(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    mcastexpression_t* expr = parse_expression(p, PRECEDENCE_LOWEST);
    if(!expr || !mc_lexer_expectcurrent(&p->lexer, TOKEN_RPAREN))
    {
        expression_destroy(expr);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    return expr;
}

TMPSTATIC mcastexpression_t* parse_function_literal(mcastparser_t* p)
{
    bool ok;
    p->depth++;
    mcptrarray_t* params = NULL;
    code_block_t* body = NULL;

    if(mc_lexer_currtokenis(&p->lexer, TOKEN_FUNCTION))
    {
        mc_lexer_nexttoken(&p->lexer);
    }

    params = ptrarray_make(p->pstate);

    ok = parse_function_parameters(p, params);

    if(!ok)
    {
        goto err;
    }

    body = parse_code_block(p);
    if(!body)
    {
        goto err;
    }

    mcastexpression_t* res = expression_make_fn_literal(p->pstate, params, body);
    if(!res)
    {
        goto err;
    }

    p->depth -= 1;

    return res;
err:
    code_block_destroy(body);
    ptrarray_destroy_with_items_(params, (ptrarrayitemdestroyfn)ident_destroy);
    p->depth -= 1;
    return NULL;
}

TMPSTATIC bool parse_function_parameters(mcastparser_t* p, mcptrarray_t* outparams)
{
    bool ok;
    ident_t* ident;
    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_LPAREN))
    {
        return false;
    }

    mc_lexer_nexttoken(&p->lexer);

    if(mc_lexer_currtokenis(&p->lexer, TOKEN_RPAREN))
    {
        mc_lexer_nexttoken(&p->lexer);
        return true;
    }

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_IDENT))
    {
        return false;
    }

    ident = ident_make(p->pstate, p->lexer.cur_token);
    if(!ident)
    {
        return false;
    }

    ok = ptrarray_push(outparams, ident);
    if(!ok)
    {
        ident_destroy(ident);
        return false;
    }

    mc_lexer_nexttoken(&p->lexer);

    while(mc_lexer_currtokenis(&p->lexer, TOKEN_COMMA))
    {
        mc_lexer_nexttoken(&p->lexer);

        if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_IDENT))
        {
            return false;
        }

        ident = ident_make(p->pstate, p->lexer.cur_token);
        if(!ident)
        {
            return false;
        }
        ok = ptrarray_push(outparams, ident);
        if(!ok)
        {
            ident_destroy(ident);
            return false;
        }

        mc_lexer_nexttoken(&p->lexer);
    }

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_RPAREN))
    {
        return false;
    }

    mc_lexer_nexttoken(&p->lexer);

    return true;
}

TMPSTATIC mcastexpression_t* parse_call_expression(mcastparser_t* p, mcastexpression_t* left)
{
    mcastexpression_t* function = left;
    mcptrarray_t* args = parse_expression_list(p, TOKEN_LPAREN, TOKEN_RPAREN, false);
    if(!args)
    {
        return NULL;
    }
    mcastexpression_t* res = expression_make_call(p->pstate, function, args);
    if(!res)
    {
        ptrarray_destroy_with_items_(args, (ptrarrayitemdestroyfn)expression_destroy);
        return NULL;
    }
    return res;
}

TMPSTATIC mcptrarray_t* parse_expression_list(mcastparser_t* p, mcasttoktype_t starttoken, mcasttoktype_t endtoken, bool trailingcommaallowed)
{
    bool ok;
    if(!mc_lexer_expectcurrent(&p->lexer, starttoken))
    {
        return NULL;
    }

    mc_lexer_nexttoken(&p->lexer);

    mcptrarray_t* res = ptrarray_make(p->pstate);

    if(mc_lexer_currtokenis(&p->lexer, endtoken))
    {
        mc_lexer_nexttoken(&p->lexer);
        return res;
    }

    mcastexpression_t* argexpr = parse_expression(p, PRECEDENCE_LOWEST);
    if(!argexpr)
    {
        goto err;
    }
    ok = ptrarray_push(res, argexpr);
    if(!ok)
    {
        expression_destroy(argexpr);
        goto err;
    }

    while(mc_lexer_currtokenis(&p->lexer, TOKEN_COMMA))
    {
        mc_lexer_nexttoken(&p->lexer);

        if(trailingcommaallowed && mc_lexer_currtokenis(&p->lexer, endtoken))
        {
            break;
        }

        argexpr = parse_expression(p, PRECEDENCE_LOWEST);
        if(!argexpr)
        {
            goto err;
        }

        ok = ptrarray_push(res, argexpr);
        if(!ok)
        {
            expression_destroy(argexpr);
            goto err;
        }
    }

    if(!mc_lexer_expectcurrent(&p->lexer, endtoken))
    {
        goto err;
    }

    mc_lexer_nexttoken(&p->lexer);

    return res;
err:
    ptrarray_destroy_with_items_(res, (ptrarrayitemdestroyfn)expression_destroy);
    return NULL;
}

TMPSTATIC mcastexpression_t* parse_index_expression(mcastparser_t* p, mcastexpression_t* left)
{
    mc_lexer_nexttoken(&p->lexer);

    mcastexpression_t* index = parse_expression(p, PRECEDENCE_LOWEST);
    if(!index)
    {
        return NULL;
    }

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_RBRACKET))
    {
        expression_destroy(index);
        return NULL;
    }

    mc_lexer_nexttoken(&p->lexer);

    mcastexpression_t* res = expression_make_index(p->pstate, left, index);
    if(!res)
    {
        expression_destroy(index);
        return NULL;
    }

    return res;
}

TMPSTATIC mcastexpression_t* parse_assign_expression(mcastparser_t* p, mcastexpression_t* left)
{
    mcastexpression_t* source = NULL;
    mcasttoktype_t assigntype = p->lexer.cur_token.type;

    mc_lexer_nexttoken(&p->lexer);

    source = parse_expression(p, PRECEDENCE_LOWEST);
    if(!source)
    {
        goto err;
    }

    switch(assigntype)
    {
        case TOKEN_PLUS_ASSIGN:
        case TOKEN_MINUS_ASSIGN:
        case TOKEN_SLASH_ASSIGN:
        case TOKEN_ASTERISK_ASSIGN:
        case TOKEN_PERCENT_ASSIGN:
        case TOKEN_BIT_AND_ASSIGN:
        case TOKEN_BIT_OR_ASSIGN:
        case TOKEN_BIT_XOR_ASSIGN:
        case TOKEN_LSHIFT_ASSIGN:
        case TOKEN_RSHIFT_ASSIGN:
        {
            mcastmathoptype_t op = token_to_operator(assigntype);
            mcastexpression_t* leftcopy = expression_copy(left);
            if(!leftcopy)
            {
                goto err;
            }
            mcastlocation_t pos = source->pos;
            mcastexpression_t* newsource = expression_make_infix(p->pstate, op, leftcopy, source);
            if(!newsource)
            {
                expression_destroy(leftcopy);
                goto err;
            }
            newsource->pos = pos;
            source = newsource;
            break;
        }
        case TOKEN_ASSIGN:
            break;
        default:
            ARCANE_ASSERT(false);
            break;
    }

    mcastexpression_t* res = expression_make_assign(p->pstate, left, source, false);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    expression_destroy(source);
    return NULL;
}

TMPSTATIC mcastexpression_t* parse_logical_expression(mcastparser_t* p, mcastexpression_t* left)
{
    mcastmathoptype_t op = token_to_operator(p->lexer.cur_token.type);
    mcastprecedence_t prec = get_precedence(p->lexer.cur_token.type);
    mc_lexer_nexttoken(&p->lexer);
    mcastexpression_t* right = parse_expression(p, prec);
    if(!right)
    {
        return NULL;
    }
    mcastexpression_t* res = expression_make_logical(p->pstate, op, left, right);
    if(!res)
    {
        expression_destroy(right);
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* parse_ternary_expression(mcastparser_t* p, mcastexpression_t* left)
{
    mc_lexer_nexttoken(&p->lexer);

    mcastexpression_t* if_true = parse_expression(p, PRECEDENCE_LOWEST);
    if(!if_true)
    {
        return NULL;
    }

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_COLON))
    {
        expression_destroy(if_true);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);

    mcastexpression_t* if_false = parse_expression(p, PRECEDENCE_LOWEST);
    if(!if_false)
    {
        expression_destroy(if_true);
        return NULL;
    }

    mcastexpression_t* res = expression_make_ternary(p->pstate, left, if_true, if_false);
    if(!res)
    {
        expression_destroy(if_true);
        expression_destroy(if_false);
        return NULL;
    }

    return res;
}

TMPSTATIC mcastexpression_t* parse_incdec_prefix_expression(mcastparser_t* p)
{
    mcastexpression_t* source = NULL;
    mcasttoktype_t operationtype = p->lexer.cur_token.type;
    mcastlocation_t pos = p->lexer.cur_token.pos;

    mc_lexer_nexttoken(&p->lexer);

    mcastmathoptype_t op = token_to_operator(operationtype);

    mcastexpression_t* dest = parse_expression(p, PRECEDENCE_PREFIX);
    if(!dest)
    {
        goto err;
    }

    mcastexpression_t* oneliteral = expression_make_number_literal(p->pstate, 1);
    if(!oneliteral)
    {
        expression_destroy(dest);
        goto err;
    }
    oneliteral->pos = pos;

    mcastexpression_t* destcopy = expression_copy(dest);
    if(!destcopy)
    {
        expression_destroy(oneliteral);
        expression_destroy(dest);
        goto err;
    }

    mcastexpression_t* operation = expression_make_infix(p->pstate, op, destcopy, oneliteral);
    if(!operation)
    {
        expression_destroy(destcopy);
        expression_destroy(dest);
        expression_destroy(oneliteral);
        goto err;
    }
    operation->pos = pos;

    mcastexpression_t* res = expression_make_assign(p->pstate, dest, operation, false);
    if(!res)
    {
        expression_destroy(dest);
        expression_destroy(operation);
        goto err;
    }
    return res;
err:
    expression_destroy(source);
    return NULL;
}

TMPSTATIC mcastexpression_t* parse_incdec_postfix_expression(mcastparser_t* p, mcastexpression_t* left)
{
    mcastexpression_t* source = NULL;
    mcasttoktype_t operationtype = p->lexer.cur_token.type;
    mcastlocation_t pos = p->lexer.cur_token.pos;

    mc_lexer_nexttoken(&p->lexer);

    mcastmathoptype_t op = token_to_operator(operationtype);
    mcastexpression_t* leftcopy = expression_copy(left);
    if(!leftcopy)
    {
        goto err;
    }

    mcastexpression_t* oneliteral = expression_make_number_literal(p->pstate, 1);
    if(!oneliteral)
    {
        expression_destroy(leftcopy);
        goto err;
    }
    oneliteral->pos = pos;

    mcastexpression_t* operation = expression_make_infix(p->pstate, op, leftcopy, oneliteral);
    if(!operation)
    {
        expression_destroy(oneliteral);
        expression_destroy(leftcopy);
        goto err;
    }
    operation->pos = pos;

    mcastexpression_t* res = expression_make_assign(p->pstate, left, operation, true);
    if(!res)
    {
        expression_destroy(operation);
        goto err;
    }

    return res;
err:
    expression_destroy(source);
    return NULL;
}

TMPSTATIC mcastexpression_t* parse_dot_expression(mcastparser_t* p, mcastexpression_t* left)
{
    mc_lexer_nexttoken(&p->lexer);

    if(!mc_lexer_expectcurrent(&p->lexer, TOKEN_IDENT))
    {
        return NULL;
    }

    char* str = token_duplicate_literal(p->pstate, &p->lexer.cur_token);
    mcastexpression_t* index = expression_make_string_literal(p->pstate, str);
    if(!index)
    {
        allocator_free(p->pstate, str);
        return NULL;
    }
    index->pos = p->lexer.cur_token.pos;

    mc_lexer_nexttoken(&p->lexer);

    mcastexpression_t* res = expression_make_index(p->pstate, left, index);
    if(!res)
    {
        expression_destroy(index);
        return NULL;
    }
    return res;
}

TMPSTATIC mcastprecedence_t get_precedence(mcasttoktype_t tk)
{
    switch(tk)
    {
        case TOKEN_EQ:
            return PRECEDENCE_EQUALS;
        case TOKEN_NOT_EQ:
            return PRECEDENCE_EQUALS;
        case TOKEN_LT:
            return PRECEDENCE_LESSGREATER;
        case TOKEN_LTE:
            return PRECEDENCE_LESSGREATER;
        case TOKEN_GT:
            return PRECEDENCE_LESSGREATER;
        case TOKEN_GTE:
            return PRECEDENCE_LESSGREATER;
        case TOKEN_PLUS:
            return PRECEDENCE_SUM;
        case TOKEN_MINUS:
            return PRECEDENCE_SUM;
        case TOKEN_SLASH:
            return PRECEDENCE_PRODUCT;
        case TOKEN_ASTERISK:
            return PRECEDENCE_PRODUCT;
        case TOKEN_PERCENT:
            return PRECEDENCE_PRODUCT;
        case TOKEN_LPAREN:
            return PRECEDENCE_POSTFIX;
        case TOKEN_LBRACKET:
            return PRECEDENCE_POSTFIX;
        case TOKEN_ASSIGN:
            return PRECEDENCE_ASSIGN;
        case TOKEN_PLUS_ASSIGN:
            return PRECEDENCE_ASSIGN;
        case TOKEN_MINUS_ASSIGN:
            return PRECEDENCE_ASSIGN;
        case TOKEN_ASTERISK_ASSIGN:
            return PRECEDENCE_ASSIGN;
        case TOKEN_SLASH_ASSIGN:
            return PRECEDENCE_ASSIGN;
        case TOKEN_PERCENT_ASSIGN:
            return PRECEDENCE_ASSIGN;
        case TOKEN_BIT_AND_ASSIGN:
            return PRECEDENCE_ASSIGN;
        case TOKEN_BIT_OR_ASSIGN:
            return PRECEDENCE_ASSIGN;
        case TOKEN_BIT_XOR_ASSIGN:
            return PRECEDENCE_ASSIGN;
        case TOKEN_LSHIFT_ASSIGN:
            return PRECEDENCE_ASSIGN;
        case TOKEN_RSHIFT_ASSIGN:
            return PRECEDENCE_ASSIGN;
        case TOKEN_DOT:
            return PRECEDENCE_POSTFIX;
        case TOKEN_AND:
            return PRECEDENCE_LOGICAL_AND;
        case TOKEN_OR:
            return PRECEDENCE_LOGICAL_OR;
        case TOKEN_BIT_OR:
            return PRECEDENCE_BIT_OR;
        case TOKEN_BIT_XOR:
            return PRECEDENCE_BIT_XOR;
        case TOKEN_BIT_AND:
            return PRECEDENCE_BIT_AND;
        case TOKEN_LSHIFT:
            return PRECEDENCE_SHIFT;
        case TOKEN_RSHIFT:
            return PRECEDENCE_SHIFT;
        case TOKEN_QUESTION:
            return PRECEDENCE_TERNARY;
        case TOKEN_PLUS_PLUS:
            return PRECEDENCE_INCDEC;
        case TOKEN_MINUS_MINUS:
            return PRECEDENCE_INCDEC;
        default:
            return PRECEDENCE_LOWEST;
    }
}

TMPSTATIC mcastmathoptype_t token_to_operator(mcasttoktype_t tk)
{
    switch(tk)
    {
        case TOKEN_ASSIGN:
            return OPERATOR_ASSIGN;
        case TOKEN_PLUS:
            return OPERATOR_PLUS;
        case TOKEN_MINUS:
            return OPERATOR_MINUS;
        case TOKEN_BANG:
            return OPERATOR_BANG;
        case TOKEN_ASTERISK:
            return OPERATOR_ASTERISK;
        case TOKEN_SLASH:
            return OPERATOR_SLASH;
        case TOKEN_LT:
            return OPERATOR_LT;
        case TOKEN_LTE:
            return OPERATOR_LTE;
        case TOKEN_GT:
            return OPERATOR_GT;
        case TOKEN_GTE:
            return OPERATOR_GTE;
        case TOKEN_EQ:
            return OPERATOR_EQ;
        case TOKEN_NOT_EQ:
            return OPERATOR_NOT_EQ;
        case TOKEN_PERCENT:
            return OPERATOR_MODULUS;
        case TOKEN_AND:
            return OPERATOR_LOGICAL_AND;
        case TOKEN_OR:
            return OPERATOR_LOGICAL_OR;
        case TOKEN_PLUS_ASSIGN:
            return OPERATOR_PLUS;
        case TOKEN_MINUS_ASSIGN:
            return OPERATOR_MINUS;
        case TOKEN_ASTERISK_ASSIGN:
            return OPERATOR_ASTERISK;
        case TOKEN_SLASH_ASSIGN:
            return OPERATOR_SLASH;
        case TOKEN_PERCENT_ASSIGN:
            return OPERATOR_MODULUS;
        case TOKEN_BIT_AND_ASSIGN:
            return OPERATOR_BIT_AND;
        case TOKEN_BIT_OR_ASSIGN:
            return OPERATOR_BIT_OR;
        case TOKEN_BIT_XOR_ASSIGN:
            return OPERATOR_BIT_XOR;
        case TOKEN_LSHIFT_ASSIGN:
            return OPERATOR_LSHIFT;
        case TOKEN_RSHIFT_ASSIGN:
            return OPERATOR_RSHIFT;
        case TOKEN_BIT_AND:
            return OPERATOR_BIT_AND;
        case TOKEN_BIT_OR:
            return OPERATOR_BIT_OR;
        case TOKEN_BIT_XOR:
            return OPERATOR_BIT_XOR;
        case TOKEN_LSHIFT:
            return OPERATOR_LSHIFT;
        case TOKEN_RSHIFT:
            return OPERATOR_RSHIFT;
        case TOKEN_PLUS_PLUS:
            return OPERATOR_PLUS;
        case TOKEN_MINUS_MINUS:
            return OPERATOR_MINUS;
        default:
        {
            ARCANE_ASSERT(false);
            return OPERATOR_NONE;
        }
    }
}

TMPSTATIC char escarcane_char(char c)
{
    switch(c)
    {
        case '\"':
            return '\"';
        case '\\':
            return '\\';
        case '/':
            return '/';
        case 'b':
            return '\b';
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case '0':
            return '\0';
        default:
            return c;
    }
}

TMPSTATIC char* process_and_copy_string(mcstate_t* state, const char* input, size_t len)
{
    char* output = (char*)allocator_malloc(state, len + 1);
    if(!output)
    {
        return NULL;
    }

    size_t ini = 0;
    size_t outi = 0;

    while(input[ini] != '\0' && ini < len)
    {
        if(input[ini] == '\\')
        {
            ini++;
            output[outi] = escarcane_char(input[ini]);
            if(output[outi] < 0)
            {
                goto error;
            }
        }
        else
        {
            output[outi] = input[ini];
        }
        outi++;
        ini++;
    }
    output[outi] = '\0';
    return output;
error:
    allocator_free(state, output);
    return NULL;
}

TMPSTATIC mcastexpression_t* wrap_expression_in_function_call(mcstate_t* state, mcastexpression_t* expr, const char* function_name)
{
    bool ok;
    mcasttoken_t fntoken;
    token_init(&fntoken, TOKEN_IDENT, function_name, strlen(function_name));
    fntoken.pos = expr->pos;

    ident_t* ident = ident_make(state, fntoken);
    if(!ident)
    {
        return NULL;
    }
    ident->pos = fntoken.pos;

    mcastexpression_t* functionidentexpr = expression_make_ident(state, ident);
    if(!functionidentexpr)
    {
        ident_destroy(ident);
        return NULL;
    }
    functionidentexpr->pos = expr->pos;
    ident = NULL;

    mcptrarray_t* args = ptrarray_make(state);
    if(!args)
    {
        expression_destroy(functionidentexpr);
        return NULL;
    }

    ok = ptrarray_push(args, expr);
    if(!ok)
    {
        ptrarray_destroy(args);
        expression_destroy(functionidentexpr);
        return NULL;
    }

    mcastexpression_t* call_expr = expression_make_call(state, functionidentexpr, args);
    if(!call_expr)
    {
        ptrarray_destroy(args);
        expression_destroy(functionidentexpr);
        return NULL;
    }
    call_expr->pos = expr->pos;

    return call_expr;
}

TMPSEMISTATIC mcastexpression_t* optimise_expression(mcastexpression_t* expr)
{
    switch(expr->type)
    {
        case MC_EXPR_INFIX:
            return optimise_infix_expression(expr);
        case MC_EXPR_PREFIX:
            return optimise_prefix_expression(expr);
        default:
            return NULL;
    }
}

TMPSTATIC mcastexpression_t* optimise_infix_expression(mcastexpression_t* expr)
{
    mcastexpression_t* left = expr->infix.left;
    mcastexpression_t* leftoptimised = optimise_expression(left);
    if(leftoptimised)
    {
        left = leftoptimised;
    }

    mcastexpression_t* right = expr->infix.right;
    mcastexpression_t* rightoptimised = optimise_expression(right);
    if(rightoptimised)
    {
        right = rightoptimised;
    }

    mcastexpression_t* res = NULL;

    bool leftisnumeric = left->type == MC_EXPR_NUMBERLITERAL || left->type == MC_EXPR_BOOLLITERAL;
    bool rightisnumeric = right->type == MC_EXPR_NUMBERLITERAL || right->type == MC_EXPR_BOOLLITERAL;

    bool leftisstring = left->type == MC_EXPR_STRINGLITERAL;
    bool rightisstring = right->type == MC_EXPR_STRINGLITERAL;

    mcstate_t* state = expr->pstate;
    if(leftisnumeric && rightisnumeric)
    {
        double leftval = left->type == MC_EXPR_NUMBERLITERAL ? left->number_literal : left->bool_literal;
        double rightval = right->type == MC_EXPR_NUMBERLITERAL ? right->number_literal : right->bool_literal;
        int64_t leftvalint = (int64_t)leftval;
        int64_t rightvalint = (int64_t)rightval;
        switch(expr->infix.op)
        {
            case OPERATOR_PLUS:
            {
                res = expression_make_number_literal(state, leftval + rightval);
                break;
            }
            case OPERATOR_MINUS:
            {
                res = expression_make_number_literal(state, leftval - rightval);
                break;
            }
            case OPERATOR_ASTERISK:
            {
                res = expression_make_number_literal(state, leftval * rightval);
                break;
            }
            case OPERATOR_SLASH:
            {
                res = expression_make_number_literal(state, leftval / rightval);
                break;
            }
            case OPERATOR_LT:
            {
                res = expression_make_bool_literal(state, leftval < rightval);
                break;
            }
            case OPERATOR_LTE:
            {
                res = expression_make_bool_literal(state, leftval <= rightval);
                break;
            }
            case OPERATOR_GT:
            {
                res = expression_make_bool_literal(state, leftval > rightval);
                break;
            }
            case OPERATOR_GTE:
            {
                res = expression_make_bool_literal(state, leftval >= rightval);
                break;
            }
            case OPERATOR_EQ:
            {
                res = expression_make_bool_literal(state, ARCANE_DBLEQ(leftval, rightval));
                break;
            }
            case OPERATOR_NOT_EQ:
            {
                res = expression_make_bool_literal(state, !ARCANE_DBLEQ(leftval, rightval));
                break;
            }
            case OPERATOR_MODULUS:
            {
                res = expression_make_number_literal(state, fmod(leftval, rightval));
                break;
            }
            case OPERATOR_BIT_AND:
            {
                res = expression_make_number_literal(state, (double)(leftvalint & rightvalint));
                break;
            }
            case OPERATOR_BIT_OR:
            {
                res = expression_make_number_literal(state, (double)(leftvalint | rightvalint));
                break;
            }
            case OPERATOR_BIT_XOR:
            {
                res = expression_make_number_literal(state, (double)(leftvalint ^ rightvalint));
                break;
            }
            case OPERATOR_LSHIFT:
            {
                res = expression_make_number_literal(state, (double)(leftvalint << rightvalint));
                break;
            }
            case OPERATOR_RSHIFT:
            {
                res = expression_make_number_literal(state, (double)(leftvalint >> rightvalint));
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else if(expr->infix.op == OPERATOR_PLUS && leftisstring && rightisstring)
    {
        const char* strleft = left->string_literal;
        const char* strright = right->string_literal;
        char* resstr = arcane_stringf(state, "%s%s", strleft, strright);
        if(resstr)
        {
            res = expression_make_string_literal(state, resstr);
            if(!res)
            {
                allocator_free(state, resstr);
            }
        }
    }

    expression_destroy(leftoptimised);
    expression_destroy(rightoptimised);

    if(res)
    {
        res->pos = expr->pos;
    }

    return res;
}

TMPSTATIC mcastexpression_t* optimise_prefix_expression(mcastexpression_t* expr)
{
    mcastexpression_t* right = expr->prefix.right;
    mcastexpression_t* rightoptimised = optimise_expression(right);
    if(rightoptimised)
    {
        right = rightoptimised;
    }
    mcastexpression_t* res = NULL;
    if(expr->prefix.op == OPERATOR_MINUS && right->type == MC_EXPR_NUMBERLITERAL)
    {
        res = expression_make_number_literal(expr->pstate, -right->number_literal);
    }
    else if(expr->prefix.op == OPERATOR_BANG && right->type == MC_EXPR_BOOLLITERAL)
    {
        res = expression_make_bool_literal(expr->pstate, !right->bool_literal);
    }
    expression_destroy(rightoptimised);
    if(res)
    {
        res->pos = expr->pos;
    }
    return res;
}

TMPSTATIC mcstate_t* mc_state_make(void* ctx)
{
    mcstate_t* state;
    state = (mcstate_t*)malloc(sizeof(mcstate_t));
    if(!state)
    {
        return NULL;
    }
    memset(state, 0, sizeof(mcstate_t));
    set_default_config(state);
    errors_init(&state->errors);
    state->mem = gcmem_make(state);
    if(!state->mem)
    {
        goto err;
    }
    state->files = ptrarray_make(state);
    if(!state->files)
    {
        goto err;
    }
    vm_make(state);
    state->global_store = global_store_make(state);
    if(!state->global_store)
    {
        goto err;
    }
    state->compiler = mc_compiler_make(state, &state->config, state->mem, &state->errors, state->files, state->global_store);
    if(!state->compiler)
    {
        goto err;
    }
    return state;
err:
    arcane_deinit(state);
    free(state);
    return NULL;
}

TMPSTATIC void arcane_destroy(mcstate_t* state)
{
    if(!state)
    {
        return;
    }
    arcane_deinit(state);
    allocator_free(state, state);
}

TMPSTATIC void arcane_free_allocated(mcstate_t* state, void* ptr)
{
    allocator_free(state, ptr);
}

TMPSTATIC void arcane_set_repl_mode(mcstate_t* state, bool enabled)
{
    state->config.repl_mode = enabled;
}

TMPSTATIC bool arcane_set_timeout(mcstate_t* state, double max_execution_time_ms)
{
    if(!arcane_timer_platform_supported())
    {
        state->config.max_execution_time_ms = 0;
        state->config.max_execution_time_set = false;
        return false;
    }

    if(max_execution_time_ms >= 0)
    {
        state->config.max_execution_time_ms = max_execution_time_ms;
        state->config.max_execution_time_set = true;
    }
    else
    {
        state->config.max_execution_time_ms = 0;
        state->config.max_execution_time_set = false;
    }
    return true;
}

TMPSTATIC void arcane_set_stdout_write_function(mcstate_t* state, arcanestdoutwritefn stdoutwrite, void* context)
{
    state->config.stdio.write.fnprintto = stdoutwrite;
    state->config.stdio.write.context = context;
}

TMPSTATIC void arcane_set_file_write_function(mcstate_t* state, arcanewritefilefn filewrite, void* context)
{
    state->config.fileio.write_file.write_file = filewrite;
    state->config.fileio.write_file.context = context;
}

TMPSTATIC void arcane_set_file_read_function(mcstate_t* state, arcanereadfilefn fileread, void* context)
{
    state->config.fileio.read_file.read_file = fileread;
    state->config.fileio.read_file.context = context;
}

TMPSTATIC mccompiledprogram_t* mc_state_compilesource(mcstate_t* state, const char* code)
{
    arcane_clear_errors(state);

    compilation_result_t* comp_res = mc_compiler_compilesource(state->compiler, code);

    if(!comp_res || errors_get_count(&state->errors) > 0)
    {
        goto err;
    }

    mccompiledprogram_t* program = (mccompiledprogram_t*)allocator_malloc(state, sizeof(mccompiledprogram_t));
    if(!program)
    {
        goto err;
    }
    program->pstate = state;
    program->comp_res = comp_res;
    return program;

err:
    compilation_result_destroy(comp_res);
    return NULL;
}


TMPSTATIC mcvalue_t arcane_execute_program(mcstate_t* state, mccompiledprogram_t* program)
{
    bool ok;
    if(program == NULL)
    {
        errors_add_error(&state->errors, ARCANE_ERROR_USER, srcposinvalid, "program passed to execute was null.");
        return mc_value_makenull();
    }

    reset_state(state);

    if(state != program->pstate)
    {
        errors_add_error(&state->errors, ARCANE_ERROR_USER, srcposinvalid, "arcane program was compiled with a different arcane instance");
        return mc_value_makenull();
    }

    ok = vm_run(state, program->comp_res, mc_compiler_getconstants(state->compiler));
    if(!ok || errors_get_count(&state->errors) > 0)
    {
        return mc_value_makenull();
    }

    ARCANE_ASSERT(state->sp == 0);

    mcvalue_t res = vm_get_last_popped(state);
    if(res.type == ARCANE_OBJECT_NONE)
    {
        return mc_value_makenull();
    }

    return res;
}

TMPSTATIC void arcane_program_destroy(mccompiledprogram_t* program)
{
    if(!program)
    {
        return;
    }
    compilation_result_destroy(program->comp_res);
    allocator_free(program->pstate, program);
}

TMPSTATIC mcvalue_t arcane_execute(mcstate_t* state, const char* code)
{
    bool ok;
    reset_state(state);

    compilation_result_t* comp_res = mc_compiler_compilesource(state->compiler, code);

    if(!comp_res || errors_get_count(&state->errors) > 0)
    {
        goto err;
    }

    ok = vm_run(state, comp_res, mc_compiler_getconstants(state->compiler));
    if(!ok || errors_get_count(&state->errors) > 0)
    {
        goto err;
    }

    ARCANE_ASSERT(state->sp == 0);

    mcvalue_t res = vm_get_last_popped(state);
    if(res.type == ARCANE_OBJECT_NONE)
    {
        goto err;
    }

    compilation_result_destroy(comp_res);

    return res;

err:
    compilation_result_destroy(comp_res);
    return mc_value_makenull();
}


TMPSTATIC mcvalue_t arcane_call(mcstate_t* state, const char* function_name, int argc, mcvalue_t* args)
{
    reset_state(state);

    mcvalue_t callee = arcane_get_object(state, function_name);
    if(callee.type == ARCANE_OBJECT_NULL)
    {
        return mc_value_makenull();
    }
    mcvalue_t res = vm_call(state, mc_compiler_getconstants(state->compiler), callee, argc, (mcvalue_t*)args);
    if(errors_get_count(&state->errors) > 0)
    {
        return mc_value_makenull();
    }
    return res;
}

TMPSTATIC bool arcane_has_errors(mcstate_t* state)
{
    return arcane_errors_count(state) > 0;
}

TMPSTATIC int arcane_errors_count(mcstate_t* state)
{
    return errors_get_count(&state->errors);
}

TMPSTATIC void arcane_clear_errors(mcstate_t* state)
{
    errors_clear(&state->errors);
}

TMPSTATIC mcerror_t* arcane_get_error(mcstate_t* state, int index)
{
    return (mcerror_t*)errors_getc(&state->errors, index);
}

TMPSTATIC bool arcane_set_native_function(mcstate_t* state, const char* name, arcanenativefn fn, void* data)
{
    mcvalue_t obj = arcane_object_make_native_function_with_name(state, name, fn, data);
    if(arcane_object_is_null(obj))
    {
        return false;
    }
    return arcane_set_global_constant(state, name, obj);
}

TMPSTATIC bool arcane_set_global_constant(mcstate_t* state, const char* name, mcvalue_t obj)
{
    return global_store_set(state->global_store, name, obj);
}

TMPSTATIC mcvalue_t arcane_get_object(mcstate_t* state, const char* name)
{
    bool ok;
    mcastsymtable_t* st = mc_compiler_getsymtable(state->compiler);
    mcastsymbol_t* symbol = symbol_table_resolve(st, name);
    if(!symbol)
    {
        errors_add_errorf(&state->errors, ARCANE_ERROR_USER, srcposinvalid, "Symbol \"%s\" is not defined", name);
        return mc_value_makenull();
    }
    mcvalue_t res = mc_value_makenull();
    if(symbol->type == SYMBOL_MODULE_GLOBAL)
    {
        res = vm_get_global(state, symbol->index);
    }
    else if(symbol->type == SYMBOL_ARCANE_GLOBAL)
    {
        ok = false;
        res = global_store_get_object_at(state->global_store, symbol->index, &ok);
        if(!ok)
        {
            errors_add_errorf(&state->errors, ARCANE_ERROR_USER, srcposinvalid, "Failed to get global object at %d", symbol->index);
            return mc_value_makenull();
        }
    }
    else
    {
        errors_add_errorf(&state->errors, ARCANE_ERROR_USER, srcposinvalid, "Value associated with symbol \"%s\" could not be loaded", name);
        return mc_value_makenull();
    }
    return res;
}

TMPSTATIC mcvalue_t arcane_object_make_error(mcstate_t* state, const char* msg)
{
    return object_make_error(state, msg);
}

TMPSTATIC mcvalue_t arcane_object_make_errorf(mcstate_t* state, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int towrite = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    va_start(args, fmt);
    char* res = (char*)allocator_malloc(state, towrite + 1);
    if(!res)
    {
        return mc_value_makenull();
    }
    int written = vsprintf(res, fmt, args);
    (void)written;
    ARCANE_ASSERT(written == towrite);
    va_end(args);
    return object_make_error_no_copy(state, res);
}

TMPSTATIC bool arcane_object_is_null(mcvalue_t obj)
{
    return obj.type == ARCANE_OBJECT_NULL;
}


TMPSTATIC void arcane_set_runtime_error(mcstate_t* state, const char* message)
{
    errors_add_error(&state->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, message);
}

TMPSEMISTATIC void arcane_set_runtime_errorf(mcstate_t* state, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int towrite = vsnprintf(NULL, 0, fmt, args);
    (void)towrite;
    va_end(args);
    va_start(args, fmt);
    char res[MC_CONF_ERROR_MSGMAXLENGTH];
    int written = vsnprintf(res, MC_CONF_ERROR_MSGMAXLENGTH, fmt, args);
    (void)written;
    ARCANE_ASSERT(towrite == written);
    va_end(args);
    errors_add_error(&state->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, res);
}

TMPSTATIC mcobjtype_t object_get_type(mcvalue_t v)
{
    return (v).type;
}

TMPSTATIC bool object_is_numeric(mcvalue_t obj)
{
    mcobjtype_t type = object_get_type(obj);
    return type == ARCANE_OBJECT_NUMBER || type == ARCANE_OBJECT_BOOL;
}

TMPSTATIC bool object_is_null(mcvalue_t obj)
{
    return object_get_type(obj) == ARCANE_OBJECT_NULL;
}

TMPSTATIC bool object_is_callable(mcvalue_t obj)
{
    mcobjtype_t type = object_get_type(obj);
    return type == ARCANE_OBJECT_NATIVE_FUNCTION || type == ARCANE_OBJECT_FUNCTION;
}

TMPSTATIC mcobjtype_t arcane_object_get_type(mcvalue_t arcaneobj)
{
    mcvalue_t obj = arcaneobj;
    switch(object_get_type(obj))
    {
        case ARCANE_OBJECT_NONE:
            return ARCANE_OBJECT_NONE;
        case ARCANE_OBJECT_ERROR:
            return ARCANE_OBJECT_ERROR;
        case ARCANE_OBJECT_NUMBER:
            return ARCANE_OBJECT_NUMBER;
        case ARCANE_OBJECT_BOOL:
            return ARCANE_OBJECT_BOOL;
        case ARCANE_OBJECT_STRING:
            return ARCANE_OBJECT_STRING;
        case ARCANE_OBJECT_NULL:
            return ARCANE_OBJECT_NULL;
        case ARCANE_OBJECT_NATIVE_FUNCTION:
            return ARCANE_OBJECT_NATIVE_FUNCTION;
        case ARCANE_OBJECT_ARRAY:
            return ARCANE_OBJECT_ARRAY;
        case ARCANE_OBJECT_MAP:
            return ARCANE_OBJECT_MAP;
        case ARCANE_OBJECT_FUNCTION:
            return ARCANE_OBJECT_FUNCTION;
        case ARCANE_OBJECT_EXTERNAL:
            return ARCANE_OBJECT_EXTERNAL;
        case ARCANE_OBJECT_FREED:
            return ARCANE_OBJECT_FREED;
        case ARCANE_OBJECT_ANY:
            return ARCANE_OBJECT_ANY;
        default:
            return ARCANE_OBJECT_NONE;
    }
}

TMPSTATIC const char* arcane_object_get_type_name(mcobjtype_t type)
{
    switch(type)
    {
        case ARCANE_OBJECT_NONE:
            return "NONE";
        case ARCANE_OBJECT_ERROR:
            return "ERROR";
        case ARCANE_OBJECT_NUMBER:
            return "NUMBER";
        case ARCANE_OBJECT_BOOL:
            return "BOOL";
        case ARCANE_OBJECT_STRING:
            return "STRING";
        case ARCANE_OBJECT_NULL:
            return "NULL";
        case ARCANE_OBJECT_NATIVE_FUNCTION:
            return "NATIVE_FUNCTION";
        case ARCANE_OBJECT_ARRAY:
            return "ARRAY";
        case ARCANE_OBJECT_MAP:
            return "MAP";
        case ARCANE_OBJECT_FUNCTION:
            return "FUNCTION";
        case ARCANE_OBJECT_EXTERNAL:
            return "EXTERNAL";
        case ARCANE_OBJECT_FREED:
            return "FREED";
        case ARCANE_OBJECT_ANY:
            return "ANY";
        default:
            return "NONE";
    }
}

TMPSTATIC const char* arcane_object_get_string(mcvalue_t obj)
{
    return object_get_string(obj);
}

TMPSTATIC mcvalue_t mc_object_makedatafrom(mcobjtype_t type, mcobjdata_t* data)
{
    /*
    uint64_t typetag;
    */
    mcvalue_t object;
    object.type = type;
    data->type = type;
    object.isallocated = true;
    object.objdatahandle = data;
    return object;
}

TMPSTATIC mcvalue_t mc_value_makeempty(mcobjtype_t t)
{
    mcvalue_t o;
    memset(&o, 0, sizeof(mcvalue_t));
    o.type = t;
    o.isallocated = false;
    return o;
}

TMPSTATIC mcvalue_t mc_value_makenumber(double val)
{
    mcvalue_t o;
    o = mc_value_makeempty(ARCANE_OBJECT_NUMBER);
    o.valnumber = val;
    /*
    if((o.objdatahandle & MC_CONF_OBJECT_PATTERN) == MC_CONF_OBJECT_PATTERN)
    {
        o.objdatahandle = 0x7ff8000000000000;
    }
    */
    return o;
}

TMPSTATIC mcvalue_t mc_value_makebool(bool val)
{
    mcvalue_t o;
    o = mc_value_makeempty(ARCANE_OBJECT_BOOL);
    o.valbool = val;
    /*
    o.objdatahandle = MC_CONF_OBJECT_HDRBOOL | val;
    */
    return o;
}

TMPSTATIC mcvalue_t mc_value_makenull()
{
    mcvalue_t o;
    o = mc_value_makeempty(ARCANE_OBJECT_NULL);
    /*
    o.objdatahandle = MC_CONF_OBJECT_NULLPATTERN;
    */
    return o;
}

TMPSTATIC mcvalue_t mc_value_makestrcapacity(mcstate_t* vm, int capacity)
{
    bool ok;
    mcobjdata_t* data = gcmem_get_object_data_from_pool(vm, ARCANE_OBJECT_STRING);
    if(!data)
    {
        data = gcmem_alloc_object_data(vm);
        if(!data)
        {
            return mc_value_makenull();
        }
        data->string.capacity = MC_CONF_OBJECT_STRING_BUF_SIZE - 1;
        data->string.is_allocated = false;
    }
    data->string.length = 0;
    data->string.hash = 0;
    if(capacity > data->string.capacity)
    {
        ok = object_data_string_reserve_capacity(data, capacity);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return mc_object_makedatafrom(ARCANE_OBJECT_STRING, data);
}

TMPSTATIC mcvalue_t mc_value_makestrformat(mcstate_t* vm, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int towrite = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    va_start(args, fmt);
    mcvalue_t res = mc_value_makestrcapacity(vm, towrite);
    if(object_is_null(res))
    {
        return mc_value_makenull();
    }
    char* resbuf = object_get_mutable_string(res);
    int written = vsprintf(resbuf, fmt, args);
    (void)written;
    ARCANE_ASSERT(written == towrite);
    va_end(args);
    object_set_string_length(res, towrite);
    return res;
}

TMPSTATIC mcvalue_t mc_value_makestringlen(mcstate_t* vm, const char* string, size_t len)
{
    bool ok;
    mcvalue_t res;
    res = mc_value_makestrcapacity(vm, len);
    if(object_is_null(res))
    {
        return res;
    }
    ok = mc_value_strappend(res, string, len);
    if(!ok)
    {
        return mc_value_makenull();
    }
    return res;
}

TMPSTATIC mcvalue_t mc_value_makestring(mcstate_t* vm, const char* string)
{
    return mc_value_makestringlen(vm, string, strlen(string));
}

TMPSTATIC mcvalue_t object_make_native_function(mcstate_t* vm, const char* name, nativefn fn, void* data, int data_len)
{
    if(data_len > NATIVE_FN_MAX_DATA_LEN)
    {
        return mc_value_makenull();
    }
    mcobjdata_t* obj = gcmem_alloc_object_data(vm);
    if(!obj)
    {
        return mc_value_makenull();
    }
    obj->native_function.name = arcane_strdup(vm, name);
    if(!obj->native_function.name)
    {
        return mc_value_makenull();
    }
    obj->native_function.fn = fn;
    if(data)
    {
        memcpy(obj->native_function.data, data, data_len);
    }
    obj->native_function.data_len = data_len;
    return mc_object_makedatafrom(ARCANE_OBJECT_NATIVE_FUNCTION, obj);
}

TMPSTATIC mcvalue_t object_make_array(mcstate_t* vm)
{
    return object_make_array_with_capacity(vm, 8);
}

TMPSTATIC mcvalue_t object_make_array_with_capacity(mcstate_t* vm, unsigned capacity)
{
    mcobjdata_t* data = gcmem_get_object_data_from_pool(vm, ARCANE_OBJECT_ARRAY);
    if(data)
    {
        array_clear(data->array);
        return mc_object_makedatafrom(ARCANE_OBJECT_ARRAY, data);
    }
    data = gcmem_alloc_object_data(vm);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->array = array_make_with_capacity(vm, capacity, sizeof(mcvalue_t));
    if(!data->array)
    {
        return mc_value_makenull();
    }
    return mc_object_makedatafrom(ARCANE_OBJECT_ARRAY, data);
}

TMPSTATIC mcvalue_t object_make_map(mcstate_t* vm)
{
    return object_make_map_with_capacity(vm, 32);
}

TMPSTATIC mcvalue_t object_make_map_with_capacity(mcstate_t* vm, unsigned capacity)
{
    mcobjdata_t* data = gcmem_get_object_data_from_pool(vm, ARCANE_OBJECT_MAP);
    if(data)
    {
        valdict_clear(data->map);
        return mc_object_makedatafrom(ARCANE_OBJECT_MAP, data);
    }
    data = gcmem_alloc_object_data(vm);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->map = valdict_make_with_capacity(vm, capacity, sizeof(mcvalue_t), sizeof(mcvalue_t));
    if(!data->map)
    {
        return mc_value_makenull();
    }
    valdict_set_hash_function(data->map, (collectionshashfn)object_hash);
    valdict_set_equals_function(data->map, (collectionsequalsfn)object_equals_wrapped);
    return mc_object_makedatafrom(ARCANE_OBJECT_MAP, data);
}

TMPSTATIC mcvalue_t object_make_error(mcstate_t* vm, const char* error)
{
    char* errorstr = arcane_strdup(vm, error);
    if(!errorstr)
    {
        return mc_value_makenull();
    }
    mcvalue_t res = object_make_error_no_copy(vm, errorstr);
    if(object_is_null(res))
    {
        allocator_free(vm, errorstr);
        return mc_value_makenull();
    }
    return res;
}

TMPSTATIC mcvalue_t object_make_error_no_copy(mcstate_t* vm, char* error)
{
    mcobjdata_t* data = gcmem_alloc_object_data(vm);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->error.message = error;
    data->error.traceback = NULL;
    return mc_object_makedatafrom(ARCANE_OBJECT_ERROR, data);
}

TMPSTATIC mcvalue_t object_make_errorf(mcstate_t* vm, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int towrite = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    va_start(args, fmt);
    char* res = (char*)allocator_malloc(vm, towrite + 1);
    if(!res)
    {
        return mc_value_makenull();
    }
    int written = vsprintf(res, fmt, args);
    (void)written;
    ARCANE_ASSERT(written == towrite);
    va_end(args);
    mcvalue_t resobj = object_make_error_no_copy(vm, res);
    if(object_is_null(resobj))
    {
        allocator_free(vm, res);
        return mc_value_makenull();
    }
    return resobj;
}

TMPSTATIC mcvalue_t object_make_function(mcstate_t* vm, const char* name, compilation_result_t* comp_res, bool owns_data, int num_locals, int num_args, int free_vals_count)
{
    mcobjdata_t* data = gcmem_alloc_object_data(vm);
    if(!data)
    {
        return mc_value_makenull();
    }
    if(owns_data)
    {
        data->function.name = name ? arcane_strdup(vm, name) : arcane_strdup(vm, "anonymous");
        if(!data->function.name)
        {
            return mc_value_makenull();
        }
    }
    else
    {
        data->function.const_name = name ? name : "anonymous";
    }
    data->function.comp_result = comp_res;
    data->function.owns_data = owns_data;
    data->function.num_locals = num_locals;
    data->function.num_args = num_args;
    if(free_vals_count >= ARCANE_ARRAY_LEN(data->function.free_vals_buf))
    {
        data->function.free_vals_allocated = (mcvalue_t*)allocator_malloc(vm, sizeof(mcvalue_t) * free_vals_count);
        if(!data->function.free_vals_allocated)
        {
            return mc_value_makenull();
        }
    }
    data->function.free_vals_count = free_vals_count;
    return mc_object_makedatafrom(ARCANE_OBJECT_FUNCTION, data);
}

TMPSTATIC mcvalue_t object_make_external(mcstate_t* vm, void* data)
{
    mcobjdata_t* obj = gcmem_alloc_object_data(vm);
    if(!obj)
    {
        return mc_value_makenull();
    }
    obj->external.data = data;
    obj->external.data_destroy_fn = NULL;
    obj->external.data_copy_fn = NULL;
    return mc_object_makedatafrom(ARCANE_OBJECT_EXTERNAL, obj);
}

TMPSTATIC void object_deinit(mcvalue_t obj)
{
    if(object_is_allocated(obj))
    {
        mcobjdata_t* data = object_get_allocated_data(obj);
        object_data_deinit(data);
    }
}

TMPSTATIC void object_data_deinit(mcobjdata_t* data)
{
    switch(data->type)
    {
        case ARCANE_OBJECT_FREED:
        {
            ARCANE_ASSERT(false);
            return;
        }
        case ARCANE_OBJECT_STRING:
        {
            if(data->string.is_allocated)
            {
                allocator_free(data->pstate, data->string.actualallocated);
            }
            break;
        }
        case ARCANE_OBJECT_FUNCTION:
        {
            if(data->function.owns_data)
            {
                allocator_free(data->pstate, data->function.name);
                compilation_result_destroy(data->function.comp_result);
            }
            if(freevals_are_allocated(&data->function))
            {
                allocator_free(data->pstate, data->function.free_vals_allocated);
            }
            break;
        }
        case ARCANE_OBJECT_ARRAY:
        {
            array_destroy(data->array);
            break;
        }
        case ARCANE_OBJECT_MAP:
        {
            valdict_destroy(data->map);
            break;
        }
        case ARCANE_OBJECT_NATIVE_FUNCTION:
        {
            allocator_free(data->pstate, data->native_function.name);
            break;
        }
        case ARCANE_OBJECT_EXTERNAL:
        {
            if(data->external.data_destroy_fn)
            {
                data->external.data_destroy_fn(data->external.data);
            }
            break;
        }
        case ARCANE_OBJECT_ERROR:
        {
            allocator_free(data->pstate, data->error.message);
            traceback_destroy(data->error.traceback);
            break;
        }
        default:
        {
            break;
        }
    }
    data->type = ARCANE_OBJECT_FREED;
}

TMPSTATIC bool object_is_allocated(mcvalue_t object)
{
    return object.isallocated;
}

TMPSTATIC mcgcmemory_t* object_get_mem(mcvalue_t obj)
{
    mcobjdata_t* data = object_get_allocated_data(obj);
    return data->mem;
}

TMPSTATIC const char* arcane_error_get_message(mcerror_t* error)
{
    return error->message;
}

TMPSTATIC const char* arcane_error_get_filepath(mcerror_t* error)
{
    if(!error->pos.file)
    {
        return NULL;
    }
    return error->pos.file->path;
}

TMPSTATIC const char* arcane_error_get_line(mcerror_t* error)
{
    if(!error->pos.file)
    {
        return NULL;
    }
    mcptrarray_t* lines = error->pos.file->lines;
    if(error->pos.line >= ptrarray_count(lines))
    {
        return NULL;
    }
    const char* line = ptrarray_get(lines, error->pos.line);
    return line;
}

TMPSTATIC int arcane_error_get_line_number(mcerror_t* error)
{
    if(error->pos.line < 0)
    {
        return -1;
    }
    return error->pos.line + 1;
}

TMPSTATIC int arcane_error_get_column_number(mcerror_t* error)
{
    if(error->pos.column < 0)
    {
        return -1;
    }
    return error->pos.column + 1;
}

TMPSTATIC mcerrtype_t arcane_error_get_type(mcerror_t* error)
{
    switch(error->type)
    {
        case ARCANE_ERROR_NONE:
            return ARCANE_ERROR_NONE;
        case ARCANE_ERROR_PARSING:
            return ARCANE_ERROR_PARSING;
        case ARCANE_ERROR_COMPILATION:
            return ARCANE_ERROR_COMPILATION;
        case ARCANE_ERROR_RUNTIME:
            return ARCANE_ERROR_RUNTIME;
        case ARCANE_ERROR_TIMEOUT:
            return ARCANE_ERROR_TIMEOUT;
        case ARCANE_ERROR_ALLOCATION:
            return ARCANE_ERROR_ALLOCATION;
        case ARCANE_ERROR_USER:
            return ARCANE_ERROR_USER;
        default:
            return ARCANE_ERROR_NONE;
    }
}

TMPSTATIC const char* arcane_error_get_type_string(mcerror_t* error)
{
    return arcane_error_type_to_string(arcane_error_get_type(error));
}

TMPSTATIC const char* arcane_error_type_to_string(mcerrtype_t type)
{
    switch(type)
    {
        case ARCANE_ERROR_PARSING:
            return "PARSING";
        case ARCANE_ERROR_COMPILATION:
            return "COMPILATION";
        case ARCANE_ERROR_RUNTIME:
            return "RUNTIME";
        case ARCANE_ERROR_TIMEOUT:
            return "TIMEOUT";
        case ARCANE_ERROR_ALLOCATION:
            return "ALLOCATION";
        case ARCANE_ERROR_USER:
            return "USER";
        default:
            return "NONE";
    }
}

TMPSTATIC char* arcane_error_serialize(mcstate_t* state, mcerror_t* err)
{
    const char* typestr = arcane_error_get_type_string(err);
    const char* filename = arcane_error_get_filepath(err);
    const char* line = arcane_error_get_line(err);
    int linenum = arcane_error_get_line_number(err);
    int colnum = arcane_error_get_column_number(err);
    mcprintstate_t* buf = mc_printer_make(state, NULL);
    if(!buf)
    {
        return NULL;
    }
    if(line)
    {
        mc_printer_append(buf, line);
        mc_printer_append(buf, "\n");
        if(colnum >= 0)
        {
            for(int j = 0; j < (colnum - 1); j++)
            {
                mc_printer_append(buf, " ");
            }
            mc_printer_append(buf, "^\n");
        }
    }
    mc_printer_appendf(buf, "%s ERROR in \"%s\" on %d:%d: %s\n", typestr, filename, linenum, colnum, arcane_error_get_message(err));
    mctraceback_t* traceback = arcane_error_get_traceback(err);
    if(traceback)
    {
        mc_printer_appendf(buf, "Traceback:\n");
        traceback_to_string((mctraceback_t*)arcane_error_get_traceback(err), buf);
    }
    if(mc_printer_failed(buf))
    {
        mc_printer_destroy(buf);
        return NULL;
    }
    return mc_printer_get_string_and_destroy(buf);
}

TMPSTATIC mctraceback_t* arcane_error_get_traceback(mcerror_t* error)
{
    return (mctraceback_t*)error->traceback;
}

TMPSTATIC int arcane_traceback_get_depth(mctraceback_t* traceback)
{
    return array_count(traceback->items);
}

TMPSTATIC const char* arcane_traceback_get_filepath(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item = array_get(traceback->items, depth);
    if(!item)
    {
        return NULL;
    }
    return traceback_item_get_filepath(item);
}

TMPSTATIC const char* arcane_traceback_get_line(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item = array_get(traceback->items, depth);
    if(!item)
    {
        return NULL;
    }
    return traceback_item_get_line(item);
}

int arcane_traceback_get_line_number(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item = array_get(traceback->items, depth);
    if(!item)
    {
        return -1;
    }
    return item->pos.line;
}

int arcane_traceback_get_column_number(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item = array_get(traceback->items, depth);
    if(!item)
    {
        return -1;
    }
    return item->pos.column;
}

const char* arcane_traceback_get_function_name(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item = array_get(traceback->items, depth);
    if(!item)
    {
        return "";
    }
    return item->function_name;
}

TMPSTATIC void arcane_deinit(mcstate_t* state)
{
    mc_compiler_destroy(state->compiler);
    global_store_destroy(state->global_store);
    gcmem_destroy(state->mem);
    ptrarray_destroy_with_items_(state->files, (ptrarrayitemdestroyfn)mc_compiledfile_destroy);
    errors_deinit(&state->errors);
    vm_destroy(state);
}

TMPSTATIC mcvalue_t arcane_native_fn_wrapper(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)state;
    mcnatfnbox_t* wrapper = data;
    ARCANE_ASSERT(state == wrapper->state);
    mcvalue_t res = wrapper->fn(wrapper->pstate, wrapper->data, argc, (mcvalue_t*)args);
    if(arcane_has_errors(wrapper->pstate))
    {
        return mc_value_makenull();
    }
    return res;
}


TMPSTATIC mcvalue_t arcane_object_make_native_function_with_name(mcstate_t* state, const char* name, arcanenativefn fn, void* data)
{
    mcnatfnbox_t wrapper;
    memset(&wrapper, 0, sizeof(mcnatfnbox_t));
    wrapper.fn = fn;
    wrapper.pstate = state;
    wrapper.data = data;
    mcvalue_t wrappernativefunction = object_make_native_function(state, name, arcane_native_fn_wrapper, &wrapper, sizeof(wrapper));
    if(object_is_null(wrappernativefunction))
    {
        return mc_value_makenull();
    }
    return wrappernativefunction;
}

TMPSTATIC void reset_state(mcstate_t* state)
{
    arcane_clear_errors(state);
    vm_reset(state);
}

TMPSTATIC void set_default_config(mcstate_t* state)
{
    memset(&state->config, 0, sizeof(mcconfig_t));
    arcane_set_repl_mode(state, false);
    arcane_set_timeout(state, -1);
    arcane_set_file_read_function(state, read_file_default, state);
    arcane_set_file_write_function(state, write_file_default, state);
    arcane_set_stdout_write_function(state, stdout_write_default, state);
}

TMPSTATIC char* read_file_default(void* ctx, const char* filename)
{
    mcstate_t* state = ctx;
    FILE* fp = fopen(filename, "r");
    size_t sizetoread = 0;
    size_t sizeread = 0;
    long pos;
    char* filecontents;
    if(!fp)
    {
        return NULL;
    }
    fseek(fp, 0L, SEEK_END);
    pos = ftell(fp);
    if(pos < 0)
    {
        fclose(fp);
        return NULL;
    }
    sizetoread = pos;
    rewind(fp);
    filecontents = (char*)allocator_malloc(state, sizeof(char) * (sizetoread + 1));
    if(!filecontents)
    {
        fclose(fp);
        return NULL;
    }
    sizeread = fread(filecontents, 1, sizetoread, fp);
    if(ferror(fp))
    {
        fclose(fp);
        free(filecontents);
        return NULL;
    }
    fclose(fp);
    filecontents[sizeread] = '\0';
    return filecontents;
}

TMPSTATIC size_t write_file_default(void* ctx, const char* path, const char* string, size_t stringsize)
{
    (void)ctx;
    FILE* fp = fopen(path, "w");
    if(!fp)
    {
        return 0;
    }
    size_t written = fwrite(string, 1, stringsize, fp);
    fclose(fp);
    return written;
}

TMPSTATIC size_t stdout_write_default(void* ctx, const void* data, size_t size)
{
    (void)ctx;
    return fwrite(data, 1, size, stdout);
}



mcastexpression_t* expression_make_ident(mcstate_t* state, ident_t* ident)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_IDENT);
    if(!res)
    {
        return NULL;
    }
    res->ident = ident;
    return res;
}

mcastexpression_t* expression_make_number_literal(mcstate_t* state, double val)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_NUMBERLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->number_literal = val;
    return res;
}

mcastexpression_t* expression_make_bool_literal(mcstate_t* state, bool val)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_BOOLLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->bool_literal = val;
    return res;
}

mcastexpression_t* expression_make_string_literal(mcstate_t* state, char* value)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_STRINGLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->string_literal = value;
    return res;
}

mcastexpression_t* expression_make_null_literal(mcstate_t* state)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_NULLLITERAL);
    if(!res)
    {
        return NULL;
    }
    return res;
}

mcastexpression_t* expression_make_array_literal(mcstate_t* state, mcptrarray_t* values)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_ARRAYLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->array = values;
    return res;
}

mcastexpression_t* expression_make_map_literal(mcstate_t* state, mcptrarray_t* keys, mcptrarray_t* values)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_MAPLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->map.keys = keys;
    res->map.values = values;
    return res;
}

mcastexpression_t* expression_make_prefix(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* right)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_PREFIX);
    if(!res)
    {
        return NULL;
    }
    res->prefix.op = op;
    res->prefix.right = right;
    return res;
}

mcastexpression_t* expression_make_infix(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* left, mcastexpression_t* right)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_INFIX);
    if(!res)
    {
        return NULL;
    }
    res->infix.op = op;
    res->infix.left = left;
    res->infix.right = right;
    return res;
}

mcastexpression_t* expression_make_fn_literal(mcstate_t* state, mcptrarray_t* params, code_block_t* body)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_FUNCTIONLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->fn_literal.name = NULL;
    res->fn_literal.params = params;
    res->fn_literal.body = body;
    return res;
}

mcastexpression_t* expression_make_call(mcstate_t* state, mcastexpression_t* function, mcptrarray_t* args)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_CALL);
    if(!res)
    {
        return NULL;
    }
    res->call_expr.function = function;
    res->call_expr.args = args;
    return res;
}

mcastexpression_t* expression_make_index(mcstate_t* state, mcastexpression_t* left, mcastexpression_t* index)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_INDEX);
    if(!res)
    {
        return NULL;
    }
    res->index_expr.left = left;
    res->index_expr.index = index;
    return res;
}

mcastexpression_t* expression_make_assign(mcstate_t* state, mcastexpression_t* dest, mcastexpression_t* source, bool is_postfix)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_ASSIGN);
    if(!res)
    {
        return NULL;
    }
    res->assign.dest = dest;
    res->assign.source = source;
    res->assign.is_postfix = is_postfix;
    return res;
}

mcastexpression_t* expression_make_logical(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* left, mcastexpression_t* right)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_LOGICAL);
    if(!res)
    {
        return NULL;
    }
    res->logical.op = op;
    res->logical.left = left;
    res->logical.right = right;
    return res;
}

mcastexpression_t* expression_make_ternary(mcstate_t* state, mcastexpression_t* test, mcastexpression_t* if_true, mcastexpression_t* if_false)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_TERNARY);
    if(!res)
    {
        return NULL;
    }
    res->ternary.test = test;
    res->ternary.if_true = if_true;
    res->ternary.if_false = if_false;
    return res;
}

void expression_destroy(mcastexpression_t* expr)
{
    if(!expr)
    {
        return;
    }

    switch(expr->type)
    {
        case MC_EXPR_NONE:
        {
            ARCANE_ASSERT(false);
            break;
        }
        case MC_EXPR_IDENT:
        {
            ident_destroy(expr->ident);
            break;
        }
        case MC_EXPR_NUMBERLITERAL:
        case MC_EXPR_BOOLLITERAL:
        {
            break;
        }
        case MC_EXPR_STRINGLITERAL:
        {
            allocator_free(expr->pstate, expr->string_literal);
            break;
        }
        case MC_EXPR_NULLLITERAL:
        {
            break;
        }
        case MC_EXPR_ARRAYLITERAL:
        {
            ptrarray_destroy_with_items_(expr->array, (ptrarrayitemdestroyfn)expression_destroy);
            break;
        }
        case MC_EXPR_MAPLITERAL:
        {
            ptrarray_destroy_with_items_(expr->map.keys, (ptrarrayitemdestroyfn)expression_destroy);
            ptrarray_destroy_with_items_(expr->map.values, (ptrarrayitemdestroyfn)expression_destroy);
            break;
        }
        case MC_EXPR_PREFIX:
        {
            expression_destroy(expr->prefix.right);
            break;
        }
        case MC_EXPR_INFIX:
        {
            expression_destroy(expr->infix.left);
            expression_destroy(expr->infix.right);
            break;
        }
        case MC_EXPR_FUNCTIONLITERAL:
        {
            fn_literal_t* fn = &expr->fn_literal;
            allocator_free(expr->pstate, fn->name);
            ptrarray_destroy_with_items_(fn->params, (ptrarrayitemdestroyfn)ident_destroy);
            code_block_destroy(fn->body);
            break;
        }
        case MC_EXPR_CALL:
        {
            ptrarray_destroy_with_items_(expr->call_expr.args, (ptrarrayitemdestroyfn)expression_destroy);
            expression_destroy(expr->call_expr.function);
            break;
        }
        case MC_EXPR_INDEX:
        {
            expression_destroy(expr->index_expr.left);
            expression_destroy(expr->index_expr.index);
            break;
        }
        case MC_EXPR_ASSIGN:
        {
            expression_destroy(expr->assign.dest);
            expression_destroy(expr->assign.source);
            break;
        }
        case MC_EXPR_LOGICAL:
        {
            expression_destroy(expr->logical.left);
            expression_destroy(expr->logical.right);
            break;
        }
        case MC_EXPR_TERNARY:
        {
            expression_destroy(expr->ternary.test);
            expression_destroy(expr->ternary.if_true);
            expression_destroy(expr->ternary.if_false);
            break;
        }
        default:
            break;
    }
    allocator_free(expr->pstate, expr);
}

mcastexpression_t* expression_copy(mcastexpression_t* expr)
{
    if(!expr)
    {
        return NULL;
    }
    mcastexpression_t* res = NULL;
    switch(expr->type)
    {
        case MC_EXPR_NONE:
        {
            ARCANE_ASSERT(false);
            break;
        }
        case MC_EXPR_IDENT:
        {
            ident_t* ident = ident_copy(expr->ident);
            if(!ident)
            {
                return NULL;
            }
            res = expression_make_ident(expr->pstate, ident);
            if(!res)
            {
                ident_destroy(ident);
                return NULL;
            }
            break;
        }
        case MC_EXPR_NUMBERLITERAL:
        {
            res = expression_make_number_literal(expr->pstate, expr->number_literal);
            break;
        }
        case MC_EXPR_BOOLLITERAL:
        {
            res = expression_make_bool_literal(expr->pstate, expr->bool_literal);
            break;
        }
        case MC_EXPR_STRINGLITERAL:
        {
            char* stringcopy = arcane_strdup(expr->pstate, expr->string_literal);
            if(!stringcopy)
            {
                return NULL;
            }
            res = expression_make_string_literal(expr->pstate, stringcopy);
            if(!res)
            {
                allocator_free(expr->pstate, stringcopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_NULLLITERAL:
        {
            res = expression_make_null_literal(expr->pstate);
            break;
        }
        case MC_EXPR_ARRAYLITERAL:
        {
            mcptrarray_t* valuescopy = ptrarray_copy_with_items_(expr->array, (ptrarrayitemcopyfn)expression_copy, (ptrarrayitemdestroyfn)expression_destroy);
            if(!valuescopy)
            {
                return NULL;
            }
            res = expression_make_array_literal(expr->pstate, valuescopy);
            if(!res)
            {
                ptrarray_destroy_with_items_(valuescopy, (ptrarrayitemdestroyfn)expression_destroy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_MAPLITERAL:
        {
            mcptrarray_t* keyscopy = ptrarray_copy_with_items_(expr->map.keys, (ptrarrayitemcopyfn)expression_copy, (ptrarrayitemdestroyfn)expression_destroy);
            mcptrarray_t* valuescopy = ptrarray_copy_with_items_(expr->map.values, (ptrarrayitemcopyfn)expression_copy, (ptrarrayitemdestroyfn)expression_destroy);
            if(!keyscopy || !valuescopy)
            {
                ptrarray_destroy_with_items_(keyscopy, (ptrarrayitemdestroyfn)expression_destroy);
                ptrarray_destroy_with_items_(valuescopy, (ptrarrayitemdestroyfn)expression_destroy);
                return NULL;
            }
            res = expression_make_map_literal(expr->pstate, keyscopy, valuescopy);
            if(!res)
            {
                ptrarray_destroy_with_items_(keyscopy, (ptrarrayitemdestroyfn)expression_destroy);
                ptrarray_destroy_with_items_(valuescopy, (ptrarrayitemdestroyfn)expression_destroy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_PREFIX:
        {
            mcastexpression_t* rightcopy = expression_copy(expr->prefix.right);
            if(!rightcopy)
            {
                return NULL;
            }
            res = expression_make_prefix(expr->pstate, expr->prefix.op, rightcopy);
            if(!res)
            {
                expression_destroy(rightcopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_INFIX:
        {
            mcastexpression_t* leftcopy = expression_copy(expr->infix.left);
            mcastexpression_t* rightcopy = expression_copy(expr->infix.right);
            if(!leftcopy || !rightcopy)
            {
                expression_destroy(leftcopy);
                expression_destroy(rightcopy);
                return NULL;
            }
            res = expression_make_infix(expr->pstate, expr->infix.op, leftcopy, rightcopy);
            if(!res)
            {
                expression_destroy(leftcopy);
                expression_destroy(rightcopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_FUNCTIONLITERAL:
        {
            mcptrarray_t* paramscopy = ptrarray_copy_with_items_(expr->fn_literal.params, (ptrarrayitemcopyfn)ident_copy, (ptrarrayitemdestroyfn)ident_destroy);
            code_block_t* bodycopy = code_block_copy(expr->fn_literal.body);
            char* namecopy = arcane_strdup(expr->pstate, expr->fn_literal.name);
            if(!paramscopy || !bodycopy)
            {
                ptrarray_destroy_with_items_(paramscopy, (ptrarrayitemdestroyfn)ident_destroy);
                code_block_destroy(bodycopy);
                allocator_free(expr->pstate, namecopy);
                return NULL;
            }
            res = expression_make_fn_literal(expr->pstate, paramscopy, bodycopy);
            if(!res)
            {
                ptrarray_destroy_with_items_(paramscopy, (ptrarrayitemdestroyfn)ident_destroy);
                code_block_destroy(bodycopy);
                allocator_free(expr->pstate, namecopy);
                return NULL;
            }
            res->fn_literal.name = namecopy;
            break;
        }
        case MC_EXPR_CALL:
        {
            mcastexpression_t* functioncopy = expression_copy(expr->call_expr.function);
            mcptrarray_t* argscopy = ptrarray_copy_with_items_(expr->call_expr.args, (ptrarrayitemcopyfn)expression_copy, (ptrarrayitemdestroyfn)expression_destroy);
            if(!functioncopy || !argscopy)
            {
                expression_destroy(functioncopy);
                ptrarray_destroy_with_items_(expr->call_expr.args, (ptrarrayitemdestroyfn)expression_destroy);
                return NULL;
            }
            res = expression_make_call(expr->pstate, functioncopy, argscopy);
            if(!res)
            {
                expression_destroy(functioncopy);
                ptrarray_destroy_with_items_(expr->call_expr.args, (ptrarrayitemdestroyfn)expression_destroy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_INDEX:
        {
            mcastexpression_t* leftcopy = expression_copy(expr->index_expr.left);
            mcastexpression_t* indexcopy = expression_copy(expr->index_expr.index);
            if(!leftcopy || !indexcopy)
            {
                expression_destroy(leftcopy);
                expression_destroy(indexcopy);
                return NULL;
            }
            res = expression_make_index(expr->pstate, leftcopy, indexcopy);
            if(!res)
            {
                expression_destroy(leftcopy);
                expression_destroy(indexcopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_ASSIGN:
        {
            mcastexpression_t* destcopy = expression_copy(expr->assign.dest);
            mcastexpression_t* sourcecopy = expression_copy(expr->assign.source);
            if(!destcopy || !sourcecopy)
            {
                expression_destroy(destcopy);
                expression_destroy(sourcecopy);
                return NULL;
            }
            res = expression_make_assign(expr->pstate, destcopy, sourcecopy, expr->assign.is_postfix);
            if(!res)
            {
                expression_destroy(destcopy);
                expression_destroy(sourcecopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_LOGICAL:
        {
            mcastexpression_t* leftcopy = expression_copy(expr->logical.left);
            mcastexpression_t* rightcopy = expression_copy(expr->logical.right);
            if(!leftcopy || !rightcopy)
            {
                expression_destroy(leftcopy);
                expression_destroy(rightcopy);
                return NULL;
            }
            res = expression_make_logical(expr->pstate, expr->logical.op, leftcopy, rightcopy);
            if(!res)
            {
                expression_destroy(leftcopy);
                expression_destroy(rightcopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_TERNARY:
        {
            mcastexpression_t* testcopy = expression_copy(expr->ternary.test);
            mcastexpression_t* iftruecopy = expression_copy(expr->ternary.if_true);
            mcastexpression_t* iffalsecopy = expression_copy(expr->ternary.if_false);
            if(!testcopy || !iftruecopy || !iffalsecopy)
            {
                expression_destroy(testcopy);
                expression_destroy(iftruecopy);
                expression_destroy(iffalsecopy);
                return NULL;
            }
            res = expression_make_ternary(expr->pstate, testcopy, iftruecopy, iffalsecopy);
            if(!res)
            {
                expression_destroy(testcopy);
                expression_destroy(iftruecopy);
                expression_destroy(iffalsecopy);
                return NULL;
            }
            break;
        }
        default:
            break;
    }
    if(!res)
    {
        return NULL;
    }
    res->pos = expr->pos;
    return res;
}

mcastexpression_t* statement_make_define(mcstate_t* state, ident_t* name, mcastexpression_t* value, bool assignable)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTDEFINE);
    if(!res)
    {
        return NULL;
    }
    res->define.name = name;
    res->define.value = value;
    res->define.assignable = assignable;
    return res;
}

mcastexpression_t* statement_make_if(mcstate_t* state, mcptrarray_t* cases, code_block_t* alternative)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTIF);
    if(!res)
    {
        return NULL;
    }
    res->if_statement.cases = cases;
    res->if_statement.alternative = alternative;
    return res;
}

mcastexpression_t* statement_make_return(mcstate_t* state, mcastexpression_t* value)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTRETURN);
    if(!res)
    {
        return NULL;
    }
    res->return_value = value;
    return res;
}

mcastexpression_t* statement_make_expression(mcstate_t* state, mcastexpression_t* value)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTEXPRESSION);
    if(!res)
    {
        return NULL;
    }
    res->expression = value;
    return res;
}

mcastexpression_t* statement_make_while_loop(mcstate_t* state, mcastexpression_t* test, code_block_t* body)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTLOOPWHILE);
    if(!res)
    {
        return NULL;
    }
    res->while_loop.test = test;
    res->while_loop.body = body;
    return res;
}

mcastexpression_t* statement_make_break(mcstate_t* state)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTBREAK);
    if(!res)
    {
        return NULL;
    }
    return res;
}

mcastexpression_t* statement_make_foreach(mcstate_t* state, ident_t* iterator, mcastexpression_t* source, code_block_t* body)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTLOOPFOREACH);
    if(!res)
    {
        return NULL;
    }
    res->foreach.iterator = iterator;
    res->foreach.source = source;
    res->foreach.body = body;
    return res;
}

mcastexpression_t* statement_make_for_loop(mcstate_t* state, mcastexpression_t* init, mcastexpression_t* test, mcastexpression_t* update, code_block_t* body)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTLOOPFORCLASSIC);
    if(!res)
    {
        return NULL;
    }
    res->for_loop.init = init;
    res->for_loop.test = test;
    res->for_loop.update = update;
    res->for_loop.body = body;
    return res;
}

mcastexpression_t* statement_make_continue(mcstate_t* state)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTCONTINUE);
    if(!res)
    {
        return NULL;
    }
    return res;
}

mcastexpression_t* statement_make_block(mcstate_t* state, code_block_t* block)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTBLOCK);
    if(!res)
    {
        return NULL;
    }
    res->block = block;
    return res;
}

mcastexpression_t* statement_make_import(mcstate_t* state, char* path)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTIMPORT);
    if(!res)
    {
        return NULL;
    }
    res->import.path = path;
    return res;
}

mcastexpression_t* statement_make_recover(mcstate_t* state, ident_t* error_ident, code_block_t* body)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTRECOVER);
    if(!res)
    {
        return NULL;
    }
    res->recover.error_ident = error_ident;
    res->recover.body = body;
    return res;
}

void statement_destroy(mcastexpression_t* stmt)
{
    if(!stmt)
    {
        return;
    }
    switch(stmt->type)
    {
        case MC_EXPR_NONE:
        {
            ARCANE_ASSERT(false);
            break;
        }
        case MC_EXPR_STMTDEFINE:
        {
            ident_destroy(stmt->define.name);
            expression_destroy(stmt->define.value);
            break;
        }
        case MC_EXPR_STMTIF:
        {
            ptrarray_destroy_with_items_(stmt->if_statement.cases, (ptrarrayitemdestroyfn)if_case_destroy);
            code_block_destroy(stmt->if_statement.alternative);
            break;
        }
        case MC_EXPR_STMTRETURN:
        {
            expression_destroy(stmt->return_value);
            break;
        }
        case MC_EXPR_STMTEXPRESSION:
        {
            expression_destroy(stmt->expression);
            break;
        }
        case MC_EXPR_STMTLOOPWHILE:
        {
            expression_destroy(stmt->while_loop.test);
            code_block_destroy(stmt->while_loop.body);
            break;
        }
        case MC_EXPR_STMTBREAK:
        {
            break;
        }
        case MC_EXPR_STMTCONTINUE:
        {
            break;
        }
        case MC_EXPR_STMTLOOPFOREACH:
        {
            ident_destroy(stmt->foreach.iterator);
            expression_destroy(stmt->foreach.source);
            code_block_destroy(stmt->foreach.body);
            break;
        }
        case MC_EXPR_STMTLOOPFORCLASSIC:
        {
            statement_destroy(stmt->for_loop.init);
            expression_destroy(stmt->for_loop.test);
            expression_destroy(stmt->for_loop.update);
            code_block_destroy(stmt->for_loop.body);
            break;
        }
        case MC_EXPR_STMTBLOCK:
        {
            code_block_destroy(stmt->block);
            break;
        }
        case MC_EXPR_STMTIMPORT:
        {
            allocator_free(stmt->pstate, stmt->import.path);
            break;
        }
        case MC_EXPR_STMTRECOVER:
        {
            code_block_destroy(stmt->recover.body);
            ident_destroy(stmt->recover.error_ident);
            break;
        }
        default:
            break;
    }
    allocator_free(stmt->pstate, stmt);
}

mcastexpression_t* statement_copy(mcastexpression_t* stmt)
{
    if(!stmt)
    {
        return NULL;
    }
    mcastexpression_t* res = NULL;
    switch(stmt->type)
    {
        case MC_EXPR_NONE:
        {
            ARCANE_ASSERT(false);
            break;
        }
        case MC_EXPR_STMTDEFINE:
        {
            mcastexpression_t* valuecopy = expression_copy(stmt->define.value);
            if(!valuecopy)
            {
                return NULL;
            }
            res = statement_make_define(stmt->pstate, ident_copy(stmt->define.name), valuecopy, stmt->define.assignable);
            if(!res)
            {
                expression_destroy(valuecopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTIF:
        {
            mcptrarray_t* casescopy = ptrarray_copy_with_items_(stmt->if_statement.cases, (ptrarrayitemcopyfn)if_case_copy, (ptrarrayitemdestroyfn)if_case_destroy);
            code_block_t* alternativecopy = code_block_copy(stmt->if_statement.alternative);
            if(!casescopy || !alternativecopy)
            {
                ptrarray_destroy_with_items_(casescopy, (ptrarrayitemdestroyfn)if_case_destroy);
                code_block_destroy(alternativecopy);
                return NULL;
            }
            res = statement_make_if(stmt->pstate, casescopy, alternativecopy);
            if(res)
            {
                ptrarray_destroy_with_items_(casescopy, (ptrarrayitemdestroyfn)if_case_destroy);
                code_block_destroy(alternativecopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTRETURN:
        {
            mcastexpression_t* valuecopy = expression_copy(stmt->return_value);
            if(!valuecopy)
            {
                return NULL;
            }
            res = statement_make_return(stmt->pstate, valuecopy);
            if(!res)
            {
                expression_destroy(valuecopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTEXPRESSION:
        {
            mcastexpression_t* valuecopy = expression_copy(stmt->expression);
            if(!valuecopy)
            {
                return NULL;
            }
            res = statement_make_expression(stmt->pstate, valuecopy);
            if(!res)
            {
                expression_destroy(valuecopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTLOOPWHILE:
        {
            mcastexpression_t* testcopy = expression_copy(stmt->while_loop.test);
            code_block_t* bodycopy = code_block_copy(stmt->while_loop.body);
            if(!testcopy || !bodycopy)
            {
                expression_destroy(testcopy);
                code_block_destroy(bodycopy);
                return NULL;
            }
            res = statement_make_while_loop(stmt->pstate, testcopy, bodycopy);
            if(!res)
            {
                expression_destroy(testcopy);
                code_block_destroy(bodycopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTBREAK:
        {
            res = statement_make_break(stmt->pstate);
            break;
        }
        case MC_EXPR_STMTCONTINUE:
        {
            res = statement_make_continue(stmt->pstate);
            break;
        }
        case MC_EXPR_STMTLOOPFOREACH:
        {
            mcastexpression_t* sourcecopy = expression_copy(stmt->foreach.source);
            code_block_t* bodycopy = code_block_copy(stmt->foreach.body);
            if(!sourcecopy || !bodycopy)
            {
                expression_destroy(sourcecopy);
                code_block_destroy(bodycopy);
                return NULL;
            }
            res = statement_make_foreach(stmt->pstate, ident_copy(stmt->foreach.iterator), sourcecopy, bodycopy);
            if(!res)
            {
                expression_destroy(sourcecopy);
                code_block_destroy(bodycopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTLOOPFORCLASSIC:
        {
            mcastexpression_t* initcopy = statement_copy(stmt->for_loop.init);
            mcastexpression_t* testcopy = expression_copy(stmt->for_loop.test);
            mcastexpression_t* updatecopy = expression_copy(stmt->for_loop.update);
            code_block_t* bodycopy = code_block_copy(stmt->for_loop.body);
            if(!initcopy || !testcopy || !updatecopy || !bodycopy)
            {
                statement_destroy(initcopy);
                expression_destroy(testcopy);
                expression_destroy(updatecopy);
                code_block_destroy(bodycopy);
                return NULL;
            }
            res = statement_make_for_loop(stmt->pstate, initcopy, testcopy, updatecopy, bodycopy);
            if(!res)
            {
                statement_destroy(initcopy);
                expression_destroy(testcopy);
                expression_destroy(updatecopy);
                code_block_destroy(bodycopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTBLOCK:
        {
            code_block_t* blockcopy = code_block_copy(stmt->block);
            if(!blockcopy)
            {
                return NULL;
            }
            res = statement_make_block(stmt->pstate, blockcopy);
            if(!res)
            {
                code_block_destroy(blockcopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTIMPORT:
        {
            char* pathcopy = arcane_strdup(stmt->pstate, stmt->import.path);
            if(!pathcopy)
            {
                return NULL;
            }
            res = statement_make_import(stmt->pstate, pathcopy);
            if(!res)
            {
                allocator_free(stmt->pstate, pathcopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTRECOVER:
        {
            code_block_t* bodycopy = code_block_copy(stmt->recover.body);
            ident_t* erroridentcopy = ident_copy(stmt->recover.error_ident);
            if(!bodycopy || !erroridentcopy)
            {
                code_block_destroy(bodycopy);
                ident_destroy(erroridentcopy);
                return NULL;
            }
            res = statement_make_recover(stmt->pstate, erroridentcopy, bodycopy);
            if(!res)
            {
                code_block_destroy(bodycopy);
                ident_destroy(erroridentcopy);
                return NULL;
            }
            break;
        }
        default:
            break;
    }
    if(!res)
    {
        return NULL;
    }
    res->pos = stmt->pos;
    return res;
}

code_block_t* code_block_make(mcstate_t* state, mcptrarray_t* statements)
{
    code_block_t* block = (code_block_t*)allocator_malloc(state, sizeof(code_block_t));
    if(!block)
    {
        return NULL;
    }
    block->pstate = state;
    block->statements = statements;
    return block;
}

void code_block_destroy(code_block_t* block)
{
    if(!block)
    {
        return;
    }
    ptrarray_destroy_with_items_(block->statements, (ptrarrayitemdestroyfn)statement_destroy);
    allocator_free(block->pstate, block);
}

code_block_t* code_block_copy(code_block_t* block)
{
    if(!block)
    {
        return NULL;
    }
    mcptrarray_t* statementscopy = ptrarray_copy_with_items_(block->statements, (ptrarrayitemcopyfn)statement_copy, (ptrarrayitemdestroyfn)statement_destroy);
    if(!statementscopy)
    {
        return NULL;
    }
    code_block_t* res = code_block_make(block->pstate, statementscopy);
    if(!res)
    {
        ptrarray_destroy_with_items_(statementscopy, (ptrarrayitemdestroyfn)statement_destroy);
        return NULL;
    }
    return res;
}

char* statements_to_string(mcstate_t* state, mcptrarray_t* statements)
{
    mcprintstate_t* buf = mc_printer_make(state, NULL);
    if(!buf)
    {
        return NULL;
    }
    int count = ptrarray_count(statements);
    for(int i = 0; i < count; i++)
    {
        mcastexpression_t* stmt = ptrarray_get(statements, i);
        statement_to_string(stmt, buf);
        if(i < (count - 1))
        {
            mc_printer_append(buf, "\n");
        }
    }
    return mc_printer_get_string_and_destroy(buf);
}

void statement_to_string(mcastexpression_t* stmt, mcprintstate_t* buf)
{
    switch(stmt->type)
    {
        case MC_EXPR_STMTDEFINE:
        {
            define_statement_t* defstmt = &stmt->define;
            if(stmt->define.assignable)
            {
                mc_printer_append(buf, "var ");
            }
            else
            {
                mc_printer_append(buf, "const ");
            }
            mc_printer_append(buf, defstmt->name->value);
            mc_printer_append(buf, " = ");

            if(defstmt->value)
            {
                expression_to_string(defstmt->value, buf);
            }

            break;
        }
        case MC_EXPR_STMTIF:
        {
            if_case_t* ifcase = ptrarray_get(stmt->if_statement.cases, 0);
            mc_printer_append(buf, "if (");
            expression_to_string(ifcase->test, buf);
            mc_printer_append(buf, ") ");
            code_block_to_string(ifcase->consequence, buf);
            for(int i = 1; i < ptrarray_count(stmt->if_statement.cases); i++)
            {
                if_case_t* elifcase = ptrarray_get(stmt->if_statement.cases, i);
                mc_printer_append(buf, " elif (");
                expression_to_string(elifcase->test, buf);
                mc_printer_append(buf, ") ");
                code_block_to_string(elifcase->consequence, buf);
            }
            if(stmt->if_statement.alternative)
            {
                mc_printer_append(buf, " else ");
                code_block_to_string(stmt->if_statement.alternative, buf);
            }
            break;
        }
        case MC_EXPR_STMTRETURN:
        {
            mc_printer_append(buf, "return ");
            if(stmt->return_value)
            {
                expression_to_string(stmt->return_value, buf);
            }
            break;
        }
        case MC_EXPR_STMTEXPRESSION:
        {
            if(stmt->expression)
            {
                expression_to_string(stmt->expression, buf);
            }
            break;
        }
        case MC_EXPR_STMTLOOPWHILE:
        {
            mc_printer_append(buf, "while (");
            expression_to_string(stmt->while_loop.test, buf);
            mc_printer_append(buf, ")");
            code_block_to_string(stmt->while_loop.body, buf);
            break;
        }
        case MC_EXPR_STMTLOOPFORCLASSIC:
        {
            mc_printer_append(buf, "for (");
            if(stmt->for_loop.init)
            {
                statement_to_string(stmt->for_loop.init, buf);
                mc_printer_append(buf, " ");
            }
            else
            {
                mc_printer_append(buf, ";");
            }
            if(stmt->for_loop.test)
            {
                expression_to_string(stmt->for_loop.test, buf);
                mc_printer_append(buf, "; ");
            }
            else
            {
                mc_printer_append(buf, ";");
            }
            if(stmt->for_loop.update)
            {
                expression_to_string(stmt->for_loop.test, buf);
            }
            mc_printer_append(buf, ")");
            code_block_to_string(stmt->for_loop.body, buf);
            break;
        }
        case MC_EXPR_STMTLOOPFOREACH:
        {
            mc_printer_append(buf, "for (");
            mc_printer_appendf(buf, "%s", stmt->foreach.iterator->value);
            mc_printer_append(buf, " in ");
            expression_to_string(stmt->foreach.source, buf);
            mc_printer_append(buf, ")");
            code_block_to_string(stmt->foreach.body, buf);
            break;
        }
        case MC_EXPR_STMTBLOCK:
        {
            code_block_to_string(stmt->block, buf);
            break;
        }
        case MC_EXPR_STMTBREAK:
        {
            mc_printer_append(buf, "break");
            break;
        }
        case MC_EXPR_STMTCONTINUE:
        {
            mc_printer_append(buf, "continue");
            break;
        }
        case MC_EXPR_STMTIMPORT:
        {
            mc_printer_appendf(buf, "import \"%s\"", stmt->import.path);
            break;
        }
        case MC_EXPR_NONE:
        {
            mc_printer_append(buf, "MC_EXPR_NONE");
            break;
        }
        case MC_EXPR_STMTRECOVER:
        {
            mc_printer_appendf(buf, "recover (%s)", stmt->recover.error_ident->value);
            code_block_to_string(stmt->recover.body, buf);
            break;
        }
        default:
            break;
    }
}

void expression_to_string(mcastexpression_t* expr, mcprintstate_t* buf)
{
    switch(expr->type)
    {
        case MC_EXPR_IDENT:
        {
            mc_printer_append(buf, expr->ident->value);
            break;
        }
        case MC_EXPR_NUMBERLITERAL:
        {
            mc_printer_appendf(buf, "%1.17g", expr->number_literal);
            break;
        }
        case MC_EXPR_BOOLLITERAL:
        {
            mc_printer_appendf(buf, "%s", expr->bool_literal ? "true" : "false");
            break;
        }
        case MC_EXPR_STRINGLITERAL:
        {
            mc_printer_appendf(buf, "\"%s\"", expr->string_literal);
            break;
        }
        case MC_EXPR_NULLLITERAL:
        {
            mc_printer_append(buf, "null");
            break;
        }
        case MC_EXPR_ARRAYLITERAL:
        {
            mc_printer_append(buf, "[");
            for(int i = 0; i < ptrarray_count(expr->array); i++)
            {
                mcastexpression_t* arrexpr = ptrarray_get(expr->array, i);
                expression_to_string(arrexpr, buf);
                if(i < (ptrarray_count(expr->array) - 1))
                {
                    mc_printer_append(buf, ", ");
                }
            }
            mc_printer_append(buf, "]");
            break;
        }
        case MC_EXPR_MAPLITERAL:
        {
            map_literal_t* map = &expr->map;

            mc_printer_append(buf, "{");
            for(int i = 0; i < ptrarray_count(map->keys); i++)
            {
                mcastexpression_t* keyexpr = ptrarray_get(map->keys, i);
                mcastexpression_t* valexpr = ptrarray_get(map->values, i);

                expression_to_string(keyexpr, buf);
                mc_printer_append(buf, " : ");
                expression_to_string(valexpr, buf);

                if(i < (ptrarray_count(map->keys) - 1))
                {
                    mc_printer_append(buf, ", ");
                }
            }
            mc_printer_append(buf, "}");
            break;
        }
        case MC_EXPR_PREFIX:
        {
            mc_printer_append(buf, "(");
            mc_printer_append(buf, operator_to_string(expr->infix.op));
            expression_to_string(expr->prefix.right, buf);
            mc_printer_append(buf, ")");
            break;
        }
        case MC_EXPR_INFIX:
        {
            mc_printer_append(buf, "(");
            expression_to_string(expr->infix.left, buf);
            mc_printer_append(buf, " ");
            mc_printer_append(buf, operator_to_string(expr->infix.op));
            mc_printer_append(buf, " ");
            expression_to_string(expr->infix.right, buf);
            mc_printer_append(buf, ")");
            break;
        }
        case MC_EXPR_FUNCTIONLITERAL:
        {
            fn_literal_t* fn = &expr->fn_literal;

            mc_printer_append(buf, "fn");

            mc_printer_append(buf, "(");
            for(int i = 0; i < ptrarray_count(fn->params); i++)
            {
                ident_t* param = ptrarray_get(fn->params, i);
                mc_printer_append(buf, param->value);
                if(i < (ptrarray_count(fn->params) - 1))
                {
                    mc_printer_append(buf, ", ");
                }
            }
            mc_printer_append(buf, ") ");

            code_block_to_string(fn->body, buf);

            break;
        }
        case MC_EXPR_CALL:
        {
            call_expression_t* call_expr = &expr->call_expr;

            expression_to_string(call_expr->function, buf);

            mc_printer_append(buf, "(");
            for(int i = 0; i < ptrarray_count(call_expr->args); i++)
            {
                mcastexpression_t* arg = ptrarray_get(call_expr->args, i);
                expression_to_string(arg, buf);
                if(i < (ptrarray_count(call_expr->args) - 1))
                {
                    mc_printer_append(buf, ", ");
                }
            }
            mc_printer_append(buf, ")");

            break;
        }
        case MC_EXPR_INDEX:
        {
            mc_printer_append(buf, "(");
            expression_to_string(expr->index_expr.left, buf);
            mc_printer_append(buf, "[");
            expression_to_string(expr->index_expr.index, buf);
            mc_printer_append(buf, "])");
            break;
        }
        case MC_EXPR_ASSIGN:
        {
            expression_to_string(expr->assign.dest, buf);
            mc_printer_append(buf, " = ");
            expression_to_string(expr->assign.source, buf);
            break;
        }
        case MC_EXPR_LOGICAL:
        {
            expression_to_string(expr->logical.left, buf);
            mc_printer_append(buf, " ");
            mc_printer_append(buf, operator_to_string(expr->infix.op));
            mc_printer_append(buf, " ");
            expression_to_string(expr->logical.right, buf);
            break;
        }
        case MC_EXPR_TERNARY:
        {
            expression_to_string(expr->ternary.test, buf);
            mc_printer_append(buf, " ? ");
            expression_to_string(expr->ternary.if_true, buf);
            mc_printer_append(buf, " : ");
            expression_to_string(expr->ternary.if_false, buf);
            break;
        }
        case MC_EXPR_NONE:
        {
            mc_printer_append(buf, "MC_EXPR_NONE");
            break;
        }
        default:
            break;
    }
}

void code_block_to_string(code_block_t* stmt, mcprintstate_t* buf)
{
    mc_printer_append(buf, "{ ");
    for(int i = 0; i < ptrarray_count(stmt->statements); i++)
    {
        mcastexpression_t* istmt = ptrarray_get(stmt->statements, i);
        statement_to_string(istmt, buf);
        mc_printer_append(buf, "\n");
    }
    mc_printer_append(buf, " }");
}

const char* operator_to_string(mcastmathoptype_t op)
{
    switch(op)
    {
        case OPERATOR_NONE:
            return "OPERATOR_NONE";
        case OPERATOR_ASSIGN:
            return "=";
        case OPERATOR_PLUS:
            return "+";
        case OPERATOR_MINUS:
            return "-";
        case OPERATOR_BANG:
            return "!";
        case OPERATOR_ASTERISK:
            return "*";
        case OPERATOR_SLASH:
            return "/";
        case OPERATOR_LT:
            return "<";
        case OPERATOR_GT:
            return ">";
        case OPERATOR_EQ:
            return "==";
        case OPERATOR_NOT_EQ:
            return "!=";
        case OPERATOR_MODULUS:
            return "%";
        case OPERATOR_LOGICAL_AND:
            return "&&";
        case OPERATOR_LOGICAL_OR:
            return "||";
        case OPERATOR_BIT_AND:
            return "&";
        case OPERATOR_BIT_OR:
            return "|";
        case OPERATOR_BIT_XOR:
            return "^";
        case OPERATOR_LSHIFT:
            return "<<";
        case OPERATOR_RSHIFT:
            return ">>";
        default:
            return "OPERATOR_UNKNOWN";
    }
}

const char* expression_type_to_string(mcastexprtype_t type)
{
    switch(type)
    {
        case MC_EXPR_NONE:
            return "NONE";
        case MC_EXPR_IDENT:
            return "IDENT";
        case MC_EXPR_NUMBERLITERAL:
            return "INT_LITERAL";
        case MC_EXPR_BOOLLITERAL:
            return "BOOL_LITERAL";
        case MC_EXPR_STRINGLITERAL:
            return "STRING_LITERAL";
        case MC_EXPR_ARRAYLITERAL:
            return "ARRAY_LITERAL";
        case MC_EXPR_MAPLITERAL:
            return "MAP_LITERAL";
        case MC_EXPR_PREFIX:
            return "PREFIX";
        case MC_EXPR_INFIX:
            return "INFIX";
        case MC_EXPR_FUNCTIONLITERAL:
            return "FN_LITERAL";
        case MC_EXPR_CALL:
            return "CALL";
        case MC_EXPR_INDEX:
            return "INDEX";
        case MC_EXPR_ASSIGN:
            return "ASSIGN";
        case MC_EXPR_LOGICAL:
            return "LOGICAL";
        case MC_EXPR_TERNARY:
            return "TERNARY";
        default:
            return "UNKNOWN";
    }
}

ident_t* ident_make(mcstate_t* state, mcasttoken_t tok)
{
    ident_t* res = (ident_t*)allocator_malloc(state, sizeof(ident_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->value = token_duplicate_literal(state, &tok);
    if(!res->value)
    {
        allocator_free(state, res);
        return NULL;
    }
    res->pos = tok.pos;
    return res;
}

ident_t* ident_copy(ident_t* ident)
{
    ident_t* res = allocator_malloc(ident->pstate, sizeof(ident_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = ident->pstate;
    res->value = arcane_strdup(ident->pstate, ident->value);
    if(!res->value)
    {
        allocator_free(ident->pstate, res);
        return NULL;
    }
    res->pos = ident->pos;
    return res;
}

void ident_destroy(ident_t* ident)
{
    if(!ident)
    {
        return;
    }
    allocator_free(ident->pstate, ident->value);
    ident->value = NULL;
    ident->pos = srcposinvalid;
    allocator_free(ident->pstate, ident);
}

if_case_t* if_case_make(mcstate_t* state, mcastexpression_t* test, code_block_t* consequence)
{
    if_case_t* res = (if_case_t*)allocator_malloc(state, sizeof(if_case_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->test = test;
    res->consequence = consequence;
    return res;
}

void if_case_destroy(if_case_t* cond)
{
    if(!cond)
    {
        return;
    }
    expression_destroy(cond->test);
    code_block_destroy(cond->consequence);
    allocator_free(cond->pstate, cond);
}

if_case_t* if_case_copy(if_case_t* ifcase)
{
    if(!ifcase)
    {
        return NULL;
    }
    mcastexpression_t* testcopy = NULL;
    code_block_t* consequencecopy = NULL;
    if_case_t* if_case_copy = NULL;

    testcopy = expression_copy(ifcase->test);
    if(!testcopy)
    {
        goto err;
    }
    consequencecopy = code_block_copy(ifcase->consequence);
    if(!testcopy || !consequencecopy)
    {
        goto err;
    }
    if_case_copy = if_case_make(ifcase->pstate, testcopy, consequencecopy);
    if(!if_case_copy)
    {
        goto err;
    }
    return if_case_copy;
err:
    expression_destroy(testcopy);
    code_block_destroy(consequencecopy);
    if_case_destroy(if_case_copy);
    return NULL;
}

TMPSTATIC mcastexpression_t* expression_make(mcstate_t* state, mcastexprtype_t type)
{
    mcastexpression_t* res = (mcastexpression_t*)allocator_malloc(state, sizeof(mcastexpression_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->type = type;
    res->pos = srcposinvalid;
    return res;
}

TMPSTATIC mcastexpression_t* statement_make(mcstate_t* state, mcastexprtype_t type)
{
    mcastexpression_t* res = (mcastexpression_t*)allocator_malloc(state, sizeof(mcastexpression_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->type = type;
    res->pos = srcposinvalid;
    return res;
}

#define CHECK_ARGS(state, generateerror, argc, args, ...) \
    mc_args_check((state), (generateerror), (argc), (args), sizeof((mcobjtype_t[]){ __VA_ARGS__ }) / sizeof(mcobjtype_t), (mcobjtype_t[]){ __VA_ARGS__ })

static struct
{
    const char* name;
    nativefn fn;
} gnativefunctions[] = {
    /*
    set_native_function(state, "externalfntest", externalfntest, &gexternalfntest);
    set_global_constant(state, "test", mc_value_makenumber(42));
    set_global_constant(state, "teststr", mc_value_makestrformat(state, "%s %s", "lorem", "ipsum"));
    set_native_function(state, "testcheckargs", testcheckargsfun, NULL);
    set_native_function(state, "vec2add", vec2addfun, NULL);
    set_native_function(state, "vec2sub", vec2subfun, NULL);
    */
    { "maketestdict", cfn_maketestdict },
    { "squarearray", cfn_squarearray },
    { "len", cfn_len },
    { "println", cfn_println },
    { "print", cfn_print },
    { "read_file", cfn_readfile },
    { "write_file", cfn_writefile },
    { "first", cfn_first },
    { "last", cfn_last },
    { "rest", cfn_rest },
    { "append", cfn_append },
    { "remove", cfn_remove },
    { "removeat", cfn_removeat },
    { "tostring", cfn_tostring },
    { "tonum", cfn_tonum },
    { "range", cfn_range },
    { "keys", cfn_keys },
    { "values", cfn_values },
    { "copy", cfn_copy },
    { "deepcopy", cfn_copydeep },
    { "concat", cfn_concat },
    { "chr", cfn_chr },
    { "reverse", cfn_reverse },
    { "array", cfn_array },
    { "error", cfn_error },
    { "crash", cfn_crash },
    { "assert", cfn_assert },
    { "randomseed", cfn_randseed },
    { "random", cfn_random },
    { "slice", cfn_slice },

    /* Custom */
    { "indexof", cfn_index },
    { "left", cfn_left },
    { "right", cfn_right },
    { "replace", cfn_replace },
    { "replacefirst", cfn_replacefirst },
    { "trim", cfn_trim },

    /* Type checks */
    { "isstring", cfn_isstring },
    { "isarray", cfn_isarray },
    { "ismap", cfn_ismap },
    { "isnumber", cfn_isnumber },
    { "isbool", cfn_isbool },
    { "isnull", cfn_isnull },
    { "isfunction", cfn_isfunction },
    { "isexternal", cfn_isexternal },
    { "iserror", cfn_iserror },
    { "isnativefunction", cfn_isnative_function },

    /* Math */
    { "sqrt", cfn_sqrt },
    { "pow", cfn_pow },
    { "sin", cfn_sin },
    { "cos", cfn_cos },
    { "tan", cfn_tan },
    { "log", cfn_log },
    { "ceil", cfn_ceil },
    { "floor", cfn_floor },
    { "abs", cfn_abs },
};

int builtins_count()
{
    return ARCANE_ARRAY_LEN(gnativefunctions);
}

nativefn builtins_get_fn(int ix)
{
    return gnativefunctions[ix].fn;
}

const char* builtins_get_name(int ix)
{
    return gnativefunctions[ix].name;
}

/**
 * \brief Searches a string an instance of another string in it and returns the index of the first occurance.  If no occurance is found a -1 is returned.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return The index of the found string or -1 if it's not found.
 */
TMPSTATIC mcvalue_t cfn_index(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)state;
    (void)data;
    int startindex = argc == 3 && args[2].type == ARCANE_OBJECT_NUMBER ? object_get_number(args[2]) : 0;

    if((argc == 2 || argc == 3) && args[0].type == ARCANE_OBJECT_STRING && args[1].type == ARCANE_OBJECT_STRING)
    {
        const char* searchstr = object_get_string(args[0]);
        const char* searchfor = object_get_string(args[1]);
        char* result = strstr(searchstr + startindex, searchfor);

        if(result == NULL)
        {
            return mc_value_makenumber(-1);
        }

        return mc_value_makenumber(result - searchstr);
    }

    return mc_value_makenumber(-1);
}

/**
 * \brief Returns the specified number of characters from the left hand side of the string.  If more characters exist than the length of the string the entire string is returned.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return The section of the string from the left-hand side.
 */
TMPSTATIC mcvalue_t cfn_left(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(argc == 2 && args[0].type == ARCANE_OBJECT_STRING && args[1].type == ARCANE_OBJECT_NUMBER)
    {
        const char* searchstr = object_get_string(args[0]);
        int length = object_get_number(args[1]);
        /*
        * If the requested length is longer than the string then return a new string
        * of the full length.
        */
        if(length > (int)strlen(searchstr))
        {
            return mc_value_makestring(state, searchstr);
        }

        char* result = (char*)malloc(length + 1);

        if(result == NULL)
        {
            return mc_value_makenull();
        }

        strncpy(result, searchstr, length);
        result[length] = '\0';

        mcvalue_t obj = mc_value_makestring(state, result);
        free(result);

        return obj;
    }

    return mc_value_makenull();
}

/**
 * \brief Returns the specified number of characters from the right hand side of the string.  If more characters exist than the length of the string the entire string is returned.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return The section of the string from the right-hand side.
 */
TMPSTATIC mcvalue_t cfn_right(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(argc == 2 && args[0].type == ARCANE_OBJECT_STRING && args[1].type == ARCANE_OBJECT_NUMBER)
    {
        const char* searchstr = object_get_string(args[0]);
        int length = object_get_number(args[1]);
        /*
        * If the requested length is longer than the string then return a new string
        * of the full length.
        */
        if(length >= (int)strlen(searchstr))
        {
            return mc_value_makestring(state, searchstr);
        }

        char* result = (char*)malloc(length + 1);

        if(result == NULL)
        {
            return mc_value_makenull();
        }

        int strlength = strlen(searchstr);
        strncpy(result, searchstr + strlength - length, length);
        result[length] = '\0';

        mcvalue_t obj = mc_value_makestring(state, result);
        free(result);

        return obj;
    }

    return mc_value_makenull();
}

/**
 * \brief Replaces all occurances of one string in another.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return The string with all occurances replaced.
 */
TMPSTATIC mcvalue_t cfn_replace(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(argc == 3 && args[0].type == ARCANE_OBJECT_STRING && args[1].type == ARCANE_OBJECT_STRING && args[2].type == ARCANE_OBJECT_STRING)
    {
        const char* str = object_get_string(args[0]);
        const char* searchstr = object_get_string(args[1]);
        const char* replacestr = object_get_string(args[2]);

        size_t searchlen = strlen(searchstr);
        size_t replacelen = strlen(replacestr);

        size_t count = 0;
        const char* temp = str;

        /* Count number of occurrences of searchstr in str */
        while((temp = strstr(temp, searchstr)))
        {
            count++;
            temp += searchlen;
        }
        /* Allocate new string to store result */
        size_t newlen = strlen(str) + count * (replacelen - searchlen) + 1;
        char* result = (char*)malloc(newlen);

        if(result == NULL)
        {
            return mc_value_makenull();
        }

        /* Replace all instances of searchstr with replacestr */
        char* ptr = result;
        while((temp = strstr(str, searchstr)))
        {
            size_t len = temp - str;
            memcpy(ptr, str, len);
            ptr += len;
            memcpy(ptr, replacestr, replacelen);
            ptr += replacelen;
            str = temp + searchlen;
        }

        /* Copy remaining part of str */
        strcpy(ptr, str);
        mcvalue_t obj = mc_value_makestring(state, result);
        free(result);
        return obj;
    }

    return mc_value_makenull();
}

/**
 * \brief Replaces the first occurance of one string in another.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return The string with the first occurance of the replacement replaced.
 */
TMPSTATIC mcvalue_t cfn_replacefirst(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(argc == 3 && args[0].type == ARCANE_OBJECT_STRING && args[1].type == ARCANE_OBJECT_STRING && args[2].type == ARCANE_OBJECT_STRING)
    {
        const char* str = object_get_string(args[0]);
        const char* searchstr = object_get_string(args[1]);
        const char* replacestr = object_get_string(args[2]);

        size_t searchlen = strlen(searchstr);
        size_t replacelen = strlen(replacestr);

        const char* temp = strstr(str, searchstr);
        if(temp == NULL)
        {
            return mc_value_makestring(state, str);
        }

        /* Allocate new string to store result */
        size_t newlen = strlen(str) + (replacelen - searchlen) + 1;
        char* result = (char*)malloc(newlen);
        if(result == NULL)
        {
            return mc_value_makenull();
        }

        /* Replace the first instance of searchstr with replacestr */
        size_t len = temp - str;
        memcpy(result, str, len);
        memcpy(result + len, replacestr, replacelen);
        strcpy(result + len + replacelen, temp + searchlen);

        mcvalue_t obj = mc_value_makestring(state, result);
        free(result);
        return obj;
    }

    return mc_value_makenull();
}

/**
 * \brief Trims whitespace off the start and end of a string.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return Returns a string that has whitespace trimmed from the start and finish.
 */
TMPSTATIC mcvalue_t cfn_trim(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(argc == 1 && args[0].type == ARCANE_OBJECT_STRING)
    {
        const char* str = object_get_string(args[0]);

        if(IS_NULLSTR(str))
        {
            return mc_value_makestring(state, "");
        }

        int length = strlen(str);
        char* result = (char*)malloc(length + 1);

        if(result == NULL)
        {
            return mc_value_makestring(state, "");
        }

        strncpy(result, str, length);
        result[length] = '\0';

        int i = 0, j = length - 1;

        /* Trim whitespace from the front of the string */
        while((isspace(result[i]) || result[i] == '\t') && result[i] != '\0')
        {
            i++;
        }
        /* Trim whitespace from the end of the string */
        while((isspace(result[j]) || result[j] == '\t') && j >= i)
        {
            j--;
        }
        /* Shift the trimmed string to the beginning of the buffer */
        int k = 0;
        while(i <= j)
        {
            result[k] = result[i];
            k++;
            i++;
        }

        result[k] = '\0';

        mcvalue_t obj = mc_value_makestring(state, result);
        free(result);

        return obj;
    }

    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_len(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, ARCANE_OBJECT_STRING | ARCANE_OBJECT_ARRAY | ARCANE_OBJECT_MAP))
    {
        return mc_value_makenull();
    }

    mcvalue_t arg = args[0];
    mcobjtype_t type = arg.type;
    if(type == ARCANE_OBJECT_STRING)
    {
        int len = object_get_string_length(arg);
        return mc_value_makenumber(len);
    }
    if(type == ARCANE_OBJECT_ARRAY)
    {
        int len = object_get_array_length(arg);
        return mc_value_makenumber(len);
    }
    if(type == ARCANE_OBJECT_MAP)
    {
        int len = object_get_map_length(arg);
        return mc_value_makenumber(len);
    }

    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_first(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, ARCANE_OBJECT_ARRAY))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    return object_get_array_value_at(arg, 0);
}

TMPSTATIC mcvalue_t cfn_last(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, ARCANE_OBJECT_ARRAY))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    return object_get_array_value_at(arg, object_get_array_length(arg) - 1);
}

TMPSTATIC mcvalue_t cfn_rest(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, ARCANE_OBJECT_ARRAY))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    int len = object_get_array_length(arg);
    if(len == 0)
    {
        return mc_value_makenull();
    }

    mcvalue_t res = object_make_array(state);
    if(object_is_null(res))
    {
        return mc_value_makenull();
    }
    for(int i = 1; i < len; i++)
    {
        mcvalue_t item = object_get_array_value_at(arg, i);
        ok = object_add_array_value(res, item);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

TMPSTATIC mcvalue_t cfn_reverse(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, ARCANE_OBJECT_ARRAY | ARCANE_OBJECT_STRING))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    mcobjtype_t type = arg.type;
    if(type == ARCANE_OBJECT_ARRAY)
    {
        int len = object_get_array_length(arg);
        mcvalue_t res = object_make_array_with_capacity(state, len);
        if(object_is_null(res))
        {
            return mc_value_makenull();
        }
        for(int i = 0; i < len; i++)
        {
            mcvalue_t obj = object_get_array_value_at(arg, i);
            ok = object_set_array_value_at(res, len - i - 1, obj);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return res;
    }
    if(type == ARCANE_OBJECT_STRING)
    {
        const char* str = object_get_string(arg);
        int len = object_get_string_length(arg);

        mcvalue_t res = mc_value_makestrcapacity(state, len);
        if(object_is_null(res))
        {
            return mc_value_makenull();
        }
        char* resbuf = object_get_mutable_string(res);
        for(int i = 0; i < len; i++)
        {
            resbuf[len - i - 1] = str[i];
        }
        resbuf[len] = '\0';
        object_set_string_length(res, len);
        return res;
    }
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_array(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    (void)data;
    if(argc == 1)
    {
        if(!CHECK_ARGS(state, true, argc, args, ARCANE_OBJECT_NUMBER))
        {
            return mc_value_makenull();
        }
        int capacity = (int)object_get_number(args[0]);
        mcvalue_t res = object_make_array_with_capacity(state, capacity);
        if(object_is_null(res))
        {
            return mc_value_makenull();
        }
        mcvalue_t objnull = mc_value_makenull();
        for(int i = 0; i < capacity; i++)
        {
            ok = object_add_array_value(res, objnull);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return res;
    }
    if(argc == 2)
    {
        if(!CHECK_ARGS(state, true, argc, args, ARCANE_OBJECT_NUMBER, ARCANE_OBJECT_ANY))
        {
            return mc_value_makenull();
        }
        int capacity = (int)object_get_number(args[0]);
        mcvalue_t res = object_make_array_with_capacity(state, capacity);
        if(object_is_null(res))
        {
            return mc_value_makenull();
        }
        for(int i = 0; i < capacity; i++)
        {
            ok = object_add_array_value(res, args[1]);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return res;
    }
    CHECK_ARGS(state, true, argc, args, ARCANE_OBJECT_NUMBER);
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_append(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, ARCANE_OBJECT_ARRAY, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    ok = object_add_array_value(args[0], args[1]);
    if(!ok)
    {
        return mc_value_makenull();
    }
    int len = object_get_array_length(args[0]);
    return mc_value_makenumber(len);
}

TMPSTATIC mcvalue_t cfn_externalfn(mcstate_t* state, void *data, int argc, mcvalue_t *args)
{
    int *test;
    (void)state;
    (void)argc;
    (void)args;
    test = (int*)data;
    *test = 42;
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t vec2_add_fun(mcstate_t *state, void *data, int argc, mcvalue_t *args)
{
    mcvalue_t keyx;
    mcvalue_t keyy;
    (void)data;    
    if (!CHECK_ARGS(state, true, argc, args, ARCANE_OBJECT_MAP, ARCANE_OBJECT_MAP))
    {
        return mc_value_makenull();
    }
    keyx = mc_value_makestring(state, "x");
    keyy = mc_value_makestring(state, "y");
    double a_x = object_get_number(object_get_map_value(args[0], keyx));
    double a_y = object_get_number(object_get_map_value(args[0], keyy));

    double b_x = object_get_number(object_get_map_value(args[1], keyx));
    double b_y = object_get_number(object_get_map_value(args[1], keyy));

    mcvalue_t res = object_make_map(state);
    if (object_get_type(res) == ARCANE_OBJECT_NULL) {
        return res;
    }
    object_set_map_value(res, keyx, mc_value_makenumber(a_x + b_x));
    object_set_map_value(res, keyy, mc_value_makenumber(a_y + b_y));
    return res;
}

TMPSTATIC mcvalue_t vec2_sub_fun(mcstate_t *state, void *data, int argc, mcvalue_t *args)
{
    mcvalue_t keyx;
    mcvalue_t keyy;
    (void)data;
    if (!CHECK_ARGS(state, true, argc, args, ARCANE_OBJECT_MAP, ARCANE_OBJECT_MAP)) {
        return mc_value_makenull();
    }
    keyx = mc_value_makestring(state, "x");
    keyx = mc_value_makestring(state, "y");
    double a_x = object_get_number(object_get_map_value(args[0], keyx));
    double a_y = object_get_number(object_get_map_value(args[0], keyy));

    double b_x = object_get_number(object_get_map_value(args[1], keyx));
    double b_y = object_get_number(object_get_map_value(args[1], keyy));

    mcvalue_t res = object_make_map(state);
    object_set_map_value(res, keyx, mc_value_makenumber(a_x - b_x));
    object_set_map_value(res, keyy, mc_value_makenumber(a_y - b_y));
    return res;
}

TMPSTATIC mcvalue_t cfn_test_check_args(mcstate_t* state, void *data, int argc, mcvalue_t *args)
{
    (void)data;
    if (!CHECK_ARGS(state, true, argc, args,
                  ARCANE_OBJECT_NUMBER,
                  ARCANE_OBJECT_ARRAY | ARCANE_OBJECT_MAP,
                  ARCANE_OBJECT_MAP,
                  ARCANE_OBJECT_STRING,
                  ARCANE_OBJECT_NUMBER | ARCANE_OBJECT_BOOL,
                  ARCANE_OBJECT_FUNCTION | ARCANE_OBJECT_NATIVE_FUNCTION,
                  ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makenumber(42);
}


TMPSTATIC mcvalue_t cfn_maketestdict(mcstate_t *vm, void *data, int argc, mcvalue_t *args)
{
    (void)data;
    if (argc != 1)
    {
        arcane_set_runtime_errorf(vm, "Invalid type passed to maketestdict, got %d, expected 1", argc);
        return mc_value_makenull();
    }    
    if (args[0].type != ARCANE_OBJECT_NUMBER)
    {
        const char *typename = arcane_object_get_type_name(args[0].type);
        arcane_set_runtime_errorf(vm, "Invalid type passed to maketestdict, got %s", typename);
        return mc_value_makenull();
    }
    int numitems = object_get_number(args[0]);
    mcvalue_t res = object_make_map(vm);
    if (res.type == ARCANE_OBJECT_NULL)
    {
        return mc_value_makenull();
    }
    for (int i = 0; i < numitems; i++)
    {
        char keybuf[64];
        sprintf(keybuf, "%d", i);
        mcvalue_t key = mc_value_makestring(vm, keybuf);
        mcvalue_t val = mc_value_makenumber(i);
        object_set_map_value(res, key, val);
    }
    return res;
}

TMPSTATIC mcvalue_t cfn_squarearray(mcstate_t *vm, void *data, int argc, mcvalue_t *args)
{
    int i;
    double num;
    mcvalue_t res;
    mcvalue_t resitem;    
    (void)data;
    res = object_make_array_with_capacity(vm, argc);

    for(i = 0; i < argc; i++)
    {
        if(object_get_type(args[i]) != ARCANE_OBJECT_NUMBER)
        {
            arcane_set_runtime_error(vm, "Invalid type passed to squarearray");
            return mc_value_makenull();
        }
        num = object_get_number(args[i]);
        resitem = mc_value_makenumber(num * num);
        object_add_array_value(res, resitem);
    }
    return res;
}

TMPSTATIC mcvalue_t cfn_print(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    int i;
    mcvalue_t arg;
    mcprintstate_t pr;
    (void)data;
    mc_printer_init(&pr, vm, 0, stdout, true);
    for(i = 0; i < argc; i++)
    {
        arg = args[i];
        mc_printer_printobject(arg, &pr, false);
    }
    mc_printer_destroy(&pr);
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_println(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    mcvalue_t o;
    o = cfn_print(vm, data, argc, args);
    fputc('\n', stdout);
    fflush(stdout);
    return o;
}

TMPSTATIC mcvalue_t cfn_writefile(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_STRING, ARCANE_OBJECT_STRING))
    {
        return mc_value_makenull();
    }

    mcconfig_t* config = &vm->config;

    if(!config->fileio.write_file.write_file)
    {
        return mc_value_makenull();
    }

    const char* path = object_get_string(args[0]);
    const char* string = object_get_string(args[1]);
    int stringlen = object_get_string_length(args[1]);

    int written = config->fileio.write_file.write_file(config->fileio.write_file.context, path, string, stringlen);

    return mc_value_makenumber(written);
}

TMPSTATIC mcvalue_t cfn_readfile(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_STRING))
    {
        return mc_value_makenull();
    }

    mcconfig_t* config = &vm->config;

    if(!config->fileio.read_file.read_file)
    {
        return mc_value_makenull();
    }

    const char* path = object_get_string(args[0]);

    char* contents = config->fileio.read_file.read_file(config->fileio.read_file.context, path);
    if(!contents)
    {
        return mc_value_makenull();
    }
    mcvalue_t res = mc_value_makestring(vm, contents);
    allocator_free(vm, contents);
    return res;
}

TMPSTATIC mcvalue_t cfn_tostring(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_STRING | ARCANE_OBJECT_NUMBER | ARCANE_OBJECT_BOOL | ARCANE_OBJECT_NULL | ARCANE_OBJECT_MAP | ARCANE_OBJECT_ARRAY))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    mcprintstate_t* buf = mc_printer_make(vm, NULL);
    if(!buf)
    {
        return mc_value_makenull();
    }
    mc_printer_printobject(arg, buf, false);
    if(mc_printer_failed(buf))
    {
        mc_printer_destroy(buf);
        return mc_value_makenull();
    }
    mcvalue_t res = mc_value_makestring(vm, mc_printer_getstring(buf));
    mc_printer_destroy(buf);
    return res;
}

TMPSTATIC mcvalue_t cfn_tonum(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_STRING | ARCANE_OBJECT_NUMBER | ARCANE_OBJECT_BOOL | ARCANE_OBJECT_NULL))
    {
        return mc_value_makenull();
    }
    double result = 0;
    const char* string = "";
    if(object_is_numeric(args[0]))
    {
        result = object_get_number(args[0]);
    }
    else if(object_is_null(args[0]))
    {
        result = 0;
    }
    else if(args[0].type == ARCANE_OBJECT_STRING)
    {
        string = object_get_string(args[0]);
        char* end;
        errno = 0;
        result = strtod(string, &end);
        if(errno == ERANGE && (result <= -HUGE_VAL || result >= HUGE_VAL))
        {
            goto err;
        }
        if(errno && errno != ERANGE)
        {
            goto err;
        }
        int stringlen = object_get_string_length(args[0]);
        int parsedlen = end - string;
        if(stringlen != parsedlen)
        {
            goto err;
        }
    }
    else
    {
        goto err;
    }
    return mc_value_makenumber(result);
err:
    errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, "Cannot convert \"%s\" to number", string);
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_chr(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_NUMBER))
    {
        return mc_value_makenull();
    }

    double val = object_get_number(args[0]);

    char c = (char)val;
    char str[2] = { c, '\0' };
    return mc_value_makestring(vm, str);
}

TMPSTATIC mcvalue_t cfn_range(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    (void)data;
    for(int i = 0; i < argc; i++)
    {
        mcobjtype_t type = args[i].type;
        if(type != ARCANE_OBJECT_NUMBER)
        {
            const char* typestr = object_get_type_name(type);
            const char* expectedstr = object_get_type_name(ARCANE_OBJECT_NUMBER);
            errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, "Invalid argument %d passed to range, got %s instead of %s", i, typestr, expectedstr);
            return mc_value_makenull();
        }
    }

    int start = 0;
    int end = 0;
    int step = 1;

    if(argc == 1)
    {
        end = (int)object_get_number(args[0]);
    }
    else if(argc == 2)
    {
        start = (int)object_get_number(args[0]);
        end = (int)object_get_number(args[1]);
    }
    else if(argc == 3)
    {
        start = (int)object_get_number(args[0]);
        end = (int)object_get_number(args[1]);
        step = (int)object_get_number(args[2]);
    }
    else
    {
        errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, "Invalid number of arguments passed to range, got %d", argc);
        return mc_value_makenull();
    }

    if(step == 0)
    {
        errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, "range step cannot be 0");
        return mc_value_makenull();
    }

    mcvalue_t res = object_make_array(vm);
    if(object_is_null(res))
    {
        return mc_value_makenull();
    }
    for(int i = start; i < end; i += step)
    {
        mcvalue_t item = mc_value_makenumber(i);
        ok = object_add_array_value(res, item);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

TMPSTATIC mcvalue_t cfn_keys(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_MAP))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    mcvalue_t res = object_make_array(vm);
    if(object_is_null(res))
    {
        return mc_value_makenull();
    }
    int len = object_get_map_length(arg);
    for(int i = 0; i < len; i++)
    {
        mcvalue_t key = object_get_map_key_at(arg, i);
        ok = object_add_array_value(res, key);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

TMPSTATIC mcvalue_t cfn_values(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_MAP))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    mcvalue_t res = object_make_array(vm);
    if(object_is_null(res))
    {
        return mc_value_makenull();
    }
    int len = object_get_map_length(arg);
    for(int i = 0; i < len; i++)
    {
        mcvalue_t key = object_get_map_value_at(arg, i);
        ok = object_add_array_value(res, key);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

TMPSTATIC mcvalue_t cfn_copy(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    return object_copy(vm, args[0]);
}

TMPSTATIC mcvalue_t cfn_copydeep(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    return object_deep_copy(vm, vm->mem, args[0]);
}

TMPSTATIC mcvalue_t cfn_concat(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ARRAY | ARCANE_OBJECT_STRING, ARCANE_OBJECT_ARRAY | ARCANE_OBJECT_STRING))
    {
        return mc_value_makenull();
    }
    mcobjtype_t type = args[0].type;
    mcobjtype_t itemtype = args[1].type;
    if(type == ARCANE_OBJECT_ARRAY)
    {
        if(itemtype != ARCANE_OBJECT_ARRAY)
        {
            const char* itemtypestr = object_get_type_name(itemtype);
            errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, "Invalid argument 2 passed to concat, got %s", itemtypestr);
            return mc_value_makenull();
        }
        for(int i = 0; i < object_get_array_length(args[1]); i++)
        {
            mcvalue_t item = object_get_array_value_at(args[1], i);
            ok = object_add_array_value(args[0], item);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return mc_value_makenumber(object_get_array_length(args[0]));
    }
    if(type == ARCANE_OBJECT_STRING)
    {
        if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_STRING, ARCANE_OBJECT_STRING))
        {
            return mc_value_makenull();
        }
        const char* leftval = object_get_string(args[0]);
        int leftlen = object_get_string_length(args[0]);

        const char* rightval = object_get_string(args[1]);
        int rightlen = object_get_string_length(args[1]);

        mcvalue_t res = mc_value_makestrcapacity(vm, leftlen + rightlen);
        if(object_is_null(res))
        {
            return mc_value_makenull();
        }

        ok = mc_value_strappend(res, leftval, leftlen);
        if(!ok)
        {
            return mc_value_makenull();
        }
        ok = mc_value_strappend(res, rightval, rightlen);
        if(!ok)
        {
            return mc_value_makenull();
        }

        return res;
    }
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_remove(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ARRAY, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }

    int ix = -1;
    for(int i = 0; i < object_get_array_length(args[0]); i++)
    {
        mcvalue_t obj = object_get_array_value_at(args[0], i);
        if(object_equals(obj, args[1]))
        {
            ix = i;
            break;
        }
    }

    if(ix == -1)
    {
        return mc_value_makebool(false);
    }

    bool res = object_remove_array_value_at(args[0], ix);
    return mc_value_makebool(res);
}

TMPSTATIC mcvalue_t cfn_removeat(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ARRAY, ARCANE_OBJECT_NUMBER))
    {
        return mc_value_makenull();
    }

    mcobjtype_t type = args[0].type;
    int ix = (int)object_get_number(args[1]);

    switch(type)
    {
        case ARCANE_OBJECT_ARRAY:
        {
            bool res = object_remove_array_value_at(args[0], ix);
            return mc_value_makebool(res);
        }
        default:
            break;
    }

    return mc_value_makebool(true);
}

TMPSTATIC mcvalue_t cfn_error(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(argc == 1 && args[0].type == ARCANE_OBJECT_STRING)
    {
        return object_make_error(vm, object_get_string(args[0]));
    }
    return object_make_error(vm, "");
}

TMPSTATIC mcvalue_t cfn_crash(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(argc == 1 && args[0].type == ARCANE_OBJECT_STRING)
    {
        errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), object_get_string(args[0]));
    }
    else
    {
        errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "");
    }
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_assert(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_BOOL))
    {
        return mc_value_makenull();
    }

    if(!object_get_bool(args[0]))
    {
        errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, "assertion failed");
        return mc_value_makenull();
    }

    return mc_value_makebool(true);
}

TMPSTATIC mcvalue_t cfn_randseed(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_NUMBER))
    {
        return mc_value_makenull();
    }
    int seed = (int)object_get_number(args[0]);
    srand(seed);
    return mc_value_makebool(true);
}

TMPSTATIC mcvalue_t cfn_random(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    double res = (double)rand() / RAND_MAX;
    if(argc == 0)
    {
        return mc_value_makenumber(res);
    }
    if(argc == 2)
    {
        if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_NUMBER, ARCANE_OBJECT_NUMBER))
        {
            return mc_value_makenull();
        }
        double min = object_get_number(args[0]);
        double max = object_get_number(args[1]);
        if(min >= max)
        {
            errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, "max is bigger than min");
            return mc_value_makenull();
        }
        double range = max - min;
        res = min + (res * range);
        return mc_value_makenumber(res);
    }
    errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, "Invalid number or arguments");
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_slice(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_STRING | ARCANE_OBJECT_ARRAY, ARCANE_OBJECT_NUMBER))
    {
        return mc_value_makenull();
    }
    mcobjtype_t argtype = args[0].type;
    int index = (int)object_get_number(args[1]);
    if(argtype == ARCANE_OBJECT_ARRAY)
    {
        int len = object_get_array_length(args[0]);
        if(index < 0)
        {
            index = len + index;
            if(index < 0)
            {
                index = 0;
            }
        }
        mcvalue_t res = object_make_array_with_capacity(vm, len - index);
        if(object_is_null(res))
        {
            return mc_value_makenull();
        }
        for(int i = index; i < len; i++)
        {
            mcvalue_t item = object_get_array_value_at(args[0], i);
            ok = object_add_array_value(res, item);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return res;
    }
    if(argtype == ARCANE_OBJECT_STRING)
    {
        const char* str = object_get_string(args[0]);
        int len = object_get_string_length(args[0]);
        if(index < 0)
        {
            index = len + index;
            if(index < 0)
            {
                return mc_value_makestring(vm, "");
            }
        }
        if(index >= len)
        {
            return mc_value_makestring(vm, "");
        }
        int reslen = len - index;
        mcvalue_t res = mc_value_makestrcapacity(vm, reslen);
        if(object_is_null(res))
        {
            return mc_value_makenull();
        }

        char* resbuf = object_get_mutable_string(res);
        memset(resbuf, 0, reslen + 1);
        for(int i = index; i < len; i++)
        {
            char c = str[i];
            resbuf[i - index] = c;
        }
        object_set_string_length(res, reslen);
        return res;
    }
    const char* typestr = object_get_type_name(argtype);
    errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, "Invalid argument 0 passed to slice, got %s instead", typestr);
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_isstring(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == ARCANE_OBJECT_STRING);
}

TMPSTATIC mcvalue_t cfn_isarray(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == ARCANE_OBJECT_ARRAY);
}

TMPSTATIC mcvalue_t cfn_ismap(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == ARCANE_OBJECT_MAP);
}

TMPSTATIC mcvalue_t cfn_isnumber(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == ARCANE_OBJECT_NUMBER);
}

TMPSTATIC mcvalue_t cfn_isbool(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == ARCANE_OBJECT_BOOL);
}

TMPSTATIC mcvalue_t cfn_isnull(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(object_get_type(args[0]) == ARCANE_OBJECT_NULL);
}

TMPSTATIC mcvalue_t cfn_isfunction(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(object_get_type(args[0]) == ARCANE_OBJECT_FUNCTION);
}

TMPSTATIC mcvalue_t cfn_isexternal(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(object_get_type(args[0]) == ARCANE_OBJECT_EXTERNAL);
}

TMPSTATIC mcvalue_t cfn_iserror(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(object_get_type(args[0]) == ARCANE_OBJECT_ERROR);
}

TMPSTATIC mcvalue_t cfn_isnative_function(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(object_get_type(args[0]) == ARCANE_OBJECT_NATIVE_FUNCTION);
}

TMPSTATIC mcvalue_t cfn_sqrt(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = sqrt(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_pow(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_NUMBER, ARCANE_OBJECT_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg1 = object_get_number(args[0]);
    double arg2 = object_get_number(args[1]);
    double res = pow(arg1, arg2);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_sin(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = sin(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_cos(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = cos(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_tan(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = tan(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_log(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = log(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_ceil(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = ceil(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_floor(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = floor(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_abs(mcstate_t* vm, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(!CHECK_ARGS(vm, true, argc, args, ARCANE_OBJECT_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = fabs(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC bool mc_args_check(mcstate_t* vm, bool generateerror, int argc, mcvalue_t* args, int expectedargc, mcobjtype_t* expectedtypes)
{
    if(argc != expectedargc)
    {
        if(generateerror)
        {
            errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, "Invalid number or arguments, got %d instead of %d", argc, expectedargc);
        }
        return false;
    }

    for(int i = 0; i < argc; i++)
    {
        mcvalue_t arg = args[i];
        mcobjtype_t type = object_get_type(arg);
        mcobjtype_t expectedtype = expectedtypes[i];
        if(!(type & expectedtype))
        {
            if(generateerror)
            {
                const char* typestr = object_get_type_name(type);
                char* expectedtypestr = object_get_type_union_name(vm, expectedtype);
                if(!expectedtypestr)
                {
                    return false;
                }
                errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, "Invalid argument %d type, got %s, expected %s", i, typestr, expectedtypestr);
                allocator_free(vm, expectedtypestr);
            }
            return false;
        }
    }
    return true;
}

void code_to_string(uint8_t* code, mcastlocation_t* sourcepositions, size_t codesize, mcprintstate_t* res)
{
    bool ok;
    unsigned pos = 0;
    while(pos < codesize)
    {
        uint8_t op = code[pos];
        mcopdefinition_t* def = opcode_lookup(op);
        ARCANE_ASSERT(def);
        if(sourcepositions)
        {
            mcastlocation_t srcpos = sourcepositions[pos];
            mc_printer_appendf(res, "%d:%-4d\t%04d\t%s", srcpos.line, srcpos.column, pos, def->name);
        }
        else
        {
            mc_printer_appendf(res, "%04d %s", pos, def->name);
        }
        pos++;

        uint64_t operands[2];
        ok = code_read_operands(def, code + pos, operands);
        if(!ok)
        {
            return;
        }
        for(int i = 0; i < def->num_operands; i++)
        {
            if(op == OPCODE_NUMBER)
            {
                double val_double = arcane_uint64_to_double(operands[i]);
                mc_printer_appendf(res, " %1.17g", val_double);
            }
            else
            {
                mc_printer_appendf(res, " %llu", operands[i]);
            }
            pos += def->operand_widths[i];
        }
        mc_printer_append(res, "\n");
    }
}

bool code_read_operands(mcopdefinition_t* def, uint8_t* instr, uint64_t outoperands[2])
{
    int offset = 0;
    for(int i = 0; i < def->num_operands; i++)
    {
        int operandwidth = def->operand_widths[i];
        switch(operandwidth)
        {
            case 1:
            {
                outoperands[i] = instr[offset];
                break;
            }
            case 2:
            {
                uint64_t operand = 0;
                operand = operand | ((uint64_t)instr[offset] << 8);
                operand = operand | ((uint64_t)instr[offset + 1]);
                outoperands[i] = operand;
                break;
            }
            case 4:
            {
                uint64_t operand = 0;
                operand = operand | ((uint64_t)instr[offset + 0] << 24);
                operand = operand | ((uint64_t)instr[offset + 1] << 16);
                operand = operand | ((uint64_t)instr[offset + 2] << 8);
                operand = operand | ((uint64_t)instr[offset + 3]);
                outoperands[i] = operand;
                break;
            }
            case 8:
            {
                uint64_t operand = 0;
                operand = operand | ((uint64_t)instr[offset + 0] << 56);
                operand = operand | ((uint64_t)instr[offset + 1] << 48);
                operand = operand | ((uint64_t)instr[offset + 2] << 40);
                operand = operand | ((uint64_t)instr[offset + 3] << 32);
                operand = operand | ((uint64_t)instr[offset + 4] << 24);
                operand = operand | ((uint64_t)instr[offset + 5] << 16);
                operand = operand | ((uint64_t)instr[offset + 6] << 8);
                operand = operand | ((uint64_t)instr[offset + 7]);
                outoperands[i] = operand;
                break;
            }
            default:
            {
                ARCANE_ASSERT(false);
                return false;
            }
        }
        offset += operandwidth;
    }
    return true;
}


#ifdef COLLECTIONS_DEBUG
    #define COLLECTIONS_ASSERT(x) assert(x)
#else
    #define COLLECTIONS_ASSERT(x)
#endif

TMPSTATIC char* collections_strndup(mcstate_t* state, const char* string, size_t n)
{
    char* outputstring = (char*)allocator_malloc(state, n + 1);
    if(!outputstring)
    {
        return NULL;
    }
    outputstring[n] = '\0';
    memcpy(outputstring, string, n);
    return outputstring;
}

TMPSTATIC char* collections_strdup(mcstate_t* state, const char* string)
{
    if(!string)
    {
        return NULL;
    }
    return collections_strndup(state, string, strlen(string));
}

TMPSTATIC unsigned long collections_hash(void* ptr, size_t len)
{
    /* djb2 */
    uint8_t* ptru8 = ptr;
    unsigned long hash = 5381;
    for(size_t i = 0; i < len; i++)
    {
        uint8_t val = ptru8[i];
        hash = ((hash << 5) + hash) + val;
    }
    return hash;
}

TMPSTATIC unsigned int upper_power_of_two(unsigned int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

void* allocator_malloc(mcstate_t* state, size_t size)
{
    return malloc(size);
}

void allocator_free(mcstate_t* state, void* ptr)
{
    free(ptr);
    return;
}

#define DICT_INVALID_IX UINT_MAX
#define DICT_INITIAL_SIZE 32

mcgenericdict_t* dict_make_(mcstate_t* state, dictitemcopyfn copy_fn, dictitemdestroyfn destroy_fn)
{
    bool ok;
    mcgenericdict_t* dict = (mcgenericdict_t*)allocator_malloc(state, sizeof(mcgenericdict_t));
    if(dict == NULL)
    {
        return NULL;
    }
    ok = dict_init(dict, state, DICT_INITIAL_SIZE, copy_fn, destroy_fn);
    if(!ok)
    {
        allocator_free(state, dict);
        return NULL;
    }
    dict->pstate = state;
    return dict;
}

void dict_destroy(mcgenericdict_t* dict)
{
    mcstate_t* state;
    if(!dict)
    {
        return;
    }
    state = dict->pstate;
    dict_deinit(dict, true);
    allocator_free(state, dict);
}

void dict_destroy_with_items(mcgenericdict_t* dict)
{
    if(!dict)
    {
        return;
    }

    if(dict->destroy_fn)
    {
        for(unsigned int i = 0; i < dict->count; i++)
        {
            dict->destroy_fn(dict->values[i]);
        }
    }

    dict_destroy(dict);
}

mcgenericdict_t* dict_copy_with_items(mcgenericdict_t* dict)
{
    bool ok;
    if(!dict->copy_fn || !dict->destroy_fn)
    {
        return false;
    }

    mcgenericdict_t* dictcopy = dict_make_(dict->pstate, dict->copy_fn, dict->destroy_fn);
    if(!dictcopy)
    {
        return NULL;
    }
    dictcopy->pstate = dict->pstate;
    for(int i = 0; i < dict_count(dict); i++)
    {
        const char* key = dict_get_key_at(dict, i);
        void* item = dict_get_value_at(dict, i);
        void* itemcopy = dictcopy->copy_fn(item);
        if(item && !itemcopy)
        {
            dict_destroy_with_items(dictcopy);
            return NULL;
        }
        ok = dict_set(dictcopy, key, itemcopy);
        if(!ok)
        {
            dictcopy->destroy_fn(itemcopy);
            dict_destroy_with_items(dictcopy);
            return NULL;
        }
    }
    return dictcopy;
}

bool dict_set(mcgenericdict_t* dict, const char* key, void* value)
{
    return dict_set_internal(dict, key, NULL, value);
}

void* dict_get(mcgenericdict_t* dict, const char* key)
{
    unsigned long hash = hash_string(key);
    bool found = false;
    unsigned long cellix = dict_get_cell_ix(dict, key, hash, &found);
    if(found == false)
    {
        return NULL;
    }
    unsigned int itemix = dict->cells[cellix];
    return dict->values[itemix];
}

void* dict_get_value_at(mcgenericdict_t* dict, unsigned int ix)
{
    if(ix >= dict->count)
    {
        return NULL;
    }
    return dict->values[ix];
}

const char* dict_get_key_at(mcgenericdict_t* dict, unsigned int ix)
{
    if(ix >= dict->count)
    {
        return NULL;
    }
    return dict->keys[ix];
}

int dict_count(mcgenericdict_t* dict)
{
    if(!dict)
    {
        return 0;
    }
    return dict->count;
}

bool dict_remove(mcgenericdict_t* dict, const char* key)
{
    unsigned long hash = hash_string(key);
    bool found = false;
    unsigned int cell = dict_get_cell_ix(dict, key, hash, &found);
    if(!found)
    {
        return false;
    }

    unsigned int itemix = dict->cells[cell];
    allocator_free(dict->pstate, dict->keys[itemix]);
    unsigned int lastitemix = dict->count - 1;
    if(itemix < lastitemix)
    {
        dict->keys[itemix] = dict->keys[lastitemix];
        dict->values[itemix] = dict->values[lastitemix];
        dict->cell_ixs[itemix] = dict->cell_ixs[lastitemix];
        dict->hashes[itemix] = dict->hashes[lastitemix];
        dict->cells[dict->cell_ixs[itemix]] = itemix;
    }
    dict->count--;

    unsigned int i = cell;
    unsigned int j = i;
    for(unsigned int x = 0; x < (dict->cell_capacity - 1); x++)
    {
        j = (j + 1) & (dict->cell_capacity - 1);
        if(dict->cells[j] == DICT_INVALID_IX)
        {
            break;
        }
        unsigned int k = (unsigned int)(dict->hashes[dict->cells[j]]) & (dict->cell_capacity - 1);
        if((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j)))
        {
            dict->cell_ixs[dict->cells[j]] = i;
            dict->cells[i] = dict->cells[j];
            i = j;
        }
    }
    dict->cells[i] = DICT_INVALID_IX;
    return true;
}

TMPSTATIC bool dict_init(mcgenericdict_t* dict, mcstate_t* state, unsigned int initialcapacity, dictitemcopyfn copy_fn, dictitemdestroyfn destroy_fn)
{
    dict->pstate = state;
    dict->cells = NULL;
    dict->keys = NULL;
    dict->values = NULL;
    dict->cell_ixs = NULL;
    dict->hashes = NULL;

    dict->count = 0;
    dict->cell_capacity = initialcapacity;
    dict->item_capacity = (unsigned int)(initialcapacity * 0.7f);
    dict->copy_fn = copy_fn;
    dict->destroy_fn = destroy_fn;

    dict->cells = (unsigned int*)allocator_malloc(dict->pstate, dict->cell_capacity * sizeof(*dict->cells));
    dict->keys = (char**)allocator_malloc(dict->pstate, dict->item_capacity * sizeof(*dict->keys));
    dict->values = (void**)allocator_malloc(dict->pstate, dict->item_capacity * sizeof(*dict->values));
    dict->cell_ixs = (unsigned int*)allocator_malloc(dict->pstate, dict->item_capacity * sizeof(*dict->cell_ixs));
    dict->hashes = (long unsigned int*)allocator_malloc(dict->pstate, dict->item_capacity * sizeof(*dict->hashes));
    if(dict->cells == NULL || dict->keys == NULL || dict->values == NULL || dict->cell_ixs == NULL || dict->hashes == NULL)
    {
        goto error;
    }
    for(unsigned int i = 0; i < dict->cell_capacity; i++)
    {
        dict->cells[i] = DICT_INVALID_IX;
    }
    return true;
error:
    allocator_free(dict->pstate, dict->cells);
    allocator_free(dict->pstate, dict->keys);
    allocator_free(dict->pstate, dict->values);
    allocator_free(dict->pstate, dict->cell_ixs);
    allocator_free(dict->pstate, dict->hashes);
    return false;
}

TMPSTATIC void dict_deinit(mcgenericdict_t* dict, bool freekeys)
{
    if(freekeys)
    {
        for(unsigned int i = 0; i < dict->count; i++)
        {
            allocator_free(dict->pstate, dict->keys[i]);
        }
    }
    dict->count = 0;
    dict->item_capacity = 0;
    dict->cell_capacity = 0;

    allocator_free(dict->pstate, dict->cells);
    allocator_free(dict->pstate, dict->keys);
    allocator_free(dict->pstate, dict->values);
    allocator_free(dict->pstate, dict->cell_ixs);
    allocator_free(dict->pstate, dict->hashes);

    dict->cells = NULL;
    dict->keys = NULL;
    dict->values = NULL;
    dict->cell_ixs = NULL;
    dict->hashes = NULL;
}

TMPSTATIC unsigned int dict_get_cell_ix(mcgenericdict_t* dict, const char* key, unsigned long hash, bool* outfound)
{
    *outfound = false;
    unsigned int cellix = (unsigned int)hash & (dict->cell_capacity - 1);
    for(unsigned int i = 0; i < dict->cell_capacity; i++)
    {
        unsigned int ix = (cellix + i) & (dict->cell_capacity - 1);
        unsigned int cell = dict->cells[ix];
        if(cell == DICT_INVALID_IX)
        {
            return ix;
        }
        unsigned long hashtocheck = dict->hashes[cell];
        if(hash != hashtocheck)
        {
            continue;
        }
        const char* keytocheck = dict->keys[cell];
        if(strcmp(key, keytocheck) == 0)
        {
            *outfound = true;
            return ix;
        }
    }
    return DICT_INVALID_IX;
}

TMPSTATIC unsigned long hash_string(const char* str)
{
    /* djb2 */
    unsigned long hash = 5381;
    uint8_t c;
    while((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

TMPSEMISTATIC bool dict_grow_and_rehash(mcgenericdict_t* dict)
{
    bool ok;
    mcgenericdict_t newdict;
    ok = dict_init(&newdict, dict->pstate, dict->cell_capacity * 2, dict->copy_fn, dict->destroy_fn);
    if(!ok)
    {
        return false;
    }
    for(unsigned int i = 0; i < dict->count; i++)
    {
        char* key = dict->keys[i];
        void* value = dict->values[i];
        ok = dict_set_internal(&newdict, key, key, value);
        if(!ok)
        {
            dict_deinit(&newdict, false);
            return false;
        }
    }
    dict_deinit(dict, false);
    *dict = newdict;
    return true;
}

TMPSTATIC bool dict_set_internal(mcgenericdict_t* dict, const char* ckey, char* mkey, void* value)
{
    bool ok;
    unsigned long hash = hash_string(ckey);
    bool found = false;
    unsigned int cellix = dict_get_cell_ix(dict, ckey, hash, &found);
    if(found)
    {
        unsigned int itemix = dict->cells[cellix];
        dict->values[itemix] = value;
        return true;
    }
    if(dict->count >= dict->item_capacity)
    {
        ok = dict_grow_and_rehash(dict);
        if(!ok)
        {
            return false;
        }
        cellix = dict_get_cell_ix(dict, ckey, hash, &found);
    }

    if(mkey)
    {
        dict->keys[dict->count] = mkey;
    }
    else
    {
        char* keycopy = collections_strdup(dict->pstate, ckey);
        if(!keycopy)
        {
            return false;
        }
        dict->keys[dict->count] = keycopy;
    }
    dict->cells[cellix] = dict->count;
    dict->values[dict->count] = value;
    dict->cell_ixs[dict->count] = cellix;
    dict->hashes[dict->count] = hash;
    dict->count++;
    return true;
}

#define VALDICT_INVALID_IX UINT_MAX

mcvaldict_t* valdict_make_(mcstate_t* state, size_t key_size, size_t val_size)
{
    return valdict_make_with_capacity(state, DICT_INITIAL_SIZE, key_size, val_size);
}

mcvaldict_t* valdict_make_with_capacity(mcstate_t* state, unsigned int mincapacity, size_t key_size, size_t val_size)
{
    bool ok;
    unsigned int capacity = upper_power_of_two(mincapacity * 2);
    mcvaldict_t* dict = (mcvaldict_t*)allocator_malloc(state, sizeof(mcvaldict_t));
    if(!dict)
    {
        return NULL;
    }
    ok = valdict_init(dict, state, key_size, val_size, capacity);
    if(!ok)
    {
        allocator_free(state, dict);
        return NULL;
    }
    dict->pstate = state;
    return dict;
}

void valdict_destroy(mcvaldict_t* dict)
{
    mcstate_t* state;
    if(!dict)
    {
        return;
    }
    state = dict->pstate;
    valdict_deinit(dict);
    allocator_free(state, dict);
}

void valdict_set_hash_function(mcvaldict_t* dict, collectionshashfn hashfn)
{
    dict->_hash_key = hashfn;
}

void valdict_set_equals_function(mcvaldict_t* dict, collectionsequalsfn equalsfn)
{
    dict->_keys_equals = equalsfn;
}

bool valdict_set(mcvaldict_t* dict, void* key, void* value)
{
    bool ok;
    unsigned long hash = valdict_hash_key(dict, key);
    bool found = false;
    unsigned int cellix = valdict_get_cell_ix(dict, key, hash, &found);
    if(found)
    {
        unsigned int itemix = dict->cells[cellix];
        valdict_set_value_at(dict, itemix, value);
        return true;
    }
    if(dict->count >= dict->item_capacity)
    {
        ok = valdict_grow_and_rehash(dict);
        if(!ok)
        {
            return false;
        }
        cellix = valdict_get_cell_ix(dict, key, hash, &found);
    }
    unsigned int lastix = dict->count;
    dict->count++;
    dict->cells[cellix] = lastix;
    valdict_set_key_at(dict, lastix, key);
    valdict_set_value_at(dict, lastix, value);
    dict->cell_ixs[lastix] = cellix;
    dict->hashes[lastix] = hash;
    return true;
}

void* valdict_get(mcvaldict_t* dict, void* key)
{
    unsigned long hash = valdict_hash_key(dict, key);
    bool found = false;
    unsigned long cellix = valdict_get_cell_ix(dict, key, hash, &found);
    if(!found)
    {
        return NULL;
    }
    unsigned int itemix = dict->cells[cellix];
    return valdict_get_value_at(dict, itemix);
}

void* valdict_get_key_at(mcvaldict_t* dict, unsigned int ix)
{
    if(ix >= dict->count)
    {
        return NULL;
    }
    return (char*)dict->keys + (dict->key_size * ix);
}

void* valdict_get_value_at(mcvaldict_t* dict, unsigned int ix)
{
    if(ix >= dict->count)
    {
        return NULL;
    }
    return (char*)dict->values + (dict->val_size * ix);
}

unsigned int valdict_get_capacity(mcvaldict_t* dict)
{
    return dict->item_capacity;
}

bool valdict_set_value_at(mcvaldict_t* dict, unsigned int ix, void* value)
{
    if(ix >= dict->count)
    {
        return false;
    }
    size_t offset = ix * dict->val_size;
    memcpy((char*)dict->values + offset, value, dict->val_size);
    return true;
}

int valdict_count(mcvaldict_t* dict)
{
    if(!dict)
    {
        return 0;
    }
    return dict->count;
}

bool valdict_remove(mcvaldict_t* dict, void* key)
{
    unsigned long hash = valdict_hash_key(dict, key);
    bool found = false;
    unsigned int cell = valdict_get_cell_ix(dict, key, hash, &found);
    if(!found)
    {
        return false;
    }

    unsigned int itemix = dict->cells[cell];
    unsigned int lastitemix = dict->count - 1;
    if(itemix < lastitemix)
    {
        void* lastkey = valdict_get_key_at(dict, lastitemix);
        valdict_set_key_at(dict, itemix, lastkey);
        void* lastvalue = valdict_get_key_at(dict, lastitemix);
        valdict_set_value_at(dict, itemix, lastvalue);
        dict->cell_ixs[itemix] = dict->cell_ixs[lastitemix];
        dict->hashes[itemix] = dict->hashes[lastitemix];
        dict->cells[dict->cell_ixs[itemix]] = itemix;
    }
    dict->count--;

    unsigned int i = cell;
    unsigned int j = i;
    for(unsigned int x = 0; x < (dict->cell_capacity - 1); x++)
    {
        j = (j + 1) & (dict->cell_capacity - 1);
        if(dict->cells[j] == VALDICT_INVALID_IX)
        {
            break;
        }
        unsigned int k = (unsigned int)(dict->hashes[dict->cells[j]]) & (dict->cell_capacity - 1);
        if((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j)))
        {
            dict->cell_ixs[dict->cells[j]] = i;
            dict->cells[i] = dict->cells[j];
            i = j;
        }
    }
    dict->cells[i] = VALDICT_INVALID_IX;
    return true;
}

void valdict_clear(mcvaldict_t* dict)
{
    dict->count = 0;
    for(unsigned int i = 0; i < dict->cell_capacity; i++)
    {
        dict->cells[i] = VALDICT_INVALID_IX;
    }
}

TMPSTATIC bool valdict_init(mcvaldict_t* dict, mcstate_t* state, size_t key_size, size_t val_size, unsigned int initialcapacity)
{
    dict->pstate = state;
    dict->key_size = key_size;
    dict->val_size = val_size;
    dict->cells = NULL;
    dict->keys = NULL;
    dict->values = NULL;
    dict->cell_ixs = NULL;
    dict->hashes = NULL;

    dict->count = 0;
    dict->cell_capacity = initialcapacity;
    dict->item_capacity = (unsigned int)(initialcapacity * 0.7f);

    dict->_keys_equals = NULL;
    dict->_hash_key = NULL;

    dict->cells = (unsigned int*)allocator_malloc(dict->pstate, dict->cell_capacity * sizeof(*dict->cells));
    dict->keys = (void*)allocator_malloc(dict->pstate, dict->item_capacity * key_size);
    dict->values = (void*)allocator_malloc(dict->pstate, dict->item_capacity * val_size);
    dict->cell_ixs = (unsigned int*)allocator_malloc(dict->pstate, dict->item_capacity * sizeof(*dict->cell_ixs));
    dict->hashes = (long unsigned int*)allocator_malloc(dict->pstate, dict->item_capacity * sizeof(*dict->hashes));
    if(dict->cells == NULL || dict->keys == NULL || dict->values == NULL || dict->cell_ixs == NULL || dict->hashes == NULL)
    {
        goto error;
    }
    for(unsigned int i = 0; i < dict->cell_capacity; i++)
    {
        dict->cells[i] = VALDICT_INVALID_IX;
    }
    return true;
error:
    allocator_free(dict->pstate, dict->cells);
    allocator_free(dict->pstate, dict->keys);
    allocator_free(dict->pstate, dict->values);
    allocator_free(dict->pstate, dict->cell_ixs);
    allocator_free(dict->pstate, dict->hashes);
    return false;
}

TMPSTATIC void valdict_deinit(mcvaldict_t* dict)
{
    dict->key_size = 0;
    dict->val_size = 0;
    dict->count = 0;
    dict->item_capacity = 0;
    dict->cell_capacity = 0;

    allocator_free(dict->pstate, dict->cells);
    allocator_free(dict->pstate, dict->keys);
    allocator_free(dict->pstate, dict->values);
    allocator_free(dict->pstate, dict->cell_ixs);
    allocator_free(dict->pstate, dict->hashes);

    dict->cells = NULL;
    dict->keys = NULL;
    dict->values = NULL;
    dict->cell_ixs = NULL;
    dict->hashes = NULL;
}

TMPSTATIC unsigned int valdict_get_cell_ix(mcvaldict_t* dict, void* key, unsigned long hash, bool* outfound)
{
    bool areequal;
    unsigned int i;
    unsigned int ix;
    unsigned int cell;
    unsigned int cellix;
    unsigned long hashtocheck;
    void* keytocheck;
    *outfound = false;
    cellix = (unsigned int)hash & (dict->cell_capacity - 1);
    for(i = 0; i < dict->cell_capacity; i++)
    {
        ix = (cellix + i) & (dict->cell_capacity - 1);
        cell = dict->cells[ix];
        if(cell == VALDICT_INVALID_IX)
        {
            return ix;
        }
        hashtocheck = dict->hashes[cell];
        if(hash != hashtocheck)
        {
            continue;
        }

        keytocheck = valdict_get_key_at(dict, cell);
        areequal = valdict_keys_are_equal(dict, key, keytocheck);
        if(areequal)
        {
            *outfound = true;
            return ix;
        }
    }
    return VALDICT_INVALID_IX;
}

TMPSTATIC bool valdict_grow_and_rehash(mcvaldict_t* dict)
{
    bool ok;
    mcvaldict_t newdict;
    unsigned newcapacity = dict->cell_capacity == 0 ? DICT_INITIAL_SIZE : dict->cell_capacity * 2;
    ok = valdict_init(&newdict, dict->pstate, dict->key_size, dict->val_size, newcapacity);
    if(!ok)
    {
        return false;
    }
    newdict._keys_equals = dict->_keys_equals;
    newdict._hash_key = dict->_hash_key;
    for(unsigned int i = 0; i < dict->count; i++)
    {
        char* key = valdict_get_key_at(dict, i);
        void* value = valdict_get_value_at(dict, i);
        ok = valdict_set(&newdict, key, value);
        if(!ok)
        {
            valdict_deinit(&newdict);
            return false;
        }
    }
    valdict_deinit(dict);
    *dict = newdict;
    return true;
}

TMPSTATIC bool valdict_set_key_at(mcvaldict_t* dict, unsigned int ix, void* key)
{
    if(ix >= dict->count)
    {
        return false;
    }
    size_t offset = ix * dict->key_size;
    memcpy((char*)dict->keys + offset, key, dict->key_size);
    return true;
}

TMPSTATIC bool valdict_keys_are_equal(mcvaldict_t* dict, void* a, void* b)
{
    if(dict->_keys_equals)
    {
        return dict->_keys_equals(a, b);
    }
    return memcmp(a, b, dict->key_size) == 0;
}

TMPSTATIC unsigned long valdict_hash_key(mcvaldict_t* dict, void* key)
{
    if(dict->_hash_key)
    {
        return dict->_hash_key(key);
    }
    return collections_hash(key, dict->key_size);
}

mcbasicarray_t* array_make(mcstate_t* state, size_t element_size)
{
    return array_make_with_capacity(state, 32, element_size);
}

mcbasicarray_t* array_make_with_capacity(mcstate_t* state, unsigned int capacity, size_t element_size)
{
    bool ok;
    mcbasicarray_t* arr = (mcbasicarray_t*)allocator_malloc(state, sizeof(mcbasicarray_t));
    if(!arr)
    {
        return NULL;
    }

    ok = array_init_with_capacity(arr, state, capacity, element_size);
    if(!ok)
    {
        allocator_free(state, arr);
        return NULL;
    }
    arr->pstate = state;
    return arr;
}

void array_destroy(mcbasicarray_t* arr)
{
    mcstate_t* state;
    if(!arr)
    {
        return;
    }
    state = arr->pstate;
    array_deinit(arr);
    allocator_free(state, arr);
}

void array_destroy_with_items_(mcbasicarray_t* arr, arrayitemdeinitfn deinit_fn)
{
    for(int i = 0; i < array_count(arr); i++)
    {
        void* item = array_get(arr, i);
        deinit_fn(item);
    }
    array_destroy(arr);
}

mcbasicarray_t* array_copy(mcbasicarray_t* arr)
{
    mcbasicarray_t* copy = (mcbasicarray_t*)allocator_malloc(arr->pstate, sizeof(mcbasicarray_t));
    if(!copy)
    {
        return NULL;
    }
    copy->pstate = arr->pstate;
    copy->capacity = arr->capacity;
    copy->count = arr->count;
    copy->element_size = arr->element_size;
    copy->lock_capacity = arr->lock_capacity;
    if(arr->data_allocated)
    {
        copy->data_allocated = (unsigned char*)allocator_malloc(arr->pstate, arr->capacity * arr->element_size);
        if(!copy->data_allocated)
        {
            allocator_free(arr->pstate, copy);
            return NULL;
        }
        copy->data = copy->data_allocated;
        memcpy(copy->data_allocated, arr->data, arr->capacity * arr->element_size);
    }
    else
    {
        copy->data_allocated = NULL;
        copy->data = NULL;
    }

    return copy;
}

bool array_add(mcbasicarray_t* arr, void* value)
{
    if(arr->count >= arr->capacity)
    {
        COLLECTIONS_ASSERT(!arr->lock_capacity);
        if(arr->lock_capacity)
        {
            return false;
        }
        unsigned int newcapacity = arr->capacity > 0 ? arr->capacity * 2 : 1;
        unsigned char* newdata = (unsigned char*)allocator_malloc(arr->pstate, newcapacity * arr->element_size);
        if(!newdata)
        {
            return false;
        }
        memcpy(newdata, arr->data, arr->count * arr->element_size);
        allocator_free(arr->pstate, arr->data_allocated);
        arr->data_allocated = newdata;
        arr->data = arr->data_allocated;
        arr->capacity = newcapacity;
    }
    if(value)
    {
        memcpy(arr->data + (arr->count * arr->element_size), value, arr->element_size);
    }
    arr->count++;
    return true;
}

bool array_addn(mcbasicarray_t* arr, void* values, int n)
{
    bool ok;
    for(int i = 0; i < n; i++)
    {
        uint8_t* value = NULL;
        if(values)
        {
            value = (uint8_t*)values + (i * arr->element_size);
        }
        ok = array_add(arr, value);
        if(!ok)
        {
            return false;
        }
    }
    return true;
}

bool array_add_array(mcbasicarray_t* dest, mcbasicarray_t* source)
{
    bool ok;
    COLLECTIONS_ASSERT(dest->element_size == source->element_size);
    if(dest->element_size != source->element_size)
    {
        return false;
    }
    int destbeforecount = array_count(dest);
    for(int i = 0; i < array_count(source); i++)
    {
        void* item = array_get(source, i);
        ok = array_add(dest, item);
        if(!ok)
        {
            dest->count = destbeforecount;
            return false;
        }
    }
    return true;
}

bool array_push(mcbasicarray_t* arr, void* value)
{
    return array_add(arr, value);
}

bool array_pop(mcbasicarray_t* arr, void* outvalue)
{
    if(arr->count <= 0)
    {
        return false;
    }
    if(outvalue)
    {
        void* res = array_get(arr, arr->count - 1);
        memcpy(outvalue, res, arr->element_size);
    }
    array_remove_at(arr, arr->count - 1);
    return true;
}

void* array_top(mcbasicarray_t* arr)
{
    if(arr->count <= 0)
    {
        return NULL;
    }
    return array_get(arr, arr->count - 1);
}

bool array_set(mcbasicarray_t* arr, unsigned int ix, void* value)
{
    if(ix >= arr->count)
    {
        COLLECTIONS_ASSERT(false);
        return false;
    }
    size_t offset = ix * arr->element_size;
    memmove(arr->data + offset, value, arr->element_size);
    return true;
}

bool array_setn(mcbasicarray_t* arr, unsigned int ix, void* values, int n)
{
    bool ok;
    for(int i = 0; i < n; i++)
    {
        int destix = ix + i;
        unsigned char* value = (unsigned char*)values + (i * arr->element_size);
        if(destix < array_count(arr))
        {
            ok = array_set(arr, destix, value);
            if(!ok)
            {
                return false;
            }
        }
        else
        {
            ok = array_add(arr, value);
            if(!ok)
            {
                return false;
            }
        }
    }
    return true;
}

void* array_get(mcbasicarray_t* arr, unsigned int ix)
{
    if(ix >= arr->count)
    {
        COLLECTIONS_ASSERT(false);
        return NULL;
    }
    size_t offset = ix * arr->element_size;
    return arr->data + offset;
}

void* array_get_const(mcbasicarray_t* arr, unsigned int ix)
{
    if(ix >= arr->count)
    {
        COLLECTIONS_ASSERT(false);
        return NULL;
    }
    size_t offset = ix * arr->element_size;
    return arr->data + offset;
}

void* array_get_last(mcbasicarray_t* arr)
{
    if(arr->count <= 0)
    {
        return NULL;
    }
    return array_get(arr, arr->count - 1);
}

int array_count(mcbasicarray_t* arr)
{
    if(!arr)
    {
        return 0;
    }
    return arr->count;
}

unsigned int array_get_capacity(mcbasicarray_t* arr)
{
    return arr->capacity;
}

bool array_remove_at(mcbasicarray_t* arr, unsigned int ix)
{
    if(ix >= arr->count)
    {
        return false;
    }
    if(ix == 0)
    {
        arr->data += arr->element_size;
        arr->capacity--;
        arr->count--;
        return true;
    }
    if(ix == (arr->count - 1))
    {
        arr->count--;
        return true;
    }
    size_t tomovebytes = (arr->count - 1 - ix) * arr->element_size;
    void* dest = arr->data + (ix * arr->element_size);
    void* src = arr->data + ((ix + 1) * arr->element_size);
    memmove(dest, src, tomovebytes);
    arr->count--;
    return true;
}

bool array_remove_item(mcbasicarray_t* arr, void* ptr)
{
    int ix = array_get_index(arr, ptr);
    if(ix < 0)
    {
        return false;
    }
    return array_remove_at(arr, ix);
}

void array_clear(mcbasicarray_t* arr)
{
    arr->count = 0;
}

void array_clear_and_deinit_items_(mcbasicarray_t* arr, arrayitemdeinitfn deinit_fn)
{
    for(int i = 0; i < array_count(arr); i++)
    {
        void* item = array_get(arr, i);
        deinit_fn(item);
    }
    arr->count = 0;
}

void array_lock_capacity(mcbasicarray_t* arr)
{
    arr->lock_capacity = true;
}

int array_get_index(mcbasicarray_t* arr, void* ptr)
{
    for(int i = 0; i < array_count(arr); i++)
    {
        if(array_get_const(arr, i) == ptr)
        {
            return i;
        }
    }
    return -1;
}

bool array_contains(mcbasicarray_t* arr, void* ptr)
{
    return array_get_index(arr, ptr) >= 0;
}

void* array_data(mcbasicarray_t* arr)
{
    return arr->data;
}

void* array_const_data(mcbasicarray_t* arr)
{
    return arr->data;
}

void array_orphan_data(mcbasicarray_t* arr)
{
    array_init_with_capacity(arr, arr->pstate, 0, arr->element_size);
}

bool array_reverse(mcbasicarray_t* arr)
{
    int count = array_count(arr);
    if(count < 2)
    {
        return true;
    }
    void* temp = (void*)allocator_malloc(arr->pstate, arr->element_size);
    if(!temp)
    {
        return false;
    }
    for(int aix = 0; aix < (count / 2); aix++)
    {
        int bix = count - aix - 1;
        void* a = array_get(arr, aix);
        void* b = array_get(arr, bix);
        memcpy(temp, a, arr->element_size);
        /* no need for check because it will be within range */
        array_set(arr, aix, b);
        array_set(arr, bix, temp);
    }
    allocator_free(arr->pstate, temp);
    return true;
}

TMPSTATIC bool array_init_with_capacity(mcbasicarray_t* arr, mcstate_t* state, unsigned int capacity, size_t element_size)
{
    arr->pstate = state;
    if(capacity > 0)
    {
        arr->data_allocated = (unsigned char*)allocator_malloc(arr->pstate, capacity * element_size);
        arr->data = arr->data_allocated;
        if(!arr->data_allocated)
        {
            return false;
        }
    }
    else
    {
        arr->data_allocated = NULL;
        arr->data = NULL;
    }
    arr->capacity = capacity;
    arr->count = 0;
    arr->element_size = element_size;
    arr->lock_capacity = false;
    return true;
}

TMPSTATIC void array_deinit(mcbasicarray_t* arr)
{
    allocator_free(arr->pstate, arr->data_allocated);
}

mcptrarray_t* ptrarray_make(mcstate_t* state)
{
    return ptrarray_make_with_capacity(state, 0);
}

mcptrarray_t* ptrarray_make_with_capacity(mcstate_t* state, unsigned int capacity)
{
    bool ok;
    mcptrarray_t* ptrarr = (mcptrarray_t*)allocator_malloc(state, sizeof(mcptrarray_t));
    if(!ptrarr)
    {
        return NULL;
    }
    ptrarr->pstate = state;
    ok = array_init_with_capacity(&ptrarr->arr, state, capacity, sizeof(void*));
    if(!ok)
    {
        allocator_free(state, ptrarr);
        return NULL;
    }
    return ptrarr;
}

void ptrarray_destroy(mcptrarray_t* arr)
{
    if(!arr)
    {
        return;
    }
    array_deinit(&arr->arr);
    allocator_free(arr->pstate, arr);
}

void ptrarray_destroy_with_items_(mcptrarray_t* arr, ptrarrayitemdestroyfn destroy_fn)
{
    /* todo: destroy and copy in make fn */
    if(arr == NULL)
    {
        return;
    }
    if(destroy_fn)
    {
        ptrarray_clear_and_destroy_items_(arr, destroy_fn);
    }
    ptrarray_destroy(arr);
}

mcptrarray_t* ptrarray_copy(mcptrarray_t* arr)
{
    bool ok;
    mcptrarray_t* arrcopy = ptrarray_make_with_capacity(arr->pstate, arr->arr.capacity);
    if(!arrcopy)
    {
        return NULL;
    }
    for(int i = 0; i < ptrarray_count(arr); i++)
    {
        void* item = ptrarray_get(arr, i);
        ok = ptrarray_push(arrcopy, item);
        if(!ok)
        {
            ptrarray_destroy(arrcopy);
            return NULL;
        }
    }
    return arrcopy;
}

mcptrarray_t* ptrarray_copy_with_items_(mcptrarray_t* arr, ptrarrayitemcopyfn copy_fn, ptrarrayitemdestroyfn destroy_fn)
{
    bool ok;
    mcptrarray_t* arrcopy = ptrarray_make_with_capacity(arr->pstate, arr->arr.capacity);
    if(!arrcopy)
    {
        return NULL;
    }
    for(int i = 0; i < ptrarray_count(arr); i++)
    {
        void* item = ptrarray_get(arr, i);
        void* itemcopy = copy_fn(item);
        if(item && !itemcopy)
        {
            goto err;
        }
        ok = ptrarray_push(arrcopy, itemcopy);
        if(!ok)
        {
            goto err;
        }
    }
    return arrcopy;
err:
    ptrarray_destroy_with_items_(arrcopy, destroy_fn);
    return NULL;
}

bool ptrarray_push(mcptrarray_t* arr, void* ptr)
{
    return array_add(&arr->arr, &ptr);
}

void* ptrarray_get(mcptrarray_t* arr, unsigned int ix)
{
    void* res = array_get(&arr->arr, ix);
    if(!res)
    {
        return NULL;
    }
    return *(void**)res;
}

void* ptrarray_pop(mcptrarray_t* arr)
{
    int ix = ptrarray_count(arr) - 1;
    void* res = ptrarray_get(arr, ix);
    ptrarray_remove_at(arr, ix);
    return res;
}

void* ptrarray_top(mcptrarray_t* arr)
{
    int count = ptrarray_count(arr);
    if(count == 0)
    {
        return NULL;
    }
    return ptrarray_get(arr, count - 1);
}

int ptrarray_count(mcptrarray_t* arr)
{
    if(!arr)
    {
        return 0;
    }
    return array_count(&arr->arr);
}

bool ptrarray_remove_at(mcptrarray_t* arr, unsigned int ix)
{
    return array_remove_at(&arr->arr, ix);
}

void ptrarray_clear(mcptrarray_t* arr)
{
    array_clear(&arr->arr);
}

void ptrarray_clear_and_destroy_items_(mcptrarray_t* arr, ptrarrayitemdestroyfn destroy_fn)
{
    for(int i = 0; i < ptrarray_count(arr); i++)
    {
        void* item = ptrarray_get(arr, i);
        destroy_fn(item);
    }
    ptrarray_clear(arr);
}

void ptrarray_lock_capacity(mcptrarray_t* arr)
{
    array_lock_capacity(&arr->arr);
}

int ptrarray_get_index(mcptrarray_t* arr, void* ptr)
{
    for(int i = 0; i < ptrarray_count(arr); i++)
    {
        if(ptrarray_get(arr, i) == ptr)
        {
            return i;
        }
    }
    return -1;
}

bool ptrarray_contains(mcptrarray_t* arr, void* item)
{
    return ptrarray_get_index(arr, item) >= 0;
}

void* ptrarray_get_addr(mcptrarray_t* arr, unsigned int ix)
{
    void* res = array_get(&arr->arr, ix);
    if(res == NULL)
    {
        return NULL;
    }
    return res;
}

void* ptrarray_data(mcptrarray_t* arr)
{
    return array_data(&arr->arr);
}

void ptrarray_reverse(mcptrarray_t* arr)
{
    array_reverse(&arr->arr);
}

mcprintstate_t* mc_printer_make(mcstate_t* state, FILE* ofh)
{
    return mc_printer_make_with_capacity(state, 1, ofh);
}

bool mc_printer_init(mcprintstate_t* pr, mcstate_t* state, size_t capacity, FILE* ofh, bool onstack)
{
    memset(pr, 0, sizeof(mcprintstate_t));
    pr->pstate = state;
    pr->failed = false;
    pr->destfile = ofh;
    pr->data = NULL;
    pr->capacity = 0;
    pr->len = 0;
    pr->onstack = onstack;
    if(ofh == NULL)
    {
        pr->data = (char*)allocator_malloc(state, capacity);
        if(pr->data == NULL)
        {
            allocator_free(state, pr);
            return false;
        }
        pr->capacity = capacity;
        pr->len = 0;
        pr->data[0] = '\0';
    }
    return true;
}

mcprintstate_t* mc_printer_make_with_capacity(mcstate_t* state, unsigned int capacity, FILE* ofh)
{
    mcprintstate_t* pr;
    pr = (mcprintstate_t*)allocator_malloc(state, sizeof(mcprintstate_t));
    if(pr == NULL)
    {
        return NULL;
    }
    if(!mc_printer_init(pr, state, capacity, ofh, false))
    {
        return NULL;
    }
    return pr;
}

void mc_printer_destroy(mcprintstate_t* pr)
{
    if(pr == NULL)
    {
        return;
    }
    allocator_free(pr->pstate, pr->data);
    if(!pr->onstack)
    {
        allocator_free(pr->pstate, pr);
    }
}

void mc_printer_clear(mcprintstate_t* pr)
{
    if(pr->destfile == NULL)
    {
        if(pr->failed)
        {
            return;
        }
        pr->len = 0;
        pr->data[0] = '\0';
    }
}

bool mc_printer_appendlen(mcprintstate_t* pr, const char* str, size_t len)
{
    bool ok;
    size_t needcap;
    if(pr->failed)
    {
        return false;
    }
    if(len == 0)
    {
        return true;
    }
    if(pr->destfile != NULL)
    {
        fwrite(str, sizeof(char), len, pr->destfile);
        fflush(pr->destfile);
        return true;
    }
    needcap = pr->len + len + 1;
    if(needcap > pr->capacity)
    {
        ok = mc_printer_grow(pr, needcap * 2);
        if(!ok)
        {
            return false;
        }
    }
    memcpy(pr->data + pr->len, str, len);
    pr->len = pr->len + len;
    pr->data[pr->len] = '\0';
    return true;
}

bool mc_printer_append(mcprintstate_t* pr, const char* str)
{
    return mc_printer_appendlen(pr, str, strlen(str));
}

bool mc_printer_appendf(mcprintstate_t* pr, const char* fmt, ...)
{
    bool ok;
    int resz;
    int needsz;
    size_t needcap;
    va_list va;
    (void)resz;
    if(pr->failed)
    {
        return false;
    }
    if(pr->destfile != NULL)
    {
        va_start(va, fmt);
        vfprintf(pr->destfile, fmt, va);
        fflush(pr->destfile);
        va_end(va);
        return true;
    }
    va_start(va, fmt);
    needsz = vsnprintf(NULL, 0, fmt, va);
    va_end(va);
    if(needsz == 0)
    {
        return true;
    }
    needcap = pr->len + needsz + 1;
    if(needcap > pr->capacity)
    {
        ok = mc_printer_grow(pr, needcap * 2);
        if(!ok)
        {
            return false;
        }
    }
    va_start(va, fmt);
    resz = vsprintf(pr->data + pr->len, fmt, va);
    va_end(va);
    if(resz != needsz)
    {
        return false;
    }
    pr->len = pr->len + needsz;
    pr->data[pr->len] = '\0';
    return true;
}

const char* mc_printer_getstring(mcprintstate_t* pr)
{
    if(pr->failed)
    {
        return NULL;
    }
    if(pr->destfile != NULL)
    {
        return NULL;
    }
    return pr->data;
}

size_t mc_printer_getlength(mcprintstate_t* pr)
{
    if(pr->failed)
    {
        return 0;
    }
    if(pr->destfile != NULL)
    {
        return 0;
    }
    return pr->len;
}

char* mc_printer_get_string_and_destroy(mcprintstate_t* pr)
{
    char* res;
    if(pr->failed)
    {
        mc_printer_destroy(pr);
        return NULL;
    }
    if(pr->destfile != NULL)
    {
        return NULL;
    }
    res = pr->data;
    pr->data = NULL;
    mc_printer_destroy(pr);
    return res;
}

bool mc_printer_failed(mcprintstate_t* pr)
{
    return pr->failed;
}

TMPSTATIC bool mc_printer_grow(mcprintstate_t* pr, size_t newcapacity)
{
    char* ndata;
    if(pr->destfile != NULL)
    {
        return true;
    }
    ndata = (char*)allocator_malloc(pr->pstate, newcapacity);
    if(ndata == NULL)
    {
        pr->failed = true;
        return false;
    }
    memcpy(ndata, pr->data, pr->len);
    ndata[pr->len] = '\0';
    allocator_free(pr->pstate, pr->data);
    pr->data = ndata;
    pr->capacity = newcapacity;
    return true;
}

mcptrarray_t* kg_split_string(mcstate_t* state, const char* str, const char* delimiter)
{
    bool ok;
    mcptrarray_t* res = ptrarray_make(state);
    char* rest = NULL;
    if(!str)
    {
        return res;
    }
    const char* linestart = str;
    const char* lineend = strstr(linestart, delimiter);
    while(lineend != NULL)
    {
        long len = lineend - linestart;
        char* line = collections_strndup(state, linestart, len);
        if(!line)
        {
            goto err;
        }
        ok = ptrarray_push(res, line);
        if(!ok)
        {
            allocator_free(state, line);
            goto err;
        }
        linestart = lineend + 1;
        lineend = strstr(linestart, delimiter);
    }
    rest = collections_strdup(state, linestart);
    if(!rest)
    {
        goto err;
    }
    ok = ptrarray_push(res, rest);
    if(!ok)
    {
        goto err;
    }
    return res;
err:
    allocator_free(state, rest);
    if(res)
    {
        for(int i = 0; i < ptrarray_count(res); i++)
        {
            char* line = ptrarray_get(res, i);
            allocator_free(state, line);
        }
    }
    ptrarray_destroy(res);
    return NULL;
}

char* kg_join(mcstate_t* state, mcptrarray_t* items, const char* with)
{
    mcprintstate_t* res = mc_printer_make(state, NULL);
    if(!res)
    {
        return NULL;
    }
    for(int i = 0; i < ptrarray_count(items); i++)
    {
        char* item = ptrarray_get(items, i);
        mc_printer_append(res, item);
        if(i < (ptrarray_count(items) - 1))
        {
            mc_printer_append(res, with);
        }
    }
    return mc_printer_get_string_and_destroy(res);
}

char* kg_canonicalise_path(mcstate_t* state, const char* path)
{
    if(!strchr(path, '/') || (!strstr(path, "/../") && !strstr(path, "./")))
    {
        return collections_strdup(state, path);
    }

    mcptrarray_t* split = kg_split_string(state, path, "/");
    if(!split)
    {
        return NULL;
    }

    for(int i = 0; i < ptrarray_count(split) - 1; i++)
    {
        char* item = ptrarray_get(split, i);
        char* nextitem = ptrarray_get(split, i + 1);
        if(kg_streq(item, "."))
        {
            allocator_free(state, item);
            ptrarray_remove_at(split, i);
            i = -1;
            continue;
        }
        if(kg_streq(nextitem, ".."))
        {
            allocator_free(state, item);
            allocator_free(state, nextitem);
            ptrarray_remove_at(split, i);
            ptrarray_remove_at(split, i);
            i = -1;
        }
    }

    char* joined = kg_join(state, split, "/");
    for(int i = 0; i < ptrarray_count(split); i++)
    {
        void* item = ptrarray_get(split, i);
        allocator_free(state, item);
    }
    ptrarray_destroy(split);
    return joined;
}

bool kg_is_path_absolute(const char* path)
{
    return path[0] == '/';
}

bool kg_streq(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
}


void errors_init(mcerrlist_t* errors)
{
    memset(errors, 0, sizeof(mcerrlist_t));
    errors->count = 0;
}

void errors_deinit(mcerrlist_t* errors)
{
    errors_clear(errors);
}

void errors_add_error(mcerrlist_t* errors, mcerrtype_t type, mcastlocation_t pos, const char* message)
{
    if(errors->count >= MC_CONF_ERROR_MAXERRORCOUNT)
    {
        return;
    }
    mcerror_t err;
    memset(&err, 0, sizeof(mcerror_t));
    err.type = type;
    int len = strlen(message);
    int tocopy = len;
    if(tocopy >= (MC_CONF_ERROR_MSGMAXLENGTH - 1))
    {
        tocopy = MC_CONF_ERROR_MSGMAXLENGTH - 1;
    }
    memcpy(err.message, message, tocopy);
    err.message[tocopy] = '\0';
    err.pos = pos;
    err.traceback = NULL;
    errors->errors[errors->count] = err;
    errors->count++;
}

void errors_add_errorf(mcerrlist_t* errors, mcerrtype_t type, mcastlocation_t pos, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int towrite = vsnprintf(NULL, 0, format, args);
    (void)towrite;
    va_end(args);
    va_start(args, format);
    char res[MC_CONF_ERROR_MSGMAXLENGTH];
    int written = vsnprintf(res, MC_CONF_ERROR_MSGMAXLENGTH, format, args);
    (void)written;
    ARCANE_ASSERT(towrite == written);
    va_end(args);
    errors_add_error(errors, type, pos, res);
}

void errors_clear(mcerrlist_t* errors)
{
    for(int i = 0; i < errors_get_count(errors); i++)
    {
        mcerror_t* error = errors_get(errors, i);
        if(error->traceback)
        {
            traceback_destroy(error->traceback);
        }
    }
    errors->count = 0;
}

int errors_get_count(mcerrlist_t* errors)
{
    return errors->count;
}

mcerror_t* errors_get(mcerrlist_t* errors, int ix)
{
    if(ix >= errors->count)
    {
        return NULL;
    }
    return &errors->errors[ix];
}

mcerror_t* errors_getc(mcerrlist_t* errors, int ix)
{
    if(ix >= errors->count)
    {
        return NULL;
    }
    return &errors->errors[ix];
}

const char* error_type_to_string(mcerrtype_t type)
{
    switch(type)
    {
        case ARCANE_ERROR_PARSING:
            return "PARSING";
        case ARCANE_ERROR_COMPILATION:
            return "COMPILATION";
        case ARCANE_ERROR_RUNTIME:
            return "RUNTIME";
        case ARCANE_ERROR_TIMEOUT:
            return "TIMEOUT";
        case ARCANE_ERROR_ALLOCATION:
            return "ALLOCATION";
        case ARCANE_ERROR_USER:
            return "USER";
        default:
            return "INVALID";
    }
}

mcerror_t* errors_get_last_error(mcerrlist_t* errors)
{
    if(errors->count <= 0)
    {
        return NULL;
    }
    return &errors->errors[errors->count - 1];
}

bool errors_has_errors(mcerrlist_t* errors)
{
    return errors_get_count(errors) > 0;
}


bool frame_init(mcvmframe_t* frame, mcvalue_t functionobj, int base_pointer)
{
    if(object_get_type(functionobj) != ARCANE_OBJECT_FUNCTION)
    {
        return false;
    }
    mcobjfuncscript_t* function = object_get_function(functionobj);
    frame->function = functionobj;
    frame->ip = 0;
    frame->base_pointer = base_pointer;
    frame->src_ip = 0;
    frame->bytecode = function->comp_result->bytecode;
    frame->src_positions = function->comp_result->src_positions;
    frame->bytecode_size = function->comp_result->count;
    frame->recover_ip = -1;
    frame->is_recovering = false;
    return true;
}

mcopcode_t frame_read_opcode(mcvmframe_t* frame)
{
    frame->src_ip = frame->ip;
    return frame_read_uint8(frame);
}

uint64_t frame_read_uint64(mcvmframe_t* frame)
{
    uint8_t* data = frame->bytecode + frame->ip;
    frame->ip += 8;
    uint64_t res = 0;
    res |= (uint64_t)data[7];
    res |= (uint64_t)data[6] << 8;
    res |= (uint64_t)data[5] << 16;
    res |= (uint64_t)data[4] << 24;
    res |= (uint64_t)data[3] << 32;
    res |= (uint64_t)data[2] << 40;
    res |= (uint64_t)data[1] << 48;
    res |= (uint64_t)data[0] << 56;
    return res;
}

uint16_t frame_read_uint16(mcvmframe_t* frame)
{
    uint8_t* data = frame->bytecode + frame->ip;
    frame->ip += 2;
    return (data[0] << 8) | data[1];
}

uint8_t frame_read_uint8(mcvmframe_t* frame)
{
    uint8_t* data = frame->bytecode + frame->ip;
    frame->ip++;
    return data[0];
}

mcastlocation_t frame_src_position(mcvmframe_t* frame)
{
    if(frame->src_positions)
    {
        return frame->src_positions[frame->src_ip];
    }
    return srcposinvalid;
}

mcgcmemory_t* gcmem_make(mcstate_t* state)
{
    mcgcmemory_t* mem = (mcgcmemory_t*)allocator_malloc(state, sizeof(mcgcmemory_t));
    if(!mem)
    {
        return NULL;
    }
    memset(mem, 0, sizeof(mcgcmemory_t));
    mem->pstate = state;
    mem->gcobjlist = ptrarray_make(state);
    if(!mem->gcobjlist)
    {
        goto error;
    }
    mem->gcobjlistback = ptrarray_make(state);
    if(!mem->gcobjlistback)
    {
        goto error;
    }
    mem->gcobjlistremains = array_make(state, sizeof(mcvalue_t));
    if(!mem->gcobjlistremains)
    {
        goto error;
    }
    mem->allocssincesweep = 0;
    mem->onlydatapool.count = 0;

    for(int i = 0; i < GCMEM_POOLS_NUM; i++)
    {
        mcgcobjdatapool_t* pool = &mem->mempools[i];
        mem->mempools[i].count = 0;
        memset(pool, 0, sizeof(mcgcobjdatapool_t));
    }

    return mem;
error:
    gcmem_destroy(mem);
    return NULL;
}

void gcmem_destroy(mcgcmemory_t* mem)
{
    if(!mem)
    {
        return;
    }

    array_destroy(mem->gcobjlistremains);
    ptrarray_destroy(mem->gcobjlistback);

    for(int i = 0; i < ptrarray_count(mem->gcobjlist); i++)
    {
        mcobjdata_t* obj = ptrarray_get(mem->gcobjlist, i);
        object_data_deinit(obj);
        allocator_free(mem->pstate, obj);
    }
    ptrarray_destroy(mem->gcobjlist);

    for(int i = 0; i < GCMEM_POOLS_NUM; i++)
    {
        mcgcobjdatapool_t* pool = &mem->mempools[i];
        for(int j = 0; j < pool->count; j++)
        {
            mcobjdata_t* data = pool->data[j];
            object_data_deinit(data);
            allocator_free(mem->pstate, data);
        }
        memset(pool, 0, sizeof(mcgcobjdatapool_t));
    }

    for(int i = 0; i < mem->onlydatapool.count; i++)
    {
        allocator_free(mem->pstate, mem->onlydatapool.data[i]);
    }

    allocator_free(mem->pstate, mem);
}

mcobjdata_t* gcmem_alloc_object_data(mcstate_t* vm)
{
    bool ok;
    mcobjdata_t* data = NULL;
    vm->mem->allocssincesweep++;
    if(vm->mem->onlydatapool.count > 0)
    {
        data = vm->mem->onlydatapool.data[vm->mem->onlydatapool.count - 1];
        vm->mem->onlydatapool.count--;
    }
    else
    {
        data = (mcobjdata_t*)allocator_malloc(vm, sizeof(mcobjdata_t));
        if(!data)
        {
            return NULL;
        }
    }
    memset(data, 0, sizeof(mcobjdata_t));
    data->pstate = vm;
    ARCANE_ASSERT(ptrarray_count(v->mem->gcobjlistback) >= ptrarray_count(vm->mem->gcobjlist));
    /*
    * we want to make sure that appending to gcobjlistback never fails in sweep
    * so this only reserves space there.
    */
    ok = ptrarray_push(vm->mem->gcobjlistback, data);
    if(!ok)
    {
        allocator_free(vm, data);
        return NULL;
    }
    ok = ptrarray_push(vm->mem->gcobjlist, data);
    if(!ok)
    {
        allocator_free(vm, data);
        return NULL;
    }
    data->mem = vm->mem;
    return data;
}

mcobjdata_t* gcmem_get_object_data_from_pool(mcstate_t* vm, mcobjtype_t type)
{
    bool ok;
    mcgcobjdatapool_t* pool = get_pool_for_type(vm->mem, type);
    if(!pool || pool->count <= 0)
    {
        return NULL;
    }
    mcobjdata_t* data = pool->data[pool->count - 1];

    ARCANE_ASSERT(ptrarray_count(vm->mem->gcobjlistback) >= ptrarray_count(vm->mem->gcobjlist));
    /*
    * we want to make sure that appending to gcobjlistback never fails in sweep
    * so this only reserves space there.
    */
    ok = ptrarray_push(vm->mem->gcobjlistback, data);
    if(!ok)
    {
        return NULL;
    }
    ok = ptrarray_push(vm->mem->gcobjlist, data);
    if(!ok)
    {
        return NULL;
    }

    pool->count--;

    return data;
}

void gc_unmark_all(mcgcmemory_t* mem)
{
    for(int i = 0; i < ptrarray_count(mem->gcobjlist); i++)
    {
        mcobjdata_t* data = ptrarray_get(mem->gcobjlist, i);
        data->gcmark = false;
    }
}

void gc_mark_objects(mcvalue_t* objects, int count)
{
    for(int i = 0; i < count; i++)
    {
        mcvalue_t obj = objects[i];
        gc_mark_object(obj);
    }
}

void gc_mark_object(mcvalue_t obj)
{
    if(!object_is_allocated(obj))
    {
        return;
    }
    mcobjdata_t* data = object_get_allocated_data(obj);
    if(data->gcmark)
    {
        return;
    }
    data->gcmark = true;
    switch(obj.type)
    {
        case ARCANE_OBJECT_MAP:
        {
            int len = object_get_map_length(obj);
            for(int i = 0; i < len; i++)
            {
                mcvalue_t key = object_get_map_key_at(obj, i);
                if(object_is_allocated(key))
                {
                    mcobjdata_t* keydata = object_get_allocated_data(key);
                    if(!keydata->gcmark)
                    {
                        gc_mark_object(key);
                    }
                }
                mcvalue_t val = object_get_map_value_at(obj, i);
                if(object_is_allocated(val))
                {
                    mcobjdata_t* valdata = object_get_allocated_data(val);
                    if(!valdata->gcmark)
                    {
                        gc_mark_object(val);
                    }
                }
            }
            break;
        }
        case ARCANE_OBJECT_ARRAY:
        {
            int len = object_get_array_length(obj);
            for(int i = 0; i < len; i++)
            {
                mcvalue_t val = object_get_array_value_at(obj, i);
                if(object_is_allocated(val))
                {
                    mcobjdata_t* valdata = object_get_allocated_data(val);
                    if(!valdata->gcmark)
                    {
                        gc_mark_object(val);
                    }
                }
            }
            break;
        }
        case ARCANE_OBJECT_FUNCTION:
        {
            mcobjfuncscript_t* function = object_get_function(obj);
            for(int i = 0; i < function->free_vals_count; i++)
            {
                mcvalue_t freeval = object_get_function_free_val(obj, i);
                gc_mark_object(freeval);
                if(object_is_allocated(freeval))
                {
                    mcobjdata_t* freevaldata = object_get_allocated_data(freeval);
                    if(!freevaldata->gcmark)
                    {
                        gc_mark_object(freeval);
                    }
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

void gc_sweep(mcgcmemory_t* mem)
{
    bool ok;
    gc_mark_objects(array_data(mem->gcobjlistremains), array_count(mem->gcobjlistremains));

    ARCANE_ASSERT(ptrarray_count(mem->gcobjlistback) >= ptrarray_count(mem->gcobjlist));

    ptrarray_clear(mem->gcobjlistback);
    for(int i = 0; i < ptrarray_count(mem->gcobjlist); i++)
    {
        mcobjdata_t* data = ptrarray_get(mem->gcobjlist, i);
        if(data->gcmark)
        {
            /*
            * this should never fail because gcobjlistback's size should be equal to objects
            */
            ok = ptrarray_push(mem->gcobjlistback, data);
            (void)ok;
            ARCANE_ASSERT(ok);
        }
        else
        {
            if(can_data_be_put_in_pool(mem, data))
            {
                mcgcobjdatapool_t* pool = get_pool_for_type(mem, data->type);
                pool->data[pool->count] = data;
                pool->count++;
            }
            else
            {
                object_data_deinit(data);
                if(mem->onlydatapool.count < GCMEM_POOL_SIZE)
                {
                    mem->onlydatapool.data[mem->onlydatapool.count] = data;
                    mem->onlydatapool.count++;
                }
                else
                {
                    allocator_free(mem->pstate, data);
                }
            }
        }
    }
    mcptrarray_t* objstemp = mem->gcobjlist;
    mem->gcobjlist = mem->gcobjlistback;
    mem->gcobjlistback = objstemp;
    mem->allocssincesweep = 0;
}

bool gc_disable_on_object(mcvalue_t obj)
{
    bool ok;
    if(!object_is_allocated(obj))
    {
        return false;
    }
    mcobjdata_t* data = object_get_allocated_data(obj);
    if(array_contains(data->mem->gcobjlistremains, &obj))
    {
        return false;
    }
    ok = array_add(data->mem->gcobjlistremains, &obj);
    return ok;
}

void gc_enable_on_object(mcvalue_t obj)
{
    if(!object_is_allocated(obj))
    {
        return;
    }
    mcobjdata_t* data = object_get_allocated_data(obj);
    array_remove_item(data->mem->gcobjlistremains, &obj);
}

int gc_should_sweep(mcgcmemory_t* mem)
{
    return mem->allocssincesweep > GCMEM_SWEEP_INTERVAL;
}

TMPSTATIC mcgcobjdatapool_t* get_pool_for_type(mcgcmemory_t* mem, mcobjtype_t type)
{
    switch(type)
    {
        case ARCANE_OBJECT_ARRAY:
            return &mem->mempools[0];
        case ARCANE_OBJECT_MAP:
            return &mem->mempools[1];
        case ARCANE_OBJECT_STRING:
            return &mem->mempools[2];
        default:
            return NULL;
    }
}

TMPSTATIC bool can_data_be_put_in_pool(mcgcmemory_t* mem, mcobjdata_t* data)
{
    mcvalue_t obj = mc_object_makedatafrom(data->type, data);
    /*
    * this is to ensure that large objects won't be kept in pool indefinitely
    */
    switch(data->type)
    {
        case ARCANE_OBJECT_ARRAY:
        {
            if(object_get_array_length(obj) > 1024)
            {
                return false;
            }
            break;
        }
        case ARCANE_OBJECT_MAP:
        {
            if(object_get_map_length(obj) > 1024)
            {
                return false;
            }
            break;
        }
        case ARCANE_OBJECT_STRING:
        {
            if(!data->string.is_allocated || data->string.capacity > 4096)
            {
                return false;
            }
            break;
        }
        default:
            break;
    }

    mcgcobjdatapool_t* pool = get_pool_for_type(mem, data->type);
    if(!pool || pool->count >= GCMEM_POOL_SIZE)
    {
        return false;
    }
    return true;
}

mcglobalstore_t* global_store_make(mcstate_t* state)
{
    bool ok;
    
    mcglobalstore_t* store = (mcglobalstore_t*)allocator_malloc(state, sizeof(mcglobalstore_t));
    if(!store)
    {
        return NULL;
    }
    memset(store, 0, sizeof(mcglobalstore_t));
    store->pstate = state;
    store->symbols = dict_make_(state, (dictitemcopyfn)symbol_copy, (dictitemdestroyfn)symbol_destroy);
    if(!store->symbols)
    {
        goto err;
    }
    store->objects = array_make(state, sizeof(mcvalue_t));
    if(!store->objects)
    {
        goto err;
    }

    if(state->mem)
    {
        for(int i = 0; i < builtins_count(); i++)
        {
            const char* name = builtins_get_name(i);
            mcvalue_t builtin = object_make_native_function(state, name, builtins_get_fn(i), NULL, 0);
            if(object_is_null(builtin))
            {
                goto err;
            }
            ok = global_store_set(store, name, builtin);
            if(!ok)
            {
                goto err;
            }
        }
    }

    return store;
err:
    global_store_destroy(store);
    return NULL;
}

void global_store_destroy(mcglobalstore_t* store)
{
    if(!store)
    {
        return;
    }
    dict_destroy_with_items(store->symbols);
    array_destroy(store->objects);
    allocator_free(store->pstate, store);
}

mcastsymbol_t* global_store_get_symbol(mcglobalstore_t* store, const char* name)
{
    return dict_get(store->symbols, name);
}


bool global_store_set(mcglobalstore_t* store, const char* name, mcvalue_t object)
{
    bool ok;
    mcastsymbol_t* existingsymbol = global_store_get_symbol(store, name);
    if(existingsymbol)
    {
        ok = array_set(store->objects, existingsymbol->index, &object);
        return ok;
    }
    int ix = array_count(store->objects);
    ok = array_add(store->objects, &object);
    if(!ok)
    {
        return false;
    }
    mcastsymbol_t* symbol = symbol_make(store->pstate, name, SYMBOL_ARCANE_GLOBAL, ix, false);
    if(!symbol)
    {
        goto err;
    }
    ok = dict_set(store->symbols, name, symbol);
    if(!ok)
    {
        symbol_destroy(symbol);
        goto err;
    }
    return true;
err:
    array_pop(store->objects, NULL);
    return false;
}

mcvalue_t global_store_get_object_at(mcglobalstore_t* store, int ix, bool* outok)
{
    mcvalue_t* res = array_get(store->objects, ix);
    if(!res)
    {
        *outok = false;
        return mc_value_makenull();
    }
    *outok = true;
    return *res;
}

mcvalue_t* global_store_get_object_data(mcglobalstore_t* store)
{
    return array_data(store->objects);
}

int global_store_get_object_count(mcglobalstore_t* store)
{
    return array_count(store->objects);
}

bool object_is_hashable(mcvalue_t obj)
{
    mcobjtype_t type = object_get_type(obj);
    switch(type)
    {
        case ARCANE_OBJECT_STRING:
            return true;
        case ARCANE_OBJECT_NUMBER:
            return true;
        case ARCANE_OBJECT_BOOL:
            return true;
        default:
            return false;
    }
}

void mc_printer_printobject(mcvalue_t obj, mcprintstate_t* pr, bool quotestr)
{
    mcobjtype_t type = object_get_type(obj);
    switch(type)
    {
        case ARCANE_OBJECT_FREED:
        {
            mc_printer_append(pr, "FREED");
            break;
        }
        case ARCANE_OBJECT_NONE:
        {
            mc_printer_append(pr, "NONE");
            break;
        }
        case ARCANE_OBJECT_NUMBER:
        {
            double number = object_get_number(obj);
            mc_printer_appendf(pr, "%1.10g", number);
            break;
        }
        case ARCANE_OBJECT_BOOL:
        {
            mc_printer_append(pr, object_get_bool(obj) ? "true" : "false");
            break;
        }
        case ARCANE_OBJECT_STRING:
        {
            const char* string = object_get_string(obj);
            if(quotestr)
            {
                mc_printer_appendf(pr, "\"%s\"", string);
            }
            else
            {
                mc_printer_append(pr, string);
            }
            break;
        }
        case ARCANE_OBJECT_NULL:
        {
            mc_printer_append(pr, "null");
            break;
        }
        case ARCANE_OBJECT_FUNCTION:
        {
            mcobjfuncscript_t* function = object_get_function(obj);
            mc_printer_appendf(pr, "CompiledFunction: %s\n", object_get_function_name(obj));
            code_to_string(function->comp_result->bytecode, function->comp_result->src_positions, function->comp_result->count, pr);
            break;
        }
        case ARCANE_OBJECT_ARRAY:
        {
            size_t i;
            size_t alen;
            alen = object_get_array_length(obj);
            mc_printer_append(pr, "[");
            for(i = 0; i < alen; i++)
            {
                mcvalue_t iobj = object_get_array_value_at(obj, i);
                mc_printer_printobject(iobj, pr, true);
                if(i < (alen - 1))
                {
                    mc_printer_append(pr, ", ");
                }
            }
            mc_printer_append(pr, "]");
            break;
        }
        case ARCANE_OBJECT_MAP:
        {
            size_t i;
            size_t alen;
            alen = object_get_map_length(obj);
            mc_printer_append(pr, "{");
            for(i = 0; i < alen; i++)
            {
                mcvalue_t key = object_get_map_key_at(obj, i);
                mcvalue_t val = object_get_map_value_at(obj, i);
                mc_printer_printobject(key, pr, true);
                mc_printer_append(pr, ": ");
                mc_printer_printobject(val, pr, true);
                if(i < (alen - 1))
                {
                    mc_printer_append(pr, ", ");
                }
            }
            mc_printer_append(pr, "}");
            break;
        }
        case ARCANE_OBJECT_NATIVE_FUNCTION:
        {
            mc_printer_append(pr, "NATIVE_FUNCTION");
            break;
        }
        case ARCANE_OBJECT_EXTERNAL:
        {
            mc_printer_append(pr, "EXTERNAL");
            break;
        }
        case ARCANE_OBJECT_ERROR:
        {
            mc_printer_appendf(pr, "ERROR: %s\n", object_get_error_message(obj));
            mctraceback_t* traceback = object_get_error_traceback(obj);
            ARCANE_ASSERT(traceback);
            if(traceback)
            {
                mc_printer_append(pr, "Traceback:\n");
                traceback_to_string(traceback, pr);
            }
            break;
        }
        case ARCANE_OBJECT_ANY:
        {
            ARCANE_ASSERT(false);
        }
    }
}

const char* object_get_type_name(mcobjtype_t type)
{
    switch(type)
    {
        case ARCANE_OBJECT_NONE:
            return "NONE";
        case ARCANE_OBJECT_FREED:
            return "NONE";
        case ARCANE_OBJECT_NUMBER:
            return "NUMBER";
        case ARCANE_OBJECT_BOOL:
            return "BOOL";
        case ARCANE_OBJECT_STRING:
            return "STRING";
        case ARCANE_OBJECT_NULL:
            return "NULL";
        case ARCANE_OBJECT_NATIVE_FUNCTION:
            return "NATIVE_FUNCTION";
        case ARCANE_OBJECT_ARRAY:
            return "ARRAY";
        case ARCANE_OBJECT_MAP:
            return "MAP";
        case ARCANE_OBJECT_FUNCTION:
            return "FUNCTION";
        case ARCANE_OBJECT_EXTERNAL:
            return "EXTERNAL";
        case ARCANE_OBJECT_ERROR:
            return "ERROR";
        case ARCANE_OBJECT_ANY:
            return "ANY";
    }
    return "NONE";
}

char* object_get_type_union_name(mcstate_t* state, mcobjtype_t type)
{
    if(type == ARCANE_OBJECT_ANY || type == ARCANE_OBJECT_NONE || type == ARCANE_OBJECT_FREED)
    {
        return arcane_strdup(state, object_get_type_name(type));
    }
    mcprintstate_t* res = mc_printer_make(state, NULL);
    if(!res)
    {
        return NULL;
    }
    bool inbetween = false;
#define CHECK_TYPE(t)                                    \
    do                                                   \
    {                                                    \
        if((type & t) == t)                              \
        {                                                \
            if(inbetween)                               \
            {                                            \
                mc_printer_append(res, "|");                 \
            }                                            \
            mc_printer_append(res, object_get_type_name(t)); \
            inbetween = true;                           \
        }                                                \
    } while(0)

    CHECK_TYPE(ARCANE_OBJECT_NUMBER);
    CHECK_TYPE(ARCANE_OBJECT_BOOL);
    CHECK_TYPE(ARCANE_OBJECT_STRING);
    CHECK_TYPE(ARCANE_OBJECT_NULL);
    CHECK_TYPE(ARCANE_OBJECT_NATIVE_FUNCTION);
    CHECK_TYPE(ARCANE_OBJECT_ARRAY);
    CHECK_TYPE(ARCANE_OBJECT_MAP);
    CHECK_TYPE(ARCANE_OBJECT_FUNCTION);
    CHECK_TYPE(ARCANE_OBJECT_EXTERNAL);
    CHECK_TYPE(ARCANE_OBJECT_ERROR);

    return mc_printer_get_string_and_destroy(res);
}

char* object_serialize(mcstate_t* state, mcvalue_t object)
{
    mcprintstate_t* buf = mc_printer_make(state, NULL);
    if(!buf)
    {
        return NULL;
    }
    mc_printer_printobject(object, buf, true);
    char* string = mc_printer_get_string_and_destroy(buf);
    return string;
}

mcvalue_t object_deep_copy(mcstate_t* vm, mcgcmemory_t* mem, mcvalue_t obj)
{
    mcvaldict_t* copies = valdict_make_(vm, sizeof(mcvalue_t), sizeof(mcvalue_t));
    if(!copies)
    {
        return mc_value_makenull();
    }
    mcvalue_t res = object_deep_copy_internal(vm, mem, obj, copies);
    valdict_destroy(copies);
    return res;
}

mcvalue_t object_copy(mcstate_t* vm, mcvalue_t obj)
{
    bool ok;
    mcvalue_t copy = mc_value_makenull();
    mcobjtype_t type = object_get_type(obj);
    switch(type)
    {
        case ARCANE_OBJECT_ANY:
        case ARCANE_OBJECT_FREED:
        case ARCANE_OBJECT_NONE:
        {
            ARCANE_ASSERT(false);
            copy = mc_value_makenull();
            break;
        }
        case ARCANE_OBJECT_NUMBER:
        case ARCANE_OBJECT_BOOL:
        case ARCANE_OBJECT_NULL:
        case ARCANE_OBJECT_FUNCTION:
        case ARCANE_OBJECT_NATIVE_FUNCTION:
        case ARCANE_OBJECT_ERROR:
        {
            copy = obj;
            break;
        }
        case ARCANE_OBJECT_STRING:
        {
            const char* str = object_get_string(obj);
            copy = mc_value_makestring(vm, str);
            break;
        }
        case ARCANE_OBJECT_ARRAY:
        {
            int len = object_get_array_length(obj);
            copy = object_make_array_with_capacity(vm, len);
            if(object_is_null(copy))
            {
                return mc_value_makenull();
            }
            for(int i = 0; i < len; i++)
            {
                mcvalue_t item = object_get_array_value_at(obj, i);
                ok = object_add_array_value(copy, item);
                if(!ok)
                {
                    return mc_value_makenull();
                }
            }
            break;
        }
        case ARCANE_OBJECT_MAP:
        {
            copy = object_make_map(vm);
            for(int i = 0; i < object_get_map_length(obj); i++)
            {
                mcvalue_t key = object_get_map_key_at(obj, i);
                mcvalue_t val = object_get_map_value_at(obj, i);
                ok = object_set_map_value(copy, key, val);
                if(!ok)
                {
                    return mc_value_makenull();
                }
            }
            break;
        }
        case ARCANE_OBJECT_EXTERNAL:
        {
            copy = object_make_external(vm, NULL);
            if(object_is_null(copy))
            {
                return mc_value_makenull();
            }
            mcobjexternal_t* external = object_get_external_data(obj);
            void* datacopy = NULL;
            if(external->data_copy_fn)
            {
                datacopy = external->data_copy_fn(external->data);
            }
            else
            {
                datacopy = external->data;
            }
            object_set_external_data(copy, datacopy);
            object_set_external_destroy_function(copy, external->data_destroy_fn);
            object_set_external_copy_function(copy, external->data_copy_fn);
            break;
        }
    }
    return copy;
}

double object_compare(mcvalue_t a, mcvalue_t b, bool* outok)
{
    /*
    if(a.objdatahandle == b.objdatahandle)
    {
        return 0;
    }
    */

    *outok = true;

    mcobjtype_t atype = object_get_type(a);
    mcobjtype_t btype = object_get_type(b);

    if((atype == ARCANE_OBJECT_NUMBER || atype == ARCANE_OBJECT_BOOL || atype == ARCANE_OBJECT_NULL) && (btype == ARCANE_OBJECT_NUMBER || btype == ARCANE_OBJECT_BOOL || btype == ARCANE_OBJECT_NULL))
    {
        double leftval = object_get_number(a);
        double rightval = object_get_number(b);
        return leftval - rightval;
    }
    if(atype == btype && atype == ARCANE_OBJECT_STRING)
    {
        int alen = object_get_string_length(a);
        int blen = object_get_string_length(b);
        if(alen != blen)
        {
            return alen - blen;
        }
        unsigned long ahash = object_get_string_hash(a);
        unsigned long bhash = object_get_string_hash(b);
        if(ahash != bhash)
        {
            return ahash - bhash;
        }
        const char* astring = object_get_string(a);
        const char* bstring = object_get_string(b);
        return strcmp(astring, bstring);
    }
    if((object_is_allocated(a) || object_is_null(a)) && (object_is_allocated(b) || object_is_null(b)))
    {
        intptr_t adataval = (intptr_t)object_get_allocated_data(a);
        intptr_t bdataval = (intptr_t)object_get_allocated_data(b);
        return (double)(adataval - bdataval);
    }
    *outok = false;
    return 1;
}

bool object_equals(mcvalue_t a, mcvalue_t b)
{
    bool ok;
    mcobjtype_t atype = object_get_type(a);
    mcobjtype_t btype = object_get_type(b);

    if(atype != btype)
    {
        return false;
    }
    ok = false;
    double res = object_compare(a, b, &ok);
    return ARCANE_DBLEQ(res, 0);
}

mcobjexternal_t* object_get_external_data(mcvalue_t object)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_EXTERNAL);
    mcobjdata_t* data = object_get_allocated_data(object);
    return &data->external;
}

bool object_set_external_destroy_function(mcvalue_t object, externaldatadestroyfn destroy_fn)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_EXTERNAL);
    mcobjexternal_t* data = object_get_external_data(object);
    if(!data)
    {
        return false;
    }
    data->data_destroy_fn = destroy_fn;
    return true;
}

mcobjdata_t* object_get_allocated_data(mcvalue_t object)
{
    ARCANE_ASSERT(object_is_allocated(object) || object_get_type(object) == ARCANE_OBJECT_NULL);
    return object.objdatahandle;
}

bool object_get_bool(mcvalue_t obj)
{
    if(object_is_number(obj))
    {
        return obj.valnumber;
    }
    return obj.valbool;
}

double object_get_number(mcvalue_t obj)
{
    if(object_is_number(obj))
    {
        if(obj.type == ARCANE_OBJECT_BOOL)
        {
            return obj.valbool;
        }
        return obj.valnumber;
    }
    return obj.valnumber;
}

const char* object_get_string(mcvalue_t object)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_STRING);
    mcobjdata_t* data = object_get_allocated_data(object);
    return object_data_get_string(data);
}

int object_get_string_length(mcvalue_t object)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_STRING);
    mcobjdata_t* data = object_get_allocated_data(object);
    return data->string.length;
}

void object_set_string_length(mcvalue_t object, int len)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_STRING);
    mcobjdata_t* data = object_get_allocated_data(object);
    data->string.length = len;
}

int object_get_string_capacity(mcvalue_t object)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_STRING);
    mcobjdata_t* data = object_get_allocated_data(object);
    return data->string.capacity;
}

char* object_get_mutable_string(mcvalue_t object)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_STRING);
    mcobjdata_t* data = object_get_allocated_data(object);
    return object_data_get_string(data);
}

bool mc_value_strappend(mcvalue_t obj, const char* src, int len)
{
    ARCANE_ASSERT(object_get_type(obj) == ARCANE_OBJECT_STRING);
    mcobjdata_t* data = object_get_allocated_data(obj);
    mcobjstring_t* string = &data->string;
    char* strbuf = object_get_mutable_string(obj);
    int currentlen = string->length;
    int capacity = string->capacity;
    if((len + currentlen) > capacity)
    {
        ARCANE_ASSERT(false);
        return false;
    }
    memcpy(strbuf + currentlen, src, len);
    string->length += len;
    strbuf[string->length] = '\0';
    return true;
}

unsigned long object_get_string_hash(mcvalue_t obj)
{
    ARCANE_ASSERT(object_get_type(obj) == ARCANE_OBJECT_STRING);
    mcobjdata_t* data = object_get_allocated_data(obj);
    if(data->string.hash == 0)
    {
        data->string.hash = object_hash_string(object_get_string(obj));
        if(data->string.hash == 0)
        {
            data->string.hash = 1;
        }
    }
    return data->string.hash;
}

mcobjfuncscript_t* object_get_function(mcvalue_t object)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_FUNCTION);
    mcobjdata_t* data = object_get_allocated_data(object);
    return &data->function;
}

mcobjfuncnative_t* object_get_native_function(mcvalue_t obj)
{
    mcobjdata_t* data = object_get_allocated_data(obj);
    return &data->native_function;
}


const char* object_get_function_name(mcvalue_t obj)
{
    ARCANE_ASSERT(object_get_type(obj) == ARCANE_OBJECT_FUNCTION);
    mcobjdata_t* data = object_get_allocated_data(obj);
    ARCANE_ASSERT(data);
    if(!data)
    {
        return NULL;
    }

    if(data->function.owns_data)
    {
        return data->function.name;
    }
    return data->function.const_name;
}

mcvalue_t object_get_function_free_val(mcvalue_t obj, int ix)
{
    ARCANE_ASSERT(object_get_type(obj) == ARCANE_OBJECT_FUNCTION);
    mcobjdata_t* data = object_get_allocated_data(obj);
    ARCANE_ASSERT(data);
    if(!data)
    {
        return mc_value_makenull();
    }
    mcobjfuncscript_t* fun = &data->function;
    ARCANE_ASSERT(ix >= 0 && ix < fun->free_vals_count);
    if(ix < 0 || ix >= fun->free_vals_count)
    {
        return mc_value_makenull();
    }
    if(freevals_are_allocated(fun))
    {
        return fun->free_vals_allocated[ix];
    }
    return fun->free_vals_buf[ix];
}

void object_set_function_free_val(mcvalue_t obj, int ix, mcvalue_t val)
{
    ARCANE_ASSERT(object_get_type(obj) == ARCANE_OBJECT_FUNCTION);
    mcobjdata_t* data = object_get_allocated_data(obj);
    ARCANE_ASSERT(data);
    if(!data)
    {
        return;
    }
    mcobjfuncscript_t* fun = &data->function;
    ARCANE_ASSERT(ix >= 0 && ix < fun->free_vals_count);
    if(ix < 0 || ix >= fun->free_vals_count)
    {
        return;
    }
    if(freevals_are_allocated(fun))
    {
        fun->free_vals_allocated[ix] = val;
    }
    else
    {
        fun->free_vals_buf[ix] = val;
    }
}

mcvalue_t* object_get_function_free_vals(mcvalue_t obj)
{
    ARCANE_ASSERT(object_get_type(obj) == ARCANE_OBJECT_FUNCTION);
    mcobjdata_t* data = object_get_allocated_data(obj);
    ARCANE_ASSERT(data);
    if(!data)
    {
        return NULL;
    }
    mcobjfuncscript_t* fun = &data->function;
    if(freevals_are_allocated(fun))
    {
        return fun->free_vals_allocated;
    }
    return fun->free_vals_buf;
}

const char* object_get_error_message(mcvalue_t object)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_ERROR);
    mcobjdata_t* data = object_get_allocated_data(object);
    return data->error.message;
}

void object_set_error_traceback(mcvalue_t object, mctraceback_t* traceback)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_ERROR);
    if(object_get_type(object) != ARCANE_OBJECT_ERROR)
    {
        return;
    }
    mcobjdata_t* data = object_get_allocated_data(object);
    ARCANE_ASSERT(data->error.traceback == NULL);
    data->error.traceback = traceback;
}

mctraceback_t* object_get_error_traceback(mcvalue_t object)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_ERROR);
    mcobjdata_t* data = object_get_allocated_data(object);
    return data->error.traceback;
}

bool object_set_external_data(mcvalue_t object, void* extdata)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_EXTERNAL);
    mcobjexternal_t* data = object_get_external_data(object);
    if(!data)
    {
        return false;
    }
    data->data = extdata;
    return true;
}

bool object_set_external_copy_function(mcvalue_t object, externaldatacopyfn copy_fn)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_EXTERNAL);
    mcobjexternal_t* data = object_get_external_data(object);
    if(!data)
    {
        return false;
    }
    data->data_copy_fn = copy_fn;
    return true;
}

mcvalue_t object_get_array_value_at(mcvalue_t object, int ix)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_ARRAY);
    mcbasicarray_t* array = object_get_allocated_array(object);
    if(ix < 0 || ix >= array_count(array))
    {
        return mc_value_makenull();
    }
    mcvalue_t* res = array_get(array, ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

bool object_set_array_value_at(mcvalue_t object, int ix, mcvalue_t val)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_ARRAY);
    mcbasicarray_t* array = object_get_allocated_array(object);
    if(ix < 0 || ix >= array_count(array))
    {
        return false;
    }
    return array_set(array, ix, &val);
}

bool object_add_array_value(mcvalue_t object, mcvalue_t val)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_ARRAY);
    mcbasicarray_t* array = object_get_allocated_array(object);
    return array_add(array, &val);
}

int object_get_array_length(mcvalue_t object)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_ARRAY);
    mcbasicarray_t* array = object_get_allocated_array(object);
    return array_count(array);
}

bool object_remove_array_value_at(mcvalue_t object, int ix)
{
    mcbasicarray_t* array = object_get_allocated_array(object);
    return array_remove_at(array, ix);
}

int object_get_map_length(mcvalue_t object)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_MAP);
    mcobjdata_t* data = object_get_allocated_data(object);
    return valdict_count(data->map);
}

mcvalue_t object_get_map_key_at(mcvalue_t object, int ix)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_MAP);
    mcobjdata_t* data = object_get_allocated_data(object);
    mcvalue_t* res = valdict_get_key_at(data->map, ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

mcvalue_t object_get_map_value_at(mcvalue_t object, int ix)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_MAP);
    mcobjdata_t* data = object_get_allocated_data(object);
    mcvalue_t* res = valdict_get_value_at(data->map, ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

bool object_set_map_value_at(mcvalue_t object, int ix, mcvalue_t val)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_MAP);
    if(ix >= object_get_map_length(object))
    {
        return false;
    }
    mcobjdata_t* data = object_get_allocated_data(object);
    return valdict_set_value_at(data->map, ix, &val);
}

mcvalue_t object_get_kv_pair_at(mcstate_t* vm, mcvalue_t object, int ix)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_MAP);
    mcobjdata_t* data = object_get_allocated_data(object);
    if(ix >= valdict_count(data->map))
    {
        return mc_value_makenull();
    }
    mcvalue_t key = object_get_map_key_at(object, ix);
    mcvalue_t val = object_get_map_value_at(object, ix);
    mcvalue_t res = object_make_map(vm);
    if(object_is_null(res))
    {
        return mc_value_makenull();
    }

    mcvalue_t keyobj = mc_value_makestring(vm, "key");
    if(object_is_null(keyobj))
    {
        return mc_value_makenull();
    }
    object_set_map_value(res, keyobj, key);

    mcvalue_t valobj = mc_value_makestring(vm, "value");
    if(object_is_null(valobj))
    {
        return mc_value_makenull();
    }
    object_set_map_value(res, valobj, val);

    return res;
}

bool object_set_map_value(mcvalue_t object, mcvalue_t key, mcvalue_t val)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_MAP);
    mcobjdata_t* data = object_get_allocated_data(object);
    return valdict_set(data->map, &key, &val);
}

mcvalue_t object_get_map_value(mcvalue_t object, mcvalue_t key)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_MAP);
    mcobjdata_t* data = object_get_allocated_data(object);
    mcvalue_t* res = valdict_get(data->map, &key);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

bool object_map_has_key(mcvalue_t object, mcvalue_t key)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_MAP);
    mcobjdata_t* data = object_get_allocated_data(object);
    mcvalue_t* res = valdict_get(data->map, &key);
    return res != NULL;
}

TMPSEMISTATIC mcvalue_t object_deep_copy_internal(mcstate_t* vm, mcgcmemory_t* mem, mcvalue_t obj, mcvaldict_t* copies)
{
    bool ok;
    mcvalue_t* copyptr = valdict_get(copies, &obj);
    if(copyptr)
    {
        return *copyptr;
    }

    mcvalue_t copy = mc_value_makenull();

    mcobjtype_t type = object_get_type(obj);
    switch(type)
    {
        case ARCANE_OBJECT_FREED:
        case ARCANE_OBJECT_ANY:
        case ARCANE_OBJECT_NONE:
        {
            ARCANE_ASSERT(false);
            copy = mc_value_makenull();
            break;
        }
        case ARCANE_OBJECT_NUMBER:
        case ARCANE_OBJECT_BOOL:
        case ARCANE_OBJECT_NULL:
        case ARCANE_OBJECT_NATIVE_FUNCTION:
        {
            copy = obj;
            break;
        }
        case ARCANE_OBJECT_STRING:
        {
            const char* str = object_get_string(obj);
            copy = mc_value_makestring(vm, str);
            break;
        }
        case ARCANE_OBJECT_FUNCTION:
        {
            mcobjfuncscript_t* function = object_get_function(obj);
            uint8_t* bytecodecopy = NULL;
            mcastlocation_t* srcpositionscopy = NULL;
            compilation_result_t* comprescopy = NULL;

            bytecodecopy = (uint8_t*)allocator_malloc(vm, sizeof(uint8_t) * function->comp_result->count);
            if(!bytecodecopy)
            {
                return mc_value_makenull();
            }
            memcpy(bytecodecopy, function->comp_result->bytecode, sizeof(uint8_t) * function->comp_result->count);

            srcpositionscopy = (mcastlocation_t*)allocator_malloc(vm, sizeof(mcastlocation_t) * function->comp_result->count);
            if(!srcpositionscopy)
            {
                allocator_free(vm, bytecodecopy);
                return mc_value_makenull();
            }
            memcpy(srcpositionscopy, function->comp_result->src_positions, sizeof(mcastlocation_t) * function->comp_result->count);

            comprescopy = compilation_result_make(vm, bytecodecopy, srcpositionscopy, function->comp_result->count);
            /*
            * todo: add compilation result copy function
            */
            if(!comprescopy)
            {
                allocator_free(vm, srcpositionscopy);
                allocator_free(vm, bytecodecopy);
                return mc_value_makenull();
            }

            copy = object_make_function(vm, object_get_function_name(obj), comprescopy, true, function->num_locals, function->num_args, 0);
            if(object_is_null(copy))
            {
                compilation_result_destroy(comprescopy);
                return mc_value_makenull();
            }

            ok = valdict_set(copies, &obj, &copy);
            if(!ok)
            {
                return mc_value_makenull();
            }

            mcobjfuncscript_t* functioncopy = object_get_function(copy);
            if(freevals_are_allocated(function))
            {
                functioncopy->free_vals_allocated = (mcvalue_t*)allocator_malloc(vm, sizeof(mcvalue_t) * function->free_vals_count);
                if(!functioncopy->free_vals_allocated)
                {
                    return mc_value_makenull();
                }
            }

            functioncopy->free_vals_count = function->free_vals_count;
            for(int i = 0; i < function->free_vals_count; i++)
            {
                mcvalue_t freeval = object_get_function_free_val(obj, i);
                mcvalue_t freevalcopy = object_deep_copy_internal(vm, mem, freeval, copies);
                if(!object_is_null(freeval) && object_is_null(freevalcopy))
                {
                    return mc_value_makenull();
                }
                object_set_function_free_val(copy, i, freevalcopy);
            }
            break;
        }
        case ARCANE_OBJECT_ARRAY:
        {
            int len = object_get_array_length(obj);
            copy = object_make_array_with_capacity(vm, len);
            if(object_is_null(copy))
            {
                return mc_value_makenull();
            }
            ok = valdict_set(copies, &obj, &copy);
            if(!ok)
            {
                return mc_value_makenull();
            }
            for(int i = 0; i < len; i++)
            {
                mcvalue_t item = object_get_array_value_at(obj, i);
                mcvalue_t itemcopy = object_deep_copy_internal(vm, mem, item, copies);
                if(!object_is_null(item) && object_is_null(itemcopy))
                {
                    return mc_value_makenull();
                }
                ok = object_add_array_value(copy, itemcopy);
                if(!ok)
                {
                    return mc_value_makenull();
                }
            }
            break;
        }
        case ARCANE_OBJECT_MAP:
        {
            copy = object_make_map(vm);
            if(object_is_null(copy))
            {
                return mc_value_makenull();
            }
            ok = valdict_set(copies, &obj, &copy);
            if(!ok)
            {
                return mc_value_makenull();
            }
            for(int i = 0; i < object_get_map_length(obj); i++)
            {
                mcvalue_t key = object_get_map_key_at(obj, i);
                mcvalue_t val = object_get_map_value_at(obj, i);

                mcvalue_t keycopy = object_deep_copy_internal(vm, mem, key, copies);
                if(!object_is_null(key) && object_is_null(keycopy))
                {
                    return mc_value_makenull();
                }

                mcvalue_t valcopy = object_deep_copy_internal(vm, mem, val, copies);
                if(!object_is_null(val) && object_is_null(valcopy))
                {
                    return mc_value_makenull();
                }

                ok = object_set_map_value(copy, keycopy, valcopy);
                if(!ok)
                {
                    return mc_value_makenull();
                }
            }
            break;
        }
        case ARCANE_OBJECT_EXTERNAL:
        {
            copy = object_copy(vm, obj);
            break;
        }
        case ARCANE_OBJECT_ERROR:
        {
            copy = obj;
            break;
        }
    }
    return copy;
}

TMPSTATIC bool object_equals_wrapped(mcvalue_t* aptr, mcvalue_t* bptr)
{
    mcvalue_t a = *aptr;
    mcvalue_t b = *bptr;
    return object_equals(a, b);
}

TMPSTATIC unsigned long object_hash(mcvalue_t* objptr)
{
    mcvalue_t obj = *objptr;
    mcobjtype_t type = object_get_type(obj);

    switch(type)
    {
        case ARCANE_OBJECT_NUMBER:
        {
            double val = object_get_number(obj);
            return object_hash_double(val);
        }
        case ARCANE_OBJECT_BOOL:
        {
            bool val = object_get_bool(obj);
            return val;
        }
        case ARCANE_OBJECT_STRING:
        {
            return object_get_string_hash(obj);
        }
        default:
        {
            return 0;
        }
    }
}

TMPSTATIC unsigned long object_hash_string(const char* str)
{
    /* djb2 */
    unsigned long hash = 5381;
    int c;
    while((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

TMPSTATIC unsigned long object_hash_double(double val)
{
    /* djb2 */
    uint32_t* valptr = (uint32_t*)&val;
    unsigned long hash = 5381;
    hash = ((hash << 5) + hash) + valptr[0];
    hash = ((hash << 5) + hash) + valptr[1];
    return hash;
}

mcbasicarray_t* object_get_allocated_array(mcvalue_t object)
{
    ARCANE_ASSERT(object_get_type(object) == ARCANE_OBJECT_ARRAY);
    mcobjdata_t* data = object_get_allocated_data(object);
    return data->array;
}

TMPSTATIC bool object_is_number(mcvalue_t o)
{
    /*
    return (o.objdatahandle & MC_CONF_OBJECT_PATTERN) != MC_CONF_OBJECT_PATTERN;
    */
    return (o.type == ARCANE_OBJECT_NUMBER || o.type == ARCANE_OBJECT_BOOL);
}

/*
TMPSTATIC uint64_t get_type_tag(mcobjtype_t type)
{
    switch(type)
    {
        case ARCANE_OBJECT_NONE:
            return 0;
        case ARCANE_OBJECT_BOOL:
            return 1;
        case ARCANE_OBJECT_NULL:
            return 2;
        default:
            return 4;
    }
}
*/

TMPSTATIC bool freevals_are_allocated(mcobjfuncscript_t* fun)
{
    return fun->free_vals_count >= ARCANE_ARRAY_LEN(fun->free_vals_buf);
}

TMPSTATIC char* object_data_get_string(mcobjdata_t* data)
{
    ARCANE_ASSERT(data->type == ARCANE_OBJECT_STRING);
    if(data->string.is_allocated)
    {
        return data->string.actualallocated;
    }
    return data->string.actualonstack;
}

TMPSTATIC bool object_data_string_reserve_capacity(mcobjdata_t* data, int capacity)
{
    char* newvalue;
    mcobjstring_t* string;
    ARCANE_ASSERT(capacity >= 0);
    string = &data->string;
    string->length = 0;
    string->hash = 0;
    string->data = NULL;
    if(capacity <= string->capacity)
    {
        return true;
    }
    if(capacity <= (MC_CONF_OBJECT_STRING_BUF_SIZE - 1))
    {
        if(string->is_allocated)
        {
            /* should never happen */
            ARCANE_ASSERT(false);
            /* just in case */
            allocator_free(data->pstate, string->actualallocated);
        }
        string->capacity = MC_CONF_OBJECT_STRING_BUF_SIZE - 1;
        string->is_allocated = false;
        string->data = string->actualonstack;
        return true;
    }
    newvalue = (char*)allocator_malloc(data->pstate, capacity + 1);
    if(!newvalue)
    {
        return false;
    }
    if(string->is_allocated)
    {
        allocator_free(data->pstate, string->actualallocated);
    }
    string->actualallocated = newvalue;
    string->is_allocated = true;
    string->capacity = capacity;
    string->data = string->actualallocated;
    return true;
}

mcastsymbol_t* symbol_make(mcstate_t* state, const char* name, mcastsymtype_t type, int index, bool assignable)
{
    mcastsymbol_t* symbol = (mcastsymbol_t*)allocator_malloc(state, sizeof(mcastsymbol_t));
    if(!symbol)
    {
        return NULL;
    }
    memset(symbol, 0, sizeof(mcastsymbol_t));
    symbol->pstate = state;
    symbol->name = arcane_strdup(state, name);
    if(!symbol->name)
    {
        allocator_free(state, symbol);
        return NULL;
    }
    symbol->type = type;
    symbol->index = index;
    symbol->assignable = assignable;
    return symbol;
}

void symbol_destroy(mcastsymbol_t* symbol)
{
    if(!symbol)
    {
        return;
    }
    allocator_free(symbol->pstate, symbol->name);
    allocator_free(symbol->pstate, symbol);
}

mcastsymbol_t* symbol_copy(mcastsymbol_t* symbol)
{
    return symbol_make(symbol->pstate, symbol->name, symbol->type, symbol->index, symbol->assignable);
}

mcastsymtable_t* symbol_table_make(mcstate_t* state, mcastsymtable_t* outer, mcglobalstore_t* global_store, int module_global_offset)
{
    bool ok;
    mcastsymtable_t* table = (mcastsymtable_t*)allocator_malloc(state, sizeof(mcastsymtable_t));
    if(!table)
    {
        return NULL;
    }
    memset(table, 0, sizeof(mcastsymtable_t));
    table->pstate = state;
    table->max_num_definitions = 0;
    table->outer = outer;
    table->global_store = global_store;
    table->module_global_offset = module_global_offset;

    table->block_scopes = ptrarray_make(state);
    if(!table->block_scopes)
    {
        goto err;
    }

    table->free_symbols = ptrarray_make(state);
    if(!table->free_symbols)
    {
        goto err;
    }

    table->module_global_symbols = ptrarray_make(state);
    if(!table->module_global_symbols)
    {
        goto err;
    }

    ok = symbol_table_push_block_scope(table);
    if(!ok)
    {
        goto err;
    }

    return table;
err:
    symbol_table_destroy(table);
    return NULL;
}

void symbol_table_destroy(mcastsymtable_t* table)
{
    mcstate_t* state;
    if(!table)
    {
        return;
    }

    while(ptrarray_count(table->block_scopes) > 0)
    {
        symbol_table_pop_block_scope(table);
    }
    ptrarray_destroy(table->block_scopes);
    ptrarray_destroy_with_items_(table->module_global_symbols, (ptrarrayitemdestroyfn)symbol_destroy);
    ptrarray_destroy_with_items_(table->free_symbols, (ptrarrayitemdestroyfn)symbol_destroy);
    state = table->pstate;
    memset(table, 0, sizeof(mcastsymtable_t));
    allocator_free(state, table);
}

mcastsymtable_t* symbol_table_copy(mcastsymtable_t* table)
{
    mcastsymtable_t* copy = (mcastsymtable_t*)allocator_malloc(table->pstate, sizeof(mcastsymtable_t));
    if(!copy)
    {
        return NULL;
    }
    memset(copy, 0, sizeof(mcastsymtable_t));
    copy->pstate = table->pstate;
    copy->outer = table->outer;
    copy->global_store = table->global_store;
    copy->block_scopes = ptrarray_copy_with_items_(table->block_scopes, (ptrarrayitemcopyfn)block_scope_copy, (ptrarrayitemdestroyfn)block_scope_destroy);
    if(!copy->block_scopes)
    {
        goto err;
    }
    copy->free_symbols = ptrarray_copy_with_items_(table->free_symbols, (ptrarrayitemcopyfn)symbol_copy, (ptrarrayitemdestroyfn)symbol_destroy);
    if(!copy->free_symbols)
    {
        goto err;
    }
    copy->module_global_symbols = ptrarray_copy_with_items_(table->module_global_symbols, (ptrarrayitemcopyfn)symbol_copy, (ptrarrayitemdestroyfn)symbol_destroy);
    if(!copy->module_global_symbols)
    {
        goto err;
    }
    copy->max_num_definitions = table->max_num_definitions;
    copy->module_global_offset = table->module_global_offset;
    return copy;
err:
    symbol_table_destroy(copy);
    return NULL;
}

bool symbol_table_add_module_symbol(mcastsymtable_t* st, mcastsymbol_t* symbol)
{
    bool ok;
    if(symbol->type != SYMBOL_MODULE_GLOBAL)
    {
        ARCANE_ASSERT(false);
        return false;
    }
    if(symbol_table_symbol_is_defined(st, symbol->name))
    {
        /* todo: make sure it should be true in this case */
        return true;
    }
    mcastsymbol_t* copy = symbol_copy(symbol);
    if(!copy)
    {
        return false;
    }
    ok = set_symbol(st, copy);
    if(!ok)
    {
        symbol_destroy(copy);
        return false;
    }
    return true;
}

mcastsymbol_t* symbol_table_define(mcastsymtable_t* table, const char* name, bool assignable)
{
    bool ok;
    mcastsymbol_t* globalsymbol = global_store_get_symbol(table->global_store, name);
    if(globalsymbol)
    {
        return NULL;
    }
    /* module symbol */
    if(strchr(name, ':'))
    {
        return NULL;
    }
    /* "this" is reserved */
    if(ARCANE_STREQ(name, "this"))
    {
        return NULL;
    }
    mcastsymtype_t symboltype = table->outer == NULL ? SYMBOL_MODULE_GLOBAL : SYMBOL_LOCAL;
    int ix = next_symbol_index(table);
    mcastsymbol_t* symbol = symbol_make(table->pstate, name, symboltype, ix, assignable);
    if(!symbol)
    {
        return NULL;
    }

    bool globalsymboladded = false;
    ok = false;

    if(symboltype == SYMBOL_MODULE_GLOBAL && ptrarray_count(table->block_scopes) == 1)
    {
        mcastsymbol_t* globalsymbolcopy = symbol_copy(symbol);
        if(!globalsymbolcopy)
        {
            symbol_destroy(symbol);
            return NULL;
        }
        ok = ptrarray_push(table->module_global_symbols, globalsymbolcopy);
        if(!ok)
        {
            symbol_destroy(globalsymbolcopy);
            symbol_destroy(symbol);
            return NULL;
        }
        globalsymboladded = true;
    }

    ok = set_symbol(table, symbol);
    if(!ok)
    {
        if(globalsymboladded)
        {
            mcastsymbol_t* globalsymbolcopy = ptrarray_pop(table->module_global_symbols);
            symbol_destroy(globalsymbolcopy);
        }
        symbol_destroy(symbol);
        return NULL;
    }

    mcastscopeblock_t* topscope = ptrarray_top(table->block_scopes);
    topscope->num_definitions++;
    int definitionscount = count_num_definitions(table);
    if(definitionscount > table->max_num_definitions)
    {
        table->max_num_definitions = definitionscount;
    }

    return symbol;
}

mcastsymbol_t* symbol_table_define_free(mcastsymtable_t* st, mcastsymbol_t* original)
{
    bool ok;
    mcastsymbol_t* copy = symbol_make(st->pstate, original->name, original->type, original->index, original->assignable);
    if(!copy)
    {
        return NULL;
    }
    ok = ptrarray_push(st->free_symbols, copy);
    if(!ok)
    {
        symbol_destroy(copy);
        return NULL;
    }

    mcastsymbol_t* symbol = symbol_make(st->pstate, original->name, SYMBOL_FREE, ptrarray_count(st->free_symbols) - 1, original->assignable);
    if(!symbol)
    {
        return NULL;
    }

    ok = set_symbol(st, symbol);
    if(!ok)
    {
        symbol_destroy(symbol);
        return NULL;
    }

    return symbol;
}

mcastsymbol_t* symbol_table_define_function_name(mcastsymtable_t* st, const char* name, bool assignable)
{
    bool ok;
    /* module symbol */
    if(strchr(name, ':'))
    {
        return NULL;
    }
    mcastsymbol_t* symbol = symbol_make(st->pstate, name, SYMBOL_FUNCTION, 0, assignable);
    if(!symbol)
    {
        return NULL;
    }

    ok = set_symbol(st, symbol);
    if(!ok)
    {
        symbol_destroy(symbol);
        return NULL;
    }

    return symbol;
}

mcastsymbol_t* symbol_table_define_this(mcastsymtable_t* st)
{
    bool ok;
    mcastsymbol_t* symbol = symbol_make(st->pstate, "this", SYMBOL_THIS, 0, false);
    if(!symbol)
    {
        return NULL;
    }

    ok = set_symbol(st, symbol);
    if(!ok)
    {
        symbol_destroy(symbol);
        return NULL;
    }

    return symbol;
}

mcastsymbol_t* symbol_table_resolve(mcastsymtable_t* table, const char* name)
{
    mcastsymbol_t* symbol = NULL;
    mcastscopeblock_t* scope = NULL;

    symbol = global_store_get_symbol(table->global_store, name);
    if(symbol)
    {
        return symbol;
    }

    for(int i = ptrarray_count(table->block_scopes) - 1; i >= 0; i--)
    {
        scope = ptrarray_get(table->block_scopes, i);
        symbol = dict_get(scope->store, name);
        if(symbol)
        {
            break;
        }
    }

    if(symbol && symbol->type == SYMBOL_THIS)
    {
        symbol = symbol_table_define_free(table, symbol);
    }

    if(!symbol && table->outer)
    {
        symbol = symbol_table_resolve(table->outer, name);
        if(!symbol)
        {
            return NULL;
        }
        if(symbol->type == SYMBOL_MODULE_GLOBAL || symbol->type == SYMBOL_ARCANE_GLOBAL)
        {
            return symbol;
        }
        symbol = symbol_table_define_free(table, symbol);
    }
    return symbol;
}

bool symbol_table_symbol_is_defined(mcastsymtable_t* table, const char* name)
{
    /* todo: rename to something more obvious */
    mcastsymbol_t* symbol = global_store_get_symbol(table->global_store, name);
    if(symbol)
    {
        return true;
    }

    mcastscopeblock_t* topscope = ptrarray_top(table->block_scopes);
    symbol = dict_get(topscope->store, name);
    if(symbol)
    {
        return true;
    }
    return false;
}

bool symbol_table_push_block_scope(mcastsymtable_t* table)
{
    bool ok;
    int blockscopeoffset = 0;
    mcastscopeblock_t* prevblockscope = ptrarray_top(table->block_scopes);
    if(prevblockscope)
    {
        blockscopeoffset = table->module_global_offset + prevblockscope->offset + prevblockscope->num_definitions;
    }
    else
    {
        blockscopeoffset = table->module_global_offset;
    }

    mcastscopeblock_t* newscope = block_scope_make(table->pstate, blockscopeoffset);
    if(!newscope)
    {
        return false;
    }
    ok = ptrarray_push(table->block_scopes, newscope);
    if(!ok)
    {
        block_scope_destroy(newscope);
        return false;
    }
    return true;
}

void symbol_table_pop_block_scope(mcastsymtable_t* table)
{
    mcastscopeblock_t* topscope = ptrarray_top(table->block_scopes);
    ptrarray_pop(table->block_scopes);
    block_scope_destroy(topscope);
}

mcastscopeblock_t* symbol_table_get_block_scope(mcastsymtable_t* table)
{
    mcastscopeblock_t* topscope = ptrarray_top(table->block_scopes);
    return topscope;
}

bool symbol_table_is_module_global_scope(mcastsymtable_t* table)
{
    return table->outer == NULL;
}

bool symbol_table_is_top_block_scope(mcastsymtable_t* table)
{
    return ptrarray_count(table->block_scopes) == 1;
}

bool symbol_table_is_top_global_scope(mcastsymtable_t* table)
{
    return symbol_table_is_module_global_scope(table) && symbol_table_is_top_block_scope(table);
}

int symbol_table_get_module_global_symbol_count(mcastsymtable_t* table)
{
    return ptrarray_count(table->module_global_symbols);
}

mcastsymbol_t* symbol_table_get_module_global_symbol_at(mcastsymtable_t* table, int ix)
{
    return ptrarray_get(table->module_global_symbols, ix);
}

TMPSTATIC mcastscopeblock_t* block_scope_make(mcstate_t* state, int offset)
{
    mcastscopeblock_t* newscope = (mcastscopeblock_t*)allocator_malloc(state, sizeof(mcastscopeblock_t));
    if(!newscope)
    {
        return NULL;
    }
    memset(newscope, 0, sizeof(mcastscopeblock_t));
    newscope->pstate = state;
    newscope->store = dict_make_(state, (dictitemcopyfn)symbol_copy, (dictitemdestroyfn)symbol_destroy);
    if(!newscope->store)
    {
        block_scope_destroy(newscope);
        return NULL;
    }
    newscope->num_definitions = 0;
    newscope->offset = offset;
    return newscope;
}

TMPSTATIC void block_scope_destroy(mcastscopeblock_t* scope)
{
    dict_destroy_with_items(scope->store);
    allocator_free(scope->pstate, scope);
}

TMPSTATIC mcastscopeblock_t* block_scope_copy(mcastscopeblock_t* scope)
{
    mcastscopeblock_t* copy = (mcastscopeblock_t*)allocator_malloc(scope->pstate, sizeof(mcastscopeblock_t));
    if(!copy)
    {
        return NULL;
    }
    memset(copy, 0, sizeof(mcastscopeblock_t));
    copy->pstate = scope->pstate;
    copy->num_definitions = scope->num_definitions;
    copy->offset = scope->offset;
    copy->store = dict_copy_with_items(scope->store);
    if(!copy->store)
    {
        block_scope_destroy(copy);
        return NULL;
    }
    return copy;
}

TMPSTATIC bool set_symbol(mcastsymtable_t* table, mcastsymbol_t* symbol)
{
    mcastscopeblock_t* topscope = ptrarray_top(table->block_scopes);
    mcastsymbol_t* existing = dict_get(topscope->store, symbol->name);
    if(existing)
    {
        symbol_destroy(existing);
    }
    return dict_set(topscope->store, symbol->name, symbol);
}

TMPSTATIC int next_symbol_index(mcastsymtable_t* table)
{
    mcastscopeblock_t* topscope = ptrarray_top(table->block_scopes);
    int ix = topscope->offset + topscope->num_definitions;
    return ix;
}

TMPSTATIC int count_num_definitions(mcastsymtable_t* table)
{
    int count = 0;
    for(int i = ptrarray_count(table->block_scopes) - 1; i >= 0; i--)
    {
        mcastscopeblock_t* scope = ptrarray_get(table->block_scopes, i);
        count += scope->num_definitions;
    }
    return count;
}


static const char* gtypenames[] = {
    "ILLEGAL", "EOF",   "=",     "+=",  "-=", "*=",       "/=",       "%=",     "&=",      "|=",    "^=",     "<<=",    ">>=",
    "?",       "+",     "++",    "-",   "--", "!",        "*",        "/",      "<",       "<=",    ">",      ">=",     "==",
    "!=",      "&&",    "||",    "&",   "|",  "^",        "<<",       ">>",     ",",       ";",     ":",      "(",      ")",
    "{",       "}",     "[",     "]",   ".",  "%",        "FUNCTION", "CONST",  "VAR",     "TRUE",  "FALSE",  "IF",     "ELSE",
    "RETURN",  "WHILE", "BREAK", "FOR", "IN", "CONTINUE", "NULL",     "IMPORT", "RECOVER", "IDENT", "NUMBER", "STRING", "TEMPLATE_STRING",
};

void token_init(mcasttoken_t* tok, mcasttoktype_t type, const char* literal, int len)
{
    tok->type = type;
    tok->literal = literal;
    tok->len = len;
}

char* token_duplicate_literal(mcstate_t* state, mcasttoken_t* tok)
{
    return arcane_strndup(state, tok->literal, tok->len);
}

const char* token_type_to_string(mcasttoktype_t type)
{
    return gtypenames[type];
}

mctraceback_t* traceback_make(mcstate_t* state)
{
    mctraceback_t* traceback = (mctraceback_t*)allocator_malloc(state, sizeof(mctraceback_t));
    if(!traceback)
    {
        return NULL;
    }
    memset(traceback, 0, sizeof(mctraceback_t));
    traceback->pstate = state;
    traceback->items = array_make(state, sizeof(mctraceitem_t));
    if(!traceback->items)
    {
        traceback_destroy(traceback);
        return NULL;
    }
    return traceback;
}

void traceback_destroy(mctraceback_t* traceback)
{
    if(!traceback)
    {
        return;
    }
    for(int i = 0; i < array_count(traceback->items); i++)
    {
        mctraceitem_t* item = array_get(traceback->items, i);
        allocator_free(traceback->pstate, item->function_name);
    }
    array_destroy(traceback->items);
    allocator_free(traceback->pstate, traceback);
}

bool traceback_append(mctraceback_t* traceback, const char* function_name, mcastlocation_t pos)
{
    bool ok;
    mctraceitem_t item;
    item.function_name = arcane_strdup(traceback->pstate, function_name);
    if(!item.function_name)
    {
        return false;
    }
    item.pos = pos;
    ok = array_add(traceback->items, &item);
    if(!ok)
    {
        allocator_free(traceback->pstate, item.function_name);
        return false;
    }
    return true;
}

bool traceback_append_from_vm(mctraceback_t* traceback, mcstate_t* vm)
{
    bool ok;
    for(int i = vm->frames_count - 1; i >= 0; i--)
    {
        mcvmframe_t* frame = &vm->frames[i];
        ok = traceback_append(traceback, object_get_function_name(frame->function), frame_src_position(frame));
        if(!ok)
        {
            return false;
        }
    }
    return true;
}

bool traceback_to_string(mctraceback_t* traceback, mcprintstate_t* buf)
{
    int depth = array_count(traceback->items);
    for(int i = 0; i < depth; i++)
    {
        mctraceitem_t* item = array_get(traceback->items, i);
        const char* filename = traceback_item_get_filepath(item);
        if(item->pos.line >= 0 && item->pos.column >= 0)
        {
            mc_printer_appendf(buf, "%s in %s on %d:%d\n", item->function_name, filename, item->pos.line, item->pos.column);
        }
        else
        {
            mc_printer_appendf(buf, "%s\n", item->function_name);
        }
    }
    return !mc_printer_failed(buf);
}

const char* traceback_item_get_line(mctraceitem_t* item)
{
    if(!item->pos.file)
    {
        return NULL;
    }
    mcptrarray_t* lines = item->pos.file->lines;
    if(item->pos.line >= ptrarray_count(lines))
    {
        return NULL;
    }
    const char* line = ptrarray_get(lines, item->pos.line);
    return line;
}

const char* traceback_item_get_filepath(mctraceitem_t* item)
{
    if(!item->pos.file)
    {
        return NULL;
    }
    return item->pos.file->path;
}

bool vm_make(mcstate_t* state)
{
    state->globals_count = 0;
    state->sp = 0;
    state->this_sp = 0;
    state->frames_count = 0;
    state->last_popped = mc_value_makenull();
    state->running = false;

    for(int i = 0; i < OPCODE_MAX; i++)
    {
        state->operator_oveload_keys[i] = mc_value_makenull();
    }
#define SET_OPERATOR_OVERLOAD_KEY(op, key)                   \
    do                                                       \
    {                                                        \
        mcvalue_t keyobj = mc_value_makestring(state, key); \
        if(object_is_null(keyobj))                          \
        {                                                    \
            goto err;                                        \
        }                                                    \
        state->operator_oveload_keys[op] = keyobj;             \
    } while(0)
    SET_OPERATOR_OVERLOAD_KEY(OPCODE_ADD, "__operator_add__");
    SET_OPERATOR_OVERLOAD_KEY(OPCODE_SUB, "__operator_sub__");
    SET_OPERATOR_OVERLOAD_KEY(OPCODE_MUL, "__operator_mul__");
    SET_OPERATOR_OVERLOAD_KEY(OPCODE_DIV, "__operator_div__");
    SET_OPERATOR_OVERLOAD_KEY(OPCODE_MOD, "__operator_mod__");
    SET_OPERATOR_OVERLOAD_KEY(OPCODE_OR, "__operator_or__");
    SET_OPERATOR_OVERLOAD_KEY(OPCODE_XOR, "__operator_xor__");
    SET_OPERATOR_OVERLOAD_KEY(OPCODE_AND, "__operator_and__");
    SET_OPERATOR_OVERLOAD_KEY(OPCODE_LSHIFT, "__operator_lshift__");
    SET_OPERATOR_OVERLOAD_KEY(OPCODE_RSHIFT, "__operator_rshift__");
    SET_OPERATOR_OVERLOAD_KEY(OPCODE_MINUS, "__operator_minus__");
    SET_OPERATOR_OVERLOAD_KEY(OPCODE_BANG, "__operator_bang__");
    SET_OPERATOR_OVERLOAD_KEY(OPCODE_COMPARE, "__cmp__");
#undef SET_OPERATOR_OVERLOAD_KEY
    return true;
    err:
        return false;
}

void vm_destroy(mcstate_t* vm)
{
    if(!vm)
    {
        return;
    }
}

void vm_reset(mcstate_t* vm)
{
    vm->sp = 0;
    vm->this_sp = 0;
    while(vm->frames_count > 0)
    {
        pop_frame(vm);
    }
}

bool vm_run(mcstate_t* vm, compilation_result_t* comp_res, mcbasicarray_t* constants)
{
#ifdef ARCANE_DEBUG
    int oldsp = vm->sp;
#endif
    int oldthissp = vm->this_sp;
    int oldframescount = vm->frames_count;
    mcvalue_t mainfn = object_make_function(vm, "main", comp_res, false, 0, 0, 0);
    if(object_is_null(mainfn))
    {
        return false;
    }
    stack_push(vm, mainfn);
    bool res = vm_execute_function(vm, mainfn, constants);
    while(vm->frames_count > oldframescount)
    {
        pop_frame(vm);
    }
    ARCANE_ASSERT(vm->sp == oldsp);
    vm->this_sp = oldthissp;
    return res;
}

mcvalue_t vm_call(mcstate_t* vm, mcbasicarray_t* constants, mcvalue_t callee, int argc, mcvalue_t* args)
{
    bool ok;
    mcobjtype_t type = object_get_type(callee);
    if(type == ARCANE_OBJECT_FUNCTION)
    {
#ifdef ARCANE_DEBUG
        int oldsp = vm->sp;
#endif
        int oldthissp = vm->this_sp;
        int oldframescount = vm->frames_count;
        stack_push(vm, callee);
        for(int i = 0; i < argc; i++)
        {
            stack_push(vm, args[i]);
        }
        ok = vm_execute_function(vm, callee, constants);
        if(!ok)
        {
            return mc_value_makenull();
        }
        while(vm->frames_count > oldframescount)
        {
            pop_frame(vm);
        }
        ARCANE_ASSERT(vm->sp == oldsp);
        vm->this_sp = oldthissp;
        return vm_get_last_popped(vm);
    }
    if(type == ARCANE_OBJECT_NATIVE_FUNCTION)
    {
        return call_native_function(vm, callee, srcposinvalid, argc, args);
    }
    errors_add_error(&vm->errors, ARCANE_ERROR_USER, srcposinvalid, "Object is not callable");
    return mc_value_makenull();
}

bool mc_vmdo_math(mcstate_t* vm, mcopcode_t opcode)
{
    bool ok;
    double res;
    double dnright;
    double dnleft;
    int leftlen;
    int rightlen;
    int64_t ivleft;
    int64_t ivright;
    mcvalue_t valright;
    mcvalue_t valleft;
    mcobjtype_t lefttype;
    mcobjtype_t righttype;
    valright = stack_pop(vm);
    valleft = stack_pop(vm);
    lefttype = object_get_type(valleft);
    righttype = object_get_type(valright);
    if(object_is_numeric(valleft) && object_is_numeric(valright))
    {
        dnright = object_get_number(valright);
        dnleft = object_get_number(valleft);
        ivleft = (int64_t)dnleft;
        ivright = (int64_t)dnright;
        res = 0;
        switch(opcode)
        {
            case OPCODE_ADD:
                res = dnleft + dnright;
                break;
            case OPCODE_SUB:
                res = dnleft - dnright;
                break;
            case OPCODE_MUL:
                res = dnleft * dnright;
                break;
            case OPCODE_DIV:
                res = dnleft / dnright;
                break;
            case OPCODE_MOD:
                res = fmod(dnleft, dnright);
                break;
            case OPCODE_OR:
                res = (double)(ivleft | ivright);
                break;
            case OPCODE_XOR:
                res = (double)(ivleft ^ ivright);
                break;
            case OPCODE_AND:
                res = (double)(ivleft & ivright);
                break;
            case OPCODE_LSHIFT:
                res = (double)(ivleft << ivright);
                break;
            case OPCODE_RSHIFT:
                res = (double)(ivleft >> ivright);
                break;
            default:
                ARCANE_ASSERT(false);
                break;
        }
        stack_push(vm, mc_value_makenumber(res));
    }
    else if(lefttype == ARCANE_OBJECT_STRING && righttype == ARCANE_OBJECT_STRING && opcode == OPCODE_ADD)
    {
        leftlen = object_get_string_length(valleft);
        rightlen = object_get_string_length(valright);

        if(leftlen == 0)
        {
            stack_push(vm, valright);
        }
        else if(rightlen == 0)
        {
            stack_push(vm, valleft);
        }
        else
        {
            const char* strleft = object_get_string(valleft);
            const char* strright = object_get_string(valright);

            mcvalue_t nstring = mc_value_makestrcapacity(vm, leftlen + rightlen);
            if(object_is_null(nstring))
            {
                return false;
            }

            ok = mc_value_strappend(nstring, strleft, leftlen);
            if(!ok)
            {
                return false;
            }

            ok = mc_value_strappend(nstring, strright, rightlen);
            if(!ok)
            {
                return false;
            }
            stack_push(vm, nstring);
        }
    }
    else
    {
        bool overloadfound = false;
        ok = try_overload_operator(vm, valleft, valright, opcode, &overloadfound);
        if(!ok)
        {
            return false;
        }
        if(!overloadfound)
        {
            const char* opcodename = opcode_get_name(opcode);
            const char* lefttypename = object_get_type_name(lefttype);
            const char* righttypename = object_get_type_name(righttype);
            errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Invalid operand types for %s, got %s and %s",
                              opcodename, lefttypename, righttypename);
            return false;
        }
    }
    return true;
}

bool vm_execute_function(mcstate_t* vm, mcvalue_t function, mcbasicarray_t* constants)
{
    bool ok;
    if(vm->running)
    {
        errors_add_error(&vm->errors, ARCANE_ERROR_USER, srcposinvalid, "VM is already executing code");
        return false;
    }
    /* naming is hard */
    mcobjfuncscript_t* functionfunction = object_get_function(function);
    mcvmframe_t newframe;
    ok = false;
    ok = frame_init(&newframe, function, vm->sp - functionfunction->num_args);
    if(!ok)
    {
        return false;
    }
    ok = push_frame(vm, newframe);
    if(!ok)
    {
        errors_add_error(&vm->errors, ARCANE_ERROR_USER, srcposinvalid, "Pushing frame failed");
        return false;
    }

    vm->running = true;
    vm->last_popped = mc_value_makenull();

    bool checktime = false;
    double maxexectimems = 0;
    #if 0
    if(vm->config)
    #endif
    {
        checktime = vm->config.max_execution_time_set;
        maxexectimems = vm->config.max_execution_time_ms;
    }
    unsigned timecheckinterval = 1000;
    unsigned timecheckcounter = 0;
    mctimer_t timer;
    memset(&timer, 0, sizeof(mctimer_t));
    if(checktime)
    {
        timer = arcane_timer_start();
    }

    while(vm->current_frame->ip < vm->current_frame->bytecode_size)
    {
        mcopcode_t opcode = frame_read_opcode(vm->current_frame);
        switch(opcode)
        {
            case OPCODE_CONSTANT:
            {
                uint16_t constantix = frame_read_uint16(vm->current_frame);
                mcvalue_t* constant = array_get(constants, constantix);
                if(!constant)
                {
                    errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Constant at %d not found", constantix);
                    goto err;
                }
                stack_push(vm, *constant);
                break;
            }
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_MUL:
            case OPCODE_DIV:
            case OPCODE_MOD:
            case OPCODE_OR:
            case OPCODE_XOR:
            case OPCODE_AND:
            case OPCODE_LSHIFT:
            case OPCODE_RSHIFT:
            {
                if(!mc_vmdo_math(vm, opcode))
                {
                    goto err;
                }
                break;
            }
            case OPCODE_POP:
            {
                stack_pop(vm);
                break;
            }
            case OPCODE_TRUE:
            {
                stack_push(vm, mc_value_makebool(true));
                break;
            }
            case OPCODE_FALSE:
            {
                stack_push(vm, mc_value_makebool(false));
                break;
            }
            case OPCODE_COMPARE:
            case OPCODE_COMPARE_EQ:
            {
                mcvalue_t right = stack_pop(vm);
                mcvalue_t left = stack_pop(vm);
                bool isoverloaded = false;
                ok = try_overload_operator(vm, left, right, OPCODE_COMPARE, &isoverloaded);
                if(!ok)
                {
                    goto err;
                }
                if(!isoverloaded)
                {
                    double comparisonres = object_compare(left, right, &ok);
                    if(ok || opcode == OPCODE_COMPARE_EQ)
                    {
                        mcvalue_t res = mc_value_makenumber(comparisonres);
                        stack_push(vm, res);
                    }
                    else
                    {
                        const char* righttypestring = object_get_type_name(object_get_type(right));
                        const char* lefttypestring = object_get_type_name(object_get_type(left));
                        errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Cannot compare %s and %s", lefttypestring, righttypestring);
                        goto err;
                    }
                }
                break;
            }
            case OPCODE_EQUAL:
            case OPCODE_NOT_EQUAL:
            case OPCODE_GREATER_THAN:
            case OPCODE_GREATER_THAN_EQUAL:
            {
                mcvalue_t value = stack_pop(vm);
                double comparisonres = object_get_number(value);
                bool resval = false;
                switch(opcode)
                {
                    case OPCODE_EQUAL:
                        resval = ARCANE_DBLEQ(comparisonres, 0);
                        break;
                    case OPCODE_NOT_EQUAL:
                        resval = !ARCANE_DBLEQ(comparisonres, 0);
                        break;
                    case OPCODE_GREATER_THAN:
                        resval = comparisonres > 0;
                        break;
                    case OPCODE_GREATER_THAN_EQUAL:
                    {
                        resval = comparisonres > 0 || ARCANE_DBLEQ(comparisonres, 0);
                        break;
                    }
                    default:
                        ARCANE_ASSERT(false);
                        break;
                }
                mcvalue_t res = mc_value_makebool(resval);
                stack_push(vm, res);
                break;
            }
            case OPCODE_MINUS:
            {
                mcvalue_t operand = stack_pop(vm);
                mcobjtype_t operandtype = object_get_type(operand);
                if(operandtype == ARCANE_OBJECT_NUMBER)
                {
                    double val = object_get_number(operand);
                    mcvalue_t res = mc_value_makenumber(-val);
                    stack_push(vm, res);
                }
                else
                {
                    bool overloadfound = false;
                    ok = try_overload_operator(vm, operand, mc_value_makenull(), OPCODE_MINUS, &overloadfound);
                    if(!ok)
                    {
                        goto err;
                    }
                    if(!overloadfound)
                    {
                        const char* operandtypestring = object_get_type_name(operandtype);
                        errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Invalid operand type for MINUS, got %s", operandtypestring);
                        goto err;
                    }
                }
                break;
            }
            case OPCODE_BANG:
            {
                mcvalue_t operand = stack_pop(vm);
                mcobjtype_t type = object_get_type(operand);
                if(type == ARCANE_OBJECT_BOOL)
                {
                    mcvalue_t res = mc_value_makebool(!object_get_bool(operand));
                    stack_push(vm, res);
                }
                else if(type == ARCANE_OBJECT_NULL)
                {
                    mcvalue_t res = mc_value_makebool(true);
                    stack_push(vm, res);
                }
                else
                {
                    bool overloadfound = false;
                    ok = try_overload_operator(vm, operand, mc_value_makenull(), OPCODE_BANG, &overloadfound);
                    if(!ok)
                    {
                        goto err;
                    }
                    if(!overloadfound)
                    {
                        mcvalue_t res = mc_value_makebool(false);
                        stack_push(vm, res);
                    }
                }
                break;
            }
            case OPCODE_JUMP:
            {
                uint16_t pos = frame_read_uint16(vm->current_frame);
                vm->current_frame->ip = pos;
                break;
            }
            case OPCODE_JUMP_IF_FALSE:
            {
                uint16_t pos = frame_read_uint16(vm->current_frame);
                mcvalue_t test = stack_pop(vm);
                if(!object_get_bool(test))
                {
                    vm->current_frame->ip = pos;
                }
                break;
            }
            case OPCODE_JUMP_IF_TRUE:
            {
                uint16_t pos = frame_read_uint16(vm->current_frame);
                mcvalue_t test = stack_pop(vm);
                if(object_get_bool(test))
                {
                    vm->current_frame->ip = pos;
                }
                break;
            }
            case OPCODE_NULL:
            {
                stack_push(vm, mc_value_makenull());
                break;
            }
            case OPCODE_DEFINE_MODULE_GLOBAL:
            {
                uint16_t ix = frame_read_uint16(vm->current_frame);
                mcvalue_t value = stack_pop(vm);
                vm_set_global(vm, ix, value);
                break;
            }
            case OPCODE_SET_MODULE_GLOBAL:
            {
                uint16_t ix = frame_read_uint16(vm->current_frame);
                mcvalue_t newvalue = stack_pop(vm);
                mcvalue_t oldvalue = vm_get_global(vm, ix);
                if(!check_assign(vm, oldvalue, newvalue))
                {
                    goto err;
                }
                vm_set_global(vm, ix, newvalue);
                break;
            }
            case OPCODE_GET_MODULE_GLOBAL:
            {
                uint16_t ix = frame_read_uint16(vm->current_frame);
                mcvalue_t global = vm->globals[ix];
                stack_push(vm, global);
                break;
            }
            case OPCODE_ARRAY:
            {
                uint16_t count = frame_read_uint16(vm->current_frame);
                mcvalue_t arrayobj = object_make_array_with_capacity(vm, count);
                if(object_is_null(arrayobj))
                {
                    goto err;
                }
                mcvalue_t* items = vm->stack + vm->sp - count;
                for(int i = 0; i < count; i++)
                {
                    mcvalue_t item = items[i];
                    ok = object_add_array_value(arrayobj, item);
                    if(!ok)
                    {
                        goto err;
                    }
                }
                set_sp(vm, vm->sp - count);
                stack_push(vm, arrayobj);
                break;
            }
            case OPCODE_MAP_START:
            {
                uint16_t count = frame_read_uint16(vm->current_frame);
                mcvalue_t mapobj = object_make_map_with_capacity(vm, count);
                if(object_is_null(mapobj))
                {
                    goto err;
                }
                this_stack_push(vm, mapobj);
                break;
            }
            case OPCODE_MAP_END:
            {
                uint16_t kvpcount = frame_read_uint16(vm->current_frame);
                uint16_t itemscount = kvpcount * 2;
                mcvalue_t mapobj = this_stack_pop(vm);
                mcvalue_t* kvpairs = vm->stack + vm->sp - itemscount;
                for(int i = 0; i < itemscount; i += 2)
                {
                    mcvalue_t key = kvpairs[i];
                    if(!object_is_hashable(key))
                    {
                        mcobjtype_t keytype = object_get_type(key);
                        const char* keytypename = object_get_type_name(keytype);
                        errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Key of type %s is not hashable", keytypename);
                        goto err;
                    }
                    mcvalue_t val = kvpairs[i + 1];
                    ok = object_set_map_value(mapobj, key, val);
                    if(!ok)
                    {
                        goto err;
                    }
                }
                set_sp(vm, vm->sp - itemscount);
                stack_push(vm, mapobj);
                break;
            }
            case OPCODE_GET_THIS:
            {
                mcvalue_t obj = this_stack_get(vm, 0);
                stack_push(vm, obj);
                break;
            }
            case OPCODE_GET_INDEX:
            {
                mcvalue_t index = stack_pop(vm);
                mcvalue_t left = stack_pop(vm);
                mcobjtype_t lefttype = object_get_type(left);
                mcobjtype_t indextype = object_get_type(index);
                const char* lefttypename = object_get_type_name(lefttype);
                const char* indextypename = object_get_type_name(indextype);

                if(lefttype != ARCANE_OBJECT_ARRAY && lefttype != ARCANE_OBJECT_MAP && lefttype != ARCANE_OBJECT_STRING)
                {
                    errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Type %s is not indexable", lefttypename);
                    goto err;
                }

                mcvalue_t res = mc_value_makenull();

                if(lefttype == ARCANE_OBJECT_ARRAY)
                {
                    if(indextype != ARCANE_OBJECT_NUMBER)
                    {
                        errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Cannot index %s with %s", lefttypename, indextypename);
                        goto err;
                    }
                    int ix = (int)object_get_number(index);
                    if(ix < 0)
                    {
                        ix = object_get_array_length(left) + ix;
                    }
                    if(ix >= 0 && ix < object_get_array_length(left))
                    {
                        res = object_get_array_value_at(left, ix);
                    }
                }
                else if(lefttype == ARCANE_OBJECT_MAP)
                {
                    res = object_get_map_value(left, index);
                }
                else if(lefttype == ARCANE_OBJECT_STRING)
                {
                    const char* str = object_get_string(left);
                    int leftlen = object_get_string_length(left);
                    int ix = (int)object_get_number(index);
                    if(ix >= 0 && ix < leftlen)
                    {
                        char resstr[2] = { str[ix], '\0' };
                        res = mc_value_makestring(vm, resstr);
                    }
                }
                stack_push(vm, res);
                break;
            }
            case OPCODE_GET_VALUE_AT:
            {
                int ix;
                mcvalue_t index = stack_pop(vm);
                mcvalue_t left = stack_pop(vm);
                mcobjtype_t lefttype = object_get_type(left);
                mcobjtype_t indextype = object_get_type(index);
                const char* lefttypename = object_get_type_name(lefttype);
                const char* indextypename = object_get_type_name(indextype);

                if(lefttype != ARCANE_OBJECT_ARRAY && lefttype != ARCANE_OBJECT_MAP && lefttype != ARCANE_OBJECT_STRING)
                {
                    errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Type %s is not indexable", lefttypename);
                    goto err;
                }

                mcvalue_t res = mc_value_makenull();
                if(indextype != ARCANE_OBJECT_NUMBER)
                {
                    errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Cannot index %s with %s", lefttypename, indextypename);
                    goto err;
                }
                ix = (int)object_get_number(index);

                if(lefttype == ARCANE_OBJECT_ARRAY)
                {
                    res = object_get_array_value_at(left, ix);
                }
                else if(lefttype == ARCANE_OBJECT_MAP)
                {
                    res = object_get_kv_pair_at(vm, left, ix);
                }
                else if(lefttype == ARCANE_OBJECT_STRING)
                {
                    const char* str = object_get_string(left);
                    int leftlen = object_get_string_length(left);
                    ix = (int)object_get_number(index);
                    if(ix >= 0 && ix < leftlen)
                    {
                        char resstr[2] = { str[ix], '\0' };
                        res = mc_value_makestring(vm, resstr);
                    }
                }
                stack_push(vm, res);
                break;
            }
            case OPCODE_CALL:
            {
                uint8_t num_args = frame_read_uint8(vm->current_frame);
                mcvalue_t callee = stack_get(vm, num_args);
                ok = call_object(vm, callee, num_args);
                if(!ok)
                {
                    goto err;
                }
                break;
            }
            case OPCODE_RETURN_VALUE:
            {
                mcvalue_t res = stack_pop(vm);
                ok = pop_frame(vm);
                if(!ok)
                {
                    goto end;
                }
                stack_push(vm, res);
                break;
            }
            case OPCODE_RETURN:
            {
                ok = pop_frame(vm);
                stack_push(vm, mc_value_makenull());
                if(!ok)
                {
                    stack_pop(vm);
                    goto end;
                }
                break;
            }
            case OPCODE_DEFINE_LOCAL:
            {
                uint8_t pos = frame_read_uint8(vm->current_frame);
                vm->stack[vm->current_frame->base_pointer + pos] = stack_pop(vm);
                break;
            }
            case OPCODE_SET_LOCAL:
            {
                uint8_t pos = frame_read_uint8(vm->current_frame);
                mcvalue_t newvalue = stack_pop(vm);
                mcvalue_t oldvalue = vm->stack[vm->current_frame->base_pointer + pos];
                if(!check_assign(vm, oldvalue, newvalue))
                {
                    goto err;
                }
                vm->stack[vm->current_frame->base_pointer + pos] = newvalue;
                break;
            }
            case OPCODE_GET_LOCAL:
            {
                uint8_t pos = frame_read_uint8(vm->current_frame);
                mcvalue_t val = vm->stack[vm->current_frame->base_pointer + pos];
                stack_push(vm, val);
                break;
            }
            case OPCODE_GET_ARCANE_GLOBAL:
            {
                uint16_t ix = frame_read_uint16(vm->current_frame);
                ok = false;
                mcvalue_t val = global_store_get_object_at(vm->global_store, ix, &ok);
                if(!ok)
                {
                    errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Global value %d not found", ix);
                    goto err;
                }
                stack_push(vm, val);
                break;
            }
            case OPCODE_FUNCTION:
            {
                uint16_t constantix = frame_read_uint16(vm->current_frame);
                uint8_t numfree = frame_read_uint8(vm->current_frame);
                mcvalue_t* constant = array_get(constants, constantix);
                if(!constant)
                {
                    errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Constant %d not found", constantix);
                    goto err;
                }
                mcobjtype_t constanttype = object_get_type(*constant);
                if(constanttype != ARCANE_OBJECT_FUNCTION)
                {
                    const char* typename = object_get_type_name(constanttype);
                    errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "%s is not a function", typename);
                    goto err;
                }

                mcobjfuncscript_t* constantfunction = object_get_function(*constant);
                mcvalue_t functionobj = object_make_function(vm, object_get_function_name(*constant), constantfunction->comp_result, false,
                                                             constantfunction->num_locals, constantfunction->num_args, numfree);
                if(object_is_null(functionobj))
                {
                    goto err;
                }
                for(int i = 0; i < numfree; i++)
                {
                    mcvalue_t freeval = vm->stack[vm->sp - numfree + i];
                    object_set_function_free_val(functionobj, i, freeval);
                }
                set_sp(vm, vm->sp - numfree);
                stack_push(vm, functionobj);
                break;
            }
            case OPCODE_GET_FREE:
            {
                uint8_t freeix = frame_read_uint8(vm->current_frame);
                mcvalue_t val = object_get_function_free_val(vm->current_frame->function, freeix);
                stack_push(vm, val);
                break;
            }
            case OPCODE_SET_FREE:
            {
                uint8_t freeix = frame_read_uint8(vm->current_frame);
                mcvalue_t val = stack_pop(vm);
                object_set_function_free_val(vm->current_frame->function, freeix, val);
                break;
            }
            case OPCODE_CURRENT_FUNCTION:
            {
                mcvalue_t currentfunction = vm->current_frame->function;
                stack_push(vm, currentfunction);
                break;
            }
            case OPCODE_SET_INDEX:
            {
                mcvalue_t index = stack_pop(vm);
                mcvalue_t left = stack_pop(vm);
                mcvalue_t newvalue = stack_pop(vm);
                mcobjtype_t lefttype = object_get_type(left);
                mcobjtype_t indextype = object_get_type(index);
                const char* lefttypename = object_get_type_name(lefttype);
                const char* indextypename = object_get_type_name(indextype);

                if(lefttype != ARCANE_OBJECT_ARRAY && lefttype != ARCANE_OBJECT_MAP)
                {
                    errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Type %s is not indexable", lefttypename);
                    goto err;
                }

                if(lefttype == ARCANE_OBJECT_ARRAY)
                {
                    if(indextype != ARCANE_OBJECT_NUMBER)
                    {
                        errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Cannot index %s with %s", lefttypename, indextypename);
                        goto err;
                    }
                    int ix = (int)object_get_number(index);
                    ok = object_set_array_value_at(left, ix, newvalue);
                    if(!ok)
                    {
                        errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Setting array item failed (out of bounds?)");
                        goto err;
                    }
                }
                else if(lefttype == ARCANE_OBJECT_MAP)
                {
                    mcvalue_t oldvalue = object_get_map_value(left, index);
                    if(!check_assign(vm, oldvalue, newvalue))
                    {
                        goto err;
                    }
                    ok = object_set_map_value(left, index, newvalue);
                    if(!ok)
                    {
                        goto err;
                    }
                }
                break;
            }
            case OPCODE_DUP:
            {
                mcvalue_t val = stack_get(vm, 0);
                stack_push(vm, val);
                break;
            }
            case OPCODE_LEN:
            {
                mcvalue_t val = stack_pop(vm);
                int len = 0;
                mcobjtype_t type = object_get_type(val);
                if(type == ARCANE_OBJECT_ARRAY)
                {
                    len = object_get_array_length(val);
                }
                else if(type == ARCANE_OBJECT_MAP)
                {
                    len = object_get_map_length(val);
                }
                else if(type == ARCANE_OBJECT_STRING)
                {
                    len = object_get_string_length(val);
                }
                else
                {
                    const char* typename = object_get_type_name(type);
                    errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Cannot get length of %s", typename);
                    goto err;
                }
                stack_push(vm, mc_value_makenumber(len));
                break;
            }
            case OPCODE_NUMBER:
            {
                uint64_t val = frame_read_uint64(vm->current_frame);
                double val_double = arcane_uint64_to_double(val);
                mcvalue_t obj = mc_value_makenumber(val_double);
                stack_push(vm, obj);
                break;
            }
            case OPCODE_SET_RECOVER:
            {
                uint16_t recover_ip = frame_read_uint16(vm->current_frame);
                vm->current_frame->recover_ip = recover_ip;
                break;
            }
            default:
            {
                ARCANE_ASSERT(false);
                errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Unknown opcode: 0x%x", opcode);
                goto err;
            }
        }

        if(checktime)
        {
            timecheckcounter++;
            if(timecheckcounter > timecheckinterval)
            {
                int elapsedms = (int)arcane_timer_get_elapsed_ms(&timer);
                if(elapsedms > maxexectimems)
                {
                    errors_add_errorf(&vm->errors, ARCANE_ERROR_TIMEOUT, frame_src_position(vm->current_frame), "Execution took more than %1.17g ms", maxexectimems);
                    goto err;
                }
                timecheckcounter = 0;
            }
        }
    err:
        if(errors_get_count(&vm->errors) > 0)
        {
            mcerror_t* err = errors_get_last_error(&vm->errors);
            if(err->type == ARCANE_ERROR_RUNTIME && errors_get_count(&vm->errors) == 1)
            {
                int recoverframeix = -1;
                for(int i = vm->frames_count - 1; i >= 0; i--)
                {
                    mcvmframe_t* frame = &vm->frames[i];
                    if(frame->recover_ip >= 0 && !frame->is_recovering)
                    {
                        recoverframeix = i;
                        break;
                    }
                }
                if(recoverframeix < 0)
                {
                    goto end;
                }
                if(!err->traceback)
                {
                    err->traceback = traceback_make(vm);
                }
                if(err->traceback)
                {
                    traceback_append_from_vm(err->traceback, vm);
                }
                while(vm->frames_count > (recoverframeix + 1))
                {
                    pop_frame(vm);
                }
                mcvalue_t errobj = object_make_error(vm, err->message);
                if(!object_is_null(errobj))
                {
                    object_set_error_traceback(errobj, err->traceback);
                    err->traceback = NULL;
                }
                stack_push(vm, errobj);
                vm->current_frame->ip = vm->current_frame->recover_ip;
                vm->current_frame->is_recovering = true;
                errors_clear(&vm->errors);
            }
            else
            {
                goto end;
            }
        }
        if(gc_should_sweep(vm->mem))
        {
            run_gc(vm, constants);
        }
    }

end:
    if(errors_get_count(&vm->errors) > 0)
    {
        mcerror_t* err = errors_get_last_error(&vm->errors);
        if(!err->traceback)
        {
            err->traceback = traceback_make(vm);
        }
        if(err->traceback)
        {
            traceback_append_from_vm(err->traceback, vm);
        }
    }

    run_gc(vm, constants);

    vm->running = false;
    return errors_get_count(&vm->errors) == 0;
}

mcvalue_t vm_get_last_popped(mcstate_t* vm)
{
    return vm->last_popped;
}

bool vm_has_errors(mcstate_t* vm)
{
    return errors_get_count(&vm->errors) > 0;
}

bool vm_set_global(mcstate_t* vm, int ix, mcvalue_t val)
{
    if(ix >= VM_MAX_GLOBALS)
    {
        ARCANE_ASSERT(false);
        errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Global write out of range");
        return false;
    }
    vm->globals[ix] = val;
    if(ix >= vm->globals_count)
    {
        vm->globals_count = ix + 1;
    }
    return true;
}

mcvalue_t vm_get_global(mcstate_t* vm, int ix)
{
    if(ix >= VM_MAX_GLOBALS)
    {
        ARCANE_ASSERT(false);
        errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Global read out of range");
        return mc_value_makenull();
    }
    return vm->globals[ix];
}

TMPSTATIC void set_sp(mcstate_t* vm, int newsp)
{
    if(newsp > vm->sp)
    {
        /* to avoid gcing freed objects */
        int count = newsp - vm->sp;
        size_t bytescount = count * sizeof(mcvalue_t);
        memset(vm->stack + vm->sp, 0, bytescount);
    }
    vm->sp = newsp;
}

TMPSTATIC void stack_push(mcstate_t* vm, mcvalue_t obj)
{
#ifdef ARCANE_DEBUG
    if(vm->sp >= VM_STACK_SIZE)
    {
        ARCANE_ASSERT(false);
        errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Stack overflow");
        return;
    }
    if(vm->current_frame)
    {
        mcvmframe_t* frame = vm->current_frame;
        mcobjfuncscript_t* currentfunction = object_get_function(frame->function);
        int num_locals = currentfunction->num_locals;
        ARCANE_ASSERT(vm->sp >= (frame->base_pointer + num_locals));
    }
#endif
    vm->stack[vm->sp] = obj;
    vm->sp++;
}

TMPSTATIC mcvalue_t stack_pop(mcstate_t* vm)
{
#ifdef ARCANE_DEBUG
    if(vm->sp == 0)
    {
        errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Stack underflow");
        ARCANE_ASSERT(false);
        return mc_value_makenull();
    }
    if(vm->current_frame)
    {
        mcvmframe_t* frame = vm->current_frame;
        mcobjfuncscript_t* currentfunction = object_get_function(frame->function);
        int num_locals = currentfunction->num_locals;
        ARCANE_ASSERT((vm->sp - 1) >= (frame->base_pointer + num_locals));
    }
#endif
    vm->sp--;
    mcvalue_t res = vm->stack[vm->sp];
    vm->last_popped = res;
    return res;
}

TMPSTATIC mcvalue_t stack_get(mcstate_t* vm, int nthitem)
{
    int ix = vm->sp - 1 - nthitem;
#ifdef ARCANE_DEBUG
    if(ix < 0 || ix >= VM_STACK_SIZE)
    {
        errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Invalid stack index: %d", nthitem);
        ARCANE_ASSERT(false);
        return mc_value_makenull();
    }
#endif
    return vm->stack[ix];
}

TMPSTATIC void this_stack_push(mcstate_t* vm, mcvalue_t obj)
{
#ifdef ARCANE_DEBUG
    if(vm->this_sp >= VM_THIS_STACK_SIZE)
    {
        ARCANE_ASSERT(false);
        errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "this stack overflow");
        return;
    }
#endif
    vm->this_stack[vm->this_sp] = obj;
    vm->this_sp++;
}

TMPSTATIC mcvalue_t this_stack_pop(mcstate_t* vm)
{
#ifdef ARCANE_DEBUG
    if(vm->this_sp == 0)
    {
        errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "this stack underflow");
        ARCANE_ASSERT(false);
        return mc_value_makenull();
    }
#endif
    vm->this_sp--;
    return vm->this_stack[vm->this_sp];
}

TMPSTATIC mcvalue_t this_stack_get(mcstate_t* vm, int nthitem)
{
    int ix = vm->this_sp - 1 - nthitem;
#ifdef ARCANE_DEBUG
    if(ix < 0 || ix >= VM_THIS_STACK_SIZE)
    {
        errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Invalid this stack index: %d", nthitem);
        ARCANE_ASSERT(false);
        return mc_value_makenull();
    }
#endif
    return vm->this_stack[ix];
}

TMPSTATIC bool push_frame(mcstate_t* vm, mcvmframe_t frame)
{
    if(vm->frames_count >= VM_MAX_FRAMES)
    {
        ARCANE_ASSERT(false);
        return false;
    }
    vm->frames[vm->frames_count] = frame;
    vm->current_frame = &vm->frames[vm->frames_count];
    vm->frames_count++;
    mcobjfuncscript_t* framefunction = object_get_function(frame.function);
    set_sp(vm, frame.base_pointer + framefunction->num_locals);
    return true;
}

TMPSTATIC bool pop_frame(mcstate_t* vm)
{
    set_sp(vm, vm->current_frame->base_pointer - 1);
    if(vm->frames_count <= 0)
    {
        ARCANE_ASSERT(false);
        vm->current_frame = NULL;
        return false;
    }
    vm->frames_count--;
    if(vm->frames_count == 0)
    {
        vm->current_frame = NULL;
        return false;
    }
    vm->current_frame = &vm->frames[vm->frames_count - 1];
    return true;
}

TMPSTATIC void run_gc(mcstate_t* vm, mcbasicarray_t* constants)
{
    gc_unmark_all(vm->mem);
    gc_mark_objects(global_store_get_object_data(vm->global_store), global_store_get_object_count(vm->global_store));
    gc_mark_objects(array_data(constants), array_count(constants));
    gc_mark_objects(vm->globals, vm->globals_count);
    for(int i = 0; i < vm->frames_count; i++)
    {
        mcvmframe_t* frame = &vm->frames[i];
        gc_mark_object(frame->function);
    }
    gc_mark_objects(vm->stack, vm->sp);
    gc_mark_objects(vm->this_stack, vm->this_sp);
    gc_mark_object(vm->last_popped);
    gc_mark_objects(vm->operator_oveload_keys, OPCODE_MAX);
    gc_sweep(vm->mem);
}

TMPSTATIC bool call_object(mcstate_t* vm, mcvalue_t callee, int num_args)
{
    bool ok;
    mcobjtype_t calleetype = object_get_type(callee);
    if(calleetype == ARCANE_OBJECT_FUNCTION)
    {
        mcobjfuncscript_t* calleefunction = object_get_function(callee);
        if(num_args != calleefunction->num_args)
        {
            errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Invalid number of arguments to \"%s\", expected %d, got %d",
                              object_get_function_name(callee), calleefunction->num_args, num_args);
            return false;
        }
        mcvmframe_t calleeframe;
        ok = frame_init(&calleeframe, callee, vm->sp - num_args);
        if(!ok)
        {
            errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, "Frame init failed in call_object");
            return false;
        }
        ok = push_frame(vm, calleeframe);
        if(!ok)
        {
            errors_add_error(&vm->errors, ARCANE_ERROR_RUNTIME, srcposinvalid, "Pushing frame failed in call_object");
            return false;
        }
    }
    else if(calleetype == ARCANE_OBJECT_NATIVE_FUNCTION)
    {
        mcvalue_t* stackpos = vm->stack + vm->sp - num_args;
        mcvalue_t res = call_native_function(vm, callee, frame_src_position(vm->current_frame), num_args, stackpos);
        if(vm_has_errors(vm))
        {
            return false;
        }
        set_sp(vm, vm->sp - num_args - 1);
        stack_push(vm, res);
    }
    else
    {
        const char* calleetypename = object_get_type_name(calleetype);
        errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "%s object is not callable", calleetypename);
        return false;
    }
    return true;
}

TMPSTATIC mcvalue_t call_native_function(mcstate_t* vm, mcvalue_t callee, mcastlocation_t srcpos, int argc, mcvalue_t* args)
{
    mcobjfuncnative_t* nativefun = object_get_native_function(callee);
    mcvalue_t res = nativefun->fn(vm, nativefun->data, argc, args);
    if(errors_has_errors(&vm->errors) && !ARCANE_STREQ(nativefun->name, "crash"))
    {
        mcerror_t* err = errors_get_last_error(&vm->errors);
        err->pos = srcpos;
        err->traceback = traceback_make(vm);
        if(err->traceback)
        {
            traceback_append(err->traceback, nativefun->name, srcposinvalid);
        }
        return mc_value_makenull();
    }
    mcobjtype_t restype = object_get_type(res);
    if(restype == ARCANE_OBJECT_ERROR)
    {
        mctraceback_t* traceback = traceback_make(vm);
        if(traceback)
        {
            /* error builtin is treated in a special way */
            if(!ARCANE_STREQ(nativefun->name, "error"))
            {
                traceback_append(traceback, nativefun->name, srcposinvalid);
            }
            traceback_append_from_vm(traceback, vm);
            object_set_error_traceback(res, traceback);
        }
    }
    return res;
}

TMPSTATIC bool check_assign(mcstate_t* vm, mcvalue_t oldvalue, mcvalue_t newvalue)
{
    mcobjtype_t oldvaluetype = object_get_type(oldvalue);
    mcobjtype_t newvaluetype = object_get_type(newvalue);
    if(oldvaluetype == ARCANE_OBJECT_NULL || newvaluetype == ARCANE_OBJECT_NULL)
    {
        return true;
    }
    if(oldvaluetype != newvaluetype)
    {
        errors_add_errorf(&vm->errors, ARCANE_ERROR_RUNTIME, frame_src_position(vm->current_frame), "Trying to assign variable of type %s to %s",
                          object_get_type_name(newvaluetype), object_get_type_name(oldvaluetype));
        return false;
    }
    return true;
}

TMPSTATIC bool try_overload_operator(mcstate_t* vm, mcvalue_t left, mcvalue_t right, opcode_t op, bool* outoverloadfound)
{
    *outoverloadfound = false;
    mcobjtype_t lefttype = object_get_type(left);
    mcobjtype_t righttype = object_get_type(right);
    if(lefttype != ARCANE_OBJECT_MAP && righttype != ARCANE_OBJECT_MAP)
    {
        *outoverloadfound = false;
        return true;
    }

    int num_operands = 2;
    if(op == OPCODE_MINUS || op == OPCODE_BANG)
    {
        num_operands = 1;
    }

    mcvalue_t key = vm->operator_oveload_keys[op];
    mcvalue_t callee = mc_value_makenull();
    if(lefttype == ARCANE_OBJECT_MAP)
    {
        callee = object_get_map_value(left, key);
    }
    if(!object_is_callable(callee))
    {
        if(righttype == ARCANE_OBJECT_MAP)
        {
            callee = object_get_map_value(right, key);
        }

        if(!object_is_callable(callee))
        {
            *outoverloadfound = false;
            return true;
        }
    }

    *outoverloadfound = true;

    stack_push(vm, callee);
    stack_push(vm, left);
    if(num_operands == 2)
    {
        stack_push(vm, right);
    }
    return call_object(vm, callee, num_operands);
}

TMPSTATIC void print_ape_errors(mcstate_t* state)
{
    for(int i = 0; i < arcane_errors_count(state); i++)
    {
        mcerror_t* err = arcane_get_error(state, i);
        char* errstr = arcane_error_serialize(state, err);
        puts(errstr);
        arcane_free_allocated(state, errstr);
    }
}


char* read_file(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if(file == NULL)
    {
        perror("Error opening file");
        return NULL;
    }
    /* Determine the size of the file */
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    char* filecontents = (char*)calloc(filesize, sizeof(char) + 1);
    if(filecontents == NULL)
    {
        perror("Error allocating memory");
        fclose(file);
        return NULL;
    }
    size_t result = fread(filecontents, sizeof(char), filesize, file);
    filecontents[filesize] = '\0';

    if(result != (size_t)filesize)
    {
        perror("Error reading file");
        free(filecontents);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return filecontents;
}


static int g_extfnvar;

int main(int argc, char** argv)
{
    (void)argc;
    char* code;
    g_extfnvar = 0;
    code = read_file(argv[1]);
    mcstate_t* state = mc_state_make(NULL);
    arcane_set_global_constant(state, "test", mc_value_makenumber(42));
    arcane_set_native_function(state, "external_fn_test", cfn_externalfn, &g_extfnvar);
    arcane_set_native_function(state, "test_check_args", cfn_test_check_args, NULL);
    arcane_set_native_function(state, "vec2_add", vec2_add_fun, NULL);
    arcane_set_native_function(state, "vec2_sub", vec2_sub_fun, NULL);
    mccompiledprogram_t* program = mc_state_compilesource(state, code);
    arcane_execute_program(state, program);
    if(arcane_has_errors(state))
    {
        print_ape_errors(state);
    }
    arcane_program_destroy(program);
    arcane_destroy(state);
    free(code);
    code = NULL;
}

