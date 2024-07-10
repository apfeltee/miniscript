
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#if defined(__unix__)
    #include <unistd.h>
    #include <sys/time.h>
#endif

#include "optparse.h"
/*
**funcnames***
*/
#if 0
{

collections_strndup
collections_strdup
hash_string
src_pos_make
compilation_scope_make
compilation_scope_destroy
compilation_scope_orphan_result
compilation_result_make
compilation_result_destroy
opcode_lookup
opcode_get_name
parser_make
parser_destroy
parser_parse_all
parse_statement
parse_define_statement
parse_if_statement
parse_return_statement
parse_expression_statement
parse_while_loop_statement
parse_break_statement
parse_continue_statement
parse_block_statement
parse_import_statement
parse_recover_statement
parse_for_loop_statement
parse_foreach
parse_classic_for_loop
parse_function_statement
parse_code_block
parse_expression
mc_parser_parseident
mc_parser_parseliteralnumber
mc_parser_parseliteralbool
mc_parser_parseliteralstring
mc_parser_parseliteraltemplatestring
mc_parser_parseliteralnull
mc_parser_parseliteralarray
mc_parser_parseliteralmap
mc_parser_parseprefixexpr
mc_parser_parseinfixexpr
mc_parser_parsegroupedexpr
mc_parser_parseliteralfunction
parse_function_parameters
parse_call_expression
parse_expression_list
mc_parser_parseindexexpr
mc_parser_parseassignexpr
parse_logical_expression
parse_ternary_expression
mc_parser_parseincdecprefixexpr
parse_incdec_postfix_expression
parse_dot_expression
get_precedence
token_to_operator
escape_char
process_and_copy_string
wrap_expression_in_function_call
object_get_type
object_is_numeric
object_is_null
object_is_callable
object_make_native_function
object_make_array
object_make_array_with_capacity
object_make_map
object_make_map_with_capacity
object_make_error
object_make_error_no_copy
object_make_errorf
object_make_function
object_make_external
object_deinit
object_data_deinit
object_is_allocated
object_get_mem
reset_state
set_default_config
read_file_default
write_file_default
stdout_write_default
expression_make_ident
expression_make_number_literal
expression_make_bool_literal
expression_make_string_literal
expression_make_null_literal
expression_make_array_literal
expression_make_map_literal
expression_make_prefix
expression_make_infix
expression_make_fn_literal
expression_make_call
expression_make_index
expression_make_assign
expression_make_logical
expression_make_ternary
expression_destroy
expression_copy
statement_make_define
statement_make_if
statement_make_return
statement_make_expression
statement_make_while_loop
statement_make_break
statement_make_foreach
statement_make_for_loop
statement_make_continue
statement_make_block
statement_make_import
statement_make_recover
statement_destroy
statement_copy
code_block_make
code_block_destroy
code_block_copy
operator_to_string
expression_type_to_string
ident_make
ident_copy
ident_destroy
if_case_make
if_case_destroy
if_case_copy
expression_make
statement_make
builtins_count
builtins_get_fn
builtins_get_name
code_to_string
code_read_operands
kg_split_string
kg_join
kg_canonicalise_path
kg_is_path_absolute
kg_streq
errors_get
errors_getc
error_type_to_string
errors_get_last_error
errors_has_errors
frame_init
frame_read_uint64
frame_read_uint16
frame_read_uint8
frame_read_opcode
frame_src_position
gcmem_make
gcmem_destroy
gcmem_alloc_object_data
gcmem_get_object_data_from_pool
gc_unmark_all
gc_mark_objects
gc_mark_object
gc_sweep
gc_disable_on_object
gc_enable_on_object
gc_should_sweep
get_pool_for_type
can_data_be_put_in_pool
global_store_make
global_store_destroy
global_store_get_symbol
global_store_set
global_store_get_object_at
global_store_get_object_data
global_store_get_object_count
object_is_hashable
object_get_type_name
object_get_type_union_name
object_serialize
object_equals
object_get_external_data
object_set_external_destroy_function
object_get_allocated_data
object_get_bool
object_get_number
object_set_string_length
object_equals_wrapped
object_hash
object_hash_string
object_hash_double
object_get_allocated_array
object_is_number
freevals_are_allocated
block_scope_make
block_scope_destroy
block_scope_copy
token_init
token_duplicate_literal
token_type_to_string
traceback_make
traceback_destroy
traceback_append
traceback_append_from_vm
traceback_to_string
traceback_item_get_line
traceback_item_get_filepath
vm_make
vm_destroy
vm_reset
vm_call
vm_get_last_popped
vm_has_errors
vm_set_global
vm_get_global
set_sp
stack_push
stack_pop
stack_get
this_stack_push
this_stack_pop
this_stack_get
push_frame
pop_frame
run_gc
check_assign
try_overload_operator
read_file
main
}
#endif

/*
SPDX-License-Identifier: MIT

https://github.com/kgabis/ape
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

#define MC_CONF_DEBUG 0

#if 0
    #define TMPSTATIC static inline  __attribute__((always_inline))
    #define TMPSEMISTATIC static inline
#else
    #if 1
        #define TMPSTATIC static
        #define TMPSEMISTATIC static
    #else
        #if defined(__STRICT_ANSI__)
            #define TMPSTATIC static
            #define TMPSEMISTATIC static
        #else
            #define TMPSTATIC static inline
            #define TMPSEMISTATIC static
        #endif
    #endif
#endif

#define TMPUNUSED

#ifdef _MSC_VER
    #define __attribute__(x)
#endif

#ifdef COLLECTIONS_DEBUG
    #define COLLECTIONS_ASSERT(x) assert(x)
#else
    #define COLLECTIONS_ASSERT(x)
#endif

#define MC_UTIL_STREQ(a, b) (strcmp((a), (b)) == 0)
#define MC_UTIL_STRNEQ(a, b, n) (strncmp((a), (b), (n)) == 0)
#define MC_UTIL_STATICARRAYSIZE(array) ((int)(sizeof(array) / sizeof(array[0])))
#define MC_UTIL_CMPFLOAT(a, b) (fabs((a) - (b)) < DBL_EPSILON)

#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    #define MC_ASSERT(x) assert((x))
#else
    #define MC_ASSERT(x) ((void)0)
#endif

#define IS_NULLSTR(str) ((str) == NULL || (str)[0] == '\0')

#define VM_STACK_SIZE 2048
#define VM_MAX_GLOBALS 2048
#define VM_MAX_FRAMES 2048
#define VM_THIS_STACK_SIZE 2048
#define NATIVE_FN_MAX_DATA_LEN 24

#define MC_CONF_OBJECT_STRING_BUF_SIZE 24

#define GCMEM_POOL_SIZE 2048
#define GCMEM_POOLS_NUM 3
#define GCMEM_SWEEP_INTERVAL 128

#define MC_CONF_ERROR_MAXERRORCOUNT 16
#define MC_CONF_ERROR_MSGMAXLENGTH 255
#define DICT_INVALID_IX UINT_MAX
#define DICT_INITIAL_SIZE 32

#define VALDICT_INVALID_IX UINT_MAX


enum mcerrtype_t
{
    MC_ERROR_NONE = 0,
    MC_ERROR_PARSING,
    MC_ERROR_COMPILING,
    MC_ERROR_RUNTIME,
    MC_ERROR_TIMEOUT,
    MC_ERROR_MEMORY,
    MC_ERROR_USER,
};

enum mcobjtype_t
{
    MC_OBJ_NONE = 0,
    MC_OBJ_ERROR = 1 << 0,
    MC_OBJ_NUMBER = 1 << 1,
    MC_OBJ_BOOL = 1 << 2,
    MC_OBJ_STRING = 1 << 3,
    MC_OBJ_NULL = 1 << 4,
    MC_OBJ_NATIVE_FUNCTION = 1 << 5,
    MC_OBJ_ARRAY = 1 << 6,
    MC_OBJ_MAP = 1 << 7,
    MC_OBJ_FUNCTION = 1 << 8,
    MC_OBJ_EXTERNAL = 1 << 9,
    MC_OBJ_FREED = 1 << 10,
    /* for checking types with & */
    MC_OBJ_ANY = 0xffff,
};

enum mcasttoktype_t
{
    MC_TOK_INVALID = 0,
    MC_TOK_EOF,

    /* Operators */
    MC_TOK_ASSIGN,

    MC_TOK_PLUSASSIGN,
    MC_TOK_MINUSASSIGN,
    MC_TOK_ASTERISKASSIGN,
    MC_TOK_SLASHASSIGN,
    MC_TOK_PERCENTASSIGN,
    MC_TOK_BITANDASSIGN,
    MC_TOK_BITORASSIGN,
    MC_TOK_BITXORASSIGN,
    MC_TOK_LSHIFTASSIGN,
    MC_TOK_RSHIFTASSIGN,

    MC_TOK_QUESTION,

    MC_TOK_PLUS,
    MC_TOK_PLUSPLUS,
    MC_TOK_MINUS,
    MC_TOK_MINUSMINUS,
    MC_TOK_BANG,
    MC_TOK_ASTERISK,
    MC_TOK_SLASH,

    MC_TOK_LT,
    MC_TOK_LTE,
    MC_TOK_GT,
    MC_TOK_GTE,

    MC_TOK_EQ,
    MC_TOK_NOTEQ,

    MC_TOK_AND,
    MC_TOK_OR,

    MC_TOK_BITAND,
    MC_TOK_BITOR,
    MC_TOK_BITXOR,
    MC_TOK_LSHIFT,
    MC_TOK_RSHIFT,

    /* Delimiters */
    MC_TOK_COMMA,
    MC_TOK_SEMICOLON,
    MC_TOK_COLON,
    MC_TOK_LPAREN,
    MC_TOK_RPAREN,
    MC_TOK_LBRACE,
    MC_TOK_RBRACE,
    MC_TOK_LBRACKET,
    MC_TOK_RBRACKET,
    MC_TOK_DOT,
    MC_TOK_PERCENT,

    /* Keywords */
    MC_TOK_FUNCTION,
    MC_TOK_CONST,
    MC_TOK_VAR,
    MC_TOK_TRUE,
    MC_TOK_FALSE,
    MC_TOK_IF,
    MC_TOK_ELSE,
    MC_TOK_RETURN,
    MC_TOK_WHILE,
    MC_TOK_BREAK,
    MC_TOK_FOR,
    MC_TOK_IN,
    MC_TOK_CONTINUE,
    MC_TOK_NULL,
    MC_TOK_IMPORT,
    MC_TOK_RECOVER,

    /* Identifiers and literals */
    MC_TOK_IDENT,
    MC_TOK_NUMBER,
    MC_TOK_STRING,
    MC_TOK_TEMPLATESTRING,

    MC_TOK_TYPEMAX
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
    OPCODE_GETGLOBALBUILTIN,
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
    SYMBOL_GLOBALBUILTIN,
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

typedef uint8_t mcinternopcode_t;


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

typedef struct mcastlexer_t mcastlexer_t;
typedef struct mcvmframe_t mcvmframe_t;
typedef struct mctraceitem_t mctraceitem_t;
typedef struct module_t module_t;
typedef struct mcnatfnbox_t mcnatfnbox_t;

typedef mcvalue_t (*mcnativefn_t)(mcstate_t* state, void* data, int argc, mcvalue_t* args);
typedef size_t (*mcstdoutwritefn_t)(void* context, const void* data, size_t datasize);
typedef char* (*mcreadfilefn_t)(void* context, const char* path);
typedef size_t (*mcwritefilefn_t)(void* context, const char* path, const char* string, size_t stringsize);
typedef unsigned long (*mccollectionshashfn_t)(void* val);
typedef bool (*mccollectionsequalsfn_t)(void* a, void* b);
typedef void (*mcdictitemdestroyfn_t)(void* item);
typedef void* (*mcdictitemcopyfn_t)(void* item);
typedef void (*mcarrayitemdeinitfn_t)(void* item);
typedef void (*mcptrarrayitemdestroyfn_t)(void* item);
typedef void* (*mcptrarrayitemcopyfn_t)(void* item);
typedef mcvalue_t (*mcnativeactualfn_t)(mcstate_t* state, void* data, int argc, mcvalue_t* args);
typedef void (*mcextdatadestroyfn_t)(void* data);
typedef void* (*mcextdatacopyfn_t)(void* data);
typedef mcastexpression_t* (*mcastrightassocparsefn_t)(mcastparser_t* p);
typedef mcastexpression_t* (*mcleftassocparsefn_t)(mcastparser_t* p, mcastexpression_t* expr);


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
        mcstdoutwritefn_t fnprintto;
        void* context;
    } stdio;
    struct
    {
        mcreadfilefn_t fnreadfile;
        mcwritefilefn_t fnwritefile;
        void* context;
    } fileio;
    /* allows redefinition of symbols */
    bool replmode;
    double maxexecutiontime;
    bool havemaxexectime;
};

struct mctimer_t
{
    int64_t startoffset;
    double starttimems;
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
    mcastexpression_t* ifcond;
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
    mcastexpression_t* tercond;
    mcastexpression_t* teriftrue;
    mcastexpression_t* teriffalse;
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
    mcastexpression_t* loopcond;
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
    mcastexpression_t* loopcond;
    mcastexpression_t* update;
    code_block_t* body;
};

struct import_statement_t
{
    char* path;
};

struct recover_statement_t
{
    ident_t* errident;
    code_block_t* body;
};

struct mcastexpression_t
{
    mcstate_t* pstate;
    mcastexprtype_t type;

    union
    {
        ident_t* exprident;
        double exprlitnumber;
        bool exprlitbool;
        struct {
            size_t length;
            char* data;
        } exprlitstring;
        mcptrarray_t* exprlitarray;
        map_literal_t exprlitmap;
        prefix_expression_t exprprefix;
        infix_expression_t exprinfix;
        fn_literal_t exprlitfunction;
        call_expression_t exprcall;
        index_expression_t exprindex;
        assign_expression_t exprassign;
        logical_expression_t exprlogical;
        ternary_expression_t exprternary;

        define_statement_t exprdefine;
        if_statement_t exprifstmt;
        mcastexpression_t* exprreturnvalue;
        mcastexpression_t* exprexpression;
        while_loop_statement_t exprwhileloopstmt;
        foreach_statement_t exprforeachloopstmt;
        for_loop_statement_t exprforloopstmt;
        code_block_t* exprblockstmt;
        import_statement_t exprimportstmt;
        recover_statement_t exprrecoverstmt;
    };

    mcastlocation_t pos;
};


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

struct mcobjfuncnative_t
{
    char* name;
    mcnativeactualfn_t fn;
    uint8_t data[NATIVE_FN_MAX_DATA_LEN];
    int data_len;
};

struct mcobjexternal_t
{
    void* data;
    mcextdatadestroyfn_t data_destroy_fn;
    mcextdatacopyfn_t data_copy_fn;
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
        mcobjstring_t valstring;
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
    mcinternopcode_t last_opcode;
};


struct mcastcompiledfile_t
{
    mcstate_t* pstate;
    char* dir_path;
    char* path;
    mcptrarray_t* lines;
};



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
    const char* inputsource;
    int inputlength;
    int position;
    int nextposition;
    char ch;
    int line;
    int column;
    mcastcompiledfile_t* file;
    bool failed;
    bool continuetplstring;

    struct
    {
        int position;
        int nextposition;
        char ch;
        int line;
        int column;
    } prevstate;

    mcasttoken_t prevtoken;
    mcasttoken_t currtoken;
    mcasttoken_t peektoken;
};

struct mcastparser_t
{
    mcstate_t* pstate;
    mcconfig_t* config;
    mcastlexer_t lexer;
    mcerrlist_t* errors;
    mcastrightassocparsefn_t rightassocfuncs[MC_TOK_TYPEMAX];
    mcleftassocparsefn_t leftassocfuncs[MC_TOK_TYPEMAX];
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
    unsigned char* allocdata;
    unsigned int count;
    unsigned int capacity;
    size_t typesize;
    bool caplocked;
};

struct mcptrarray_t
{
    mcstate_t* pstate;
    mcbasicarray_t innerbarray;
};

struct mcvaldict_t
{
    mcstate_t* pstate;
    size_t keytypesize;
    size_t valtypesize;
    unsigned int* cells;
    unsigned long* hashes;
    void* keys;
    void* values;
    unsigned int* cellindices;
    unsigned int count;
    unsigned int itemcapacity;
    unsigned int cellcapacity;
    mccollectionshashfn_t funchashfn;
    mccollectionsequalsfn_t funckeyequalsfn;
};

struct mcgenericdict_t
{
    mcstate_t* pstate;
    unsigned int* cells;
    unsigned long* hashes;
    char** keys;
    void** values;
    unsigned int* cellindices;
    unsigned int count;
    unsigned int itemcapacity;
    unsigned int cellcapacity;
    mcdictitemcopyfn_t funccopyfn;
    mcdictitemdestroyfn_t destroy_fn;
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
    mcnativefn_t fn;
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

TMPSTATIC unsigned long mc_util_hashdata(void* ptr, size_t len)
{
    /* djb2 */
    size_t i;
    unsigned long hash;
    uint8_t val;
    uint8_t* up;
    up = (uint8_t*)ptr;
    hash = 5381;
    for(i = 0; i < len; i++)
    {
        val = up[i];
        hash = ((hash << 5) + hash) + val;
    }
    return hash;
}

TMPSTATIC unsigned int mc_util_upperpowoftwo(unsigned int v)
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
    
TMPSTATIC void* mc_allocator_alloc(mcstate_t* state, size_t size)
{
    (void)state;
    return malloc(size);
}

TMPSTATIC void mc_allocator_free(mcstate_t* state, void* ptr)
{
    (void)state;
    free(ptr);
}


typedef uint32_t mcshiftint_t;
TMPSTATIC double mc_mathutil_binshiftleft(double dnleft, double dnright)
{
    int64_t sileft;
    int64_t siright;
    mcshiftint_t ivleft;
    mcshiftint_t ivright;
    if((dnleft < 0) || (dnright < 0))
    {
        sileft = (int64_t)dnleft;
        siright = (int64_t)dnright;
        return (double)(sileft << siright);
    }
    ivleft = (int32_t)dnleft;
    ivright = (mcshiftint_t)dnright;
    ivright &= 0x1f;
    return (double)(ivleft << ivright);
}

TMPSTATIC double mc_mathutil_binshiftright(double dnleft, double dnright)
{
    int64_t sileft;
    int64_t siright;
    mcshiftint_t ivleft;
    mcshiftint_t ivright;
    if((dnleft < 0) || (dnright < 0))
    {
        sileft = (int64_t)dnleft;
        siright = (int64_t)dnright;
        return (double)(sileft >> siright);
    }
    ivleft = (mcshiftint_t)dnleft;
    ivright = (mcshiftint_t)dnright;
    ivright &= 0x1f;
    return (double)(ivleft >> ivright);
}

TMPSTATIC double mc_mathutil_binor(double dnleft, double dnright)
{
    int64_t ivleft;
    int64_t ivright;
    ivleft = (int64_t)dnleft;
    ivright = (int64_t)dnright;
    return (double)(ivleft | ivright);
}

TMPSTATIC double mc_mathutil_binand(double dnleft, double dnright)
{
    int64_t ivleft;
    int64_t ivright;
    ivleft = (int64_t)dnleft;
    ivright = (int64_t)dnright;
    return (double)(ivleft & ivright);
}

TMPSTATIC double mc_mathutil_binxor(double dnleft, double dnright)
{
    int64_t ivleft;
    int64_t ivright;
    ivleft = (int64_t)dnleft;
    ivright = (int64_t)dnright;
    return (double)(ivleft ^ ivright);
}

TMPSTATIC double mc_mathutil_add(double dnleft, double dnright)
{
    return dnleft + dnright;
}

TMPSTATIC double mc_mathutil_sub(double dnleft, double dnright)
{
    return dnleft - dnright;
}

TMPSTATIC double mc_mathutil_mult(double dnleft, double dnright)
{
    return dnleft * dnright;
}

TMPSTATIC double mc_mathutil_div(double dnleft, double dnright)
{
    return dnleft / dnright;
}

TMPSTATIC double mc_mathutil_mod(double dnleft, double dnright)
{
    return fmod(dnleft, dnright);
}

TMPSTATIC char* collections_strndup(mcstate_t* state, const char* string, size_t n)
{
    char* outputstring = (char*)mc_allocator_alloc(state, n + 1);
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



TMPSTATIC mcgenericdict_t* mc_genericdict_make(mcstate_t* state, mcdictitemcopyfn_t copyfn, mcdictitemdestroyfn_t destroy_fn)
{
    bool ok;
    mcgenericdict_t* dict = (mcgenericdict_t*)mc_allocator_alloc(state, sizeof(mcgenericdict_t));
    if(dict == NULL)
    {
        return NULL;
    }
    ok = mc_genericdict_init(dict, state, DICT_INITIAL_SIZE, copyfn, destroy_fn);
    if(!ok)
    {
        mc_allocator_free(state, dict);
        return NULL;
    }
    dict->pstate = state;
    return dict;
}

TMPSTATIC void mc_genericdict_destroy(mcgenericdict_t* dict)
{
    mcstate_t* state;
    if(!dict)
    {
        return;
    }
    state = dict->pstate;
    mc_genericdict_deinit(dict, true);
    mc_allocator_free(state, dict);
}

TMPSTATIC void mc_genericdict_destroyitemsanddict(mcgenericdict_t* dict)
{
    unsigned int i;
    if(!dict)
    {
        return;
    }
    if(dict->destroy_fn)
    {
        for(i = 0; i < dict->count; i++)
        {
            dict->destroy_fn(dict->values[i]);
        }
    }
    mc_genericdict_destroy(dict);
}

TMPSTATIC mcgenericdict_t* mc_genericdict_copy(mcgenericdict_t* dict)
{
    bool ok;
    int i;
    if(!dict->funccopyfn || !dict->destroy_fn)
    {
        return NULL;
    }

    mcgenericdict_t* dictcopy = mc_genericdict_make(dict->pstate, dict->funccopyfn, dict->destroy_fn);
    if(!dictcopy)
    {
        return NULL;
    }
    dictcopy->pstate = dict->pstate;
    for(i = 0; i < mc_genericdict_count(dict); i++)
    {
        const char* key = mc_genericdict_getkeyat(dict, i);
        void* item = mc_genericdict_getvalueat(dict, i);
        void* itemcopy = dictcopy->funccopyfn(item);
        if(item && !itemcopy)
        {
            mc_genericdict_destroyitemsanddict(dictcopy);
            return NULL;
        }
        ok = mc_genericdict_set(dictcopy, key, itemcopy);
        if(!ok)
        {
            dictcopy->destroy_fn(itemcopy);
            mc_genericdict_destroyitemsanddict(dictcopy);
            return NULL;
        }
    }
    return dictcopy;
}

TMPSTATIC bool mc_genericdict_set(mcgenericdict_t* dict, const char* key, void* value)
{
    return mc_genericdict_setinternal(dict, key, NULL, value);
}

TMPSTATIC void* mc_genericdict_get(mcgenericdict_t* dict, const char* key)
{
    unsigned long hash = hash_string(key);
    bool found = false;
    unsigned long cellix = mc_genericdict_getcellindex(dict, key, hash, &found);
    if(found == false)
    {
        return NULL;
    }
    unsigned int itemix = dict->cells[cellix];
    return dict->values[itemix];
}

TMPSTATIC void* mc_genericdict_getvalueat(mcgenericdict_t* dict, unsigned int ix)
{
    if(ix >= dict->count)
    {
        return NULL;
    }
    return dict->values[ix];
}

TMPSTATIC const char* mc_genericdict_getkeyat(mcgenericdict_t* dict, unsigned int ix)
{
    if(ix >= dict->count)
    {
        return NULL;
    }
    return dict->keys[ix];
}

TMPSTATIC int mc_genericdict_count(mcgenericdict_t* dict)
{
    if(!dict)
    {
        return 0;
    }
    return dict->count;
}

TMPUNUSED bool mc_genericdict_remove(mcgenericdict_t* dict, const char* key)
{
    unsigned int x;
    unsigned long hash = hash_string(key);
    bool found = false;
    unsigned int cell = mc_genericdict_getcellindex(dict, key, hash, &found);
    if(!found)
    {
        return false;
    }

    unsigned int itemix = dict->cells[cell];
    mc_allocator_free(dict->pstate, dict->keys[itemix]);
    unsigned int lastitemix = dict->count - 1;
    if(itemix < lastitemix)
    {
        dict->keys[itemix] = dict->keys[lastitemix];
        dict->values[itemix] = dict->values[lastitemix];
        dict->cellindices[itemix] = dict->cellindices[lastitemix];
        dict->hashes[itemix] = dict->hashes[lastitemix];
        dict->cells[dict->cellindices[itemix]] = itemix;
    }
    dict->count--;

    unsigned int i = cell;
    unsigned int j = i;
    for(x = 0; x < (dict->cellcapacity - 1); x++)
    {
        j = (j + 1) & (dict->cellcapacity - 1);
        if(dict->cells[j] == DICT_INVALID_IX)
        {
            break;
        }
        unsigned int k = (unsigned int)(dict->hashes[dict->cells[j]]) & (dict->cellcapacity - 1);
        if((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j)))
        {
            dict->cellindices[dict->cells[j]] = i;
            dict->cells[i] = dict->cells[j];
            i = j;
        }
    }
    dict->cells[i] = DICT_INVALID_IX;
    return true;
}

TMPSTATIC bool mc_genericdict_init(mcgenericdict_t* dict, mcstate_t* state, unsigned int initialcapacity, mcdictitemcopyfn_t copyfn, mcdictitemdestroyfn_t destroy_fn)
{
    unsigned int i;
    dict->pstate = state;
    dict->cells = NULL;
    dict->keys = NULL;
    dict->values = NULL;
    dict->cellindices = NULL;
    dict->hashes = NULL;
    dict->count = 0;
    dict->cellcapacity = initialcapacity;
    dict->itemcapacity = (unsigned int)(initialcapacity * 0.7f);
    dict->funccopyfn = copyfn;
    dict->destroy_fn = destroy_fn;
    dict->cells = (unsigned int*)mc_allocator_alloc(dict->pstate, dict->cellcapacity * sizeof(*dict->cells));
    dict->keys = (char**)mc_allocator_alloc(dict->pstate, dict->itemcapacity * sizeof(*dict->keys));
    dict->values = (void**)mc_allocator_alloc(dict->pstate, dict->itemcapacity * sizeof(*dict->values));
    dict->cellindices = (unsigned int*)mc_allocator_alloc(dict->pstate, dict->itemcapacity * sizeof(*dict->cellindices));
    dict->hashes = (long unsigned int*)mc_allocator_alloc(dict->pstate, dict->itemcapacity * sizeof(*dict->hashes));
    if(dict->cells == NULL || dict->keys == NULL || dict->values == NULL || dict->cellindices == NULL || dict->hashes == NULL)
    {
        goto error;
    }
    for(i = 0; i < dict->cellcapacity; i++)
    {
        dict->cells[i] = DICT_INVALID_IX;
    }
    return true;
error:
    mc_allocator_free(dict->pstate, dict->cells);
    mc_allocator_free(dict->pstate, dict->keys);
    mc_allocator_free(dict->pstate, dict->values);
    mc_allocator_free(dict->pstate, dict->cellindices);
    mc_allocator_free(dict->pstate, dict->hashes);
    return false;
}

TMPSTATIC void mc_genericdict_deinit(mcgenericdict_t* dict, bool freekeys)
{
    unsigned int i;
    if(freekeys)
    {
        for(i = 0; i < dict->count; i++)
        {
            mc_allocator_free(dict->pstate, dict->keys[i]);
        }
    }
    dict->count = 0;
    dict->itemcapacity = 0;
    dict->cellcapacity = 0;
    mc_allocator_free(dict->pstate, dict->cells);
    mc_allocator_free(dict->pstate, dict->keys);
    mc_allocator_free(dict->pstate, dict->values);
    mc_allocator_free(dict->pstate, dict->cellindices);
    mc_allocator_free(dict->pstate, dict->hashes);
    dict->cells = NULL;
    dict->keys = NULL;
    dict->values = NULL;
    dict->cellindices = NULL;
    dict->hashes = NULL;
}

TMPSTATIC unsigned int mc_genericdict_getcellindex(mcgenericdict_t* dict, const char* key, unsigned long hash, bool* outfound)
{
    unsigned int i;
    *outfound = false;
    unsigned int cellix = (unsigned int)hash & (dict->cellcapacity - 1);
    for(i = 0; i < dict->cellcapacity; i++)
    {
        unsigned int ix = (cellix + i) & (dict->cellcapacity - 1);
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

TMPSEMISTATIC bool mc_genericdict_growandrehash(mcgenericdict_t* dict)
{
    bool ok;
    unsigned int i;
    mcgenericdict_t newdict;
    ok = mc_genericdict_init(&newdict, dict->pstate, dict->cellcapacity * 2, dict->funccopyfn, dict->destroy_fn);
    if(!ok)
    {
        return false;
    }
    for(i = 0; i < dict->count; i++)
    {
        char* key = dict->keys[i];
        void* value = dict->values[i];
        ok = mc_genericdict_setinternal(&newdict, key, key, value);
        if(!ok)
        {
            mc_genericdict_deinit(&newdict, false);
            return false;
        }
    }
    mc_genericdict_deinit(dict, false);
    *dict = newdict;
    return true;
}

TMPSTATIC bool mc_genericdict_setinternal(mcgenericdict_t* dict, const char* ckey, char* mkey, void* value)
{
    bool ok;
    unsigned long hash = hash_string(ckey);
    bool found = false;
    unsigned int cellix = mc_genericdict_getcellindex(dict, ckey, hash, &found);
    if(found)
    {
        unsigned int itemix = dict->cells[cellix];
        dict->values[itemix] = value;
        return true;
    }
    if(dict->count >= dict->itemcapacity)
    {
        ok = mc_genericdict_growandrehash(dict);
        if(!ok)
        {
            return false;
        }
        cellix = mc_genericdict_getcellindex(dict, ckey, hash, &found);
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
    dict->cellindices[dict->count] = cellix;
    dict->hashes[dict->count] = hash;
    dict->count++;
    return true;
}


TMPSTATIC bool mc_valdict_init(mcvaldict_t* dict, mcstate_t* state, size_t ktsz, size_t vtsz, unsigned int initialcapacity)
{
    unsigned int i;
    dict->pstate = state;
    dict->keytypesize = ktsz;
    dict->valtypesize = vtsz;
    dict->cells = NULL;
    dict->keys = NULL;
    dict->values = NULL;
    dict->cellindices = NULL;
    dict->hashes = NULL;
    dict->count = 0;
    dict->cellcapacity = initialcapacity;
    dict->itemcapacity = (unsigned int)(initialcapacity * 0.7f);

    dict->funckeyequalsfn = NULL;
    dict->funchashfn = NULL;

    dict->cells = (unsigned int*)mc_allocator_alloc(dict->pstate, dict->cellcapacity * sizeof(*dict->cells));
    dict->keys = (void*)mc_allocator_alloc(dict->pstate, dict->itemcapacity * ktsz);
    dict->values = (void*)mc_allocator_alloc(dict->pstate, dict->itemcapacity * vtsz);
    dict->cellindices = (unsigned int*)mc_allocator_alloc(dict->pstate, dict->itemcapacity * sizeof(*dict->cellindices));
    dict->hashes = (long unsigned int*)mc_allocator_alloc(dict->pstate, dict->itemcapacity * sizeof(*dict->hashes));
    if(dict->cells == NULL || dict->keys == NULL || dict->values == NULL || dict->cellindices == NULL || dict->hashes == NULL)
    {
        goto error;
    }
    for(i = 0; i < dict->cellcapacity; i++)
    {
        dict->cells[i] = VALDICT_INVALID_IX;
    }
    return true;
error:
    mc_allocator_free(dict->pstate, dict->cells);
    mc_allocator_free(dict->pstate, dict->keys);
    mc_allocator_free(dict->pstate, dict->values);
    mc_allocator_free(dict->pstate, dict->cellindices);
    mc_allocator_free(dict->pstate, dict->hashes);
    return false;
}

TMPSTATIC void mc_valdict_deinit(mcvaldict_t* dict)
{
    dict->keytypesize = 0;
    dict->valtypesize = 0;
    dict->count = 0;
    dict->itemcapacity = 0;
    dict->cellcapacity = 0;

    mc_allocator_free(dict->pstate, dict->cells);
    mc_allocator_free(dict->pstate, dict->keys);
    mc_allocator_free(dict->pstate, dict->values);
    mc_allocator_free(dict->pstate, dict->cellindices);
    mc_allocator_free(dict->pstate, dict->hashes);

    dict->cells = NULL;
    dict->keys = NULL;
    dict->values = NULL;
    dict->cellindices = NULL;
    dict->hashes = NULL;
}

TMPSTATIC mcvaldict_t* mc_valdict_make(mcstate_t* state, size_t ktsz, size_t vtsz)
{
    return mc_valdict_makecapacity(state, DICT_INITIAL_SIZE, ktsz, vtsz);
}

TMPSTATIC mcvaldict_t* mc_valdict_makecapacity(mcstate_t* state, unsigned int mincapacity, size_t ktsz, size_t vtsz)
{
    bool ok;
    unsigned int capacity = mc_util_upperpowoftwo(mincapacity * 2);
    mcvaldict_t* dict = (mcvaldict_t*)mc_allocator_alloc(state, sizeof(mcvaldict_t));
    if(!dict)
    {
        return NULL;
    }
    ok = mc_valdict_init(dict, state, ktsz, vtsz, capacity);
    if(!ok)
    {
        mc_allocator_free(state, dict);
        return NULL;
    }
    dict->pstate = state;
    return dict;
}

TMPSTATIC void mc_valdict_destroy(mcvaldict_t* dict)
{
    mcstate_t* state;
    if(!dict)
    {
        return;
    }
    state = dict->pstate;
    mc_valdict_deinit(dict);
    mc_allocator_free(state, dict);
}

TMPSTATIC void mc_valdict_sethashfunction(mcvaldict_t* dict, mccollectionshashfn_t hashfn)
{
    dict->funchashfn = hashfn;
}

TMPSTATIC void mc_valdict_setequalsfunction(mcvaldict_t* dict, mccollectionsequalsfn_t equalsfn)
{
    dict->funckeyequalsfn = equalsfn;
}

TMPSTATIC bool mc_valdict_setkv(mcvaldict_t* dict, void* key, void* value)
{
    bool ok;
    unsigned long hash = mc_valdict_hashkey(dict, key);
    bool found = false;
    unsigned int cellix = mc_valdict_getcellindex(dict, key, hash, &found);
    if(found)
    {
        unsigned int itemix = dict->cells[cellix];
        mc_valdict_setvalueat(dict, itemix, value);
        return true;
    }
    if(dict->count >= dict->itemcapacity)
    {
        ok = mc_valdict_growandrehash(dict);
        if(!ok)
        {
            return false;
        }
        cellix = mc_valdict_getcellindex(dict, key, hash, &found);
    }
    unsigned int lastix = dict->count;
    dict->count++;
    dict->cells[cellix] = lastix;
    mc_valdict_setkeyat(dict, lastix, key);
    mc_valdict_setvalueat(dict, lastix, value);
    dict->cellindices[lastix] = cellix;
    dict->hashes[lastix] = hash;
    return true;
}

TMPSTATIC void* mc_valdict_get(mcvaldict_t* dict, void* key)
{
    if(dict->count == 0)
    {
        return NULL;
    }
    unsigned long hash = mc_valdict_hashkey(dict, key);
    bool found = false;
    unsigned long cellix = mc_valdict_getcellindex(dict, key, hash, &found);
    if(!found)
    {
        return NULL;
    }
    unsigned int itemix = dict->cells[cellix];
    return mc_valdict_getvalueat(dict, itemix);
}

TMPSTATIC void* mc_valdict_getkeyat(mcvaldict_t* dict, unsigned int ix)
{
    if(ix >= dict->count)
    {
        return NULL;
    }
    return (char*)dict->keys + (dict->keytypesize * ix);
}

TMPSTATIC void* mc_valdict_getvalueat(mcvaldict_t* dict, unsigned int ix)
{
    if(ix >= dict->count)
    {
        return NULL;
    }
    return (char*)dict->values + (dict->valtypesize * ix);
}

TMPUNUSED unsigned int mc_valdict_getcapacity(mcvaldict_t* dict)
{
    return dict->itemcapacity;
}

TMPSTATIC bool mc_valdict_setvalueat(mcvaldict_t* dict, unsigned int ix, void* value)
{
    if(ix >= dict->count)
    {
        return false;
    }
    size_t offset = ix * dict->valtypesize;
    memcpy((char*)dict->values + offset, value, dict->valtypesize);
    return true;
}

TMPSTATIC int mc_valdict_count(mcvaldict_t* dict)
{
    if(!dict)
    {
        return 0;
    }
    return dict->count;
}

TMPUNUSED bool mc_valdict_removebykey(mcvaldict_t* dict, void* key)
{
    unsigned int x;
    unsigned long hash = mc_valdict_hashkey(dict, key);
    bool found = false;
    unsigned int cell = mc_valdict_getcellindex(dict, key, hash, &found);
    if(!found)
    {
        return false;
    }

    unsigned int itemix = dict->cells[cell];
    unsigned int lastitemix = dict->count - 1;
    if(itemix < lastitemix)
    {
        void* lastkey = mc_valdict_getkeyat(dict, lastitemix);
        mc_valdict_setkeyat(dict, itemix, lastkey);
        void* lastvalue = mc_valdict_getkeyat(dict, lastitemix);
        mc_valdict_setvalueat(dict, itemix, lastvalue);
        dict->cellindices[itemix] = dict->cellindices[lastitemix];
        dict->hashes[itemix] = dict->hashes[lastitemix];
        dict->cells[dict->cellindices[itemix]] = itemix;
    }
    dict->count--;

    unsigned int i = cell;
    unsigned int j = i;
    for(x = 0; x < (dict->cellcapacity - 1); x++)
    {
        j = (j + 1) & (dict->cellcapacity - 1);
        if(dict->cells[j] == VALDICT_INVALID_IX)
        {
            break;
        }
        unsigned int k = (unsigned int)(dict->hashes[dict->cells[j]]) & (dict->cellcapacity - 1);
        if((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j)))
        {
            dict->cellindices[dict->cells[j]] = i;
            dict->cells[i] = dict->cells[j];
            i = j;
        }
    }
    dict->cells[i] = VALDICT_INVALID_IX;
    return true;
}

TMPSTATIC void mc_valdict_clear(mcvaldict_t* dict)
{
    unsigned int i;
    dict->count = 0;
    for(i = 0; i < dict->cellcapacity; i++)
    {
        dict->cells[i] = VALDICT_INVALID_IX;
    }
}

TMPSTATIC unsigned int mc_valdict_getcellindex(mcvaldict_t* dict, void* key, unsigned long hash, bool* outfound)
{
    bool areequal;
    unsigned int i;
    unsigned int ix;
    unsigned int cell;
    unsigned int cellix;
    unsigned long hashtocheck;
    void* keytocheck;
    *outfound = false;
    cellix = (unsigned int)hash & (dict->cellcapacity - 1);
    for(i = 0; i < dict->cellcapacity; i++)
    {
        ix = (cellix + i) & (dict->cellcapacity - 1);
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
        keytocheck = mc_valdict_getkeyat(dict, cell);
        areequal = mc_valdict_keysareequal(dict, key, keytocheck);
        if(areequal)
        {
            *outfound = true;
            return ix;
        }
    }
    return VALDICT_INVALID_IX;
}

TMPSEMISTATIC bool mc_valdict_growandrehash(mcvaldict_t* dict)
{
    bool ok;
    mcvaldict_t newdict;
    unsigned int i;
    unsigned newcapacity = dict->cellcapacity == 0 ? DICT_INITIAL_SIZE : dict->cellcapacity * 2;
    ok = mc_valdict_init(&newdict, dict->pstate, dict->keytypesize, dict->valtypesize, newcapacity);
    if(!ok)
    {
        return false;
    }
    newdict.funckeyequalsfn = dict->funckeyequalsfn;
    newdict.funchashfn = dict->funchashfn;
    for(i = 0; i < dict->count; i++)
    {
        char* key = (char*)mc_valdict_getkeyat(dict, i);
        void* value = mc_valdict_getvalueat(dict, i);
        ok = mc_valdict_setkv(&newdict, key, value);
        if(!ok)
        {
            mc_valdict_deinit(&newdict);
            return false;
        }
    }
    mc_valdict_deinit(dict);
    *dict = newdict;
    return true;
}

TMPSTATIC bool mc_valdict_setkeyat(mcvaldict_t* dict, unsigned int ix, void* key)
{
    if(ix >= dict->count)
    {
        return false;
    }
    size_t offset = ix * dict->keytypesize;
    memcpy((char*)dict->keys + offset, key, dict->keytypesize);
    return true;
}

TMPSTATIC bool mc_valdict_keysareequal(mcvaldict_t* dict, void* a, void* b)
{
    if(dict->funckeyequalsfn)
    {
        return dict->funckeyequalsfn(a, b);
    }
    return memcmp(a, b, dict->keytypesize) == 0;
}

TMPSTATIC unsigned long mc_valdict_hashkey(mcvaldict_t* dict, void* key)
{
    if(dict->funchashfn)
    {
        return dict->funchashfn(key);
    }
    return mc_util_hashdata(key, dict->keytypesize);
}

TMPSTATIC mcbasicarray_t* mc_basicarray_make(mcstate_t* state, size_t tsz)
{
    return mc_basicarray_makecapacity(state, 32, tsz);
}

TMPSTATIC mcbasicarray_t* mc_basicarray_makecapacity(mcstate_t* state, unsigned int capacity, size_t tsz)
{
    bool ok;
    mcbasicarray_t* arr = (mcbasicarray_t*)mc_allocator_alloc(state, sizeof(mcbasicarray_t));
    if(!arr)
    {
        return NULL;
    }

    ok = mc_basicarray_initcapacity(arr, state, capacity, tsz);
    if(!ok)
    {
        mc_allocator_free(state, arr);
        return NULL;
    }
    arr->pstate = state;
    return arr;
}

TMPSTATIC void mc_basicarray_destroy(mcbasicarray_t* arr)
{
    mcstate_t* state;
    if(!arr)
    {
        return;
    }
    state = arr->pstate;
    mc_basicarray_deinit(arr);
    mc_allocator_free(state, arr);
}

TMPSTATIC mcbasicarray_t* mc_basicarray_copy(mcbasicarray_t* arr)
{
    mcbasicarray_t* copy = (mcbasicarray_t*)mc_allocator_alloc(arr->pstate, sizeof(mcbasicarray_t));
    if(!copy)
    {
        return NULL;
    }
    copy->pstate = arr->pstate;
    copy->capacity = arr->capacity;
    copy->count = arr->count;
    copy->typesize = arr->typesize;
    copy->caplocked = arr->caplocked;
    if(arr->allocdata)
    {
        copy->allocdata = (unsigned char*)mc_allocator_alloc(arr->pstate, arr->capacity * arr->typesize);
        if(!copy->allocdata)
        {
            mc_allocator_free(arr->pstate, copy);
            return NULL;
        }
        copy->data = copy->allocdata;
        memcpy(copy->allocdata, arr->data, arr->capacity * arr->typesize);
    }
    else
    {
        copy->allocdata = NULL;
        copy->data = NULL;
    }

    return copy;
}

TMPSTATIC bool mc_basicarray_push(mcbasicarray_t* arr, void* value)
{
    if(arr->count >= arr->capacity)
    {
        COLLECTIONS_ASSERT(!arr->caplocked);
        if(arr->caplocked)
        {
            return false;
        }
        unsigned int newcapacity = arr->capacity > 0 ? arr->capacity * 2 : 1;
        unsigned char* newdata = (unsigned char*)mc_allocator_alloc(arr->pstate, newcapacity * arr->typesize);
        if(!newdata)
        {
            return false;
        }
        memcpy(newdata, arr->data, arr->count * arr->typesize);
        mc_allocator_free(arr->pstate, arr->allocdata);
        arr->allocdata = newdata;
        arr->data = arr->allocdata;
        arr->capacity = newcapacity;
    }
    if(value)
    {
        memcpy(arr->data + (arr->count * arr->typesize), value, arr->typesize);
    }
    arr->count++;
    return true;
}

TMPUNUSED bool mc_basicarray_addn(mcbasicarray_t* arr, void* values, int n)
{
    bool ok;
    int i;
    for(i = 0; i < n; i++)
    {
        uint8_t* value = NULL;
        if(values)
        {
            value = (uint8_t*)values + (i * arr->typesize);
        }
        ok = mc_basicarray_push(arr, value);
        if(!ok)
        {
            return false;
        }
    }
    return true;
}

TMPUNUSED bool mc_basicarray_addarray(mcbasicarray_t* dest, mcbasicarray_t* source)
{
    bool ok;
    int i;
    COLLECTIONS_ASSERT(dest->typesize == source->typesize);
    if(dest->typesize != source->typesize)
    {
        return false;
    }
    int destbeforecount = mc_basicarray_count(dest);
    for(i = 0; i < mc_basicarray_count(source); i++)
    {
        void* item = mc_basicarray_get(source, i);
        ok = mc_basicarray_push(dest, item);
        if(!ok)
        {
            dest->count = destbeforecount;
            return false;
        }
    }
    return true;
}

TMPSTATIC bool mc_basicarray_pop(mcbasicarray_t* arr, void* outvalue)
{
    if(arr->count <= 0)
    {
        return false;
    }
    if(outvalue)
    {
        void* res = mc_basicarray_get(arr, arr->count - 1);
        memcpy(outvalue, res, arr->typesize);
    }
    mc_basicarray_removeat(arr, arr->count - 1);
    return true;
}

TMPSTATIC void* mc_basicarray_top(mcbasicarray_t* arr)
{
    if(arr->count <= 0)
    {
        return NULL;
    }
    return mc_basicarray_get(arr, arr->count - 1);
}

TMPSTATIC bool mc_basicarray_set(mcbasicarray_t* arr, unsigned int ix, void* value)
{
    if(ix >= arr->count)
    {
        COLLECTIONS_ASSERT(false);
        return false;
    }
    size_t offset = ix * arr->typesize;
    memmove(arr->data + offset, value, arr->typesize);
    return true;
}

TMPUNUSED bool mc_basicarray_setn(mcbasicarray_t* arr, unsigned int ix, void* values, int n)
{
    bool ok;
    int i;
    for(i = 0; i < n; i++)
    {
        int destix = ix + i;
        unsigned char* value = (unsigned char*)values + (i * arr->typesize);
        if(destix < mc_basicarray_count(arr))
        {
            ok = mc_basicarray_set(arr, destix, value);
            if(!ok)
            {
                return false;
            }
        }
        else
        {
            ok = mc_basicarray_push(arr, value);
            if(!ok)
            {
                return false;
            }
        }
    }
    return true;
}

TMPSTATIC void* mc_basicarray_get(mcbasicarray_t* arr, unsigned int ix)
{
    if(ix >= arr->count)
    {
        COLLECTIONS_ASSERT(false);
        return NULL;
    }
    size_t offset = ix * arr->typesize;
    return arr->data + offset;
}

TMPSTATIC void* mc_basicarray_getconst(mcbasicarray_t* arr, unsigned int ix)
{
    if(ix >= arr->count)
    {
        COLLECTIONS_ASSERT(false);
        return NULL;
    }
    size_t offset = ix * arr->typesize;
    return arr->data + offset;
}

TMPUNUSED void* mc_basicarray_getlast(mcbasicarray_t* arr)
{
    if(arr->count <= 0)
    {
        return NULL;
    }
    return mc_basicarray_get(arr, arr->count - 1);
}

TMPSTATIC int mc_basicarray_count(mcbasicarray_t* arr)
{
    if(!arr)
    {
        return 0;
    }
    return arr->count;
}

TMPUNUSED unsigned int mc_basicarray_getcapacity(mcbasicarray_t* arr)
{
    return arr->capacity;
}

TMPSTATIC bool mc_basicarray_removeat(mcbasicarray_t* arr, unsigned int ix)
{
    if(ix >= arr->count)
    {
        return false;
    }
    if(ix == 0)
    {
        arr->data += arr->typesize;
        arr->capacity--;
        arr->count--;
        return true;
    }
    if(ix == (arr->count - 1))
    {
        arr->count--;
        return true;
    }
    size_t tomovebytes = (arr->count - 1 - ix) * arr->typesize;
    void* dest = arr->data + (ix * arr->typesize);
    void* src = arr->data + ((ix + 1) * arr->typesize);
    memmove(dest, src, tomovebytes);
    arr->count--;
    return true;
}

TMPSTATIC bool mc_basicarray_removeitem(mcbasicarray_t* arr, void* ptr)
{
    int ix = mc_basicarray_getindex(arr, ptr);
    if(ix < 0)
    {
        return false;
    }
    return mc_basicarray_removeat(arr, ix);
}

TMPSTATIC void mc_basicarray_clear(mcbasicarray_t* arr)
{
    arr->count = 0;
}

TMPUNUSED void mc_basicarray_clearanddeinititems(mcbasicarray_t* arr, mcarrayitemdeinitfn_t deinit_fn)
{
    int i;
    for(i = 0; i < mc_basicarray_count(arr); i++)
    {
        void* item = mc_basicarray_get(arr, i);
        deinit_fn(item);
    }
    arr->count = 0;
}

TMPSTATIC void mc_basicarray_lockcapacity(mcbasicarray_t* arr)
{
    arr->caplocked = true;
}

TMPSTATIC int mc_basicarray_getindex(mcbasicarray_t* arr, void* ptr)
{
    int i;
    for(i = 0; i < mc_basicarray_count(arr); i++)
    {
        if(mc_basicarray_getconst(arr, i) == ptr)
        {
            return i;
        }
    }
    return -1;
}

TMPSTATIC bool mc_basicarray_contains(mcbasicarray_t* arr, void* ptr)
{
    return mc_basicarray_getindex(arr, ptr) >= 0;
}

TMPSTATIC void* mc_basicarray_data(mcbasicarray_t* arr)
{
    return arr->data;
}

TMPUNUSED void* mc_basicarray_constdata(mcbasicarray_t* arr)
{
    return arr->data;
}

TMPSTATIC void mc_basicarray_orphandata(mcbasicarray_t* arr)
{
    mc_basicarray_initcapacity(arr, arr->pstate, 0, arr->typesize);
}

TMPSTATIC bool mc_basicarray_reverse(mcbasicarray_t* arr)
{
    int aix;
    int count = mc_basicarray_count(arr);
    if(count < 2)
    {
        return true;
    }
    void* temp = (void*)mc_allocator_alloc(arr->pstate, arr->typesize);
    if(!temp)
    {
        return false;
    }
    for(aix = 0; aix < (count / 2); aix++)
    {
        int bix = count - aix - 1;
        void* a = mc_basicarray_get(arr, aix);
        void* b = mc_basicarray_get(arr, bix);
        memcpy(temp, a, arr->typesize);
        /* no need for check because it will be within range */
        mc_basicarray_set(arr, aix, b);
        mc_basicarray_set(arr, bix, temp);
    }
    mc_allocator_free(arr->pstate, temp);
    return true;
}

TMPSTATIC bool mc_basicarray_initcapacity(mcbasicarray_t* arr, mcstate_t* state, unsigned int capacity, size_t tsz)
{
    arr->pstate = state;
    if(capacity > 0)
    {
        arr->allocdata = (unsigned char*)mc_allocator_alloc(arr->pstate, capacity * tsz);
        arr->data = arr->allocdata;
        if(!arr->allocdata)
        {
            return false;
        }
    }
    else
    {
        arr->allocdata = NULL;
        arr->data = NULL;
    }
    arr->capacity = capacity;
    arr->count = 0;
    arr->typesize = tsz;
    arr->caplocked = false;
    return true;
}

TMPSTATIC void mc_basicarray_deinit(mcbasicarray_t* arr)
{
    mc_allocator_free(arr->pstate, arr->allocdata);
}

TMPSTATIC mcptrarray_t* mc_ptrarray_make(mcstate_t* state)
{
    return mc_ptrarray_makecapacity(state, 0);
}

TMPSTATIC mcptrarray_t* mc_ptrarray_makecapacity(mcstate_t* state, unsigned int capacity)
{
    bool ok;
    mcptrarray_t* ptrarr = (mcptrarray_t*)mc_allocator_alloc(state, sizeof(mcptrarray_t));
    if(!ptrarr)
    {
        return NULL;
    }
    ptrarr->pstate = state;
    ok = mc_basicarray_initcapacity(&ptrarr->innerbarray, state, capacity, sizeof(void*));
    if(!ok)
    {
        mc_allocator_free(state, ptrarr);
        return NULL;
    }
    return ptrarr;
}


TMPSTATIC void mc_ptrarray_destroy(mcptrarray_t* arr, mcptrarrayitemdestroyfn_t destroy_fn)
{
    /* todo: destroy and copy in make fn */
    if(arr == NULL)
    {
        return;
    }
    if(destroy_fn)
    {
        mc_ptrarray_clearanddestroy(arr, destroy_fn);
    }
    else
    {
    }
    mc_basicarray_deinit(&arr->innerbarray);
    mc_allocator_free(arr->pstate, arr);

}

TMPSTATIC mcptrarray_t* mc_ptrarray_copy(mcptrarray_t* arr, mcptrarrayitemcopyfn_t copyfn, mcptrarrayitemdestroyfn_t destroy_fn)
{
    bool ok;
    int i;
    void* item;
    void* itemcopy;
    mcptrarray_t* arrcopy;
    arrcopy = mc_ptrarray_makecapacity(arr->pstate, arr->innerbarray.capacity);
    if(!arrcopy)
    {
        return NULL;
    }
    for(i = 0; i < mc_ptrarray_count(arr); i++)
    {
        item = (void*)mc_ptrarray_get(arr, i);
        itemcopy = item;
        if(copyfn)
        {
            itemcopy = copyfn(item);
        }
        if(item && !itemcopy)
        {
            goto err;
        }
        ok = mc_ptrarray_push(arrcopy, itemcopy);
        if(!ok)
        {
            goto err;
        }
    }
    return arrcopy;
err:
    mc_ptrarray_destroy(arrcopy, destroy_fn);
    return NULL;
}

TMPSTATIC bool mc_ptrarray_push(mcptrarray_t* arr, void* ptr)
{
    return mc_basicarray_push(&arr->innerbarray, &ptr);
}

TMPSTATIC void* mc_ptrarray_get(mcptrarray_t* arr, unsigned int ix)
{
    void* res = mc_basicarray_get(&arr->innerbarray, ix);
    if(!res)
    {
        return NULL;
    }
    return *(void**)res;
}

TMPSTATIC void* mc_ptrarray_pop(mcptrarray_t* arr)
{
    int ix = mc_ptrarray_count(arr) - 1;
    void* res = mc_ptrarray_get(arr, ix);
    mc_ptrarray_removeat(arr, ix);
    return res;
}

TMPSTATIC void* mc_ptrarray_top(mcptrarray_t* arr)
{
    int count = mc_ptrarray_count(arr);
    if(count == 0)
    {
        return NULL;
    }
    return mc_ptrarray_get(arr, count - 1);
}

TMPSTATIC int mc_ptrarray_count(mcptrarray_t* arr)
{
    if(!arr)
    {
        return 0;
    }
    return mc_basicarray_count(&arr->innerbarray);
}

TMPSTATIC bool mc_ptrarray_removeat(mcptrarray_t* arr, unsigned int ix)
{
    return mc_basicarray_removeat(&arr->innerbarray, ix);
}

TMPSTATIC void mc_ptrarray_clear(mcptrarray_t* arr)
{
    mc_basicarray_clear(&arr->innerbarray);
}

TMPSTATIC void mc_ptrarray_clearanddestroy(mcptrarray_t* arr, mcptrarrayitemdestroyfn_t destroy_fn)
{
    int i;
    for(i = 0; i < mc_ptrarray_count(arr); i++)
    {
        void* item = mc_ptrarray_get(arr, i);
        destroy_fn(item);
    }
    mc_ptrarray_clear(arr);
}

TMPUNUSED void mc_ptrarray_lockcapacity(mcptrarray_t* arr)
{
    mc_basicarray_lockcapacity(&arr->innerbarray);
}

TMPSTATIC int mc_ptrarray_getindex(mcptrarray_t* arr, void* ptr)
{
    int i;
    for(i = 0; i < mc_ptrarray_count(arr); i++)
    {
        if(mc_ptrarray_get(arr, i) == ptr)
        {
            return i;
        }
    }
    return -1;
}

TMPUNUSED bool mc_ptrarray_contains(mcptrarray_t* arr, void* item)
{
    return mc_ptrarray_getindex(arr, item) >= 0;
}

TMPUNUSED void* mc_ptrarray_getaddr(mcptrarray_t* arr, unsigned int ix)
{
    void* res = mc_basicarray_get(&arr->innerbarray, ix);
    if(res == NULL)
    {
        return NULL;
    }
    return res;
}

TMPUNUSED void* mc_ptrarray_data(mcptrarray_t* arr)
{
    return mc_basicarray_data(&arr->innerbarray);
}

TMPUNUSED void mc_ptrarray_reverse(mcptrarray_t* arr)
{
    mc_basicarray_reverse(&arr->innerbarray);
}

TMPSTATIC mcprintstate_t* mc_printer_make(mcstate_t* state, FILE* ofh)
{
    return mc_printer_make_with_capacity(state, 1, ofh);
}

TMPSTATIC bool mc_printer_init(mcprintstate_t* pr, mcstate_t* state, size_t capacity, FILE* ofh, bool onstack)
{
    memset(pr, 0, sizeof(mcprintstate_t));
    pr->pstate = state;
    pr->failed = false;
    pr->destfile = ofh;
    pr->data = NULL;
    pr->capacity = 0;
    pr->len = 0;
    pr->onstack = onstack;
    if(ofh != NULL)
    {
        return true;
    }
    pr->data = (char*)mc_allocator_alloc(state, capacity);
    if(pr->data == NULL)
    {
        mc_allocator_free(state, pr);
        return false;
    }
    pr->capacity = capacity;
    pr->len = 0;
    pr->data[0] = '\0';
    return true;
}

TMPSTATIC mcprintstate_t* mc_printer_make_with_capacity(mcstate_t* state, unsigned int capacity, FILE* ofh)
{
    mcprintstate_t* pr;
    pr = (mcprintstate_t*)mc_allocator_alloc(state, sizeof(mcprintstate_t));
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

TMPSTATIC void mc_printer_destroy(mcprintstate_t* pr)
{
    if(pr == NULL)
    {
        return;
    }
    if(pr->data != NULL)
    {
        mc_allocator_free(pr->pstate, pr->data);
    }
    if(pr->onstack)
    {
        return;
    }
    mc_allocator_free(pr->pstate, pr);
}

TMPUNUSED void mc_printer_clear(mcprintstate_t* pr)
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

TMPSTATIC bool mc_printer_putlen(mcprintstate_t* pr, const char* str, size_t len)
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

TMPSTATIC bool mc_printer_puts(mcprintstate_t* pr, const char* str)
{
    return mc_printer_putlen(pr, str, strlen(str));
}

TMPSEMISTATIC bool mc_printer_printf(mcprintstate_t* pr, const char* fmt, ...)
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

TMPSTATIC const char* mc_printer_getstring(mcprintstate_t* pr)
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

TMPUNUSED size_t mc_printer_getlength(mcprintstate_t* pr)
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

TMPSTATIC char* mc_printer_get_string_and_destroy(mcprintstate_t* pr)
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

TMPSTATIC bool mc_printer_failed(mcprintstate_t* pr)
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
    ndata = (char*)mc_allocator_alloc(pr->pstate, newcapacity);
    if(ndata == NULL)
    {
        pr->failed = true;
        return false;
    }
    memcpy(ndata, pr->data, pr->len);
    ndata[pr->len] = '\0';
    mc_allocator_free(pr->pstate, pr->data);
    pr->data = ndata;
    pr->capacity = newcapacity;
    return true;
}

TMPSTATIC mcastlocation_t src_pos_make(mcastcompiledfile_t* file, int line, int column)
{
    mcastlocation_t loc;
    loc.file = file;
    loc.line = line;
    loc.column = column;
    return loc;
}

TMPSEMISTATIC char* mc_util_stringallocfmt(mcstate_t* state, const char* format, ...)
{
    int towrite;
    int written;
    char* res;
    va_list args;
    va_start(args, format);
    towrite = vsnprintf(NULL, 0, format, args);
    va_end(args);
    va_start(args, format);
    res = (char*)mc_allocator_alloc(state, towrite + 1);
    if(!res)
    {
        return NULL;
    }
    written = vsprintf(res, format, args);
    va_end(args);
    (void)written;
    MC_ASSERT(written == towrite);
    return res;
}


TMPSTATIC char* mc_util_strndup(mcstate_t* state, const char* string, size_t n)
{
    char* outputstring;
    outputstring = (char*)mc_allocator_alloc(state, n + 1);
    if(!outputstring)
    {
        return NULL;
    }
    outputstring[n] = '\0';
    memcpy(outputstring, string, n);
    return outputstring;
}

TMPSTATIC char* mc_util_strdup(mcstate_t* state, const char* string)
{
    if(!string)
    {
        return NULL;
    }
    return mc_util_strndup(state, string, strlen(string));
}

TMPSTATIC uint64_t mc_util_doubletouint64(double val)
{
    union
    {
        uint64_t val_uint64;
        double val_double;
    } temp;
    temp.val_double = val;
    return temp.val_uint64;
}

TMPSTATIC double mc_util_uint64todouble(uint64_t val)
{
    union
    {
        uint64_t val_uint64;
        double val_double;
    } temp;
    temp.val_uint64 = val;
    return temp.val_double;
}

TMPSTATIC bool mc_util_istimerplatformsupported()
{
    return true;
}

TMPSTATIC mctimer_t mc_timer_start()
{
    mctimer_t timer;
    memset(&timer, 0, sizeof(mctimer_t));
    #if defined(__unix__)
        /* At some point it should be replaced with more accurate per-platform timers */
        struct timeval starttime;
        gettimeofday(&starttime, NULL);
        timer.startoffset = starttime.tv_sec;
        timer.starttimems = starttime.tv_usec / 1000.0;
    #endif
    return timer;
}

TMPSTATIC double mc_timer_getelapsedms(mctimer_t* timer)
{
    #if defined(__unix__)
        struct timeval currenttime;
        gettimeofday(&currenttime, NULL);
        int times = (int)((int64_t)currenttime.tv_sec - timer->startoffset);
        double currenttimems = (times * 1000) + (currenttime.tv_usec / 1000.0);
        return currenttimems - timer->starttimems;
    #else
        return 0;
    #endif
}

TMPSTATIC mcastscopecomp_t* compilation_scope_make(mcstate_t* state, mcastscopecomp_t* outer)
{
    mcastscopecomp_t* scope;
    scope = (mcastscopecomp_t*)mc_allocator_alloc(state, sizeof(mcastscopecomp_t));
    if(!scope)
    {
        return NULL;
    }
    memset(scope, 0, sizeof(mcastscopecomp_t));
    scope->pstate = state;
    scope->outer = outer;
    scope->bytecode = mc_basicarray_make(state, sizeof(uint8_t));
    if(!scope->bytecode)
    {
        goto err;
    }
    scope->src_positions = mc_basicarray_make(state, sizeof(mcastlocation_t));
    if(!scope->src_positions)
    {
        goto err;
    }
    scope->break_ip_stack = mc_basicarray_make(state, sizeof(int));
    if(!scope->break_ip_stack)
    {
        goto err;
    }
    scope->continue_ip_stack = mc_basicarray_make(state, sizeof(int));
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
    mc_basicarray_destroy(scope->continue_ip_stack);
    mc_basicarray_destroy(scope->break_ip_stack);
    mc_basicarray_destroy(scope->bytecode);
    mc_basicarray_destroy(scope->src_positions);
    mc_allocator_free(state, scope);
}

TMPSTATIC compilation_result_t* compilation_scope_orphan_result(mcastscopecomp_t* scope)
{
    compilation_result_t* res = compilation_result_make(scope->pstate, (uint8_t*)mc_basicarray_data(scope->bytecode), (mcastlocation_t*)mc_basicarray_data(scope->src_positions), mc_basicarray_count(scope->bytecode));
    if(!res)
    {
        return NULL;
    }
    mc_basicarray_orphandata(scope->bytecode);
    mc_basicarray_orphandata(scope->src_positions);
    return res;
}

TMPSTATIC compilation_result_t* compilation_result_make(mcstate_t* state, uint8_t* bytecode, mcastlocation_t* src_positions, int count)
{
    compilation_result_t* res = (compilation_result_t*)mc_allocator_alloc(state, sizeof(compilation_result_t));
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
    mc_allocator_free(state, res->bytecode);
    mc_allocator_free(state, res->src_positions);
    mc_allocator_free(state, res);
}

TMPSTATIC bool mc_lexer_init(mcastlexer_t* lex, mcstate_t* state, mcerrlist_t* errs, const char* input, mcastcompiledfile_t* file)
{
    bool ok;
    lex->pstate = state;
    lex->errors = errs;
    lex->inputsource = input;
    lex->inputlength = (int)strlen(input);
    lex->position = 0;
    lex->nextposition = 0;
    lex->ch = '\0';
    if(file)
    {
        lex->line = mc_ptrarray_count(file->lines);
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
    lex->continuetplstring = false;
    memset(&lex->prevstate, 0, sizeof(lex->prevstate));
    token_init(&lex->prevtoken, MC_TOK_INVALID, NULL, 0);
    token_init(&lex->currtoken, MC_TOK_INVALID, NULL, 0);
    token_init(&lex->peektoken, MC_TOK_INVALID, NULL, 0);
    return true;
}

TMPSTATIC bool mc_lexer_failed(mcastlexer_t* lex)
{
    return lex->failed;
}

TMPSTATIC void mc_lexer_conttplstring(mcastlexer_t* lex)
{
    lex->continuetplstring = true;
}

TMPSTATIC bool mc_lexer_currtokenis(mcastlexer_t* lex, mcasttoktype_t type)
{
    return lex->currtoken.type == type;
}

TMPSTATIC bool mc_lexer_peektokenis(mcastlexer_t* lex, mcasttoktype_t type)
{
    return lex->peektoken.type == type;
}

TMPSTATIC bool mc_lexer_nexttoken(mcastlexer_t* lex)
{
    lex->prevtoken = lex->currtoken;
    lex->currtoken = lex->peektoken;
    lex->peektoken = mc_lexer_nexttokinternal(lex);
    return !lex->failed;
}

TMPSTATIC bool mc_lexer_previoustoken(mcastlexer_t* lex)
{
    if(lex->prevtoken.type == MC_TOK_INVALID)
    {
        return false;
    }
    lex->peektoken = lex->currtoken;
    lex->currtoken = lex->prevtoken;
    token_init(&lex->prevtoken, MC_TOK_INVALID, NULL, 0);
    lex->ch = lex->prevstate.ch;
    lex->column = lex->prevstate.column;
    lex->line = lex->prevstate.line;
    lex->position = lex->prevstate.position;
    lex->nextposition = lex->prevstate.nextposition;
    return true;
}

TMPSTATIC mcasttoken_t mc_lexer_nexttokinternal(mcastlexer_t* lex)
{
    char c;
    mcasttoken_t outtok;
    lex->prevstate.ch = lex->ch;
    lex->prevstate.column = lex->column;
    lex->prevstate.line = lex->line;
    lex->prevstate.position = lex->position;
    lex->prevstate.nextposition = lex->nextposition;
    while(true)
    {
        if(!lex->continuetplstring)
        {
            mc_lexer_skipspace(lex);
        }
        outtok.type = MC_TOK_INVALID;
        outtok.literal = lex->inputsource + lex->position;
        outtok.len = 1;
        outtok.pos = src_pos_make(lex->file, lex->line, lex->column);
        c = lex->continuetplstring ? '`' : lex->ch;
        switch(c)
        {
            case '\0':
                {
                    token_init(&outtok, MC_TOK_EOF, "EOF", 3);
                }
                break;
            case '=':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        token_init(&outtok, MC_TOK_EQ, "==", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_ASSIGN, "=", 1);
                    }
                }
                break;
            case '&':
                {
                    if(mc_lexer_peekchar(lex) == '&')
                    {
                        token_init(&outtok, MC_TOK_AND, "&&", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '=')
                    {
                        token_init(&outtok, MC_TOK_BITANDASSIGN, "&=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_BITAND, "&", 1);
                    }
                }
                break;
            case '|':
                {
                    if(mc_lexer_peekchar(lex) == '|')
                    {
                        token_init(&outtok, MC_TOK_OR, "||", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '=')
                    {
                        token_init(&outtok, MC_TOK_BITORASSIGN, "|=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_BITOR, "|", 1);
                    }
                }
                break;
            case '^':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        token_init(&outtok, MC_TOK_BITXORASSIGN, "^=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_BITXOR, "^", 1);
                        break;
                    }
                }
                break;
            case '+':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        token_init(&outtok, MC_TOK_PLUSASSIGN, "+=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '+')
                    {
                        token_init(&outtok, MC_TOK_PLUSPLUS, "++", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_PLUS, "+", 1);
                        break;
                    }
                }
                break;
            case '-':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        token_init(&outtok, MC_TOK_MINUSASSIGN, "-=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '-')
                    {
                        token_init(&outtok, MC_TOK_MINUSMINUS, "--", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_MINUS, "-", 1);
                        break;
                    }
                }
                break;
            case '!':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        token_init(&outtok, MC_TOK_NOTEQ, "!=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_BANG, "!", 1);
                    }
                }
                break;
            case '*':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        token_init(&outtok, MC_TOK_ASTERISKASSIGN, "*=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_ASTERISK, "*", 1);
                        break;
                    }
                }
                break;
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
                        token_init(&outtok, MC_TOK_SLASHASSIGN, "/=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_SLASH, "/", 1);
                        break;
                    }
                }
                break;
            case '<':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        token_init(&outtok, MC_TOK_LTE, "<=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '<')
                    {
                        mc_lexer_readchar(lex);
                        if(mc_lexer_peekchar(lex) == '=')
                        {
                            token_init(&outtok, MC_TOK_LSHIFTASSIGN, "<<=", 3);
                            mc_lexer_readchar(lex);
                        }
                        else
                        {
                            token_init(&outtok, MC_TOK_LSHIFT, "<<", 2);
                        }
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_LT, "<", 1);
                        break;
                    }
                }
                break;
            case '>':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        token_init(&outtok, MC_TOK_GTE, ">=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '>')
                    {
                        mc_lexer_readchar(lex);
                        if(mc_lexer_peekchar(lex) == '=')
                        {
                            token_init(&outtok, MC_TOK_RSHIFTASSIGN, ">>=", 3);
                            mc_lexer_readchar(lex);
                        }
                        else
                        {
                            token_init(&outtok, MC_TOK_RSHIFT, ">>", 2);
                        }
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_GT, ">", 1);
                    }
                }
                break;
            case ',':
                {
                    token_init(&outtok, MC_TOK_COMMA, ",", 1);
                }
                break;
            case ';':
                {
                    token_init(&outtok, MC_TOK_SEMICOLON, ";", 1);
                }
                break;
            case ':':
                {
                    token_init(&outtok, MC_TOK_COLON, ":", 1);
                }
                break;
            case '(':
                {
                    token_init(&outtok, MC_TOK_LPAREN, "(", 1);
                }
                break;
            case ')':
                {
                    token_init(&outtok, MC_TOK_RPAREN, ")", 1);
                }
                break;
            case '{':
                {
                    token_init(&outtok, MC_TOK_LBRACE, "{", 1);
                }
                break;
            case '}':
                {
                    token_init(&outtok, MC_TOK_RBRACE, "}", 1);
                }
                break;
            case '[':
                {
                    token_init(&outtok, MC_TOK_LBRACKET, "[", 1);
                }
                break;
            case ']':
                {
                    token_init(&outtok, MC_TOK_RBRACKET, "]", 1);
                }
                break;
            case '.':
                {
                    token_init(&outtok, MC_TOK_DOT, ".", 1);
                }
                break;
            case '?':
                {
                    token_init(&outtok, MC_TOK_QUESTION, "?", 1);
                }
                break;
            case '%':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        token_init(&outtok, MC_TOK_PERCENTASSIGN, "%=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_PERCENT, "%", 1);
                        break;
                    }
                }
                break;
            case '"':
                {
                    int len;
                    const char* str;
                    mc_lexer_readchar(lex);
                    str = mc_lexer_scanstring(lex, '"', false, NULL, &len);
                    if(str)
                    {
                        token_init(&outtok, MC_TOK_STRING, str, len);
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_INVALID, NULL, 0);
                    }
                }
                break;
            case '\'':
                {
                    int len;
                    const char* str;
                    mc_lexer_readchar(lex);
                    str = mc_lexer_scanstring(lex, '\'', false, NULL, &len);
                    if(str)
                    {
                        token_init(&outtok, MC_TOK_STRING, str, len);
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_INVALID, NULL, 0);
                    }
                }
                break;
            case '`':
                {
                    int len;
                    bool templatefound;
                    const char* str;
                    if(!lex->continuetplstring)
                    {
                        mc_lexer_readchar(lex);
                    }
                    templatefound = false;
                    str = mc_lexer_scanstring(lex, '`', true, &templatefound, &len);
                    if(str)
                    {
                        if(templatefound)
                        {
                            token_init(&outtok, MC_TOK_TEMPLATESTRING, str, len);
                        }
                        else
                        {
                            token_init(&outtok, MC_TOK_STRING, str, len);
                        }
                    }
                    else
                    {
                        token_init(&outtok, MC_TOK_INVALID, NULL, 0);
                    }
                }
                break;
            default:
                {
                    int identlen;
                    int numberlen;
                    const char* ident;
                    const char* number;
                    mcasttoktype_t type;
                    if(mc_lexer_charisletter(lex->ch))
                    {
                        identlen = 0;
                        ident = mc_lexer_scanident(lex, &identlen);
                        type = mc_lexer_lookupident(ident, identlen);
                        token_init(&outtok, type, ident, identlen);
                        return outtok;
                    }
                    if(mc_lexer_charisdigit(lex->ch))
                    {
                        numberlen = 0;
                        number = mc_lexer_scannumber(lex, &numberlen);
                        token_init(&outtok, MC_TOK_NUMBER, number, numberlen);
                        return outtok;
                    }
                }
                break;
        }
        mc_lexer_readchar(lex);
        if(mc_lexer_failed(lex))
        {
            token_init(&outtok, MC_TOK_INVALID, NULL, 0);
        }
        lex->continuetplstring = false;
        return outtok;
    }
    /* NB. never reached; but keep the compiler from complaining. */
    return outtok;
}

TMPSTATIC bool mc_lexer_expectcurrent(mcastlexer_t* lex, mcasttoktype_t type)
{
    const char* actualtypestr;
    const char* expectedtypestr;
    if(mc_lexer_failed(lex))
    {
        return false;
    }

    if(!mc_lexer_currtokenis(lex, type))
    {
        expectedtypestr = token_type_to_string(type);
        actualtypestr = token_type_to_string(lex->currtoken.type);
        mc_errlist_addf(lex->errors, MC_ERROR_PARSING, lex->currtoken.pos, "Expected current token to be \"%s\", got \"%s\" instead", expectedtypestr, actualtypestr);
        return false;
    }
    return true;
}

TMPSTATIC bool mc_lexer_readchar(mcastlexer_t* lex)
{
    bool ok; 
    if(lex->nextposition >= lex->inputlength)
    {
        lex->ch = '\0';
    }
    else
    {
        lex->ch = lex->inputsource[lex->nextposition];
    }
    lex->position = lex->nextposition;
    lex->nextposition++;
    if(lex->ch == '\n')
    {
        lex->line++;
        lex->column = -1;
        ok = mc_lexer_addline(lex, lex->nextposition);
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
    if(lex->nextposition >= lex->inputlength)
    {
        return '\0';
    }
    return lex->inputsource[lex->nextposition];
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
    int i;
    for(i = 0; i < allowedlen; i++)
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
    int len;
    int position;
    position = lex->position;
    len = 0;
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
    return lex->inputsource + position;
}

TMPSTATIC const char* mc_lexer_scannumber(mcastlexer_t* lex, int* outlen)
{
    int len;
    int position;
    static const char allowed[] = ".xXaAbBcCdDeEfF";
    position = lex->position;
    while(mc_lexer_charisdigit(lex->ch) || mc_lexer_charisoneof(lex->ch, allowed, MC_UTIL_STATICARRAYSIZE(allowed) - 1))
    {
        mc_lexer_readchar(lex);
    }
    len = lex->position - position;
    *outlen = len;
    return lex->inputsource + position;
}

TMPSTATIC const char* mc_lexer_scanstring(mcastlexer_t* lex, char delimiter, bool istemplate, bool* outtemplatefound, int* outlen)
{
    bool escaped;
    int len;
    int position;
    *outlen = 0;
    escaped = false;
    position = lex->position;
    while(true)
    {
        if(lex->ch == '\0')
        {
            return NULL;
        }
        if(lex->ch == delimiter && !escaped)
        {
            break;
        }
        if(istemplate && !escaped && lex->ch == '$' && mc_lexer_peekchar(lex) == '{')
        {
            *outtemplatefound = true;
            break;
        }
        escaped = false;
        if(lex->ch == '\\')
        {
            escaped = true;
        }
        mc_lexer_readchar(lex);
    }
    len = lex->position - position;
    *outlen = len;
    return lex->inputsource + position;
}

TMPSTATIC mcasttoktype_t mc_lexer_lookupident(const char* ident, int len)
{
    int i;
    int klen;
    static struct
    {
        const char* value;
        mcasttoktype_t type;
    } keywords[] = {
        { "function", MC_TOK_FUNCTION },
        { "const", MC_TOK_CONST },
        { "var", MC_TOK_VAR },
        { "true", MC_TOK_TRUE },
        { "false", MC_TOK_FALSE },
        { "if", MC_TOK_IF },
        { "else", MC_TOK_ELSE },
        { "return", MC_TOK_RETURN },
        { "while", MC_TOK_WHILE },
        { "break", MC_TOK_BREAK },
        { "for", MC_TOK_FOR },
        { "in", MC_TOK_IN },
        { "continue", MC_TOK_CONTINUE },
        { "null", MC_TOK_NULL },
        { "import", MC_TOK_IMPORT },
        { "recover", MC_TOK_RECOVER },
        { NULL, (mcasttoktype_t)0}
    };
    for(i = 0; keywords[i].value != NULL; i++)
    {
        klen = strlen(keywords[i].value);
        if(klen == len && MC_UTIL_STRNEQ(ident, keywords[i].value, len))
        {
            return keywords[i].type;
        }
    }
    return MC_TOK_IDENT;
}

TMPSTATIC void mc_lexer_skipspace(mcastlexer_t* lex)
{
    char ch;
    ch = lex->ch;
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

    if(lex->line < mc_ptrarray_count(lex->file->lines))
    {
        return true;
    }

    const char* linestart = lex->inputsource + offset;
    const char* newlineptr = strchr(linestart, '\n');
    char* line = NULL;
    if(!newlineptr)
    {
        line = mc_util_strdup(lex->pstate, linestart);
    }
    else
    {
        size_t linelen = newlineptr - linestart;
        line = mc_util_strndup(lex->pstate, linestart, linelen);
    }
    if(!line)
    {
        lex->failed = true;
        return false;
    }
    ok = mc_ptrarray_push(lex->file->lines, line);
    if(!ok)
    {
        lex->failed = true;
        mc_allocator_free(lex->pstate, line);
        return false;
    }
    return true;
}

TMPSTATIC mcastcompiledfile_t* mc_compiledfile_make(mcstate_t* state, const char* path)
{
    size_t len;
    const char* lastslashpos;
    mcastcompiledfile_t* file;
    file = (mcastcompiledfile_t*)mc_allocator_alloc(state, sizeof(mcastcompiledfile_t));
    if(!file)
    {
        return NULL;
    }
    memset(file, 0, sizeof(mcastcompiledfile_t));
    file->pstate = state;
    lastslashpos = strrchr(path, '/');
    if(lastslashpos)
    {
        len = lastslashpos - path + 1;
        file->dir_path = mc_util_strndup(state, path, len);
    }
    else
    {
        file->dir_path = mc_util_strdup(state, "");
    }
    if(!file->dir_path)
    {
        goto error;
    }
    file->path = mc_util_strdup(state, path);
    if(!file->path)
    {
        goto error;
    }
    file->lines = mc_ptrarray_make(state);
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
    int i;
    void* item;
    mcstate_t* state;
    if(!file)
    {
        return;
    }
    state = file->pstate;
    for(i = 0; i < mc_ptrarray_count(file->lines); i++)
    {
        item = (void*)mc_ptrarray_get(file->lines, i);
        mc_allocator_free(state, item);
    }
    mc_ptrarray_destroy(file->lines, NULL);
    mc_allocator_free(state, file->dir_path);
    mc_allocator_free(state, file->path);
    mc_allocator_free(state, file);
}

TMPSTATIC mcastcompiler_t* mc_compiler_make(mcstate_t* state, mcconfig_t* config, mcgcmemory_t* mem, mcerrlist_t* errors, mcptrarray_t* files, mcglobalstore_t* global_store)
{
    bool ok;
    mcastcompiler_t* comp = (mcastcompiler_t*)mc_allocator_alloc(state, sizeof(mcastcompiler_t));
    if(!comp)
    {
        return NULL;
    }
    ok = mc_compiler_init(comp, state, config, mem, errors, files, global_store);
    if(!ok)
    {
        mc_allocator_free(state, comp);
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
    mc_allocator_free(state, comp);
}

TMPSTATIC compilation_result_t* mc_compiler_compilesource(mcastcompiler_t* comp, const char* code)
{
    bool ok;
    mcastcompiler_t compshallowcopy;
    mcastscopecomp_t* compscope;
    compilation_result_t* res;
    compscope = mc_compiler_getcompilationscope(comp);

    MC_ASSERT(mc_basicarray_count(comp->src_positions_stack) == 0);
    MC_ASSERT(mc_basicarray_count(compscope->bytecode) == 0);
    MC_ASSERT(mc_basicarray_count(compscope->break_ip_stack) == 0);
    MC_ASSERT(mc_basicarray_count(compscope->continue_ip_stack) == 0);

    mc_basicarray_clear(comp->src_positions_stack);
    mc_basicarray_clear(compscope->bytecode);
    mc_basicarray_clear(compscope->src_positions);
    mc_basicarray_clear(compscope->break_ip_stack);
    mc_basicarray_clear(compscope->continue_ip_stack);
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
    compscope = mc_compiler_getcompilationscope(comp);
    MC_ASSERT(compscope->outer == NULL);
    compscope = mc_compiler_getcompilationscope(comp);
    res = compilation_scope_orphan_result(compscope);
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
    mcastscopefile_t* filescope = (mcastscopefile_t*)mc_ptrarray_top(comp->file_scopes);
    if(!filescope)
    {
        MC_ASSERT(false);
        return NULL;
    }
    return filescope->symbol_table;
}

TMPSTATIC void mc_compiler_setsymtable(mcastcompiler_t* comp, mcastsymtable_t* table)
{
    mcastscopefile_t* filescope = (mcastscopefile_t*)mc_ptrarray_top(comp->file_scopes);
    if(!filescope)
    {
        MC_ASSERT(false);
        return;
    }
    filescope->symbol_table = table;
}

TMPSTATIC mcbasicarray_t* mc_compiler_getconstants(mcastcompiler_t* comp)
{
    return comp->constants;
}

TMPSTATIC bool mc_compiler_init(mcastcompiler_t* comp, mcstate_t* state, mcconfig_t* cfg, mcgcmemory_t* mem, mcerrlist_t* errors, mcptrarray_t* files, mcglobalstore_t* gstor)
{
    bool ok;
    memset(comp, 0, sizeof(mcastcompiler_t));
    comp->pstate = state;
    comp->config = cfg;
    comp->mem = mem;
    comp->errors = errors;
    comp->files = files;
    comp->global_store = gstor;
    comp->file_scopes = mc_ptrarray_make(state);
    if(!comp->file_scopes)
    {
        goto err;
    }
    comp->constants = mc_basicarray_make(state, sizeof(mcvalue_t));
    if(!comp->constants)
    {
        goto err;
    }
    comp->src_positions_stack = mc_basicarray_make(state, sizeof(mcastlocation_t));
    if(!comp->src_positions_stack)
    {
        goto err;
    }
    comp->modules = mc_genericdict_make(state, (mcdictitemcopyfn_t)mc_module_copy, (mcdictitemdestroyfn_t)mc_module_destroy);
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
    comp->string_constants_positions = mc_genericdict_make(comp->pstate, NULL, NULL);
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
    int i;
    int* val;
    if(!comp)
    {
        return;
    }
    for(i = 0; i < mc_genericdict_count(comp->string_constants_positions); i++)
    {
        val = (int*)mc_genericdict_getvalueat(comp->string_constants_positions, i);
        mc_allocator_free(comp->pstate, val);
    }
    mc_genericdict_destroy(comp->string_constants_positions);
    while(mc_ptrarray_count(comp->file_scopes) > 0)
    {
        mc_compiler_filescopepop(comp);
    }
    while(mc_compiler_getcompilationscope(comp))
    {
        mc_compiler_popcompilationscope(comp);
    }
    mc_genericdict_destroyitemsanddict(comp->modules);
    mc_basicarray_destroy(comp->src_positions_stack);
    mc_basicarray_destroy(comp->constants);
    mc_ptrarray_destroy(comp->file_scopes, NULL);
    memset(comp, 0, sizeof(mcastcompiler_t));
}

TMPSTATIC bool mc_compiler_initshallowcopy(mcastcompiler_t* copy, mcastcompiler_t* src)
{
    bool ok;
    int i;
    int* val;
    int* valcopy;
    char* loadednamecopy;
    const char* key;
    const char* loadedname;
    mcgenericdict_t* modulescopy;
    mcbasicarray_t* constantscopy;
    mcptrarray_t* srcloadedmodulenames;
    mcptrarray_t* copyloadedmodulenames;
    mcastsymtable_t* srcst;
    mcastsymtable_t* srcstocopy;
    mcastsymtable_t* copyst;
    mcastscopefile_t* srcfilescope;
    mcastscopefile_t* copyfilescope;
    ok = mc_compiler_init(copy, src->pstate, src->config, src->mem, src->errors, src->files, src->global_store);
    if(!ok)
    {
        return false;
    }
    srcst = mc_compiler_getsymtable(src);
    MC_ASSERT(mc_ptrarray_count(src->file_scopes) == 1);
    MC_ASSERT(srcst->outer == NULL);
    srcstocopy = mc_symtable_copy(srcst);
    if(!srcstocopy)
    {
        goto err;
    }
    copyst = mc_compiler_getsymtable(copy);
    mc_symtable_destroy(copyst);
    copyst = NULL;
    mc_compiler_setsymtable(copy, srcstocopy);
    modulescopy = mc_genericdict_copy(src->modules);
    if(!modulescopy)
    {
        goto err;
    }
    mc_genericdict_destroyitemsanddict(copy->modules);
    copy->modules = modulescopy;
    constantscopy = mc_basicarray_copy(src->constants);
    if(!constantscopy)
    {
        goto err;
    }
    mc_basicarray_destroy(copy->constants);
    copy->constants = constantscopy;
    for(i = 0; i < mc_genericdict_count(src->string_constants_positions); i++)
    {
        key = mc_genericdict_getkeyat(src->string_constants_positions, i);
        val = (int*)mc_genericdict_getvalueat(src->string_constants_positions, i);
        valcopy = (int*)mc_allocator_alloc(src->pstate, sizeof(int));
        if(!valcopy)
        {
            goto err;
        }
        *valcopy = *val;
        ok = mc_genericdict_set(copy->string_constants_positions, key, valcopy);
        if(!ok)
        {
            mc_allocator_free(src->pstate, valcopy);
            goto err;
        }
    }
    srcfilescope = (mcastscopefile_t*)mc_ptrarray_top(src->file_scopes);
    copyfilescope = (mcastscopefile_t*)mc_ptrarray_top(copy->file_scopes);
    srcloadedmodulenames = srcfilescope->loaded_module_names;
    copyloadedmodulenames = copyfilescope->loaded_module_names;
    for(i = 0; i < mc_ptrarray_count(srcloadedmodulenames); i++)
    {

        loadedname = (const char*)mc_ptrarray_get(srcloadedmodulenames, i);
        loadednamecopy = mc_util_strdup(copy->pstate, loadedname);
        if(!loadednamecopy)
        {
            goto err;
        }
        ok = mc_ptrarray_push(copyloadedmodulenames, loadednamecopy);
        if(!ok)
        {
            mc_allocator_free(copy->pstate, loadednamecopy);
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
    { "GETGLOBALBUILTIN", 1, { 2 } },
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

TMPSTATIC mcopdefinition_t* opcode_lookup(mcinternopcode_t op)
{
    if(op <= OPCODE_NONE || op >= OPCODE_MAX)
    {
        return NULL;
    }
    return &g_definitions[op];
}

TMPSTATIC const char* opcode_get_name(mcinternopcode_t op)
{
    if(op <= OPCODE_NONE || op >= OPCODE_MAX)
    {
        return NULL;
    }
    return g_definitions[op].name;
}

TMPSTATIC int mc_compiler_gencode(mcinternopcode_t op, int operandscount, const uint64_t* operands, mcbasicarray_t* res)
{
    bool ok;
    int i;
    int width;
    int instrlen;
    uint8_t val;
    mcopdefinition_t* def;
    def = opcode_lookup(op);
    if(!def)
    {
        return 0;
    }
    instrlen = 1;
    for(i = 0; i < def->num_operands; i++)
    {
        instrlen += def->operand_widths[i];
    }
    val = op;
    ok = false;
    ok = mc_basicarray_push(res, &val);
    if(!ok)
    {
        return 0;
    }
#define APPEND_BYTE(n)                           \
    do                                           \
    {                                            \
        val = (uint8_t)(operands[i] >> (n * 8)); \
        ok = mc_basicarray_push(res, &val);               \
        if(!ok)                                  \
        {                                        \
            return 0;                            \
        }                                        \
    } while(0)

    for(i = 0; i < operandscount; i++)
    {
        width = def->operand_widths[i];
        switch(width)
        {
            case 1:
                {
                    APPEND_BYTE(0);
                }
                break;
            case 2:
                {
                    APPEND_BYTE(1);
                    APPEND_BYTE(0);
                }
                break;
            case 4:
                {
                    APPEND_BYTE(3);
                    APPEND_BYTE(2);
                    APPEND_BYTE(1);
                    APPEND_BYTE(0);
                }
                break;
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
                }
                break;
            default:
                {
                    MC_ASSERT(false);
                }
                break;
        }
    }
#undef APPEND_BYTE
    return instrlen;
}


TMPSTATIC int mc_compiler_emit(mcastcompiler_t* comp, mcinternopcode_t op, int operandscount, uint64_t* operands)
{
    bool ok;
    int i;
    int ip;
    int len;
    mcastlocation_t* srcpos;
    ip = mc_compiler_getip(comp);
    len = mc_compiler_gencode(op, operandscount, operands, mc_compiler_getbytecode(comp));
    if(len == 0)
    {
        return -1;
    }
    for(i = 0; i < len; i++)
    {
        srcpos = (mcastlocation_t*)mc_basicarray_top(comp->src_positions_stack);
        MC_ASSERT(srcpos->line >= 0);
        MC_ASSERT(srcpos->column >= 0);
        ok = mc_basicarray_push(mc_compiler_getsrcpositions(comp), srcpos);
        if(!ok)
        {
            return -1;
        }
    }
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    compscope->last_opcode = op;
    return ip;
}

TMPSTATIC mcastscopecomp_t* mc_compiler_getcompilationscope(mcastcompiler_t* comp)
{
    return comp->compilation_scope;
}

TMPSTATIC bool mc_compiler_pushcompilationscope(mcastcompiler_t* comp)
{
    mcastscopecomp_t* nscope;
    mcastscopecomp_t* currentscope;
    currentscope = mc_compiler_getcompilationscope(comp);
    nscope = compilation_scope_make(comp->pstate, currentscope);
    if(!nscope)
    {
        return false;
    }
    mc_compiler_setcompilationscope(comp, nscope);
    return true;
}

TMPSTATIC void mc_compiler_popcompilationscope(mcastcompiler_t* comp)
{
    mcastscopecomp_t* currentscope = mc_compiler_getcompilationscope(comp);
    MC_ASSERT(currentscope);
    mc_compiler_setcompilationscope(comp, currentscope->outer);
    compilation_scope_destroy(currentscope);
}

TMPSTATIC bool mc_compiler_pushsymtable(mcastcompiler_t* comp, int globaloffset)
{
    mcastscopefile_t* filescope = (mcastscopefile_t*)mc_ptrarray_top(comp->file_scopes);
    if(!filescope)
    {
        MC_ASSERT(false);
        return false;
    }
    mcastsymtable_t* currenttable = filescope->symbol_table;
    filescope->symbol_table = mc_symtable_make(comp->pstate, currenttable, comp->global_store, globaloffset);
    if(!filescope->symbol_table)
    {
        filescope->symbol_table = currenttable;
        return false;
    }
    return true;
}

TMPSTATIC void mc_compiler_popsymtable(mcastcompiler_t* comp)
{
    mcastscopefile_t* filescope = (mcastscopefile_t*)mc_ptrarray_top(comp->file_scopes);
    if(!filescope)
    {
        MC_ASSERT(false);
        return;
    }
    mcastsymtable_t* currenttable = filescope->symbol_table;
    if(!currenttable)
    {
        MC_ASSERT(false);
        return;
    }
    filescope->symbol_table = currenttable->outer;
    mc_symtable_destroy(currenttable);
}

TMPSTATIC mcinternopcode_t mc_compiler_getlastopcode(mcastcompiler_t* comp)
{
    mcastscopecomp_t* currentscope = mc_compiler_getcompilationscope(comp);
    return currentscope->last_opcode;
}

TMPSTATIC bool mc_compiler_docompilesource(mcastcompiler_t* comp, const char* code)
{
    bool ok;
    mcastscopefile_t* filescope = (mcastscopefile_t*)mc_ptrarray_top(comp->file_scopes);
    MC_ASSERT(filescope);

    mcptrarray_t* statements = parser_parse_all(filescope->parser, code, filescope->file);
    if(!statements)
    {
        /* errors are added by parser */
        return false;
    }
    {
        mcprintstate_t pr;
        mc_printer_init(&pr, comp->pstate, 0, stderr, true);
        fprintf(stderr, "---AST dump begin---\n");
        mc_astprint_stmtlist(&pr, statements);
        fprintf(stderr, "\n---AST dump end---\n");
    }
    ok = mc_compiler_compilestmtlist(comp, statements);

    mc_ptrarray_destroy(statements, (mcptrarrayitemdestroyfn_t)statement_destroy);
    /*
    * //Left for debugging purposes
    if (ok)
    {
        mcprintstate_t *buf = mc_printer_make(NULL, NULL);
        code_to_string(
            mc_basicarray_data(comp->compilation_scope->bytecode),
            mc_basicarray_data(comp->compilation_scope->src_positions),
            mc_basicarray_count(comp->compilation_scope->bytecode), buf);
        puts(mc_printer_getstring(buf));
        mc_printer_destroy(buf);
    }
    */

    return ok;
}

TMPSEMISTATIC bool mc_compiler_compilestmtlist(mcastcompiler_t* comp, mcptrarray_t* statements)
{
    bool ok;
    int i;
    mcastexpression_t* expr;
    ok = true;
    for(i = 0; i < mc_ptrarray_count(statements); i++)
    {
        expr = (mcastexpression_t*)mc_ptrarray_get(statements, i);
        ok = mc_compiler_compilestatement(comp, expr);
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
    bool result;
    int i;
    char* code;
    char* filepath;
    char* namecopy;
    const char* modulepath;
    const char* modulename;
    const char* loadedname;
    const char* filepathnoncanonicalised;
    mcprintstate_t* filepathbuf;
    mcastsymtable_t* symtab;
    mcastscopefile_t* fs;
    module_t* module;
    mcastsymtable_t* st;
    mcastscopefile_t* filescope;
    mcastsymbol_t* symbol;
    /* todo: split into smaller functions */
    result = false;
    filepath = NULL;
    code = NULL;
    filescope = (mcastscopefile_t*)mc_ptrarray_top(comp->file_scopes);
    modulepath = importstmt->exprimportstmt.path;
    modulename = mc_util_getmodulename(modulepath);
    for(i = 0; i < mc_ptrarray_count(filescope->loaded_module_names); i++)
    {
        loadedname = (const char*)mc_ptrarray_get(filescope->loaded_module_names, i);
        if(kg_streq(loadedname, modulename))
        {
            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, importstmt->pos, "Module \"%s\" was already imported", modulename);
            result = false;
            goto end;
        }
    }
    filepathbuf = mc_printer_make(comp->pstate, NULL);
    if(!filepathbuf)
    {
        result = false;
        goto end;
    }
    if(kg_is_path_absolute(modulepath))
    {
        mc_printer_printf(filepathbuf, "%s.mc", modulepath);
    }
    else
    {
        mc_printer_printf(filepathbuf, "%s%s.mc", filescope->file->dir_path, modulepath);
    }

    if(mc_printer_failed(filepathbuf))
    {
        mc_printer_destroy(filepathbuf);
        result = false;
        goto end;
    }
    filepathnoncanonicalised = mc_printer_getstring(filepathbuf);
    filepath = kg_canonicalise_path(comp->pstate, filepathnoncanonicalised);
    mc_printer_destroy(filepathbuf);
    if(!filepath)
    {
        result = false;
        goto end;
    }
    symtab = mc_compiler_getsymtable(comp);
    if(symtab->outer != NULL || mc_ptrarray_count(symtab->block_scopes) > 1)
    {
        mc_errlist_push(comp->errors, MC_ERROR_COMPILING, importstmt->pos, "Modules can only be imported in global scope");
        result = false;
        goto end;
    }
    for(i = 0; i < mc_ptrarray_count(comp->file_scopes); i++)
    {
        fs = (mcastscopefile_t*)mc_ptrarray_get(comp->file_scopes, i);
        if(MC_UTIL_STREQ(fs->file->path, filepath))
        {
            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, importstmt->pos, "Cyclic reference of file \"%s\"", filepath);
            result = false;
            goto end;
        }
    }
    module = (module_t*)mc_genericdict_get(comp->modules, filepath);
    if(!module)
    {
        /* todo: create new module function */
        if(!comp->config->fileio.fnreadfile)
        {
            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, importstmt->pos, "Cannot import module \"%s\", file read function not configured", filepath);
            result = false;
            goto end;
        }
        code = comp->config->fileio.fnreadfile(comp->config->fileio.context, filepath);
        if(!code)
        {
            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, importstmt->pos, "Reading module file \"%s\" failed", filepath);
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
        st = mc_compiler_getsymtable(comp);
        for(i = 0; i < mc_symtable_getmodglobalsymcount(st); i++)
        {
            symbol = mc_symtable_getmodglobalsymat(st, i);
            mc_module_addsymbol(module, symbol);
        }

        mc_compiler_filescopepop(comp);

        ok = mc_genericdict_set(comp->modules, filepath, module);
        if(!ok)
        {
            mc_module_destroy(module);
            result = false;
            goto end;
        }
    }

    for(i = 0; i < mc_ptrarray_count(module->symbols); i++)
    {
        symbol = (mcastsymbol_t*)mc_ptrarray_get(module->symbols, i);
        ok = mc_symtable_addmodsymbol(symtab, symbol);
        if(!ok)
        {
            result = false;
            goto end;
        }
    }
    namecopy = mc_util_strdup(comp->pstate, modulename);
    if(!namecopy)
    {
        result = false;
        goto end;
    }
    ok = mc_ptrarray_push(filescope->loaded_module_names, namecopy);
    if(!ok)
    {
        mc_allocator_free(comp->pstate, namecopy);
        result = false;
        goto end;
    }
    result = true;
end:
    mc_allocator_free(comp->pstate, filepath);
    mc_allocator_free(comp->pstate, code);
    return result;
}

TMPSEMISTATIC bool mc_compiler_compilestatement(mcastcompiler_t* comp, mcastexpression_t* expr)
{
    bool ok;
    int ip;
    uint64_t opbuf[10];
    mcastscopecomp_t* compscope;
    mcastsymtable_t* symtab;
    ok = false;
    ip = -1;
    ok = mc_basicarray_push(comp->src_positions_stack, &expr->pos);
    if(!ok)
    {
        return false;
    }
    compscope = mc_compiler_getcompilationscope(comp);
    symtab = mc_compiler_getsymtable(comp);
    switch(expr->type)
    {
        case MC_EXPR_STMTEXPRESSION:
        {
            ok = mc_compiler_compileexpression(comp, expr->exprexpression);
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
            mcastsymbol_t* symbol;
            ok = mc_compiler_compileexpression(comp, expr->exprdefine.value);
            if(!ok)
            {
                return false;
            }
            symbol = mc_compiler_defsymbol(comp, expr->exprdefine.name->pos, expr->exprdefine.name->value, expr->exprdefine.assignable, false);
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
            int i;
            int afteraltip;
            int nextcasejumpip;
            int jumptoendip;
            int afterelifip;
            int* pos;
            if_case_t* ifcase;
            if_statement_t* ifstmt;
            ifstmt = &expr->exprifstmt;
            mcbasicarray_t* jumptoendips;
            jumptoendips = mc_basicarray_make(comp->pstate, sizeof(int));
            if(!jumptoendips)
            {
                goto statementiferror;
            }
            for(i = 0; i < mc_ptrarray_count(ifstmt->cases); i++)
            {
                ifcase = (if_case_t*)mc_ptrarray_get(ifstmt->cases, i);
                ok = mc_compiler_compileexpression(comp, ifcase->ifcond);
                if(!ok)
                {
                    goto statementiferror;
                }
                opbuf[0] = 0xbeef;
                nextcasejumpip = mc_compiler_emit(comp, OPCODE_JUMP_IF_FALSE, 1, opbuf);
                ok = mc_compiler_compilecodeblock(comp, ifcase->consequence);
                if(!ok)
                {
                    goto statementiferror;
                }
                /* don't emit jump for the last statement */
                if(i < (mc_ptrarray_count(ifstmt->cases) - 1) || ifstmt->alternative)
                {
                    opbuf[0] = 0xbeef;
                    jumptoendip = mc_compiler_emit(comp, OPCODE_JUMP, 1, opbuf);
                    ok = mc_basicarray_push(jumptoendips, &jumptoendip);
                    if(!ok)
                    {
                        goto statementiferror;
                    }
                }
                afterelifip = mc_compiler_getip(comp);
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
            afteraltip = mc_compiler_getip(comp);
            for(i = 0; i < mc_basicarray_count(jumptoendips); i++)
            {
                pos = (int*)mc_basicarray_get(jumptoendips, i);
                mc_compiler_changeuint16operand(comp, *pos + 1, afteraltip);
            }
            mc_basicarray_destroy(jumptoendips);
            break;
        statementiferror:
            mc_basicarray_destroy(jumptoendips);
            return false;
        }
        case MC_EXPR_STMTRETURN:
        {
            if(compscope->outer == NULL)
            {
                mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "Nothing to return from");
                return false;
            }
            ip = -1;
            if(expr->exprreturnvalue)
            {
                ok = mc_compiler_compileexpression(comp, expr->exprreturnvalue);
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
            int beforetestip;
            int aftertestip;
            int afterbodyip;
            int jumptoafterbodyip;
            while_loop_statement_t* loop;
            loop = &expr->exprwhileloopstmt;
            beforetestip = mc_compiler_getip(comp);
            ok = mc_compiler_compileexpression(comp, loop->loopcond);
            if(!ok)
            {
                return false;
            }
            aftertestip = mc_compiler_getip(comp);
            opbuf[0] = aftertestip + 6;
            ip = mc_compiler_emit(comp, OPCODE_JUMP_IF_TRUE, 1, opbuf);
            if(ip < 0)
            {
                return false;
            }
            opbuf[0] = 0xdead;
            jumptoafterbodyip = mc_compiler_emit(comp, OPCODE_JUMP, 1, opbuf);
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
            opbuf[0] = beforetestip;
            ip = mc_compiler_emit(comp, OPCODE_JUMP, 1, opbuf);
            if(ip < 0)
            {
                return false;
            }
            afterbodyip = mc_compiler_getip(comp);
            mc_compiler_changeuint16operand(comp, jumptoafterbodyip + 1, afterbodyip);
            break;
        }
        case MC_EXPR_STMTBREAK:
        {
            int breakip;
            breakip = mc_compiler_getbreakip(comp);
            if(breakip < 0)
            {
                mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "Nothing to break from.");
                return false;
            }
            opbuf[0] = breakip;
            ip = mc_compiler_emit(comp, OPCODE_JUMP, 1, opbuf);
            if(ip < 0)
            {
                return false;
            }
            break;
        }
        case MC_EXPR_STMTCONTINUE:
        {
            int continueip;
            continueip = mc_compiler_getcontinueip(comp);
            if(continueip < 0)
            {
                mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "Nothing to continue from.");
                return false;
            }
            opbuf[0] = continueip;
            ip = mc_compiler_emit(comp, OPCODE_JUMP, 1, opbuf);
            if(ip < 0)
            {
                return false;
            }
            break;
        }
        case MC_EXPR_STMTLOOPFOREACH:
        {
            int jumptoafterupdateip;
            int updateip;
            int afterupdateip;
            int aftertestip;
            int jumptoafterbodyip;
            int afterbodyip;
            mcastsymbol_t* itersymbol;
            mcastsymbol_t* indexsymbol;
            mcastsymbol_t* sourcesymbol;
            foreach_statement_t* foreach;
            foreach = &expr->exprforeachloopstmt;
            ok = mc_symtable_pushblockscope(symtab);
            if(!ok)
            {
                return false;
            }
            /* Init */
            indexsymbol = mc_compiler_defsymbol(comp, expr->pos, "@i", false, true);
            if(!indexsymbol)
            {
                return false;
            }
            opbuf[0] = 0;
            ip = mc_compiler_emit(comp, OPCODE_NUMBER, 1, opbuf);
            if(ip < 0)
            {
                return false;
            }
            ok = mc_compiler_storesymbol(comp, indexsymbol, true);
            if(!ok)
            {
                return false;
            }
            sourcesymbol = NULL;
            if(foreach->source->type == MC_EXPR_IDENT)
            {
                sourcesymbol = mc_symtable_resolve(symtab, foreach->source->exprident->value);
                if(!sourcesymbol)
                {
                    mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, foreach->source->pos, "Symbol \"%s\" could not be resolved", foreach->source->exprident->value);
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
            opbuf[0] = 0xbeef;
            jumptoafterupdateip = mc_compiler_emit(comp, OPCODE_JUMP, 1, opbuf);
            if(jumptoafterupdateip < 0)
            {
                return false;
            }
            updateip = mc_compiler_getip(comp);
            ok = mc_compiler_readsymbol(comp, indexsymbol);
            if(!ok)
            {
                return false;
            }
            opbuf[0] = mc_util_doubletouint64(1);
            ip = mc_compiler_emit(comp, OPCODE_NUMBER, 1, opbuf);
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
            afterupdateip = mc_compiler_getip(comp);
            mc_compiler_changeuint16operand(comp, jumptoafterupdateip + 1, afterupdateip);
            /* Test */
            ok = mc_basicarray_push(comp->src_positions_stack, &foreach->source->pos);
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
            mc_basicarray_pop(comp->src_positions_stack, NULL);
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
            aftertestip = mc_compiler_getip(comp);
            opbuf[0] = aftertestip + 6;
            ip = mc_compiler_emit(comp, OPCODE_JUMP_IF_FALSE, 1, opbuf);
            if(ip < 0)
            {
                return false;
            }
            opbuf[0] = 0xdead;
            jumptoafterbodyip = mc_compiler_emit(comp, OPCODE_JUMP, 1, opbuf);
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
            itersymbol = mc_compiler_defsymbol(comp, foreach->iterator->pos, foreach->iterator->value, false, false);
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
            opbuf[0] = updateip;
            ip = mc_compiler_emit(comp, OPCODE_JUMP, 1, opbuf);
            if(ip < 0)
            {
                return false;
            }
            afterbodyip = mc_compiler_getip(comp);
            mc_compiler_changeuint16operand(comp, jumptoafterbodyip + 1, afterbodyip);
            mc_symtable_popblockscope(symtab);
            break;
        }
        case MC_EXPR_STMTLOOPFORCLASSIC:
            {
                int afterbodyip;
                int jumptoafterupdateip;
                int updateip;
                int afterupdateip;
                int aftertestip;
                int jumptoafterbodyip;
                for_loop_statement_t* loop;
                loop = &expr->exprforloopstmt;
                ok = mc_symtable_pushblockscope(symtab);
                if(!ok)
                {
                    return false;
                }
                /* Init */
                jumptoafterupdateip = 0;
                ok = false;
                if(loop->init)
                {
                    ok = mc_compiler_compilestatement(comp, loop->init);
                    if(!ok)
                    {
                        return false;
                    }
                    opbuf[0] = 0xbeef;
                    jumptoafterupdateip = mc_compiler_emit(comp, OPCODE_JUMP, 1, opbuf);
                    if(jumptoafterupdateip < 0)
                    {
                        return false;
                    }
                }
                /* Update */
                updateip = mc_compiler_getip(comp);
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
                    afterupdateip = mc_compiler_getip(comp);
                    mc_compiler_changeuint16operand(comp, jumptoafterupdateip + 1, afterupdateip);
                }
                /* Test */
                if(loop->loopcond)
                {
                    ok = mc_compiler_compileexpression(comp, loop->loopcond);
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
                aftertestip = mc_compiler_getip(comp);
                opbuf[0] = aftertestip + 6;
                ip = mc_compiler_emit(comp, OPCODE_JUMP_IF_TRUE, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                opbuf[0] = 0xdead;
                jumptoafterbodyip = mc_compiler_emit(comp, OPCODE_JUMP, 1, opbuf);
                if(jumptoafterbodyip < 0)
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

                ok = mc_compiler_compilecodeblock(comp, loop->body);
                if(!ok)
                {
                    return false;
                }
                mc_compiler_popbreakip(comp);
                mc_compiler_popcontinueip(comp);
                opbuf[0] = updateip;
                ip = mc_compiler_emit(comp, OPCODE_JUMP, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                afterbodyip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, jumptoafterbodyip + 1, afterbodyip);
                mc_symtable_popblockscope(symtab);
            }
            break;
        case MC_EXPR_STMTBLOCK:
            {
                ok = mc_compiler_compilecodeblock(comp, expr->exprblockstmt);
                if(!ok)
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTIMPORT:
            {
                ok = mc_compiler_compileimport(comp, expr);
                if(!ok)
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTRECOVER:
            {
                int recover_ip;
                int afterrecoverip;
                int afterjumptorecoverip;
                int jumptoafterrecoverip;
                mcastsymbol_t* errorsymbol;
                recover_statement_t* recover;
                recover = &expr->exprrecoverstmt;
                if(mc_symtable_ismodglobalscope(symtab))
                {
                    mc_errlist_push(comp->errors, MC_ERROR_COMPILING, expr->pos, "Recover statement cannot be defined in global scope");
                    return false;
                }
                if(!mc_symtable_istopblockscope(symtab))
                {
                    mc_errlist_push(comp->errors, MC_ERROR_COMPILING, expr->pos, "Recover statement cannot be defined within other statements");
                    return false;
                }
                opbuf[0] = 0xbeef;
                recover_ip = mc_compiler_emit(comp, OPCODE_SET_RECOVER, 1, opbuf);
                if(recover_ip < 0)
                {
                    return false;
                }
                opbuf[0] = 0xbeef;
                jumptoafterrecoverip = mc_compiler_emit(comp, OPCODE_JUMP, 1, opbuf);
                if(jumptoafterrecoverip < 0)
                {
                    return false;
                }
                afterjumptorecoverip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, recover_ip + 1, afterjumptorecoverip);
                ok = mc_symtable_pushblockscope(symtab);
                if(!ok)
                {
                    return false;
                }
                errorsymbol = mc_compiler_defsymbol(comp, recover->errident->pos, recover->errident->value, false, false);
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
                    mc_errlist_push(comp->errors, MC_ERROR_COMPILING, expr->pos, "Recover body must end with a return statement");
                    return false;
                }
                mc_symtable_popblockscope(symtab);
                afterrecoverip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, jumptoafterrecoverip + 1, afterrecoverip);
            }
            break;

        default:
        {
            MC_ASSERT(false);
            return false;
        }
    }
    mc_basicarray_pop(comp->src_positions_stack, NULL);
    return true;
}

TMPSEMISTATIC bool mc_compiler_compileexpression(mcastcompiler_t* comp, mcastexpression_t* expr)
{
    bool ok;
    int ip;
    uint64_t opbuf[10];
    mcastexpression_t* exproptimized;
    ok = false;
    ip = -1;
    exproptimized = NULL;
    #if 0
    exproptimized = mc_optimizer_optexpression(expr);
    if(exproptimized != NULL)
    {
        expr = exproptimized;
    }
    #endif

    ok = mc_basicarray_push(comp->src_positions_stack, &expr->pos);
    if(!ok)
    {
        return false;
    }

    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    mcastsymtable_t* symtab = mc_compiler_getsymtable(comp);

    bool res = false;

    switch(expr->type)
    {
        case MC_EXPR_INFIX:
        {
            bool rearrange = false;

            mcinternopcode_t op = OPCODE_NONE;
            switch(expr->exprinfix.op)
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
                    mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "Unknown infix operator");
                    goto error;
                }
            }

            mcastexpression_t* left = rearrange ? expr->exprinfix.right : expr->exprinfix.left;
            mcastexpression_t* right = rearrange ? expr->exprinfix.left : expr->exprinfix.right;

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

            switch(expr->exprinfix.op)
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
            double number = expr->exprlitnumber;
            opbuf[0] = mc_util_doubletouint64(number);
            ip = mc_compiler_emit(comp, OPCODE_NUMBER, 1, opbuf);
            if(ip < 0)
            {
                goto error;
            }

            break;
        }
        case MC_EXPR_STRINGLITERAL:
        {
            int pos = 0;
            int* currentpos = (int*)mc_genericdict_get(comp->string_constants_positions, expr->exprlitstring.data);
            if(currentpos)
            {
                pos = *currentpos;
            }
            else
            {
                mcvalue_t obj = mc_value_makestringlen(comp->pstate, expr->exprlitstring.data, expr->exprlitstring.length);
                if(object_is_null(obj))
                {
                    goto error;
                }

                pos = mc_compiler_addconstant(comp, obj);
                if(pos < 0)
                {
                    goto error;
                }

                int* posval = (int*)mc_allocator_alloc(comp->pstate, sizeof(int));
                if(!posval)
                {
                    goto error;
                }

                *posval = pos;
                ok = mc_genericdict_set(comp->string_constants_positions, expr->exprlitstring.data, posval);
                if(!ok)
                {
                    mc_allocator_free(comp->pstate, posval);
                    goto error;
                }
            }
            opbuf[0] = pos;
            ip = mc_compiler_emit(comp, OPCODE_CONSTANT, 1, opbuf);
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
            ip = mc_compiler_emit(comp, expr->exprlitbool ? OPCODE_TRUE : OPCODE_FALSE, 0, NULL);
            if(ip < 0)
            {
                goto error;
            }
            break;
        }
        case MC_EXPR_ARRAYLITERAL:
        {
            int i;
            for(i = 0; i < mc_ptrarray_count(expr->exprlitarray); i++)
            {
                ok = mc_compiler_compileexpression(comp, (mcastexpression_t*)mc_ptrarray_get(expr->exprlitarray, i));
                if(!ok)
                {
                    goto error;
                }
            }
            opbuf[0] = mc_ptrarray_count(expr->exprlitarray);
            ip = mc_compiler_emit(comp, OPCODE_ARRAY, 1, opbuf);
            if(ip < 0)
            {
                goto error;
            }
            break;
        }
        case MC_EXPR_MAPLITERAL:
        {
            int i;
            map_literal_t* map = &expr->exprlitmap;
            int len = mc_ptrarray_count(map->keys);
            opbuf[0] = len;
            ip = mc_compiler_emit(comp, OPCODE_MAP_START, 1, opbuf);
            if(ip < 0)
            {
                goto error;
            }

            for(i = 0; i < len; i++)
            {
                mcastexpression_t* key = (mcastexpression_t*)mc_ptrarray_get(map->keys, i);
                mcastexpression_t* val = (mcastexpression_t*)mc_ptrarray_get(map->values, i);

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
            opbuf[0] = len;
            ip = mc_compiler_emit(comp, OPCODE_MAP_END, 1, opbuf);
            if(ip < 0)
            {
                goto error;
            }

            break;
        }
        case MC_EXPR_PREFIX:
        {
            ok = mc_compiler_compileexpression(comp, expr->exprprefix.right);
            if(!ok)
            {
                goto error;
            }

            mcinternopcode_t op = OPCODE_NONE;
            switch(expr->exprprefix.op)
            {
                case OPERATOR_MINUS:
                    op = OPCODE_MINUS;
                    break;
                case OPERATOR_BANG:
                    op = OPCODE_BANG;
                    break;
                default:
                {
                    mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "Unknown prefix operator.");
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
            ident_t* ident = expr->exprident;
            mcastsymbol_t* symbol = mc_symtable_resolve(symtab, ident->value);
            if(!symbol)
            {
                mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, ident->pos, "Symbol \"%s\" could not be resolved", ident->value);
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
            index_expression_t* index = &expr->exprindex;
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
            int i;
            fn_literal_t* fn = &expr->exprlitfunction;

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

            compscope = mc_compiler_getcompilationscope(comp);
            symtab = mc_compiler_getsymtable(comp);

            if(fn->name)
            {
                mcastsymbol_t* fnsymbol = mc_symtable_definefunctionname(symtab, fn->name, false);
                if(!fnsymbol)
                {
                    mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "Cannot define symbol \"%s\"", fn->name);
                    goto error;
                }
            }

            mcastsymbol_t* thissymbol = mc_symtable_definethis(symtab);
            if(!thissymbol)
            {
                mc_errlist_push(comp->errors, MC_ERROR_COMPILING, expr->pos, "Cannot define \"this\" symbol");
                goto error;
            }

            for(i = 0; i < mc_ptrarray_count(expr->exprlitfunction.params); i++)
            {
                ident_t* param = (ident_t*)mc_ptrarray_get(expr->exprlitfunction.params, i);
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

            mcptrarray_t* free_symbols = symtab->free_symbols;
            /* because it gets destroyed with compiler_pop_compilation_scope() */
            symtab->free_symbols = NULL;

            int num_locals = symtab->max_num_definitions;

            compilation_result_t* comp_res = compilation_scope_orphan_result(compscope);
            if(!comp_res)
            {
                mc_ptrarray_destroy(free_symbols, (mcptrarrayitemdestroyfn_t)mc_symbol_destroy);
                goto error;
            }
            mc_compiler_popsymtable(comp);
            mc_compiler_popcompilationscope(comp);
            compscope = mc_compiler_getcompilationscope(comp);
            symtab = mc_compiler_getsymtable(comp);

            mcvalue_t obj = object_make_function(comp->pstate, fn->name, comp_res, true, num_locals, mc_ptrarray_count(fn->params), 0);

            if(object_is_null(obj))
            {
                mc_ptrarray_destroy(free_symbols, (mcptrarrayitemdestroyfn_t)mc_symbol_destroy);
                compilation_result_destroy(comp_res);
                goto error;
            }

            for(i = 0; i < mc_ptrarray_count(free_symbols); i++)
            {
                mcastsymbol_t* symbol = (mcastsymbol_t*)mc_ptrarray_get(free_symbols, i);
                ok = mc_compiler_readsymbol(comp, symbol);
                if(!ok)
                {
                    mc_ptrarray_destroy(free_symbols, (mcptrarrayitemdestroyfn_t)mc_symbol_destroy);
                    goto error;
                }
            }

            int pos = mc_compiler_addconstant(comp, obj);
            if(pos < 0)
            {
                mc_ptrarray_destroy(free_symbols, (mcptrarrayitemdestroyfn_t)mc_symbol_destroy);
                goto error;
            }
            opbuf[0] = pos;
            opbuf[1] = mc_ptrarray_count(free_symbols);
            ip = mc_compiler_emit(comp, OPCODE_FUNCTION, 2, opbuf);
            if(ip < 0)
            {
                mc_ptrarray_destroy(free_symbols, (mcptrarrayitemdestroyfn_t)mc_symbol_destroy);
                goto error;
            }

            mc_ptrarray_destroy(free_symbols, (mcptrarrayitemdestroyfn_t)mc_symbol_destroy);

            break;
        }
        case MC_EXPR_CALL:
        {
            int i;
            ok = mc_compiler_compileexpression(comp, expr->exprcall.function);
            if(!ok)
            {
                goto error;
            }

            for(i = 0; i < mc_ptrarray_count(expr->exprcall.args); i++)
            {
                mcastexpression_t* argexpr = (mcastexpression_t*)mc_ptrarray_get(expr->exprcall.args, i);
                ok = mc_compiler_compileexpression(comp, argexpr);
                if(!ok)
                {
                    goto error;
                }
            }
            opbuf[0] = mc_ptrarray_count(expr->exprcall.args);
            ip = mc_compiler_emit(comp, OPCODE_CALL, 1, opbuf);
            if(ip < 0)
            {
                goto error;
            }

            break;
        }
        case MC_EXPR_ASSIGN:
        {
            assign_expression_t* assign = &expr->exprassign;
            if(assign->dest->type != MC_EXPR_IDENT && assign->dest->type != MC_EXPR_INDEX)
            {
                mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, assign->dest->pos, "Expression is not assignable.");
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

            ok = mc_basicarray_push(comp->src_positions_stack, &assign->dest->pos);
            if(!ok)
            {
                goto error;
            }

            if(assign->dest->type == MC_EXPR_IDENT)
            {
                ident_t* ident = assign->dest->exprident;
                mcastsymbol_t* symbol = mc_symtable_resolve(symtab, ident->value);
                if(!symbol)
                {
                    mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, assign->dest->pos, "Symbol \"%s\" could not be resolved", ident->value);
                    goto error;
                }
                if(!symbol->assignable)
                {
                    mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, assign->dest->pos, "Symbol \"%s\" is not assignable", ident->value);
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
                index_expression_t* index = &assign->dest->exprindex;
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

            mc_basicarray_pop(comp->src_positions_stack, NULL);
            break;
        }
        case MC_EXPR_LOGICAL:
            {
                int afterrightip;
                logical_expression_t* logi;
                logi = &expr->exprlogical;
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
                    opbuf[0] = 0xbeef;
                    afterleftjumpip = mc_compiler_emit(comp, OPCODE_JUMP_IF_FALSE, 1, opbuf);
                }
                else
                {
                    opbuf[0] = 0xbeef;
                    afterleftjumpip = mc_compiler_emit(comp, OPCODE_JUMP_IF_TRUE, 1, opbuf);
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
                afterrightip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, afterleftjumpip + 1, afterrightip);
            }
            break;

        case MC_EXPR_TERNARY:
            {
                int endip;
                int elseip;
                int endjumpip;
                int elsejumpip;
                ternary_expression_t* ternary;
                ternary = &expr->exprternary;
                ok = mc_compiler_compileexpression(comp, ternary->tercond);
                if(!ok)
                {
                    goto error;
                }
                opbuf[0] = 0xbeef;
                elsejumpip = mc_compiler_emit(comp, OPCODE_JUMP_IF_FALSE, 1, opbuf);
                ok = mc_compiler_compileexpression(comp, ternary->teriftrue);
                if(!ok)
                {
                    goto error;
                }
                opbuf[0] = 0xbeef;
                endjumpip = mc_compiler_emit(comp, OPCODE_JUMP, 1, opbuf);
                elseip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, elsejumpip + 1, elseip);
                ok = mc_compiler_compileexpression(comp, ternary->teriffalse);
                if(!ok)
                {
                    goto error;
                }
                endip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, endjumpip + 1, endip);
            }
            break;
        default:
            {
                MC_ASSERT(false);
            }
            break;
    }
    res = true;
    goto end;
error:
    res = false;
end:
    mc_basicarray_pop(comp->src_positions_stack, NULL);
    expression_destroy(exproptimized);
    return res;
}

TMPSEMISTATIC bool mc_compiler_compilecodeblock(mcastcompiler_t* comp, code_block_t* block)
{
    bool ok;
    int i;
    int ip;
    mcastsymtable_t* symtab;
    mcastexpression_t* expr;
    symtab = mc_compiler_getsymtable(comp);
    if(!symtab)
    {
        return false;
    }
    ok = mc_symtable_pushblockscope(symtab);
    if(!ok)
    {
        return false;
    }
    if(mc_ptrarray_count(block->statements) == 0)
    {
        ip = mc_compiler_emit(comp, OPCODE_NULL, 0, NULL);
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
    for(i = 0; i < mc_ptrarray_count(block->statements); i++)
    {
        expr = (mcastexpression_t*)mc_ptrarray_get(block->statements, i);
        ok = mc_compiler_compilestatement(comp, expr);
        if(!ok)
        {
            return false;
        }
    }
    mc_symtable_popblockscope(symtab);
    return true;
}

TMPSTATIC int mc_compiler_addconstant(mcastcompiler_t* comp, mcvalue_t obj)
{
    bool ok;
    ok = mc_basicarray_push(comp->constants, &obj);
    if(!ok)
    {
        return -1;
    }
    int pos = mc_basicarray_count(comp->constants) - 1;
    return pos;
}

TMPSTATIC void mc_compiler_changeuint16operand(mcastcompiler_t* comp, int ip, uint16_t operand)
{
    mcbasicarray_t* bytecode = mc_compiler_getbytecode(comp);
    if((ip + 1) >= mc_basicarray_count(bytecode))
    {
        MC_ASSERT(false);
        return;
    }
    uint8_t hi = operand >> 8;
    mc_basicarray_set(bytecode, ip, &hi);
    uint8_t lo = operand;
    mc_basicarray_set(bytecode, ip + 1, &lo);
}

TMPSTATIC bool mc_compiler_lastopcodeis(mcastcompiler_t* comp, mcinternopcode_t op)
{
    mcinternopcode_t last_opcode = mc_compiler_getlastopcode(comp);
    return last_opcode == op;
}

TMPSTATIC bool mc_compiler_readsymbol(mcastcompiler_t* comp, mcastsymbol_t* symbol)
{
    int ip = -1;
    uint64_t opbuf[10];
    if(symbol->type == SYMBOL_MODULE_GLOBAL)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, OPCODE_GET_MODULE_GLOBAL, 1, opbuf);
    }
    else if(symbol->type == SYMBOL_GLOBALBUILTIN)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, OPCODE_GETGLOBALBUILTIN, 1, opbuf);
    }
    else if(symbol->type == SYMBOL_LOCAL)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, OPCODE_GET_LOCAL, 1, opbuf);
    }
    else if(symbol->type == SYMBOL_FREE)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, OPCODE_GET_FREE, 1, opbuf);
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
    int ip;
    uint64_t opbuf[10];
    ip = -1;
    if(symbol->type == SYMBOL_MODULE_GLOBAL)
    {
        if(define)
        {
            opbuf[0] = symbol->index;
            ip = mc_compiler_emit(comp, OPCODE_DEFINE_MODULE_GLOBAL, 1, opbuf);
        }
        else
        {
            opbuf[0] = symbol->index;
            ip = mc_compiler_emit(comp, OPCODE_SET_MODULE_GLOBAL, 1, opbuf);
        }
    }
    else if(symbol->type == SYMBOL_LOCAL)
    {
        if(define)
        {
            opbuf[0] = symbol->index;
            ip = mc_compiler_emit(comp, OPCODE_DEFINE_LOCAL, 1, opbuf);
        }
        else
        {
            opbuf[0] = symbol->index;
            ip = mc_compiler_emit(comp, OPCODE_SET_LOCAL, 1, opbuf);
        }
    }
    else if(symbol->type == SYMBOL_FREE)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, OPCODE_SET_FREE, 1, opbuf);
    }
    return ip >= 0;
}

TMPSTATIC bool mc_compiler_pushbreakip(mcastcompiler_t* comp, int ip)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    return mc_basicarray_push(compscope->break_ip_stack, &ip);
}

TMPSTATIC void mc_compiler_popbreakip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    if(mc_basicarray_count(compscope->break_ip_stack) == 0)
    {
        MC_ASSERT(false);
        return;
    }
    mc_basicarray_pop(compscope->break_ip_stack, NULL);
}

TMPSTATIC int mc_compiler_getbreakip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    if(mc_basicarray_count(compscope->break_ip_stack) == 0)
    {
        return -1;
    }
    int* res = (int*)mc_basicarray_top(compscope->break_ip_stack);
    return *res;
}

TMPSTATIC bool mc_compiler_pushcontinueip(mcastcompiler_t* comp, int ip)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    return mc_basicarray_push(compscope->continue_ip_stack, &ip);
}

TMPSTATIC void mc_compiler_popcontinueip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    if(mc_basicarray_count(compscope->continue_ip_stack) == 0)
    {
        MC_ASSERT(false);
        return;
    }
    mc_basicarray_pop(compscope->continue_ip_stack, NULL);
}

TMPSTATIC int mc_compiler_getcontinueip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    if(mc_basicarray_count(compscope->continue_ip_stack) == 0)
    {
        MC_ASSERT(false);
        return -1;
    }
    int* res = (int*)mc_basicarray_top(compscope->continue_ip_stack);
    return *res;
}

TMPSTATIC int mc_compiler_getip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    return mc_basicarray_count(compscope->bytecode);
}

TMPSTATIC mcbasicarray_t* mc_compiler_getsrcpositions(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    return compscope->src_positions;
}

TMPSTATIC mcbasicarray_t* mc_compiler_getbytecode(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope = mc_compiler_getcompilationscope(comp);
    return compscope->bytecode;
}

TMPSTATIC mcastscopefile_t* mc_compiler_filescopemake(mcastcompiler_t* comp, mcastcompiledfile_t* file)
{
    mcastscopefile_t* filescope = (mcastscopefile_t*)mc_allocator_alloc(comp->pstate, sizeof(mcastscopefile_t));
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
    filescope->loaded_module_names = mc_ptrarray_make(comp->pstate);
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
    int i;
    void* name;
    for(i = 0; i < mc_ptrarray_count(scope->loaded_module_names); i++)
    {
        name = (void*)mc_ptrarray_get(scope->loaded_module_names, i);
        mc_allocator_free(scope->pstate, name);
    }
    mc_ptrarray_destroy(scope->loaded_module_names, NULL);
    parser_destroy(scope->parser);
    mc_allocator_free(scope->pstate, scope);
}

TMPSTATIC bool mc_compiler_filescopepush(mcastcompiler_t* comp, const char* filepath)
{
    bool ok;
    mcastsymtable_t* prevst = NULL;
    if(mc_ptrarray_count(comp->file_scopes) > 0)
    {
        prevst = mc_compiler_getsymtable(comp);
    }

    mcastcompiledfile_t* file = mc_compiledfile_make(comp->pstate, filepath);
    if(!file)
    {
        return false;
    }

    ok = mc_ptrarray_push(comp->files, file);
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

    ok = mc_ptrarray_push(comp->file_scopes, filescope);
    if(!ok)
    {
        mc_compiler_filescopedestroy(filescope);
        return false;
    }

    int globaloffset = 0;
    if(prevst)
    {
        mcastscopeblock_t* prevsttopscope = mc_symtable_getblockscope(prevst);
        globaloffset = prevsttopscope->offset + prevsttopscope->num_definitions;
    }

    ok = mc_compiler_pushsymtable(comp, globaloffset);
    if(!ok)
    {
        mc_ptrarray_pop(comp->file_scopes);
        mc_compiler_filescopedestroy(filescope);
        return false;
    }

    return true;
}

TMPSTATIC void mc_compiler_filescopepop(mcastcompiler_t* comp)
{
    mcastsymtable_t* poppedst = mc_compiler_getsymtable(comp);
    mcastscopeblock_t* poppedsttopscope = mc_symtable_getblockscope(poppedst);
    int poppednumdefs = poppedsttopscope->num_definitions;

    while(mc_compiler_getsymtable(comp))
    {
        mc_compiler_popsymtable(comp);
    }
    mcastscopefile_t* scope = (mcastscopefile_t*)mc_ptrarray_top(comp->file_scopes);
    if(!scope)
    {
        MC_ASSERT(false);
        return;
    }
    mc_compiler_filescopedestroy(scope);

    mc_ptrarray_pop(comp->file_scopes);

    if(mc_ptrarray_count(comp->file_scopes) > 0)
    {
        mcastsymtable_t* currentst = mc_compiler_getsymtable(comp);
        mcastscopeblock_t* currentsttopscope = mc_symtable_getblockscope(currentst);
        currentsttopscope->num_definitions += poppednumdefs;
    }
}

TMPSTATIC void mc_compiler_setcompilationscope(mcastcompiler_t* comp, mcastscopecomp_t* scope)
{
    comp->compilation_scope = scope;
}

TMPSTATIC module_t* mc_module_make(mcstate_t* state, const char* name)
{
    module_t* module = (module_t*)mc_allocator_alloc(state, sizeof(module_t));
    if(!module)
    {
        return NULL;
    }
    memset(module, 0, sizeof(module_t));
    module->pstate = state;
    module->name = mc_util_strdup(state, name);
    if(!module->name)
    {
        mc_module_destroy(module);
        return NULL;
    }
    module->symbols = mc_ptrarray_make(state);
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
    mc_allocator_free(module->pstate, module->name);
    mc_ptrarray_destroy(module->symbols, (mcptrarrayitemdestroyfn_t)mc_symbol_destroy);
    mc_allocator_free(module->pstate, module);
}

TMPSTATIC module_t* mc_module_copy(module_t* src)
{
    module_t* copy;
    copy = (module_t*)mc_allocator_alloc(src->pstate, sizeof(module_t));
    if(!copy)
    {
        return NULL;
    }
    memset(copy, 0, sizeof(module_t));
    copy->pstate = src->pstate;
    copy->name = mc_util_strdup(copy->pstate, src->name);
    if(!copy->name)
    {
        mc_module_destroy(copy);
        return NULL;
    }
    copy->symbols = mc_ptrarray_copy(src->symbols, (mcptrarrayitemcopyfn_t)mc_symbol_copy, (mcptrarrayitemdestroyfn_t)mc_symbol_destroy);
    if(!copy->symbols)
    {
        mc_module_destroy(copy);
        return NULL;
    }
    return copy;
}

TMPSTATIC const char* mc_util_getmodulename(const char* path)
{
    const char* lastslashpos;
    lastslashpos = strrchr(path, '/');
    if(lastslashpos)
    {
        return lastslashpos + 1;
    }
    return path;
}

TMPSTATIC bool mc_module_addsymbol(module_t* module, mcastsymbol_t* symbol)
{
    bool ok;
    mcastsymbol_t* modulesymbol;
    mcprintstate_t* namebuf;
    namebuf = mc_printer_make(module->pstate, NULL);
    if(!namebuf)
    {
        return false;
    }
    ok = mc_printer_printf(namebuf, "%s::%s", module->name, symbol->name);
    if(!ok)
    {
        mc_printer_destroy(namebuf);
        return false;
    }
    modulesymbol = mc_symbol_make(module->pstate, mc_printer_getstring(namebuf), SYMBOL_MODULE_GLOBAL, symbol->index, false);
    mc_printer_destroy(namebuf);
    if(!modulesymbol)
    {
        return false;
    }
    ok = mc_ptrarray_push(module->symbols, modulesymbol);
    if(!ok)
    {
        mc_symbol_destroy(modulesymbol);
        return false;
    }
    return true;
}

TMPSTATIC mcastsymbol_t* mc_compiler_defsymbol(mcastcompiler_t* comp, mcastlocation_t pos, const char* name, bool assignable, bool canshadow)
{
    mcastsymbol_t* symbol;
    mcastsymbol_t* currentsymbol;
    mcastsymtable_t* symtab;
    symtab = mc_compiler_getsymtable(comp);
    if(!canshadow && !mc_symtable_istopglobalscope(symtab))
    {
        currentsymbol = mc_symtable_resolve(symtab, name);
        if(currentsymbol)
        {
            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, pos, "Symbol \"%s\" is already defined", name);
            return NULL;
        }
    }
    symbol = mc_symtable_define(symtab, name, assignable);
    if(!symbol)
    {
        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, pos, "Cannot define symbol \"%s\"", name);
        return NULL;
    }
    return symbol;
}

TMPSTATIC mcastparser_t* parser_make(mcstate_t* state, mcconfig_t* config, mcerrlist_t* errors)
{
    mcastparser_t* parser;
    parser = (mcastparser_t*)mc_allocator_alloc(state, sizeof(mcastparser_t));
    if(!parser)
    {
        return NULL;
    }
    memset(parser, 0, sizeof(mcastparser_t));
    parser->pstate = state;
    parser->config = config;
    parser->errors = errors;
    {
        parser->rightassocfuncs[MC_TOK_IDENT] = mc_parser_parseident;
        parser->rightassocfuncs[MC_TOK_NUMBER] = mc_parser_parseliteralnumber;
        parser->rightassocfuncs[MC_TOK_TRUE] = mc_parser_parseliteralbool;
        parser->rightassocfuncs[MC_TOK_FALSE] = mc_parser_parseliteralbool;
        parser->rightassocfuncs[MC_TOK_STRING] = mc_parser_parseliteralstring;
        parser->rightassocfuncs[MC_TOK_TEMPLATESTRING] = mc_parser_parseliteraltemplatestring;
        parser->rightassocfuncs[MC_TOK_NULL] = mc_parser_parseliteralnull;
        parser->rightassocfuncs[MC_TOK_BANG] = mc_parser_parseprefixexpr;
        parser->rightassocfuncs[MC_TOK_MINUS] = mc_parser_parseprefixexpr;
        parser->rightassocfuncs[MC_TOK_LPAREN] = mc_parser_parsegroupedexpr;
        parser->rightassocfuncs[MC_TOK_FUNCTION] = mc_parser_parseliteralfunction;
        parser->rightassocfuncs[MC_TOK_LBRACKET] = mc_parser_parseliteralarray;
        parser->rightassocfuncs[MC_TOK_LBRACE] = mc_parser_parseliteralmap;
        parser->rightassocfuncs[MC_TOK_PLUSPLUS] = mc_parser_parseincdecprefixexpr;
        parser->rightassocfuncs[MC_TOK_MINUSMINUS] = mc_parser_parseincdecprefixexpr;
    }
    {
        parser->leftassocfuncs[MC_TOK_PLUS] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_MINUS] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_SLASH] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_ASTERISK] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_PERCENT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_EQ] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_NOTEQ] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_LT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_LTE] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_GT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_GTE] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_LPAREN] = parse_call_expression;
        parser->leftassocfuncs[MC_TOK_LBRACKET] = mc_parser_parseindexexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGN] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_PLUSASSIGN] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_MINUSASSIGN] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_SLASHASSIGN] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASTERISKASSIGN] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_PERCENTASSIGN] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_BITANDASSIGN] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_BITORASSIGN] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_BITXORASSIGN] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_LSHIFTASSIGN] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_RSHIFTASSIGN] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_DOT] = parse_dot_expression;
        parser->leftassocfuncs[MC_TOK_AND] = parse_logical_expression;
        parser->leftassocfuncs[MC_TOK_OR] = parse_logical_expression;
        parser->leftassocfuncs[MC_TOK_BITAND] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_BITOR] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_BITXOR] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_LSHIFT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_RSHIFT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_QUESTION] = parse_ternary_expression;
        parser->leftassocfuncs[MC_TOK_PLUSPLUS] = parse_incdec_postfix_expression;
        parser->leftassocfuncs[MC_TOK_MINUSMINUS] = parse_incdec_postfix_expression;
    }
    parser->depth = 0;
    return parser;
}

TMPSTATIC void parser_destroy(mcastparser_t* parser)
{
    if(!parser)
    {
        return;
    }
    mc_allocator_free(parser->pstate, parser);
}

TMPSTATIC mcptrarray_t* parser_parse_all(mcastparser_t* parser, const char* input, mcastcompiledfile_t* file)
{
    bool ok;
    mcastexpression_t* expr;
    mcptrarray_t* statements;
    parser->depth = 0;
    ok = mc_lexer_init(&parser->lexer, parser->pstate, parser->errors, input, file);
    if(!ok)
    {
        return NULL;
    }
    mc_lexer_nexttoken(&parser->lexer);
    mc_lexer_nexttoken(&parser->lexer);
    statements = mc_ptrarray_make(parser->pstate);
    if(!statements)
    {
        return NULL;
    }
    while(!mc_lexer_currtokenis(&parser->lexer, MC_TOK_EOF))
    {
        if(mc_lexer_currtokenis(&parser->lexer, MC_TOK_SEMICOLON))
        {
            mc_lexer_nexttoken(&parser->lexer);
            continue;
        }
        expr = parse_statement(parser);
        if(!expr)
        {
            goto err;
        }
        ok = mc_ptrarray_push(statements, expr);
        if(!ok)
        {
            statement_destroy(expr);
            goto err;
        }
    }
    if(mc_errlist_size(parser->errors) > 0)
    {
        goto err;
    }
    return statements;
err:
    mc_ptrarray_destroy(statements, (mcptrarrayitemdestroyfn_t)statement_destroy);
    return NULL;
}

TMPSTATIC mcastexpression_t* parse_statement(mcastparser_t* p)
{
    mcastlocation_t pos;
    mcastexpression_t* res;
    pos = p->lexer.currtoken.pos;
    res = NULL;
    switch(p->lexer.currtoken.type)
    {
        case MC_TOK_VAR:
        case MC_TOK_CONST:
            {
                res = parse_define_statement(p);
            }
            break;
        case MC_TOK_IF:
            {
                res = parse_if_statement(p);
            }
            break;
        case MC_TOK_RETURN:
            {
                res = parse_return_statement(p);
            }
            break;
        case MC_TOK_WHILE:
            {
                res = parse_while_loop_statement(p);
            }
            break;
        case MC_TOK_BREAK:
            {
                res = parse_break_statement(p);
            }
            break;
        case MC_TOK_FOR:
            {
                res = parse_for_loop_statement(p);
            }
            break;
        case MC_TOK_FUNCTION:
            {
                if(mc_lexer_peektokenis(&p->lexer, MC_TOK_IDENT))
                {
                    res = parse_function_statement(p);
                }
                else
                {
                    res = parse_expression_statement(p);
                }
            }
            break;
        case MC_TOK_LBRACE:
            {
                if(p->config->replmode && p->depth == 0)
                {
                    res = parse_expression_statement(p);
                }
                else
                {
                    res = parse_block_statement(p);
                }
            }
            break;
        case MC_TOK_CONTINUE:
            {
                res = parse_continue_statement(p);
            }
            break;
        case MC_TOK_IMPORT:
            {
                res = parse_import_statement(p);
            }
            break;
        case MC_TOK_RECOVER:
            {
                res = parse_recover_statement(p);
            }
            break;
        default:
            {
                res = parse_expression_statement(p);
            }
            break;
    }
    if(res)
    {
        res->pos = pos;
    }
    return res;
}

TMPSEMISTATIC mcastexpression_t* parse_define_statement(mcastparser_t* p)
{
    bool assignable;
    ident_t* nameident;
    mcastexpression_t* value;
    mcastexpression_t* res;
    nameident = NULL;
    value = NULL;
    assignable = mc_lexer_currtokenis(&p->lexer, MC_TOK_VAR);
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IDENT))
    {
        goto err;
    }
    nameident = ident_make(p->pstate, p->lexer.currtoken);
    if(!nameident)
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_ASSIGN))
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
        value->exprlitfunction.name = mc_util_strdup(p->pstate, nameident->value);
        if(!value->exprlitfunction.name)
        {
            goto err;
        }
    }
    res = statement_make_define(p->pstate, nameident, value, assignable);
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
    mcptrarray_t* cases;
    if_case_t* cond;
    if_case_t* elif;
    code_block_t* alternative;
    mcastexpression_t* res;
    cases = NULL;
    alternative = NULL;
    cases = mc_ptrarray_make(p->pstate);
    if(!cases)
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    cond = if_case_make(p->pstate, NULL, NULL);
    if(!cond)
    {
        goto err;
    }
    ok = mc_ptrarray_push(cases, cond);
    if(!ok)
    {
        if_case_destroy(cond);
        goto err;
    }
    cond->ifcond = parse_expression(p, PRECEDENCE_LOWEST);
    if(!cond->ifcond)
    {
        goto err;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    cond->consequence = parse_code_block(p);
    if(!cond->consequence)
    {
        goto err;
    }
    while(mc_lexer_currtokenis(&p->lexer, MC_TOK_ELSE))
    {
        mc_lexer_nexttoken(&p->lexer);
        if(mc_lexer_currtokenis(&p->lexer, MC_TOK_IF))
        {
            mc_lexer_nexttoken(&p->lexer);
            if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
            {
                goto err;
            }
            mc_lexer_nexttoken(&p->lexer);
            elif = if_case_make(p->pstate, NULL, NULL);
            if(!elif)
            {
                goto err;
            }
            ok = mc_ptrarray_push(cases, elif);
            if(!ok)
            {
                if_case_destroy(elif);
                goto err;
            }
            elif->ifcond = parse_expression(p, PRECEDENCE_LOWEST);
            if(!elif->ifcond)
            {
                goto err;
            }
            if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
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
    res = statement_make_if(p->pstate, cases, alternative);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_ptrarray_destroy(cases, (mcptrarrayitemdestroyfn_t)if_case_destroy);
    code_block_destroy(alternative);
    return NULL;
}

TMPSEMISTATIC mcastexpression_t* parse_return_statement(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* expr;
    expr = NULL;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_currtokenis(&p->lexer, MC_TOK_SEMICOLON) && !mc_lexer_currtokenis(&p->lexer, MC_TOK_RBRACE) && !mc_lexer_currtokenis(&p->lexer, MC_TOK_EOF))
    {
        expr = parse_expression(p, PRECEDENCE_LOWEST);
        if(!expr)
        {
            return NULL;
        }
    }
    res = statement_make_return(p->pstate, expr);
    if(!res)
    {
        expression_destroy(expr);
        return NULL;
    }
    return res;
}

TMPSEMISTATIC mcastexpression_t* parse_expression_statement(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* expr;
    expr = parse_expression(p, PRECEDENCE_LOWEST);
    if(!expr)
    {
        return NULL;
    }
    if(expr && (!p->config->replmode || p->depth > 0))
    {
        if(expr->type != MC_EXPR_ASSIGN && expr->type != MC_EXPR_CALL)
        {
            mc_errlist_addf(p->errors, MC_ERROR_PARSING, expr->pos, "Only assignments and function calls can be expression statements");
            expression_destroy(expr);
            return NULL;
        }
    }
    res = statement_make_expression(p->pstate, expr);
    if(!res)
    {
        expression_destroy(expr);
        return NULL;
    }
    return res;
}

TMPSEMISTATIC mcastexpression_t* parse_while_loop_statement(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* test;
    code_block_t* body;
    test = NULL;
    body = NULL;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    test = parse_expression(p, PRECEDENCE_LOWEST);
    if(!test)
    {
        goto err;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    body = parse_code_block(p);
    if(!body)
    {
        goto err;
    }
    res = statement_make_while_loop(p->pstate, test, body);
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
    code_block_t* block;
    mcastexpression_t* res;
    block = parse_code_block(p);
    if(!block)
    {
        return NULL;
    }
    res = statement_make_block(p->pstate, block);
    if(!res)
    {
        code_block_destroy(block);
        return NULL;
    }
    return res;
}

TMPSEMISTATIC mcastexpression_t* parse_import_statement(mcastparser_t* p)
{
    char* processedname;
    mcastexpression_t* res;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_STRING))
    {
        return NULL;
    }
    processedname = process_and_copy_string(p->pstate, p->lexer.currtoken.literal, p->lexer.currtoken.len);
    if(!processedname)
    {
        mc_errlist_push(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "Error when parsing module name");
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    res = statement_make_import(p->pstate, processedname);
    if(!res)
    {
        mc_allocator_free(p->pstate, processedname);
        return NULL;
    }
    return res;
}

TMPSEMISTATIC mcastexpression_t* parse_recover_statement(mcastparser_t* p)
{
    mcastexpression_t* res;
    ident_t* eid;
    code_block_t* body;
    eid = NULL;
    body = NULL;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IDENT))
    {
        return NULL;
    }
    eid = ident_make(p->pstate, p->lexer.currtoken);
    if(!eid)
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    body = parse_code_block(p);
    if(!body)
    {
        goto err;
    }
    res = statement_make_recover(p->pstate, eid, body);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    code_block_destroy(body);
    ident_destroy(eid);
    return NULL;
}

TMPSEMISTATIC mcastexpression_t* parse_for_loop_statement(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(mc_lexer_currtokenis(&p->lexer, MC_TOK_IDENT) && mc_lexer_peektokenis(&p->lexer, MC_TOK_IN))
    {
        return parse_foreach(p);
    }
    return parse_classic_for_loop(p);
}

TMPSTATIC mcastexpression_t* parse_foreach(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* source;
    code_block_t* body;
    ident_t* iteratorident;
    source = NULL;
    body = NULL;
    iteratorident = NULL;
    iteratorident = ident_make(p->pstate, p->lexer.currtoken);
    if(!iteratorident)
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    source = parse_expression(p, PRECEDENCE_LOWEST);
    if(!source)
    {
        goto err;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    body = parse_code_block(p);
    if(!body)
    {
        goto err;
    }
    res = statement_make_foreach(p->pstate, iteratorident, source, body);
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
    mcastexpression_t* res;
    mcastexpression_t* init;
    mcastexpression_t* test;
    mcastexpression_t* update;
    init = NULL;
    test = NULL;
    update = NULL;
    code_block_t* body = NULL;
    if(!mc_lexer_currtokenis(&p->lexer, MC_TOK_SEMICOLON))
    {
        init = parse_statement(p);
        if(!init)
        {
            goto err;
        }
        if(init->type != MC_EXPR_STMTDEFINE && init->type != MC_EXPR_STMTEXPRESSION)
        {
            mc_errlist_addf(p->errors, MC_ERROR_PARSING, init->pos, "for loop's init clause should be a define statement or an expression");
            goto err;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_SEMICOLON))
        {
            goto err;
        }
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_currtokenis(&p->lexer, MC_TOK_SEMICOLON))
    {
        test = parse_expression(p, PRECEDENCE_LOWEST);
        if(!test)
        {
            goto err;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_SEMICOLON))
        {
            goto err;
        }
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_currtokenis(&p->lexer, MC_TOK_RPAREN))
    {
        update = parse_expression(p, PRECEDENCE_LOWEST);
        if(!update)
        {
            goto err;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
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
    res = statement_make_for_loop(p->pstate, init, test, update, body);
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
    ident_t* nameident;
    mcastexpression_t* res;
    mcastexpression_t* value;
    mcastlocation_t pos;
    nameident = NULL;
    value = NULL;
    pos = p->lexer.currtoken.pos;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IDENT))
    {
        goto err;
    }
    nameident = ident_make(p->pstate, p->lexer.currtoken);
    if(!nameident)
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    value = mc_parser_parseliteralfunction(p);
    if(!value)
    {
        goto err;
    }
    value->pos = pos;
    value->exprlitfunction.name = mc_util_strdup(p->pstate, nameident->value);
    if(!value->exprlitfunction.name)
    {
        goto err;
    }
    res = statement_make_define(p->pstate, nameident, value, false);
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
    code_block_t* res;
    mcastexpression_t* expr;
    mcptrarray_t* statements;
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LBRACE))
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    p->depth++;
    statements = mc_ptrarray_make(p->pstate);
    if(!statements)
    {
        goto err;
    }
    while(!mc_lexer_currtokenis(&p->lexer, MC_TOK_RBRACE))
    {
        if(mc_lexer_currtokenis(&p->lexer, MC_TOK_EOF))
        {
            mc_errlist_push(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "Unexpected EOF");
            goto err;
        }
        if(mc_lexer_currtokenis(&p->lexer, MC_TOK_SEMICOLON))
        {
            mc_lexer_nexttoken(&p->lexer);
            continue;
        }
        expr = parse_statement(p);
        if(!expr)
        {
            goto err;
        }
        ok = mc_ptrarray_push(statements, expr);
        if(!ok)
        {
            statement_destroy(expr);
            goto err;
        }
    }
    mc_lexer_nexttoken(&p->lexer);
    p->depth--;
    res = code_block_make(p->pstate, statements);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    p->depth--;
    mc_ptrarray_destroy(statements, (mcptrarrayitemdestroyfn_t)statement_destroy);
    return NULL;
}

TMPSTATIC mcastexpression_t* parse_expression(mcastparser_t* p, mcastprecedence_t prec)
{
    char* literal;
    mcastlocation_t pos;
    mcleftassocparsefn_t parseleftassoc;
    mcastrightassocparsefn_t parserightassoc;
    mcastexpression_t* newleftexpr;
    mcastexpression_t* leftexpr;
    pos = p->lexer.currtoken.pos;
    if(p->lexer.currtoken.type == MC_TOK_INVALID)
    {
        mc_errlist_push(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "Illegal token");
        return NULL;
    }
    parserightassoc = p->rightassocfuncs[p->lexer.currtoken.type];
    if(!parserightassoc)
    {
        literal = token_duplicate_literal(p->pstate, &p->lexer.currtoken);
        mc_errlist_addf(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "No prefix parse function for \"%s\" found", literal);
        mc_allocator_free(p->pstate, literal);
        return NULL;
    }
    leftexpr = parserightassoc(p);
    if(!leftexpr)
    {
        return NULL;
    }
    leftexpr->pos = pos;
    while(!mc_lexer_currtokenis(&p->lexer, MC_TOK_SEMICOLON) && prec < get_precedence(p->lexer.currtoken.type))
    {
        parseleftassoc = p->leftassocfuncs[p->lexer.currtoken.type];
        if(!parseleftassoc)
        {
            return leftexpr;
        }
        pos = p->lexer.currtoken.pos;
        newleftexpr = parseleftassoc(p, leftexpr);
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

TMPSTATIC mcastexpression_t* mc_parser_parseident(mcastparser_t* p)
{
    ident_t* ident;
    mcastexpression_t* res;
    ident = ident_make(p->pstate, p->lexer.currtoken);
    if(!ident)
    {
        return NULL;
    }
    res = expression_make_ident(p->pstate, ident);
    if(!res)
    {
        ident_destroy(ident);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    return res;
}

TMPSTATIC mcastexpression_t* mc_parser_parseliteralnumber(mcastparser_t* p)
{
    double number;
    long parsedlen;
    char* end;
    char* literal;
    number = 0;
    errno = 0;
    number = strtod(p->lexer.currtoken.literal, &end);
    parsedlen = end - p->lexer.currtoken.literal;
    if(errno || parsedlen != p->lexer.currtoken.len)
    {
        literal = token_duplicate_literal(p->pstate, &p->lexer.currtoken);
        mc_errlist_addf(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "Parsing number literal \"%s\" failed", literal);
        mc_allocator_free(p->pstate, literal);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    return expression_make_number_literal(p->pstate, number);
}

TMPSTATIC mcastexpression_t* mc_parser_parseliteralbool(mcastparser_t* p)
{
    mcastexpression_t* res;
    res = expression_make_bool_literal(p->pstate, p->lexer.currtoken.type == MC_TOK_TRUE);
    mc_lexer_nexttoken(&p->lexer);
    return res;
}

TMPSTATIC mcastexpression_t* mc_parser_parseliteralstring(mcastparser_t* p)
{
    size_t len;
    char* processedliteral;
    mcastexpression_t* res;
    processedliteral = process_and_copy_string(p->pstate, p->lexer.currtoken.literal, p->lexer.currtoken.len);
    if(!processedliteral)
    {
        mc_errlist_push(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "Error when parsing string literal");
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    len = strlen(processedliteral);
    res = expression_make_string_literal(p->pstate, processedliteral, len);
    if(!res)
    {
        mc_allocator_free(p->pstate, processedliteral);
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* mc_parser_parseliteraltemplatestring(mcastparser_t* p)
{
    size_t len;
    char* processedliteral;
    mcastlocation_t pos;
    mcastexpression_t* leftstringexpr;
    mcastexpression_t* templateexpr;
    mcastexpression_t* tostrcallexpr;
    mcastexpression_t* leftaddexpr;
    mcastexpression_t* rightexpr;
    mcastexpression_t* rightaddexpr;
    processedliteral = NULL;
    leftstringexpr = NULL;
    templateexpr = NULL;
    tostrcallexpr = NULL;
    leftaddexpr = NULL;
    rightexpr = NULL;
    rightaddexpr = NULL;
    processedliteral = process_and_copy_string(p->pstate, p->lexer.currtoken.literal, p->lexer.currtoken.len);
    if(!processedliteral)
    {
        mc_errlist_push(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "Error when parsing string literal");
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LBRACE))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    pos = p->lexer.currtoken.pos;
    len = strlen(processedliteral);
    leftstringexpr = expression_make_string_literal(p->pstate, processedliteral, len);
    if(!leftstringexpr)
    {
        goto err;
    }
    leftstringexpr->pos = pos;
    processedliteral = NULL;
    pos = p->lexer.currtoken.pos;
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
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RBRACE))
    {
        goto err;
    }
    mc_lexer_previoustoken(&p->lexer);
    mc_lexer_conttplstring(&p->lexer);
    mc_lexer_nexttoken(&p->lexer);
    mc_lexer_nexttoken(&p->lexer);
    pos = p->lexer.currtoken.pos;
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
    mc_allocator_free(p->pstate, processedliteral);
    return NULL;
}

TMPSTATIC mcastexpression_t* mc_parser_parseliteralnull(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    return expression_make_null_literal(p->pstate);
}

TMPSTATIC mcastexpression_t* mc_parser_parseliteralarray(mcastparser_t* p)
{
    mcptrarray_t* array;
    mcastexpression_t* res;
    array = parse_expression_list(p, MC_TOK_LBRACKET, MC_TOK_RBRACKET, true);
    if(!array)
    {
        return NULL;
    }
    res = expression_make_array_literal(p->pstate, array);
    if(!res)
    {
        mc_ptrarray_destroy(array, (mcptrarrayitemdestroyfn_t)expression_destroy);
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* mc_parser_parseliteralmap(mcastparser_t* p)
{
    bool ok;
    size_t len;
    char* str;
    mcptrarray_t* keys;
    mcptrarray_t* values;
    mcastexpression_t* res;
    mcastexpression_t* key;
    mcastexpression_t* value;
    keys = mc_ptrarray_make(p->pstate);
    values = mc_ptrarray_make(p->pstate);
    if(!keys || !values)
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    while(!mc_lexer_currtokenis(&p->lexer, MC_TOK_RBRACE))
    {
        key = NULL;
        if(mc_lexer_currtokenis(&p->lexer, MC_TOK_IDENT))
        {
            str = token_duplicate_literal(p->pstate, &p->lexer.currtoken);
            len = strlen(str);
            key = expression_make_string_literal(p->pstate, str, len);
            if(!key)
            {
                mc_allocator_free(p->pstate, str);
                goto err;
            }
            key->pos = p->lexer.currtoken.pos;
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
                    mc_errlist_addf(p->errors, MC_ERROR_PARSING, key->pos, "Invalid map literal key type");
                    expression_destroy(key);
                    goto err;
                }
            }
        }
        ok = mc_ptrarray_push(keys, key);
        if(!ok)
        {
            expression_destroy(key);
            goto err;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_COLON))
        {
            goto err;
        }
        mc_lexer_nexttoken(&p->lexer);
        value = parse_expression(p, PRECEDENCE_LOWEST);
        if(!value)
        {
            goto err;
        }
        ok = mc_ptrarray_push(values, value);
        if(!ok)
        {
            expression_destroy(value);
            goto err;
        }
        if(mc_lexer_currtokenis(&p->lexer, MC_TOK_RBRACE))
        {
            break;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_COMMA))
        {
            goto err;
        }
        mc_lexer_nexttoken(&p->lexer);
    }
    mc_lexer_nexttoken(&p->lexer);
    res = expression_make_map_literal(p->pstate, keys, values);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_ptrarray_destroy(keys, (mcptrarrayitemdestroyfn_t)expression_destroy);
    mc_ptrarray_destroy(values, (mcptrarrayitemdestroyfn_t)expression_destroy);
    return NULL;
}

TMPSTATIC mcastexpression_t* mc_parser_parseprefixexpr(mcastparser_t* p)
{
    mcastmathoptype_t op;
    mcastexpression_t* res;
    mcastexpression_t* right;
    op = token_to_operator(p->lexer.currtoken.type);
    mc_lexer_nexttoken(&p->lexer);
    right = parse_expression(p, PRECEDENCE_PREFIX);
    if(!right)
    {
        return NULL;
    }
    res = expression_make_prefix(p->pstate, op, right);
    if(!res)
    {
        expression_destroy(right);
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* mc_parser_parseinfixexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastmathoptype_t op;
    mcastprecedence_t prec;
    mcastexpression_t* res;
    mcastexpression_t* right;
    op = token_to_operator(p->lexer.currtoken.type);
    prec = get_precedence(p->lexer.currtoken.type);
    mc_lexer_nexttoken(&p->lexer);
    right = parse_expression(p, prec);
    if(!right)
    {
        return NULL;
    }
    res = expression_make_infix(p->pstate, op, left, right);
    if(!res)
    {
        expression_destroy(right);
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* mc_parser_parsegroupedexpr(mcastparser_t* p)
{
    mcastexpression_t* expr;
    mc_lexer_nexttoken(&p->lexer);
    expr = parse_expression(p, PRECEDENCE_LOWEST);
    if(!expr || !mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        expression_destroy(expr);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    return expr;
}

TMPSTATIC mcastexpression_t* mc_parser_parseliteralfunction(mcastparser_t* p)
{
    bool ok;
    mcptrarray_t* params;
    code_block_t* body;
    mcastexpression_t* res;
    p->depth++;
    params = NULL;
    body = NULL;
    if(mc_lexer_currtokenis(&p->lexer, MC_TOK_FUNCTION))
    {
        mc_lexer_nexttoken(&p->lexer);
    }
    params = mc_ptrarray_make(p->pstate);
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
    res = expression_make_fn_literal(p->pstate, params, body);
    if(!res)
    {
        goto err;
    }
    p->depth -= 1;
    return res;
err:
    code_block_destroy(body);
    mc_ptrarray_destroy(params, (mcptrarrayitemdestroyfn_t)ident_destroy);
    p->depth -= 1;
    return NULL;
}

TMPSTATIC bool parse_function_parameters(mcastparser_t* p, mcptrarray_t* outparams)
{
    bool ok;
    ident_t* ident;
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
    {
        return false;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(mc_lexer_currtokenis(&p->lexer, MC_TOK_RPAREN))
    {
        mc_lexer_nexttoken(&p->lexer);
        return true;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IDENT))
    {
        return false;
    }
    ident = ident_make(p->pstate, p->lexer.currtoken);
    if(!ident)
    {
        return false;
    }
    ok = mc_ptrarray_push(outparams, ident);
    if(!ok)
    {
        ident_destroy(ident);
        return false;
    }
    mc_lexer_nexttoken(&p->lexer);
    while(mc_lexer_currtokenis(&p->lexer, MC_TOK_COMMA))
    {
        mc_lexer_nexttoken(&p->lexer);
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IDENT))
        {
            return false;
        }
        ident = ident_make(p->pstate, p->lexer.currtoken);
        if(!ident)
        {
            return false;
        }
        ok = mc_ptrarray_push(outparams, ident);
        if(!ok)
        {
            ident_destroy(ident);
            return false;
        }
        mc_lexer_nexttoken(&p->lexer);
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        return false;
    }
    mc_lexer_nexttoken(&p->lexer);
    return true;
}

TMPSTATIC mcastexpression_t* parse_call_expression(mcastparser_t* p, mcastexpression_t* left)
{
    mcptrarray_t* args;
    mcastexpression_t* res;
    mcastexpression_t* function;
    function = left;
    args = parse_expression_list(p, MC_TOK_LPAREN, MC_TOK_RPAREN, false);
    if(!args)
    {
        return NULL;
    }
    res = expression_make_call(p->pstate, function, args);
    if(!res)
    {
        mc_ptrarray_destroy(args, (mcptrarrayitemdestroyfn_t)expression_destroy);
        return NULL;
    }
    return res;
}

TMPSTATIC mcptrarray_t* parse_expression_list(mcastparser_t* p, mcasttoktype_t starttoken, mcasttoktype_t endtoken, bool trailingcommaallowed)
{
    bool ok;
    mcptrarray_t* res;
    mcastexpression_t* argexpr;
    if(!mc_lexer_expectcurrent(&p->lexer, starttoken))
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    res = mc_ptrarray_make(p->pstate);
    if(mc_lexer_currtokenis(&p->lexer, endtoken))
    {
        mc_lexer_nexttoken(&p->lexer);
        return res;
    }
    argexpr = parse_expression(p, PRECEDENCE_LOWEST);
    if(!argexpr)
    {
        goto err;
    }
    ok = mc_ptrarray_push(res, argexpr);
    if(!ok)
    {
        expression_destroy(argexpr);
        goto err;
    }
    while(mc_lexer_currtokenis(&p->lexer, MC_TOK_COMMA))
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
        ok = mc_ptrarray_push(res, argexpr);
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
    mc_ptrarray_destroy(res, (mcptrarrayitemdestroyfn_t)expression_destroy);
    return NULL;
}

TMPSTATIC mcastexpression_t* mc_parser_parseindexexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastexpression_t* res;
    mcastexpression_t* index;
    mc_lexer_nexttoken(&p->lexer);
    index = parse_expression(p, PRECEDENCE_LOWEST);
    if(!index)
    {
        return NULL;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RBRACKET))
    {
        expression_destroy(index);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    res = expression_make_index(p->pstate, left, index);
    if(!res)
    {
        expression_destroy(index);
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* mc_parser_parseassignexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastlocation_t pos;
    mcastmathoptype_t op;
    mcasttoktype_t assigntype;
    mcastexpression_t* res;
    mcastexpression_t* source;
    mcastexpression_t* leftcopy;
    mcastexpression_t* newsource;
    source = NULL;
    assigntype = p->lexer.currtoken.type;
    mc_lexer_nexttoken(&p->lexer);
    source = parse_expression(p, PRECEDENCE_LOWEST);
    if(!source)
    {
        goto err;
    }
    switch(assigntype)
    {
        case MC_TOK_PLUSASSIGN:
        case MC_TOK_MINUSASSIGN:
        case MC_TOK_SLASHASSIGN:
        case MC_TOK_ASTERISKASSIGN:
        case MC_TOK_PERCENTASSIGN:
        case MC_TOK_BITANDASSIGN:
        case MC_TOK_BITORASSIGN:
        case MC_TOK_BITXORASSIGN:
        case MC_TOK_LSHIFTASSIGN:
        case MC_TOK_RSHIFTASSIGN:
            {
                op = token_to_operator(assigntype);
                leftcopy = expression_copy(left);
                if(!leftcopy)
                {
                    goto err;
                }
                pos = source->pos;
                newsource = expression_make_infix(p->pstate, op, leftcopy, source);
                if(!newsource)
                {
                    expression_destroy(leftcopy);
                    goto err;
                }
                newsource->pos = pos;
                source = newsource;
            }
            break;
        case MC_TOK_ASSIGN:
            {
            }
            break;
        default:
            {
                MC_ASSERT(false);
            }
            break;
    }
    res = expression_make_assign(p->pstate, left, source, false);
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
    mcastmathoptype_t op;
    mcastprecedence_t prec;
    mcastexpression_t* res;
    mcastexpression_t* right;
    op = token_to_operator(p->lexer.currtoken.type);
    prec = get_precedence(p->lexer.currtoken.type);
    mc_lexer_nexttoken(&p->lexer);
    right = parse_expression(p, prec);
    if(!right)
    {
        return NULL;
    }
    res = expression_make_logical(p->pstate, op, left, right);
    if(!res)
    {
        expression_destroy(right);
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* parse_ternary_expression(mcastparser_t* p, mcastexpression_t* left)
{
    mcastexpression_t* res;
    mcastexpression_t* ift;
    mcastexpression_t* iffalse;
    mc_lexer_nexttoken(&p->lexer);
    ift = parse_expression(p, PRECEDENCE_LOWEST);
    if(!ift)
    {
        return NULL;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_COLON))
    {
        expression_destroy(ift);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    iffalse = parse_expression(p, PRECEDENCE_LOWEST);
    if(!iffalse)
    {
        expression_destroy(ift);
        return NULL;
    }
    res = expression_make_ternary(p->pstate, left, ift, iffalse);
    if(!res)
    {
        expression_destroy(ift);
        expression_destroy(iffalse);
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* mc_parser_parseincdecprefixexpr(mcastparser_t* p)
{
    mcastlocation_t pos;
    mcastmathoptype_t op;
    mcasttoktype_t operationtype;
    mcastexpression_t* res;
    mcastexpression_t* dest;
    mcastexpression_t* source;
    mcastexpression_t* destcopy;
    mcastexpression_t* operation;
    mcastexpression_t* oneliteral;
    source = NULL;
    operationtype = p->lexer.currtoken.type;
    pos = p->lexer.currtoken.pos;
    mc_lexer_nexttoken(&p->lexer);
    op = token_to_operator(operationtype);
    dest = parse_expression(p, PRECEDENCE_PREFIX);
    if(!dest)
    {
        goto err;
    }
    oneliteral = expression_make_number_literal(p->pstate, 1);
    if(!oneliteral)
    {
        expression_destroy(dest);
        goto err;
    }
    oneliteral->pos = pos;
    destcopy = expression_copy(dest);
    if(!destcopy)
    {
        expression_destroy(oneliteral);
        expression_destroy(dest);
        goto err;
    }
    operation = expression_make_infix(p->pstate, op, destcopy, oneliteral);
    if(!operation)
    {
        expression_destroy(destcopy);
        expression_destroy(dest);
        expression_destroy(oneliteral);
        goto err;
    }
    operation->pos = pos;
    res = expression_make_assign(p->pstate, dest, operation, false);
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
    mcastlocation_t pos;
    mcastmathoptype_t op;
    mcasttoktype_t operationtype;
    mcastexpression_t* res;
    mcastexpression_t* source;
    mcastexpression_t* leftcopy;
    mcastexpression_t* operation;
    mcastexpression_t* oneliteral;
    source = NULL;
    operationtype = p->lexer.currtoken.type;
    pos = p->lexer.currtoken.pos;
    mc_lexer_nexttoken(&p->lexer);
    op = token_to_operator(operationtype);
    leftcopy = expression_copy(left);
    if(!leftcopy)
    {
        goto err;
    }
    oneliteral = expression_make_number_literal(p->pstate, 1);
    if(!oneliteral)
    {
        expression_destroy(leftcopy);
        goto err;
    }
    oneliteral->pos = pos;
    operation = expression_make_infix(p->pstate, op, leftcopy, oneliteral);
    if(!operation)
    {
        expression_destroy(oneliteral);
        expression_destroy(leftcopy);
        goto err;
    }
    operation->pos = pos;
    res = expression_make_assign(p->pstate, left, operation, true);
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
    size_t len;
    char* str;
    mcastexpression_t* res;
    mcastexpression_t* index;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IDENT))
    {
        return NULL;
    }
    str = token_duplicate_literal(p->pstate, &p->lexer.currtoken);
    len = strlen(str);
    index = expression_make_string_literal(p->pstate, str, len);
    if(!index)
    {
        mc_allocator_free(p->pstate, str);
        return NULL;
    }
    index->pos = p->lexer.currtoken.pos;
    mc_lexer_nexttoken(&p->lexer);
    res = expression_make_index(p->pstate, left, index);
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
        case MC_TOK_EQ:
        case MC_TOK_NOTEQ:
            return PRECEDENCE_EQUALS;
        case MC_TOK_LT:
        case MC_TOK_LTE:
        case MC_TOK_GT:
        case MC_TOK_GTE:
            return PRECEDENCE_LESSGREATER;
        case MC_TOK_PLUS:
        case MC_TOK_MINUS:
            return PRECEDENCE_SUM;
        case MC_TOK_SLASH:
        case MC_TOK_ASTERISK:
        case MC_TOK_PERCENT:
            return PRECEDENCE_PRODUCT;
        case MC_TOK_LPAREN:
        case MC_TOK_LBRACKET:
            return PRECEDENCE_POSTFIX;
        case MC_TOK_ASSIGN:
        case MC_TOK_PLUSASSIGN:
        case MC_TOK_MINUSASSIGN:
        case MC_TOK_ASTERISKASSIGN:
        case MC_TOK_SLASHASSIGN:
        case MC_TOK_PERCENTASSIGN:
        case MC_TOK_BITANDASSIGN:
        case MC_TOK_BITORASSIGN:
        case MC_TOK_BITXORASSIGN:
        case MC_TOK_LSHIFTASSIGN:
        case MC_TOK_RSHIFTASSIGN:
            return PRECEDENCE_ASSIGN;
        case MC_TOK_DOT:
            return PRECEDENCE_POSTFIX;
        case MC_TOK_AND:
            return PRECEDENCE_LOGICAL_AND;
        case MC_TOK_OR:
            return PRECEDENCE_LOGICAL_OR;
        case MC_TOK_BITOR:
            return PRECEDENCE_BIT_OR;
        case MC_TOK_BITXOR:
            return PRECEDENCE_BIT_XOR;
        case MC_TOK_BITAND:
            return PRECEDENCE_BIT_AND;
        case MC_TOK_LSHIFT:
        case MC_TOK_RSHIFT:
            return PRECEDENCE_SHIFT;
        case MC_TOK_QUESTION:
            return PRECEDENCE_TERNARY;
        case MC_TOK_PLUSPLUS:
        case MC_TOK_MINUSMINUS:
            return PRECEDENCE_INCDEC;
        default:
            return PRECEDENCE_LOWEST;
    }
}

TMPSTATIC mcastmathoptype_t token_to_operator(mcasttoktype_t tk)
{
    switch(tk)
    {
        case MC_TOK_ASSIGN:
            return OPERATOR_ASSIGN;
        case MC_TOK_PLUS:
            return OPERATOR_PLUS;
        case MC_TOK_MINUS:
            return OPERATOR_MINUS;
        case MC_TOK_BANG:
            return OPERATOR_BANG;
        case MC_TOK_ASTERISK:
            return OPERATOR_ASTERISK;
        case MC_TOK_SLASH:
            return OPERATOR_SLASH;
        case MC_TOK_LT:
            return OPERATOR_LT;
        case MC_TOK_LTE:
            return OPERATOR_LTE;
        case MC_TOK_GT:
            return OPERATOR_GT;
        case MC_TOK_GTE:
            return OPERATOR_GTE;
        case MC_TOK_EQ:
            return OPERATOR_EQ;
        case MC_TOK_NOTEQ:
            return OPERATOR_NOT_EQ;
        case MC_TOK_PERCENT:
            return OPERATOR_MODULUS;
        case MC_TOK_AND:
            return OPERATOR_LOGICAL_AND;
        case MC_TOK_OR:
            return OPERATOR_LOGICAL_OR;
        case MC_TOK_PLUSASSIGN:
            return OPERATOR_PLUS;
        case MC_TOK_MINUSASSIGN:
            return OPERATOR_MINUS;
        case MC_TOK_ASTERISKASSIGN:
            return OPERATOR_ASTERISK;
        case MC_TOK_SLASHASSIGN:
            return OPERATOR_SLASH;
        case MC_TOK_PERCENTASSIGN:
            return OPERATOR_MODULUS;
        case MC_TOK_BITANDASSIGN:
            return OPERATOR_BIT_AND;
        case MC_TOK_BITORASSIGN:
            return OPERATOR_BIT_OR;
        case MC_TOK_BITXORASSIGN:
            return OPERATOR_BIT_XOR;
        case MC_TOK_LSHIFTASSIGN:
            return OPERATOR_LSHIFT;
        case MC_TOK_RSHIFTASSIGN:
            return OPERATOR_RSHIFT;
        case MC_TOK_BITAND:
            return OPERATOR_BIT_AND;
        case MC_TOK_BITOR:
            return OPERATOR_BIT_OR;
        case MC_TOK_BITXOR:
            return OPERATOR_BIT_XOR;
        case MC_TOK_LSHIFT:
            return OPERATOR_LSHIFT;
        case MC_TOK_RSHIFT:
            return OPERATOR_RSHIFT;
        case MC_TOK_PLUSPLUS:
            return OPERATOR_PLUS;
        case MC_TOK_MINUSMINUS:
            return OPERATOR_MINUS;
        default:
            {
                MC_ASSERT(false);
            }
            break;
    }
    return OPERATOR_NONE;

}

TMPSTATIC char escape_char(char c)
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
            break;
    }
    return c;
}

TMPSTATIC char* process_and_copy_string(mcstate_t* state, const char* input, size_t len)
{
    size_t ini;
    size_t outi;
    char* output;
    output = (char*)mc_allocator_alloc(state, len + 1);
    if(!output)
    {
        return NULL;
    }
    ini = 0;
    outi = 0;
    while(input[ini] != '\0' && ini < len)
    {
        if(input[ini] == '\\')
        {
            ini++;
            output[outi] = escape_char(input[ini]);
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
    mc_allocator_free(state, output);
    return NULL;
}

TMPSTATIC mcastexpression_t* wrap_expression_in_function_call(mcstate_t* state, mcastexpression_t* expr, const char* function_name)
{
    bool ok;
    mcasttoken_t fntoken;
    ident_t* ident;
    mcptrarray_t* args;
    mcastexpression_t* ce;
    mcastexpression_t* functionidentexpr;
    token_init(&fntoken, MC_TOK_IDENT, function_name, strlen(function_name));
    fntoken.pos = expr->pos;
    ident = ident_make(state, fntoken);
    if(!ident)
    {
        return NULL;
    }
    ident->pos = fntoken.pos;
    functionidentexpr = expression_make_ident(state, ident);
    if(!functionidentexpr)
    {
        ident_destroy(ident);
        return NULL;
    }
    functionidentexpr->pos = expr->pos;
    ident = NULL;
    args = mc_ptrarray_make(state);
    if(!args)
    {
        expression_destroy(functionidentexpr);
        return NULL;
    }
    ok = mc_ptrarray_push(args, expr);
    if(!ok)
    {
        mc_ptrarray_destroy(args, NULL);
        expression_destroy(functionidentexpr);
        return NULL;
    }
    ce = expression_make_call(state, functionidentexpr, args);
    if(!ce)
    {
        mc_ptrarray_destroy(args, NULL);
        expression_destroy(functionidentexpr);
        return NULL;
    }
    ce->pos = expr->pos;
    return ce;
}

TMPSEMISTATIC mcastexpression_t* mc_optimizer_optexpression(mcastexpression_t* expr)
{
    switch(expr->type)
    {
        case MC_EXPR_INFIX:
            return mc_optimizer_optinfixexpr(expr);
        case MC_EXPR_PREFIX:
            return mc_optimizer_optprefixexpr(expr);
        default:
            break;
    }
    return NULL;
}

TMPSTATIC mcastexpression_t* mc_optimizer_optinfixexpr(mcastexpression_t* expr)
{
    bool leftisnumeric;
    bool rightisnumeric;
    bool leftisstring;
    bool rightisstring;
    double dnleft;
    double dnright;
    size_t len;
    mcstate_t* state;
    mcastexpression_t* res;
    mcastexpression_t* left;
    mcastexpression_t* right;
    mcastexpression_t* leftoptimized;
    mcastexpression_t* rightoptimized;
    state = expr->pstate;
    left = expr->exprinfix.left;
    leftoptimized = mc_optimizer_optexpression(left);
    if(leftoptimized)
    {
        left = leftoptimized;
    }
    right = expr->exprinfix.right;
    rightoptimized = mc_optimizer_optexpression(right);
    if(rightoptimized)
    {
        right = rightoptimized;
    }
    res = NULL;
    leftisnumeric = left->type == MC_EXPR_NUMBERLITERAL || left->type == MC_EXPR_BOOLLITERAL;
    rightisnumeric = right->type == MC_EXPR_NUMBERLITERAL || right->type == MC_EXPR_BOOLLITERAL;
    leftisstring = left->type == MC_EXPR_STRINGLITERAL;
    rightisstring = right->type == MC_EXPR_STRINGLITERAL;
    if(leftisnumeric && rightisnumeric)
    {
        dnleft = left->type == MC_EXPR_NUMBERLITERAL ? left->exprlitnumber : left->exprlitbool;
        dnright = right->type == MC_EXPR_NUMBERLITERAL ? right->exprlitnumber : right->exprlitbool;
        switch(expr->exprinfix.op)
        {
            case OPERATOR_PLUS:
            {
                res = expression_make_number_literal(state, mc_mathutil_add(dnleft, dnright));
                break;
            }
            case OPERATOR_MINUS:
            {
                res = expression_make_number_literal(state, mc_mathutil_sub(dnleft, dnright));
                break;
            }
            case OPERATOR_ASTERISK:
            {
                res = expression_make_number_literal(state, mc_mathutil_mult(dnleft, dnright));
                break;
            }
            case OPERATOR_SLASH:
            {
                res = expression_make_number_literal(state, mc_mathutil_div(dnleft, dnright));
                break;
            }
            case OPERATOR_LT:
            {
                res = expression_make_bool_literal(state, dnleft < dnright);
                break;
            }
            case OPERATOR_LTE:
            {
                res = expression_make_bool_literal(state, dnleft <= dnright);
                break;
            }
            case OPERATOR_GT:
            {
                res = expression_make_bool_literal(state, dnleft > dnright);
                break;
            }
            case OPERATOR_GTE:
            {
                res = expression_make_bool_literal(state, dnleft >= dnright);
                break;
            }
            case OPERATOR_EQ:
            {
                res = expression_make_bool_literal(state, MC_UTIL_CMPFLOAT(dnleft, dnright));
                break;
            }
            case OPERATOR_NOT_EQ:
            {
                res = expression_make_bool_literal(state, !MC_UTIL_CMPFLOAT(dnleft, dnright));
                break;
            }
            case OPERATOR_MODULUS:
            {
                res = expression_make_number_literal(state, mc_mathutil_mod(dnleft, dnright));
                break;
            }
            case OPERATOR_BIT_AND:
            {
                res = expression_make_number_literal(state, mc_mathutil_binand(dnleft, dnright));
                break;
            }
            case OPERATOR_BIT_OR:
            {
                res = expression_make_number_literal(state, mc_mathutil_binor(dnleft, dnright));
                break;
            }
            case OPERATOR_BIT_XOR:
            {
                res = expression_make_number_literal(state, mc_mathutil_binxor(dnleft, dnright));
                break;
            }
            case OPERATOR_LSHIFT:
            {
                res = expression_make_number_literal(state, mc_mathutil_binshiftleft(dnleft, dnright));
                break;
            }
            case OPERATOR_RSHIFT:
            {
                res = expression_make_number_literal(state, mc_mathutil_binshiftright(dnleft, dnright));
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else if(expr->exprinfix.op == OPERATOR_PLUS && leftisstring && rightisstring)
    {
        /* TODO:FIXME: horrible method of joining strings!!!!!!! */
        char* resstr;
        const char* strleft;
        const char* strright;
        strleft = left->exprlitstring.data;
        strright = right->exprlitstring.data;
        resstr = mc_util_stringallocfmt(state, "%s%s", strleft, strright);
        len = strlen(resstr);
        if(resstr)
        {
            res = expression_make_string_literal(state, resstr, len);
            if(!res)
            {
                mc_allocator_free(state, resstr);
            }
        }
    }
    expression_destroy(leftoptimized);
    expression_destroy(rightoptimized);
    if(res)
    {
        res->pos = expr->pos;
    }
    return res;
}

TMPSTATIC mcastexpression_t* mc_optimizer_optprefixexpr(mcastexpression_t* expr)
{
    mcastexpression_t* res;
    mcastexpression_t* right;
    mcastexpression_t* rightoptimized;
    right = expr->exprprefix.right;
    rightoptimized = mc_optimizer_optexpression(right);
    if(rightoptimized)
    {
        right = rightoptimized;
    }
    res = NULL;
    if(expr->exprprefix.op == OPERATOR_MINUS && right->type == MC_EXPR_NUMBERLITERAL)
    {
        res = expression_make_number_literal(expr->pstate, -right->exprlitnumber);
    }
    else if(expr->exprprefix.op == OPERATOR_BANG && right->type == MC_EXPR_BOOLLITERAL)
    {
        res = expression_make_bool_literal(expr->pstate, !right->exprlitbool);
    }
    expression_destroy(rightoptimized);
    if(res)
    {
        res->pos = expr->pos;
    }
    return res;
}

TMPSTATIC mcstate_t* mc_state_make(void* ctx)
{
    mcstate_t* state;
    (void)ctx;
    state = (mcstate_t*)malloc(sizeof(mcstate_t));
    if(!state)
    {
        return NULL;
    }
    memset(state, 0, sizeof(mcstate_t));
    set_default_config(state);
    mc_errlist_init(&state->errors);
    state->mem = gcmem_make(state);
    if(!state->mem)
    {
        goto err;
    }
    state->files = mc_ptrarray_make(state);
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
    mc_state_deinit(state);
    free(state);
    return NULL;
}

TMPSTATIC void mc_state_destroy(mcstate_t* state)
{
    if(!state)
    {
        return;
    }
    mc_state_deinit(state);
    mc_allocator_free(state, state);
}

TMPSTATIC void mc_state_freeallocated(mcstate_t* state, void* ptr)
{
    mc_allocator_free(state, ptr);
}

TMPSTATIC void mc_state_setreplmode(mcstate_t* state, bool enabled)
{
    state->config.replmode = enabled;
}

TMPSTATIC bool mc_state_settimeout(mcstate_t* state, double maxexecms)
{
    if(!mc_util_istimerplatformsupported())
    {
        state->config.maxexecutiontime = 0;
        state->config.havemaxexectime = false;
        return false;
    }
    if(maxexecms >= 0)
    {
        state->config.maxexecutiontime = maxexecms;
        state->config.havemaxexectime = true;
    }
    else
    {
        state->config.maxexecutiontime = 0;
        state->config.havemaxexectime = false;
    }
    return true;
}

TMPSTATIC void mc_state_setstdoutwritefunction(mcstate_t* state, mcstdoutwritefn_t stdoutwrite, void* context)
{
    state->config.stdio.fnprintto = stdoutwrite;
    state->config.stdio.context = context;
}

TMPSTATIC void mc_state_setfilewritefunction(mcstate_t* state, mcwritefilefn_t filewrite, void* context)
{
    state->config.fileio.fnwritefile = filewrite;
    state->config.fileio.context = context;
}

TMPSTATIC void mc_state_setfilereadfunction(mcstate_t* state, mcreadfilefn_t fileread, void* context)
{
    state->config.fileio.fnreadfile = fileread;
    state->config.fileio.context = context;
}

TMPSTATIC mccompiledprogram_t* mc_state_compilesource(mcstate_t* state, const char* code)
{
    mccompiledprogram_t* program;
    compilation_result_t* compres;
    mc_state_clearerrors(state);
    compres = mc_compiler_compilesource(state->compiler, code);
    if(!compres || mc_errlist_size(&state->errors) > 0)
    {
        goto err;
    }
    program = (mccompiledprogram_t*)mc_allocator_alloc(state, sizeof(mccompiledprogram_t));
    if(!program)
    {
        goto err;
    }
    program->pstate = state;
    program->comp_res = compres;
    return program;
err:
    compilation_result_destroy(compres);
    return NULL;
}

TMPSTATIC mcvalue_t mc_program_execute(mcstate_t* state, mccompiledprogram_t* program)
{
    bool ok;
    mcvalue_t res;
    if(program == NULL)
    {
        mc_errlist_push(&state->errors, MC_ERROR_USER, srcposinvalid, "program passed to execute was null.");
        return mc_value_makenull();
    }
    reset_state(state);
    if(state != program->pstate)
    {
        mc_errlist_push(&state->errors, MC_ERROR_USER, srcposinvalid, "program was compiled with an incompatible instance");
        return mc_value_makenull();
    }
    ok = mc_vm_runexecfunc(state, program->comp_res, mc_compiler_getconstants(state->compiler));
    if(!ok || mc_errlist_size(&state->errors) > 0)
    {
        return mc_value_makenull();
    }
    MC_ASSERT(state->sp == 0);
    res = vm_get_last_popped(state);
    if(res.type == MC_OBJ_NONE)
    {
        return mc_value_makenull();
    }
    return res;
}

TMPSTATIC void mc_program_destroy(mccompiledprogram_t* program)
{
    if(!program)
    {
        return;
    }
    compilation_result_destroy(program->comp_res);
    mc_allocator_free(program->pstate, program);
}

TMPUNUSED mcvalue_t mc_state_execcode(mcstate_t* state, const char* code)
{
    bool ok;
    mcvalue_t res;
    compilation_result_t* compres;
    reset_state(state);
    compres = mc_compiler_compilesource(state->compiler, code);
    if(!compres || mc_errlist_size(&state->errors) > 0)
    {
        goto err;
    }
    ok = mc_vm_runexecfunc(state, compres, mc_compiler_getconstants(state->compiler));
    if(!ok || mc_errlist_size(&state->errors) > 0)
    {
        goto err;
    }
    MC_ASSERT(state->sp == 0);
    res = vm_get_last_popped(state);
    if(res.type == MC_OBJ_NONE)
    {
        goto err;
    }
    compilation_result_destroy(compres);
    return res;
err:
    compilation_result_destroy(compres);
    return mc_value_makenull();
}


TMPUNUSED mcvalue_t mc_state_callfunctionbyname(mcstate_t* state, const char* function_name, int argc, mcvalue_t* args)
{
    mcvalue_t res;
    mcvalue_t callee;
    reset_state(state);
    callee = mc_state_getglobalobjectbyname(state, function_name);
    if(callee.type == MC_OBJ_NULL)
    {
        return mc_value_makenull();
    }
    res = vm_call(state, mc_compiler_getconstants(state->compiler), callee, argc, (mcvalue_t*)args);
    if(mc_errlist_size(&state->errors) > 0)
    {
        return mc_value_makenull();
    }
    return res;
}

TMPSTATIC bool mc_state_haserrors(mcstate_t* state)
{
    return mc_state_errorcount(state) > 0;
}

TMPSTATIC int mc_state_errorcount(mcstate_t* state)
{
    return mc_errlist_size(&state->errors);
}

TMPSTATIC void mc_state_clearerrors(mcstate_t* state)
{
    mc_errlist_clear(&state->errors);
}

TMPSTATIC mcerror_t* mc_state_geterror(mcstate_t* state, int index)
{
    return (mcerror_t*)mc_errlist_get(&state->errors, index);
}

TMPSTATIC bool mc_state_setnativefunction(mcstate_t* state, const char* name, mcnativefn_t fn, void* data)
{
    mcvalue_t obj;
    obj = mc_object_makenamednatfunctionbox(state, name, fn, data);
    if(object_is_null(obj))
    {
        return false;
    }
    return mc_state_setglobalconstant(state, name, obj);
}

TMPSTATIC bool mc_state_setglobalconstant(mcstate_t* state, const char* name, mcvalue_t obj)
{
    return global_store_set(state->global_store, name, obj);
}

TMPSTATIC mcvalue_t mc_state_getglobalobjectbyname(mcstate_t* state, const char* name)
{
    bool ok;
    mcvalue_t res;
    mcastsymbol_t* symbol;
    mcastsymtable_t* st;
    st = mc_compiler_getsymtable(state->compiler);
    symbol = mc_symtable_resolve(st, name);
    if(!symbol)
    {
        mc_errlist_addf(&state->errors, MC_ERROR_USER, srcposinvalid, "Symbol \"%s\" is not defined", name);
        return mc_value_makenull();
    }
    res = mc_value_makenull();
    if(symbol->type == SYMBOL_MODULE_GLOBAL)
    {
        res = vm_get_global(state, symbol->index);
    }
    else if(symbol->type == SYMBOL_GLOBALBUILTIN)
    {
        ok = false;
        res = global_store_get_object_at(state->global_store, symbol->index, &ok);
        if(!ok)
        {
            mc_errlist_addf(&state->errors, MC_ERROR_USER, srcposinvalid, "Failed to get global object at %d", symbol->index);
            return mc_value_makenull();
        }
    }
    else
    {
        mc_errlist_addf(&state->errors, MC_ERROR_USER, srcposinvalid, "Value associated with symbol \"%s\" could not be loaded", name);
        return mc_value_makenull();
    }
    return res;
}

TMPUNUSED mcvalue_t mc_state_makeerrorf(mcstate_t* state, const char* fmt, ...)
{
    int towrite;
    int written;
    char* res;
    va_list args;
    va_start(args, fmt);
    towrite = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    va_start(args, fmt);
    res = (char*)mc_allocator_alloc(state, towrite + 1);
    if(!res)
    {
        return mc_value_makenull();
    }
    written = vsprintf(res, fmt, args);
    (void)written;
    MC_ASSERT(written == towrite);
    va_end(args);
    return object_make_error_no_copy(state, res);
}

TMPSEMISTATIC void mc_state_setruntimeerrorf(mcstate_t* state, const char* fmt, ...)
{
    int written;
    int towrite;
    va_list args;
    char res[MC_CONF_ERROR_MSGMAXLENGTH];
    (void)towrite;
    (void)written;
    va_start(args, fmt);
    towrite = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    va_start(args, fmt);
    written = vsnprintf(res, MC_CONF_ERROR_MSGMAXLENGTH, fmt, args);
    MC_ASSERT(towrite == written);
    va_end(args);
    mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, res);
}

TMPSTATIC mcobjtype_t object_get_type(mcvalue_t v)
{
    return (v).type;
}

TMPSTATIC bool object_is_numeric(mcvalue_t obj)
{
    mcobjtype_t type;
    type = object_get_type(obj);
    return type == MC_OBJ_NUMBER || type == MC_OBJ_BOOL;
}

TMPSTATIC bool object_is_null(mcvalue_t obj)
{
    return object_get_type(obj) == MC_OBJ_NULL;
}

TMPSTATIC bool object_is_callable(mcvalue_t obj)
{
    mcobjtype_t type;
    type = object_get_type(obj);
    return type == MC_OBJ_NATIVE_FUNCTION || type == MC_OBJ_FUNCTION;
}

TMPSTATIC const char* mc_util_objtypename(mcobjtype_t type)
{
    switch(type)
    {
        case MC_OBJ_NONE:
            return "NONE";
        case MC_OBJ_ERROR:
            return "ERROR";
        case MC_OBJ_NUMBER:
            return "NUMBER";
        case MC_OBJ_BOOL:
            return "BOOL";
        case MC_OBJ_STRING:
            return "STRING";
        case MC_OBJ_NULL:
            return "NULL";
        case MC_OBJ_NATIVE_FUNCTION:
            return "NATIVE_FUNCTION";
        case MC_OBJ_ARRAY:
            return "ARRAY";
        case MC_OBJ_MAP:
            return "MAP";
        case MC_OBJ_FUNCTION:
            return "FUNCTION";
        case MC_OBJ_EXTERNAL:
            return "EXTERNAL";
        case MC_OBJ_FREED:
            return "FREED";
        case MC_OBJ_ANY:
            return "ANY";
        default:
            break;
    }
    return "NONE";
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
    o = mc_value_makeempty(MC_OBJ_NUMBER);
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
    o = mc_value_makeempty(MC_OBJ_BOOL);
    o.valbool = val;
    /*
    o.objdatahandle = MC_CONF_OBJECT_HDRBOOL | val;
    */
    return o;
}

TMPSTATIC mcvalue_t mc_value_makenull()
{
    mcvalue_t o;
    o = mc_value_makeempty(MC_OBJ_NULL);
    /*
    o.objdatahandle = MC_CONF_OBJECT_NULLPATTERN;
    */
    return o;
}

TMPSTATIC mcvalue_t mc_value_makestrcapacity(mcstate_t* state, int capacity)
{
    bool ok;
    mcobjdata_t* data;
    data = gcmem_get_object_data_from_pool(state, MC_OBJ_STRING);
    if(!data)
    {
        data = gcmem_alloc_object_data(state);
        if(!data)
        {
            return mc_value_makenull();
        }
        data->valstring.capacity = MC_CONF_OBJECT_STRING_BUF_SIZE - 1;
        data->valstring.is_allocated = false;
        data->valstring.data = data->valstring.actualonstack;
    }
    data->valstring.length = 0;
    data->valstring.hash = 0;
    if(capacity > data->valstring.capacity)
    {
        ok = mc_value_stringreservecapacity(data, capacity);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return mc_object_makedatafrom(MC_OBJ_STRING, data);
}

TMPUNUSED mcvalue_t mc_value_makestrformat(mcstate_t* state, const char* fmt, ...)
{
    int towrite;
    int written;
    char* resbuf;
    va_list args;
    mcvalue_t res;
    (void)written;
    va_start(args, fmt);
    towrite = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    va_start(args, fmt);
    res = mc_value_makestrcapacity(state, towrite);
    if(object_is_null(res))
    {
        return mc_value_makenull();
    }
    resbuf = mc_value_stringgetmutabledata(res);
    written = vsprintf(resbuf, fmt, args);
    MC_ASSERT(written == towrite);
    va_end(args);
    object_set_string_length(res, towrite);
    return res;
}

TMPSTATIC mcvalue_t mc_value_makestringlen(mcstate_t* state, const char* string, size_t len)
{
    bool ok;
    mcvalue_t res;
    res = mc_value_makestrcapacity(state, len);
    if(object_is_null(res))
    {
        return res;
    }
    ok = mc_value_stringappend(res, string, len);
    if(!ok)
    {
        return mc_value_makenull();
    }
    return res;
}

TMPSTATIC mcvalue_t mc_value_makestring(mcstate_t* state, const char* string)
{
    return mc_value_makestringlen(state, string, strlen(string));
}

TMPSTATIC mcvalue_t object_make_native_function(mcstate_t* state, const char* name, mcnativeactualfn_t fn, void* data, int data_len)
{
    mcobjdata_t* obj;
    if(data_len > NATIVE_FN_MAX_DATA_LEN)
    {
        return mc_value_makenull();
    }
    obj = gcmem_alloc_object_data(state);
    if(!obj)
    {
        return mc_value_makenull();
    }
    obj->native_function.name = mc_util_strdup(state, name);
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
    return mc_object_makedatafrom(MC_OBJ_NATIVE_FUNCTION, obj);
}

TMPSTATIC mcvalue_t object_make_array(mcstate_t* state)
{
    return object_make_array_with_capacity(state, 8);
}

TMPSTATIC mcvalue_t object_make_array_with_capacity(mcstate_t* state, unsigned capacity)
{
    mcobjdata_t* data;
    data = gcmem_get_object_data_from_pool(state, MC_OBJ_ARRAY);
    if(data)
    {
        mc_basicarray_clear(data->array);
        return mc_object_makedatafrom(MC_OBJ_ARRAY, data);
    }
    data = gcmem_alloc_object_data(state);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->array = mc_basicarray_makecapacity(state, capacity, sizeof(mcvalue_t));
    if(!data->array)
    {
        return mc_value_makenull();
    }
    return mc_object_makedatafrom(MC_OBJ_ARRAY, data);
}

TMPSTATIC mcvalue_t object_make_map(mcstate_t* state)
{
    return object_make_map_with_capacity(state, 32);
}

TMPSTATIC mcvalue_t object_make_map_with_capacity(mcstate_t* state, unsigned capacity)
{
    mcobjdata_t* data;
    data = gcmem_get_object_data_from_pool(state, MC_OBJ_MAP);
    if(data)
    {
        mc_valdict_clear(data->map);
        return mc_object_makedatafrom(MC_OBJ_MAP, data);
    }
    data = gcmem_alloc_object_data(state);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->map = mc_valdict_makecapacity(state, capacity, sizeof(mcvalue_t), sizeof(mcvalue_t));
    if(!data->map)
    {
        return mc_value_makenull();
    }
    mc_valdict_sethashfunction(data->map, (mccollectionshashfn_t)object_hash);
    mc_valdict_setequalsfunction(data->map, (mccollectionsequalsfn_t)object_equals_wrapped);
    return mc_object_makedatafrom(MC_OBJ_MAP, data);
}

TMPSTATIC mcvalue_t object_make_error(mcstate_t* state, const char* error)
{
    char* errorstr;
    mcvalue_t res;
    errorstr = mc_util_strdup(state, error);
    if(!errorstr)
    {
        return mc_value_makenull();
    }
    res = object_make_error_no_copy(state, errorstr);
    if(object_is_null(res))
    {
        mc_allocator_free(state, errorstr);
        return mc_value_makenull();
    }
    return res;
}

TMPSTATIC mcvalue_t object_make_error_no_copy(mcstate_t* state, char* error)
{
    mcobjdata_t* data;
    data = gcmem_alloc_object_data(state);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->error.message = error;
    data->error.traceback = NULL;
    return mc_object_makedatafrom(MC_OBJ_ERROR, data);
}

TMPUNUSED mcvalue_t object_make_errorf(mcstate_t* vm, const char* fmt, ...)
{
    int towrite;
    int written;
    char* res;
    va_list args;
    mcvalue_t resobj;
    (void)written;
    va_start(args, fmt);
    towrite = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    va_start(args, fmt);
    res = (char*)mc_allocator_alloc(vm, towrite + 1);
    if(!res)
    {
        return mc_value_makenull();
    }
    written = vsprintf(res, fmt, args);
    MC_ASSERT(written == towrite);
    va_end(args);
    resobj = object_make_error_no_copy(vm, res);
    if(object_is_null(resobj))
    {
        mc_allocator_free(vm, res);
        return mc_value_makenull();
    }
    return resobj;
}

TMPSTATIC mcvalue_t object_make_function(mcstate_t* vm, const char* name, compilation_result_t* cres, bool ownsdt, int nlocals, int num_args, int fvc)
{
    mcobjdata_t* data = gcmem_alloc_object_data(vm);
    if(!data)
    {
        return mc_value_makenull();
    }
    if(ownsdt)
    {
        data->function.name = name ? mc_util_strdup(vm, name) : mc_util_strdup(vm, "anonymous");
        if(!data->function.name)
        {
            return mc_value_makenull();
        }
    }
    else
    {
        data->function.const_name = name ? name : "anonymous";
    }
    data->function.comp_result = cres;
    data->function.owns_data = ownsdt;
    data->function.num_locals = nlocals;
    data->function.num_args = num_args;
    if(fvc >= MC_UTIL_STATICARRAYSIZE(data->function.free_vals_buf))
    {
        data->function.free_vals_allocated = (mcvalue_t*)mc_allocator_alloc(vm, sizeof(mcvalue_t) * fvc);
        if(!data->function.free_vals_allocated)
        {
            return mc_value_makenull();
        }
    }
    data->function.free_vals_count = fvc;
    return mc_object_makedatafrom(MC_OBJ_FUNCTION, data);
}

TMPSTATIC mcvalue_t object_make_external(mcstate_t* vm, void* data)
{
    mcobjdata_t* obj;
    obj = gcmem_alloc_object_data(vm);
    if(!obj)
    {
        return mc_value_makenull();
    }
    obj->external.data = data;
    obj->external.data_destroy_fn = NULL;
    obj->external.data_copy_fn = NULL;
    return mc_object_makedatafrom(MC_OBJ_EXTERNAL, obj);
}

TMPUNUSED void object_deinit(mcvalue_t obj)
{
    mcobjdata_t* data;
    if(object_is_allocated(obj))
    {
        data = object_get_allocated_data(obj);
        object_data_deinit(data);
    }
}

TMPSTATIC void object_data_deinit(mcobjdata_t* data)
{
    switch(data->type)
    {
        case MC_OBJ_FREED:
            {
                MC_ASSERT(false);
                return;
            }
            break;
        case MC_OBJ_STRING:
            {
                if(data->valstring.is_allocated)
                {
                    mc_allocator_free(data->pstate, data->valstring.actualallocated);
                }
            }
            break;

        case MC_OBJ_FUNCTION:
            {
                if(data->function.owns_data)
                {
                    mc_allocator_free(data->pstate, data->function.name);
                    compilation_result_destroy(data->function.comp_result);
                }
                if(freevals_are_allocated(&data->function))
                {
                    mc_allocator_free(data->pstate, data->function.free_vals_allocated);
                }
            }
            break;
        case MC_OBJ_ARRAY:
            {
                mc_basicarray_destroy(data->array);
            }
            break;
        case MC_OBJ_MAP:
            {
                mc_valdict_destroy(data->map);
            }
            break;
        case MC_OBJ_NATIVE_FUNCTION:
            {
                mc_allocator_free(data->pstate, data->native_function.name);
            }
            break;
        case MC_OBJ_EXTERNAL:
            {
                if(data->external.data_destroy_fn)
                {
                    data->external.data_destroy_fn(data->external.data);
                }
            }
            break;
        case MC_OBJ_ERROR:
            {
                mc_allocator_free(data->pstate, data->error.message);
                traceback_destroy(data->error.traceback);
            }
            break;
        default:
            {
            }
            break;
    }
    data->type = MC_OBJ_FREED;
}

TMPSTATIC bool object_is_allocated(mcvalue_t object)
{
    return object.isallocated;
}

TMPUNUSED mcgcmemory_t* object_get_mem(mcvalue_t obj)
{
    mcobjdata_t* data = object_get_allocated_data(obj);
    return data->mem;
}

TMPSTATIC const char* mc_error_getmessage(mcerror_t* error)
{
    return error->message;
}

TMPSTATIC const char* mc_error_getfilepath(mcerror_t* error)
{
    if(!error->pos.file)
    {
        return NULL;
    }
    return error->pos.file->path;
}

TMPSTATIC const char* mc_error_getsourcelinecode(mcerror_t* error)
{
    if(!error->pos.file)
    {
        return NULL;
    }
    mcptrarray_t* lines = error->pos.file->lines;
    if(error->pos.line >= mc_ptrarray_count(lines))
    {
        return NULL;
    }
    const char* line = (const char*)mc_ptrarray_get(lines, error->pos.line);
    return line;
}

TMPSTATIC int mc_error_getsourcelinenumber(mcerror_t* error)
{
    if(error->pos.line < 0)
    {
        return -1;
    }
    return error->pos.line + 1;
}

TMPSTATIC int mc_error_getsourcecolumn(mcerror_t* error)
{
    if(error->pos.column < 0)
    {
        return -1;
    }
    return error->pos.column + 1;
}

TMPSTATIC mcerrtype_t mc_error_gettype(mcerror_t* error)
{
    switch(error->type)
    {
        case MC_ERROR_NONE:
            return MC_ERROR_NONE;
        case MC_ERROR_PARSING:
            return MC_ERROR_PARSING;
        case MC_ERROR_COMPILING:
            return MC_ERROR_COMPILING;
        case MC_ERROR_RUNTIME:
            return MC_ERROR_RUNTIME;
        case MC_ERROR_TIMEOUT:
            return MC_ERROR_TIMEOUT;
        case MC_ERROR_MEMORY:
            return MC_ERROR_MEMORY;
        case MC_ERROR_USER:
            return MC_ERROR_USER;
        default:
            return MC_ERROR_NONE;
    }
}

TMPSTATIC const char* mc_error_gettypestring(mcerror_t* error)
{
    return mc_util_errortypename(mc_error_gettype(error));
}

TMPSTATIC const char* mc_util_errortypename(mcerrtype_t type)
{
    switch(type)
    {
        case MC_ERROR_PARSING:
            return "PARSING";
        case MC_ERROR_COMPILING:
            return "COMPILATION";
        case MC_ERROR_RUNTIME:
            return "RUNTIME";
        case MC_ERROR_TIMEOUT:
            return "TIMEOUT";
        case MC_ERROR_MEMORY:
            return "ALLOCATION";
        case MC_ERROR_USER:
            return "USER";
        default:
            return "NONE";
    }
}

TMPSTATIC char* mc_error_serializetostring(mcstate_t* state, mcerror_t* err)
{
    int j;
    const char* typestr = mc_error_gettypestring(err);
    const char* filename = mc_error_getfilepath(err);
    const char* line = mc_error_getsourcelinecode(err);
    int linenum = mc_error_getsourcelinenumber(err);
    int colnum = mc_error_getsourcecolumn(err);
    mcprintstate_t* buf = mc_printer_make(state, NULL);
    if(!buf)
    {
        return NULL;
    }
    if(line)
    {
        mc_printer_puts(buf, line);
        mc_printer_puts(buf, "\n");
        if(colnum >= 0)
        {
            for(j = 0; j < (colnum - 1); j++)
            {
                mc_printer_puts(buf, " ");
            }
            mc_printer_puts(buf, "^\n");
        }
    }
    mc_printer_printf(buf, "%s ERROR in \"%s\" on %d:%d: %s\n", typestr, filename, linenum, colnum, mc_error_getmessage(err));
    mctraceback_t* traceback = mc_error_gettraceback(err);
    if(traceback)
    {
        mc_printer_printf(buf, "Traceback:\n");
        traceback_to_string((mctraceback_t*)mc_error_gettraceback(err), buf);
    }
    if(mc_printer_failed(buf))
    {
        mc_printer_destroy(buf);
        return NULL;
    }
    return mc_printer_get_string_and_destroy(buf);
}

TMPSTATIC mctraceback_t* mc_error_gettraceback(mcerror_t* error)
{
    return (mctraceback_t*)error->traceback;
}

TMPUNUSED int mc_traceback_getdepth(mctraceback_t* traceback)
{
    return mc_basicarray_count(traceback->items);
}

TMPUNUSED const char* mc_traceback_getfilepath(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item = (mctraceitem_t*)mc_basicarray_get(traceback->items, depth);
    if(!item)
    {
        return NULL;
    }
    return traceback_item_get_filepath(item);
}

TMPUNUSED const char* mc_traceback_getsourcelinecode(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item = (mctraceitem_t*)mc_basicarray_get(traceback->items, depth);
    if(!item)
    {
        return NULL;
    }
    return traceback_item_get_line(item);
}

TMPUNUSED int mc_traceback_getsourcelinenumber(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item = (mctraceitem_t*)mc_basicarray_get(traceback->items, depth);
    if(!item)
    {
        return -1;
    }
    return item->pos.line;
}

TMPUNUSED int mc_traceback_getsourcecolumn(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item = (mctraceitem_t*)mc_basicarray_get(traceback->items, depth);
    if(!item)
    {
        return -1;
    }
    return item->pos.column;
}

TMPUNUSED const char* mc_traceback_getfunctionname(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item = (mctraceitem_t*)mc_basicarray_get(traceback->items, depth);
    if(!item)
    {
        return "";
    }
    return item->function_name;
}

TMPSTATIC void mc_state_deinit(mcstate_t* state)
{
    mc_compiler_destroy(state->compiler);
    global_store_destroy(state->global_store);
    gcmem_destroy(state->mem);
    mc_ptrarray_destroy(state->files, (mcptrarrayitemdestroyfn_t)mc_compiledfile_destroy);
    mc_errlist_deinit(&state->errors);
    vm_destroy(state);
}

TMPSTATIC mcvalue_t mc_natfuncbox_callerfn(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)state;
    mcnatfnbox_t* wrapper = (mcnatfnbox_t*)data;
    MC_ASSERT(state == wrapper->pstate);
    mcvalue_t res = wrapper->fn(wrapper->pstate, wrapper->data, argc, (mcvalue_t*)args);
    if(mc_state_haserrors(wrapper->pstate))
    {
        return mc_value_makenull();
    }
    return res;
}

TMPSTATIC mcvalue_t mc_object_makenamednatfunctionbox(mcstate_t* state, const char* name, mcnativefn_t fn, void* data)
{
    mcnatfnbox_t wrapper;
    memset(&wrapper, 0, sizeof(mcnatfnbox_t));
    wrapper.fn = fn;
    wrapper.pstate = state;
    wrapper.data = data;
    mcvalue_t wrappernativefunction = object_make_native_function(state, name, mc_natfuncbox_callerfn, &wrapper, sizeof(wrapper));
    if(object_is_null(wrappernativefunction))
    {
        return mc_value_makenull();
    }
    return wrappernativefunction;
}

TMPSTATIC void reset_state(mcstate_t* state)
{
    mc_state_clearerrors(state);
    vm_reset(state);
}

TMPSTATIC void set_default_config(mcstate_t* state)
{
    memset(&state->config, 0, sizeof(mcconfig_t));
    mc_state_setreplmode(state, false);
    mc_state_settimeout(state, -1);
    mc_state_setfilereadfunction(state, read_file_default, state);
    mc_state_setfilewritefunction(state, write_file_default, state);
    mc_state_setstdoutwritefunction(state, stdout_write_default, state);
}

TMPSTATIC char* read_file_default(void* ctx, const char* filename)
{
    mcstate_t* state = (mcstate_t*)ctx;
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
    filecontents = (char*)mc_allocator_alloc(state, sizeof(char) * (sizetoread + 1));
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

TMPSTATIC mcastexpression_t* expression_make_ident(mcstate_t* state, ident_t* ident)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_IDENT);
    if(!res)
    {
        return NULL;
    }
    res->exprident = ident;
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_number_literal(mcstate_t* state, double val)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_NUMBERLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->exprlitnumber = val;
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_bool_literal(mcstate_t* state, bool val)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_BOOLLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->exprlitbool = val;
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_string_literal(mcstate_t* state, char* value, size_t len)
{
    mcastexpression_t* res;
    res = expression_make(state, MC_EXPR_STRINGLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->exprlitstring.data = value;
    res->exprlitstring.length = len;
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_null_literal(mcstate_t* state)
{
    mcastexpression_t* res;
    res = expression_make(state, MC_EXPR_NULLLITERAL);
    if(!res)
    {
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_array_literal(mcstate_t* state, mcptrarray_t* values)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_ARRAYLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->exprlitarray = values;
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_map_literal(mcstate_t* state, mcptrarray_t* keys, mcptrarray_t* values)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_MAPLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->exprlitmap.keys = keys;
    res->exprlitmap.values = values;
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_prefix(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* right)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_PREFIX);
    if(!res)
    {
        return NULL;
    }
    res->exprprefix.op = op;
    res->exprprefix.right = right;
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_infix(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* left, mcastexpression_t* right)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_INFIX);
    if(!res)
    {
        return NULL;
    }
    res->exprinfix.op = op;
    res->exprinfix.left = left;
    res->exprinfix.right = right;
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_fn_literal(mcstate_t* state, mcptrarray_t* params, code_block_t* body)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_FUNCTIONLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->exprlitfunction.name = NULL;
    res->exprlitfunction.params = params;
    res->exprlitfunction.body = body;
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_call(mcstate_t* state, mcastexpression_t* function, mcptrarray_t* args)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_CALL);
    if(!res)
    {
        return NULL;
    }
    res->exprcall.function = function;
    res->exprcall.args = args;
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_index(mcstate_t* state, mcastexpression_t* left, mcastexpression_t* index)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_INDEX);
    if(!res)
    {
        return NULL;
    }
    res->exprindex.left = left;
    res->exprindex.index = index;
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_assign(mcstate_t* state, mcastexpression_t* dest, mcastexpression_t* source, bool is_postfix)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_ASSIGN);
    if(!res)
    {
        return NULL;
    }
    res->exprassign.dest = dest;
    res->exprassign.source = source;
    res->exprassign.is_postfix = is_postfix;
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_logical(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* left, mcastexpression_t* right)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_LOGICAL);
    if(!res)
    {
        return NULL;
    }
    res->exprlogical.op = op;
    res->exprlogical.left = left;
    res->exprlogical.right = right;
    return res;
}

TMPSTATIC mcastexpression_t* expression_make_ternary(mcstate_t* state, mcastexpression_t* test, mcastexpression_t* ift, mcastexpression_t* iffalse)
{
    mcastexpression_t* res = expression_make(state, MC_EXPR_TERNARY);
    if(!res)
    {
        return NULL;
    }
    res->exprternary.tercond = test;
    res->exprternary.teriftrue = ift;
    res->exprternary.teriffalse = iffalse;
    return res;
}

TMPSEMISTATIC void expression_destroy(mcastexpression_t* expr)
{
    if(!expr)
    {
        return;
    }

    switch(expr->type)
    {
        case MC_EXPR_NONE:
        {
            MC_ASSERT(false);
            break;
        }
        case MC_EXPR_IDENT:
        {
            ident_destroy(expr->exprident);
            break;
        }
        case MC_EXPR_NUMBERLITERAL:
        case MC_EXPR_BOOLLITERAL:
        {
            break;
        }
        case MC_EXPR_STRINGLITERAL:
        {
            mc_allocator_free(expr->pstate, expr->exprlitstring.data);
            break;
        }
        case MC_EXPR_NULLLITERAL:
        {
            break;
        }
        case MC_EXPR_ARRAYLITERAL:
        {
            mc_ptrarray_destroy(expr->exprlitarray, (mcptrarrayitemdestroyfn_t)expression_destroy);
            break;
        }
        case MC_EXPR_MAPLITERAL:
        {
            mc_ptrarray_destroy(expr->exprlitmap.keys, (mcptrarrayitemdestroyfn_t)expression_destroy);
            mc_ptrarray_destroy(expr->exprlitmap.values, (mcptrarrayitemdestroyfn_t)expression_destroy);
            break;
        }
        case MC_EXPR_PREFIX:
        {
            expression_destroy(expr->exprprefix.right);
            break;
        }
        case MC_EXPR_INFIX:
        {
            expression_destroy(expr->exprinfix.left);
            expression_destroy(expr->exprinfix.right);
            break;
        }
        case MC_EXPR_FUNCTIONLITERAL:
        {
            fn_literal_t* fn = &expr->exprlitfunction;
            mc_allocator_free(expr->pstate, fn->name);
            mc_ptrarray_destroy(fn->params, (mcptrarrayitemdestroyfn_t)ident_destroy);
            code_block_destroy(fn->body);
            break;
        }
        case MC_EXPR_CALL:
        {
            mc_ptrarray_destroy(expr->exprcall.args, (mcptrarrayitemdestroyfn_t)expression_destroy);
            expression_destroy(expr->exprcall.function);
            break;
        }
        case MC_EXPR_INDEX:
        {
            expression_destroy(expr->exprindex.left);
            expression_destroy(expr->exprindex.index);
            break;
        }
        case MC_EXPR_ASSIGN:
        {
            expression_destroy(expr->exprassign.dest);
            expression_destroy(expr->exprassign.source);
            break;
        }
        case MC_EXPR_LOGICAL:
        {
            expression_destroy(expr->exprlogical.left);
            expression_destroy(expr->exprlogical.right);
            break;
        }
        case MC_EXPR_TERNARY:
        {
            expression_destroy(expr->exprternary.tercond);
            expression_destroy(expr->exprternary.teriftrue);
            expression_destroy(expr->exprternary.teriffalse);
            break;
        }
        default:
            break;
    }
    mc_allocator_free(expr->pstate, expr);
}

TMPSEMISTATIC mcastexpression_t* expression_copy(mcastexpression_t* expr)
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
            MC_ASSERT(false);
            break;
        }
        case MC_EXPR_IDENT:
        {
            ident_t* ident = ident_copy(expr->exprident);
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
            res = expression_make_number_literal(expr->pstate, expr->exprlitnumber);
            break;
        }
        case MC_EXPR_BOOLLITERAL:
        {
            res = expression_make_bool_literal(expr->pstate, expr->exprlitbool);
            break;
        }
        case MC_EXPR_STRINGLITERAL:
        {
            char* stringcopy = mc_util_strndup(expr->pstate, expr->exprlitstring.data, expr->exprlitstring.length);
            if(!stringcopy)
            {
                return NULL;
            }
            res = expression_make_string_literal(expr->pstate, stringcopy, expr->exprlitstring.length);
            if(!res)
            {
                mc_allocator_free(expr->pstate, stringcopy);
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
            mcptrarray_t* valuescopy = mc_ptrarray_copy(expr->exprlitarray, (mcptrarrayitemcopyfn_t)expression_copy, (mcptrarrayitemdestroyfn_t)expression_destroy);
            if(!valuescopy)
            {
                return NULL;
            }
            res = expression_make_array_literal(expr->pstate, valuescopy);
            if(!res)
            {
                mc_ptrarray_destroy(valuescopy, (mcptrarrayitemdestroyfn_t)expression_destroy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_MAPLITERAL:
        {
            mcptrarray_t* keyscopy = mc_ptrarray_copy(expr->exprlitmap.keys, (mcptrarrayitemcopyfn_t)expression_copy, (mcptrarrayitemdestroyfn_t)expression_destroy);
            mcptrarray_t* valuescopy = mc_ptrarray_copy(expr->exprlitmap.values, (mcptrarrayitemcopyfn_t)expression_copy, (mcptrarrayitemdestroyfn_t)expression_destroy);
            if(!keyscopy || !valuescopy)
            {
                mc_ptrarray_destroy(keyscopy, (mcptrarrayitemdestroyfn_t)expression_destroy);
                mc_ptrarray_destroy(valuescopy, (mcptrarrayitemdestroyfn_t)expression_destroy);
                return NULL;
            }
            res = expression_make_map_literal(expr->pstate, keyscopy, valuescopy);
            if(!res)
            {
                mc_ptrarray_destroy(keyscopy, (mcptrarrayitemdestroyfn_t)expression_destroy);
                mc_ptrarray_destroy(valuescopy, (mcptrarrayitemdestroyfn_t)expression_destroy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_PREFIX:
        {
            mcastexpression_t* rightcopy = expression_copy(expr->exprprefix.right);
            if(!rightcopy)
            {
                return NULL;
            }
            res = expression_make_prefix(expr->pstate, expr->exprprefix.op, rightcopy);
            if(!res)
            {
                expression_destroy(rightcopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_INFIX:
        {
            mcastexpression_t* leftcopy = expression_copy(expr->exprinfix.left);
            mcastexpression_t* rightcopy = expression_copy(expr->exprinfix.right);
            if(!leftcopy || !rightcopy)
            {
                expression_destroy(leftcopy);
                expression_destroy(rightcopy);
                return NULL;
            }
            res = expression_make_infix(expr->pstate, expr->exprinfix.op, leftcopy, rightcopy);
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
            mcptrarray_t* paramscopy = mc_ptrarray_copy(expr->exprlitfunction.params, (mcptrarrayitemcopyfn_t)ident_copy, (mcptrarrayitemdestroyfn_t)ident_destroy);
            code_block_t* bodycopy = code_block_copy(expr->exprlitfunction.body);
            char* namecopy = mc_util_strdup(expr->pstate, expr->exprlitfunction.name);
            if(!paramscopy || !bodycopy)
            {
                mc_ptrarray_destroy(paramscopy, (mcptrarrayitemdestroyfn_t)ident_destroy);
                code_block_destroy(bodycopy);
                mc_allocator_free(expr->pstate, namecopy);
                return NULL;
            }
            res = expression_make_fn_literal(expr->pstate, paramscopy, bodycopy);
            if(!res)
            {
                mc_ptrarray_destroy(paramscopy, (mcptrarrayitemdestroyfn_t)ident_destroy);
                code_block_destroy(bodycopy);
                mc_allocator_free(expr->pstate, namecopy);
                return NULL;
            }
            res->exprlitfunction.name = namecopy;
            break;
        }
        case MC_EXPR_CALL:
        {
            mcastexpression_t* functioncopy = expression_copy(expr->exprcall.function);
            mcptrarray_t* argscopy = mc_ptrarray_copy(expr->exprcall.args, (mcptrarrayitemcopyfn_t)expression_copy, (mcptrarrayitemdestroyfn_t)expression_destroy);
            if(!functioncopy || !argscopy)
            {
                expression_destroy(functioncopy);
                mc_ptrarray_destroy(expr->exprcall.args, (mcptrarrayitemdestroyfn_t)expression_destroy);
                return NULL;
            }
            res = expression_make_call(expr->pstate, functioncopy, argscopy);
            if(!res)
            {
                expression_destroy(functioncopy);
                mc_ptrarray_destroy(expr->exprcall.args, (mcptrarrayitemdestroyfn_t)expression_destroy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_INDEX:
        {
            mcastexpression_t* leftcopy = expression_copy(expr->exprindex.left);
            mcastexpression_t* indexcopy = expression_copy(expr->exprindex.index);
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
            mcastexpression_t* destcopy = expression_copy(expr->exprassign.dest);
            mcastexpression_t* sourcecopy = expression_copy(expr->exprassign.source);
            if(!destcopy || !sourcecopy)
            {
                expression_destroy(destcopy);
                expression_destroy(sourcecopy);
                return NULL;
            }
            res = expression_make_assign(expr->pstate, destcopy, sourcecopy, expr->exprassign.is_postfix);
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
            mcastexpression_t* leftcopy = expression_copy(expr->exprlogical.left);
            mcastexpression_t* rightcopy = expression_copy(expr->exprlogical.right);
            if(!leftcopy || !rightcopy)
            {
                expression_destroy(leftcopy);
                expression_destroy(rightcopy);
                return NULL;
            }
            res = expression_make_logical(expr->pstate, expr->exprlogical.op, leftcopy, rightcopy);
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
            mcastexpression_t* testcopy = expression_copy(expr->exprternary.tercond);
            mcastexpression_t* iftruecopy = expression_copy(expr->exprternary.teriftrue);
            mcastexpression_t* iffalsecopy = expression_copy(expr->exprternary.teriffalse);
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

TMPSTATIC mcastexpression_t* statement_make_define(mcstate_t* state, ident_t* name, mcastexpression_t* value, bool assignable)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTDEFINE);
    if(!res)
    {
        return NULL;
    }
    res->exprdefine.name = name;
    res->exprdefine.value = value;
    res->exprdefine.assignable = assignable;
    return res;
}

TMPSTATIC mcastexpression_t* statement_make_if(mcstate_t* state, mcptrarray_t* cases, code_block_t* alternative)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTIF);
    if(!res)
    {
        return NULL;
    }
    res->exprifstmt.cases = cases;
    res->exprifstmt.alternative = alternative;
    return res;
}

TMPSTATIC mcastexpression_t* statement_make_return(mcstate_t* state, mcastexpression_t* value)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTRETURN);
    if(!res)
    {
        return NULL;
    }
    res->exprreturnvalue = value;
    return res;
}

TMPSTATIC mcastexpression_t* statement_make_expression(mcstate_t* state, mcastexpression_t* value)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTEXPRESSION);
    if(!res)
    {
        return NULL;
    }
    res->exprexpression = value;
    return res;
}

TMPSTATIC mcastexpression_t* statement_make_while_loop(mcstate_t* state, mcastexpression_t* test, code_block_t* body)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTLOOPWHILE);
    if(!res)
    {
        return NULL;
    }
    res->exprwhileloopstmt.loopcond = test;
    res->exprwhileloopstmt.body = body;
    return res;
}

TMPSTATIC mcastexpression_t* statement_make_break(mcstate_t* state)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTBREAK);
    if(!res)
    {
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* statement_make_foreach(mcstate_t* state, ident_t* iterator, mcastexpression_t* source, code_block_t* body)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTLOOPFOREACH);
    if(!res)
    {
        return NULL;
    }
    res->exprforeachloopstmt.iterator = iterator;
    res->exprforeachloopstmt.source = source;
    res->exprforeachloopstmt.body = body;
    return res;
}

TMPSTATIC mcastexpression_t* statement_make_for_loop(mcstate_t* state, mcastexpression_t* init, mcastexpression_t* test, mcastexpression_t* update, code_block_t* body)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTLOOPFORCLASSIC);
    if(!res)
    {
        return NULL;
    }
    res->exprforloopstmt.init = init;
    res->exprforloopstmt.loopcond = test;
    res->exprforloopstmt.update = update;
    res->exprforloopstmt.body = body;
    return res;
}

TMPSTATIC mcastexpression_t* statement_make_continue(mcstate_t* state)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTCONTINUE);
    if(!res)
    {
        return NULL;
    }
    return res;
}

TMPSTATIC mcastexpression_t* statement_make_block(mcstate_t* state, code_block_t* block)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTBLOCK);
    if(!res)
    {
        return NULL;
    }
    res->exprblockstmt = block;
    return res;
}

TMPSTATIC mcastexpression_t* statement_make_import(mcstate_t* state, char* path)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTIMPORT);
    if(!res)
    {
        return NULL;
    }
    res->exprimportstmt.path = path;
    return res;
}

TMPSTATIC mcastexpression_t* statement_make_recover(mcstate_t* state, ident_t* eid, code_block_t* body)
{
    mcastexpression_t* res = statement_make(state, MC_EXPR_STMTRECOVER);
    if(!res)
    {
        return NULL;
    }
    res->exprrecoverstmt.errident = eid;
    res->exprrecoverstmt.body = body;
    return res;
}

TMPSEMISTATIC void statement_destroy(mcastexpression_t* expr)
{
    if(!expr)
    {
        return;
    }
    switch(expr->type)
    {
        case MC_EXPR_NONE:
        {
            MC_ASSERT(false);
            break;
        }
        case MC_EXPR_STMTDEFINE:
        {
            ident_destroy(expr->exprdefine.name);
            expression_destroy(expr->exprdefine.value);
            break;
        }
        case MC_EXPR_STMTIF:
        {
            mc_ptrarray_destroy(expr->exprifstmt.cases, (mcptrarrayitemdestroyfn_t)if_case_destroy);
            code_block_destroy(expr->exprifstmt.alternative);
            break;
        }
        case MC_EXPR_STMTRETURN:
        {
            expression_destroy(expr->exprreturnvalue);
            break;
        }
        case MC_EXPR_STMTEXPRESSION:
        {
            expression_destroy(expr->exprexpression);
            break;
        }
        case MC_EXPR_STMTLOOPWHILE:
        {
            expression_destroy(expr->exprwhileloopstmt.loopcond);
            code_block_destroy(expr->exprwhileloopstmt.body);
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
            ident_destroy(expr->exprforeachloopstmt.iterator);
            expression_destroy(expr->exprforeachloopstmt.source);
            code_block_destroy(expr->exprforeachloopstmt.body);
            break;
        }
        case MC_EXPR_STMTLOOPFORCLASSIC:
        {
            statement_destroy(expr->exprforloopstmt.init);
            expression_destroy(expr->exprforloopstmt.loopcond);
            expression_destroy(expr->exprforloopstmt.update);
            code_block_destroy(expr->exprforloopstmt.body);
            break;
        }
        case MC_EXPR_STMTBLOCK:
        {
            code_block_destroy(expr->exprblockstmt);
            break;
        }
        case MC_EXPR_STMTIMPORT:
        {
            mc_allocator_free(expr->pstate, expr->exprimportstmt.path);
            break;
        }
        case MC_EXPR_STMTRECOVER:
        {
            code_block_destroy(expr->exprrecoverstmt.body);
            ident_destroy(expr->exprrecoverstmt.errident);
            break;
        }
        default:
            break;
    }
    mc_allocator_free(expr->pstate, expr);
}

TMPSEMISTATIC mcastexpression_t* statement_copy(mcastexpression_t* expr)
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
            MC_ASSERT(false);
            break;
        }
        case MC_EXPR_STMTDEFINE:
        {
            mcastexpression_t* valuecopy = expression_copy(expr->exprdefine.value);
            if(!valuecopy)
            {
                return NULL;
            }
            res = statement_make_define(expr->pstate, ident_copy(expr->exprdefine.name), valuecopy, expr->exprdefine.assignable);
            if(!res)
            {
                expression_destroy(valuecopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTIF:
        {
            mcptrarray_t* casescopy = mc_ptrarray_copy(expr->exprifstmt.cases, (mcptrarrayitemcopyfn_t)if_case_copy, (mcptrarrayitemdestroyfn_t)if_case_destroy);
            code_block_t* alternativecopy = code_block_copy(expr->exprifstmt.alternative);
            if(!casescopy || !alternativecopy)
            {
                mc_ptrarray_destroy(casescopy, (mcptrarrayitemdestroyfn_t)if_case_destroy);
                code_block_destroy(alternativecopy);
                return NULL;
            }
            res = statement_make_if(expr->pstate, casescopy, alternativecopy);
            if(res)
            {
                mc_ptrarray_destroy(casescopy, (mcptrarrayitemdestroyfn_t)if_case_destroy);
                code_block_destroy(alternativecopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTRETURN:
        {
            mcastexpression_t* valuecopy = expression_copy(expr->exprreturnvalue);
            if(!valuecopy)
            {
                return NULL;
            }
            res = statement_make_return(expr->pstate, valuecopy);
            if(!res)
            {
                expression_destroy(valuecopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTEXPRESSION:
        {
            mcastexpression_t* valuecopy = expression_copy(expr->exprexpression);
            if(!valuecopy)
            {
                return NULL;
            }
            res = statement_make_expression(expr->pstate, valuecopy);
            if(!res)
            {
                expression_destroy(valuecopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTLOOPWHILE:
        {
            mcastexpression_t* testcopy = expression_copy(expr->exprwhileloopstmt.loopcond);
            code_block_t* bodycopy = code_block_copy(expr->exprwhileloopstmt.body);
            if(!testcopy || !bodycopy)
            {
                expression_destroy(testcopy);
                code_block_destroy(bodycopy);
                return NULL;
            }
            res = statement_make_while_loop(expr->pstate, testcopy, bodycopy);
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
            res = statement_make_break(expr->pstate);
            break;
        }
        case MC_EXPR_STMTCONTINUE:
        {
            res = statement_make_continue(expr->pstate);
            break;
        }
        case MC_EXPR_STMTLOOPFOREACH:
        {
            mcastexpression_t* sourcecopy = expression_copy(expr->exprforeachloopstmt.source);
            code_block_t* bodycopy = code_block_copy(expr->exprforeachloopstmt.body);
            if(!sourcecopy || !bodycopy)
            {
                expression_destroy(sourcecopy);
                code_block_destroy(bodycopy);
                return NULL;
            }
            res = statement_make_foreach(expr->pstate, ident_copy(expr->exprforeachloopstmt.iterator), sourcecopy, bodycopy);
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
            mcastexpression_t* initcopy = statement_copy(expr->exprforloopstmt.init);
            mcastexpression_t* testcopy = expression_copy(expr->exprforloopstmt.loopcond);
            mcastexpression_t* updatecopy = expression_copy(expr->exprforloopstmt.update);
            code_block_t* bodycopy = code_block_copy(expr->exprforloopstmt.body);
            if(!initcopy || !testcopy || !updatecopy || !bodycopy)
            {
                statement_destroy(initcopy);
                expression_destroy(testcopy);
                expression_destroy(updatecopy);
                code_block_destroy(bodycopy);
                return NULL;
            }
            res = statement_make_for_loop(expr->pstate, initcopy, testcopy, updatecopy, bodycopy);
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
            code_block_t* blockcopy = code_block_copy(expr->exprblockstmt);
            if(!blockcopy)
            {
                return NULL;
            }
            res = statement_make_block(expr->pstate, blockcopy);
            if(!res)
            {
                code_block_destroy(blockcopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTIMPORT:
        {
            char* pathcopy = mc_util_strdup(expr->pstate, expr->exprimportstmt.path);
            if(!pathcopy)
            {
                return NULL;
            }
            res = statement_make_import(expr->pstate, pathcopy);
            if(!res)
            {
                mc_allocator_free(expr->pstate, pathcopy);
                return NULL;
            }
            break;
        }
        case MC_EXPR_STMTRECOVER:
        {
            code_block_t* bodycopy = code_block_copy(expr->exprrecoverstmt.body);
            ident_t* erroridentcopy = ident_copy(expr->exprrecoverstmt.errident);
            if(!bodycopy || !erroridentcopy)
            {
                code_block_destroy(bodycopy);
                ident_destroy(erroridentcopy);
                return NULL;
            }
            res = statement_make_recover(expr->pstate, erroridentcopy, bodycopy);
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
    res->pos = expr->pos;
    return res;
}

TMPSTATIC code_block_t* code_block_make(mcstate_t* state, mcptrarray_t* statements)
{
    code_block_t* block = (code_block_t*)mc_allocator_alloc(state, sizeof(code_block_t));
    if(!block)
    {
        return NULL;
    }
    block->pstate = state;
    block->statements = statements;
    return block;
}

TMPSTATIC void code_block_destroy(code_block_t* block)
{
    if(!block)
    {
        return;
    }
    mc_ptrarray_destroy(block->statements, (mcptrarrayitemdestroyfn_t)statement_destroy);
    mc_allocator_free(block->pstate, block);
}

TMPSTATIC code_block_t* code_block_copy(code_block_t* block)
{
    if(!block)
    {
        return NULL;
    }
    mcptrarray_t* statementscopy = mc_ptrarray_copy(block->statements, (mcptrarrayitemcopyfn_t)statement_copy, (mcptrarrayitemdestroyfn_t)statement_destroy);
    if(!statementscopy)
    {
        return NULL;
    }
    code_block_t* res = code_block_make(block->pstate, statementscopy);
    if(!res)
    {
        mc_ptrarray_destroy(statementscopy, (mcptrarrayitemdestroyfn_t)statement_destroy);
        return NULL;
    }
    return res;
}

TMPSTATIC void mc_astprint_stmtlist(mcprintstate_t* pr, mcptrarray_t* statements)
{
    int i;
    int count;
    mcastexpression_t* expr;
    count = mc_ptrarray_count(statements);
    for(i = 0; i < count; i++)
    {
        expr = (mcastexpression_t*)mc_ptrarray_get(statements, i);
        mc_astprint_statement(expr, pr);
        if(i < (count - 1))
        {
            mc_printer_puts(pr, "\n");
        }
    }
}

TMPSEMISTATIC void mc_astprint_statement(mcastexpression_t* expr, mcprintstate_t* buf)
{
    switch(expr->type)
    {
        case MC_EXPR_STMTDEFINE:
        {
            define_statement_t* defstmt = &expr->exprdefine;
            if(expr->exprdefine.assignable)
            {
                mc_printer_puts(buf, "var ");
            }
            else
            {
                mc_printer_puts(buf, "const ");
            }
            mc_printer_puts(buf, defstmt->name->value);
            mc_printer_puts(buf, " = ");

            if(defstmt->value)
            {
                mc_astprint_expression(defstmt->value, buf);
            }

            break;
        }
        case MC_EXPR_STMTIF:
            {
                int i;
                if_case_t* ifcase = (if_case_t*)mc_ptrarray_get(expr->exprifstmt.cases, 0);
                mc_printer_puts(buf, "if (");
                mc_astprint_expression(ifcase->ifcond, buf);
                mc_printer_puts(buf, ") ");
                mc_astprint_codeblock(ifcase->consequence, buf);
                for(i = 1; i < mc_ptrarray_count(expr->exprifstmt.cases); i++)
                {
                    if_case_t* elifcase = (if_case_t*)mc_ptrarray_get(expr->exprifstmt.cases, i);
                    mc_printer_puts(buf, " elif (");
                    mc_astprint_expression(elifcase->ifcond, buf);
                    mc_printer_puts(buf, ") ");
                    mc_astprint_codeblock(elifcase->consequence, buf);
                }
                if(expr->exprifstmt.alternative)
                {
                    mc_printer_puts(buf, " else ");
                    mc_astprint_codeblock(expr->exprifstmt.alternative, buf);
                }
            }
            break;
        case MC_EXPR_STMTRETURN:
        {
            mc_printer_puts(buf, "return ");
            if(expr->exprreturnvalue)
            {
                mc_astprint_expression(expr->exprreturnvalue, buf);
            }
            break;
        }
        case MC_EXPR_STMTEXPRESSION:
        {
            if(expr->exprexpression)
            {
                mc_astprint_expression(expr->exprexpression, buf);
            }
            break;
        }
        case MC_EXPR_STMTLOOPWHILE:
        {
            mc_printer_puts(buf, "while (");
            mc_astprint_expression(expr->exprwhileloopstmt.loopcond, buf);
            mc_printer_puts(buf, ")");
            mc_astprint_codeblock(expr->exprwhileloopstmt.body, buf);
            break;
        }
        case MC_EXPR_STMTLOOPFORCLASSIC:
        {
            mc_printer_puts(buf, "for (");
            if(expr->exprforloopstmt.init)
            {
                mc_astprint_statement(expr->exprforloopstmt.init, buf);
                mc_printer_puts(buf, " ");
            }
            else
            {
                mc_printer_puts(buf, ";");
            }
            if(expr->exprforloopstmt.loopcond)
            {
                mc_astprint_expression(expr->exprforloopstmt.loopcond, buf);
                mc_printer_puts(buf, "; ");
            }
            else
            {
                mc_printer_puts(buf, ";");
            }
            if(expr->exprforloopstmt.update)
            {
                mc_astprint_expression(expr->exprforloopstmt.loopcond, buf);
            }
            mc_printer_puts(buf, ")");
            mc_astprint_codeblock(expr->exprforloopstmt.body, buf);
            break;
        }
        case MC_EXPR_STMTLOOPFOREACH:
        {
            mc_printer_puts(buf, "for (");
            mc_printer_printf(buf, "%s", expr->exprforeachloopstmt.iterator->value);
            mc_printer_puts(buf, " in ");
            mc_astprint_expression(expr->exprforeachloopstmt.source, buf);
            mc_printer_puts(buf, ")");
            mc_astprint_codeblock(expr->exprforeachloopstmt.body, buf);
            break;
        }
        case MC_EXPR_STMTBLOCK:
        {
            mc_astprint_codeblock(expr->exprblockstmt, buf);
            break;
        }
        case MC_EXPR_STMTBREAK:
        {
            mc_printer_puts(buf, "break");
            break;
        }
        case MC_EXPR_STMTCONTINUE:
        {
            mc_printer_puts(buf, "continue");
            break;
        }
        case MC_EXPR_STMTIMPORT:
        {
            mc_printer_printf(buf, "import \"%s\"", expr->exprimportstmt.path);
            break;
        }
        case MC_EXPR_NONE:
        {
            mc_printer_puts(buf, "MC_EXPR_NONE");
            break;
        }
        case MC_EXPR_STMTRECOVER:
        {
            mc_printer_printf(buf, "recover (%s)", expr->exprrecoverstmt.errident->value);
            mc_astprint_codeblock(expr->exprrecoverstmt.body, buf);
            break;
        }
        default:
            break;
    }
}

TMPSEMISTATIC void mc_astprint_expression(mcastexpression_t* expr, mcprintstate_t* buf)
{
    switch(expr->type)
    {
        case MC_EXPR_IDENT:
        {
            mc_printer_puts(buf, expr->exprident->value);
            break;
        }
        case MC_EXPR_NUMBERLITERAL:
        {
            mc_printer_printf(buf, "%1.17g", expr->exprlitnumber);
            break;
        }
        case MC_EXPR_BOOLLITERAL:
        {
            mc_printer_printf(buf, "%s", expr->exprlitbool ? "true" : "false");
            break;
        }
        case MC_EXPR_STRINGLITERAL:
        {
            mc_printer_printf(buf, "\"%.*s\"", (int)expr->exprlitstring.length, expr->exprlitstring.data);
            break;
        }
        case MC_EXPR_NULLLITERAL:
        {
            mc_printer_puts(buf, "null");
            break;
        }
        case MC_EXPR_ARRAYLITERAL:
            {
                int i;
                mc_printer_puts(buf, "[");
                for(i = 0; i < mc_ptrarray_count(expr->exprlitarray); i++)
                {
                    mcastexpression_t* arrexpr = (mcastexpression_t*)mc_ptrarray_get(expr->exprlitarray, i);
                    mc_astprint_expression(arrexpr, buf);
                    if(i < (mc_ptrarray_count(expr->exprlitarray) - 1))
                    {
                        mc_printer_puts(buf, ", ");
                    }
                }
                mc_printer_puts(buf, "]");
            }
            break;
        case MC_EXPR_MAPLITERAL:
        {
            int i;
            map_literal_t* map;
            map = &expr->exprlitmap;
            mc_printer_puts(buf, "{");
            for(i = 0; i < mc_ptrarray_count(map->keys); i++)
            {
                mcastexpression_t* keyexpr = (mcastexpression_t*)mc_ptrarray_get(map->keys, i);
                mcastexpression_t* valexpr = (mcastexpression_t*)mc_ptrarray_get(map->values, i);

                mc_astprint_expression(keyexpr, buf);
                mc_printer_puts(buf, " : ");
                mc_astprint_expression(valexpr, buf);

                if(i < (mc_ptrarray_count(map->keys) - 1))
                {
                    mc_printer_puts(buf, ", ");
                }
            }
            mc_printer_puts(buf, "}");
            break;
        }
        case MC_EXPR_PREFIX:
        {
            mc_printer_puts(buf, "(");
            mc_printer_puts(buf, operator_to_string(expr->exprinfix.op));
            mc_astprint_expression(expr->exprprefix.right, buf);
            mc_printer_puts(buf, ")");
            break;
        }
        case MC_EXPR_INFIX:
        {
            mc_printer_puts(buf, "(");
            mc_astprint_expression(expr->exprinfix.left, buf);
            mc_printer_puts(buf, " ");
            mc_printer_puts(buf, operator_to_string(expr->exprinfix.op));
            mc_printer_puts(buf, " ");
            mc_astprint_expression(expr->exprinfix.right, buf);
            mc_printer_puts(buf, ")");
            break;
        }
        case MC_EXPR_FUNCTIONLITERAL:
        {
            int i;
            fn_literal_t* fn = &expr->exprlitfunction;
            mc_printer_puts(buf, "function");
            mc_printer_puts(buf, "(");
            for(i = 0; i < mc_ptrarray_count(fn->params); i++)
            {
                ident_t* param = (ident_t*)mc_ptrarray_get(fn->params, i);
                mc_printer_puts(buf, param->value);
                if(i < (mc_ptrarray_count(fn->params) - 1))
                {
                    mc_printer_puts(buf, ", ");
                }
            }
            mc_printer_puts(buf, ") ");

            mc_astprint_codeblock(fn->body, buf);

            break;
        }
        case MC_EXPR_CALL:
        {
            int i;
            call_expression_t* ce = &expr->exprcall;

            mc_astprint_expression(ce->function, buf);

            mc_printer_puts(buf, "(");
            for(i = 0; i < mc_ptrarray_count(ce->args); i++)
            {
                mcastexpression_t* arg = (mcastexpression_t*)mc_ptrarray_get(ce->args, i);
                mc_astprint_expression(arg, buf);
                if(i < (mc_ptrarray_count(ce->args) - 1))
                {
                    mc_printer_puts(buf, ", ");
                }
            }
            mc_printer_puts(buf, ")");

            break;
        }
        case MC_EXPR_INDEX:
        {
            mc_printer_puts(buf, "(");
            mc_astprint_expression(expr->exprindex.left, buf);
            mc_printer_puts(buf, "[");
            mc_astprint_expression(expr->exprindex.index, buf);
            mc_printer_puts(buf, "])");
            break;
        }
        case MC_EXPR_ASSIGN:
        {
            mc_astprint_expression(expr->exprassign.dest, buf);
            mc_printer_puts(buf, " = ");
            mc_astprint_expression(expr->exprassign.source, buf);
            break;
        }
        case MC_EXPR_LOGICAL:
        {
            mc_astprint_expression(expr->exprlogical.left, buf);
            mc_printer_puts(buf, " ");
            mc_printer_puts(buf, operator_to_string(expr->exprinfix.op));
            mc_printer_puts(buf, " ");
            mc_astprint_expression(expr->exprlogical.right, buf);
            break;
        }
        case MC_EXPR_TERNARY:
        {
            mc_astprint_expression(expr->exprternary.tercond, buf);
            mc_printer_puts(buf, " ? ");
            mc_astprint_expression(expr->exprternary.teriftrue, buf);
            mc_printer_puts(buf, " : ");
            mc_astprint_expression(expr->exprternary.teriffalse, buf);
            break;
        }
        case MC_EXPR_NONE:
        {
            mc_printer_puts(buf, "MC_EXPR_NONE");
            break;
        }
        default:
            break;
    }
}

TMPSTATIC void mc_astprint_codeblock(code_block_t* expr, mcprintstate_t* buf)
{
    int i;
    mcastexpression_t* istmt;
    mc_printer_puts(buf, "{ ");
    for(i = 0; i < mc_ptrarray_count(expr->statements); i++)
    {
        istmt = (mcastexpression_t*)mc_ptrarray_get(expr->statements, i);
        mc_astprint_statement(istmt, buf);
        mc_printer_puts(buf, "\n");
    }
    mc_printer_puts(buf, " }");
}

TMPSTATIC const char* operator_to_string(mcastmathoptype_t op)
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

TMPUNUSED const char* expression_type_to_string(mcastexprtype_t type)
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

TMPSTATIC ident_t* ident_make(mcstate_t* state, mcasttoken_t tok)
{
    ident_t* res = (ident_t*)mc_allocator_alloc(state, sizeof(ident_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->value = token_duplicate_literal(state, &tok);
    if(!res->value)
    {
        mc_allocator_free(state, res);
        return NULL;
    }
    res->pos = tok.pos;
    return res;
}

TMPSTATIC ident_t* ident_copy(ident_t* ident)
{
    ident_t* res = (ident_t*)mc_allocator_alloc(ident->pstate, sizeof(ident_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = ident->pstate;
    res->value = mc_util_strdup(ident->pstate, ident->value);
    if(!res->value)
    {
        mc_allocator_free(ident->pstate, res);
        return NULL;
    }
    res->pos = ident->pos;
    return res;
}

TMPSTATIC void ident_destroy(ident_t* ident)
{
    if(!ident)
    {
        return;
    }
    mc_allocator_free(ident->pstate, ident->value);
    ident->value = NULL;
    ident->pos = srcposinvalid;
    mc_allocator_free(ident->pstate, ident);
}

TMPSTATIC if_case_t* if_case_make(mcstate_t* state, mcastexpression_t* test, code_block_t* consequence)
{
    if_case_t* res = (if_case_t*)mc_allocator_alloc(state, sizeof(if_case_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->ifcond = test;
    res->consequence = consequence;
    return res;
}

TMPSTATIC void if_case_destroy(if_case_t* cond)
{
    if(!cond)
    {
        return;
    }
    expression_destroy(cond->ifcond);
    code_block_destroy(cond->consequence);
    mc_allocator_free(cond->pstate, cond);
}

TMPSTATIC if_case_t* if_case_copy(if_case_t* ifcase)
{
    if(!ifcase)
    {
        return NULL;
    }
    mcastexpression_t* testcopy = NULL;
    code_block_t* consequencecopy = NULL;
    if_case_t* if_case_copy = NULL;

    testcopy = expression_copy(ifcase->ifcond);
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
    mcastexpression_t* res = (mcastexpression_t*)mc_allocator_alloc(state, sizeof(mcastexpression_t));
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
    mcastexpression_t* res = (mcastexpression_t*)mc_allocator_alloc(state, sizeof(mcastexpression_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->type = type;
    res->pos = srcposinvalid;
    return res;
}

#if defined(__cplusplus) || 1
    #define CHECK_ARGS(state, generateerror, argc, args, ...) true
#else
    #define CHECK_ARGS(state, generateerror, argc, args, ...) \
        mc_args_check((state), (generateerror), (argc), (args), sizeof((mcobjtype_t[]){ __VA_ARGS__ }) / sizeof(mcobjtype_t), (mcobjtype_t[]){ __VA_ARGS__ })
#endif

TMPUNUSED bool mc_args_check(mcstate_t* vm, bool generateerror, int argc, mcvalue_t* args, int expectedargc, const mcobjtype_t* expectedtypes)
{
    int i;
    if(argc != expectedargc)
    {
        if(generateerror)
        {
            mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, srcposinvalid, "Invalid number or arguments, got %d instead of %d", argc, expectedargc);
        }
        return false;
    }

    for(i = 0; i < argc; i++)
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
                mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, srcposinvalid, "Invalid argument %d type, got %s, expected %s", i, typestr, expectedtypestr);
                mc_allocator_free(vm, expectedtypestr);
            }
            return false;
        }
    }
    return true;
}


TMPUNUSED mcvalue_t cfn_binnot(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    double dn;
    int64_t iv;
    (void)state;
    (void)data;
    (void)argc;
    dn = args[0].valnumber;
    iv = (int64_t)dn;
    iv = ~iv;
    return mc_value_makenumber(iv);
}


TMPUNUSED mcvalue_t cfn_ord(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    char ch;
    const char* str;
    (void)state;
    (void)data;
    (void)argc;
    str = args[0].objdatahandle->valstring.data;
    ch = str[0];
    return mc_value_makenumber(ch);
}


TMPUNUSED mcvalue_t cfn_arrayjoin(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int i;
    (void)state;
    (void)argc;
    (void)data;
    mcprintstate_t pr;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ARRAY, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    mcvalue_t array = args[0];
    int alen = mc_value_arraygetlength(array);
    mc_printer_init(&pr, state, alen, NULL, true);
    for(i=1; i<alen; i++)
    {
        mcvalue_t item = mc_value_arraygetvalueat(array, i);
        mc_printer_printobject(item, &pr, false);
    }
    const char* str = pr.data;
    int slen = pr.len;
    mcvalue_t rt = mc_value_makestringlen(state, str, slen);
    mc_printer_destroy(&pr);
    return rt;


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
    int startindex = argc == 3 && args[2].type == MC_OBJ_NUMBER ? object_get_number(args[2]) : 0;

    if((argc == 2 || argc == 3) && args[0].type == MC_OBJ_STRING && args[1].type == MC_OBJ_STRING)
    {
        const char* searchstr = mc_value_stringgetdata(args[0]);
        const char* searchfor = mc_value_stringgetdata(args[1]);
        char* result = (char*)strstr(searchstr + startindex, searchfor);

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
    if(argc == 2 && args[0].type == MC_OBJ_STRING && args[1].type == MC_OBJ_NUMBER)
    {
        const char* searchstr = mc_value_stringgetdata(args[0]);
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
    if(argc == 2 && args[0].type == MC_OBJ_STRING && args[1].type == MC_OBJ_NUMBER)
    {
        const char* searchstr = mc_value_stringgetdata(args[0]);
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
    if(argc == 3 && args[0].type == MC_OBJ_STRING && args[1].type == MC_OBJ_STRING && args[2].type == MC_OBJ_STRING)
    {
        const char* str = mc_value_stringgetdata(args[0]);
        const char* searchstr = mc_value_stringgetdata(args[1]);
        const char* replacestr = mc_value_stringgetdata(args[2]);

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
    if(argc == 3 && args[0].type == MC_OBJ_STRING && args[1].type == MC_OBJ_STRING && args[2].type == MC_OBJ_STRING)
    {
        const char* str = mc_value_stringgetdata(args[0]);
        const char* searchstr = mc_value_stringgetdata(args[1]);
        const char* replacestr = mc_value_stringgetdata(args[2]);

        size_t searchlen = strlen(searchstr);
        size_t replacelen = strlen(replacestr);

        const char* temp = strstr(str, searchstr);
        if(temp == NULL)
        {
            return mc_value_makestring(state, str);
        }

        /* Allocate new string to store result */
        size_t newlen = strlen(str) + (replacelen - searchlen) + 1;
        char* result = (char*)malloc(newlen + 1);
        if(result == NULL)
        {
            return mc_value_makenull();
        }

        /* Replace the first instance of searchstr with replacestr */
        size_t len = temp - str;
        memcpy(result, str, len);
        strcpy(result + len, replacestr);
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
    if(argc == 1 && args[0].type == MC_OBJ_STRING)
    {
        const char* str = mc_value_stringgetdata(args[0]);

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

        int i = 0;
        int j = length - 1;

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
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_STRING | MC_OBJ_ARRAY | MC_OBJ_MAP))
    {
        return mc_value_makenull();
    }

    mcvalue_t arg = args[0];
    mcobjtype_t type = arg.type;
    if(type == MC_OBJ_STRING)
    {
        int len = mc_value_stringgetlength(arg);
        return mc_value_makenumber(len);
    }
    if(type == MC_OBJ_ARRAY)
    {
        int len = mc_value_arraygetlength(arg);
        return mc_value_makenumber(len);
    }
    if(type == MC_OBJ_MAP)
    {
        int len = mc_value_mapgetlength(arg);
        return mc_value_makenumber(len);
    }

    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_first(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)state;
    (void)data;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ARRAY))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    return mc_value_arraygetvalueat(arg, 0);
}

TMPSTATIC mcvalue_t cfn_last(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)state;
    (void)argc;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ARRAY))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    return mc_value_arraygetvalueat(arg, mc_value_arraygetlength(arg) - 1);
}

TMPSTATIC mcvalue_t cfn_rest(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    (void)state;
    (void)argc;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ARRAY))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    int len = mc_value_arraygetlength(arg);
    if(len == 0)
    {
        return mc_value_makenull();
    }

    mcvalue_t res = object_make_array(state);
    if(object_is_null(res))
    {
        return mc_value_makenull();
    }
    for(i = 1; i < len; i++)
    {
        mcvalue_t item = mc_value_arraygetvalueat(arg, i);
        ok = mc_value_arraypush(res, item);
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
    int i;
    (void)state;
    (void)argc;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ARRAY | MC_OBJ_STRING))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    mcobjtype_t type = arg.type;
    if(type == MC_OBJ_ARRAY)
    {
        int len = mc_value_arraygetlength(arg);
        mcvalue_t res = object_make_array_with_capacity(state, len);
        if(object_is_null(res))
        {
            return mc_value_makenull();
        }
        for(i = 0; i < len; i++)
        {
            mcvalue_t obj = mc_value_arraygetvalueat(arg, i);
            ok = mc_value_arraysetvalueat(res, len - i - 1, obj);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return res;
    }
    if(type == MC_OBJ_STRING)
    {
        const char* str = mc_value_stringgetdata(arg);
        int len = mc_value_stringgetlength(arg);

        mcvalue_t res = mc_value_makestrcapacity(state, len);
        if(object_is_null(res))
        {
            return mc_value_makenull();
        }
        char* resbuf = mc_value_stringgetmutabledata(res);
        for(i = 0; i < len; i++)
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
    int i;
    (void)state;
    (void)argc;
    (void)data;
    if(argc == 1)
    {
        if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER))
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
        for(i = 0; i < capacity; i++)
        {
            ok = mc_value_arraypush(res, objnull);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return res;
    }
    if(argc == 2)
    {
        if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER, MC_OBJ_ANY))
        {
            return mc_value_makenull();
        }
        int capacity = (int)object_get_number(args[0]);
        mcvalue_t res = object_make_array_with_capacity(state, capacity);
        if(object_is_null(res))
        {
            return mc_value_makenull();
        }
        for(i = 0; i < capacity; i++)
        {
            ok = mc_value_arraypush(res, args[1]);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return res;
    }
    CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER);
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_arraypush(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    (void)state;
    (void)argc;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ARRAY, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    for(i=1; i<argc; i++)
    {
        ok = mc_value_arraypush(args[0], args[i]);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    int len = mc_value_arraygetlength(args[0]);
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

TMPSTATIC mcvalue_t cfn_vec2add(mcstate_t *state, void *data, int argc, mcvalue_t *args)
{
    mcvalue_t keyx;
    mcvalue_t keyy;
    (void)state;
    (void)argc;
    (void)data;
    if (!CHECK_ARGS(state, true, argc, args, MC_OBJ_MAP, MC_OBJ_MAP))
    {
        return mc_value_makenull();
    }
    keyx = mc_value_makestring(state, "x");
    keyy = mc_value_makestring(state, "y");
    double a_x = object_get_number(mc_value_mapgetvalue(args[0], keyx));
    double a_y = object_get_number(mc_value_mapgetvalue(args[0], keyy));

    double b_x = object_get_number(mc_value_mapgetvalue(args[1], keyx));
    double b_y = object_get_number(mc_value_mapgetvalue(args[1], keyy));

    mcvalue_t res = object_make_map(state);
    if (object_get_type(res) == MC_OBJ_NULL) {
        return res;
    }
    mc_value_mapsetvalue(res, keyx, mc_value_makenumber(a_x + b_x));
    mc_value_mapsetvalue(res, keyy, mc_value_makenumber(a_y + b_y));
    return res;
}

TMPSTATIC mcvalue_t cfn_vec2sub(mcstate_t *state, void *data, int argc, mcvalue_t *args)
{
    mcvalue_t keyx;
    mcvalue_t keyy;
    (void)state;
    (void)argc;
    (void)data;
    if (!CHECK_ARGS(state, true, argc, args, MC_OBJ_MAP, MC_OBJ_MAP)) {
        return mc_value_makenull();
    }
    keyx = mc_value_makestring(state, "x");
    keyy = mc_value_makestring(state, "y");
    double a_x = object_get_number(mc_value_mapgetvalue(args[0], keyx));
    double a_y = object_get_number(mc_value_mapgetvalue(args[0], keyy));

    double b_x = object_get_number(mc_value_mapgetvalue(args[1], keyx));
    double b_y = object_get_number(mc_value_mapgetvalue(args[1], keyy));

    mcvalue_t res = object_make_map(state);
    mc_value_mapsetvalue(res, keyx, mc_value_makenumber(a_x - b_x));
    mc_value_mapsetvalue(res, keyy, mc_value_makenumber(a_y - b_y));
    return res;
}

TMPSTATIC mcvalue_t cfn_test_check_args(mcstate_t* state, void *data, int argc, mcvalue_t *args)
{
    (void)state;
    (void)args;
    (void)argc;
    (void)data;
    if (!CHECK_ARGS(state, true, argc, args,
                  MC_OBJ_NUMBER,
                  MC_OBJ_ARRAY | MC_OBJ_MAP,
                  MC_OBJ_MAP,
                  MC_OBJ_STRING,
                  MC_OBJ_NUMBER | MC_OBJ_BOOL,
                  MC_OBJ_FUNCTION | MC_OBJ_NATIVE_FUNCTION,
                  MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makenumber(42);
}


TMPSTATIC mcvalue_t cfn_maketestdict(mcstate_t *state, void *data, int argc, mcvalue_t *args)
{
    int i;
    (void)data;
    if (argc != 1)
    {
        mc_state_setruntimeerrorf(state, "Invalid type passed to maketestdict, got %d, expected 1", argc);
        return mc_value_makenull();
    }    
    if (args[0].type != MC_OBJ_NUMBER)
    {
        const char *tname = mc_util_objtypename(args[0].type);
        mc_state_setruntimeerrorf(state, "Invalid type passed to maketestdict, got %s", tname);
        return mc_value_makenull();
    }
    int numitems = object_get_number(args[0]);
    mcvalue_t res = object_make_map(state);
    if (res.type == MC_OBJ_NULL)
    {
        return mc_value_makenull();
    }
    for (i = 0; i < numitems; i++)
    {
        char keybuf[64];
        sprintf(keybuf, "%d", i);
        mcvalue_t key = mc_value_makestring(state, keybuf);
        mcvalue_t val = mc_value_makenumber(i);
        mc_value_mapsetvalue(res, key, val);
    }
    return res;
}

TMPSTATIC mcvalue_t cfn_squarearray(mcstate_t *state, void *data, int argc, mcvalue_t *args)
{
    int i;
    double num;
    mcvalue_t res;
    mcvalue_t resitem;    
    (void)data;
    res = object_make_array_with_capacity(state, argc);

    for(i = 0; i < argc; i++)
    {
        if(object_get_type(args[i]) != MC_OBJ_NUMBER)
        {
            mc_state_setruntimeerrorf(state, "Invalid type passed to squarearray");
            return mc_value_makenull();
        }
        num = object_get_number(args[i]);
        resitem = mc_value_makenumber(num * num);
        mc_value_arraypush(res, resitem);
    }
    return res;
}

TMPSTATIC mcvalue_t cfn_print(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int i;
    mcvalue_t arg;
    mcprintstate_t pr;
    (void)data;
    mc_printer_init(&pr, state, 0, stdout, true);
    for(i = 0; i < argc; i++)
    {
        arg = args[i];
        mc_printer_printobject(arg, &pr, false);
    }
    mc_printer_destroy(&pr);
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_println(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcvalue_t o;
    o = cfn_print(state, data, argc, args);
    fputc('\n', stdout);
    fflush(stdout);
    return o;
}

TMPSTATIC mcvalue_t cfn_writefile(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)state;
    (void)argc;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_STRING, MC_OBJ_STRING))
    {
        return mc_value_makenull();
    }

    mcconfig_t* config = &state->config;

    if(!config->fileio.fnwritefile)
    {
        return mc_value_makenull();
    }

    const char* path = mc_value_stringgetdata(args[0]);
    const char* string = mc_value_stringgetdata(args[1]);
    int stringlen = mc_value_stringgetlength(args[1]);

    int written = config->fileio.fnwritefile(config->fileio.context, path, string, stringlen);

    return mc_value_makenumber(written);
}

TMPSTATIC mcvalue_t cfn_readfile(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)state;
    (void)argc;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_STRING))
    {
        return mc_value_makenull();
    }

    mcconfig_t* config = &state->config;

    if(!config->fileio.fnreadfile)
    {
        return mc_value_makenull();
    }

    const char* path = mc_value_stringgetdata(args[0]);

    char* contents = config->fileio.fnreadfile(config->fileio.context, path);
    if(!contents)
    {
        return mc_value_makenull();
    }
    mcvalue_t res = mc_value_makestring(state, contents);
    mc_allocator_free(state, contents);
    return res;
}

TMPSTATIC mcvalue_t cfn_tostring(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)state;
    (void)argc;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_STRING | MC_OBJ_NUMBER | MC_OBJ_BOOL | MC_OBJ_NULL | MC_OBJ_MAP | MC_OBJ_ARRAY))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    mcprintstate_t* buf = mc_printer_make(state, NULL);
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
    mcvalue_t res = mc_value_makestring(state, mc_printer_getstring(buf));
    mc_printer_destroy(buf);
    return res;
}

TMPSTATIC mcvalue_t cfn_tonum(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)state;
    (void)argc;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_STRING | MC_OBJ_NUMBER | MC_OBJ_BOOL | MC_OBJ_NULL))
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
    else if(args[0].type == MC_OBJ_STRING)
    {
        string = mc_value_stringgetdata(args[0]);
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
        int stringlen = mc_value_stringgetlength(args[0]);
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
    mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "Cannot convert \"%s\" to number", string);
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_chr(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)state;
    (void)argc;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER))
    {
        return mc_value_makenull();
    }

    double val = object_get_number(args[0]);

    char c = (char)val;
    char str[2] = { c, '\0' };
    return mc_value_makestring(state, str);
}

TMPSTATIC mcvalue_t cfn_range(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    (void)data;
    for(i = 0; i < argc; i++)
    {
        mcobjtype_t type = args[i].type;
        if(type != MC_OBJ_NUMBER)
        {
            const char* typestr = object_get_type_name(type);
            const char* expectedstr = object_get_type_name(MC_OBJ_NUMBER);
            mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "Invalid argument %d passed to range, got %s instead of %s", i, typestr, expectedstr);
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
        mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "Invalid number of arguments passed to range, got %d", argc);
        return mc_value_makenull();
    }

    if(step == 0)
    {
        mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "range step cannot be 0");
        return mc_value_makenull();
    }

    mcvalue_t res = object_make_array(state);
    if(object_is_null(res))
    {
        return mc_value_makenull();
    }
    for(i = start; i < end; i += step)
    {
        mcvalue_t item = mc_value_makenumber(i);
        ok = mc_value_arraypush(res, item);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

TMPSTATIC mcvalue_t cfn_keys(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    (void)state;
    (void)argc;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_MAP))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    mcvalue_t res = object_make_array(state);
    if(object_is_null(res))
    {
        return mc_value_makenull();
    }
    int len = mc_value_mapgetlength(arg);
    for(i = 0; i < len; i++)
    {
        mcvalue_t key = mc_value_mapgetkeyat(arg, i);
        ok = mc_value_arraypush(res, key);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

TMPSTATIC mcvalue_t cfn_values(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    (void)state;
    (void)argc;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_MAP))
    {
        return mc_value_makenull();
    }
    mcvalue_t arg = args[0];
    mcvalue_t res = object_make_array(state);
    if(object_is_null(res))
    {
        return mc_value_makenull();
    }
    int len = mc_value_mapgetlength(arg);
    for(i = 0; i < len; i++)
    {
        mcvalue_t key = mc_value_mapgetvalueat(arg, i);
        ok = mc_value_arraypush(res, key);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

TMPSTATIC mcvalue_t cfn_copy(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)state;
    (void)argc;
    (void)data;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_copyflat(state, args[0]);
}

TMPSTATIC mcvalue_t cfn_copydeep(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_copydeep(state, args[0]);
}

TMPSTATIC mcvalue_t cfn_concat(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ARRAY | MC_OBJ_STRING, MC_OBJ_ARRAY | MC_OBJ_STRING))
    {
        return mc_value_makenull();
    }
    mcobjtype_t type = args[0].type;
    mcobjtype_t itemtype = args[1].type;
    if(type == MC_OBJ_ARRAY)
    {
        if(itemtype != MC_OBJ_ARRAY)
        {
            const char* itemtypestr = object_get_type_name(itemtype);
            mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "Invalid argument 2 passed to concat, got %s", itemtypestr);
            return mc_value_makenull();
        }
        for(i = 0; i < mc_value_arraygetlength(args[1]); i++)
        {
            mcvalue_t item = mc_value_arraygetvalueat(args[1], i);
            ok = mc_value_arraypush(args[0], item);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return mc_value_makenumber(mc_value_arraygetlength(args[0]));
    }
    if(type == MC_OBJ_STRING)
    {
        if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_STRING, MC_OBJ_STRING))
        {
            return mc_value_makenull();
        }
        const char* leftval = mc_value_stringgetdata(args[0]);
        int leftlen = mc_value_stringgetlength(args[0]);

        const char* rightval = mc_value_stringgetdata(args[1]);
        int rightlen = mc_value_stringgetlength(args[1]);

        mcvalue_t res = mc_value_makestrcapacity(state, leftlen + rightlen);
        if(object_is_null(res))
        {
            return mc_value_makenull();
        }

        ok = mc_value_stringappend(res, leftval, leftlen);
        if(!ok)
        {
            return mc_value_makenull();
        }
        ok = mc_value_stringappend(res, rightval, rightlen);
        if(!ok)
        {
            return mc_value_makenull();
        }

        return res;
    }
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_remove(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int i;
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ARRAY, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }

    int ix = -1;
    for(i = 0; i < mc_value_arraygetlength(args[0]); i++)
    {
        mcvalue_t obj = mc_value_arraygetvalueat(args[0], i);
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

    bool res = mc_value_arrayremovevalueat(args[0], ix);
    return mc_value_makebool(res);
}

TMPSTATIC mcvalue_t cfn_removeat(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ARRAY, MC_OBJ_NUMBER))
    {
        return mc_value_makenull();
    }

    mcobjtype_t type = args[0].type;
    int ix = (int)object_get_number(args[1]);

    switch(type)
    {
        case MC_OBJ_ARRAY:
        {
            bool res = mc_value_arrayremovevalueat(args[0], ix);
            return mc_value_makebool(res);
        }
        default:
            break;
    }

    return mc_value_makebool(true);
}

TMPSTATIC mcvalue_t cfn_error(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(argc == 1 && args[0].type == MC_OBJ_STRING)
    {
        return object_make_error(state, mc_value_stringgetdata(args[0]));
    }
    return object_make_error(state, "");
}

TMPSTATIC mcvalue_t cfn_crash(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(argc == 1 && args[0].type == MC_OBJ_STRING)
    {
        mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(state->current_frame), mc_value_stringgetdata(args[0]));
    }
    else
    {
        mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(state->current_frame), "");
    }
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_assert(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_BOOL))
    {
        return mc_value_makenull();
    }

    if(!object_get_bool(args[0]))
    {
        mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "assertion failed");
        return mc_value_makenull();
    }

    return mc_value_makebool(true);
}

TMPSTATIC mcvalue_t cfn_randseed(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER))
    {
        return mc_value_makenull();
    }
    int seed = (int)object_get_number(args[0]);
    srand(seed);
    return mc_value_makebool(true);
}

TMPSTATIC mcvalue_t cfn_random(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    double res = (double)rand() / RAND_MAX;
    if(argc == 0)
    {
        return mc_value_makenumber(res);
    }
    if(argc == 2)
    {
        if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER, MC_OBJ_NUMBER))
        {
            return mc_value_makenull();
        }
        double min = object_get_number(args[0]);
        double max = object_get_number(args[1]);
        if(min >= max)
        {
            mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "max is bigger than min");
            return mc_value_makenull();
        }
        double range = max - min;
        res = min + (res * range);
        return mc_value_makenumber(res);
    }
    mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "Invalid number or arguments");
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_slice(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_STRING | MC_OBJ_ARRAY, MC_OBJ_NUMBER))
    {
        return mc_value_makenull();
    }
    mcobjtype_t argtype = args[0].type;
    int index = (int)object_get_number(args[1]);
    if(argtype == MC_OBJ_ARRAY)
    {
        int len = mc_value_arraygetlength(args[0]);
        if(index < 0)
        {
            index = len + index;
            if(index < 0)
            {
                index = 0;
            }
        }
        mcvalue_t res = object_make_array_with_capacity(state, len - index);
        if(object_is_null(res))
        {
            return mc_value_makenull();
        }
        for(i = index; i < len; i++)
        {
            mcvalue_t item = mc_value_arraygetvalueat(args[0], i);
            ok = mc_value_arraypush(res, item);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return res;
    }
    if(argtype == MC_OBJ_STRING)
    {
        const char* str = mc_value_stringgetdata(args[0]);
        int len = mc_value_stringgetlength(args[0]);
        if(index < 0)
        {
            index = len + index;
            if(index < 0)
            {
                return mc_value_makestring(state, "");
            }
        }
        if(index >= len)
        {
            return mc_value_makestring(state, "");
        }
        int reslen = len - index;
        mcvalue_t res = mc_value_makestrcapacity(state, reslen);
        if(object_is_null(res))
        {
            return mc_value_makenull();
        }

        char* resbuf = mc_value_stringgetmutabledata(res);
        memset(resbuf, 0, reslen + 1);
        for(i = index; i < len; i++)
        {
            char c = str[i];
            resbuf[i - index] = c;
        }
        object_set_string_length(res, reslen);
        return res;
    }
    const char* typestr = object_get_type_name(argtype);
    mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "Invalid argument 0 passed to slice, got %s instead", typestr);
    return mc_value_makenull();
}

TMPSTATIC mcvalue_t cfn_isstring(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == MC_OBJ_STRING);
}

TMPSTATIC mcvalue_t cfn_isarray(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == MC_OBJ_ARRAY);
}

TMPSTATIC mcvalue_t cfn_ismap(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == MC_OBJ_MAP);
}

TMPSTATIC mcvalue_t cfn_isnumber(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == MC_OBJ_NUMBER);
}

TMPSTATIC mcvalue_t cfn_isbool(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == MC_OBJ_BOOL);
}

TMPSTATIC mcvalue_t cfn_isnull(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(object_get_type(args[0]) == MC_OBJ_NULL);
}

TMPSTATIC mcvalue_t cfn_isfunction(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(object_get_type(args[0]) == MC_OBJ_FUNCTION);
}

TMPSTATIC mcvalue_t cfn_isexternal(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(object_get_type(args[0]) == MC_OBJ_EXTERNAL);
}

TMPSTATIC mcvalue_t cfn_iserror(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(object_get_type(args[0]) == MC_OBJ_ERROR);
}

TMPSTATIC mcvalue_t cfn_isnative_function(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(object_get_type(args[0]) == MC_OBJ_NATIVE_FUNCTION);
}

TMPSTATIC mcvalue_t cfn_sqrt(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = sqrt(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_pow(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER, MC_OBJ_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg1 = object_get_number(args[0]);
    double arg2 = object_get_number(args[1]);
    double res = pow(arg1, arg2);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_sin(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = sin(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_cos(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = cos(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_tan(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = tan(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_log(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = log(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_ceil(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = ceil(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_floor(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = floor(arg);
    return mc_value_makenumber(res);
}

TMPSTATIC mcvalue_t cfn_abs(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!CHECK_ARGS(state, true, argc, args, MC_OBJ_NUMBER))
    {
        return mc_value_makenull();
    }
    double arg = object_get_number(args[0]);
    double res = fabs(arg);
    return mc_value_makenumber(res);
}

static struct
{
    const char* name;
    mcnativeactualfn_t fn;
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
    { "binnot", cfn_binnot},
    { "ord", cfn_ord},
    { "println", cfn_println },
    { "print", cfn_print },
    { "read_file", cfn_readfile },
    { "write_file", cfn_writefile },
    { "first", cfn_first },
    { "last", cfn_last },
    { "rest", cfn_rest },
    { "arrayjoin", cfn_arrayjoin},
    { "arraypush", cfn_arraypush },
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

TMPSTATIC int builtins_count()
{
    return MC_UTIL_STATICARRAYSIZE(gnativefunctions);
}

TMPSTATIC mcnativeactualfn_t builtins_get_fn(int ix)
{
    return gnativefunctions[ix].fn;
}

TMPSTATIC const char* builtins_get_name(int ix)
{
    return gnativefunctions[ix].name;
}

TMPSTATIC void code_to_string(uint8_t* code, mcastlocation_t* sourcepositions, size_t codesize, mcprintstate_t* res)
{
    bool ok;
    int i;
    unsigned pos;
    pos = 0;
    while(pos < codesize)
    {
        uint8_t op = code[pos];
        mcopdefinition_t* def = opcode_lookup(op);
        MC_ASSERT(def);
        if(sourcepositions)
        {
            mcastlocation_t srcpos = sourcepositions[pos];
            mc_printer_printf(res, "%d:%-4d\t%04d\t%s", srcpos.line, srcpos.column, pos, def->name);
        }
        else
        {
            mc_printer_printf(res, "%04d %s", pos, def->name);
        }
        pos++;

        uint64_t operands[2];
        ok = code_read_operands(def, code + pos, operands);
        if(!ok)
        {
            return;
        }
        for(i = 0; i < def->num_operands; i++)
        {
            if(op == OPCODE_NUMBER)
            {
                double val_double = mc_util_uint64todouble(operands[i]);
                mc_printer_printf(res, " %1.17g", val_double);
            }
            else
            {
                mc_printer_printf(res, " %llu", operands[i]);
            }
            pos += def->operand_widths[i];
        }
        mc_printer_puts(res, "\n");
    }
}

TMPSTATIC bool code_read_operands(mcopdefinition_t* def, const uint8_t* instr, uint64_t outoperands[2])
{
    int i;
    int offset;
    int operandwidth;
    uint64_t operand;
    offset = 0;
    for(i = 0; i < def->num_operands; i++)
    {
        operandwidth = def->operand_widths[i];
        switch(operandwidth)
        {
            case 1:
            {
                outoperands[i] = instr[offset];
                break;
            }
            case 2:
            {
                operand = 0;
                operand = operand | ((uint64_t)instr[offset] << 8);
                operand = operand | ((uint64_t)instr[offset + 1]);
                outoperands[i] = operand;
                break;
            }
            case 4:
            {
                operand = 0;
                operand = operand | ((uint64_t)instr[offset + 0] << 24);
                operand = operand | ((uint64_t)instr[offset + 1] << 16);
                operand = operand | ((uint64_t)instr[offset + 2] << 8);
                operand = operand | ((uint64_t)instr[offset + 3]);
                outoperands[i] = operand;
                break;
            }
            case 8:
            {
                operand = 0;
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
                MC_ASSERT(false);
                return false;
            }
        }
        offset += operandwidth;
    }
    return true;
}



TMPSTATIC mcptrarray_t* kg_split_string(mcstate_t* state, const char* str, const char* delimiter)
{
    bool ok;
    int i;
    mcptrarray_t* res = mc_ptrarray_make(state);
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
        ok = mc_ptrarray_push(res, line);
        if(!ok)
        {
            mc_allocator_free(state, line);
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
    ok = mc_ptrarray_push(res, rest);
    if(!ok)
    {
        goto err;
    }
    return res;
err:
    mc_allocator_free(state, rest);
    if(res)
    {
        for(i = 0; i < mc_ptrarray_count(res); i++)
        {
            char* line = (char*)mc_ptrarray_get(res, i);
            mc_allocator_free(state, line);
        }
    }
    mc_ptrarray_destroy(res, NULL);
    return NULL;
}

TMPSTATIC char* kg_join(mcstate_t* state, mcptrarray_t* items, const char* with)
{
    int i;
    mcprintstate_t* res = mc_printer_make(state, NULL);
    if(!res)
    {
        return NULL;
    }
    for(i = 0; i < mc_ptrarray_count(items); i++)
    {
        char* item = (char*)mc_ptrarray_get(items, i);
        mc_printer_puts(res, item);
        if(i < (mc_ptrarray_count(items) - 1))
        {
            mc_printer_puts(res, with);
        }
    }
    return mc_printer_get_string_and_destroy(res);
}

TMPSTATIC char* kg_canonicalise_path(mcstate_t* state, const char* path)
{
    int i;
    if(!strchr(path, '/') || (!strstr(path, "/../") && !strstr(path, "./")))
    {
        return collections_strdup(state, path);
    }

    mcptrarray_t* split = kg_split_string(state, path, "/");
    if(!split)
    {
        return NULL;
    }

    for(i = 0; i < mc_ptrarray_count(split) - 1; i++)
    {
        char* item = (char*)mc_ptrarray_get(split, i);
        char* nextitem = (char*)mc_ptrarray_get(split, i + 1);
        if(kg_streq(item, "."))
        {
            mc_allocator_free(state, item);
            mc_ptrarray_removeat(split, i);
            i = -1;
            continue;
        }
        if(kg_streq(nextitem, ".."))
        {
            mc_allocator_free(state, item);
            mc_allocator_free(state, nextitem);
            mc_ptrarray_removeat(split, i);
            mc_ptrarray_removeat(split, i);
            i = -1;
        }
    }

    char* joined = kg_join(state, split, "/");
    for(i = 0; i < mc_ptrarray_count(split); i++)
    {
        void* item = mc_ptrarray_get(split, i);
        mc_allocator_free(state, item);
    }
    mc_ptrarray_destroy(split, NULL);
    return joined;
}

TMPSTATIC bool kg_is_path_absolute(const char* path)
{
    return path[0] == '/';
}

TMPSTATIC bool kg_streq(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
}


TMPSTATIC void mc_errlist_init(mcerrlist_t* errors)
{
    memset(errors, 0, sizeof(mcerrlist_t));
    errors->count = 0;
}

TMPSTATIC void mc_errlist_deinit(mcerrlist_t* errors)
{
    mc_errlist_clear(errors);
}

TMPSTATIC void mc_errlist_push(mcerrlist_t* errors, mcerrtype_t type, mcastlocation_t pos, const char* message)
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

TMPSEMISTATIC void mc_errlist_addf(mcerrlist_t* errors, mcerrtype_t type, mcastlocation_t pos, const char* format, ...)
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
    MC_ASSERT(towrite == written);
    va_end(args);
    mc_errlist_push(errors, type, pos, res);
}

TMPSTATIC void mc_errlist_clear(mcerrlist_t* errors)
{
    int i;
    for(i = 0; i < mc_errlist_size(errors); i++)
    {
        mcerror_t* error = mc_errlist_get(errors, i);
        if(error->traceback)
        {
            traceback_destroy(error->traceback);
        }
    }
    errors->count = 0;
}

TMPSTATIC int mc_errlist_size(mcerrlist_t* errors)
{
    return errors->count;
}

TMPSTATIC mcerror_t* mc_errlist_get(mcerrlist_t* errors, int ix)
{
    if(ix >= errors->count)
    {
        return NULL;
    }
    return &errors->errors[ix];
}


TMPUNUSED const char* error_type_to_string(mcerrtype_t type)
{
    switch(type)
    {
        case MC_ERROR_PARSING:
            return "PARSING";
        case MC_ERROR_COMPILING:
            return "COMPILATION";
        case MC_ERROR_RUNTIME:
            return "RUNTIME";
        case MC_ERROR_TIMEOUT:
            return "TIMEOUT";
        case MC_ERROR_MEMORY:
            return "ALLOCATION";
        case MC_ERROR_USER:
            return "USER";
        default:
            return "INVALID";
    }
}

TMPSTATIC mcerror_t* errors_get_last_error(mcerrlist_t* errors)
{
    if(errors->count <= 0)
    {
        return NULL;
    }
    return &errors->errors[errors->count - 1];
}

TMPSTATIC bool mc_errlist_haserrors(mcerrlist_t* errors)
{
    return mc_errlist_size(errors) > 0;
}

TMPSTATIC bool mc_callframe_init(mcvmframe_t* frame, mcvalue_t functionobj, int base_pointer)
{
    if(object_get_type(functionobj) != MC_OBJ_FUNCTION)
    {
        return false;
    }
    mcobjfuncscript_t* function = mc_value_functiongetscriptfunction(functionobj);
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

TMPSTATIC uint64_t mc_callframe_readuint64(mcvmframe_t* frame)
{
    uint64_t res;
    uint8_t* data;
    data = frame->bytecode + frame->ip;
    frame->ip += 8;
    res = 0;
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

TMPSTATIC uint16_t mc_callframe_readuint16(mcvmframe_t* frame)
{
    uint8_t* data;
    data = frame->bytecode + frame->ip;
    frame->ip += 2;
    return (data[0] << 8) | data[1];
}

TMPSTATIC uint8_t mc_callframe_readuint8(mcvmframe_t* frame)
{
    uint8_t* data;
    data = frame->bytecode + frame->ip;
    frame->ip++;
    return data[0];
}

TMPSTATIC mcopcode_t mc_callframe_readopcode(mcvmframe_t* frame)
{
    frame->src_ip = frame->ip;
    return (mcopcode_t)mc_callframe_readuint8(frame);
}

TMPSTATIC mcastlocation_t mc_callframe_getsourcelocation(mcvmframe_t* frame)
{
    if(frame->src_positions)
    {
        return frame->src_positions[frame->src_ip];
    }
    return srcposinvalid;
}

TMPSTATIC mcgcmemory_t* gcmem_make(mcstate_t* state)
{
    int i;
    mcgcmemory_t* mem = (mcgcmemory_t*)mc_allocator_alloc(state, sizeof(mcgcmemory_t));
    if(!mem)
    {
        return NULL;
    }
    memset(mem, 0, sizeof(mcgcmemory_t));
    mem->pstate = state;
    mem->gcobjlist = mc_ptrarray_make(state);
    if(!mem->gcobjlist)
    {
        goto error;
    }
    mem->gcobjlistback = mc_ptrarray_make(state);
    if(!mem->gcobjlistback)
    {
        goto error;
    }
    mem->gcobjlistremains = mc_basicarray_make(state, sizeof(mcvalue_t));
    if(!mem->gcobjlistremains)
    {
        goto error;
    }
    mem->allocssincesweep = 0;
    mem->onlydatapool.count = 0;

    for(i = 0; i < GCMEM_POOLS_NUM; i++)
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

TMPSTATIC void gcmem_destroy(mcgcmemory_t* mem)
{
    int i;
    int j;
    if(!mem)
    {
        return;
    }

    mc_basicarray_destroy(mem->gcobjlistremains);
    mc_ptrarray_destroy(mem->gcobjlistback, NULL);

    for(i = 0; i < mc_ptrarray_count(mem->gcobjlist); i++)
    {
        mcobjdata_t* obj = (mcobjdata_t*)mc_ptrarray_get(mem->gcobjlist, i);
        object_data_deinit(obj);
        mc_allocator_free(mem->pstate, obj);
    }
    mc_ptrarray_destroy(mem->gcobjlist, NULL);

    for(i = 0; i < GCMEM_POOLS_NUM; i++)
    {
        mcgcobjdatapool_t* pool = &mem->mempools[i];
        for(j = 0; j < pool->count; j++)
        {
            mcobjdata_t* data = pool->data[j];
            object_data_deinit(data);
            mc_allocator_free(mem->pstate, data);
        }
        memset(pool, 0, sizeof(mcgcobjdatapool_t));
    }

    for(i = 0; i < mem->onlydatapool.count; i++)
    {
        mc_allocator_free(mem->pstate, mem->onlydatapool.data[i]);
    }

    mc_allocator_free(mem->pstate, mem);
}

TMPSTATIC mcobjdata_t* gcmem_alloc_object_data(mcstate_t* state)
{
    bool ok;
    mcobjdata_t* data = NULL;
    state->mem->allocssincesweep++;
    if(state->mem->onlydatapool.count > 0)
    {
        data = state->mem->onlydatapool.data[state->mem->onlydatapool.count - 1];
        state->mem->onlydatapool.count--;
    }
    else
    {
        data = (mcobjdata_t*)mc_allocator_alloc(state, sizeof(mcobjdata_t));
        if(!data)
        {
            return NULL;
        }
    }
    memset(data, 0, sizeof(mcobjdata_t));
    data->pstate = state;
    MC_ASSERT(mc_ptrarray_count(state->mem->gcobjlistback) >= mc_ptrarray_count(state->mem->gcobjlist));
    /*
    * we want to make sure that appending to gcobjlistback never fails in sweep
    * so this only reserves space there.
    */
    ok = mc_ptrarray_push(state->mem->gcobjlistback, data);
    if(!ok)
    {
        mc_allocator_free(state, data);
        return NULL;
    }
    ok = mc_ptrarray_push(state->mem->gcobjlist, data);
    if(!ok)
    {
        mc_allocator_free(state, data);
        return NULL;
    }
    data->mem = state->mem;
    return data;
}

TMPSTATIC mcobjdata_t* gcmem_get_object_data_from_pool(mcstate_t* state, mcobjtype_t type)
{
    bool ok;
    mcgcobjdatapool_t* pool = get_pool_for_type(state->mem, type);
    if(!pool || pool->count <= 0)
    {
        return NULL;
    }
    mcobjdata_t* data = pool->data[pool->count - 1];

    MC_ASSERT(mc_ptrarray_count(state->mem->gcobjlistback) >= mc_ptrarray_count(state->mem->gcobjlist));
    /*
    * we want to make sure that appending to gcobjlistback never fails in sweep
    * so this only reserves space there.
    */
    ok = mc_ptrarray_push(state->mem->gcobjlistback, data);
    if(!ok)
    {
        return NULL;
    }
    ok = mc_ptrarray_push(state->mem->gcobjlist, data);
    if(!ok)
    {
        return NULL;
    }

    pool->count--;

    return data;
}

TMPSTATIC void gc_unmark_all(mcgcmemory_t* mem)
{
    int i;
    for(i = 0; i < mc_ptrarray_count(mem->gcobjlist); i++)
    {
        mcobjdata_t* data = (mcobjdata_t*)mc_ptrarray_get(mem->gcobjlist, i);
        data->gcmark = false;
    }
}

TMPSEMISTATIC void gc_mark_objects(mcvalue_t* objects, int count)
{
    int i;
    for(i = 0; i < count; i++)
    {
        mcvalue_t obj = objects[i];
        gc_mark_object(obj);
    }
}

TMPSEMISTATIC void gc_mark_object(mcvalue_t obj)
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
        case MC_OBJ_MAP:
        {
            int i;
            int len = mc_value_mapgetlength(obj);
            for(i = 0; i < len; i++)
            {
                mcvalue_t key = mc_value_mapgetkeyat(obj, i);
                if(object_is_allocated(key))
                {
                    mcobjdata_t* keydata = object_get_allocated_data(key);
                    if(!keydata->gcmark)
                    {
                        gc_mark_object(key);
                    }
                }
                mcvalue_t val = mc_value_mapgetvalueat(obj, i);
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
        case MC_OBJ_ARRAY:
        {
            int i;
            int len = mc_value_arraygetlength(obj);
            for(i = 0; i < len; i++)
            {
                mcvalue_t val = mc_value_arraygetvalueat(obj, i);
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
        case MC_OBJ_FUNCTION:
        {
            int i;
            mcobjfuncscript_t* function = mc_value_functiongetscriptfunction(obj);
            for(i = 0; i < function->free_vals_count; i++)
            {
                mcvalue_t freeval = mc_value_functiongetfreevalat(obj, i);
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

TMPSTATIC void gc_sweep(mcgcmemory_t* mem)
{
    bool ok;
    int i;
    gc_mark_objects((mcvalue_t*)mc_basicarray_data(mem->gcobjlistremains), mc_basicarray_count(mem->gcobjlistremains));

    MC_ASSERT(mc_ptrarray_count(mem->gcobjlistback) >= mc_ptrarray_count(mem->gcobjlist));

    mc_ptrarray_clear(mem->gcobjlistback);
    for(i = 0; i < mc_ptrarray_count(mem->gcobjlist); i++)
    {
        mcobjdata_t* data = (mcobjdata_t*)mc_ptrarray_get(mem->gcobjlist, i);
        if(data->gcmark)
        {
            /*
            * this should never fail because gcobjlistback's size should be equal to objects
            */
            ok = mc_ptrarray_push(mem->gcobjlistback, data);
            (void)ok;
            MC_ASSERT(ok);
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
                    mc_allocator_free(mem->pstate, data);
                }
            }
        }
    }
    mcptrarray_t* objstemp = mem->gcobjlist;
    mem->gcobjlist = mem->gcobjlistback;
    mem->gcobjlistback = objstemp;
    mem->allocssincesweep = 0;
}

TMPUNUSED bool gc_disable_on_object(mcvalue_t obj)
{
    bool ok;
    if(!object_is_allocated(obj))
    {
        return false;
    }
    mcobjdata_t* data = object_get_allocated_data(obj);
    if(mc_basicarray_contains(data->mem->gcobjlistremains, &obj))
    {
        return false;
    }
    ok = mc_basicarray_push(data->mem->gcobjlistremains, &obj);
    return ok;
}

TMPUNUSED void gc_enable_on_object(mcvalue_t obj)
{
    if(!object_is_allocated(obj))
    {
        return;
    }
    mcobjdata_t* data = object_get_allocated_data(obj);
    mc_basicarray_removeitem(data->mem->gcobjlistremains, &obj);
}

TMPSTATIC int gc_should_sweep(mcgcmemory_t* mem)
{
    return mem->allocssincesweep > GCMEM_SWEEP_INTERVAL;
}

TMPSTATIC mcgcobjdatapool_t* get_pool_for_type(mcgcmemory_t* mem, mcobjtype_t type)
{
    switch(type)
    {
        case MC_OBJ_ARRAY:
            return &mem->mempools[0];
        case MC_OBJ_MAP:
            return &mem->mempools[1];
        case MC_OBJ_STRING:
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
        case MC_OBJ_ARRAY:
        {
            if(mc_value_arraygetlength(obj) > 1024)
            {
                return false;
            }
            break;
        }
        case MC_OBJ_MAP:
        {
            if(mc_value_mapgetlength(obj) > 1024)
            {
                return false;
            }
            break;
        }
        case MC_OBJ_STRING:
        {
            if(!data->valstring.is_allocated || data->valstring.capacity > 4096)
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

TMPSTATIC mcglobalstore_t* global_store_make(mcstate_t* state)
{
    bool ok;
    int i;
    mcglobalstore_t* store = (mcglobalstore_t*)mc_allocator_alloc(state, sizeof(mcglobalstore_t));
    if(!store)
    {
        return NULL;
    }
    memset(store, 0, sizeof(mcglobalstore_t));
    store->pstate = state;
    store->symbols = mc_genericdict_make(state, (mcdictitemcopyfn_t)mc_symbol_copy, (mcdictitemdestroyfn_t)mc_symbol_destroy);
    if(!store->symbols)
    {
        goto err;
    }
    store->objects = mc_basicarray_make(state, sizeof(mcvalue_t));
    if(!store->objects)
    {
        goto err;
    }

    if(state->mem)
    {
        for(i = 0; i < builtins_count(); i++)
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

TMPSTATIC void global_store_destroy(mcglobalstore_t* store)
{
    if(!store)
    {
        return;
    }
    mc_genericdict_destroyitemsanddict(store->symbols);
    mc_basicarray_destroy(store->objects);
    mc_allocator_free(store->pstate, store);
}

TMPSTATIC mcastsymbol_t* global_store_get_symbol(mcglobalstore_t* store, const char* name)
{
    return (mcastsymbol_t*)mc_genericdict_get(store->symbols, name);
}


TMPSTATIC bool global_store_set(mcglobalstore_t* store, const char* name, mcvalue_t object)
{
    bool ok;
    mcastsymbol_t* existingsymbol = global_store_get_symbol(store, name);
    if(existingsymbol)
    {
        ok = mc_basicarray_set(store->objects, existingsymbol->index, &object);
        return ok;
    }
    int ix = mc_basicarray_count(store->objects);
    ok = mc_basicarray_push(store->objects, &object);
    if(!ok)
    {
        return false;
    }
    mcastsymbol_t* symbol = mc_symbol_make(store->pstate, name, SYMBOL_GLOBALBUILTIN, ix, false);
    if(!symbol)
    {
        goto err;
    }
    ok = mc_genericdict_set(store->symbols, name, symbol);
    if(!ok)
    {
        mc_symbol_destroy(symbol);
        goto err;
    }
    return true;
err:
    mc_basicarray_pop(store->objects, NULL);
    return false;
}

TMPSTATIC mcvalue_t global_store_get_object_at(mcglobalstore_t* store, int ix, bool* outok)
{
    mcvalue_t* res = (mcvalue_t*)mc_basicarray_get(store->objects, ix);
    if(!res)
    {
        *outok = false;
        return mc_value_makenull();
    }
    *outok = true;
    return *res;
}

TMPSTATIC mcvalue_t* global_store_get_object_data(mcglobalstore_t* store)
{
    return (mcvalue_t*)mc_basicarray_data(store->objects);
}

TMPSTATIC int global_store_get_object_count(mcglobalstore_t* store)
{
    return mc_basicarray_count(store->objects);
}

TMPSTATIC bool object_is_hashable(mcvalue_t obj)
{
    mcobjtype_t type = object_get_type(obj);
    switch(type)
    {
        case MC_OBJ_STRING:
            return true;
        case MC_OBJ_NUMBER:
            return true;
        case MC_OBJ_BOOL:
            return true;
        default:
            return false;
    }
}

TMPSEMISTATIC void mc_printer_printobject(mcvalue_t obj, mcprintstate_t* pr, bool quotestr)
{
    mcobjtype_t type = object_get_type(obj);
    switch(type)
    {
        case MC_OBJ_FREED:
        {
            mc_printer_puts(pr, "FREED");
            break;
        }
        case MC_OBJ_NONE:
        {
            mc_printer_puts(pr, "NONE");
            break;
        }
        case MC_OBJ_NUMBER:
        {
            double number = object_get_number(obj);
            mc_printer_printf(pr, "%1.10g", number);
            break;
        }
        case MC_OBJ_BOOL:
        {
            mc_printer_puts(pr, object_get_bool(obj) ? "true" : "false");
            break;
        }
        case MC_OBJ_STRING:
        {
            const char* string;
            #if 0
                string = mc_value_stringgetdata(obj);
            #else
                string = obj.objdatahandle->valstring.data;
            #endif
            if(quotestr)
            {
                mc_printer_printf(pr, "\"%s\"", string);
            }
            else
            {
                mc_printer_puts(pr, string);
            }
            break;
        }
        case MC_OBJ_NULL:
        {
            mc_printer_puts(pr, "null");
            break;
        }
        case MC_OBJ_FUNCTION:
        {
            mcobjfuncscript_t* function = mc_value_functiongetscriptfunction(obj);
            mc_printer_printf(pr, "CompiledFunction: %s\n", mc_value_functiongetname(obj));
            code_to_string(function->comp_result->bytecode, function->comp_result->src_positions, function->comp_result->count, pr);
            break;
        }
        case MC_OBJ_ARRAY:
        {
            size_t i;
            size_t alen;
            alen = mc_value_arraygetlength(obj);
            mc_printer_puts(pr, "[");
            for(i = 0; i < alen; i++)
            {
                mcvalue_t iobj = mc_value_arraygetvalueat(obj, i);
                mc_printer_printobject(iobj, pr, true);
                if(i < (alen - 1))
                {
                    mc_printer_puts(pr, ", ");
                }
            }
            mc_printer_puts(pr, "]");
            break;
        }
        case MC_OBJ_MAP:
        {
            size_t i;
            size_t alen;
            alen = mc_value_mapgetlength(obj);
            mc_printer_puts(pr, "{");
            for(i = 0; i < alen; i++)
            {
                mcvalue_t key = mc_value_mapgetkeyat(obj, i);
                mcvalue_t val = mc_value_mapgetvalueat(obj, i);
                mc_printer_printobject(key, pr, true);
                mc_printer_puts(pr, ": ");
                mc_printer_printobject(val, pr, true);
                if(i < (alen - 1))
                {
                    mc_printer_puts(pr, ", ");
                }
            }
            mc_printer_puts(pr, "}");
            break;
        }
        case MC_OBJ_NATIVE_FUNCTION:
        {
            mc_printer_puts(pr, "NATIVE_FUNCTION");
            break;
        }
        case MC_OBJ_EXTERNAL:
        {
            mc_printer_puts(pr, "EXTERNAL");
            break;
        }
        case MC_OBJ_ERROR:
        {
            mc_printer_printf(pr, "ERROR: %s\n", mc_value_errorgetmessage(obj));
            mctraceback_t* traceback = mc_value_errorgettraceback(obj);
            MC_ASSERT(traceback);
            if(traceback)
            {
                mc_printer_puts(pr, "Traceback:\n");
                traceback_to_string(traceback, pr);
            }
            break;
        }
        case MC_OBJ_ANY:
        {
            MC_ASSERT(false);
        }
    }
}

TMPSTATIC const char* object_get_type_name(mcobjtype_t type)
{
    switch(type)
    {
        case MC_OBJ_NONE:
            return "NONE";
        case MC_OBJ_FREED:
            return "NONE";
        case MC_OBJ_NUMBER:
            return "NUMBER";
        case MC_OBJ_BOOL:
            return "BOOL";
        case MC_OBJ_STRING:
            return "STRING";
        case MC_OBJ_NULL:
            return "NULL";
        case MC_OBJ_NATIVE_FUNCTION:
            return "NATIVE_FUNCTION";
        case MC_OBJ_ARRAY:
            return "ARRAY";
        case MC_OBJ_MAP:
            return "MAP";
        case MC_OBJ_FUNCTION:
            return "FUNCTION";
        case MC_OBJ_EXTERNAL:
            return "EXTERNAL";
        case MC_OBJ_ERROR:
            return "ERROR";
        case MC_OBJ_ANY:
            return "ANY";
    }
    return "NONE";
}

TMPSTATIC char* object_get_type_union_name(mcstate_t* state, mcobjtype_t type)
{
    if(type == MC_OBJ_ANY || type == MC_OBJ_NONE || type == MC_OBJ_FREED)
    {
        return mc_util_strdup(state, object_get_type_name(type));
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
                mc_printer_puts(res, "|");                 \
            }                                            \
            mc_printer_puts(res, object_get_type_name(t)); \
            inbetween = true;                           \
        }                                                \
    } while(0)

    CHECK_TYPE(MC_OBJ_NUMBER);
    CHECK_TYPE(MC_OBJ_BOOL);
    CHECK_TYPE(MC_OBJ_STRING);
    CHECK_TYPE(MC_OBJ_NULL);
    CHECK_TYPE(MC_OBJ_NATIVE_FUNCTION);
    CHECK_TYPE(MC_OBJ_ARRAY);
    CHECK_TYPE(MC_OBJ_MAP);
    CHECK_TYPE(MC_OBJ_FUNCTION);
    CHECK_TYPE(MC_OBJ_EXTERNAL);
    CHECK_TYPE(MC_OBJ_ERROR);

    return mc_printer_get_string_and_destroy(res);
}

TMPUNUSED char* object_serialize(mcstate_t* state, mcvalue_t object)
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

TMPSTATIC mcvalue_t mc_value_copydeep(mcstate_t* state, mcvalue_t obj)
{
    mcvaldict_t* copies = mc_valdict_make(state, sizeof(mcvalue_t), sizeof(mcvalue_t));
    if(!copies)
    {
        return mc_value_makenull();
    }
    mcvalue_t res = mc_value_copydeepintern(state, obj, copies);
    mc_valdict_destroy(copies);
    return res;
}

TMPSTATIC mcvalue_t mc_value_copyflat(mcstate_t* state, mcvalue_t obj)
{
    bool ok;
    mcvalue_t copy = mc_value_makenull();
    mcobjtype_t type = object_get_type(obj);
    switch(type)
    {
        case MC_OBJ_ANY:
        case MC_OBJ_FREED:
        case MC_OBJ_NONE:
        {
            MC_ASSERT(false);
            copy = mc_value_makenull();
            break;
        }
        case MC_OBJ_NUMBER:
        case MC_OBJ_BOOL:
        case MC_OBJ_NULL:
        case MC_OBJ_FUNCTION:
        case MC_OBJ_NATIVE_FUNCTION:
        case MC_OBJ_ERROR:
        {
            copy = obj;
            break;
        }
        case MC_OBJ_STRING:
        {
            const char* str = mc_value_stringgetdata(obj);
            copy = mc_value_makestring(state, str);
            break;
        }
        case MC_OBJ_ARRAY:
        {
            int i;
            int len = mc_value_arraygetlength(obj);
            copy = object_make_array_with_capacity(state, len);
            if(object_is_null(copy))
            {
                return mc_value_makenull();
            }
            for(i = 0; i < len; i++)
            {
                mcvalue_t item = mc_value_arraygetvalueat(obj, i);
                ok = mc_value_arraypush(copy, item);
                if(!ok)
                {
                    return mc_value_makenull();
                }
            }
            break;
        }
        case MC_OBJ_MAP:
        {
            int i;
            copy = object_make_map(state);
            for(i = 0; i < mc_value_mapgetlength(obj); i++)
            {
                mcvalue_t key = mc_value_mapgetkeyat(obj, i);
                mcvalue_t val = mc_value_mapgetvalueat(obj, i);
                ok = mc_value_mapsetvalue(copy, key, val);
                if(!ok)
                {
                    return mc_value_makenull();
                }
            }
            break;
        }
        case MC_OBJ_EXTERNAL:
        {
            copy = object_make_external(state, NULL);
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
            mc_value_setexternaldata(copy, datacopy);
            object_set_external_destroy_function(copy, external->data_destroy_fn);
            mc_value_setexternalcopyfunction(copy, external->data_copy_fn);
            break;
        }
    }
    return copy;
}

TMPSTATIC double mc_value_compare(mcvalue_t a, mcvalue_t b, bool* outok)
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

    if((atype == MC_OBJ_NUMBER || atype == MC_OBJ_BOOL || atype == MC_OBJ_NULL) && (btype == MC_OBJ_NUMBER || btype == MC_OBJ_BOOL || btype == MC_OBJ_NULL))
    {
        double leftval = object_get_number(a);
        double rightval = object_get_number(b);
        return leftval - rightval;
    }
    if(atype == btype && atype == MC_OBJ_STRING)
    {
        int alen = mc_value_stringgetlength(a);
        int blen = mc_value_stringgetlength(b);
        if(alen != blen)
        {
            return alen - blen;
        }
        unsigned long ahash = mc_value_stringgethash(a);
        unsigned long bhash = mc_value_stringgethash(b);
        if(ahash != bhash)
        {
            return ahash - bhash;
        }
        const char* astring = mc_value_stringgetdata(a);
        const char* bstring = mc_value_stringgetdata(b);
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

TMPSTATIC bool object_equals(mcvalue_t a, mcvalue_t b)
{
    bool ok;
    mcobjtype_t atype = object_get_type(a);
    mcobjtype_t btype = object_get_type(b);

    if(atype != btype)
    {
        return false;
    }
    ok = false;
    double res = mc_value_compare(a, b, &ok);
    return MC_UTIL_CMPFLOAT(res, 0);
}

TMPSTATIC mcobjexternal_t* object_get_external_data(mcvalue_t object)
{
    MC_ASSERT(object_get_type(object) == MC_OBJ_EXTERNAL);
    mcobjdata_t* data = object_get_allocated_data(object);
    return &data->external;
}

TMPSTATIC bool object_set_external_destroy_function(mcvalue_t object, mcextdatadestroyfn_t destroy_fn)
{
    MC_ASSERT(object_get_type(object) == MC_OBJ_EXTERNAL);
    mcobjexternal_t* data = object_get_external_data(object);
    if(!data)
    {
        return false;
    }
    data->data_destroy_fn = destroy_fn;
    return true;
}

TMPSTATIC mcobjdata_t* object_get_allocated_data(mcvalue_t object)
{
    MC_ASSERT(object_is_allocated(object) || object_get_type(object) == MC_OBJ_NULL);
    return object.objdatahandle;
}

TMPSTATIC bool object_get_bool(mcvalue_t obj)
{
    if(object_is_number(obj))
    {
        return obj.valnumber;
    }
    return obj.valbool;
}

TMPSTATIC double object_get_number(mcvalue_t obj)
{
    if(object_is_number(obj))
    {
        if(obj.type == MC_OBJ_BOOL)
        {
            return obj.valbool;
        }
        return obj.valnumber;
    }
    return obj.valnumber;
}

TMPSTATIC const char* mc_value_stringgetdata(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_STRING);
    data = object_get_allocated_data(object);
    return mc_value_stringgetdataintern(data);
}

TMPSTATIC int mc_value_stringgetlength(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_STRING);
    data = object_get_allocated_data(object);
    return data->valstring.length;
}

TMPSTATIC void object_set_string_length(mcvalue_t object, int len)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_STRING);
    data = object_get_allocated_data(object);
    data->valstring.length = len;
}

TMPUNUSED int mc_value_stringgetcapacity(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_STRING);
    data = object_get_allocated_data(object);
    return data->valstring.capacity;
}

TMPSTATIC char* mc_value_stringgetmutabledata(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_STRING);
    data = object_get_allocated_data(object);
    return mc_value_stringgetdataintern(data);
}

TMPSTATIC bool mc_value_stringappend(mcvalue_t obj, const char* src, int len)
{
    int capacity;
    int currentlen;
    char* strbuf;
    mcobjdata_t* data;
    mcobjstring_t* string;
    MC_ASSERT(object_get_type(obj) == MC_OBJ_STRING);
    data = object_get_allocated_data(obj);
    string = &data->valstring;
    strbuf = mc_value_stringgetmutabledata(obj);
    currentlen = string->length;
    capacity = string->capacity;
    if((len + currentlen) > capacity)
    {
        MC_ASSERT(false);
        return false;
    }
    memcpy(strbuf + currentlen, src, len);
    string->length += len;
    strbuf[string->length] = '\0';
    return true;
}

TMPSTATIC unsigned long mc_value_stringgethash(mcvalue_t obj)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(obj) == MC_OBJ_STRING);
    data = object_get_allocated_data(obj);
    if(data->valstring.hash == 0)
    {
        data->valstring.hash = object_hash_string(mc_value_stringgetdata(obj));
        if(data->valstring.hash == 0)
        {
            data->valstring.hash = 1;
        }
    }
    return data->valstring.hash;
}

TMPSTATIC mcobjfuncscript_t* mc_value_functiongetscriptfunction(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_FUNCTION);
    data = object_get_allocated_data(object);
    return &data->function;
}

TMPSTATIC mcobjfuncnative_t* mc_value_functiongetnativefunction(mcvalue_t obj)
{
    mcobjdata_t* data = object_get_allocated_data(obj);
    return &data->native_function;
}

TMPSTATIC const char* mc_value_functiongetname(mcvalue_t obj)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(obj) == MC_OBJ_FUNCTION);
    data = object_get_allocated_data(obj);
    MC_ASSERT(data);
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

TMPSTATIC mcvalue_t mc_value_functiongetfreevalat(mcvalue_t obj, int ix)
{
    mcobjdata_t* data;
    mcobjfuncscript_t* fun;
    MC_ASSERT(object_get_type(obj) == MC_OBJ_FUNCTION);
    data = object_get_allocated_data(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return mc_value_makenull();
    }
    fun = &data->function;
    MC_ASSERT(ix >= 0 && ix < fun->free_vals_count);
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

TMPSTATIC void mc_value_functionsetfreevalat(mcvalue_t obj, int ix, mcvalue_t val)
{
    mcobjdata_t* data;
    mcobjfuncscript_t* fun;
    MC_ASSERT(object_get_type(obj) == MC_OBJ_FUNCTION);
    data = object_get_allocated_data(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return;
    }
    fun = &data->function;
    MC_ASSERT(ix >= 0 && ix < fun->free_vals_count);
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

TMPUNUSED mcvalue_t* mc_value_functiongetfreevals(mcvalue_t obj)
{
    mcobjdata_t* data;
    mcobjfuncscript_t* fun;
    MC_ASSERT(object_get_type(obj) == MC_OBJ_FUNCTION);
    data = object_get_allocated_data(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return NULL;
    }
    fun = &data->function;
    if(freevals_are_allocated(fun))
    {
        return fun->free_vals_allocated;
    }
    return fun->free_vals_buf;
}

TMPSTATIC const char* mc_value_errorgetmessage(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_ERROR);
    data = object_get_allocated_data(object);
    return data->error.message;
}

TMPSTATIC void mc_value_errorsettraceback(mcvalue_t object, mctraceback_t* traceback)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_ERROR);
    if(object_get_type(object) != MC_OBJ_ERROR)
    {
        return;
    }
    data = object_get_allocated_data(object);
    MC_ASSERT(data->error.traceback == NULL);
    data->error.traceback = traceback;
}

TMPSTATIC mctraceback_t* mc_value_errorgettraceback(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_ERROR);
    data = object_get_allocated_data(object);
    return data->error.traceback;
}

TMPSTATIC bool mc_value_setexternaldata(mcvalue_t object, void* extdata)
{
    mcobjexternal_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_EXTERNAL);
    data = object_get_external_data(object);
    if(!data)
    {
        return false;
    }
    data->data = extdata;
    return true;
}

TMPSTATIC bool mc_value_setexternalcopyfunction(mcvalue_t object, mcextdatacopyfn_t copyfn)
{
    mcobjexternal_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_EXTERNAL);
    data = object_get_external_data(object);
    if(!data)
    {
        return false;
    }
    data->data_copy_fn = copyfn;
    return true;
}

TMPSTATIC mcvalue_t mc_value_arraygetvalueat(mcvalue_t object, int ix)
{
    mcvalue_t* res;
    mcbasicarray_t* array;
    MC_ASSERT(object_get_type(object) == MC_OBJ_ARRAY);
    array = object_get_allocated_array(object);
    if(ix < 0 || ix >= mc_basicarray_count(array))
    {
        return mc_value_makenull();
    }
    res = (mcvalue_t*)mc_basicarray_get(array, ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

TMPSTATIC bool mc_value_arraysetvalueat(mcvalue_t object, int ix, mcvalue_t val)
{
    int len;
    int toadd;
    mcbasicarray_t* array;
    MC_ASSERT(object_get_type(object) == MC_OBJ_ARRAY);
    array = object_get_allocated_array(object);
    len = mc_basicarray_count(array);
    if(((ix < 0) || (ix >= len)) || (len == 0))
    {
        toadd = len+1;
        //fprintf(stderr, "ix=%d toadd=%d len=%d\n", ix, toadd, len);
        while(toadd != (ix+2))
        {
            mc_value_arraypush(object, mc_value_makenull());
            toadd++;
        }
    }
    return mc_basicarray_set(array, ix, &val);
}

TMPSTATIC bool mc_value_arraypush(mcvalue_t object, mcvalue_t val)
{
    mcbasicarray_t* array;
    MC_ASSERT(object_get_type(object) == MC_OBJ_ARRAY);
    array = object_get_allocated_array(object);
    return mc_basicarray_push(array, &val);
}

TMPSTATIC int mc_value_arraygetlength(mcvalue_t object)
{
    mcbasicarray_t* array;
    MC_ASSERT(object_get_type(object) == MC_OBJ_ARRAY);
    array = object_get_allocated_array(object);
    return mc_basicarray_count(array);
}

TMPSTATIC bool mc_value_arrayremovevalueat(mcvalue_t object, int ix)
{
    mcbasicarray_t* array;
    array = object_get_allocated_array(object);
    return mc_basicarray_removeat(array, ix);
}

TMPSTATIC int mc_value_mapgetlength(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_MAP);
    data = object_get_allocated_data(object);
    return mc_valdict_count(data->map);
}

TMPSTATIC mcvalue_t mc_value_mapgetkeyat(mcvalue_t object, int ix)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_MAP);
    data = object_get_allocated_data(object);
    res = (mcvalue_t*)mc_valdict_getkeyat(data->map, ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

TMPSTATIC mcvalue_t mc_value_mapgetvalueat(mcvalue_t object, int ix)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_MAP);
    data = object_get_allocated_data(object);
    res = (mcvalue_t*)mc_valdict_getvalueat(data->map, ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

TMPUNUSED bool mc_value_mapsetvalueat(mcvalue_t object, int ix, mcvalue_t val)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_MAP);
    if(ix >= mc_value_mapgetlength(object))
    {
        return false;
    }
    data = object_get_allocated_data(object);
    return mc_valdict_setvalueat(data->map, ix, &val);
}

TMPSTATIC mcvalue_t mc_value_mapgetkvpairat(mcstate_t* state, mcvalue_t object, int ix)
{
    mcvalue_t key;
    mcvalue_t val;
    mcvalue_t res;
    mcvalue_t valobj;
    mcvalue_t keyobj;
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_MAP);
    data = object_get_allocated_data(object);
    if(ix >= mc_valdict_count(data->map))
    {
        return mc_value_makenull();
    }
    key = mc_value_mapgetkeyat(object, ix);
    val = mc_value_mapgetvalueat(object, ix);
    res = object_make_map(state);
    if(object_is_null(res))
    {
        return mc_value_makenull();
    }
    keyobj = mc_value_makestring(state, "key");
    if(object_is_null(keyobj))
    {
        return mc_value_makenull();
    }
    mc_value_mapsetvalue(res, keyobj, key);
    valobj = mc_value_makestring(state, "value");
    if(object_is_null(valobj))
    {
        return mc_value_makenull();
    }
    mc_value_mapsetvalue(res, valobj, val);
    return res;
}

TMPSTATIC bool mc_value_mapsetvalue(mcvalue_t object, mcvalue_t key, mcvalue_t val)
{
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_MAP);
    data = object_get_allocated_data(object);
    return mc_valdict_setkv(data->map, &key, &val);
}

TMPSTATIC mcvalue_t mc_value_mapgetvalue(mcvalue_t object, mcvalue_t key)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_MAP);
    data = object_get_allocated_data(object);
    res = (mcvalue_t*)mc_valdict_get(data->map, &key);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

TMPUNUSED bool mc_value_maphaskey(mcvalue_t object, mcvalue_t key)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(object_get_type(object) == MC_OBJ_MAP);
    data = object_get_allocated_data(object);
    res = (mcvalue_t*)mc_valdict_get(data->map, &key);
    return res != NULL;
}

TMPSEMISTATIC mcvalue_t mc_value_copydeepfuncscript(mcstate_t* state, mcvalue_t obj, mcvaldict_t* copies)
{
    bool ok;
    int i;
    uint8_t* bytecodecopy;
    mcobjfuncscript_t* functioncopy;
    mcvalue_t copy;
    mcvalue_t freeval;
    mcvalue_t freevalcopy;
    mcobjfuncscript_t* function;
    mcastlocation_t* srcpositionscopy;
    compilation_result_t* comprescopy;
    function = mc_value_functiongetscriptfunction(obj);
    bytecodecopy = NULL;
    srcpositionscopy = NULL;
    comprescopy = NULL;
    bytecodecopy = (uint8_t*)mc_allocator_alloc(state, sizeof(uint8_t) * function->comp_result->count);
    if(!bytecodecopy)
    {
        return mc_value_makenull();
    }
    memcpy(bytecodecopy, function->comp_result->bytecode, sizeof(uint8_t) * function->comp_result->count);
    srcpositionscopy = (mcastlocation_t*)mc_allocator_alloc(state, sizeof(mcastlocation_t) * function->comp_result->count);
    if(!srcpositionscopy)
    {
        mc_allocator_free(state, bytecodecopy);
        return mc_value_makenull();
    }
    memcpy(srcpositionscopy, function->comp_result->src_positions, sizeof(mcastlocation_t) * function->comp_result->count);
    comprescopy = compilation_result_make(state, bytecodecopy, srcpositionscopy, function->comp_result->count);
    /*
    * todo: add compilation result copy function
    */
    if(!comprescopy)
    {
        mc_allocator_free(state, srcpositionscopy);
        mc_allocator_free(state, bytecodecopy);
        return mc_value_makenull();
    }
    copy = object_make_function(state, mc_value_functiongetname(obj), comprescopy, true, function->num_locals, function->num_args, 0);
    if(object_is_null(copy))
    {
        compilation_result_destroy(comprescopy);
        return mc_value_makenull();
    }
    ok = mc_valdict_setkv(copies, &obj, &copy);
    if(!ok)
    {
        return mc_value_makenull();
    }
    functioncopy = mc_value_functiongetscriptfunction(copy);
    if(freevals_are_allocated(function))
    {
        functioncopy->free_vals_allocated = (mcvalue_t*)mc_allocator_alloc(state, sizeof(mcvalue_t) * function->free_vals_count);
        if(!functioncopy->free_vals_allocated)
        {
            return mc_value_makenull();
        }
    }
    functioncopy->free_vals_count = function->free_vals_count;
    for(i = 0; i < function->free_vals_count; i++)
    {
        freeval = mc_value_functiongetfreevalat(obj, i);
        freevalcopy = mc_value_copydeepintern(state, freeval, copies);
        if(!object_is_null(freeval) && object_is_null(freevalcopy))
        {
            return mc_value_makenull();
        }
        mc_value_functionsetfreevalat(copy, i, freevalcopy);
    }
    return copy;
}

TMPSEMISTATIC mcvalue_t mc_value_copydeeparray(mcstate_t* state, mcvalue_t obj, mcvaldict_t* copies)
{
    bool ok;
    int i;
    int len;
    mcvalue_t copy;
    mcvalue_t item;
    mcvalue_t itemcopy;
    len = mc_value_arraygetlength(obj);
    copy = object_make_array_with_capacity(state, len);
    if(object_is_null(copy))
    {
        return mc_value_makenull();
    }
    ok = mc_valdict_setkv(copies, &obj, &copy);
    if(!ok)
    {
        return mc_value_makenull();
    }
    for(i = 0; i < len; i++)
    {
        item = mc_value_arraygetvalueat(obj, i);
        itemcopy = mc_value_copydeepintern(state, item, copies);
        if(!object_is_null(item) && object_is_null(itemcopy))
        {
            return mc_value_makenull();
        }
        ok = mc_value_arraypush(copy, itemcopy);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return copy;
}

TMPSEMISTATIC mcvalue_t mc_value_copydeepmap(mcstate_t* state, mcvalue_t obj, mcvaldict_t* copies)
{
    bool ok;
    int i;
    mcvalue_t key;
    mcvalue_t val;
    mcvalue_t copy;
    mcvalue_t keycopy;
    mcvalue_t valcopy;
    copy = object_make_map(state);
    if(object_is_null(copy))
    {
        return mc_value_makenull();
    }
    ok = mc_valdict_setkv(copies, &obj, &copy);
    if(!ok)
    {
        return mc_value_makenull();
    }
    for(i = 0; i < mc_value_mapgetlength(obj); i++)
    {
        key = mc_value_mapgetkeyat(obj, i);
        val = mc_value_mapgetvalueat(obj, i);
        keycopy = mc_value_copydeepintern(state, key, copies);
        if(!object_is_null(key) && object_is_null(keycopy))
        {
            return mc_value_makenull();
        }
        valcopy = mc_value_copydeepintern(state, val, copies);
        if(!object_is_null(val) && object_is_null(valcopy))
        {
            return mc_value_makenull();
        }
        ok = mc_value_mapsetvalue(copy, keycopy, valcopy);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return copy;
}

TMPSEMISTATIC mcvalue_t mc_value_copydeepintern(mcstate_t* state, mcvalue_t obj, mcvaldict_t* copies)
{
    bool ok;
    mcobjtype_t type;
    mcvalue_t copy;
    mcvalue_t* copyptr;
    copyptr = (mcvalue_t*)mc_valdict_get(copies, &obj);
    if(copyptr)
    {
        return *copyptr;
    }
    copy = mc_value_makenull();
    type = object_get_type(obj);
    switch(type)
    {
        case MC_OBJ_FREED:
        case MC_OBJ_ANY:
        case MC_OBJ_NONE:
        {
            MC_ASSERT(false);
            copy = mc_value_makenull();
            break;
        }
        case MC_OBJ_NUMBER:
        case MC_OBJ_BOOL:
        case MC_OBJ_NULL:
        case MC_OBJ_NATIVE_FUNCTION:
            {
                copy = obj;
            }
            break;
        case MC_OBJ_STRING:
            {
                int len;
                const char* str;
                str = mc_value_stringgetdata(obj);
                len = mc_value_stringgetlength(obj);
                copy = mc_value_makestringlen(state, str, len);
                return copy;
            }
            break;
        case MC_OBJ_FUNCTION:
            {
                return mc_value_copydeepfuncscript(state, obj, copies);
            }
            break;
        case MC_OBJ_ARRAY:
            {
                return mc_value_copydeeparray(state, obj, copies);
            }
            break;
        case MC_OBJ_MAP:
            {
                return mc_value_copydeepmap(state, obj, copies);
            }
            break;
        case MC_OBJ_EXTERNAL:
            {
                copy = mc_value_copyflat(state, obj);
            }
            break;
        case MC_OBJ_ERROR:
            {
                copy = obj;
            }
            break;
    }
    return copy;
}

TMPSTATIC bool object_equals_wrapped(mcvalue_t* aptr, mcvalue_t* bptr)
{
    mcvalue_t a;
    mcvalue_t b;
    a = *aptr;
    b = *bptr;
    return object_equals(a, b);
}

TMPSTATIC unsigned long object_hash(mcvalue_t* objptr)
{
    bool bval;
    double dval;
    mcvalue_t obj;
    mcobjtype_t type;
    obj = *objptr;
    type = object_get_type(obj);
    switch(type)
    {
        case MC_OBJ_NUMBER:
            {
                dval = object_get_number(obj);
                return object_hash_double(dval);
            }
            break;
        case MC_OBJ_BOOL:
            {
                bval = object_get_bool(obj);
                return bval;
            }
            break;
        case MC_OBJ_STRING:
            {
                return mc_value_stringgethash(obj);
            }
            break;
        default:
            {
            }
            break;
    }
    return 0;
}

TMPSTATIC unsigned long object_hash_string(const char* str)
{
    /* djb2 */
    int c;
    unsigned long hash;
    hash = 5381;
    while((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

TMPSTATIC unsigned long object_hash_double(double val)
{
    /* djb2 */
    unsigned long hash;
    uint32_t* valptr;
    valptr = (uint32_t*)&val;
    hash = 5381;
    hash = ((hash << 5) + hash) + valptr[0];
    hash = ((hash << 5) + hash) + valptr[1];
    return hash;
}

TMPSTATIC mcbasicarray_t* object_get_allocated_array(mcvalue_t object)
{
    MC_ASSERT(object_get_type(object) == MC_OBJ_ARRAY);
    mcobjdata_t* data = object_get_allocated_data(object);
    return data->array;
}

TMPSTATIC bool object_is_number(mcvalue_t o)
{
    return (o.type == MC_OBJ_NUMBER || o.type == MC_OBJ_BOOL);
}

TMPSTATIC bool freevals_are_allocated(mcobjfuncscript_t* fun)
{
    return fun->free_vals_count >= MC_UTIL_STATICARRAYSIZE(fun->free_vals_buf);
}

TMPSTATIC char* mc_value_stringgetdataintern(mcobjdata_t* data)
{
    MC_ASSERT(data->type == MC_OBJ_STRING);
    if(data->valstring.is_allocated)
    {
        return data->valstring.actualallocated;
    }
    return data->valstring.actualonstack;
}

TMPSTATIC bool mc_value_stringreservecapacity(mcobjdata_t* data, int capacity)
{
    char* newvalue;
    mcobjstring_t* string;
    MC_ASSERT(capacity >= 0);
    string = &data->valstring;
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
            MC_ASSERT(false);
            /* just in case */
            mc_allocator_free(data->pstate, string->actualallocated);
        }
        string->capacity = MC_CONF_OBJECT_STRING_BUF_SIZE - 1;
        string->is_allocated = false;
        string->data = string->actualonstack;
        return true;
    }
    newvalue = (char*)mc_allocator_alloc(data->pstate, capacity + 1);
    if(!newvalue)
    {
        return false;
    }
    if(string->is_allocated)
    {
        mc_allocator_free(data->pstate, string->actualallocated);
    }
    string->actualallocated = newvalue;
    string->is_allocated = true;
    string->capacity = capacity;
    string->data = string->actualallocated;
    return true;
}

TMPSTATIC mcastsymbol_t* mc_symbol_make(mcstate_t* state, const char* name, mcastsymtype_t type, int index, bool assignable)
{
    mcastsymbol_t* symbol;
    symbol = (mcastsymbol_t*)mc_allocator_alloc(state, sizeof(mcastsymbol_t));
    if(!symbol)
    {
        return NULL;
    }
    memset(symbol, 0, sizeof(mcastsymbol_t));
    symbol->pstate = state;
    symbol->name = mc_util_strdup(state, name);
    if(!symbol->name)
    {
        mc_allocator_free(state, symbol);
        return NULL;
    }
    symbol->type = type;
    symbol->index = index;
    symbol->assignable = assignable;
    return symbol;
}

TMPSTATIC void mc_symbol_destroy(mcastsymbol_t* symbol)
{
    if(!symbol)
    {
        return;
    }
    mc_allocator_free(symbol->pstate, symbol->name);
    mc_allocator_free(symbol->pstate, symbol);
}

TMPSTATIC mcastsymbol_t* mc_symbol_copy(mcastsymbol_t* symbol)
{
    return mc_symbol_make(symbol->pstate, symbol->name, symbol->type, symbol->index, symbol->assignable);
}

TMPSTATIC mcastsymtable_t* mc_symtable_make(mcstate_t* state, mcastsymtable_t* outer, mcglobalstore_t* global_store, int module_global_offset)
{
    bool ok;
    mcastsymtable_t* table;
    table = (mcastsymtable_t*)mc_allocator_alloc(state, sizeof(mcastsymtable_t));
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
    table->block_scopes = mc_ptrarray_make(state);
    if(!table->block_scopes)
    {
        goto err;
    }
    table->free_symbols = mc_ptrarray_make(state);
    if(!table->free_symbols)
    {
        goto err;
    }
    table->module_global_symbols = mc_ptrarray_make(state);
    if(!table->module_global_symbols)
    {
        goto err;
    }
    ok = mc_symtable_pushblockscope(table);
    if(!ok)
    {
        goto err;
    }
    return table;
err:
    mc_symtable_destroy(table);
    return NULL;
}

TMPSTATIC void mc_symtable_destroy(mcastsymtable_t* table)
{
    mcstate_t* state;
    if(!table)
    {
        return;
    }
    while(mc_ptrarray_count(table->block_scopes) > 0)
    {
        mc_symtable_popblockscope(table);
    }
    mc_ptrarray_destroy(table->block_scopes, NULL);
    mc_ptrarray_destroy(table->module_global_symbols, (mcptrarrayitemdestroyfn_t)mc_symbol_destroy);
    mc_ptrarray_destroy(table->free_symbols, (mcptrarrayitemdestroyfn_t)mc_symbol_destroy);
    state = table->pstate;
    memset(table, 0, sizeof(mcastsymtable_t));
    mc_allocator_free(state, table);
}

TMPSTATIC mcastsymtable_t* mc_symtable_copy(mcastsymtable_t* table)
{
    mcastsymtable_t* copy;
    copy = (mcastsymtable_t*)mc_allocator_alloc(table->pstate, sizeof(mcastsymtable_t));
    if(!copy)
    {
        return NULL;
    }
    memset(copy, 0, sizeof(mcastsymtable_t));
    copy->pstate = table->pstate;
    copy->outer = table->outer;
    copy->global_store = table->global_store;
    copy->block_scopes = mc_ptrarray_copy(table->block_scopes, (mcptrarrayitemcopyfn_t)block_scope_copy, (mcptrarrayitemdestroyfn_t)block_scope_destroy);
    if(!copy->block_scopes)
    {
        goto err;
    }
    copy->free_symbols = mc_ptrarray_copy(table->free_symbols, (mcptrarrayitemcopyfn_t)mc_symbol_copy, (mcptrarrayitemdestroyfn_t)mc_symbol_destroy);
    if(!copy->free_symbols)
    {
        goto err;
    }
    copy->module_global_symbols = mc_ptrarray_copy(table->module_global_symbols, (mcptrarrayitemcopyfn_t)mc_symbol_copy, (mcptrarrayitemdestroyfn_t)mc_symbol_destroy);
    if(!copy->module_global_symbols)
    {
        goto err;
    }
    copy->max_num_definitions = table->max_num_definitions;
    copy->module_global_offset = table->module_global_offset;
    return copy;
err:
    mc_symtable_destroy(copy);
    return NULL;
}

TMPSTATIC bool mc_symtable_addmodsymbol(mcastsymtable_t* st, mcastsymbol_t* symbol)
{
    bool ok;
    mcastsymbol_t* copy;
    if(symbol->type != SYMBOL_MODULE_GLOBAL)
    {
        MC_ASSERT(false);
        return false;
    }
    if(mc_symtable_isdefined(st, symbol->name))
    {
        /* todo: make sure it should be true in this case */
        return true;
    }
    copy = mc_symbol_copy(symbol);
    if(!copy)
    {
        return false;
    }
    ok = mc_symtable_setsymbol(st, copy);
    if(!ok)
    {
        mc_symbol_destroy(copy);
        return false;
    }
    return true;
}

TMPSTATIC mcastsymbol_t* mc_symtable_define(mcastsymtable_t* table, const char* name, bool assignable)
{
    bool ok;
    bool globalsymboladded;
    int ix;
    int definitionscount;
    mcastsymtype_t symboltype;
    mcastsymbol_t* symbol;
    mcastscopeblock_t* topscope;
    mcastsymbol_t* globalsymbol;
    mcastsymbol_t* globalsymbolcopy;
    globalsymbol = global_store_get_symbol(table->global_store, name);
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
    if(MC_UTIL_STREQ(name, "this"))
    {
        return NULL;
    }
    symboltype = table->outer == NULL ? SYMBOL_MODULE_GLOBAL : SYMBOL_LOCAL;
    ix = mc_symtable_nextsymindex(table);
    symbol = mc_symbol_make(table->pstate, name, symboltype, ix, assignable);
    if(!symbol)
    {
        return NULL;
    }
    globalsymboladded = false;
    ok = false;
    if(symboltype == SYMBOL_MODULE_GLOBAL && mc_ptrarray_count(table->block_scopes) == 1)
    {
        globalsymbolcopy = mc_symbol_copy(symbol);
        if(!globalsymbolcopy)
        {
            mc_symbol_destroy(symbol);
            return NULL;
        }
        ok = mc_ptrarray_push(table->module_global_symbols, globalsymbolcopy);
        if(!ok)
        {
            mc_symbol_destroy(globalsymbolcopy);
            mc_symbol_destroy(symbol);
            return NULL;
        }
        globalsymboladded = true;
    }
    ok = mc_symtable_setsymbol(table, symbol);
    if(!ok)
    {
        if(globalsymboladded)
        {
            globalsymbolcopy = (mcastsymbol_t*)mc_ptrarray_pop(table->module_global_symbols);
            mc_symbol_destroy(globalsymbolcopy);
        }
        mc_symbol_destroy(symbol);
        return NULL;
    }
    topscope = (mcastscopeblock_t*)mc_ptrarray_top(table->block_scopes);
    topscope->num_definitions++;
    definitionscount = mc_symtable_getnumdefs(table);
    if(definitionscount > table->max_num_definitions)
    {
        table->max_num_definitions = definitionscount;
    }
    return symbol;
}

TMPSTATIC mcastsymbol_t* mc_symtable_defineanddestroyold(mcastsymtable_t* st, mcastsymbol_t* original)
{
    bool ok;
    mcastsymbol_t* copy;
    mcastsymbol_t* symbol;
    copy = mc_symbol_make(st->pstate, original->name, original->type, original->index, original->assignable);
    if(!copy)
    {
        return NULL;
    }
    ok = mc_ptrarray_push(st->free_symbols, copy);
    if(!ok)
    {
        mc_symbol_destroy(copy);
        return NULL;
    }
    symbol = mc_symbol_make(st->pstate, original->name, SYMBOL_FREE, mc_ptrarray_count(st->free_symbols) - 1, original->assignable);
    if(!symbol)
    {
        return NULL;
    }
    ok = mc_symtable_setsymbol(st, symbol);
    if(!ok)
    {
        mc_symbol_destroy(symbol);
        return NULL;
    }
    return symbol;
}

TMPSTATIC mcastsymbol_t* mc_symtable_definefunctionname(mcastsymtable_t* st, const char* name, bool assignable)
{
    bool ok;
    mcastsymbol_t* symbol;
    /* module symbol */
    if(strchr(name, ':'))
    {
        return NULL;
    }
    symbol = mc_symbol_make(st->pstate, name, SYMBOL_FUNCTION, 0, assignable);
    if(!symbol)
    {
        return NULL;
    }
    ok = mc_symtable_setsymbol(st, symbol);
    if(!ok)
    {
        mc_symbol_destroy(symbol);
        return NULL;
    }
    return symbol;
}

TMPSTATIC mcastsymbol_t* mc_symtable_definethis(mcastsymtable_t* st)
{
    bool ok;
    mcastsymbol_t* symbol;
    symbol = mc_symbol_make(st->pstate, "this", SYMBOL_THIS, 0, false);
    if(!symbol)
    {
        return NULL;
    }
    ok = mc_symtable_setsymbol(st, symbol);
    if(!ok)
    {
        mc_symbol_destroy(symbol);
        return NULL;
    }
    return symbol;
}

TMPSEMISTATIC mcastsymbol_t* mc_symtable_resolve(mcastsymtable_t* table, const char* name)
{
    int i;
    mcastsymbol_t* symbol;
    mcastscopeblock_t* scope;
    symbol = NULL;
    scope = NULL;
    symbol = global_store_get_symbol(table->global_store, name);
    if(symbol)
    {
        return symbol;
    }

    for(i = mc_ptrarray_count(table->block_scopes) - 1; i >= 0; i--)
    {
        scope = (mcastscopeblock_t*)mc_ptrarray_get(table->block_scopes, i);
        symbol = (mcastsymbol_t*)mc_genericdict_get(scope->store, name);
        if(symbol)
        {
            break;
        }
    }
    if(symbol && symbol->type == SYMBOL_THIS)
    {
        symbol = mc_symtable_defineanddestroyold(table, symbol);
    }
    if(!symbol && table->outer)
    {
        symbol = mc_symtable_resolve(table->outer, name);
        if(!symbol)
        {
            return NULL;
        }
        if(symbol->type == SYMBOL_MODULE_GLOBAL || symbol->type == SYMBOL_GLOBALBUILTIN)
        {
            return symbol;
        }
        symbol = mc_symtable_defineanddestroyold(table, symbol);
    }
    return symbol;
}

TMPSTATIC bool mc_symtable_isdefined(mcastsymtable_t* table, const char* name)
{
    /* todo: rename to something more obvious */
    mcastsymbol_t* symbol;
    mcastscopeblock_t* topscope;
    symbol = global_store_get_symbol(table->global_store, name);
    if(symbol)
    {
        return true;
    }
    topscope = (mcastscopeblock_t*)mc_ptrarray_top(table->block_scopes);
    symbol = (mcastsymbol_t*)mc_genericdict_get(topscope->store, name);
    if(symbol)
    {
        return true;
    }
    return false;
}

TMPSTATIC bool mc_symtable_pushblockscope(mcastsymtable_t* table)
{
    bool ok;
    int blockscopeoffset;
    mcastscopeblock_t* newscope;
    blockscopeoffset = 0;
    mcastscopeblock_t* prevblockscope = (mcastscopeblock_t*)mc_ptrarray_top(table->block_scopes);
    if(prevblockscope)
    {
        blockscopeoffset = table->module_global_offset + prevblockscope->offset + prevblockscope->num_definitions;
    }
    else
    {
        blockscopeoffset = table->module_global_offset;
    }
    newscope = block_scope_make(table->pstate, blockscopeoffset);
    if(!newscope)
    {
        return false;
    }
    ok = mc_ptrarray_push(table->block_scopes, newscope);
    if(!ok)
    {
        block_scope_destroy(newscope);
        return false;
    }
    return true;
}

TMPSTATIC void mc_symtable_popblockscope(mcastsymtable_t* table)
{
    mcastscopeblock_t* topscope;
    topscope = (mcastscopeblock_t*)mc_ptrarray_top(table->block_scopes);
    mc_ptrarray_pop(table->block_scopes);
    block_scope_destroy(topscope);
}

TMPSTATIC mcastscopeblock_t* mc_symtable_getblockscope(mcastsymtable_t* table)
{
    mcastscopeblock_t* topscope;
    topscope = (mcastscopeblock_t*)mc_ptrarray_top(table->block_scopes);
    return topscope;
}

TMPSTATIC bool mc_symtable_ismodglobalscope(mcastsymtable_t* table)
{
    return table->outer == NULL;
}

TMPSTATIC bool mc_symtable_istopblockscope(mcastsymtable_t* table)
{
    return mc_ptrarray_count(table->block_scopes) == 1;
}

TMPSTATIC bool mc_symtable_istopglobalscope(mcastsymtable_t* table)
{
    return mc_symtable_ismodglobalscope(table) && mc_symtable_istopblockscope(table);
}

TMPSTATIC int mc_symtable_getmodglobalsymcount(mcastsymtable_t* table)
{
    return mc_ptrarray_count(table->module_global_symbols);
}

TMPSTATIC mcastsymbol_t* mc_symtable_getmodglobalsymat(mcastsymtable_t* table, int ix)
{
    return (mcastsymbol_t*)mc_ptrarray_get(table->module_global_symbols, ix);
}

TMPSTATIC mcastscopeblock_t* block_scope_make(mcstate_t* state, int offset)
{
    mcastscopeblock_t* newscope;
    newscope = (mcastscopeblock_t*)mc_allocator_alloc(state, sizeof(mcastscopeblock_t));
    if(!newscope)
    {
        return NULL;
    }
    memset(newscope, 0, sizeof(mcastscopeblock_t));
    newscope->pstate = state;
    newscope->store = mc_genericdict_make(state, (mcdictitemcopyfn_t)mc_symbol_copy, (mcdictitemdestroyfn_t)mc_symbol_destroy);
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
    mc_genericdict_destroyitemsanddict(scope->store);
    mc_allocator_free(scope->pstate, scope);
}

TMPSTATIC mcastscopeblock_t* block_scope_copy(mcastscopeblock_t* scope)
{
    mcastscopeblock_t* copy;
    copy = (mcastscopeblock_t*)mc_allocator_alloc(scope->pstate, sizeof(mcastscopeblock_t));
    if(!copy)
    {
        return NULL;
    }
    memset(copy, 0, sizeof(mcastscopeblock_t));
    copy->pstate = scope->pstate;
    copy->num_definitions = scope->num_definitions;
    copy->offset = scope->offset;
    copy->store = mc_genericdict_copy(scope->store);
    if(!copy->store)
    {
        block_scope_destroy(copy);
        return NULL;
    }
    return copy;
}

TMPSTATIC bool mc_symtable_setsymbol(mcastsymtable_t* table, mcastsymbol_t* symbol)
{
    mcastscopeblock_t* topscope;
    topscope = (mcastscopeblock_t*)mc_ptrarray_top(table->block_scopes);
    mcastsymbol_t* existing;
    existing = (mcastsymbol_t*)mc_genericdict_get(topscope->store, symbol->name);
    if(existing)
    {
        mc_symbol_destroy(existing);
    }
    return mc_genericdict_set(topscope->store, symbol->name, symbol);
}

TMPSTATIC int mc_symtable_nextsymindex(mcastsymtable_t* table)
{
    int ix;
    mcastscopeblock_t* topscope;
    topscope = (mcastscopeblock_t*)mc_ptrarray_top(table->block_scopes);
    ix = topscope->offset + topscope->num_definitions;
    return ix;
}

TMPSTATIC int mc_symtable_getnumdefs(mcastsymtable_t* table)
{
    int i;
    int count = 0;
    mcastscopeblock_t* scope;
    for(i = mc_ptrarray_count(table->block_scopes) - 1; i >= 0; i--)
    {
        scope = (mcastscopeblock_t*)mc_ptrarray_get(table->block_scopes, i);
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

TMPSTATIC void token_init(mcasttoken_t* tok, mcasttoktype_t type, const char* literal, int len)
{
    tok->type = type;
    tok->literal = literal;
    tok->len = len;
}

TMPSTATIC char* token_duplicate_literal(mcstate_t* state, mcasttoken_t* tok)
{
    return mc_util_strndup(state, tok->literal, tok->len);
}

TMPSTATIC const char* token_type_to_string(mcasttoktype_t type)
{
    return gtypenames[type];
}

TMPSTATIC mctraceback_t* traceback_make(mcstate_t* state)
{
    mctraceback_t* traceback;
    traceback = (mctraceback_t*)mc_allocator_alloc(state, sizeof(mctraceback_t));
    if(!traceback)
    {
        return NULL;
    }
    memset(traceback, 0, sizeof(mctraceback_t));
    traceback->pstate = state;
    traceback->items = mc_basicarray_make(state, sizeof(mctraceitem_t));
    if(!traceback->items)
    {
        traceback_destroy(traceback);
        return NULL;
    }
    return traceback;
}

TMPSTATIC void traceback_destroy(mctraceback_t* traceback)
{
    int i;
    mctraceitem_t* item;
    if(!traceback)
    {
        return;
    }
    for(i = 0; i < mc_basicarray_count(traceback->items); i++)
    {
        item = (mctraceitem_t*)mc_basicarray_get(traceback->items, i);
        mc_allocator_free(traceback->pstate, item->function_name);
    }
    mc_basicarray_destroy(traceback->items);
    mc_allocator_free(traceback->pstate, traceback);
}

TMPSTATIC bool traceback_append(mctraceback_t* traceback, const char* function_name, mcastlocation_t pos)
{
    bool ok;
    mctraceitem_t item;
    item.function_name = mc_util_strdup(traceback->pstate, function_name);
    if(!item.function_name)
    {
        return false;
    }
    item.pos = pos;
    ok = mc_basicarray_push(traceback->items, &item);
    if(!ok)
    {
        mc_allocator_free(traceback->pstate, item.function_name);
        return false;
    }
    return true;
}

TMPSTATIC bool traceback_append_from_vm(mctraceback_t* traceback, mcstate_t* state)
{
    bool ok;
    int i;
    mcvmframe_t* frame;
    for(i = state->frames_count - 1; i >= 0; i--)
    {
        frame = &state->frames[i];
        ok = traceback_append(traceback, mc_value_functiongetname(frame->function), mc_callframe_getsourcelocation(frame));
        if(!ok)
        {
            return false;
        }
    }
    return true;
}

TMPSTATIC bool traceback_to_string(mctraceback_t* traceback, mcprintstate_t* buf)
{
    int i;
    int depth;
    const char* filename;
    mctraceitem_t* item;
    depth = mc_basicarray_count(traceback->items);
    for(i = 0; i < depth; i++)
    {
        item = (mctraceitem_t*)mc_basicarray_get(traceback->items, i);
        filename = traceback_item_get_filepath(item);
        if(item->pos.line >= 0 && item->pos.column >= 0)
        {
            mc_printer_printf(buf, "%s in %s on %d:%d\n", item->function_name, filename, item->pos.line, item->pos.column);
        }
        else
        {
            mc_printer_printf(buf, "%s\n", item->function_name);
        }
    }
    return !mc_printer_failed(buf);
}

TMPSTATIC const char* traceback_item_get_line(mctraceitem_t* item)
{
    const char* line;
    mcptrarray_t* lines;
    if(!item->pos.file)
    {
        return NULL;
    }
    lines = item->pos.file->lines;
    if(item->pos.line >= mc_ptrarray_count(lines))
    {
        return NULL;
    }
    line = (const char*)mc_ptrarray_get(lines, item->pos.line);
    return line;
}

TMPSTATIC const char* traceback_item_get_filepath(mctraceitem_t* item)
{
    if(!item->pos.file)
    {
        return NULL;
    }
    return item->pos.file->path;
}

TMPSTATIC bool vm_make(mcstate_t* state)
{
    int i;
    state->globals_count = 0;
    state->sp = 0;
    state->this_sp = 0;
    state->frames_count = 0;
    state->last_popped = mc_value_makenull();
    state->running = false;
    for(i = 0; i < OPCODE_MAX; i++)
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

TMPSTATIC void vm_destroy(mcstate_t* state)
{
    if(!state)
    {
        return;
    }
}

TMPSTATIC void vm_reset(mcstate_t* state)
{
    state->sp = 0;
    state->this_sp = 0;
    while(state->frames_count > 0)
    {
        pop_frame(state);
    }
}

TMPSTATIC bool mc_vm_runexecfunc(mcstate_t* state, compilation_result_t* comp_res, mcbasicarray_t* constants)
{
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    int oldsp = state->sp;
#endif
    int oldthissp = state->this_sp;
    int oldframescount = state->frames_count;
    mcvalue_t mainfn = object_make_function(state, "main", comp_res, false, 0, 0, 0);
    if(object_is_null(mainfn))
    {
        return false;
    }
    stack_push(state, mainfn);
    bool res = mc_function_execfunction(state, mainfn, constants);
    while(state->frames_count > oldframescount)
    {
        pop_frame(state);
    }
    MC_ASSERT(state->sp == oldsp);
    state->this_sp = oldthissp;
    return res;
}

TMPSTATIC mcvalue_t vm_call(mcstate_t* state, mcbasicarray_t* constants, mcvalue_t callee, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    mcobjtype_t type = object_get_type(callee);
    if(type == MC_OBJ_FUNCTION)
    {
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
        int oldsp = state->sp;
#endif
        int oldthissp = state->this_sp;
        int oldframescount = state->frames_count;
        stack_push(state, callee);
        for(i = 0; i < argc; i++)
        {
            stack_push(state, args[i]);
        }
        ok = mc_function_execfunction(state, callee, constants);
        if(!ok)
        {
            return mc_value_makenull();
        }
        while(state->frames_count > oldframescount)
        {
            pop_frame(state);
        }
        MC_ASSERT(state->sp == oldsp);
        state->this_sp = oldthissp;
        return vm_get_last_popped(state);
    }
    if(type == MC_OBJ_NATIVE_FUNCTION)
    {
        return mc_vm_callnativefunction(state, callee, srcposinvalid, argc, args);
    }
    mc_errlist_push(&state->errors, MC_ERROR_USER, srcposinvalid, "Object is not callable");
    return mc_value_makenull();
}

TMPSTATIC bool mc_vmdo_math(mcstate_t* state, mcopcode_t opcode)
{
    bool ok;
    double res;
    double dnright;
    double dnleft;
    int leftlen;
    int rightlen;
    mcvalue_t valright;
    mcvalue_t valleft;
    mcobjtype_t lefttype;
    mcobjtype_t righttype;
    valright = stack_pop(state);
    valleft = stack_pop(state);
    lefttype = object_get_type(valleft);
    righttype = object_get_type(valright);
    if(object_is_numeric(valleft) && object_is_numeric(valright))
    {
        dnright = object_get_number(valright);
        dnleft = object_get_number(valleft);
        res = 0;
        switch(opcode)
        {
            case OPCODE_ADD:
                res = mc_mathutil_add(dnleft, dnright);
                break;
            case OPCODE_SUB:
                res = mc_mathutil_sub(dnleft, dnright);
                break;
            case OPCODE_MUL:
                res = mc_mathutil_mult(dnleft, dnright);
                break;
            case OPCODE_DIV:
                res = mc_mathutil_div(dnleft, dnright);
                break;
            case OPCODE_MOD:
                res = mc_mathutil_mod(dnleft, dnright);
                break;
            case OPCODE_OR:
                res = mc_mathutil_binor(dnleft, dnright);
                break;
            case OPCODE_XOR:
                res = mc_mathutil_binxor(dnleft, dnright);
                break;
            case OPCODE_AND:
                res = mc_mathutil_binand(dnleft, dnright);
                break;
            case OPCODE_LSHIFT:
                res = mc_mathutil_binshiftleft(dnleft, dnright);
                break;
            case OPCODE_RSHIFT:
                res = mc_mathutil_binshiftright(dnleft, dnright);
                break;
            default:
                MC_ASSERT(false);
                break;
        }
        stack_push(state, mc_value_makenumber(res));
    }
    else if(lefttype == MC_OBJ_STRING && righttype == MC_OBJ_STRING && opcode == OPCODE_ADD)
    {
        leftlen = mc_value_stringgetlength(valleft);
        rightlen = mc_value_stringgetlength(valright);

        if(leftlen == 0)
        {
            stack_push(state, valright);
        }
        else if(rightlen == 0)
        {
            stack_push(state, valleft);
        }
        else
        {
            const char* strleft = mc_value_stringgetdata(valleft);
            const char* strright = mc_value_stringgetdata(valright);

            mcvalue_t nstring = mc_value_makestrcapacity(state, leftlen + rightlen);
            if(object_is_null(nstring))
            {
                return false;
            }

            ok = mc_value_stringappend(nstring, strleft, leftlen);
            if(!ok)
            {
                return false;
            }

            ok = mc_value_stringappend(nstring, strright, rightlen);
            if(!ok)
            {
                return false;
            }
            stack_push(state, nstring);
        }
    }
    else
    {
        bool overloadfound = false;
        ok = try_overload_operator(state, valleft, valright, opcode, &overloadfound);
        if(!ok)
        {
            return false;
        }
        if(!overloadfound)
        {
            const char* opcodename = opcode_get_name(opcode);
            const char* lefttypename = object_get_type_name(lefttype);
            const char* righttypename = object_get_type_name(righttype);
            mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(state->current_frame), "Invalid operand types for %s, got %s and %s",
                              opcodename, lefttypename, righttypename);
            return false;
        }
    }
    return true;
}

TMPSTATIC bool mc_function_execfunction(mcstate_t* vm, mcvalue_t function, mcbasicarray_t* constants)
{
    bool ok;
    bool checktime;
    int fri;
    int elapsedms;
    int recoverframeix;
    unsigned timecheckinterval;
    unsigned timecheckcounter;
    double maxexectimems;
    mcopcode_t opcode;
    mctimer_t timer;
    mcvmframe_t createdframe;
    mcvalue_t errobj;
    mcvmframe_t* frame;
    mcerror_t* err;
    mcobjfuncscript_t* functionfunction;
    if(vm->running)
    {
        mc_errlist_push(&vm->errors, MC_ERROR_USER, srcposinvalid, "VM is already executing code");
        return false;
    }
    /* naming is hard */
    functionfunction = mc_value_functiongetscriptfunction(function);
    ok = false;
    ok = mc_callframe_init(&createdframe, function, vm->sp - functionfunction->num_args);
    if(!ok)
    {
        fprintf(stderr, "failed to init frames!\n");
        return false;
    }
    ok = push_frame(vm, createdframe);
    if(!ok)
    {
        mc_errlist_push(&vm->errors, MC_ERROR_USER, srcposinvalid, "Pushing frame failed");
        return false;
    }
    fprintf(stderr, "**executing function**\n");
    vm->running = true;
    vm->last_popped = mc_value_makenull();
    checktime = false;
    maxexectimems = 0;
    #if 0
    if(vm->config)
    #endif
    {
        checktime = vm->config.havemaxexectime;
        maxexectimems = vm->config.maxexecutiontime;
    }
    timecheckinterval = 1000;
    timecheckcounter = 0;
    memset(&timer, 0, sizeof(mctimer_t));
    if(checktime)
    {
        timer = mc_timer_start();
    }

    while(vm->current_frame->ip < vm->current_frame->bytecode_size)
    {
        opcode = mc_callframe_readopcode(vm->current_frame);
        #if 0
            fprintf(stderr, "opcode=%d (%s)\n", opcode, opcode_lookup(opcode)->name);
        #endif
        switch(opcode)
        {
            case OPCODE_CONSTANT:
                {
                    uint16_t constantix;
                    mcvalue_t* constant;
                    constantix = mc_callframe_readuint16(vm->current_frame);
                    constant = (mcvalue_t*)mc_basicarray_get(constants, constantix);
                    if(!constant)
                    {
                        mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Constant at %d not found", constantix);
                        goto err;
                    }
                    stack_push(vm, *constant);
                }
                break;
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
                }
                break;
            case OPCODE_POP:
                {
                    stack_pop(vm);
                }
                break;
            case OPCODE_TRUE:
                {
                    stack_push(vm, mc_value_makebool(true));
                }
                break;
            case OPCODE_FALSE:
                {
                    stack_push(vm, mc_value_makebool(false));
                }
                break;
            case OPCODE_COMPARE:
            case OPCODE_COMPARE_EQ:
            {
                bool isoverloaded;
                double comparisonres;
                const char* lefttypestring;
                const char* righttypestring;
                mcvalue_t res;
                mcvalue_t left;
                mcvalue_t right;
                right = stack_pop(vm);
                left = stack_pop(vm);
                isoverloaded = false;
                ok = try_overload_operator(vm, left, right, OPCODE_COMPARE, &isoverloaded);
                if(!ok)
                {
                    goto err;
                }
                if(!isoverloaded)
                {
                    comparisonres = mc_value_compare(left, right, &ok);
                    if(ok || opcode == OPCODE_COMPARE_EQ)
                    {
                        res = mc_value_makenumber(comparisonres);
                        stack_push(vm, res);
                    }
                    else
                    {
                        righttypestring = object_get_type_name(object_get_type(right));
                        lefttypestring = object_get_type_name(object_get_type(left));
                        mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Cannot compare %s and %s", lefttypestring, righttypestring);
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
                    bool resval;
                    double comparisonres;
                    mcvalue_t res;
                    mcvalue_t value;
                    value = stack_pop(vm);
                    comparisonres = object_get_number(value);
                    resval = false;
                    switch(opcode)
                    {
                        case OPCODE_EQUAL:
                            {
                                resval = MC_UTIL_CMPFLOAT(comparisonres, 0);
                            }
                            break;
                        case OPCODE_NOT_EQUAL:
                            {
                                resval = !MC_UTIL_CMPFLOAT(comparisonres, 0);
                            }
                            break;
                        case OPCODE_GREATER_THAN:
                            {
                                resval = comparisonres > 0;
                            }
                            break;
                        case OPCODE_GREATER_THAN_EQUAL:
                            {
                                resval = comparisonres > 0 || MC_UTIL_CMPFLOAT(comparisonres, 0);
                            }
                            break;

                        default:
                            {
                                MC_ASSERT(false);
                            }break;
                    }
                    res = mc_value_makebool(resval);
                    stack_push(vm, res);
                }
                break;
            case OPCODE_MINUS:
            {
                bool overloadfound;
                double val;
                const char* operandtypestring;
                mcobjtype_t operandtype;
                mcvalue_t operand;
                operand = stack_pop(vm);
                operandtype = object_get_type(operand);
                if(operandtype == MC_OBJ_NUMBER)
                {
                    val = object_get_number(operand);
                    mcvalue_t res = mc_value_makenumber(-val);
                    stack_push(vm, res);
                }
                else
                {
                    overloadfound = false;
                    ok = try_overload_operator(vm, operand, mc_value_makenull(), OPCODE_MINUS, &overloadfound);
                    if(!ok)
                    {
                        goto err;
                    }
                    if(!overloadfound)
                    {
                        operandtypestring = object_get_type_name(operandtype);
                        mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Invalid operand type for MINUS, got %s", operandtypestring);
                        goto err;
                    }
                }
                break;
            }
            case OPCODE_BANG:
            {
                mcvalue_t operand = stack_pop(vm);
                mcobjtype_t type = object_get_type(operand);
                if(type == MC_OBJ_BOOL)
                {
                    mcvalue_t res = mc_value_makebool(!object_get_bool(operand));
                    stack_push(vm, res);
                }
                else if(type == MC_OBJ_NULL)
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
                uint16_t pos = mc_callframe_readuint16(vm->current_frame);
                vm->current_frame->ip = pos;
                break;
            }
            case OPCODE_JUMP_IF_FALSE:
            {
                uint16_t pos = mc_callframe_readuint16(vm->current_frame);
                mcvalue_t test = stack_pop(vm);
                if(!object_get_bool(test))
                {
                    vm->current_frame->ip = pos;
                }
                break;
            }
            case OPCODE_JUMP_IF_TRUE:
            {
                uint16_t pos = mc_callframe_readuint16(vm->current_frame);
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
                uint16_t ix = mc_callframe_readuint16(vm->current_frame);
                mcvalue_t value = stack_pop(vm);
                vm_set_global(vm, ix, value);
                break;
            }
            case OPCODE_SET_MODULE_GLOBAL:
            {
                uint16_t ix = mc_callframe_readuint16(vm->current_frame);
                mcvalue_t nvalue = stack_pop(vm);
                mcvalue_t oldvalue = vm_get_global(vm, ix);
                if(!check_assign(vm, oldvalue, nvalue))
                {
                    goto err;
                }
                vm_set_global(vm, ix, nvalue);
                break;
            }
            case OPCODE_GET_MODULE_GLOBAL:
            {
                uint16_t ix = mc_callframe_readuint16(vm->current_frame);
                mcvalue_t global = vm->globals[ix];
                stack_push(vm, global);
                break;
            }
            case OPCODE_ARRAY:
            {
                int i;
                uint16_t count = mc_callframe_readuint16(vm->current_frame);
                mcvalue_t arrayobj = object_make_array_with_capacity(vm, count);
                if(object_is_null(arrayobj))
                {
                    goto err;
                }
                mcvalue_t* items = vm->stack + vm->sp - count;
                for(i = 0; i < count; i++)
                {
                    mcvalue_t item = items[i];
                    ok = mc_value_arraypush(arrayobj, item);
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
                uint16_t count = mc_callframe_readuint16(vm->current_frame);
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
                int i;
                uint16_t kvpcount = mc_callframe_readuint16(vm->current_frame);
                uint16_t itemscount = kvpcount * 2;
                mcvalue_t mapobj = this_stack_pop(vm);
                mcvalue_t* kvpairs = vm->stack + vm->sp - itemscount;
                for(i = 0; i < itemscount; i += 2)
                {
                    mcvalue_t key = kvpairs[i];
                    if(!object_is_hashable(key))
                    {
                        mcobjtype_t keytype = object_get_type(key);
                        const char* keytypename = object_get_type_name(keytype);
                        mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Key of type %s is not hashable", keytypename);
                        goto err;
                    }
                    mcvalue_t val = kvpairs[i + 1];
                    ok = mc_value_mapsetvalue(mapobj, key, val);
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

                if(lefttype != MC_OBJ_ARRAY && lefttype != MC_OBJ_MAP && lefttype != MC_OBJ_STRING)
                {
                    mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Type %s is not indexable", lefttypename);
                    goto err;
                }

                mcvalue_t res = mc_value_makenull();

                if(lefttype == MC_OBJ_ARRAY)
                {
                    if(indextype != MC_OBJ_NUMBER)
                    {
                        mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Cannot index %s with %s", lefttypename, indextypename);
                        goto err;
                    }
                    int ix = (int)object_get_number(index);
                    if(ix < 0)
                    {
                        ix = mc_value_arraygetlength(left) + ix;
                    }
                    if(ix >= 0 && ix < mc_value_arraygetlength(left))
                    {
                        res = mc_value_arraygetvalueat(left, ix);
                    }
                }
                else if(lefttype == MC_OBJ_MAP)
                {
                    res = mc_value_mapgetvalue(left, index);
                }
                else if(lefttype == MC_OBJ_STRING)
                {
                    const char* str = mc_value_stringgetdata(left);
                    int leftlen = mc_value_stringgetlength(left);
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

                if(lefttype != MC_OBJ_ARRAY && lefttype != MC_OBJ_MAP && lefttype != MC_OBJ_STRING)
                {
                    mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Type %s is not indexable", lefttypename);
                    goto err;
                }

                mcvalue_t res = mc_value_makenull();
                if(indextype != MC_OBJ_NUMBER)
                {
                    mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Cannot index %s with %s", lefttypename, indextypename);
                    goto err;
                }
                ix = (int)object_get_number(index);

                if(lefttype == MC_OBJ_ARRAY)
                {
                    res = mc_value_arraygetvalueat(left, ix);
                }
                else if(lefttype == MC_OBJ_MAP)
                {
                    res = mc_value_mapgetkvpairat(vm, left, ix);
                }
                else if(lefttype == MC_OBJ_STRING)
                {
                    const char* str = mc_value_stringgetdata(left);
                    int leftlen = mc_value_stringgetlength(left);
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
                uint8_t num_args = mc_callframe_readuint8(vm->current_frame);
                mcvalue_t callee = stack_get(vm, num_args);
                ok = mc_vm_callobject(vm, callee, num_args);
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
                uint8_t pos = mc_callframe_readuint8(vm->current_frame);
                vm->stack[vm->current_frame->base_pointer + pos] = stack_pop(vm);
                break;
            }
            case OPCODE_SET_LOCAL:
            {
                uint8_t pos = mc_callframe_readuint8(vm->current_frame);
                mcvalue_t nvalue = stack_pop(vm);
                mcvalue_t oldvalue = vm->stack[vm->current_frame->base_pointer + pos];
                if(!check_assign(vm, oldvalue, nvalue))
                {
                    goto err;
                }
                vm->stack[vm->current_frame->base_pointer + pos] = nvalue;
                break;
            }
            case OPCODE_GET_LOCAL:
            {
                uint8_t pos = mc_callframe_readuint8(vm->current_frame);
                mcvalue_t val = vm->stack[vm->current_frame->base_pointer + pos];
                stack_push(vm, val);
                break;
            }
            case OPCODE_GETGLOBALBUILTIN:
            {
                uint16_t ix = mc_callframe_readuint16(vm->current_frame);
                ok = false;
                mcvalue_t val = global_store_get_object_at(vm->global_store, ix, &ok);
                if(!ok)
                {
                    mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Global value %d not found", ix);
                    goto err;
                }
                stack_push(vm, val);
                break;
            }
            case OPCODE_FUNCTION:
            {
                int i;
                uint16_t constantix = mc_callframe_readuint16(vm->current_frame);
                uint8_t numfree = mc_callframe_readuint8(vm->current_frame);
                mcvalue_t* constant = (mcvalue_t*)mc_basicarray_get(constants, constantix);
                if(!constant)
                {
                    mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Constant %d not found", constantix);
                    goto err;
                }
                mcobjtype_t constanttype = object_get_type(*constant);
                if(constanttype != MC_OBJ_FUNCTION)
                {
                    const char* tname = object_get_type_name(constanttype);
                    mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "%s is not a function", tname);
                    goto err;
                }

                mcobjfuncscript_t* constantfunction = mc_value_functiongetscriptfunction(*constant);
                mcvalue_t functionobj = object_make_function(vm, mc_value_functiongetname(*constant), constantfunction->comp_result, false,
                                                             constantfunction->num_locals, constantfunction->num_args, numfree);
                if(object_is_null(functionobj))
                {
                    goto err;
                }
                for(i = 0; i < numfree; i++)
                {
                    mcvalue_t freeval = vm->stack[vm->sp - numfree + i];
                    mc_value_functionsetfreevalat(functionobj, i, freeval);
                }
                set_sp(vm, vm->sp - numfree);
                stack_push(vm, functionobj);
                break;
            }
            case OPCODE_GET_FREE:
            {
                uint8_t freeix = mc_callframe_readuint8(vm->current_frame);
                mcvalue_t val = mc_value_functiongetfreevalat(vm->current_frame->function, freeix);
                stack_push(vm, val);
                break;
            }
            case OPCODE_SET_FREE:
            {
                uint8_t freeix = mc_callframe_readuint8(vm->current_frame);
                mcvalue_t val = stack_pop(vm);
                mc_value_functionsetfreevalat(vm->current_frame->function, freeix, val);
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
                mcvalue_t nvalue = stack_pop(vm);
                mcobjtype_t lefttype = object_get_type(left);
                mcobjtype_t indextype = object_get_type(index);
                const char* lefttypename = object_get_type_name(lefttype);
                const char* indextypename = object_get_type_name(indextype);

                if(lefttype != MC_OBJ_ARRAY && lefttype != MC_OBJ_MAP)
                {
                    mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Type %s is not indexable", lefttypename);
                    goto err;
                }

                if(lefttype == MC_OBJ_ARRAY)
                {
                    int alen;
                    if(indextype != MC_OBJ_NUMBER)
                    {
                        mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Cannot index %s with %s", lefttypename, indextypename);
                        goto err;
                    }
                    int ix = (int)object_get_number(index);
                    
                    ok = mc_value_arraysetvalueat(left, ix, nvalue);
                    alen = mc_value_arraygetlength(left);

                    if(!ok)
                    {
                        mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "failed to set array index %d (of %d)", ix, alen);
                        goto err;
                    }
                }
                else if(lefttype == MC_OBJ_MAP)
                {
                    mcvalue_t oldvalue = mc_value_mapgetvalue(left, index);
                    if(!check_assign(vm, oldvalue, nvalue))
                    {
                        goto err;
                    }
                    ok = mc_value_mapsetvalue(left, index, nvalue);
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
                if(type == MC_OBJ_ARRAY)
                {
                    len = mc_value_arraygetlength(val);
                }
                else if(type == MC_OBJ_MAP)
                {
                    len = mc_value_mapgetlength(val);
                }
                else if(type == MC_OBJ_STRING)
                {
                    len = mc_value_stringgetlength(val);
                }
                else
                {
                    const char* tname = object_get_type_name(type);
                    mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Cannot get length of %s", tname);
                    goto err;
                }
                stack_push(vm, mc_value_makenumber(len));
                break;
            }
            case OPCODE_NUMBER:
            {
                uint64_t val = mc_callframe_readuint64(vm->current_frame);
                double val_double = mc_util_uint64todouble(val);
                mcvalue_t obj = mc_value_makenumber(val_double);
                stack_push(vm, obj);
                break;
            }
            case OPCODE_SET_RECOVER:
            {
                uint16_t recover_ip = mc_callframe_readuint16(vm->current_frame);
                vm->current_frame->recover_ip = recover_ip;
                break;
            }
            default:
            {
                MC_ASSERT(false);
                mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Unknown opcode: 0x%x", opcode);
                goto err;
            }
        }
        if(checktime)
        {
            timecheckcounter++;
            if(timecheckcounter > timecheckinterval)
            {
                elapsedms = (int)mc_timer_getelapsedms(&timer);
                if(elapsedms > maxexectimems)
                {
                    mc_errlist_addf(&vm->errors, MC_ERROR_TIMEOUT, mc_callframe_getsourcelocation(vm->current_frame), "Execution took more than %1.17g ms", maxexectimems);
                    goto err;
                }
                timecheckcounter = 0;
            }
        }
    err:
        if(mc_errlist_size(&vm->errors) > 0)
        {
            err = errors_get_last_error(&vm->errors);
            if(err->type == MC_ERROR_RUNTIME && mc_errlist_size(&vm->errors) == 1)
            {
                recoverframeix = -1;
                for(fri = vm->frames_count - 1; fri >= 0; fri--)
                {
                    frame = &vm->frames[fri];
                    if(frame->recover_ip >= 0 && !frame->is_recovering)
                    {
                        recoverframeix = fri;
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
                errobj = object_make_error(vm, err->message);
                if(!object_is_null(errobj))
                {
                    mc_value_errorsettraceback(errobj, err->traceback);
                    err->traceback = NULL;
                }
                stack_push(vm, errobj);
                vm->current_frame->ip = vm->current_frame->recover_ip;
                vm->current_frame->is_recovering = true;
                mc_errlist_clear(&vm->errors);
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
    if(mc_errlist_size(&vm->errors) > 0)
    {
        err = errors_get_last_error(&vm->errors);
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
    return mc_errlist_size(&vm->errors) == 0;
}

TMPSTATIC mcvalue_t vm_get_last_popped(mcstate_t* vm)
{
    return vm->last_popped;
}

TMPSTATIC bool vm_has_errors(mcstate_t* vm)
{
    return mc_errlist_size(&vm->errors) > 0;
}

TMPSTATIC bool vm_set_global(mcstate_t* vm, int ix, mcvalue_t val)
{
    if(ix >= VM_MAX_GLOBALS)
    {
        MC_ASSERT(false);
        mc_errlist_push(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Global write out of range");
        return false;
    }
    vm->globals[ix] = val;
    if(ix >= vm->globals_count)
    {
        vm->globals_count = ix + 1;
    }
    return true;
}

TMPSTATIC mcvalue_t vm_get_global(mcstate_t* vm, int ix)
{
    if(ix >= VM_MAX_GLOBALS)
    {
        MC_ASSERT(false);
        mc_errlist_push(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Global read out of range");
        return mc_value_makenull();
    }
    return vm->globals[ix];
}

TMPSTATIC void set_sp(mcstate_t* vm, int nsp)
{
    int count;
    size_t bytescount;
    if(nsp > vm->sp)
    {
        /* to avoid gcing freed objects */
        count = nsp - vm->sp;
        bytescount = count * sizeof(mcvalue_t);
        memset(vm->stack + vm->sp, 0, bytescount);
    }
    vm->sp = nsp;
}

TMPSTATIC void stack_push(mcstate_t* vm, mcvalue_t obj)
{
    int numlocals;
    mcvmframe_t* frame;
    mcobjfuncscript_t* currentfunction;
    (void)numlocals;
    (void)frame;
    (void)currentfunction;
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(vm->sp >= VM_STACK_SIZE)
    {
        MC_ASSERT(false);
        mc_errlist_push(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Stack overflow");
        return;
    }
    if(vm->current_frame)
    {
        frame = vm->current_frame;
        currentfunction = mc_value_functiongetscriptfunction(frame->function);
        numlocals = currentfunction->num_locals;
        MC_ASSERT(vm->sp >= (frame->base_pointer + numlocals));
    }
#endif
    vm->stack[vm->sp] = obj;
    vm->sp++;
}

TMPSTATIC mcvalue_t stack_pop(mcstate_t* vm)
{
    int numlocals;
    mcvmframe_t* frame;
    mcobjfuncscript_t* currentfunction;
    (void)numlocals;
    (void)frame;
    (void)currentfunction;
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(vm->sp == 0)
    {
        mc_errlist_push(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Stack underflow");
        MC_ASSERT(false);
        return mc_value_makenull();
    }
    if(vm->current_frame)
    {
        frame = vm->current_frame;
        currentfunction = mc_value_functiongetscriptfunction(frame->function);
        numlocals = currentfunction->num_locals;
        MC_ASSERT((vm->sp - 1) >= (frame->base_pointer + numlocals));
    }
#endif
    vm->sp--;
    mcvalue_t res = vm->stack[vm->sp];
    vm->last_popped = res;
    return res;
}

TMPSTATIC mcvalue_t stack_get(mcstate_t* vm, int nthitem)
{
    int ix;
    ix = vm->sp - 1 - nthitem;
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(ix < 0 || ix >= VM_STACK_SIZE)
    {
        mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Invalid stack index: %d", nthitem);
        MC_ASSERT(false);
        return mc_value_makenull();
    }
#endif
    return vm->stack[ix];
}

TMPSTATIC void this_stack_push(mcstate_t* vm, mcvalue_t obj)
{
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(vm->this_sp >= VM_THIS_STACK_SIZE)
    {
        MC_ASSERT(false);
        mc_errlist_push(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "this stack overflow");
        return;
    }
#endif
    vm->this_stack[vm->this_sp] = obj;
    vm->this_sp++;
}

TMPSTATIC mcvalue_t this_stack_pop(mcstate_t* vm)
{
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(vm->this_sp == 0)
    {
        mc_errlist_push(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "this stack underflow");
        MC_ASSERT(false);
        return mc_value_makenull();
    }
#endif
    vm->this_sp--;
    return vm->this_stack[vm->this_sp];
}

TMPSTATIC mcvalue_t this_stack_get(mcstate_t* vm, int nthitem)
{
    int ix;
    ix = vm->this_sp - 1 - nthitem;
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(ix < 0 || ix >= VM_THIS_STACK_SIZE)
    {
        mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Invalid this stack index: %d", nthitem);
        MC_ASSERT(false);
        return mc_value_makenull();
    }
#endif
    return vm->this_stack[ix];
}

TMPSTATIC bool push_frame(mcstate_t* vm, mcvmframe_t frame)
{
    mcobjfuncscript_t* framefunction;
    if(vm->frames_count >= VM_MAX_FRAMES)
    {
        MC_ASSERT(false);
        return false;
    }
    vm->frames[vm->frames_count] = frame;
    vm->current_frame = &vm->frames[vm->frames_count];
    vm->frames_count++;
    framefunction = mc_value_functiongetscriptfunction(frame.function);
    set_sp(vm, frame.base_pointer + framefunction->num_locals);
    return true;
}

TMPSTATIC bool pop_frame(mcstate_t* vm)
{
    set_sp(vm, vm->current_frame->base_pointer - 1);
    if(vm->frames_count <= 0)
    {
        MC_ASSERT(false);
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
    int i;
    mcvmframe_t* frame;
    gc_unmark_all(vm->mem);
    gc_mark_objects(global_store_get_object_data(vm->global_store), global_store_get_object_count(vm->global_store));
    gc_mark_objects((mcvalue_t*)mc_basicarray_data(constants), mc_basicarray_count(constants));
    gc_mark_objects(vm->globals, vm->globals_count);
    for(i = 0; i < vm->frames_count; i++)
    {
        frame = &vm->frames[i];
        gc_mark_object(frame->function);
    }
    gc_mark_objects(vm->stack, vm->sp);
    gc_mark_objects(vm->this_stack, vm->this_sp);
    gc_mark_object(vm->last_popped);
    gc_mark_objects(vm->operator_oveload_keys, OPCODE_MAX);
    gc_sweep(vm->mem);
}

TMPSTATIC bool mc_vm_callobject(mcstate_t* vm, mcvalue_t callee, int nargs)
{
    bool ok;
    const char* calleetypename;
    mcobjtype_t calleetype;
    mcvmframe_t calleeframe;
    mcvalue_t res;
    mcvalue_t* stackpos;
    mcobjfuncscript_t* calleefunction;
    calleetype = object_get_type(callee);
    if(calleetype == MC_OBJ_FUNCTION)
    {
        calleefunction = mc_value_functiongetscriptfunction(callee);
        if(nargs != calleefunction->num_args)
        {
            mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Invalid number of arguments to \"%s\", expected %d, got %d",
                              mc_value_functiongetname(callee), calleefunction->num_args, nargs);
            return false;
        }
        ok = mc_callframe_init(&calleeframe, callee, vm->sp - nargs);
        if(!ok)
        {
            mc_errlist_push(&vm->errors, MC_ERROR_RUNTIME, srcposinvalid, "Frame init failed in mc_vm_callobject");
            return false;
        }
        ok = push_frame(vm, calleeframe);
        if(!ok)
        {
            mc_errlist_push(&vm->errors, MC_ERROR_RUNTIME, srcposinvalid, "Pushing frame failed in mc_vm_callobject");
            return false;
        }
    }
    else if(calleetype == MC_OBJ_NATIVE_FUNCTION)
    {
        stackpos = vm->stack + vm->sp - nargs;
        res = mc_vm_callnativefunction(vm, callee, mc_callframe_getsourcelocation(vm->current_frame), nargs, stackpos);
        if(vm_has_errors(vm))
        {
            return false;
        }
        set_sp(vm, vm->sp - nargs - 1);
        stack_push(vm, res);
    }
    else
    {
        calleetypename = object_get_type_name(calleetype);
        mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "%s object is not callable", calleetypename);
        return false;
    }
    return true;
}

TMPSTATIC mcvalue_t mc_vm_callnativefunction(mcstate_t* vm, mcvalue_t callee, mcastlocation_t srcpos, int argc, mcvalue_t* args)
{
    mcobjtype_t restype;
    mcvalue_t res;
    mcerror_t* err; 
    mctraceback_t* traceback;
    mcobjfuncnative_t* nativefun;
    nativefun = mc_value_functiongetnativefunction(callee);
    res = nativefun->fn(vm, nativefun->data, argc, args);
    if(mc_errlist_haserrors(&vm->errors) && !MC_UTIL_STREQ(nativefun->name, "crash"))
    {
        err = errors_get_last_error(&vm->errors);
        err->pos = srcpos;
        err->traceback = traceback_make(vm);
        if(err->traceback)
        {
            traceback_append(err->traceback, nativefun->name, srcposinvalid);
        }
        return mc_value_makenull();
    }
    restype = object_get_type(res);
    if(restype == MC_OBJ_ERROR)
    {
        traceback = traceback_make(vm);
        if(traceback)
        {
            /* error builtin is treated in a special way */
            if(!MC_UTIL_STREQ(nativefun->name, "error"))
            {
                traceback_append(traceback, nativefun->name, srcposinvalid);
            }
            traceback_append_from_vm(traceback, vm);
            mc_value_errorsettraceback(res, traceback);
        }
    }
    return res;
}

TMPSTATIC bool check_assign(mcstate_t* vm, mcvalue_t oldvalue, mcvalue_t nvalue)
{
    mcobjtype_t nvaluetype;
    mcobjtype_t oldvaluetype;
    oldvaluetype = object_get_type(oldvalue);
    nvaluetype = object_get_type(nvalue);
    if(oldvaluetype == MC_OBJ_NULL || nvaluetype == MC_OBJ_NULL)
    {
        return true;
    }
    if(oldvaluetype != nvaluetype)
    {
        mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getsourcelocation(vm->current_frame), "Trying to assign variable of type %s to %s",
                          object_get_type_name(nvaluetype), object_get_type_name(oldvaluetype));
        return false;
    }
    return true;
}

TMPSTATIC bool try_overload_operator(mcstate_t* vm, mcvalue_t left, mcvalue_t right, mcinternopcode_t op, bool* outoverloadfound)
{
    int numoper;
    mcvalue_t key;
    mcvalue_t callee;
    mcobjtype_t lefttype;
    mcobjtype_t righttype;
    *outoverloadfound = false;
    lefttype = object_get_type(left);
    righttype = object_get_type(right);
    if(lefttype != MC_OBJ_MAP && righttype != MC_OBJ_MAP)
    {
        *outoverloadfound = false;
        return true;
    }
    numoper = 2;
    if(op == OPCODE_MINUS || op == OPCODE_BANG)
    {
        numoper = 1;
    }
    key = vm->operator_oveload_keys[op];
    callee = mc_value_makenull();
    if(lefttype == MC_OBJ_MAP)
    {
        callee = mc_value_mapgetvalue(left, key);
    }
    if(!object_is_callable(callee))
    {
        if(righttype == MC_OBJ_MAP)
        {
            callee = mc_value_mapgetvalue(right, key);
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
    if(numoper == 2)
    {
        stack_push(vm, right);
    }
    return mc_vm_callobject(vm, callee, numoper);
}

TMPSTATIC void mc_state_printerrors(mcstate_t* state)
{
    int i;
    int ecnt;
    char* errstr;
    mcerror_t* err;
    ecnt = mc_state_errorcount(state);
    for(i = 0; i < ecnt; i++)
    {
        err = mc_state_geterror(state, i);
        errstr = mc_error_serializetostring(state, err);
        puts(errstr);
        mc_state_freeallocated(state, errstr);
    }
}


TMPSTATIC char* read_file(const char* filename)
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


void optprs_fprintmaybearg(FILE* out, const char* begin, const char* flagname, size_t flaglen, bool needval, bool maybeval, const char* delim)
{
    fprintf(out, "%s%.*s", begin, (int)flaglen, flagname);
    if(needval)
    {
        if(maybeval)
        {
            fprintf(out, "[");
        }
        if(delim != NULL)
        {
            fprintf(out, "%s", delim);
        }
        fprintf(out, "<val>");
        if(maybeval)
        {
            fprintf(out, "]");
        }
    }
}

void optprs_fprintusage(FILE* out, optlongflags_t* flags)
{
    size_t i;
    char ch;
    bool needval;
    bool maybeval;
    bool hadshort;
    optlongflags_t* flag;
    for(i=0; flags[i].longname != NULL; i++)
    {
        flag = &flags[i];
        hadshort = false;
        needval = (flag->argtype > OPTPARSE_NONE);
        maybeval = (flag->argtype == OPTPARSE_OPTIONAL);
        if(flag->shortname > 0)
        {
            hadshort = true;
            ch = flag->shortname;
            fprintf(out, "    ");
            optprs_fprintmaybearg(out, "-", &ch, 1, needval, maybeval, NULL);
        }
        if(flag->longname != NULL)
        {
            if(hadshort)
            {
                fprintf(out, ", ");
            }
            else
            {
                fprintf(out, "    ");
            }
            optprs_fprintmaybearg(out, "--", flag->longname, strlen(flag->longname), needval, maybeval, "=");
        }
        if(flag->helptext != NULL)
        {
            fprintf(out, "  -  %s", flag->helptext);
        }
        fprintf(out, "\n");
    }
}

void mc_cli_printusage(char* argv[], optlongflags_t* flags, bool fail)
{
    FILE* out;
    out = fail ? stderr : stdout;
    fprintf(out, "Usage: %s [<options>] [<filename> | -e <code>]\n", argv[0]);
    optprs_fprintusage(out, flags);
}

static int g_extfnvar;

bool mc_cli_compileandrunsource(mcstate_t* state, mcvalue_t* vdest, const char* source)
{
    bool ok;
    mcvalue_t tmp;
    ok = false;
    mccompiledprogram_t* program = mc_state_compilesource(state, source);
    tmp = mc_program_execute(state, program);
    if(mc_state_haserrors(state))
    {
        mc_state_printerrors(state);
    }
    else
    {
        ok = true;
    }
    if(vdest != NULL)
    {
        *vdest =  tmp;
    }
    mc_program_destroy(program);
    return ok;
}

bool mc_cli_compileandrunfile(mcstate_t* state, const char* filename)
{
    bool ok;
    char* code;
    code = read_file(filename);
    if(code == NULL)
    {
        return false;
    }
    ok = mc_cli_compileandrunsource(state, NULL, code);
    free(code);
    return ok;
}

void mc_cli_installargv(mcstate_t* state, int argc, char** argv, int beginat)
{
    int i;
    mcvalue_t strval;
    mcvalue_t argvobj;
    argvobj = object_make_array(state);
    for(i=beginat; i<argc; i++)
    {
        strval = mc_value_makestring(state, argv[i]);
        mc_value_arraypush(argvobj, strval);
    }
    mc_state_setglobalconstant(state, "ARGV", argvobj);
}

int oldmain(int argc, char** argv)
{
    (void)argc;
    g_extfnvar = 0;
    mcstate_t* state = mc_state_make(NULL);
    mc_state_setglobalconstant(state, "test", mc_value_makenumber(42));
    mc_state_setnativefunction(state, "external_fn_test", cfn_externalfn, &g_extfnvar);
    mc_state_setnativefunction(state, "test_check_args", cfn_test_check_args, NULL);
    mc_state_setnativefunction(state, "vec2_add", cfn_vec2add, NULL);
    mc_state_setnativefunction(state, "vec2_sub", cfn_vec2sub, NULL);
    mc_cli_compileandrunfile(state, argv[1]);
    mc_state_destroy(state);
    return 0;
}

int main(int argc, char* argv[])
{
    bool ok;
    int co;
    int opt;
    int nargc;
    int longindex;
    char* nargv[128];
    char* arg;
    const char* evalcode;
    mcvalue_t tmp;
    mcstate_t* state;
    optcontext_t options;
    ok = true;
    evalcode = NULL;
    ok = true;
    nargc = 0;
    state = mc_state_make(NULL);
    optlongflags_t longopts[] =
    {
        {"help", 'h', OPTPARSE_NONE, "this help"},
        {"eval", 'e', OPTPARSE_REQUIRED, "evaluate a single line of code"},
        {0, 0, (optargtype_t)0, NULL}
    };
    nargc = 0;
    optprs_init(&options, argc, argv);
    options.permute = 0;
    while((opt = optprs_nextlongflag(&options, longopts, &longindex)) != -1)
    {
        co = longopts[longindex].shortname;
        if(opt == '?')
        {
            printf("%s: %s\n", argv[0], options.errmsg);
        }
        else if(co == 'h')
        {
            mc_cli_printusage(argv, longopts, false);
        }
        else if(co == 'e')
        {
            evalcode = options.optarg;
        }
    }
    /*
    * IMPORTANT:
    * when '-e' is specified, the structure of ARGV would /not/ include the callee (argv[0]),
    * which in scripts, would be the script file. i.e., in 'foo.mc' ARGV[0] would be 'foo.mc', et cetera.
    * this merely fills that spot whenever '-e' is being used.
    * don't remove the angle brackets either; so it doesn't get errornously picked up
    * as an option in some other place.
    */
    if(evalcode != NULL)
    {
        nargv[0] = "<-e>";
        nargc++;
    }
    while(true)
    {
        arg = optprs_nextpositional(&options);
        if(arg == NULL)
        {
            break;
        }
        nargv[nargc] = arg;
        nargc++;
    }
    fprintf(stderr, "nargc=%d\n", nargc);
    mc_cli_installargv(state, nargc, nargv, 0);
    if(evalcode != NULL)
    {
        //ok = mc_cli_runsource(state, evalcode);
        mc_cli_compileandrunsource(state, &tmp, evalcode);
    }
    else
    {
        if(nargc > 0)
        {
            //state.currscriptpath = argv[1];
            ok = mc_cli_compileandrunfile(state, nargv[0]);
        }
        else
        {
            //if(nargc == 0)
            {
                //mc_cli_runrepl(state);
            }
            ok = false;
        }
    }
    mc_state_destroy(state);
    return (ok ? 0 : 1);
}

