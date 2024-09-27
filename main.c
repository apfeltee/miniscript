

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <inttypes.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(__unix__)
    #include <unistd.h>
    #include <sys/time.h>
#endif

#include "mem.h"
#include "optparse.h"
#include "strbuf.h"

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

#if defined(NAN)
    #define MC_CONST_NAN NAN
#else
    #define MC_CONST_NAN (0.0f / 0.0f)
#endif

#if defined(INFINITY)
    #define MC_CONST_INFINITY INFINITY
#else
    #define MC_CONST_INFINITY (1e5000f)
#endif

#if !defined(va_copy)
    #define va_copy(dest, src) memcpy(&dest, &src, sizeof(va_list))
#endif


#define MC_CONF_DEBUG 0
#define MC_CONF_VMVALSTACKSIZE (4)
#define MC_CONF_VMMAXGLOBALS (1024/4)
#define MC_CONF_VMMAXFRAMES (MC_CONF_VMVALSTACKSIZE)
#define MC_CONF_VMTHISSTACKSIZE (MC_CONF_VMVALSTACKSIZE)
#define MC_CONF_NATIVEFUNCMAXDATA (24*1)
#define MC_CONF_STRINGMAXSTACKSIZE (24*1)
#define MC_CONF_GCMEMPOOLSIZE (2048/16)
#define MC_CONF_GCMEMPOOLCOUNT (3)
#define MC_CONF_GCMEMSWEEPINTERVAL (128)
#define MC_CONF_ERROR_MAXERRORCOUNT (4)
#define MC_CONF_ERROR_MSGMAXLENGTH (128)
#define MC_CONF_GENERICDICTINVALIDIX (UINT_MAX)
#define MC_CONF_VALDICTINVALIDIX (UINT_MAX)
#define MC_CONF_GENERICDICTINITSIZE (32)

#define MC_CONF_MAXOPEROVERLOADS (25)

#ifdef _MSC_VER
    #define __attribute__(x)
#endif

#if defined(__STRICT_ANSI__)
    #define MCINLINE __attribute__((always_inline))
#else
    #define MCINLINE inline __attribute__((always_inline))
#endif

#define MC_UTIL_STREQ(a, b) (strcmp((a), (b)) == 0)
#define MC_UTIL_STRNEQ(a, b, n) (strncmp((a), (b), (n)) == 0)
#define MC_UTIL_STATICARRAYSIZE(array) ((int)(sizeof(array) / sizeof(array[0])))
#define MC_UTIL_FABS(n) fabs(n)

#if 0
    #define MC_UTIL_CMPFLOAT(a, b) (MC_UTIL_FABS((a) - (b)) < DBL_EPSILON)
#else
    #define MC_UTIL_CMPFLOAT(a, b) ((a) == (b))
#endif

#if 0
    #if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
        #define MC_ASSERT(x) assert((x))
    #else
        #define MC_ASSERT(x) ((void)0)
    #endif
#else
        #define MC_ASSERT(x)
#endif


#if 0 //defined(__GNUC__) || defined(__clang__)
    #define mc_util_likely(x)   (__builtin_expect(!!(x), 1))
    #define mc_util_unlikely(x) (__builtin_expect(!!(x), 0))
#else
    #define mc_util_likely(x)   (x)
    #define mc_util_unlikely(x) (x)
#endif

#define mc_value_gettype(v) (v).type

#define MC_GROW_CAPACITY(capacity) (((capacity) < 8) ? 8 : ((capacity) * 2))

typedef double mcfloat_t;
typedef uint8_t mcinternopcode_t;


enum mcerrtype_t
{
    MC_ERROR_NONE = 0,
    MC_ERROR_PARSING,
    MC_ERROR_COMPILING,
    MC_ERROR_RUNTIME,
    MC_ERROR_TIMEOUT,
    MC_ERROR_MEMORY,
    MC_ERROR_USER
};

enum mcvaltype_t
{
    MC_VAL_NONE,
    MC_VAL_ERROR,
    MC_VAL_NUMBER,
    MC_VAL_BOOL,
    MC_VAL_STRING,
    MC_VAL_NULL,
    MC_VAL_FUNCNATIVE,
    MC_VAL_ARRAY,
    MC_VAL_MAP,
    MC_VAL_FUNCSCRIPT,
    MC_VAL_EXTERNAL,
    MC_VAL_FREED,
    /* for checking types with & */
    MC_VAL_ANY
};

enum mcasttoktype_t
{
    MC_TOK_INVALID = 0,
    MC_TOK_EOF,
    /* Operators */
    MC_TOK_ASSIGN,
    MC_TOK_ASSIGNPLUS,
    MC_TOK_ASSIGNMINUS,
    MC_TOK_ASSIGNASTERISK,
    MC_TOK_ASSIGNSLASH,
    MC_TOK_ASSIGNPERCENT,
    MC_TOK_ASSIGNBINAND,
    MC_TOK_ASSIGNBINOR,
    MC_TOK_ASSIGNBINXOR,
    MC_TOK_ASSIGNLSHIFT,
    MC_TOK_ASSIGNRSHIFT,
    MC_TOK_QUESTION,
    MC_TOK_PLUS,
    MC_TOK_PLUSPLUS,
    MC_TOK_UNARYMINUS,
    MC_TOK_MINUSMINUS,
    MC_TOK_UNARYBINNOT,
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
    MC_TOK_BINAND,
    MC_TOK_BINOR,
    MC_TOK_BINXOR,
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
    /* MUST be last. */
    MC_TOK_TYPEMAX
};

enum mcastmathoptype_t
{
    MC_MATHOP_NONE,
    MC_MATHOP_ASSIGN,
    MC_MATHOP_PLUS,
    MC_MATHOP_MINUS,
    MC_MATHOP_BINNOT,
    MC_MATHOP_BANG,
    MC_MATHOP_ASTERISK,
    MC_MATHOP_SLASH,
    MC_MATHOP_LT,
    MC_MATHOP_LTE,
    MC_MATHOP_GT,
    MC_MATHOP_GTE,
    MC_MATHOP_EQ,
    MC_MATHOP_NOTEQ,
    MC_MATHOP_MODULUS,
    MC_MATHOP_LOGICALAND,
    MC_MATHOP_LOGICALOR,
    MC_MATHOP_BINAND,
    MC_MATHOP_BINOR,
    MC_MATHOP_BINXOR,
    MC_MATHOP_LSHIFT,
    MC_MATHOP_RSHIFT
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
    MC_EXPR_STMTRECOVER
};

enum mcopcode_t
{
    MC_OPCODE_HALT = 0,
    MC_OPCODE_CONSTANT,
    MC_OPCODE_ADD,
    MC_OPCODE_SUB,
    MC_OPCODE_MUL,
    MC_OPCODE_DIV,
    MC_OPCODE_MOD,
    MC_OPCODE_POP,
    MC_OPCODE_BINOR,
    MC_OPCODE_BINXOR,
    MC_OPCODE_BINAND,
    MC_OPCODE_LSHIFT,
    MC_OPCODE_RSHIFT,
    MC_OPCODE_BANG,
    MC_OPCODE_COMPARE,
    MC_OPCODE_TRUE,
    MC_OPCODE_FALSE,
    MC_OPCODE_COMPAREEQ,
    MC_OPCODE_EQUAL,
    MC_OPCODE_NOTEQUAL,
    MC_OPCODE_GREATERTHAN,
    MC_OPCODE_GREATERTHANEQUAL,
    MC_OPCODE_MINUS,
    MC_OPCODE_BINNOT,
    MC_OPCODE_JUMP,
    MC_OPCODE_JUMPIFFALSE,
    MC_OPCODE_JUMPIFTRUE,
    MC_OPCODE_NULL,
    MC_OPCODE_GETMODULEGLOBAL,
    MC_OPCODE_SETMODULEGLOBAL,
    MC_OPCODE_DEFINEMODULEGLOBAL,
    MC_OPCODE_ARRAY,
    MC_OPCODE_MAPSTART,
    MC_OPCODE_MAPEND,
    MC_OPCODE_GETTHIS,
    MC_OPCODE_GETINDEX,
    MC_OPCODE_SETINDEX,
    MC_OPCODE_GETDOTINDEX,
    MC_OPCODE_GETVALUEAT,
    MC_OPCODE_CALL,
    MC_OPCODE_RETURNVALUE,
    MC_OPCODE_RETURN,
    MC_OPCODE_GETLOCAL,
    MC_OPCODE_DEFINELOCAL,
    MC_OPCODE_SETLOCAL,
    MC_OPCODE_GETGLOBALBUILTIN,
    MC_OPCODE_FUNCTION,
    MC_OPCODE_GETFREE,
    MC_OPCODE_SETFREE,
    MC_OPCODE_CURRENTFUNCTION,
    MC_OPCODE_DUP,
    MC_OPCODE_NUMBER,
    MC_OPCODE_FOREACHLEN,
    MC_OPCODE_SETRECOVER,
    MC_OPCODE_MAX
};

enum mcastsymtype_t
{
    MC_SYM_NONE = 0,
    MC_SYM_MODULEGLOBAL,
    MC_SYM_LOCAL,
    MC_SYM_GLOBALBUILTIN,
    MC_SYM_FREE,
    MC_SYM_FUNCTION,
    MC_SYM_THIS
};

enum mcastprecedence_t
{
    MC_ASTPREC_LOWEST = 0,
    MC_ASTPREC_ASSIGN,
    /* a = b */
    MC_ASTPREC_TERNARY,
    /* a ? b : c */
    MC_ASTPREC_LOGICALOR,
    /* || */
    MC_ASTPREC_LOGICALAND,
    /* && */
    MC_ASTPREC_BINOR,
    /* | */
    MC_ASTPREC_BINXOR,
    /* ^ */
    MC_ASTPREC_BINAND,
    /* & */
    MC_ASTPREC_EQUALS,
    /* == != */
    MC_ASTPREC_LESSGREATER,
    /* >, >=, <, <= */
    MC_ASTPREC_SHIFT,
    /* << >> */
    MC_ASTPREC_SUM,
    /* + - */
    MC_ASTPREC_PRODUCT,
    /* * / % */
    MC_ASTPREC_PREFIX,
    /* -x !x ++x --x */
    MC_ASTPREC_INCDEC,
    /* x++ x-- */
    MC_ASTPREC_POSTFIX,
    /* myFunction(x) x["foo"] x.foo */
    MC_ASTPREC_HIGHEST
};

typedef enum mcerrtype_t mcerrtype_t;
typedef enum mcvaltype_t mcvaltype_t;
typedef enum mcasttoktype_t mcasttoktype_t;
typedef enum mcastmathoptype_t mcastmathoptype_t;
typedef enum mcastexprtype_t mcastexprtype_t;
typedef enum mcopcode_t mcopcode_t;
typedef enum mcastsymtype_t mcastsymtype_t;
typedef enum mcastprecedence_t mcastprecedence_t;

typedef struct mcgenericdict_t mcgenericdict_t;
typedef struct mcvaldict_t mcvaldict_t;
typedef struct mcbasicarray_t mcbasicarray_t;
typedef struct mcptrlist_t mcptrlist_t;
typedef struct mcprintconfig_t mcprintconfig_t;
typedef struct mcprinter_t mcprinter_t;
typedef struct mcastprinter_t mcastprinter_t;
typedef struct mcerror_t mcerror_t;
typedef struct mctraceback_t mctraceback_t;
typedef struct mcastcompiledfile_t mcastcompiledfile_t;
typedef struct mcastexpression_t mcastexpression_t;
typedef struct mccompiledprogram_t mccompiledprogram_t;
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
typedef struct mcvalcmpresult_t mcvalcmpresult_t;
typedef struct mcastlocation_t mcastlocation_t;
typedef struct mcasttoken_t mcasttoken_t;
typedef struct mcastcodeblock_t mcastcodeblock_t;
typedef struct mcastliteralmap_t mcastliteralmap_t;
typedef struct mcastliteralarray_t mcastliteralarray_t;
typedef struct mcastliteralstring_t mcastliteralstring_t;
typedef struct mcastexprprefix_t mcastexprprefix_t;
typedef struct mcastexprinfix_t mcastexprinfix_t;
typedef struct mcastifcase_t mcastifcase_t;
typedef struct mcastliteralfunction_t mcastliteralfunction_t;
typedef struct mcastexprcall_t mcastexprcall_t;
typedef struct mcastexprindex_t mcastexprindex_t;
typedef struct mcastexprassign_t mcastexprassign_t;
typedef struct mcastexprlogical_t mcastexprlogical_t;
typedef struct mcastexprternary_t mcastexprternary_t;
typedef struct mcastident_t mcastident_t;
typedef struct mcastfuncparam_t mcastfuncparam_t;

typedef struct mcastexprdefine_t mcastexprdefine_t;
typedef struct mcastexprstmtif_t mcastexprstmtif_t;
typedef struct mcastexprstmtwhile_t mcastexprstmtwhile_t;
typedef struct mcastexprstmtforeach_t mcastexprstmtforeach_t;
typedef struct mcastexprstmtforloop_t mcastexprstmtforloop_t;
typedef struct mcastexprstmtimport_t mcastexprstmtimport_t;
typedef struct mcastexprstmtrecover_t mcastexprstmtrecover_t;

typedef struct mcobjfuncscript_t mcobjfuncscript_t;
typedef struct mcobjfuncnative_t mcobjfuncnative_t;
typedef struct mcobjuserdata_t mcobjuserdata_t;
typedef struct mcobjerror_t mcobjerror_t;
typedef struct mcobjstring_t mcobjstring_t;


typedef struct mcopdefinition_t mcopdefinition_t;

typedef struct mcastscopeblock_t mcastscopeblock_t;
typedef struct mcastscopefile_t mcastscopefile_t;
typedef struct mcastscopecomp_t mcastscopecomp_t;

typedef struct mcgcobjdatapool_t mcgcobjdatapool_t;

typedef struct mcastlexer_t mcastlexer_t;
typedef struct mcastlexprevinfo_t mcastlexprevinfo_t;
typedef struct mcvmframe_t mcvmframe_t;
typedef struct mctraceitem_t mctraceitem_t;
typedef struct module_t module_t;
typedef struct mcstoddiyfp_t mcstoddiyfp_t;

typedef union mcstoddiyfpconv_t mcstoddiyfpconv_t;
typedef union mcvalunion_t mcvalunion_t;
typedef union mcobjunion_t mcobjunion_t;
typedef union mcexprunion_t mcexprunion_t;
typedef union mcfuncfvunion_t mcfuncfvunion_t;
typedef union mcfuncnameunion_t mcfuncnameunion_t;
typedef struct /**/mcvallist_t mcvallist_t;
typedef struct /**/mcframelist_t mcframelist_t;

typedef mcvalue_t (*mcnativefn_t)(mcstate_t*, void*, int, mcvalue_t*);
typedef unsigned long (*mcitemhashfn_t)(void*);
typedef bool (*mcitemcomparefn_t)(void*, void*);
typedef void (*mcitemdestroyfn_t)(void*);
typedef void* (*mcitemcopyfn_t)(void*);
typedef void (*mcitemdeinitfn_t)(void*);
typedef mcastexpression_t* (*mcastrightassocparsefn_t)(mcastparser_t*);
typedef mcastexpression_t* (*mcleftassocparsefn_t)(mcastparser_t*, mcastexpression_t*);

struct mcframelist_t
{
    mcstate_t* pstate;
    size_t listcapacity;
    size_t listcount;
    mcvmframe_t* listitems;
};

struct mcvallist_t
{
    mcstate_t* pstate;
    size_t listcapacity;
    size_t listcount;
    const char* listname;
    mcvalue_t* listitems;
};

struct mcstoddiyfp_t
{
    uint64_t significand;
    int exp;
};

union mcstoddiyfpconv_t
{
    double d;
    uint64_t u64;
};

/**
 * \brief The execution environment for an instance of the script engine.
 */

union mcvalunion_t
{
    mcobjdata_t* odata;
    mcfloat_t valnumber;
    bool valbool;
};

struct mcvalue_t
{
    mcvaltype_t type;
    bool isallocated;
    mcvalunion_t uval;
};

struct mcvalcmpresult_t
{
    mcfloat_t result;
};

struct mcastlocation_t
{
    mcastcompiledfile_t* file;
    int line;
    int column;
};

struct mcconfig_t
{
    bool dumpast;
    bool dumpbytecode;
    bool fatalcomplaints;
    /* allows redefinition of symbols */
    bool replmode;
};

struct mcasttoken_t
{
    mcasttoktype_t type;
    const char* literal;
    int len;
    mcastlocation_t pos;
};

struct mcastcodeblock_t
{
    mcstate_t* pstate;
    mcptrlist_t* statements;
};

struct mcastliteralmap_t
{
    mcptrlist_t* keys;
    mcptrlist_t* values;
};

struct mcastliteralarray_t
{
    mcptrlist_t* litarritems;
};

struct mcastliteralstring_t
{
    size_t length;
    char* data;
};

struct mcastexprprefix_t
{
    mcastmathoptype_t op;
    mcastexpression_t* right;
};

struct mcastexprinfix_t
{
    mcastmathoptype_t op;
    mcastexpression_t* left;
    mcastexpression_t* right;
};

struct mcastifcase_t
{
    mcstate_t* pstate;
    mcastexpression_t* ifcond;
    mcastcodeblock_t* consequence;
};

struct mcastliteralfunction_t
{
    char* name;
    mcptrlist_t* funcparamlist;
    mcastcodeblock_t* body;
};

struct mcastexprcall_t
{
    mcastexpression_t* function;
    mcptrlist_t* args;
};

struct mcastexprindex_t
{
    bool isdot;
    mcastexpression_t* left;
    mcastexpression_t* index;
};

struct mcastexprassign_t
{
    mcastexpression_t* dest;
    mcastexpression_t* source;
    bool is_postfix;
};

struct mcastexprlogical_t
{
    mcastmathoptype_t op;
    mcastexpression_t* left;
    mcastexpression_t* right;
};

struct mcastexprternary_t
{
    mcastexpression_t* tercond;
    mcastexpression_t* teriftrue;
    mcastexpression_t* teriffalse;
};

struct mcastident_t
{
    mcstate_t* pstate;
    char* value;
    mcastlocation_t pos;
};

struct mcastfuncparam_t
{
    mcstate_t* pstate;
    mcastident_t* ident;
};

struct mcastexprdefine_t
{
    mcastident_t* name;
    mcastexpression_t* value;
    bool assignable;
};

struct mcastexprstmtif_t
{
    mcptrlist_t* cases;
    mcastcodeblock_t* alternative;
};

struct mcastexprstmtwhile_t
{
    mcastexpression_t* loopcond;
    mcastcodeblock_t* body;
};

struct mcastexprstmtforeach_t
{
    mcastident_t* iterator;
    mcastexpression_t* source;
    mcastcodeblock_t* body;
};

struct mcastexprstmtforloop_t
{
    mcastexpression_t* init;
    mcastexpression_t* loopcond;
    mcastexpression_t* update;
    mcastcodeblock_t* body;
};

struct mcastexprstmtimport_t
{
    char* path;
};

struct mcastexprstmtrecover_t
{
    mcastident_t* errident;
    mcastcodeblock_t* body;
};

union mcexprunion_t
{
    mcastident_t* exprident;
    mcfloat_t exprlitnumber;
    bool exprlitbool;
    mcastliteralstring_t exprlitstring;
    mcastliteralarray_t exprlitarray;
    mcastliteralmap_t exprlitmap;
    mcastexprprefix_t exprprefix;
    mcastexprinfix_t exprinfix;
    mcastliteralfunction_t exprlitfunction;
    mcastexprcall_t exprcall;
    mcastexprindex_t exprindex;
    mcastexprassign_t exprassign;
    mcastexprlogical_t exprlogical;
    mcastexprternary_t exprternary;
    mcastexprdefine_t exprdefine;
    mcastexprstmtif_t exprifstmt;
    mcastexpression_t* exprreturnvalue;
    mcastexpression_t* exprexpression;
    mcastexprstmtwhile_t exprwhileloopstmt;
    mcastexprstmtforeach_t exprforeachloopstmt;
    mcastexprstmtforloop_t exprforloopstmt;
    mcastcodeblock_t* exprblockstmt;
    mcastexprstmtimport_t exprimportstmt;
    mcastexprstmtrecover_t exprrecoverstmt;
};

struct mcastexpression_t
{
    mcstate_t* pstate;
    mcastexprtype_t type;
    mcexprunion_t uexpr;
    mcastlocation_t pos;
};

union mcfuncfvunion_t
{
    mcvalue_t* freevalsallocated;
    mcvalue_t freevalsstack[2];
};

union mcfuncnameunion_t
{
    char* fallocname;
    const char* fconstname;
};

struct mcobjfuncscript_t
{
    mcfuncfvunion_t ufv;
    mcfuncnameunion_t unamev;
    mccompiledprogram_t* compiledprogcode;
    int numlocals;
    int numargs;
    int freevalscount;
    bool ownsdata;

};

struct mcobjfuncnative_t
{
    char* name;
    mcnativefn_t natptrfn;
    uint8_t userdata[MC_CONF_NATIVEFUNCMAXDATA];
    int userdlen;
};

struct mcobjuserdata_t
{
    void* data;
    mcitemdestroyfn_t datadestroyfn;
    mcitemcopyfn_t datacopyfn;
};

struct mcobjerror_t
{
    char* message;
    mctraceback_t* traceback;
};


struct mcobjstring_t
{
    unsigned long hash;
    StringBuffer* strbuf;
};


union mcobjunion_t
{
    mcobjstring_t valstring;
    mcobjerror_t valerror;
    mcvallist_t* valarray;
    mcvaldict_t* valmap;
    mcobjfuncscript_t valscriptfunc;
    mcobjfuncnative_t valnativefunc;
    mcobjuserdata_t valuserobject;
};

struct mcobjdata_t
{
    mcstate_t* pstate;
    mcgcmemory_t* mem;
    mcvaltype_t type;
    mcobjunion_t uvobj;
    bool gcmark;
};

struct mcopdefinition_t
{
    const char* name;
    int numoperands;
    int operandwidths[2];

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
    int numdefinitions;
};

struct mcastsymtable_t
{
    mcstate_t* pstate;
    mcastsymtable_t* outer;
    mcglobalstore_t* symglobalstore;
    mcptrlist_t* blockscopes;
    mcptrlist_t* freesymbols;
    mcptrlist_t* modglobalsymbols;
    int maxnumdefinitions;
    int modglobaloffset;
};

struct mcgcobjdatapool_t
{
    mcobjdata_t* data[MC_CONF_GCMEMPOOLSIZE];
    int count;
};

struct mcgcmemory_t
{
    mcstate_t* pstate;
    int allocssincesweep;
    mcptrlist_t* gcobjlist;
    mcptrlist_t* gcobjlistback;
    mcbasicarray_t* gcobjlistremains;
    mcgcobjdatapool_t onlydatapool;
    mcgcobjdatapool_t mempools[MC_CONF_GCMEMPOOLCOUNT];
};

struct mccompiledprogram_t
{
    mcstate_t* pstate;
    uint8_t* bytecode;
    mcastlocation_t* progsrcposlist;
    int count;
};

struct mcastscopecomp_t
{
    mcstate_t* pstate;
    mcastscopecomp_t* outer;
    mcbasicarray_t* bytecode;
    mcbasicarray_t* scopesrcposlist;
    mcbasicarray_t* ipstackbreak;
    mcbasicarray_t* ipstackcontinue;
    mcinternopcode_t lastopcode;
};

struct mcastcompiledfile_t
{
    mcstate_t* pstate;
    char* dir_path;
    char* path;
    mcptrlist_t* lines;
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

struct mcastlexprevinfo_t
{
    int position;
    int nextposition;
    char ch;
    int line;
    int column;
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
    size_t line;
    size_t column;
    mcastcompiledfile_t* file;
    bool failed;
    bool continuetplstring;
    mcastlexprevinfo_t prevstate;
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
    int64_t bcposition;
    int64_t basepointer;
    mcastlocation_t* framesrcposlist;
    uint8_t* bytecode;
    int64_t sourcebcpos;
    int64_t bcsize;
    int64_t recoverip;
    bool isrecovering;
};

struct mctraceitem_t
{
    char* trfuncname;
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
    mcglobalstore_t* vmglobalstore;
    mcvalue_t globalvalstack[MC_CONF_VMMAXGLOBALS];
    //mcvallist_t* globalvalstack;
    int globalvalcount;
    //mcvalue_t valuestack[MC_CONF_VMVALSTACKSIZE];
    mcvallist_t* valuestack;
    int vsposition;
    //mcvalue_t valthisstack[MC_CONF_VMTHISSTACKSIZE];
    mcvallist_t* valthisstack;
    int thisstpos;
    //mcvmframe_t framestack[MC_CONF_VMMAXFRAMES];
    mcframelist_t* framestack;
    int framecount;
    mcvalue_t lastpopped;
    mcvmframe_t* currframe;
    bool running;
    mcvalue_t operoverloadkeys[MC_CONF_MAXOPEROVERLOADS];
    mcptrlist_t* files;
    mcastcompiler_t* compiler;
    mcprinter_t* stdoutprinter;
    mcprinter_t* stderrprinter;
};

struct mcglobalstore_t
{
    mcstate_t* pstate;
    mcgenericdict_t* symbols;
    mcbasicarray_t* objects;
};

struct mcprintconfig_t
{
    bool verbosefunc;
    bool quotstring;
    bool shouldflush;
};

struct mcprinter_t
{
    mcstate_t* pstate;
    mcprintconfig_t config;
    bool failed;
    bool onstack;
    FILE* destfile;
    char* data;
    size_t capacity;
    size_t len;
};

struct mcastprinter_t
{
    mcstate_t* pstate;
    mcprinter_t* pdest;
    bool pseudolisp;
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

struct mcptrlist_t
{
    mcstate_t* pstate;
    size_t listcapacity;
    size_t listcount;
    size_t livecount;
    void** listitems;
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
    mcitemhashfn_t funchashfn;
    mcitemcomparefn_t funckeyequalsfn;
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
    mcitemcopyfn_t funccopyfn;
    mcitemdestroyfn_t funcdestroyfn;
};


struct module_t
{
    mcstate_t* pstate;
    char* name;
    mcptrlist_t* symbols;
};

struct mcastscopefile_t
{
    mcstate_t* pstate;
    mcastparser_t* parser;
    mcastsymtable_t* filesymtab;
    mcastcompiledfile_t* file;
    mcptrlist_t* loadedmodnames;
};

struct mcastcompiler_t
{
    mcstate_t* pstate;
    mcconfig_t* config;
    mcgcmemory_t* mem;
    mcerrlist_t* errors;
    mcptrlist_t* files;
    mcglobalstore_t* compglobalstore;
    mcbasicarray_t* constants;
    mcastscopecomp_t* compilationscope;
    mcptrlist_t* filescopelist;
    mcbasicarray_t* srcposstack;
    mcgenericdict_t* modules;
    mcgenericdict_t* stringconstposdict;
};

#if 1
    #include "prot.inc"
#else
    #include "tmp.h"
#endif

#if defined(__STRICT_ANSI__)
extern int vsnprintf (char* destb, size_t maxlen, const char* fmt, va_list va);
#endif

/* endheader */
const mcastlocation_t srcposinvalid = { NULL, -1, -1 };


void* mc_allocator_malloc(mcstate_t* state, size_t size)
{
    (void)state;
    return mc_memory_malloc(size);
}

void* mc_allocator_realloc(mcstate_t* state, void* ptr, size_t size)
{
    (void)state;
    return mc_memory_realloc(ptr, size);
}

void mc_allocator_free(mcstate_t* state, void* ptr)
{
    (void)state;
    mc_memory_free(ptr);
}

char* mc_util_readhandle(FILE* hnd, size_t* dlen)
{
    long rawtold;
    /*
    * the value returned by ftell() may not necessarily be the same as
    * the amount that can be read.
    * since we only ever read a maximum of $toldlen, there will
    * be no memory trashing.
    */
    size_t toldlen;
    size_t actuallen;
    char* buf;
    if(fseek(hnd, 0, SEEK_END) == -1)
    {
        return NULL;
    }
    if((rawtold = ftell(hnd)) == -1)
    {
        return NULL;
    }
    toldlen = rawtold;
    if(fseek(hnd, 0, SEEK_SET) == -1)
    {
        return NULL;
    }
    buf = (char*)mc_memory_malloc(toldlen + 1);
    memset(buf, 0, toldlen+1);
    if(buf != NULL)
    {
        actuallen = fread(buf, sizeof(char), toldlen, hnd);
        /*
        // optionally, read remainder:
        size_t tmplen;
        if(actuallen < toldlen)
        {
            tmplen = actuallen;
            actuallen += fread(buf+tmplen, sizeof(char), actuallen-toldlen, hnd);
            ...
        }
        // unlikely to be necessary, so not implemented.
        */
        if(dlen != NULL)
        {
            *dlen = actuallen;
        }
        return buf;
    }
    return NULL;
}

char* mc_util_readfile(const char* filename, size_t* dlen)
{
    char* b;
    FILE* fh;
    if((fh = fopen(filename, "rb")) == NULL)
    {
        return NULL;
    }
    b = mc_util_readhandle(fh, dlen);
    fclose(fh);
    return b;
}


char* mc_fsutil_fileread(mcstate_t* state, const char* filename, size_t* flen)
{
    (void)state;
    return mc_util_readfile(filename, flen);
}

size_t mc_fsutil_filewrite(mcstate_t* state, const char* path, const char* string, size_t stringsize)
{
    size_t written;
    FILE* fp;
    (void)state;
    fp = fopen(path, "w");
    if(!fp)
    {
        return 0;
    }
    written = fwrite(string, 1, stringsize, fp);
    fclose(fp);
    return written;
}

bool mc_fsutil_fileexists(mcstate_t* state, const char* filename)
{
    struct stat st;
    (void)state;
    if(stat(filename, &st) == -1)
    {
        return false;
    }
    return true;
}

size_t mc_util_strlen(const char* str)
{
    size_t len;
    if(str == NULL)
    {
        return 0;
    }
    for(len=0; str[len] != 0; len++)
    {
    }
    return len;
}

char* mc_util_strndup(mcstate_t* state, const char* string, size_t n)
{
    char* outputstring;
    outputstring = (char*)mc_allocator_malloc(state, n + 1);
    if(!outputstring)
    {
        return NULL;
    }
    outputstring[n] = '\0';
    memcpy(outputstring, string, n);
    return outputstring;
}

char* mc_util_strdup(mcstate_t* state, const char* string)
{
    if(!string)
    {
        return NULL;
    }
    return mc_util_strndup(state, string, mc_util_strlen(string));
}


unsigned long mc_util_hashdata(const void* ptr, size_t len)
{
    /* djb2 */
    size_t i;
    unsigned long hash;
    uint8_t val;
    const uint8_t* up;
    up = (const uint8_t*)ptr;
    hash = 5381;
    for(i = 0; i < len; i++)
    {
        val = up[i];
        hash = ((hash << 5) + hash) + val;
    }
    return hash;
}

unsigned long mc_util_hashdouble(mcfloat_t val)
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

unsigned int mc_util_upperpowoftwo(unsigned int v)
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

#include "stod.h"

mcfloat_t mc_util_strtod(const char* str, size_t slen, char** endptr)
{
    const unsigned char* p;
    const unsigned char* end; 
    p = (const unsigned char*)str;
    end = (const unsigned char*)str + slen;
    if((str[0] == '0') && (str[1] == 'x'))
    {
        return strtod(str, endptr);
    }
    if(endptr != NULL)
    {
        *endptr = (char*)end;
    }
    return stod_strtod(&p, end, true);
}

typedef uint32_t mcshiftint_t;
MCINLINE mcfloat_t mc_mathutil_binshiftleft(mcfloat_t dnleft, mcfloat_t dnright)
{
    int64_t sileft;
    int64_t siright;
    mcshiftint_t ivleft;
    mcshiftint_t ivright;
    if((dnleft < 0) || (dnright < 0))
    {
        sileft = (int64_t)dnleft;
        siright = (int64_t)dnright;
        return (mcfloat_t)(sileft << siright);
    }
    ivleft = (int32_t)dnleft;
    ivright = (mcshiftint_t)dnright;
    ivright &= 0x1f;
    return (mcfloat_t)(ivleft << ivright);
}

MCINLINE mcfloat_t mc_mathutil_binshiftright(mcfloat_t dnleft, mcfloat_t dnright)
{
    int64_t sileft;
    int64_t siright;
    mcshiftint_t ivleft;
    mcshiftint_t ivright;
    if((dnleft < 0) || (dnright < 0))
    {
        sileft = (int64_t)dnleft;
        siright = (int64_t)dnright;
        return (mcfloat_t)(sileft >> siright);
    }
    ivleft = (mcshiftint_t)dnleft;
    ivright = (mcshiftint_t)dnright;
    ivright &= 0x1f;
    return (mcfloat_t)(ivleft >> ivright);
}

MCINLINE mcfloat_t mc_mathutil_binor(mcfloat_t dnleft, mcfloat_t dnright)
{
    int64_t ivleft;
    int64_t ivright;
    ivleft = (int64_t)dnleft;
    ivright = (int64_t)dnright;
    return (mcfloat_t)(ivleft | ivright);
}

MCINLINE mcfloat_t mc_mathutil_binand(mcfloat_t dnleft, mcfloat_t dnright)
{
    int64_t ivleft;
    int64_t ivright;
    ivleft = (int64_t)dnleft;
    ivright = (int64_t)dnright;
    return (mcfloat_t)(ivleft & ivright);
}

MCINLINE mcfloat_t mc_mathutil_binxor(mcfloat_t dnleft, mcfloat_t dnright)
{
    int64_t ivleft;
    int64_t ivright;
    ivleft = (int64_t)dnleft;
    ivright = (int64_t)dnright;
    return (mcfloat_t)(ivleft ^ ivright);
}

MCINLINE mcfloat_t mc_mathutil_add(mcfloat_t dnleft, mcfloat_t dnright)
{
    return dnleft + dnright;
}

MCINLINE mcfloat_t mc_mathutil_sub(mcfloat_t dnleft, mcfloat_t dnright)
{
    return dnleft - dnright;
}

MCINLINE mcfloat_t mc_mathutil_mult(mcfloat_t dnleft, mcfloat_t dnright)
{
    return dnleft * dnright;
}

MCINLINE mcfloat_t mc_mathutil_div(mcfloat_t dnleft, mcfloat_t dnright)
{
    return dnleft / dnright;
}

MCINLINE mcfloat_t mc_mathutil_mod(mcfloat_t dnleft, mcfloat_t dnright)
{
    return fmod(dnleft, dnright);
}

mcvallist_t* mc_vallist_make(mcstate_t* state, const char* name, size_t initialsize)
{
    mcvallist_t* list;
    list = (mcvallist_t*)mc_allocator_malloc(state, sizeof(mcvallist_t));
    list->pstate = state;
    list->listcount = 0;
    list->listcapacity = 0;
    list->listitems = NULL;
    list->listname = name;
    if(initialsize > 0)
    {
        mc_vallist_ensurecapacity(list, initialsize, mc_value_makenull(), true);
    }
    return list;
}

size_t mc_vallist_count(mcvallist_t* list)
{
    return list->listcount;
}

size_t mc_vallist_capacity(mcvallist_t* list)
{
    return list->listcount;
}

void mc_vallist_setempty(mcvallist_t* list)
{
    if((list->listcapacity > 0) && (list->listitems != NULL))
    {
        memset(list->listitems, 0, sizeof(mcvalue_t) * list->listcapacity);
    }
    list->listcount = 0;
    list->listcapacity = 0;
}

void mc_vallist_destroy(mcvallist_t* list)
{
    mcstate_t* state;
    state = list->pstate;
    if(list->listname != NULL)
    {
        fprintf(stderr, "vallist of '%s' use at end: count=%ld capacity=%ld\n", list->listname, list->listcount, list->listcapacity);
    }
    if(list != NULL)
    {
        mc_allocator_free(state, list->listitems);
        mc_allocator_free(state, list);
        list = NULL;
    }
}

mcvalue_t mc_vallist_get(mcvallist_t* list, size_t idx)
{
    return list->listitems[idx];
}

mcvalue_t* mc_vallist_getp(mcvallist_t* list, size_t idx)
{
    return &list->listitems[idx];
}

bool mc_vallist_set(mcvallist_t* list, size_t idx, mcvalue_t val)
{
    size_t need;
    //need = MC_GROW_CAPACITY(list->listcapacity);
    need = idx + 8;
    if(((idx == 0) || (list->listcapacity == 0)) || (idx >= list->listcapacity))
    {
        mc_vallist_ensurecapacity(list, need, mc_value_makenull(), false);
    }
    list->listitems[idx] = val;
    if(idx > list->listcount)
    {
        list->listcount = idx;
    }
    return true;
}

bool mc_vallist_push(mcvallist_t* list, mcvalue_t value)
{
    size_t oldcap;
    #if 1
        if(list->listcapacity < list->listcount + 1)
        {
            oldcap = list->listcapacity;
            list->listcapacity = MC_GROW_CAPACITY(oldcap);
            if(list->listitems == NULL)
            {
                list->listitems = (mcvalue_t*)mc_allocator_malloc(list->pstate, sizeof(mcvalue_t) * list->listcapacity);
            }
            else
            {
                list->listitems = (mcvalue_t*)mc_allocator_realloc(list->pstate, list->listitems, sizeof(mcvalue_t) * list->listcapacity);
            }
        }
    #endif
    list->listitems[list->listcount] = value;
    list->listcount++;
    return true;
}

bool mc_vallist_pop(mcvallist_t* list, mcvalue_t* dest)
{
    if(list->listcount > 0)
    {
        *dest = list->listitems[list->listcount - 1];
        list->listcount--;
        return true;
    }
    return false;
}

bool mc_vallist_removeat(mcvallist_t* arr, unsigned int ix)
{
    size_t tomovebytes;
    void* dest;
    void* src;
    if(ix >= arr->listcount)
    {
        return false;
    }
    if(ix == 0)
    {
        arr->listitems += sizeof(mcvalue_t);
        arr->listcapacity--;
        arr->listcount--;
        return true;
    }
    if(ix == (arr->listcount - 1))
    {
        arr->listcount--;
        return true;
    }
    tomovebytes = (arr->listcount - 1 - ix) * sizeof(mcvalue_t);
    dest = arr->listitems + (ix * sizeof(mcvalue_t));
    src = arr->listitems + ((ix + 1) * sizeof(mcvalue_t));
    memmove(dest, src, tomovebytes);
    arr->listcount--;
    return true;
}

void mc_vallist_ensurecapacity(mcvallist_t* list, size_t needsize, mcvalue_t fillval, bool first)
{
    size_t i;
    size_t ncap;
    size_t oldcap;
    (void)first;
    if(list->listcapacity < needsize)
    {
        oldcap = list->listcapacity;
        //ncap = needsize;
        ncap = (list->listcount + needsize + 15) / 16 * 16;
        list->listcapacity = ncap;
        if(list->listitems == NULL)
        {
            list->listitems = (mcvalue_t*)mc_allocator_malloc(list->pstate, sizeof(mcvalue_t) * ncap);
        }
        else
        {
            list->listitems = (mcvalue_t*)mc_allocator_realloc(list->pstate, list->listitems, sizeof(mcvalue_t) * ncap);
        }
        for(i = oldcap; i < ncap; i++)
        {
            list->listitems[i] = fillval;
        }
    }
}

mcframelist_t* mc_framelist_make(mcstate_t* state, size_t initialsize)
{
    mcframelist_t* list;
    mcvmframe_t nullframe = {};
    list = (mcframelist_t*)mc_allocator_malloc(state, sizeof(mcframelist_t));
    list->pstate = state;
    list->listcount = 0;
    list->listcapacity = 0;
    list->listitems = NULL;
    if(initialsize > 0)
    {
        mc_framelist_ensurecapacity(list, initialsize, nullframe, true);
    }
    return list;
}

size_t mc_framelist_count(mcframelist_t* list)
{
    return list->listcount;
}

size_t mc_framelist_capacity(mcframelist_t* list)
{
    return list->listcount;
}

void mc_framelist_destroy(mcframelist_t* list)
{
    mcstate_t* state;
    state = list->pstate;
    fprintf(stderr, "framelist use at end: count=%ld capacity=%ld\n", list->listcount, list->listcapacity);
    if(list != NULL)
    {
        mc_allocator_free(state, list->listitems);
        mc_allocator_free(state, list);
        list = NULL;
    }
}

mcvmframe_t* mc_framelist_get(mcframelist_t* list, size_t idx)
{
    return &list->listitems[idx];
}

mcvmframe_t* mc_framelist_set(mcframelist_t* list, size_t idx, mcvmframe_t val)
{
    size_t need;
    mcvmframe_t nullframe = {};
    //need = MC_GROW_CAPACITY(list->listcapacity);
    need = idx + 8;
    if(((idx == 0) || (list->listcapacity == 0)) || (idx >= list->listcapacity))
    {
        mc_framelist_ensurecapacity(list, need, nullframe, false);
    }
    list->listitems[idx] = val;
    if(idx > list->listcount)
    {
        list->listcount = idx;
    }
    return &list->listitems[idx];
}

void mc_framelist_push(mcframelist_t* list, mcvmframe_t value)
{
    size_t oldcap;
    if(list->listcapacity < list->listcount + 1)
    {
        oldcap = list->listcapacity;
        list->listcapacity = MC_GROW_CAPACITY(oldcap);
        if(list->listitems == NULL)
        {
            list->listitems = (mcvmframe_t*)mc_allocator_malloc(list->pstate, sizeof(mcvmframe_t) * list->listcapacity);
        }
        else
        {
            list->listitems = (mcvmframe_t*)mc_allocator_realloc(list->pstate, list->listitems, sizeof(mcvmframe_t) * list->listcapacity);
        }
    }
    list->listitems[list->listcount] = value;
    list->listcount++;
}


void mc_framelist_ensurecapacity(mcframelist_t* list, size_t needsize, mcvmframe_t fillval, bool first)
{
    size_t i;
    size_t ncap;
    size_t oldcap;
    (void)first;
    if(list->listcapacity < needsize)
    {
        oldcap = list->listcapacity;
        //ncap = needsize;
        ncap = (list->listcount + needsize + 15) / 16 * 16;
        list->listcapacity = ncap;
        if(list->listitems == NULL)
        {
            list->listitems = (mcvmframe_t*)mc_allocator_malloc(list->pstate, sizeof(mcvmframe_t) * ncap);
        }
        else
        {
            list->listitems = (mcvmframe_t*)mc_allocator_realloc(list->pstate, list->listitems, sizeof(mcvmframe_t) * ncap);
        }
        for(i = oldcap; i < ncap; i++)
        {
            list->listitems[i] = fillval;
        }
    }
}

mcgenericdict_t* mc_genericdict_make(mcstate_t* state, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
{
    bool ok;
    mcgenericdict_t* dict;
    dict = (mcgenericdict_t*)mc_allocator_malloc(state, sizeof(mcgenericdict_t));
    if(dict == NULL)
    {
        return NULL;
    }
    ok = mc_genericdict_init(dict, state, MC_CONF_GENERICDICTINITSIZE, copyfn, dfn);
    if(!ok)
    {
        mc_allocator_free(state, dict);
        return NULL;
    }
    dict->pstate = state;
    return dict;
}

void mc_genericdict_destroy(mcgenericdict_t* dict)
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

void mc_genericdict_destroyitemsanddict(mcgenericdict_t* dict)
{
    unsigned int i;
    if(!dict)
    {
        return;
    }
    if(dict->funcdestroyfn)
    {
        for(i = 0; i < dict->count; i++)
        {
            dict->funcdestroyfn(dict->values[i]);
        }
    }
    mc_genericdict_destroy(dict);
}

mcgenericdict_t* mc_genericdict_copy(mcgenericdict_t* dict)
{
    bool ok;
    size_t i;
    void* item;
    void* itemcopy;
    const char* key;
    mcgenericdict_t* dictcopy;
    if(!dict->funccopyfn || !dict->funcdestroyfn)
    {
        return NULL;
    }
    dictcopy = mc_genericdict_make(dict->pstate, dict->funccopyfn, dict->funcdestroyfn);
    if(!dictcopy)
    {
        return NULL;
    }
    dictcopy->pstate = dict->pstate;
    for(i = 0; i < mc_genericdict_count(dict); i++)
    {
        key = mc_genericdict_getkeyat(dict, i);
        item = mc_genericdict_getvalueat(dict, i);
        itemcopy = dictcopy->funccopyfn(item);
        if(item && !itemcopy)
        {
            mc_genericdict_destroyitemsanddict(dictcopy);
            return NULL;
        }
        ok = mc_genericdict_set(dictcopy, key, itemcopy);
        if(!ok)
        {
            dictcopy->funcdestroyfn(itemcopy);
            mc_genericdict_destroyitemsanddict(dictcopy);
            return NULL;
        }
    }
    return dictcopy;
}

bool mc_genericdict_set(mcgenericdict_t* dict, const char* key, void* value)
{
    return mc_genericdict_setinternal(dict, key, NULL, value);
}

void* mc_genericdict_get(mcgenericdict_t* dict, const char* key)
{
    bool found;
    unsigned int itemix;
    unsigned long hash;
    unsigned long cellix;
    hash = mc_util_hashdata(key, mc_util_strlen(key));
    found = false;
    cellix = mc_genericdict_getcellindex(dict, key, hash, &found);
    if(found == false)
    {
        return NULL;
    }
    itemix = dict->cells[cellix];
    return dict->values[itemix];
}

void* mc_genericdict_getvalueat(mcgenericdict_t* dict, unsigned int ix)
{
    if(ix >= dict->count)
    {
        return NULL;
    }
    return dict->values[ix];
}

const char* mc_genericdict_getkeyat(mcgenericdict_t* dict, unsigned int ix)
{
    if(ix >= dict->count)
    {
        return NULL;
    }
    return dict->keys[ix];
}

size_t mc_genericdict_count(mcgenericdict_t* dict)
{
    if(!dict)
    {
        return 0;
    }
    return dict->count;
}

bool mc_genericdict_remove(mcgenericdict_t* dict, const char* key)
{
    bool found;
    unsigned int x;
    unsigned int k;
    unsigned int i;
    unsigned int j;
    unsigned int cell;
    unsigned int itemix;
    unsigned int lastitemix;
    unsigned long hash;
    hash = mc_util_hashdata(key, mc_util_strlen(key));
    found = false;
    cell = mc_genericdict_getcellindex(dict, key, hash, &found);
    if(!found)
    {
        return false;
    }
    itemix = dict->cells[cell];
    mc_allocator_free(dict->pstate, dict->keys[itemix]);
    lastitemix = dict->count - 1;
    if(itemix < lastitemix)
    {
        dict->keys[itemix] = dict->keys[lastitemix];
        dict->values[itemix] = dict->values[lastitemix];
        dict->cellindices[itemix] = dict->cellindices[lastitemix];
        dict->hashes[itemix] = dict->hashes[lastitemix];
        dict->cells[dict->cellindices[itemix]] = itemix;
    }
    dict->count--;
    i = cell;
    j = i;
    for(x = 0; x < (dict->cellcapacity - 1); x++)
    {
        j = (j + 1) & (dict->cellcapacity - 1);
        if(dict->cells[j] == MC_CONF_GENERICDICTINVALIDIX)
        {
            break;
        }
        k = (unsigned int)(dict->hashes[dict->cells[j]]) & (dict->cellcapacity - 1);
        if((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j)))
        {
            dict->cellindices[dict->cells[j]] = i;
            dict->cells[i] = dict->cells[j];
            i = j;
        }
    }
    dict->cells[i] = MC_CONF_GENERICDICTINVALIDIX;
    return true;
}

bool mc_genericdict_init(mcgenericdict_t* dict, mcstate_t* state, unsigned int initialcapacity, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
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
    dict->funcdestroyfn = dfn;
    dict->cells = (unsigned int*)mc_allocator_malloc(dict->pstate, dict->cellcapacity * sizeof(*dict->cells));
    dict->keys = (char**)mc_allocator_malloc(dict->pstate, dict->itemcapacity * sizeof(*dict->keys));
    dict->values = (void**)mc_allocator_malloc(dict->pstate, dict->itemcapacity * sizeof(*dict->values));
    dict->cellindices = (unsigned int*)mc_allocator_malloc(dict->pstate, dict->itemcapacity * sizeof(*dict->cellindices));
    dict->hashes = (long unsigned int*)mc_allocator_malloc(dict->pstate, dict->itemcapacity * sizeof(*dict->hashes));
    if(dict->cells == NULL || dict->keys == NULL || dict->values == NULL || dict->cellindices == NULL || dict->hashes == NULL)
    {
        goto error;
    }
    for(i = 0; i < dict->cellcapacity; i++)
    {
        dict->cells[i] = MC_CONF_GENERICDICTINVALIDIX;
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

void mc_genericdict_deinit(mcgenericdict_t* dict, bool freekeys)
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

unsigned int mc_genericdict_getcellindex(mcgenericdict_t* dict, const char* key, unsigned long hash, bool* outfound)
{
    unsigned int i;
    unsigned int ix;
    unsigned int cell;
    unsigned int cellix;
    unsigned long hashtocheck;
    const char* keytocheck;
    *outfound = false;
    cellix = (unsigned int)hash & (dict->cellcapacity - 1);
    for(i = 0; i < dict->cellcapacity; i++)
    {
        ix = (cellix + i) & (dict->cellcapacity - 1);
        cell = dict->cells[ix];
        if(cell == MC_CONF_GENERICDICTINVALIDIX)
        {
            return ix;
        }
        hashtocheck = dict->hashes[cell];
        if(hash != hashtocheck)
        {
            continue;
        }
        keytocheck = dict->keys[cell];
        if(strcmp(key, keytocheck) == 0)
        {
            *outfound = true;
            return ix;
        }
    }
    return MC_CONF_GENERICDICTINVALIDIX;
}


bool mc_genericdict_growandrehash(mcgenericdict_t* dict)
{
    bool ok;
    unsigned int i;
    char* key;
    void* value;
    mcgenericdict_t newdict;
    ok = mc_genericdict_init(&newdict, dict->pstate, dict->cellcapacity * 2, dict->funccopyfn, dict->funcdestroyfn);
    if(!ok)
    {
        return false;
    }
    for(i = 0; i < dict->count; i++)
    {
        key = dict->keys[i];
        value = dict->values[i];
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

bool mc_genericdict_setinternal(mcgenericdict_t* dict, const char* ckey, char* mkey, void* value)
{
    bool ok;
    bool found;
    unsigned int cellix;
    unsigned int itemix;
    unsigned long hash;
    hash = mc_util_hashdata(ckey, mc_util_strlen(ckey));
    found = false;
    cellix = mc_genericdict_getcellindex(dict, ckey, hash, &found);
    if(found)
    {
        itemix = dict->cells[cellix];
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
        char* keycopy = mc_util_strdup(dict->pstate, ckey);
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

bool mc_valdict_init(mcvaldict_t* dict, mcstate_t* state, size_t ktsz, size_t vtsz, unsigned int initialcapacity)
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
    dict->cells = (unsigned int*)mc_allocator_malloc(dict->pstate, dict->cellcapacity * sizeof(*dict->cells));
    dict->keys = (void*)mc_allocator_malloc(dict->pstate, dict->itemcapacity * ktsz);
    dict->values = (void*)mc_allocator_malloc(dict->pstate, dict->itemcapacity * vtsz);
    dict->cellindices = (unsigned int*)mc_allocator_malloc(dict->pstate, dict->itemcapacity * sizeof(*dict->cellindices));
    dict->hashes = (long unsigned int*)mc_allocator_malloc(dict->pstate, dict->itemcapacity * sizeof(*dict->hashes));
    if(dict->cells == NULL || dict->keys == NULL || dict->values == NULL || dict->cellindices == NULL || dict->hashes == NULL)
    {
        goto error;
    }
    for(i = 0; i < dict->cellcapacity; i++)
    {
        dict->cells[i] = MC_CONF_VALDICTINVALIDIX;
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

void mc_valdict_deinit(mcvaldict_t* dict)
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

mcvaldict_t* mc_valdict_make(mcstate_t* state, size_t ktsz, size_t vtsz)
{
    return mc_valdict_makecapacity(state, MC_CONF_GENERICDICTINITSIZE, ktsz, vtsz);
}

mcvaldict_t* mc_valdict_makecapacity(mcstate_t* state, unsigned int mincapacity, size_t ktsz, size_t vtsz)
{
    bool ok;
    unsigned int capacity;
    mcvaldict_t* dict;
    capacity = mc_util_upperpowoftwo(mincapacity * 2);
    dict = (mcvaldict_t*)mc_allocator_malloc(state, sizeof(mcvaldict_t));
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

void mc_valdict_destroy(mcvaldict_t* dict)
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

void mc_valdict_sethashfunction(mcvaldict_t* dict, mcitemhashfn_t hashfn)
{
    dict->funchashfn = hashfn;
}

void mc_valdict_setequalsfunction(mcvaldict_t* dict, mcitemcomparefn_t equalsfn)
{
    dict->funckeyequalsfn = equalsfn;
}

bool mc_valdict_setkv(mcvaldict_t* dict, void* key, void* value)
{
    bool ok;
    bool found;
    unsigned long hash;
    unsigned int lastix;
    unsigned int cellix;
    unsigned int itemix;
    hash = mc_valdict_hashkey(dict, key);
    found = false;
    cellix = mc_valdict_getcellindex(dict, key, hash, &found);
    if(found)
    {
        itemix = dict->cells[cellix];
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
    lastix = dict->count;
    dict->count++;
    dict->cells[cellix] = lastix;
    mc_valdict_setkeyat(dict, lastix, key);
    mc_valdict_setvalueat(dict, lastix, value);
    dict->cellindices[lastix] = cellix;
    dict->hashes[lastix] = hash;
    return true;
}

void* mc_valdict_get(mcvaldict_t* dict, void* key)
{
    bool found;
    unsigned int itemix;
    unsigned long hash;
    unsigned long cellix;
    if(dict->count == 0)
    {
        return NULL;
    }
    hash = mc_valdict_hashkey(dict, key);
    found = false;
    cellix = mc_valdict_getcellindex(dict, key, hash, &found);
    if(!found)
    {
        return NULL;
    }
    itemix = dict->cells[cellix];
    return mc_valdict_getvalueat(dict, itemix);
}

void* mc_valdict_getkeyat(mcvaldict_t* dict, unsigned int ix)
{
    if(ix >= dict->count)
    {
        return NULL;
    }
    return (char*)dict->keys + (dict->keytypesize * ix);
}

void* mc_valdict_getvalueat(mcvaldict_t* dict, unsigned int ix)
{
    if(ix >= dict->count)
    {
        return NULL;
    }
    return (char*)dict->values + (dict->valtypesize * ix);
}

unsigned int mc_valdict_getcapacity(mcvaldict_t* dict)
{
    return dict->itemcapacity;
}

bool mc_valdict_setvalueat(mcvaldict_t* dict, unsigned int ix, void* value)
{
    size_t offset;
    if(ix >= dict->count)
    {
        return false;
    }
    offset = ix * dict->valtypesize;
    memcpy((char*)dict->values + offset, value, dict->valtypesize);
    return true;
}

int mc_valdict_count(mcvaldict_t* dict)
{
    if(!dict)
    {
        return 0;
    }
    return dict->count;
}

bool mc_valdict_removebykey(mcvaldict_t* dict, void* key)
{
    bool found;
    unsigned int x;
    unsigned int k;
    unsigned int i;
    unsigned int j;
    unsigned int cell;
    unsigned int itemix;
    unsigned int lastitemix;
    unsigned long hash;
    void* lastkey;
    void* lastvalue;
    hash = mc_valdict_hashkey(dict, key);
    found = false;
    cell = mc_valdict_getcellindex(dict, key, hash, &found);
    if(!found)
    {
        return false;
    }
    itemix = dict->cells[cell];
    lastitemix = dict->count - 1;
    if(itemix < lastitemix)
    {
        lastkey = mc_valdict_getkeyat(dict, lastitemix);
        mc_valdict_setkeyat(dict, itemix, lastkey);
        lastvalue = mc_valdict_getkeyat(dict, lastitemix);
        mc_valdict_setvalueat(dict, itemix, lastvalue);
        dict->cellindices[itemix] = dict->cellindices[lastitemix];
        dict->hashes[itemix] = dict->hashes[lastitemix];
        dict->cells[dict->cellindices[itemix]] = itemix;
    }
    dict->count--;
    i = cell;
    j = i;
    for(x = 0; x < (dict->cellcapacity - 1); x++)
    {
        j = (j + 1) & (dict->cellcapacity - 1);
        if(dict->cells[j] == MC_CONF_VALDICTINVALIDIX)
        {
            break;
        }
        k = (unsigned int)(dict->hashes[dict->cells[j]]) & (dict->cellcapacity - 1);
        if((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j)))
        {
            dict->cellindices[dict->cells[j]] = i;
            dict->cells[i] = dict->cells[j];
            i = j;
        }
    }
    dict->cells[i] = MC_CONF_VALDICTINVALIDIX;
    return true;
}

void mc_valdict_clear(mcvaldict_t* dict)
{
    unsigned int i;
    dict->count = 0;
    for(i = 0; i < dict->cellcapacity; i++)
    {
        dict->cells[i] = MC_CONF_VALDICTINVALIDIX;
    }
}

unsigned int mc_valdict_getcellindex(mcvaldict_t* dict, void* key, unsigned long hash, bool* outfound)
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
        if(cell == MC_CONF_VALDICTINVALIDIX)
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
    return MC_CONF_VALDICTINVALIDIX;
}

bool mc_valdict_growandrehash(mcvaldict_t* dict)
{
    bool ok;
    mcvaldict_t newdict;
    unsigned int i;
    unsigned newcapacity;
    char* key;
    void* value;
    newcapacity = dict->cellcapacity == 0 ? MC_CONF_GENERICDICTINITSIZE : dict->cellcapacity * 2;
    ok = mc_valdict_init(&newdict, dict->pstate, dict->keytypesize, dict->valtypesize, newcapacity);
    if(!ok)
    {
        return false;
    }
    newdict.funckeyequalsfn = dict->funckeyequalsfn;
    newdict.funchashfn = dict->funchashfn;
    for(i = 0; i < dict->count; i++)
    {
        key = (char*)mc_valdict_getkeyat(dict, i);
        value = mc_valdict_getvalueat(dict, i);
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

bool mc_valdict_setkeyat(mcvaldict_t* dict, unsigned int ix, void* key)
{
    size_t offset;
    if(ix >= dict->count)
    {
        return false;
    }
    offset = ix * dict->keytypesize;
    memcpy((char*)dict->keys + offset, key, dict->keytypesize);
    return true;
}

bool mc_valdict_keysareequal(mcvaldict_t* dict, void* a, void* b)
{
    if(dict->funckeyequalsfn)
    {
        return dict->funckeyequalsfn(a, b);
    }
    return memcmp(a, b, dict->keytypesize) == 0;
}

unsigned long mc_valdict_hashkey(mcvaldict_t* dict, void* key)
{
    if(dict->funchashfn)
    {
        return dict->funchashfn(key);
    }
    return mc_util_hashdata(key, dict->keytypesize);
}

mcbasicarray_t* mc_basicarray_make(mcstate_t* state, size_t tsz)
{
    return mc_basicarray_makecapacity(state, 32, tsz);
}

mcbasicarray_t* mc_basicarray_makecapacity(mcstate_t* state, unsigned int capacity, size_t tsz)
{
    bool ok;
    mcbasicarray_t* arr;
    arr = (mcbasicarray_t*)mc_allocator_malloc(state, sizeof(mcbasicarray_t));
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

void mc_basicarray_destroy(mcbasicarray_t* arr)
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

mcbasicarray_t* mc_basicarray_copy(mcbasicarray_t* arr)
{
    mcbasicarray_t* copy;
    copy = (mcbasicarray_t*)mc_allocator_malloc(arr->pstate, sizeof(mcbasicarray_t));
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
        copy->allocdata = (unsigned char*)mc_allocator_malloc(arr->pstate, arr->capacity * arr->typesize);
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

bool mc_basicarray_push(mcbasicarray_t* arr, void* value)
{
    unsigned int newcapacity;
    unsigned char* newdata;
    if(arr->count >= arr->capacity)
    {
        MC_ASSERT(!arr->caplocked);
        if(arr->caplocked)
        {
            return false;
        }
        newcapacity = arr->capacity > 0 ? arr->capacity * 2 : 1;
        newdata = (unsigned char*)mc_allocator_malloc(arr->pstate, newcapacity * arr->typesize);
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


bool mc_basicarray_pop(mcbasicarray_t* arr, void* outvalue)
{
    void* res;
    if(arr->count <= 0)
    {
        return false;
    }
    if(outvalue)
    {
        res = mc_basicarray_get(arr, arr->count - 1);
        memcpy(outvalue, res, arr->typesize);
    }
    mc_basicarray_removeat(arr, arr->count - 1);
    return true;
}

void* mc_basicarray_top(mcbasicarray_t* arr)
{
    if(arr->count <= 0)
    {
        return NULL;
    }
    return mc_basicarray_get(arr, arr->count - 1);
}

bool mc_basicarray_set(mcbasicarray_t* arr, unsigned int ix, void* value)
{
    size_t offset;
    if(ix >= arr->count)
    {
        MC_ASSERT(false);
        return false;
    }
    offset = ix * arr->typesize;
    memmove(arr->data + offset, value, arr->typesize);
    return true;
}

MCINLINE void* mc_basicarray_get(mcbasicarray_t* arr, unsigned int ix)
{
    size_t offset;
    if(ix >= arr->count)
    {
        MC_ASSERT(false);
        return NULL;
    }
    offset = ix * arr->typesize;
    return arr->data + offset;
}

MCINLINE size_t mc_basicarray_count(mcbasicarray_t* arr)
{
    if(!arr)
    {
        return 0;
    }
    return arr->count;
}

bool mc_basicarray_removeat(mcbasicarray_t* arr, unsigned int ix)
{
    size_t tomovebytes;
    void* dest;
    void* src;
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
    tomovebytes = (arr->count - 1 - ix) * arr->typesize;
    dest = arr->data + (ix * arr->typesize);
    src = arr->data + ((ix + 1) * arr->typesize);
    memmove(dest, src, tomovebytes);
    arr->count--;
    return true;
}


void mc_basicarray_clear(mcbasicarray_t* arr)
{
    arr->count = 0;
}

void* mc_basicarray_data(mcbasicarray_t* arr)
{
    return arr->data;
}

void mc_basicarray_orphandata(mcbasicarray_t* arr)
{
    mc_basicarray_initcapacity(arr, arr->pstate, 0, arr->typesize);
}

bool mc_basicarray_initcapacity(mcbasicarray_t* arr, mcstate_t* state, unsigned int capacity, size_t tsz)
{
    arr->pstate = state;
    if(capacity > 0)
    {
        arr->allocdata = (unsigned char*)mc_allocator_malloc(arr->pstate, capacity * tsz);
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

void mc_basicarray_deinit(mcbasicarray_t* arr)
{
    mc_allocator_free(arr->pstate, arr->allocdata);
}

void mc_ptrlist_setempty(mcptrlist_t* list)
{
    /*
    if((list->listcapacity > 0) && (list->listitems != NULL))
    {
        memset(list->listitems, 0, sizeof(void*) * list->listcapacity);
    }
    */
    list->listcount = 0;
    list->listcapacity = 0;
}

mcptrlist_t* mc_ptrlist_make(mcstate_t* state, size_t capacity)
{
    mcptrlist_t* list;
    list = (mcptrlist_t*)mc_allocator_malloc(state, sizeof(mcptrlist_t));
    list->pstate = state;
    list->listcount = 0;
    list->livecount = 0;
    list->listcapacity = 0;
    list->listitems = NULL;
    mc_ptrlist_setempty(list);
    if(capacity > 0)
    {
        mc_ptrlist_ensurecapacity(list, capacity, NULL);
    }
    return list;
}

void mc_ptrlist_ensurecapacity(mcptrlist_t* list, size_t needsize, void* fillval)
{
    size_t i;
    size_t oldcap;
    if(list->listcapacity < needsize)
    {
        oldcap = list->listcapacity;
        list->listcapacity = needsize;
        if(list->listitems == NULL)
        {
            list->listitems = (void*)mc_allocator_malloc(list->pstate, sizeof(void*) * needsize);
        }
        else
        {
            list->listitems = (void*)mc_allocator_realloc(list->pstate, list->listitems, sizeof(void*) * needsize);
        }
        for(i = oldcap; i < needsize; i++)
        {
            list->listitems[i] = fillval;
        }
    }
}

bool mc_ptrlist_removeat(mcptrlist_t* arr, unsigned int ix)
{
    size_t tomovebytes;
    void* dest;
    void* src;
    if(ix >= arr->listcount)
    {
        return false;
    }
    if(ix == 0)
    {
        arr->listitems += sizeof(void*);
        arr->listcapacity--;
        arr->listcount--;
        return true;
    }
    if(ix == (arr->listcount - 1))
    {
        arr->listcount--;
        return true;
    }
    tomovebytes = (arr->listcount - 1 - ix) * sizeof(void*);
    dest = arr->listitems + (ix * sizeof(void*));
    src = arr->listitems + ((ix + 1) * sizeof(void*));
    memmove(dest, src, tomovebytes);
    arr->listcount--;
    return true;
}


size_t mc_ptrlist_count(mcptrlist_t* list)
{
    return list->listcount;
}

size_t mc_ptrlist_capacity(mcptrlist_t* list)
{
    return list->listcount;
}

void mc_ptrlist_destroy(mcptrlist_t* list, mcitemdestroyfn_t dfn)
{
    mcstate_t* state;
    if(list == NULL)
    {
        return;
    }
    state = list->pstate;
    if(dfn)
    {
        mc_ptrlist_clearanddestroy(list, dfn);
    }
    list->listcount = 0;
    list->livecount = 0;
    mc_allocator_free(state, list->listitems);
    mc_allocator_free(state, list);
}

void mc_ptrlist_clear(mcptrlist_t* list)
{
    list->listcount = 0;
}

bool mc_ptrlist_push(mcptrlist_t* list, void* value)
{
    size_t oldcap;
    if(list->listcapacity < list->listcount + 1)
    {
        oldcap = list->listcapacity;
        list->listcapacity = MC_GROW_CAPACITY(oldcap);
        if(list->listitems == NULL)
        {
            list->listitems = (void**)mc_allocator_malloc(list->pstate, sizeof(void*) * list->listcapacity);
        }
        else
        {
            list->listitems = (void**)mc_allocator_realloc(list->pstate, list->listitems, sizeof(void*) * list->listcapacity);
        }

    }
    list->listitems[list->listcount] = value;
    list->listcount++;
    list->livecount++;
    return true;
}

void* mc_ptrlist_get(mcptrlist_t* arr, unsigned int ix)
{
    return arr->listitems[ix];
}

void* mc_ptrlist_top(mcptrlist_t* arr)
{
    if(arr->listcount == 0)
    {
        return NULL;
    }
    return arr->listitems[arr->listcount - 1];
}

void* mc_ptrlist_pop(mcptrlist_t* list)
{
    void* v;
    if(list->listcount > 0)
    {
        v = list->listitems[list->listcount - 1];
        list->listcount--;
        return v;
    }
    return NULL;
}

mcptrlist_t* mc_ptrlist_copy(mcptrlist_t* arr, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
{
    bool ok;
    size_t i;
    void* item;
    void* itemcopy;
    mcptrlist_t* arrcopy;
    arrcopy = mc_ptrlist_make(arr->pstate, arr->listcapacity);
    if(!arrcopy)
    {
        return NULL;
    }
    for(i = 0; i < mc_ptrlist_count(arr); i++)
    {
        item = (void*)mc_ptrlist_get(arr, i);
        itemcopy = item;
        if(copyfn)
        {
            itemcopy = copyfn(item);
        }
        if(item && !itemcopy)
        {
            goto err;
        }
        ok = mc_ptrlist_push(arrcopy, itemcopy);
        if(!ok)
        {
            goto err;
        }
    }
    return arrcopy;
err:
    mc_ptrlist_destroy(arrcopy, dfn);
    return NULL;
}


void mc_ptrlist_clearanddestroy(mcptrlist_t* arr, mcitemdestroyfn_t dfn)
{
    size_t i;
    void* item;
    for(i = 0; i < mc_ptrlist_count(arr); i++)
    {
        item = mc_ptrlist_get(arr, i);
        dfn(item);
    }
    mc_ptrlist_clear(arr);
}

mcprinter_t* mc_printer_make(mcstate_t* state, FILE* ofh)
{
    return mc_printer_make_with_capacity(state, 1, ofh);
}

mcprinter_t* mc_printer_make_with_capacity(mcstate_t* state, unsigned int capacity, FILE* ofh)
{
    mcprinter_t* pr;
    pr = (mcprinter_t*)mc_allocator_malloc(state, sizeof(mcprinter_t));
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

bool mc_printer_init(mcprinter_t* pr, mcstate_t* state, size_t capacity, FILE* ofh, bool onstack)
{
    memset(pr, 0, sizeof(mcprinter_t));
    pr->pstate = state;
    pr->failed = false;
    pr->destfile = ofh;
    pr->data = NULL;
    pr->capacity = 0;
    pr->len = 0;
    pr->onstack = onstack;
    pr->config.verbosefunc = true;
    pr->config.quotstring = false;
    pr->config.shouldflush = true;
    if(ofh != NULL)
    {
        return true;
    }
    pr->data = (char*)mc_allocator_malloc(state, capacity);
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

void mc_printer_release(mcprinter_t* pr)
{
    if(pr == NULL)
    {
        return;
    }
    if(pr->data != NULL)
    {
        mc_allocator_free(pr->pstate, pr->data);
    }
}

void mc_printer_destroy(mcprinter_t* pr)
{
    mc_printer_release(pr);
    if(!pr->onstack)
    {
        mc_allocator_free(pr->pstate, pr);
    }
}

void mc_printer_clear(mcprinter_t* pr)
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

bool mc_printer_putlen(mcprinter_t* pr, const char* str, size_t len)
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
        if(pr->config.shouldflush)
        {
            fflush(pr->destfile);
        }
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

bool mc_printer_puts(mcprinter_t* pr, const char* str)
{
    return mc_printer_putlen(pr, str, mc_util_strlen(str));
}

bool mc_printer_putchar(mcprinter_t* pr, int b)
{
    char ch;
    ch = b;
    return mc_printer_putlen(pr, &ch, 1);
}

bool mc_printer_printf(mcprinter_t* pr, const char* fmt, ...)
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
        if(pr->config.shouldflush)
        {
            fflush(pr->destfile);
        }
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

void mc_printer_printescapedchar(mcprinter_t* pr, int ch)
{
    switch(ch)
    {
        case '\'':
            {
                mc_printer_puts(pr, "\\\'");
            }
            break;
        case '\"':
            {
                mc_printer_puts(pr, "\\\"");
            }
            break;
        case '\\':
            {
                mc_printer_puts(pr, "\\\\");
            }
            break;
        case '\b':
            {
                mc_printer_puts(pr, "\\b");
            }
            break;
        case '\f':
            {
                mc_printer_puts(pr, "\\f");
            }
            break;
        case '\n':
            {
                mc_printer_puts(pr, "\\n");
            }
            break;
        case '\r':
            {
                mc_printer_puts(pr, "\\r");
            }
            break;
        case '\t':
            {
                mc_printer_puts(pr, "\\t");
            }
            break;
        case 0:
            {
                mc_printer_puts(pr, "\\0");
            }
            break;
        default:
            {
                mc_printer_printf(pr, "\\x%02x", (unsigned char)ch);
            }
            break;
    }
}

void mc_printer_printescapedstring(mcprinter_t* pr, const char* str, size_t len)
{
    int ch;
    size_t i;
    mc_printer_puts(pr, "\"");
    for(i=0; i<len; i++)
    {
        ch = str[i];
        if((ch < 32) || (ch > 127) || (ch == '\"') || (ch == '\\'))
        {
            mc_printer_printescapedchar(pr, ch);
        }
        else
        {
            mc_printer_putchar(pr, ch);
        }
    }
    mc_printer_puts(pr, "\"");
}

const char* mc_printer_getstring(mcprinter_t* pr)
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

size_t mc_printer_getlength(mcprinter_t* pr)
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

char* mc_printer_getstringanddestroy(mcprinter_t* pr, size_t* lendest)
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
    if(lendest != NULL)
    {
        *lendest = pr->len;
    }
    pr->data = NULL;
    mc_printer_destroy(pr);
    return res;
}

bool mc_printer_failed(mcprinter_t* pr)
{
    return pr->failed;
}

bool mc_printer_grow(mcprinter_t* pr, size_t newcapacity)
{
    char* ndata;
    if(pr->destfile != NULL)
    {
        return true;
    }
    ndata = (char*)mc_allocator_malloc(pr->pstate, newcapacity);
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

#define mc_printer_printvalue(pr, val) \
    mc_printer_printvalue_actual((val).type, pr, val)

void mc_printer_printnumberfloat(mcprinter_t* pr, mcfloat_t flt)
{
    int64_t inum;
    //mc_printer_printf(pr, "%1.10g", flt);
    inum = (int64_t)flt;
    if(flt == inum)
    {
        #if 1
            mc_printer_printf(pr, "%" PRIiFAST64 "", inum);
        #else
            mc_printer_printf(pr, "%ld", inum);
        #endif
    }
    else
    {
        mc_printer_printf(pr, "%g", flt);
    }
}

bool mc_printutil_bcreadoperands(mcopdefinition_t* def, const uint8_t* instr, uint64_t outoperands[2])
{
    int i;
    int offset;
    int operandwidth;
    uint64_t operand;
    offset = 0;
    for(i = 0; i < def->numoperands; i++)
    {
        operandwidth = def->operandwidths[i];
        switch(operandwidth)
        {
            case 1:
                {
                    outoperands[i] = instr[offset];
                }
                break;
            case 2:
                {
                    operand = 0;
                    operand = operand | ((uint64_t)instr[offset] << 8);
                    operand = operand | ((uint64_t)instr[offset + 1]);
                    outoperands[i] = operand;
                }
                break;
            case 4:
                {
                    operand = 0;
                    operand = operand | ((uint64_t)instr[offset + 0] << 24);
                    operand = operand | ((uint64_t)instr[offset + 1] << 16);
                    operand = operand | ((uint64_t)instr[offset + 2] << 8);
                    operand = operand | ((uint64_t)instr[offset + 3]);
                    outoperands[i] = operand;
                }
                break;
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
                }
                break;
            default:
                {
                    MC_ASSERT(false);
                    return false;
                }
                break;
        }
        offset += operandwidth;
    }
    return true;
}

void mc_printer_printbytecode(mcprinter_t* pr, uint8_t* code, mcastlocation_t* sposlist, size_t codesize, bool simple)
{
    bool ok;
    int i;
    uint8_t op;
    unsigned pos;
    mcfloat_t dval;
    uint64_t operands[2];
    mcastlocation_t srcpos;
    mcopdefinition_t vdef;
    mcopdefinition_t* def;
    pos = 0;
    while(pos < codesize)
    {
        op = code[pos];
        def = mc_opdef_lookup(&vdef, op);
        MC_ASSERT(def);
        if(sposlist)
        {
            srcpos = sposlist[pos];
            if(simple)
            {
                mc_printer_puts(pr, "<");
            }
            mc_printer_printf(pr, "@%d:%d %04d %s", srcpos.line, srcpos.column, pos, def->name);
            if(simple)
            {
                mc_printer_puts(pr, ">");
            }
        }
        else
        {
            mc_printer_printf(pr, "%04d %s", pos, def->name);
        }
        pos++;
        ok = mc_printutil_bcreadoperands(def, code + pos, operands);
        if(!ok)
        {
            return;
        }
        for(i = 0; i < def->numoperands; i++)
        {
            if(op == MC_OPCODE_NUMBER)
            {
                dval = mc_util_uint64todouble(operands[i]);
                mc_printer_printf(pr, " %1.17g", dval);
            }
            else
            {
                mc_printer_printf(pr, " %llu", operands[i]);
            }
            pos += def->operandwidths[i];
        }
        if(simple)
        {
            mc_printer_puts(pr, ",");
        }
        else
        {
            mc_printer_puts(pr, "\n");
        }
    }
}

void mc_printer_printobjstring(mcprinter_t* pr, mcvalue_t obj)
{
    size_t len;
    const char* str;
    str = mc_valstring_getdata(obj);
    len = mc_valstring_getlength(obj);
    if(pr->config.quotstring)
    {
        mc_printer_printescapedstring(pr, str, len);
    }
    else
    {
        mc_printer_putlen(pr, str, len);
    }
}

void mc_printer_printobjfuncscript(mcprinter_t* pr, mcvalue_t obj)
{
    const char* fname;
    mcobjfuncscript_t* fn;
    fn = mc_value_functiongetscriptfunction(obj);
    fname = mc_value_functiongetname(obj);
    mc_printer_printf(pr, "<scriptfunction '%s' locals=%d argc=%d fvc=%d", fname, fn->numlocals, fn->numargs, fn->freevalscount);
    if(pr->config.verbosefunc)
    {
        mc_printer_puts(pr, " [");
        mc_printer_printbytecode(pr, fn->compiledprogcode->bytecode, fn->compiledprogcode->progsrcposlist, fn->compiledprogcode->count, true);
        mc_printer_puts(pr, " ]");
    }
    else
    {
    }
    mc_printer_puts(pr, ">");
}

void mc_printer_printobjarray(mcprinter_t* pr, mcvalue_t obj)
{
    bool recursion;
    size_t i;
    size_t alen;
    bool prevquot;
    mcvalue_t iobj;
    mcvallist_t* actualary;
    mcvallist_t* otherary;
    actualary = mc_valarray_getinternalarray(obj);
    alen = mc_valarray_getlength(obj);
    mc_printer_puts(pr, "[");
    for(i = 0; i < alen; i++)
    {
        recursion = false;
        iobj = mc_valarray_getvalueat(obj, i);
        if(mc_value_gettype(iobj) == MC_VAL_ARRAY)
        {
            otherary = mc_valarray_getinternalarray(iobj);
            if(otherary == actualary)
            {
                recursion = true;
            }
        }
        prevquot = pr->config.quotstring;
        pr->config.quotstring = true;
        if(recursion)
        {
            mc_printer_puts(pr, "<recursion>");
        }
        else
        {
            mc_printer_printvalue(pr, iobj);
        }
        pr->config.quotstring = prevquot;
        if(i < (alen - 1))
        {
            mc_printer_puts(pr, ", ");
        }
    }
    mc_printer_puts(pr, "]");
}

void mc_printer_printobjmap(mcprinter_t* pr, mcvalue_t obj)
{
    bool prevquot;
    size_t i;
    size_t alen;
    mcvalue_t key;
    mcvalue_t val;
    alen = mc_valmap_getlength(obj);
    mc_printer_puts(pr, "{");
    for(i = 0; i < alen; i++)
    {
        key = mc_valmap_getkeyat(obj, i);
        val = mc_valmap_getvalueat(obj, i);
        prevquot = pr->config.quotstring;
        pr->config.quotstring = true;
        mc_printer_printvalue(pr, key);
        mc_printer_puts(pr, ": ");
        mc_printer_printvalue(pr, val);
        pr->config.quotstring = prevquot;
        if(i < (alen - 1))
        {
            mc_printer_puts(pr, ", ");
        }
    }
    mc_printer_puts(pr, "}");
}

void mc_printer_printobjerror(mcprinter_t* pr, mcvalue_t obj)
{
    mctraceback_t* traceback;
    mc_printer_printf(pr, "ERROR: %s\n", mc_value_errorgetmessage(obj));
    traceback = mc_value_errorgettraceback(obj);
    MC_ASSERT(traceback);
    if(traceback)
    {
        mc_printer_puts(pr, "Traceback:\n");
        mc_printer_printtraceback(pr, traceback);
    }
}


void mc_printer_printvalue_actual(int vt, mcprinter_t* pr, mcvalue_t obj)
{
    mcvaltype_t type;
    (void)vt;
    type = mc_value_gettype(obj);
    switch(type)
    {
        case MC_VAL_FREED:
            {
                mc_printer_puts(pr, "FREED");
            }
            break;
        case MC_VAL_NONE:
            {
                mc_printer_puts(pr, "NONE");
            }
            break;
        case MC_VAL_NUMBER:
            {
                mcfloat_t number;
                number = mc_value_getnumber(obj);
                mc_printer_printnumberfloat(pr, number);
            }
            break;
        case MC_VAL_BOOL:
            {
                mc_printer_puts(pr, mc_value_getbool(obj) ? "true" : "false");
            }
            break;
        case MC_VAL_STRING:
            {
                mc_printer_printobjstring(pr, obj);
            }
            break;
        case MC_VAL_NULL:
            {
                mc_printer_puts(pr, "null");
            }
            break;
        case MC_VAL_FUNCSCRIPT:
            {
                mc_printer_printobjfuncscript(pr, obj);
            }
            break;
        case MC_VAL_ARRAY:
            {
                mc_printer_printobjarray(pr, obj);
            }
            break;
        case MC_VAL_MAP:
            {
                mc_printer_printobjmap(pr, obj);
            }
            break;
        case MC_VAL_FUNCNATIVE:
            {
                mc_printer_puts(pr, "NATIVE_FUNCTION");
            }
            break;
        case MC_VAL_EXTERNAL:
            {
                mc_printer_puts(pr, "EXTERNAL");
            }
            break;
        case MC_VAL_ERROR:
            {
                mc_printer_printobjerror(pr, obj);
            }
            break;
        case MC_VAL_ANY:
            {
                MC_ASSERT(false);
            }
            break;
    }
}

mcastlocation_t mc_astlocation_make(mcastcompiledfile_t* file, int line, int column)
{
    mcastlocation_t loc;
    loc.file = file;
    loc.line = line;
    loc.column = column;
    return loc;
}

char* mc_util_stringallocfmt(mcstate_t* state, const char* format, ...)
{
    int towrite;
    int written;
    char* res;
    va_list args;
    (void)written;
    va_start(args, format);
    towrite = vsnprintf(NULL, 0, format, args);
    va_end(args);
    va_start(args, format);
    res = (char*)mc_allocator_malloc(state, towrite + 1);
    if(!res)
    {
        return NULL;
    }
    written = vsprintf(res, format, args);
    va_end(args);
    MC_ASSERT(written == towrite);
    return res;
}

uint64_t mc_util_doubletouint64(mcfloat_t val)
{
    union
    {
        uint64_t val_uint64;
        double val_double;
    } temp;
    temp.val_double = val;
    return temp.val_uint64;
}

mcfloat_t mc_util_uint64todouble(uint64_t val)
{
    union
    {
        uint64_t val_uint64;
        double val_double;
    } temp;
    temp.val_uint64 = val;
    return temp.val_double;
}

void mc_errlist_init(mcerrlist_t* errors)
{
    memset(errors, 0, sizeof(mcerrlist_t));
    errors->count = 0;
}

void mc_errlist_deinit(mcerrlist_t* errors)
{
    mc_errlist_clear(errors);
}

void mc_errlist_push(mcerrlist_t* errors, mcerrtype_t type, mcastlocation_t pos, const char* message)
{
    int len;
    int tocopy;
    mcerror_t err;
    if(errors->count >= MC_CONF_ERROR_MAXERRORCOUNT)
    {
        return;
    }
    memset(&err, 0, sizeof(mcerror_t));
    err.type = type;
    len = mc_util_strlen(message);
    tocopy = len;
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

void mc_errlist_addfv(mcerrlist_t* errors, mcerrtype_t type, mcastlocation_t pos, const char* format, va_list va)
{
    int towrite;
    int written;
    char res[MC_CONF_ERROR_MSGMAXLENGTH];
    va_list vcopy;
    (void)towrite;
    (void)written;
    va_copy(vcopy, va);
    towrite = vsnprintf(NULL, 0, format, vcopy);
    written = vsnprintf(res, MC_CONF_ERROR_MSGMAXLENGTH, format, va);
    MC_ASSERT(towrite == written);
    mc_errlist_push(errors, type, pos, res);
}

void mc_errlist_addf(mcerrlist_t* errors, mcerrtype_t type, mcastlocation_t pos, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    mc_errlist_addfv(errors,type, pos, format, va);
    va_end(va);
}

void mc_errlist_clear(mcerrlist_t* errors)
{
    int i;
    mcerror_t* error;
    for(i = 0; i < errors->count; i++)
    {
        error = mc_errlist_get(errors, i);
        if(error->traceback)
        {
            mc_traceback_destroy(error->traceback);
        }
    }
    errors->count = 0;
}

mcerror_t* mc_errlist_get(mcerrlist_t* errors, int ix)
{
    if(ix >= errors->count)
    {
        return NULL;
    }
    return &errors->errors[ix];
}


const char* mc_util_errtypename(mcerrtype_t type)
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
            break;
    }
    return "INVALID";
}

mcerror_t* mc_errlist_getlast(mcerrlist_t* errors)
{
    if(errors->count <= 0)
    {
        return NULL;
    }
    return &errors->errors[errors->count - 1];
}

/* containerend utilsend */

MCINLINE mcvalue_t mc_object_makedatafrom(mcvaltype_t type, mcobjdata_t* data)
{
    mcvalue_t object;
    object.type = type;
    data->type = type;
    object.isallocated = true;
    object.uval.odata = data;
    return object;
}

MCINLINE mcvalue_t mc_value_makeempty(mcvaltype_t t)
{
    mcvalue_t o = {};
    //memset(&o, 0, sizeof(mcvalue_t));
    o.type = t;
    o.isallocated = false;
    return o;
}

MCINLINE mcvalue_t mc_value_makenumber(mcfloat_t val)
{
    mcvalue_t o;
    o = mc_value_makeempty(MC_VAL_NUMBER);
    o.uval.valnumber = val;
    return o;
}

MCINLINE mcvalue_t mc_value_makebool(bool val)
{
    mcvalue_t o;
    o = mc_value_makeempty(MC_VAL_BOOL);
    o.uval.valbool = val;
    return o;
}

MCINLINE mcvalue_t mc_value_makenull(void)
{
    mcvalue_t o;
    o = mc_value_makeempty(MC_VAL_NULL);
    return o;
}

mcvalue_t mc_value_makestrcapacity(mcstate_t* state, int capacity)
{
    mcobjdata_t* data;
    data = mc_gcmemory_getdatafrompool(state, MC_VAL_STRING);
    if(!data)
    {
        data = mc_gcmemory_allocobjectdata(state);
        if(!data)
        {
            return mc_value_makenull();
        }
    }
    data->uvobj.valstring.hash = 0;
    data->uvobj.valstring.strbuf = dyn_strbuf_makeempty(capacity);
    return mc_object_makedatafrom(MC_VAL_STRING, data);
}

mcvalue_t mc_value_makestrformat(mcstate_t* state, const char* fmt, ...)
{
    va_list args;
    mcvalue_t res;
    mcobjdata_t* data;
    data = mc_gcmemory_getdatafrompool(state, MC_VAL_STRING);
    res = mc_value_makestrcapacity(state, 0);
    if(mc_value_isnull(res))
    {
        return mc_value_makenull();
    }
    va_start(args, fmt);
    dyn_strbuf_appendformatv(data->uvobj.valstring.strbuf, fmt, args);
    va_end(args);
    return res;
}

mcvalue_t mc_value_makestringlen(mcstate_t* state, const char* string, size_t len)
{
    bool ok;
    mcvalue_t res;
    res = mc_value_makestrcapacity(state, len);
    if(mc_value_isnull(res))
    {
        return res;
    }
    ok = mc_valstring_appendlen(res, string, len);
    if(!ok)
    {
        return mc_value_makenull();
    }
    return res;
}

mcvalue_t mc_value_makestring(mcstate_t* state, const char* string)
{
    return mc_value_makestringlen(state, string, mc_util_strlen(string));
}

mcvalue_t mc_value_makefuncnative(mcstate_t* state, const char* name, mcnativefn_t fn, void* data, int dlen)
{
    mcobjdata_t* obj;
    if(dlen > MC_CONF_NATIVEFUNCMAXDATA)
    {
        return mc_value_makenull();
    }
    obj = mc_gcmemory_allocobjectdata(state);
    if(!obj)
    {
        return mc_value_makenull();
    }
    obj->uvobj.valnativefunc.name = mc_util_strdup(state, name);
    if(!obj->uvobj.valnativefunc.name)
    {
        return mc_value_makenull();
    }
    obj->uvobj.valnativefunc.natptrfn = fn;
    if(data)
    {
        memcpy(obj->uvobj.valnativefunc.userdata, data, dlen);
    }
    obj->uvobj.valnativefunc.userdlen = dlen;
    return mc_object_makedatafrom(MC_VAL_FUNCNATIVE, obj);
}

mcvalue_t mc_value_makearray(mcstate_t* state)
{
    return mc_value_makearraycapacity(state, 8);
}

mcvalue_t mc_value_makearraycapacity(mcstate_t* state, unsigned capacity)
{
    mcobjdata_t* data;
    data = mc_gcmemory_getdatafrompool(state, MC_VAL_ARRAY);
    if(data)
    {
        mc_vallist_setempty(data->uvobj.valarray);
        return mc_object_makedatafrom(MC_VAL_ARRAY, data);
    }
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->uvobj.valarray = mc_vallist_make(state, "<array>", capacity);
    if(!data->uvobj.valarray)
    {
        return mc_value_makenull();
    }
    return mc_object_makedatafrom(MC_VAL_ARRAY, data);
}

mcvalue_t mc_value_makemap(mcstate_t* state)
{
    return mc_value_makemapcapacity(state, 32);
}

mcvalue_t mc_value_makemapcapacity(mcstate_t* state, unsigned capacity)
{
    mcobjdata_t* data;
    data = mc_gcmemory_getdatafrompool(state, MC_VAL_MAP);
    if(data)
    {
        mc_valdict_clear(data->uvobj.valmap);
        return mc_object_makedatafrom(MC_VAL_MAP, data);
    }
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->uvobj.valmap = mc_valdict_makecapacity(state, capacity, sizeof(mcvalue_t), sizeof(mcvalue_t));
    if(!data->uvobj.valmap)
    {
        return mc_value_makenull();
    }
    mc_valdict_sethashfunction(data->uvobj.valmap, (mcitemhashfn_t)mc_value_hash);
    mc_valdict_setequalsfunction(data->uvobj.valmap, (mcitemcomparefn_t)mc_value_equalswrapped);
    return mc_object_makedatafrom(MC_VAL_MAP, data);
}

mcvalue_t mc_value_makeerror(mcstate_t* state, const char* error)
{
    char* errorstr;
    mcvalue_t res;
    errorstr = mc_util_strdup(state, error);
    if(!errorstr)
    {
        return mc_value_makenull();
    }
    res = mc_value_makeerrornocopy(state, errorstr);
    if(mc_value_isnull(res))
    {
        mc_memory_free(errorstr);
        return mc_value_makenull();
    }
    return res;
}

mcvalue_t mc_value_makeerrornocopy(mcstate_t* state, char* error)
{
    mcobjdata_t* data;
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->uvobj.valerror.message = error;
    data->uvobj.valerror.traceback = NULL;
    return mc_object_makedatafrom(MC_VAL_ERROR, data);
}

mcvalue_t mc_value_makeerrorf(mcstate_t* state, const char* fmt, ...)
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
    res = (char*)mc_memory_malloc(towrite + 1);
    if(!res)
    {
        return mc_value_makenull();
    }
    written = vsprintf(res, fmt, args);
    MC_ASSERT(written == towrite);
    va_end(args);
    resobj = mc_value_makeerrornocopy(state, res);
    if(mc_value_isnull(resobj))
    {
        mc_memory_free(res);
        return mc_value_makenull();
    }
    return resobj;
}

mcvalue_t mc_value_makefuncscript(mcstate_t* state, const char* name, mccompiledprogram_t* cres, bool ownsdt, int nlocals, int nargs, int fvc)
{
    mcobjdata_t* data;
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return mc_value_makenull();
    }
    if(ownsdt)
    {
        data->uvobj.valscriptfunc.unamev.fallocname = name ? mc_util_strdup(state, name) : mc_util_strdup(state, "anonymous");
        if(!data->uvobj.valscriptfunc.unamev.fallocname)
        {
            return mc_value_makenull();
        }
    }
    else
    {
        data->uvobj.valscriptfunc.unamev.fconstname = name ? name : "anonymous";
    }
    data->uvobj.valscriptfunc.compiledprogcode = cres;
    data->uvobj.valscriptfunc.ownsdata = ownsdt;
    data->uvobj.valscriptfunc.numlocals = nlocals;
    data->uvobj.valscriptfunc.numargs = nargs;
    if(fvc >= MC_UTIL_STATICARRAYSIZE(data->uvobj.valscriptfunc.ufv.freevalsstack))
    {
        data->uvobj.valscriptfunc.ufv.freevalsallocated = (mcvalue_t*)mc_memory_malloc(sizeof(mcvalue_t) * fvc);
        if(!data->uvobj.valscriptfunc.ufv.freevalsallocated)
        {
            return mc_value_makenull();
        }
    }
    data->uvobj.valscriptfunc.freevalscount = fvc;
    return mc_object_makedatafrom(MC_VAL_FUNCSCRIPT, data);
}

mcvalue_t mc_value_makeuserobject(mcstate_t* state, void* data)
{
    mcobjdata_t* obj;
    obj = mc_gcmemory_allocobjectdata(state);
    if(!obj)
    {
        return mc_value_makenull();
    }
    obj->uvobj.valuserobject.data = data;
    obj->uvobj.valuserobject.datadestroyfn = NULL;
    obj->uvobj.valuserobject.datacopyfn = NULL;
    return mc_object_makedatafrom(MC_VAL_EXTERNAL, obj);
}

#define mc_value_getallocateddata(object) (object).uval.odata

void mc_value_deinit(mcvalue_t obj)
{
    mcobjdata_t* data;
    if(mc_value_isallocated(obj))
    {
        data = mc_value_getallocateddata(obj);
        mc_objectdata_deinit(data);
    }
}

void mc_objectdata_deinit(mcobjdata_t* data)
{
    switch(data->type)
    {
        case MC_VAL_FREED:
            {
                MC_ASSERT(false);
                return;
            }
            break;
        case MC_VAL_STRING:
            {
                dyn_strbuf_destroy(data->uvobj.valstring.strbuf);
            }
            break;
        case MC_VAL_FUNCSCRIPT:
            {
                if(data->uvobj.valscriptfunc.ownsdata)
                {
                    mc_allocator_free(data->pstate, data->uvobj.valscriptfunc.unamev.fallocname);
                    mc_astcompresult_destroy(data->uvobj.valscriptfunc.compiledprogcode);
                }
                if(mc_objfunction_freevalsareallocated(&data->uvobj.valscriptfunc))
                {
                    mc_allocator_free(data->pstate, data->uvobj.valscriptfunc.ufv.freevalsallocated);
                }
            }
            break;
        case MC_VAL_ARRAY:
            {
                mc_vallist_destroy(data->uvobj.valarray);
            }
            break;
        case MC_VAL_MAP:
            {
                mc_valdict_destroy(data->uvobj.valmap);
            }
            break;
        case MC_VAL_FUNCNATIVE:
            {
                mc_allocator_free(data->pstate, data->uvobj.valnativefunc.name);
            }
            break;
        case MC_VAL_EXTERNAL:
            {
                if(data->uvobj.valuserobject.datadestroyfn)
                {
                    data->uvobj.valuserobject.datadestroyfn(data->uvobj.valuserobject.data);
                }
            }
            break;
        case MC_VAL_ERROR:
            {
                mc_allocator_free(data->pstate, data->uvobj.valerror.message);
                mc_traceback_destroy(data->uvobj.valerror.traceback);
            }
            break;
        default:
            {
            }
            break;
    }
    data->type = MC_VAL_FREED;
}

bool mc_value_isallocated(mcvalue_t object)
{
    return object.isallocated;
}

mcgcmemory_t* mc_value_getmem(mcvalue_t obj)
{
    mcobjdata_t* data;
    data = mc_value_getallocateddata(obj);
    return data->mem;
}

const char* mc_valtype_getname(mcvaltype_t type)
{
    switch(type)
    {
        case MC_VAL_NONE:
        case MC_VAL_FREED:
        case MC_VAL_NULL:
            return "null";
        case MC_VAL_NUMBER:
            return "number";
        case MC_VAL_BOOL:
            return "boolean";
        case MC_VAL_STRING:
            return "string";
        case MC_VAL_FUNCNATIVE:
        case MC_VAL_FUNCSCRIPT:
            return "function";
        case MC_VAL_ARRAY:
            return "array";
        case MC_VAL_MAP:
            return "object";
        case MC_VAL_EXTERNAL:
            return "extern";
        case MC_VAL_ERROR:
            return "error";
        case MC_VAL_ANY:
            return "any";
    }
    return "invalid";
}

char* mc_valtype_getunionname(mcstate_t* state, mcvaltype_t type)
{
    bool inbetween;
    mcprinter_t* res;
    if(type == MC_VAL_ANY || type == MC_VAL_NONE || type == MC_VAL_FREED)
    {
        return mc_util_strdup(state, mc_valtype_getname(type));
    }
    res = mc_printer_make(state, NULL);
    if(!res)
    {
        return NULL;
    }
    inbetween = false;
#define CHECK_TYPE(t)                                    \
    do                                                   \
    {                                                    \
        if((type & t) == t)                              \
        {                                                \
            if(inbetween)                               \
            {                                            \
                mc_printer_puts(res, "|");                 \
            }                                            \
            mc_printer_puts(res, mc_valtype_getname(t)); \
            inbetween = true;                           \
        }                                                \
    } while(0)

    CHECK_TYPE(MC_VAL_NUMBER);
    CHECK_TYPE(MC_VAL_BOOL);
    CHECK_TYPE(MC_VAL_STRING);
    CHECK_TYPE(MC_VAL_NULL);
    CHECK_TYPE(MC_VAL_FUNCNATIVE);
    CHECK_TYPE(MC_VAL_ARRAY);
    CHECK_TYPE(MC_VAL_MAP);
    CHECK_TYPE(MC_VAL_FUNCSCRIPT);
    CHECK_TYPE(MC_VAL_EXTERNAL);
    CHECK_TYPE(MC_VAL_ERROR);
    return mc_printer_getstringanddestroy(res, NULL);
}

mcvalue_t mc_value_copydeep(mcstate_t* state, mcvalue_t obj)
{
    mcvalue_t res;
    mcvaldict_t* copies;
    copies = mc_valdict_make(state, sizeof(mcvalue_t), sizeof(mcvalue_t));
    if(!copies)
    {
        return mc_value_makenull();
    }
    res = mc_value_copydeepintern(state, obj, copies);
    mc_valdict_destroy(copies);
    return res;
}

mcvalue_t mc_value_copyflat(mcstate_t* state, mcvalue_t obj)
{
    bool ok;
    mcvalue_t copy;
    mcvaltype_t type;
    copy = mc_value_makenull();
    type = mc_value_gettype(obj);
    switch(type)
    {
        case MC_VAL_ANY:
        case MC_VAL_FREED:
        case MC_VAL_NONE:
            {
                MC_ASSERT(false);
                copy = mc_value_makenull();
            }
            break;
        case MC_VAL_NUMBER:
        case MC_VAL_BOOL:
        case MC_VAL_NULL:
        case MC_VAL_FUNCSCRIPT:
        case MC_VAL_FUNCNATIVE:
        case MC_VAL_ERROR:
            {
                copy = obj;
            }
            break;
        case MC_VAL_STRING:
            {
                size_t len;
                const char* str;
                str = mc_valstring_getdata(obj);
                len = mc_valstring_getlength(obj);
                copy = mc_value_makestringlen(state, str, len);
            }
            break;
        case MC_VAL_ARRAY:
            {
                int i;
                int len;
                mcvalue_t item;
                len = mc_valarray_getlength(obj);
                copy = mc_value_makearraycapacity(state, len);
                if(mc_value_isnull(copy))
                {
                    return mc_value_makenull();
                }
                for(i = 0; i < len; i++)
                {
                    item = mc_valarray_getvalueat(obj, i);
                    ok = mc_valarray_push(copy, item);
                    if(!ok)
                    {
                        return mc_value_makenull();
                    }
                }
            }
            break;
        case MC_VAL_MAP:
            {
                int i;
                mcvalue_t key;
                mcvalue_t val;
                copy = mc_value_makemap(state);
                for(i = 0; i < mc_valmap_getlength(obj); i++)
                {
                    key = mc_valmap_getkeyat(obj, i);
                    val = mc_valmap_getvalueat(obj, i);
                    ok = mc_valmap_setvalue(copy, key, val);
                    if(!ok)
                    {
                        return mc_value_makenull();
                    }
                }
            }
            break;
        case MC_VAL_EXTERNAL:
            {
                void* datacopy;
                mcobjuserdata_t* objext;
                copy = mc_value_makeuserobject(state, NULL);
                if(mc_value_isnull(copy))
                {
                    return mc_value_makenull();
                }
                objext = mc_valuserobject_getdata(obj);
                datacopy = NULL;
                if(objext->datacopyfn)
                {
                    datacopy = objext->datacopyfn(objext->data);
                }
                else
                {
                    datacopy = objext->data;
                }
                mc_valuserobject_setdata(copy, datacopy);
                mc_valuserobject_setdestroyfunction(copy, objext->datadestroyfn);
                mc_valuserobject_setcopyfunction(copy, objext->datacopyfn);
            }
            break;
    }
    return copy;
}

bool mc_value_compare(mcvalue_t a, mcvalue_t b, mcvalcmpresult_t* cres)
{
    const char* astring;
    const char* bstring;
    int alen;
    int blen;
    intptr_t adataval;
    intptr_t bdataval;
    mcfloat_t dnleft;
    mcfloat_t dnright;
    unsigned long ahash;
    unsigned long bhash;
    mcvaltype_t atype;
    mcvaltype_t btype;
    /*
    if(a.odata == b.odata)
    {
        return 0;
    }
    */
    cres->result = 1;
    atype = mc_value_gettype(a);
    btype = mc_value_gettype(b);
    if((atype == MC_VAL_NUMBER || atype == MC_VAL_BOOL || atype == MC_VAL_NULL) && (btype == MC_VAL_NUMBER || btype == MC_VAL_BOOL || btype == MC_VAL_NULL))
    {
        dnleft = mc_value_getnumber(a);
        dnright = mc_value_getnumber(b);
        cres->result = (dnleft - dnright);
        return true;
    }
    if(atype == btype && atype == MC_VAL_STRING)
    {
        alen = mc_valstring_getlength(a);
        blen = mc_valstring_getlength(b);
        #if 0
        fprintf(stderr, "mc_value_compare: alen=%d, blen=%d\n", alen, blen);
        #endif
        if(alen != blen)
        {
            cres->result = alen - blen;
            return false;
        }
        ahash = mc_valstring_gethash(a);
        bhash = mc_valstring_gethash(b);
        if(ahash != bhash)
        {
            cres->result = ahash - bhash;
            return false;
        }
        astring = mc_valstring_getdata(a);
        bstring = mc_valstring_getdata(b);
        if(strncmp(astring, bstring, alen) == 0)
        {
            cres->result = 0;
            return true;
        }
        else
        {
            cres->result = 1;
        }
        return true;
    }
    if((mc_value_isallocated(a) || mc_value_isnull(a)) && (mc_value_isallocated(b) || mc_value_isnull(b)))
    {
        adataval = (intptr_t)mc_value_getallocateddata(a);
        bdataval = (intptr_t)mc_value_getallocateddata(b);
        cres->result = (mcfloat_t)(adataval - bdataval);
        return true;
    }
    return false;
}


bool mc_value_equals(mcvalue_t a, mcvalue_t b)
{
    bool ok;
    mcvalcmpresult_t cres;
    mcvaltype_t atype;
    mcvaltype_t btype;
    (void)ok;
    atype = mc_value_gettype(a);
    btype = mc_value_gettype(b);
    if(atype != btype)
    {
        return false;
    }
    ok = mc_value_compare(a, b, &cres);
    return MC_UTIL_CMPFLOAT(cres.result, 0);
}

mcobjuserdata_t* mc_valuserobject_getdata(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_EXTERNAL);
    data = mc_value_getallocateddata(object);
    return &data->uvobj.valuserobject;
}

bool mc_valuserobject_setdestroyfunction(mcvalue_t object, mcitemdestroyfn_t dfn)
{
    mcobjuserdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_EXTERNAL);
    data = mc_valuserobject_getdata(object);
    if(!data)
    {
        return false;
    }
    data->datadestroyfn = dfn;
    return true;
}

bool mc_value_getbool(mcvalue_t obj)
{
    if(mc_value_isnumber(obj))
    {
        return obj.uval.valnumber;
    }
    return obj.uval.valbool;
}

mcfloat_t mc_value_getnumber(mcvalue_t obj)
{
    if(mc_value_isnumber(obj))
    {
        if(obj.type == MC_VAL_BOOL)
        {
            return obj.uval.valbool;
        }
        return obj.uval.valnumber;
    }
    return obj.uval.valnumber;
}

MCINLINE const char* mc_valstring_getdata(mcvalue_t object)
{
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    return mc_valstring_getdataintern(object);
}

int mc_valstring_getlength(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valstring.strbuf->length;
}

void mc_string_setlength(mcvalue_t object, int len)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    data = mc_value_getallocateddata(object);
    data->uvobj.valstring.strbuf->length = len;
}


MCINLINE char* mc_valstring_getmutabledata(mcvalue_t object)
{
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    return mc_valstring_getdataintern(object);
}

bool mc_valstring_appendlen(mcvalue_t obj, const char* src, size_t len)
{
    mcobjdata_t* data;
    mcobjstring_t* string;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_STRING);
    data = mc_value_getallocateddata(obj);
    string = &data->uvobj.valstring;
    dyn_strbuf_appendstrn(string->strbuf, src, len);
    return true;
}

bool mc_valstring_appendformatv(mcvalue_t obj, const char* fmt, va_list va)
{
    mcobjdata_t* data;
    mcobjstring_t* string;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_STRING);
    data = mc_value_getallocateddata(obj);
    string = &data->uvobj.valstring;
    dyn_strbuf_appendformatv(string->strbuf, fmt, va);
    return true;
}

bool mc_valstring_appendformat(mcvalue_t obj, const char* fmt, ...)
{
    bool r;
    va_list va;
    va_start(va, fmt);
    r = mc_valstring_appendformatv(obj, fmt, va);
    va_end(va);
    return r;
}

unsigned long mc_valstring_gethash(mcvalue_t obj)
{
    size_t len;
    const char* str;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_STRING);
    data = mc_value_getallocateddata(obj);
    if(data->uvobj.valstring.hash == 0)
    {
        len = mc_valstring_getlength(obj);
        str = mc_valstring_getdata(obj);
        data->uvobj.valstring.hash = mc_util_hashdata(str, len);
        if(data->uvobj.valstring.hash == 0)
        {
            data->uvobj.valstring.hash = 1;
        }
    }
    return data->uvobj.valstring.hash;
}

mcobjfuncscript_t* mc_value_functiongetscriptfunction(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(object);
    return &data->uvobj.valscriptfunc;
}

MCINLINE mcobjfuncnative_t* mc_value_functiongetnativefunction(mcvalue_t obj)
{
    mcobjdata_t* data = mc_value_getallocateddata(obj);
    return &data->uvobj.valnativefunc;
}

const char* mc_value_functiongetname(mcvalue_t obj)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return NULL;
    }
    if(data->uvobj.valscriptfunc.ownsdata)
    {
        return data->uvobj.valscriptfunc.unamev.fallocname;
    }
    return data->uvobj.valscriptfunc.unamev.fconstname;
}

mcvalue_t mc_value_functiongetfreevalat(mcvalue_t obj, int ix)
{
    mcobjdata_t* data;
    mcobjfuncscript_t* fun;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return mc_value_makenull();
    }
    fun = &data->uvobj.valscriptfunc;
    MC_ASSERT(ix >= 0 && ix < fun->freevalscount);
    if(ix < 0 || ix >= fun->freevalscount)
    {
        return mc_value_makenull();
    }
    if(mc_objfunction_freevalsareallocated(fun))
    {
        return fun->ufv.freevalsallocated[ix];
    }
    return fun->ufv.freevalsstack[ix];
}

void mc_value_functionsetfreevalat(mcvalue_t obj, int ix, mcvalue_t val)
{
    mcobjdata_t* data;
    mcobjfuncscript_t* fun;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return;
    }
    fun = &data->uvobj.valscriptfunc;
    MC_ASSERT(ix >= 0 && ix < fun->freevalscount);
    if(ix < 0 || ix >= fun->freevalscount)
    {
        return;
    }
    if(mc_objfunction_freevalsareallocated(fun))
    {
        fun->ufv.freevalsallocated[ix] = val;
    }
    else
    {
        fun->ufv.freevalsstack[ix] = val;
    }
}

mcvalue_t* mc_value_functiongetfreevals(mcvalue_t obj)
{
    mcobjdata_t* data;
    mcobjfuncscript_t* fun;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return NULL;
    }
    fun = &data->uvobj.valscriptfunc;
    if(mc_objfunction_freevalsareallocated(fun))
    {
        return fun->ufv.freevalsallocated;
    }
    return fun->ufv.freevalsstack;
}

const char* mc_value_errorgetmessage(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ERROR);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valerror.message;
}

void mc_value_errorsettraceback(mcvalue_t object, mctraceback_t* traceback)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ERROR);
    if(mc_value_gettype(object) != MC_VAL_ERROR)
    {
        return;
    }
    data = mc_value_getallocateddata(object);
    MC_ASSERT(data->uvobj.valerror.traceback == NULL);
    data->uvobj.valerror.traceback = traceback;
}

mctraceback_t* mc_value_errorgettraceback(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ERROR);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valerror.traceback;
}

bool mc_valuserobject_setdata(mcvalue_t object, void* extdata)
{
    mcobjuserdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_EXTERNAL);
    data = mc_valuserobject_getdata(object);
    if(!data)
    {
        return false;
    }
    data->data = extdata;
    return true;
}

bool mc_valuserobject_setcopyfunction(mcvalue_t object, mcitemcopyfn_t copyfn)
{
    mcobjuserdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_EXTERNAL);
    data = mc_valuserobject_getdata(object);
    if(!data)
    {
        return false;
    }
    data->datacopyfn = copyfn;
    return true;
}

mcvalue_t mc_valarray_getvalueat(mcvalue_t object, size_t ix)
{
    mcvalue_t* res;
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_valarray_getinternalarray(object);
    if(ix >= mc_vallist_count(array))
    {
        return mc_value_makenull();
    }
    res = (mcvalue_t*)mc_vallist_getp(array, ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

bool mc_valarray_setvalueat(mcvalue_t object, size_t ix, mcvalue_t val)
{
    size_t len;
    size_t toadd;
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_valarray_getinternalarray(object);
    len = mc_vallist_count(array);
    if((ix >= len) || (len == 0))
    {
        toadd = len+1;
        #if 0
            fprintf(stderr, "ix=%d toadd=%d len=%d\n", ix, toadd, len);
        #endif
        while(toadd != (ix+2))
        {
            mc_valarray_push(object, mc_value_makenull());
            toadd++;
        }
    }
    return mc_vallist_set(array, ix, val);
}

bool mc_valarray_push(mcvalue_t object, mcvalue_t val)
{
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_valarray_getinternalarray(object);
    return mc_vallist_push(array, val);
}

int mc_valarray_getlength(mcvalue_t object)
{
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_valarray_getinternalarray(object);
    return mc_vallist_count(array);
}

mcvalue_t mc_valarray_pop(mcvalue_t object)
{
    mcvalue_t dest;
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_valarray_getinternalarray(object);
    if(mc_vallist_pop(array, &dest))
    {
        return dest;
    }
    return mc_value_makenull();
}

bool mc_valarray_removevalueat(mcvalue_t object, int ix)
{
    mcvallist_t* array;
    array = mc_valarray_getinternalarray(object);
    return mc_vallist_removeat(array, ix);
}

int mc_valmap_getlength(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    return mc_valdict_count(data->uvobj.valmap);
}

mcvalue_t mc_valmap_getkeyat(mcvalue_t object, int ix)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)mc_valdict_getkeyat(data->uvobj.valmap, ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

mcvalue_t mc_valmap_getvalueat(mcvalue_t object, int ix)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)mc_valdict_getvalueat(data->uvobj.valmap, ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

bool mc_valmap_setvalueat(mcvalue_t object, int ix, mcvalue_t val)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    if(ix >= mc_valmap_getlength(object))
    {
        return false;
    }
    data = mc_value_getallocateddata(object);
    return mc_valdict_setvalueat(data->uvobj.valmap, ix, &val);
}

mcvalue_t mc_valmap_getkvpairat(mcstate_t* state, mcvalue_t object, int ix)
{
    mcvalue_t key;
    mcvalue_t val;
    mcvalue_t res;
    mcvalue_t valobj;
    mcvalue_t keyobj;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    if(ix >= mc_valdict_count(data->uvobj.valmap))
    {
        return mc_value_makenull();
    }
    key = mc_valmap_getkeyat(object, ix);
    val = mc_valmap_getvalueat(object, ix);
    res = mc_value_makemap(state);
    if(mc_value_isnull(res))
    {
        return mc_value_makenull();
    }
    keyobj = mc_value_makestring(state, "key");
    if(mc_value_isnull(keyobj))
    {
        return mc_value_makenull();
    }
    mc_valmap_setvalue(res, keyobj, key);
    valobj = mc_value_makestring(state, "value");
    if(mc_value_isnull(valobj))
    {
        return mc_value_makenull();
    }
    mc_valmap_setvalue(res, valobj, val);
    return res;
}

bool mc_valmap_setvalue(mcvalue_t object, mcvalue_t key, mcvalue_t val)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    return mc_valdict_setkv(data->uvobj.valmap, &key, &val);
}

bool mc_valmap_setvalstring(mcvalue_t object, const char* strkey, mcvalue_t val)
{
    mcstate_t* state;
    mcvalue_t vkey;
    state = mc_value_getallocateddata(object)->pstate;
    vkey = mc_value_makestring(state, strkey);
    return mc_valmap_setvalue(object, vkey, val);
}

mcvalue_t mc_valmap_getvalue(mcvalue_t object, mcvalue_t key)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)mc_valdict_get(data->uvobj.valmap, &key);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

bool mc_valmap_haskey(mcvalue_t object, mcvalue_t key)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)mc_valdict_get(data->uvobj.valmap, &key);
    return res != NULL;
}

mcvalue_t mc_value_copydeepfuncscript(mcstate_t* state, mcvalue_t obj, mcvaldict_t* copies)
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
    mccompiledprogram_t* comprescopy;
    function = mc_value_functiongetscriptfunction(obj);
    bytecodecopy = NULL;
    srcpositionscopy = NULL;
    comprescopy = NULL;
    bytecodecopy = (uint8_t*)mc_allocator_malloc(state, sizeof(uint8_t) * function->compiledprogcode->count);
    if(!bytecodecopy)
    {
        return mc_value_makenull();
    }
    memcpy(bytecodecopy, function->compiledprogcode->bytecode, sizeof(uint8_t) * function->compiledprogcode->count);
    srcpositionscopy = (mcastlocation_t*)mc_allocator_malloc(state, sizeof(mcastlocation_t) * function->compiledprogcode->count);
    if(!srcpositionscopy)
    {
        mc_allocator_free(state, bytecodecopy);
        return mc_value_makenull();
    }
    memcpy(srcpositionscopy, function->compiledprogcode->progsrcposlist, sizeof(mcastlocation_t) * function->compiledprogcode->count);
    comprescopy = mc_astcompresult_make(state, bytecodecopy, srcpositionscopy, function->compiledprogcode->count);
    /*
    * todo: add compilation result copy function
    */
    if(!comprescopy)
    {
        mc_allocator_free(state, srcpositionscopy);
        mc_allocator_free(state, bytecodecopy);
        return mc_value_makenull();
    }
    copy = mc_value_makefuncscript(state, mc_value_functiongetname(obj), comprescopy, true, function->numlocals, function->numargs, 0);
    if(mc_value_isnull(copy))
    {
        mc_astcompresult_destroy(comprescopy);
        return mc_value_makenull();
    }
    ok = mc_valdict_setkv(copies, &obj, &copy);
    if(!ok)
    {
        return mc_value_makenull();
    }
    functioncopy = mc_value_functiongetscriptfunction(copy);
    if(mc_objfunction_freevalsareallocated(function))
    {
        functioncopy->ufv.freevalsallocated = (mcvalue_t*)mc_allocator_malloc(state, sizeof(mcvalue_t) * function->freevalscount);
        if(!functioncopy->ufv.freevalsallocated)
        {
            return mc_value_makenull();
        }
    }
    functioncopy->freevalscount = function->freevalscount;
    for(i = 0; i < function->freevalscount; i++)
    {
        freeval = mc_value_functiongetfreevalat(obj, i);
        freevalcopy = mc_value_copydeepintern(state, freeval, copies);
        if(!mc_value_isnull(freeval) && mc_value_isnull(freevalcopy))
        {
            return mc_value_makenull();
        }
        mc_value_functionsetfreevalat(copy, i, freevalcopy);
    }
    return copy;
}

mcvalue_t mc_value_copydeeparray(mcstate_t* state, mcvalue_t obj, mcvaldict_t* copies)
{
    bool ok;
    int i;
    int len;
    mcvalue_t copy;
    mcvalue_t item;
    mcvalue_t itemcopy;
    len = mc_valarray_getlength(obj);
    copy = mc_value_makearraycapacity(state, len);
    if(mc_value_isnull(copy))
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
        item = mc_valarray_getvalueat(obj, i);
        itemcopy = mc_value_copydeepintern(state, item, copies);
        if(!mc_value_isnull(item) && mc_value_isnull(itemcopy))
        {
            return mc_value_makenull();
        }
        ok = mc_valarray_push(copy, itemcopy);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return copy;
}

mcvalue_t mc_value_copydeepmap(mcstate_t* state, mcvalue_t obj, mcvaldict_t* copies)
{
    bool ok;
    int i;
    mcvalue_t key;
    mcvalue_t val;
    mcvalue_t copy;
    mcvalue_t keycopy;
    mcvalue_t valcopy;
    copy = mc_value_makemap(state);
    if(mc_value_isnull(copy))
    {
        return mc_value_makenull();
    }
    ok = mc_valdict_setkv(copies, &obj, &copy);
    if(!ok)
    {
        return mc_value_makenull();
    }
    for(i = 0; i < mc_valmap_getlength(obj); i++)
    {
        key = mc_valmap_getkeyat(obj, i);
        val = mc_valmap_getvalueat(obj, i);
        keycopy = mc_value_copydeepintern(state, key, copies);
        if(!mc_value_isnull(key) && mc_value_isnull(keycopy))
        {
            return mc_value_makenull();
        }
        valcopy = mc_value_copydeepintern(state, val, copies);
        if(!mc_value_isnull(val) && mc_value_isnull(valcopy))
        {
            return mc_value_makenull();
        }
        ok = mc_valmap_setvalue(copy, keycopy, valcopy);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return copy;
}

mcvalue_t mc_value_copydeepintern(mcstate_t* state, mcvalue_t obj, mcvaldict_t* copies)
{
    mcvaltype_t type;
    mcvalue_t copy;
    mcvalue_t* copyptr;
    copyptr = (mcvalue_t*)mc_valdict_get(copies, &obj);
    if(copyptr)
    {
        return *copyptr;
    }
    copy = mc_value_makenull();
    type = mc_value_gettype(obj);
    switch(type)
    {
        case MC_VAL_FREED:
        case MC_VAL_ANY:
        case MC_VAL_NONE:
            {
                MC_ASSERT(false);
                copy = mc_value_makenull();
            }
            break;
        case MC_VAL_NUMBER:
        case MC_VAL_BOOL:
        case MC_VAL_NULL:
        case MC_VAL_FUNCNATIVE:
            {
                copy = obj;
            }
            break;
        case MC_VAL_STRING:
            {
                int len;
                const char* str;
                str = mc_valstring_getdata(obj);
                len = mc_valstring_getlength(obj);
                copy = mc_value_makestringlen(state, str, len);
                return copy;
            }
            break;
        case MC_VAL_FUNCSCRIPT:
            {
                return mc_value_copydeepfuncscript(state, obj, copies);
            }
            break;
        case MC_VAL_ARRAY:
            {
                return mc_value_copydeeparray(state, obj, copies);
            }
            break;
        case MC_VAL_MAP:
            {
                return mc_value_copydeepmap(state, obj, copies);
            }
            break;
        case MC_VAL_EXTERNAL:
            {
                copy = mc_value_copyflat(state, obj);
            }
            break;
        case MC_VAL_ERROR:
            {
                copy = obj;
            }
            break;
    }
    return copy;
}

bool mc_value_equalswrapped(mcvalue_t* aptr, mcvalue_t* bptr)
{
    mcvalue_t a;
    mcvalue_t b;
    a = *aptr;
    b = *bptr;
    return mc_value_equals(a, b);
}

unsigned long mc_value_hash(mcvalue_t* objptr)
{
    bool bval;
    mcfloat_t dval;
    mcvalue_t obj;
    mcvaltype_t type;
    obj = *objptr;
    type = mc_value_gettype(obj);
    switch(type)
    {
        case MC_VAL_NUMBER:
            {
                dval = mc_value_getnumber(obj);
                return mc_util_hashdouble(dval);
            }
            break;
        case MC_VAL_BOOL:
            {
                bval = mc_value_getbool(obj);
                return bval;
            }
            break;
        case MC_VAL_STRING:
            {
                return mc_valstring_gethash(obj);
            }
            break;
        default:
            {
            }
            break;
    }
    return 0;
}

mcvallist_t* mc_valarray_getinternalarray(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valarray;
}

bool mc_value_isnumber(mcvalue_t o)
{
    return (o.type == MC_VAL_NUMBER || o.type == MC_VAL_BOOL);
}

bool mc_objfunction_freevalsareallocated(mcobjfuncscript_t* fun)
{
    return fun->freevalscount >= MC_UTIL_STATICARRAYSIZE(fun->ufv.freevalsstack);
}

MCINLINE char* mc_valstring_getdataintern(mcvalue_t object)
{
    mcobjdata_t* data;
    data = mc_value_getallocateddata(object);
    MC_ASSERT(data->type == MC_VAL_STRING);
    return data->uvobj.valstring.strbuf->data;
}

mcastscopecomp_t* mc_astcompscope_make(mcstate_t* state, mcastscopecomp_t* outer)
{
    mcastscopecomp_t* scope;
    scope = (mcastscopecomp_t*)mc_allocator_malloc(state, sizeof(mcastscopecomp_t));
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
    scope->scopesrcposlist = mc_basicarray_make(state, sizeof(mcastlocation_t));
    if(!scope->scopesrcposlist)
    {
        goto err;
    }
    scope->ipstackbreak = mc_basicarray_make(state, sizeof(int));
    if(!scope->ipstackbreak)
    {
        goto err;
    }
    scope->ipstackcontinue = mc_basicarray_make(state, sizeof(int));
    if(!scope->ipstackcontinue)
    {
        goto err;
    }
    return scope;
err:
    mc_astcompscope_destroy(scope);
    return NULL;
}

void mc_astcompscope_destroy(mcastscopecomp_t* scope)
{
    mcstate_t* state;
    state = scope->pstate;
    mc_basicarray_destroy(scope->ipstackcontinue);
    mc_basicarray_destroy(scope->ipstackbreak);
    mc_basicarray_destroy(scope->bytecode);
    mc_basicarray_destroy(scope->scopesrcposlist);
    mc_allocator_free(state, scope);
}

mccompiledprogram_t* mc_astcompscope_orphanresult(mcastscopecomp_t* scope)
{
    mccompiledprogram_t* res;
    res = mc_astcompresult_make(scope->pstate, (uint8_t*)mc_basicarray_data(scope->bytecode), (mcastlocation_t*)mc_basicarray_data(scope->scopesrcposlist), mc_basicarray_count(scope->bytecode));
    if(!res)
    {
        return NULL;
    }
    mc_basicarray_orphandata(scope->bytecode);
    mc_basicarray_orphandata(scope->scopesrcposlist);
    return res;
}

mccompiledprogram_t* mc_astcompresult_make(mcstate_t* state, uint8_t* bytecode, mcastlocation_t* srcposlist, int count)
{
    mccompiledprogram_t* res;
    res = (mccompiledprogram_t*)mc_allocator_malloc(state, sizeof(mccompiledprogram_t));
    if(!res)
    {
        return NULL;
    }
    memset(res, 0, sizeof(mccompiledprogram_t));
    res->pstate = state;
    res->bytecode = bytecode;
    res->progsrcposlist = srcposlist;
    res->count = count;
    return res;
}

void mc_astcompresult_destroy(mccompiledprogram_t* res)
{
    mcstate_t* state;
    if(!res)
    {
        return;
    }
    state = res->pstate;
    mc_allocator_free(state, res->bytecode);
    mc_allocator_free(state, res->progsrcposlist);
    mc_allocator_free(state, res);
}

bool mc_lexer_init(mcastlexer_t* lex, mcstate_t* state, mcerrlist_t* errs, const char* input, mcastcompiledfile_t* file)
{
    bool ok;
    lex->pstate = state;
    lex->errors = errs;
    lex->inputsource = input;
    lex->inputlength = (int)mc_util_strlen(input);
    lex->position = 0;
    lex->nextposition = 0;
    lex->ch = '\0';
    if(file)
    {
        lex->line = mc_ptrlist_count(file->lines);
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
    mc_asttoken_init(&lex->prevtoken, MC_TOK_INVALID, NULL, 0);
    mc_asttoken_init(&lex->currtoken, MC_TOK_INVALID, NULL, 0);
    mc_asttoken_init(&lex->peektoken, MC_TOK_INVALID, NULL, 0);
    return true;
}

bool mc_lexer_failed(mcastlexer_t* lex)
{
    return lex->failed;
}

void mc_lexer_conttplstring(mcastlexer_t* lex)
{
    lex->continuetplstring = true;
}

bool mc_lexer_currtokenis(mcastlexer_t* lex, mcasttoktype_t type)
{
    return lex->currtoken.type == type;
}

bool mc_lexer_peektokenis(mcastlexer_t* lex, mcasttoktype_t type)
{
    return lex->peektoken.type == type;
}

bool mc_lexer_nexttoken(mcastlexer_t* lex)
{
    lex->prevtoken = lex->currtoken;
    lex->currtoken = lex->peektoken;
    lex->peektoken = mc_lexer_nexttokinternal(lex);
    return !lex->failed;
}

bool mc_lexer_previoustoken(mcastlexer_t* lex)
{
    if(lex->prevtoken.type == MC_TOK_INVALID)
    {
        return false;
    }
    lex->peektoken = lex->currtoken;
    lex->currtoken = lex->prevtoken;
    mc_asttoken_init(&lex->prevtoken, MC_TOK_INVALID, NULL, 0);
    lex->ch = lex->prevstate.ch;
    lex->column = lex->prevstate.column;
    lex->line = lex->prevstate.line;
    lex->position = lex->prevstate.position;
    lex->nextposition = lex->prevstate.nextposition;
    return true;
}

mcasttoken_t mc_lexer_nexttokinternal(mcastlexer_t* lex)
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
        outtok.pos = mc_astlocation_make(lex->file, lex->line, lex->column);
        c = lex->continuetplstring ? '`' : lex->ch;
        switch(c)
        {
            case '\0':
                {
                    mc_asttoken_init(&outtok, MC_TOK_EOF, "EOF", 3);
                }
                break;
            case '=':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_EQ, "==", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGN, "=", 1);
                    }
                }
                break;
            case '&':
                {
                    if(mc_lexer_peekchar(lex) == '&')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_AND, "&&", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNBINAND, "&=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_BINAND, "&", 1);
                    }
                }
                break;
            case '|':
                {
                    if(mc_lexer_peekchar(lex) == '|')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_OR, "||", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNBINOR, "|=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_BINOR, "|", 1);
                    }
                }
                break;
            case '^':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNBINXOR, "^=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_BINXOR, "^", 1);
                        break;
                    }
                }
                break;
            case '+':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNPLUS, "+=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '+')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_PLUSPLUS, "++", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_PLUS, "+", 1);
                        break;
                    }
                }
                break;
            case '-':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNMINUS, "-=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '-')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_MINUSMINUS, "--", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_UNARYMINUS, "-", 1);
                        break;
                    }
                }
                break;
            case '~':
                {
                    mc_asttoken_init(&outtok, MC_TOK_UNARYBINNOT, "~", 1);
                }
                break;
            case '!':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_NOTEQ, "!=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_BANG, "!", 1);
                    }
                }
                break;
            case '*':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNASTERISK, "*=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASTERISK, "*", 1);
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
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNSLASH, "/=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_SLASH, "/", 1);
                        break;
                    }
                }
                break;
            case '<':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_LTE, "<=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '<')
                    {
                        mc_lexer_readchar(lex);
                        if(mc_lexer_peekchar(lex) == '=')
                        {
                            mc_asttoken_init(&outtok, MC_TOK_ASSIGNLSHIFT, "<<=", 3);
                            mc_lexer_readchar(lex);
                        }
                        else
                        {
                            mc_asttoken_init(&outtok, MC_TOK_LSHIFT, "<<", 2);
                        }
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_LT, "<", 1);
                        break;
                    }
                }
                break;
            case '>':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_GTE, ">=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '>')
                    {
                        mc_lexer_readchar(lex);
                        if(mc_lexer_peekchar(lex) == '=')
                        {
                            mc_asttoken_init(&outtok, MC_TOK_ASSIGNRSHIFT, ">>=", 3);
                            mc_lexer_readchar(lex);
                        }
                        else
                        {
                            mc_asttoken_init(&outtok, MC_TOK_RSHIFT, ">>", 2);
                        }
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_GT, ">", 1);
                    }
                }
                break;
            case ',':
                {
                    mc_asttoken_init(&outtok, MC_TOK_COMMA, ",", 1);
                }
                break;
            case ';':
                {
                    mc_asttoken_init(&outtok, MC_TOK_SEMICOLON, ";", 1);
                }
                break;
            case ':':
                {
                    mc_asttoken_init(&outtok, MC_TOK_COLON, ":", 1);
                }
                break;
            case '(':
                {
                    mc_asttoken_init(&outtok, MC_TOK_LPAREN, "(", 1);
                }
                break;
            case ')':
                {
                    mc_asttoken_init(&outtok, MC_TOK_RPAREN, ")", 1);
                }
                break;
            case '{':
                {
                    mc_asttoken_init(&outtok, MC_TOK_LBRACE, "{", 1);
                }
                break;
            case '}':
                {
                    mc_asttoken_init(&outtok, MC_TOK_RBRACE, "}", 1);
                }
                break;
            case '[':
                {
                    mc_asttoken_init(&outtok, MC_TOK_LBRACKET, "[", 1);
                }
                break;
            case ']':
                {
                    mc_asttoken_init(&outtok, MC_TOK_RBRACKET, "]", 1);
                }
                break;
            case '.':
                {
                    mc_asttoken_init(&outtok, MC_TOK_DOT, ".", 1);
                }
                break;
            case '?':
                {
                    mc_asttoken_init(&outtok, MC_TOK_QUESTION, "?", 1);
                }
                break;
            case '%':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNPERCENT, "%=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_PERCENT, "%", 1);
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
                        mc_asttoken_init(&outtok, MC_TOK_STRING, str, len);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_INVALID, NULL, 0);
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
                        mc_asttoken_init(&outtok, MC_TOK_STRING, str, len);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_INVALID, NULL, 0);
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
                            mc_asttoken_init(&outtok, MC_TOK_TEMPLATESTRING, str, len);
                        }
                        else
                        {
                            mc_asttoken_init(&outtok, MC_TOK_STRING, str, len);
                        }
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_INVALID, NULL, 0);
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
                        mc_asttoken_init(&outtok, type, ident, identlen);
                        return outtok;
                    }
                    if(mc_lexer_charisdigit(lex->ch))
                    {
                        numberlen = 0;
                        number = mc_lexer_scannumber(lex, &numberlen);
                        mc_asttoken_init(&outtok, MC_TOK_NUMBER, number, numberlen);
                        return outtok;
                    }
                }
                break;
        }
        mc_lexer_readchar(lex);
        if(mc_lexer_failed(lex))
        {
            mc_asttoken_init(&outtok, MC_TOK_INVALID, NULL, 0);
        }
        lex->continuetplstring = false;
        return outtok;
    }
    /* NB. never reached; but keep the compiler from complaining. */
    return outtok;
}

bool mc_lexer_expectcurrent(mcastlexer_t* lex, mcasttoktype_t type)
{
    const char* actualtypestr;
    const char* expectedtypestr;
    if(mc_lexer_failed(lex))
    {
        return false;
    }
    if(!mc_lexer_currtokenis(lex, type))
    {
        expectedtypestr = mc_asttoken_typename(type);
        actualtypestr = mc_asttoken_typename(lex->currtoken.type);
        mc_errlist_addf(lex->errors, MC_ERROR_PARSING, lex->currtoken.pos, "expected token \"%s\", but got \"%s\"", expectedtypestr, actualtypestr);
        return false;
    }
    return true;
}

bool mc_lexer_readchar(mcastlexer_t* lex)
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

char mc_lexer_peekchar(mcastlexer_t* lex)
{
    if(lex->nextposition >= lex->inputlength)
    {
        return '\0';
    }
    return lex->inputsource[lex->nextposition];
}

bool mc_lexer_charisletter(char ch)
{
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

bool mc_lexer_charisdigit(char ch)
{
    return ch >= '0' && ch <= '9';
}

bool mc_lexer_charisoneof(char ch, const char* allowed, int allowedlen)
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

const char* mc_lexer_scanident(mcastlexer_t* lex, int* outlen)
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

const char* mc_lexer_scannumber(mcastlexer_t* lex, int* outlen)
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

const char* mc_lexer_scanstring(mcastlexer_t* lex, char delimiter, bool istemplate, bool* outtemplatefound, int* outlen)
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

mcasttoktype_t mc_lexer_lookupident(const char* ident, int len)
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
        { "let", MC_TOK_VAR },
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
        klen = mc_util_strlen(keywords[i].value);
        if(klen == len && MC_UTIL_STRNEQ(ident, keywords[i].value, len))
        {
            return keywords[i].type;
        }
    }
    return MC_TOK_IDENT;
}

void mc_lexer_skipspace(mcastlexer_t* lex)
{
    char ch;
    ch = lex->ch;
    while(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
    {
        mc_lexer_readchar(lex);
        ch = lex->ch;
    }
}

bool mc_lexer_addline(mcastlexer_t* lex, int offset)
{
    bool ok;
    size_t linelen;
    char* line;
    const char* linestart;
    const char* newlineptr;
    if(!lex->file)
    {
        return true;
    }
    if(lex->line < mc_ptrlist_count(lex->file->lines))
    {
        return true;
    }
    linestart = lex->inputsource + offset;
    newlineptr = strchr(linestart, '\n');
    line = NULL;
    if(!newlineptr)
    {
        line = mc_util_strdup(lex->pstate, linestart);
    }
    else
    {
        linelen = newlineptr - linestart;
        line = mc_util_strndup(lex->pstate, linestart, linelen);
    }
    if(!line)
    {
        lex->failed = true;
        return false;
    }
    ok = mc_ptrlist_push(lex->file->lines, line);
    if(!ok)
    {
        lex->failed = true;
        mc_allocator_free(lex->pstate, line);
        return false;
    }
    return true;
}

mcastcompiledfile_t* mc_compiledfile_make(mcstate_t* state, const char* path)
{
    size_t len;
    const char* lastslashpos;
    mcastcompiledfile_t* file;
    file = (mcastcompiledfile_t*)mc_allocator_malloc(state, sizeof(mcastcompiledfile_t));
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
    file->lines = mc_ptrlist_make(state, 0);
    if(!file->lines)
    {
        goto error;
    }
    return file;
error:
    mc_compiledfile_destroy(file);
    return NULL;
}

void mc_compiledfile_destroy(mcastcompiledfile_t* file)
{
    size_t i;
    void* item;
    mcstate_t* state;
    if(!file)
    {
        return;
    }
    state = file->pstate;
    for(i = 0; i < mc_ptrlist_count(file->lines); i++)
    {
        item = (void*)mc_ptrlist_get(file->lines, i);
        mc_allocator_free(state, item);
    }
    mc_ptrlist_destroy(file->lines, NULL);
    mc_allocator_free(state, file->dir_path);
    mc_allocator_free(state, file->path);
    mc_allocator_free(state, file);
}

mcastcompiler_t* mc_compiler_make(mcstate_t* state, mcconfig_t* config, mcgcmemory_t* mem, mcerrlist_t* errors, mcptrlist_t* files, mcglobalstore_t* gstore)
{
    bool ok;
    mcastcompiler_t* comp = (mcastcompiler_t*)mc_allocator_malloc(state, sizeof(mcastcompiler_t));
    if(!comp)
    {
        return NULL;
    }
    ok = mc_compiler_init(comp, state, config, mem, errors, files, gstore);
    if(!ok)
    {
        mc_allocator_free(state, comp);
        return NULL;
    }
    comp->pstate = state; 
    return comp;
}

void mc_compiler_destroy(mcastcompiler_t* comp)
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

mccompiledprogram_t* mc_compiler_compilesource(mcastcompiler_t* comp, const char* code)
{
    bool ok;
    mcastcompiler_t compshallowcopy;
    mcastscopecomp_t* compscope;
    mccompiledprogram_t* res;
    compscope = mc_compiler_getcompilationscope(comp);

    MC_ASSERT(mc_basicarray_count(comp->srcposstack) == 0);
    MC_ASSERT(mc_basicarray_count(compscope->bytecode) == 0);
    MC_ASSERT(mc_basicarray_count(compscope->ipstackbreak) == 0);
    MC_ASSERT(mc_basicarray_count(compscope->ipstackcontinue) == 0);

    mc_basicarray_clear(comp->srcposstack);
    mc_basicarray_clear(compscope->bytecode);
    mc_basicarray_clear(compscope->scopesrcposlist);
    mc_basicarray_clear(compscope->ipstackbreak);
    mc_basicarray_clear(compscope->ipstackcontinue);
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
    mc_compiler_emit(comp, MC_OPCODE_HALT, 0, 0);
    /* might've changed */
    compscope = mc_compiler_getcompilationscope(comp);
    MC_ASSERT(compscope->outer == NULL);
    compscope = mc_compiler_getcompilationscope(comp);
    res = mc_astcompscope_orphanresult(compscope);
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


mcastsymtable_t* mc_compiler_getsymtable(mcastcompiler_t* comp)
{
    mcastscopefile_t* filescope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    if(!filescope)
    {
        MC_ASSERT(false);
        return NULL;
    }
    return filescope->filesymtab;
}

void mc_compiler_setsymtable(mcastcompiler_t* comp, mcastsymtable_t* table)
{
    mcastscopefile_t* filescope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    if(!filescope)
    {
        MC_ASSERT(false);
        return;
    }
    filescope->filesymtab = table;
}

mcbasicarray_t* mc_compiler_getconstants(mcastcompiler_t* comp)
{
    return comp->constants;
}

bool mc_compiler_init(mcastcompiler_t* comp, mcstate_t* state, mcconfig_t* cfg, mcgcmemory_t* mem, mcerrlist_t* errors, mcptrlist_t* files, mcglobalstore_t* gstor)
{
    bool ok;
    memset(comp, 0, sizeof(mcastcompiler_t));
    comp->pstate = state;
    comp->config = cfg;
    comp->mem = mem;
    comp->errors = errors;
    comp->files = files;
    comp->compglobalstore = gstor;
    comp->filescopelist = mc_ptrlist_make(state, 0);
    if(!comp->filescopelist)
    {
        goto err;
    }
    comp->constants = mc_basicarray_make(state, sizeof(mcvalue_t));
    if(!comp->constants)
    {
        goto err;
    }
    comp->srcposstack = mc_basicarray_make(state, sizeof(mcastlocation_t));
    if(!comp->srcposstack)
    {
        goto err;
    }
    comp->modules = mc_genericdict_make(state, (mcitemcopyfn_t)mc_module_copy, (mcitemdestroyfn_t)mc_module_destroy);
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
    comp->stringconstposdict = mc_genericdict_make(comp->pstate, NULL, NULL);
    if(!comp->stringconstposdict)
    {
        goto err;
    }

    return true;
err:
    mc_compiler_deinit(comp);
    return false;
}

void mc_compiler_deinit(mcastcompiler_t* comp)
{
    size_t i;
    int* val;
    if(!comp)
    {
        return;
    }
    for(i = 0; i < mc_genericdict_count(comp->stringconstposdict); i++)
    {
        val = (int*)mc_genericdict_getvalueat(comp->stringconstposdict, i);
        mc_allocator_free(comp->pstate, val);
    }
    mc_genericdict_destroy(comp->stringconstposdict);
    while(mc_ptrlist_count(comp->filescopelist) > 0)
    {
        mc_compiler_filescopepop(comp);
    }
    while(mc_compiler_getcompilationscope(comp))
    {
        mc_compiler_popcompilationscope(comp);
    }
    mc_genericdict_destroyitemsanddict(comp->modules);
    mc_basicarray_destroy(comp->srcposstack);
    mc_basicarray_destroy(comp->constants);
    mc_ptrlist_destroy(comp->filescopelist, NULL);
    memset(comp, 0, sizeof(mcastcompiler_t));
}

bool mc_compiler_initshallowcopy(mcastcompiler_t* copy, mcastcompiler_t* src)
{
    bool ok;
    size_t i;
    int* val;
    int* valcopy;
    char* loadednamecopy;
    const char* key;
    const char* loadedname;
    mcgenericdict_t* modulescopy;
    mcbasicarray_t* constantscopy;
    mcptrlist_t* srcloadedmodulenames;
    mcptrlist_t* copyloadedmodulenames;
    mcastsymtable_t* srcst;
    mcastsymtable_t* srcstocopy;
    mcastsymtable_t* copyst;
    mcastscopefile_t* srcfilescope;
    mcastscopefile_t* copyfilescope;
    ok = mc_compiler_init(copy, src->pstate, src->config, src->mem, src->errors, src->files, src->compglobalstore);
    if(!ok)
    {
        return false;
    }
    srcst = mc_compiler_getsymtable(src);
    MC_ASSERT(mc_ptrlist_count(src->filescopelist) == 1);
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
    for(i = 0; i < mc_genericdict_count(src->stringconstposdict); i++)
    {
        key = mc_genericdict_getkeyat(src->stringconstposdict, i);
        val = (int*)mc_genericdict_getvalueat(src->stringconstposdict, i);
        valcopy = (int*)mc_allocator_malloc(src->pstate, sizeof(int));
        if(!valcopy)
        {
            goto err;
        }
        *valcopy = *val;
        ok = mc_genericdict_set(copy->stringconstposdict, key, valcopy);
        if(!ok)
        {
            mc_allocator_free(src->pstate, valcopy);
            goto err;
        }
    }
    srcfilescope = (mcastscopefile_t*)mc_ptrlist_top(src->filescopelist);
    copyfilescope = (mcastscopefile_t*)mc_ptrlist_top(copy->filescopelist);
    srcloadedmodulenames = srcfilescope->loadedmodnames;
    copyloadedmodulenames = copyfilescope->loadedmodnames;
    for(i = 0; i < mc_ptrlist_count(srcloadedmodulenames); i++)
    {

        loadedname = (const char*)mc_ptrlist_get(srcloadedmodulenames, i);
        loadednamecopy = mc_util_strdup(copy->pstate, loadedname);
        if(!loadednamecopy)
        {
            goto err;
        }
        ok = mc_ptrlist_push(copyloadedmodulenames, loadednamecopy);
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

mcopdefinition_t* mc_parser_makedef(mcopdefinition_t* dest, const char* name, int numop, int opa1, int opa2)
{
    dest->name = name;
    dest->numoperands = numop;
    dest->operandwidths[0] = opa1;
    dest->operandwidths[1] = opa2;
    return dest;
}

#define makecase(dest, opc, name, opnum, opa1, opa2) \
    case opc: \
    { \
        return mc_parser_makedef(def, name, opnum, opa1, opa2); \
    } \
    break;

mcopdefinition_t* mc_opdef_lookup(mcopdefinition_t* def, mcinternopcode_t op)
{
    switch(op)
    {
        makecase(def, MC_OPCODE_HALT, "halt", 0, 0, 0);
        makecase(def, MC_OPCODE_CONSTANT, "pushconst", 1, 2, 0);
        makecase(def, MC_OPCODE_ADD, "add", 0, 0, 0);
        makecase(def, MC_OPCODE_POP, "pop", 0, 0, 0);
        makecase(def, MC_OPCODE_SUB, "sub", 0, 0, 0);
        makecase(def, MC_OPCODE_MUL, "mul", 0, 0, 0);
        makecase(def, MC_OPCODE_DIV, "div", 0, 0, 0);
        makecase(def, MC_OPCODE_MOD, "mod", 0, 0, 0);
        makecase(def, MC_OPCODE_TRUE, "pushtrue", 0, 0, 0);
        makecase(def, MC_OPCODE_FALSE, "pushfalse", 0, 0, 0);
        makecase(def, MC_OPCODE_COMPARE, "compare", 0, 0, 0);
        makecase(def, MC_OPCODE_COMPAREEQ, "cmpequal", 0, 0, 0);
        makecase(def, MC_OPCODE_EQUAL, "isequal", 0, 0, 0);
        makecase(def, MC_OPCODE_NOTEQUAL, "notequal", 0, 0, 0);
        makecase(def, MC_OPCODE_GREATERTHAN, "greaterthan", 0, 0, 0);
        makecase(def, MC_OPCODE_GREATERTHANEQUAL, "greaterequal", 0, 0, 0);
        makecase(def, MC_OPCODE_MINUS, "minus", 0, 0, 0);
        makecase(def, MC_OPCODE_BINNOT, "binnot", 0, 0, 0);
        makecase(def, MC_OPCODE_BANG, "not", 0, 0, 0);
        makecase(def, MC_OPCODE_JUMP, "jump", 1, 2, 0);
        makecase(def, MC_OPCODE_JUMPIFFALSE, "jmpiffalse", 1, 2, 0);
        makecase(def, MC_OPCODE_JUMPIFTRUE, "jmpiftrue", 1, 2, 0);
        makecase(def, MC_OPCODE_NULL, "pushnull", 0, 0, 0);
        makecase(def, MC_OPCODE_GETMODULEGLOBAL, "getmodglobal", 1, 2, 0);
        makecase(def, MC_OPCODE_SETMODULEGLOBAL, "setmodglobal", 1, 2, 0);
        makecase(def, MC_OPCODE_DEFINEMODULEGLOBAL, "defmodglobal", 1, 2, 0);
        makecase(def, MC_OPCODE_ARRAY, "makearray", 1, 2, 0);
        makecase(def, MC_OPCODE_MAPSTART, "pushmakemapstart", 1, 2, 0);
        makecase(def, MC_OPCODE_MAPEND, "pushmakemapend", 1, 2, 0);
        makecase(def, MC_OPCODE_GETTHIS, "getthis", 0, 0, 0);
        makecase(def, MC_OPCODE_GETINDEX, "getindex", 0, 0, 0);
        makecase(def, MC_OPCODE_SETINDEX, "setindex", 0, 0, 0);
        makecase(def, MC_OPCODE_GETDOTINDEX, "getdotindex", 0, 0, 0);
        makecase(def, MC_OPCODE_GETVALUEAT, "getvalueat", 0, 0, 0);
        makecase(def, MC_OPCODE_CALL, "call", 1, 1, 0);
        makecase(def, MC_OPCODE_RETURNVALUE, "return.value", 0, 0, 0);
        makecase(def, MC_OPCODE_RETURN, "return.nil", 0, 0, 0);
        makecase(def, MC_OPCODE_GETLOCAL, "getlocal", 1, 1, 0);
        makecase(def, MC_OPCODE_DEFINELOCAL, "deflocal", 1, 1, 0);
        makecase(def, MC_OPCODE_SETLOCAL, "setlocal", 1, 1, 0);
        makecase(def, MC_OPCODE_GETGLOBALBUILTIN, "getglobalbuiltin", 1, 2, 0);
        makecase(def, MC_OPCODE_FUNCTION, "makefunction", 2, 2, 1);
        makecase(def, MC_OPCODE_GETFREE, "getfree", 1, 1, 0);
        makecase(def, MC_OPCODE_SETFREE, "setfree", 1, 1, 0);
        makecase(def, MC_OPCODE_CURRENTFUNCTION, "pushcurrentfunc", 0, 0, 0);
        makecase(def, MC_OPCODE_DUP, "dup", 0, 0, 0);
        makecase(def, MC_OPCODE_NUMBER, "pushnumber", 1, 8, 0);
        makecase(def, MC_OPCODE_FOREACHLEN, "foreach.len", 0, 0, 0);
        makecase(def, MC_OPCODE_SETRECOVER, "recoverset", 1, 2, 0);
        makecase(def, MC_OPCODE_BINOR, "bin.or", 0, 0, 0);
        makecase(def, MC_OPCODE_BINXOR, "bin.xor", 0, 0, 0);
        makecase(def, MC_OPCODE_BINAND, "bin.and", 0, 0, 0);
        makecase(def, MC_OPCODE_LSHIFT, "bin.lshift", 0, 0, 0);
        makecase(def, MC_OPCODE_RSHIFT, "bin.rshift", 0, 0, 0);
        makecase(def, MC_OPCODE_MAX, "INVALID_MAX", 0, 0, 0);
        default:
            {
                return NULL; 
            }
            break;
    }
    return def;
}
#undef makecase

const char* mc_opdef_getname(mcinternopcode_t op)
{
    mcopdefinition_t def;
    return mc_opdef_lookup(&def, op)->name;
}

mcastexpression_t* mc_astexpr_makeident(mcstate_t* state, mcastident_t* ident)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_IDENT);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprident = ident;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralnumber(mcstate_t* state, mcfloat_t val)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_NUMBERLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlitnumber = val;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralbool(mcstate_t* state, bool val)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_BOOLLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlitbool = val;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralstring(mcstate_t* state, char* value, size_t len)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STRINGLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlitstring.data = value;
    res->uexpr.exprlitstring.length = len;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralnull(mcstate_t* state)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_NULLLITERAL);
    if(!res)
    {
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralarray(mcstate_t* state, mcptrlist_t* values)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_ARRAYLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlitarray.litarritems = values;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralmap(mcstate_t* state, mcptrlist_t* keys, mcptrlist_t* values)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_MAPLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlitmap.keys = keys;
    res->uexpr.exprlitmap.values = values;
    return res;
}

mcastexpression_t* mc_astexpr_makeprefixexpr(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* right)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_PREFIX);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprprefix.op = op;
    res->uexpr.exprprefix.right = right;
    return res;
}

mcastexpression_t* mc_astexpr_makeinfixexpr(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* left, mcastexpression_t* right)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_INFIX);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprinfix.op = op;
    res->uexpr.exprinfix.left = left;
    res->uexpr.exprinfix.right = right;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralfunction(mcstate_t* state, mcptrlist_t* params, mcastcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_FUNCTIONLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlitfunction.name = NULL;
    res->uexpr.exprlitfunction.funcparamlist = params;
    res->uexpr.exprlitfunction.body = body;
    return res;
}

mcastexpression_t* mc_astexpr_makecallexpr(mcstate_t* state, mcastexpression_t* function, mcptrlist_t* args)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_CALL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprcall.function = function;
    res->uexpr.exprcall.args = args;
    return res;
}

mcastexpression_t* mc_astexpr_makeindexexpr(mcstate_t* state, mcastexpression_t* left, mcastexpression_t* index, bool isdot)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_INDEX);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprindex.isdot = isdot;
    res->uexpr.exprindex.left = left;
    res->uexpr.exprindex.index = index;
    return res;
}

mcastexpression_t* mc_astexpr_makeassignexpr(mcstate_t* state, mcastexpression_t* dest, mcastexpression_t* source, bool is_postfix)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_ASSIGN);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprassign.dest = dest;
    res->uexpr.exprassign.source = source;
    res->uexpr.exprassign.is_postfix = is_postfix;
    return res;
}

mcastexpression_t* mc_astexpr_makelogicalexpr(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* left, mcastexpression_t* right)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_LOGICAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlogical.op = op;
    res->uexpr.exprlogical.left = left;
    res->uexpr.exprlogical.right = right;
    return res;
}

mcastexpression_t* mc_astexpr_maketernaryexpr(mcstate_t* state, mcastexpression_t* test, mcastexpression_t* ift, mcastexpression_t* iffalse)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_TERNARY);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprternary.tercond = test;
    res->uexpr.exprternary.teriftrue = ift;
    res->uexpr.exprternary.teriffalse = iffalse;
    return res;
}

void mc_astexpr_destroy(mcastexpression_t* expr)
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
            }
            break;
        case MC_EXPR_IDENT:
            {
                mc_astident_destroy(expr->uexpr.exprident);
            }
            break;
        case MC_EXPR_NUMBERLITERAL:
        case MC_EXPR_BOOLLITERAL:
            {
            }
            break;
        case MC_EXPR_STRINGLITERAL:
            {
                mc_allocator_free(expr->pstate, expr->uexpr.exprlitstring.data);
            }
            break;
        case MC_EXPR_NULLLITERAL:
            {
            }
            break;
        case MC_EXPR_ARRAYLITERAL:
            {
                mc_ptrlist_destroy(expr->uexpr.exprlitarray.litarritems, (mcitemdestroyfn_t)mc_astexpr_destroy);
            }
            break;
        case MC_EXPR_MAPLITERAL:
            {
                mc_ptrlist_destroy(expr->uexpr.exprlitmap.keys, (mcitemdestroyfn_t)mc_astexpr_destroy);
                mc_ptrlist_destroy(expr->uexpr.exprlitmap.values, (mcitemdestroyfn_t)mc_astexpr_destroy);
            }
            break;
        case MC_EXPR_PREFIX:
            {
                mc_astexpr_destroy(expr->uexpr.exprprefix.right);
            }
            break;
        case MC_EXPR_INFIX:
            {
                mc_astexpr_destroy(expr->uexpr.exprinfix.left);
                mc_astexpr_destroy(expr->uexpr.exprinfix.right);
            }
            break;
        case MC_EXPR_FUNCTIONLITERAL:
            {
                mcastliteralfunction_t* fn;
                fn = &expr->uexpr.exprlitfunction;
                mc_allocator_free(expr->pstate, fn->name);
                mc_ptrlist_destroy(fn->funcparamlist, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                mc_astcodeblock_destroy(fn->body);
            }
            break;
        case MC_EXPR_CALL:
            {
                mc_ptrlist_destroy(expr->uexpr.exprcall.args, (mcitemdestroyfn_t)mc_astexpr_destroy);
                mc_astexpr_destroy(expr->uexpr.exprcall.function);
            }
            break;
        case MC_EXPR_INDEX:
            {
                mc_astexpr_destroy(expr->uexpr.exprindex.left);
                mc_astexpr_destroy(expr->uexpr.exprindex.index);
            }
            break;
        case MC_EXPR_ASSIGN:
            {
                mc_astexpr_destroy(expr->uexpr.exprassign.dest);
                mc_astexpr_destroy(expr->uexpr.exprassign.source);
            }
            break;
        case MC_EXPR_LOGICAL:
            {
                mc_astexpr_destroy(expr->uexpr.exprlogical.left);
                mc_astexpr_destroy(expr->uexpr.exprlogical.right);
            }
            break;
        case MC_EXPR_TERNARY:
            {
                mc_astexpr_destroy(expr->uexpr.exprternary.tercond);
                mc_astexpr_destroy(expr->uexpr.exprternary.teriftrue);
                mc_astexpr_destroy(expr->uexpr.exprternary.teriffalse);
            }
            break;
        case MC_EXPR_STMTDEFINE:
            {
                mc_astident_destroy(expr->uexpr.exprdefine.name);
                mc_astexpr_destroy(expr->uexpr.exprdefine.value);
            }
            break;
        case MC_EXPR_STMTIF:
            {
                mc_ptrlist_destroy(expr->uexpr.exprifstmt.cases, (mcitemdestroyfn_t)mc_astifcase_destroy);
                mc_astcodeblock_destroy(expr->uexpr.exprifstmt.alternative);
            }
            break;
        case MC_EXPR_STMTRETURN:
            {
                mc_astexpr_destroy(expr->uexpr.exprreturnvalue);
            }
            break;
        case MC_EXPR_STMTEXPRESSION:
            {
                mc_astexpr_destroy(expr->uexpr.exprexpression);
            }
            break;
        case MC_EXPR_STMTLOOPWHILE:
            {
                mc_astexpr_destroy(expr->uexpr.exprwhileloopstmt.loopcond);
                mc_astcodeblock_destroy(expr->uexpr.exprwhileloopstmt.body);
            }
            break;
        case MC_EXPR_STMTBREAK:
            {
            }
            break;
        case MC_EXPR_STMTCONTINUE:
            {
            }
            break;
        case MC_EXPR_STMTLOOPFOREACH:
            {
                mc_astident_destroy(expr->uexpr.exprforeachloopstmt.iterator);
                mc_astexpr_destroy(expr->uexpr.exprforeachloopstmt.source);
                mc_astcodeblock_destroy(expr->uexpr.exprforeachloopstmt.body);
            }
            break;
        case MC_EXPR_STMTLOOPFORCLASSIC:
            {
                mc_astexpr_destroy(expr->uexpr.exprforloopstmt.init);
                mc_astexpr_destroy(expr->uexpr.exprforloopstmt.loopcond);
                mc_astexpr_destroy(expr->uexpr.exprforloopstmt.update);
                mc_astcodeblock_destroy(expr->uexpr.exprforloopstmt.body);
            }
            break;
        case MC_EXPR_STMTBLOCK:
            {
                mc_astcodeblock_destroy(expr->uexpr.exprblockstmt);
            }
            break;
        case MC_EXPR_STMTIMPORT:
            {
                mc_allocator_free(expr->pstate, expr->uexpr.exprimportstmt.path);
            }
            break;
        case MC_EXPR_STMTRECOVER:
            {
                mc_astcodeblock_destroy(expr->uexpr.exprrecoverstmt.body);
                mc_astident_destroy(expr->uexpr.exprrecoverstmt.errident);
            }
            break;
        default:
            {
            }
            break;
    }
    mc_allocator_free(expr->pstate, expr);
}

mcastexpression_t* mc_astexpr_copy(mcastexpression_t* expr)
{
    mcastexpression_t* res;
    if(!expr)
    {
        return NULL;
    }
    res = NULL;
    switch(expr->type)
    {
        case MC_EXPR_NONE:
            {
                MC_ASSERT(false);
            }
            break;
        case MC_EXPR_IDENT:
            {
                mcastident_t* ident;
                ident = mc_astident_copy(expr->uexpr.exprident);
                if(!ident)
                {
                    return NULL;
                }
                res = mc_astexpr_makeident(expr->pstate, ident);
                if(!res)
                {
                    mc_astident_destroy(ident);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_NUMBERLITERAL:
            {
                res = mc_astexpr_makeliteralnumber(expr->pstate, expr->uexpr.exprlitnumber);
            }
            break;
        case MC_EXPR_BOOLLITERAL:
            {
                res = mc_astexpr_makeliteralbool(expr->pstate, expr->uexpr.exprlitbool);
            }
            break;
        case MC_EXPR_STRINGLITERAL:
            {
                char* stringcopy;
                stringcopy = mc_util_strndup(expr->pstate, expr->uexpr.exprlitstring.data, expr->uexpr.exprlitstring.length);
                if(!stringcopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makeliteralstring(expr->pstate, stringcopy, expr->uexpr.exprlitstring.length);
                if(!res)
                {
                    mc_allocator_free(expr->pstate, stringcopy);
                    return NULL;
                }
            }
            break;

        case MC_EXPR_NULLLITERAL:
            {
                res = mc_astexpr_makeliteralnull(expr->pstate);
            }
            break;
        case MC_EXPR_ARRAYLITERAL:
            {
                mcptrlist_t* valuescopy;
                valuescopy = mc_ptrlist_copy(expr->uexpr.exprlitarray.litarritems, (mcitemcopyfn_t)mc_astexpr_copy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                if(!valuescopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makeliteralarray(expr->pstate, valuescopy);
                if(!res)
                {
                    mc_ptrlist_destroy(valuescopy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    return NULL;
                }
            }
            break;

        case MC_EXPR_MAPLITERAL:
            {
                mcptrlist_t* keyscopy;
                mcptrlist_t* valuescopy;
                keyscopy = mc_ptrlist_copy(expr->uexpr.exprlitmap.keys, (mcitemcopyfn_t)mc_astexpr_copy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                valuescopy = mc_ptrlist_copy(expr->uexpr.exprlitmap.values, (mcitemcopyfn_t)mc_astexpr_copy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                if(!keyscopy || !valuescopy)
                {
                    mc_ptrlist_destroy(keyscopy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    mc_ptrlist_destroy(valuescopy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    return NULL;
                }
                res = mc_astexpr_makeliteralmap(expr->pstate, keyscopy, valuescopy);
                if(!res)
                {
                    mc_ptrlist_destroy(keyscopy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    mc_ptrlist_destroy(valuescopy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_PREFIX:
            {
                mcastexpression_t* rightcopy;
                rightcopy = mc_astexpr_copy(expr->uexpr.exprprefix.right);
                if(!rightcopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makeprefixexpr(expr->pstate, expr->uexpr.exprprefix.op, rightcopy);
                if(!res)
                {
                    mc_astexpr_destroy(rightcopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_INFIX:
            {
                mcastexpression_t* leftcopy;
                mcastexpression_t* rightcopy;
                leftcopy = mc_astexpr_copy(expr->uexpr.exprinfix.left);
                rightcopy = mc_astexpr_copy(expr->uexpr.exprinfix.right);
                if(!leftcopy || !rightcopy)
                {
                    mc_astexpr_destroy(leftcopy);
                    mc_astexpr_destroy(rightcopy);
                    return NULL;
                }
                res = mc_astexpr_makeinfixexpr(expr->pstate, expr->uexpr.exprinfix.op, leftcopy, rightcopy);
                if(!res)
                {
                    mc_astexpr_destroy(leftcopy);
                    mc_astexpr_destroy(rightcopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_FUNCTIONLITERAL:
            {
                char* namecopy;
                mcptrlist_t* pacopy;
                mcastcodeblock_t* bodycopy;
                pacopy = mc_ptrlist_copy(expr->uexpr.exprlitfunction.funcparamlist, (mcitemcopyfn_t)mc_astfuncparam_copy, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                bodycopy = mc_astcodeblock_copy(expr->uexpr.exprlitfunction.body);
                namecopy = mc_util_strdup(expr->pstate, expr->uexpr.exprlitfunction.name);
                if(!pacopy || !bodycopy)
                {
                    mc_ptrlist_destroy(pacopy, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                    mc_astcodeblock_destroy(bodycopy);
                    mc_allocator_free(expr->pstate, namecopy);
                    return NULL;
                }
                res = mc_astexpr_makeliteralfunction(expr->pstate, pacopy, bodycopy);
                if(!res)
                {
                    mc_ptrlist_destroy(pacopy, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                    mc_astcodeblock_destroy(bodycopy);
                    mc_allocator_free(expr->pstate, namecopy);
                    return NULL;
                }
                res->uexpr.exprlitfunction.name = namecopy;
            }
            break;
        case MC_EXPR_CALL:
            {
                mcastexpression_t* fcopy;
                mcptrlist_t* argscopy;
                fcopy = mc_astexpr_copy(expr->uexpr.exprcall.function);
                argscopy = mc_ptrlist_copy(expr->uexpr.exprcall.args, (mcitemcopyfn_t)mc_astexpr_copy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                if(!fcopy || !argscopy)
                {
                    mc_astexpr_destroy(fcopy);
                    mc_ptrlist_destroy(expr->uexpr.exprcall.args, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    return NULL;
                }
                res = mc_astexpr_makecallexpr(expr->pstate, fcopy, argscopy);
                if(!res)
                {
                    mc_astexpr_destroy(fcopy);
                    mc_ptrlist_destroy(expr->uexpr.exprcall.args, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_INDEX:
            {
                mcastexpression_t* leftcopy;
                mcastexpression_t* indexcopy;
                leftcopy = mc_astexpr_copy(expr->uexpr.exprindex.left);
                indexcopy = mc_astexpr_copy(expr->uexpr.exprindex.index);
                if(!leftcopy || !indexcopy)
                {
                    mc_astexpr_destroy(leftcopy);
                    mc_astexpr_destroy(indexcopy);
                    return NULL;
                }
                res = mc_astexpr_makeindexexpr(expr->pstate, leftcopy, indexcopy, false);
                if(!res)
                {
                    mc_astexpr_destroy(leftcopy);
                    mc_astexpr_destroy(indexcopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_ASSIGN:
            {
                mcastexpression_t* destcopy;
                mcastexpression_t* sourcecopy;
                destcopy = mc_astexpr_copy(expr->uexpr.exprassign.dest);
                sourcecopy = mc_astexpr_copy(expr->uexpr.exprassign.source);
                if(!destcopy || !sourcecopy)
                {
                    mc_astexpr_destroy(destcopy);
                    mc_astexpr_destroy(sourcecopy);
                    return NULL;
                }
                res = mc_astexpr_makeassignexpr(expr->pstate, destcopy, sourcecopy, expr->uexpr.exprassign.is_postfix);
                if(!res)
                {
                    mc_astexpr_destroy(destcopy);
                    mc_astexpr_destroy(sourcecopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_LOGICAL:
            {
                mcastexpression_t* leftcopy;
                mcastexpression_t* rightcopy;
                leftcopy = mc_astexpr_copy(expr->uexpr.exprlogical.left);
                rightcopy = mc_astexpr_copy(expr->uexpr.exprlogical.right);
                if(!leftcopy || !rightcopy)
                {
                    mc_astexpr_destroy(leftcopy);
                    mc_astexpr_destroy(rightcopy);
                    return NULL;
                }
                res = mc_astexpr_makelogicalexpr(expr->pstate, expr->uexpr.exprlogical.op, leftcopy, rightcopy);
                if(!res)
                {
                    mc_astexpr_destroy(leftcopy);
                    mc_astexpr_destroy(rightcopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_TERNARY:
            {
                mcastexpression_t* testcopy;
                mcastexpression_t* iftruecopy;
                mcastexpression_t* iffalsecopy;
                testcopy = mc_astexpr_copy(expr->uexpr.exprternary.tercond);
                iftruecopy = mc_astexpr_copy(expr->uexpr.exprternary.teriftrue);
                iffalsecopy = mc_astexpr_copy(expr->uexpr.exprternary.teriffalse);
                if(!testcopy || !iftruecopy || !iffalsecopy)
                {
                    mc_astexpr_destroy(testcopy);
                    mc_astexpr_destroy(iftruecopy);
                    mc_astexpr_destroy(iffalsecopy);
                    return NULL;
                }
                res = mc_astexpr_maketernaryexpr(expr->pstate, testcopy, iftruecopy, iffalsecopy);
                if(!res)
                {
                    mc_astexpr_destroy(testcopy);
                    mc_astexpr_destroy(iftruecopy);
                    mc_astexpr_destroy(iffalsecopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTDEFINE:
            {
                mcastexpression_t* valuecopy;
                valuecopy = mc_astexpr_copy(expr->uexpr.exprdefine.value);
                if(!valuecopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makedefineexpr(expr->pstate, mc_astident_copy(expr->uexpr.exprdefine.name), valuecopy, expr->uexpr.exprdefine.assignable);
                if(!res)
                {
                    mc_astexpr_destroy(valuecopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTIF:
            {
                mcptrlist_t* casescopy;
                mcastcodeblock_t* alternativecopy;
                casescopy = mc_ptrlist_copy(expr->uexpr.exprifstmt.cases, (mcitemcopyfn_t)mc_astifcase_copy, (mcitemdestroyfn_t)mc_astifcase_destroy);
                alternativecopy = mc_astcodeblock_copy(expr->uexpr.exprifstmt.alternative);
                if(!casescopy || !alternativecopy)
                {
                    mc_ptrlist_destroy(casescopy, (mcitemdestroyfn_t)mc_astifcase_destroy);
                    mc_astcodeblock_destroy(alternativecopy);
                    return NULL;
                }
                res = mc_astexpr_makeifexpr(expr->pstate, casescopy, alternativecopy);
                if(res)
                {
                    mc_ptrlist_destroy(casescopy, (mcitemdestroyfn_t)mc_astifcase_destroy);
                    mc_astcodeblock_destroy(alternativecopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTRETURN:
            {
                mcastexpression_t* valuecopy;
                valuecopy = mc_astexpr_copy(expr->uexpr.exprreturnvalue);
                if(!valuecopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makereturnexpr(expr->pstate, valuecopy);
                if(!res)
                {
                    mc_astexpr_destroy(valuecopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTEXPRESSION:
            {
                mcastexpression_t* valuecopy;
                valuecopy = mc_astexpr_copy(expr->uexpr.exprexpression);
                if(!valuecopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makeexprstmt(expr->pstate, valuecopy);
                if(!res)
                {
                    mc_astexpr_destroy(valuecopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTLOOPWHILE:
            {
                mcastexpression_t* testcopy;
                mcastcodeblock_t* bodycopy;
                testcopy = mc_astexpr_copy(expr->uexpr.exprwhileloopstmt.loopcond);
                bodycopy = mc_astcodeblock_copy(expr->uexpr.exprwhileloopstmt.body);
                if(!testcopy || !bodycopy)
                {
                    mc_astexpr_destroy(testcopy);
                    mc_astcodeblock_destroy(bodycopy);
                    return NULL;
                }
                res = mc_astexpr_makewhileexpr(expr->pstate, testcopy, bodycopy);
                if(!res)
                {
                    mc_astexpr_destroy(testcopy);
                    mc_astcodeblock_destroy(bodycopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTBREAK:
            {
                res = mc_astexpr_makebreakexpr(expr->pstate);
            }
            break;
        case MC_EXPR_STMTCONTINUE:
            {
                res = mc_astexpr_makecontinueexpr(expr->pstate);
            }
            break;
        case MC_EXPR_STMTLOOPFOREACH:
            {
                mcastexpression_t* sourcecopy;
                mcastcodeblock_t* bodycopy;
                sourcecopy = mc_astexpr_copy(expr->uexpr.exprforeachloopstmt.source);
                bodycopy = mc_astcodeblock_copy(expr->uexpr.exprforeachloopstmt.body);
                if(!sourcecopy || !bodycopy)
                {
                    mc_astexpr_destroy(sourcecopy);
                    mc_astcodeblock_destroy(bodycopy);
                    return NULL;
                }
                res = mc_astexpr_makeforeachexpr(expr->pstate, mc_astident_copy(expr->uexpr.exprforeachloopstmt.iterator), sourcecopy, bodycopy);
                if(!res)
                {
                    mc_astexpr_destroy(sourcecopy);
                    mc_astcodeblock_destroy(bodycopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTLOOPFORCLASSIC:
            {
                mcastexpression_t* initcopy;
                mcastexpression_t* testcopy;
                mcastexpression_t* updatecopy;
                mcastcodeblock_t* bodycopy;
                initcopy= mc_astexpr_copy(expr->uexpr.exprforloopstmt.init);
                testcopy = mc_astexpr_copy(expr->uexpr.exprforloopstmt.loopcond);
                updatecopy = mc_astexpr_copy(expr->uexpr.exprforloopstmt.update);
                bodycopy = mc_astcodeblock_copy(expr->uexpr.exprforloopstmt.body);
                if(!initcopy || !testcopy || !updatecopy || !bodycopy)
                {
                    mc_astexpr_destroy(initcopy);
                    mc_astexpr_destroy(testcopy);
                    mc_astexpr_destroy(updatecopy);
                    mc_astcodeblock_destroy(bodycopy);
                    return NULL;
                }
                res = mc_astexpr_makeforloopexpr(expr->pstate, initcopy, testcopy, updatecopy, bodycopy);
                if(!res)
                {
                    mc_astexpr_destroy(initcopy);
                    mc_astexpr_destroy(testcopy);
                    mc_astexpr_destroy(updatecopy);
                    mc_astcodeblock_destroy(bodycopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTBLOCK:
            {
                mcastcodeblock_t* blockcopy;
                blockcopy = mc_astcodeblock_copy(expr->uexpr.exprblockstmt);
                if(!blockcopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makeblockexpr(expr->pstate, blockcopy);
                if(!res)
                {
                    mc_astcodeblock_destroy(blockcopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTIMPORT:
            {
                char* pathcopy;
                pathcopy = mc_util_strdup(expr->pstate, expr->uexpr.exprimportstmt.path);
                if(!pathcopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makeimportexpr(expr->pstate, pathcopy);
                if(!res)
                {
                    mc_allocator_free(expr->pstate, pathcopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTRECOVER:
            {
                mcastcodeblock_t* bodycopy;
                mcastident_t* erroridentcopy;
                bodycopy = mc_astcodeblock_copy(expr->uexpr.exprrecoverstmt.body);
                erroridentcopy = mc_astident_copy(expr->uexpr.exprrecoverstmt.errident);
                if(!bodycopy || !erroridentcopy)
                {
                    mc_astcodeblock_destroy(bodycopy);
                    mc_astident_destroy(erroridentcopy);
                    return NULL;
                }
                res = mc_astexpr_makerecoverexpr(expr->pstate, erroridentcopy, bodycopy);
                if(!res)
                {
                    mc_astcodeblock_destroy(bodycopy);
                    mc_astident_destroy(erroridentcopy);
                    return NULL;
                }
            }
            break;
        default:
            {
            }
            break;
    }
    if(!res)
    {
        return NULL;
    }
    res->pos = expr->pos;
    return res;
}

mcastexpression_t* mc_astexpr_makedefineexpr(mcstate_t* state, mcastident_t* name, mcastexpression_t* value, bool assignable)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTDEFINE);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprdefine.name = name;
    res->uexpr.exprdefine.value = value;
    res->uexpr.exprdefine.assignable = assignable;
    return res;
}

mcastexpression_t* mc_astexpr_makeifexpr(mcstate_t* state, mcptrlist_t* cases, mcastcodeblock_t* alternative)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTIF);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprifstmt.cases = cases;
    res->uexpr.exprifstmt.alternative = alternative;
    return res;
}

mcastexpression_t* mc_astexpr_makereturnexpr(mcstate_t* state, mcastexpression_t* value)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTRETURN);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprreturnvalue = value;
    return res;
}

mcastexpression_t* mc_astexpr_makeexprstmt(mcstate_t* state, mcastexpression_t* value)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTEXPRESSION);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprexpression = value;
    return res;
}

mcastexpression_t* mc_astexpr_makewhileexpr(mcstate_t* state, mcastexpression_t* test, mcastcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTLOOPWHILE);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprwhileloopstmt.loopcond = test;
    res->uexpr.exprwhileloopstmt.body = body;
    return res;
}

mcastexpression_t* mc_astexpr_makebreakexpr(mcstate_t* state)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTBREAK);
    if(!res)
    {
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_astexpr_makeforeachexpr(mcstate_t* state, mcastident_t* iterator, mcastexpression_t* source, mcastcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTLOOPFOREACH);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprforeachloopstmt.iterator = iterator;
    res->uexpr.exprforeachloopstmt.source = source;
    res->uexpr.exprforeachloopstmt.body = body;
    return res;
}

mcastexpression_t* mc_astexpr_makeforloopexpr(mcstate_t* state, mcastexpression_t* init, mcastexpression_t* test, mcastexpression_t* update, mcastcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTLOOPFORCLASSIC);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprforloopstmt.init = init;
    res->uexpr.exprforloopstmt.loopcond = test;
    res->uexpr.exprforloopstmt.update = update;
    res->uexpr.exprforloopstmt.body = body;
    return res;
}

mcastexpression_t* mc_astexpr_makecontinueexpr(mcstate_t* state)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTCONTINUE);
    if(!res)
    {
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_astexpr_makeblockexpr(mcstate_t* state, mcastcodeblock_t* block)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTBLOCK);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprblockstmt = block;
    return res;
}

mcastexpression_t* mc_astexpr_makeimportexpr(mcstate_t* state, char* path)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTIMPORT);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprimportstmt.path = path;
    return res;
}

mcastexpression_t* mc_astexpr_makerecoverexpr(mcstate_t* state, mcastident_t* eid, mcastcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTRECOVER);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprrecoverstmt.errident = eid;
    res->uexpr.exprrecoverstmt.body = body;
    return res;
}

mcastcodeblock_t* mc_astcodeblock_make(mcstate_t* state, mcptrlist_t* statements)
{
    mcastcodeblock_t* block;
    block = (mcastcodeblock_t*)mc_allocator_malloc(state, sizeof(mcastcodeblock_t));
    if(!block)
    {
        return NULL;
    }
    block->pstate = state;
    block->statements = statements;
    return block;
}

void mc_astcodeblock_destroy(mcastcodeblock_t* block)
{
    if(!block)
    {
        return;
    }
    mc_ptrlist_destroy(block->statements, (mcitemdestroyfn_t)mc_astexpr_destroy);
    mc_allocator_free(block->pstate, block);
}

mcastcodeblock_t* mc_astcodeblock_copy(mcastcodeblock_t* block)
{
    mcastcodeblock_t* res;
    mcptrlist_t* statementscopy;
    if(!block)
    {
        return NULL;
    }
    statementscopy = mc_ptrlist_copy(block->statements, (mcitemcopyfn_t)mc_astexpr_copy, (mcitemdestroyfn_t)mc_astexpr_destroy);
    if(!statementscopy)
    {
        return NULL;
    }
    res = mc_astcodeblock_make(block->pstate, statementscopy);
    if(!res)
    {
        mc_ptrlist_destroy(statementscopy, (mcitemdestroyfn_t)mc_astexpr_destroy);
        return NULL;
    }
    return res;
}

mcastfuncparam_t* mc_astfuncparam_make(mcstate_t* state, mcastident_t* ident)
{
    mcastfuncparam_t* res;
    res = (mcastfuncparam_t*)mc_allocator_malloc(state, sizeof(mcastfuncparam_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->ident = ident;
    if(!res->ident->value)
    {
        mc_allocator_free(state, res);
        return NULL;
    }
    return res;
}

mcastfuncparam_t* mc_astfuncparam_copy(mcastfuncparam_t* param)
{
    mcastfuncparam_t* res;
    res = (mcastfuncparam_t*)mc_allocator_malloc(param->pstate, sizeof(mcastfuncparam_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = param->pstate;
    res->ident = mc_astident_copy(param->ident);
    if(!res->ident->value)
    {
        mc_allocator_free(param->pstate, res);
        return NULL;
    }
    return res;
}

void mc_astfuncparam_destroy(mcastfuncparam_t* param)
{
    if(!param)
    {
        return;
    }
    mc_astident_destroy(param->ident);
    mc_allocator_free(param->pstate, param);
}

mcastident_t* mc_astident_make(mcstate_t* state, mcasttoken_t tok)
{
    mcastident_t* res = (mcastident_t*)mc_allocator_malloc(state, sizeof(mcastident_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->value = mc_asttoken_dupliteralstring(state, &tok);
    if(!res->value)
    {
        mc_allocator_free(state, res);
        return NULL;
    }
    res->pos = tok.pos;
    return res;
}

mcastident_t* mc_astident_copy(mcastident_t* ident)
{
    mcastident_t* res = (mcastident_t*)mc_allocator_malloc(ident->pstate, sizeof(mcastident_t));
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

void mc_astident_destroy(mcastident_t* ident)
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

mcastifcase_t* mc_astifcase_make(mcstate_t* state, mcastexpression_t* test, mcastcodeblock_t* consequence)
{
    mcastifcase_t* res;
    res = (mcastifcase_t*)mc_allocator_malloc(state, sizeof(mcastifcase_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->ifcond = test;
    res->consequence = consequence;
    return res;
}

void mc_astifcase_destroy(mcastifcase_t* cond)
{
    if(!cond)
    {
        return;
    }
    mc_astexpr_destroy(cond->ifcond);
    mc_astcodeblock_destroy(cond->consequence);
    mc_allocator_free(cond->pstate, cond);
}

mcastifcase_t* mc_astifcase_copy(mcastifcase_t* ifcase)
{
    mcastexpression_t* testcopy;
    mcastcodeblock_t* consequencecopy;
    mcastifcase_t* ifcasecopy;
    if(!ifcase)
    {
        return NULL;
    }
    testcopy = NULL;
    consequencecopy = NULL;
    ifcasecopy = NULL;
    testcopy = mc_astexpr_copy(ifcase->ifcond);
    if(!testcopy)
    {
        goto err;
    }
    consequencecopy = mc_astcodeblock_copy(ifcase->consequence);
    if(!testcopy || !consequencecopy)
    {
        goto err;
    }
    ifcasecopy = mc_astifcase_make(ifcase->pstate, testcopy, consequencecopy);
    if(!ifcasecopy)
    {
        goto err;
    }
    return ifcasecopy;
err:
    mc_astexpr_destroy(testcopy);
    mc_astcodeblock_destroy(consequencecopy);
    mc_astifcase_destroy(ifcasecopy);
    return NULL;
}

mcastexpression_t* mc_astexpr_makeexpression(mcstate_t* state, mcastexprtype_t type)
{
    mcastexpression_t* res = (mcastexpression_t*)mc_allocator_malloc(state, sizeof(mcastexpression_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->type = type;
    res->pos = srcposinvalid;
    return res;
}

mcastparser_t* mc_astparser_make(mcstate_t* state, mcconfig_t* config, mcerrlist_t* errors)
{
    mcastparser_t* parser;
    parser = (mcastparser_t*)mc_allocator_malloc(state, sizeof(mcastparser_t));
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
        parser->rightassocfuncs[MC_TOK_UNARYMINUS] = mc_parser_parseprefixexpr;
        parser->rightassocfuncs[MC_TOK_UNARYBINNOT] = mc_parser_parseprefixexpr;
        parser->rightassocfuncs[MC_TOK_LPAREN] = mc_parser_parsegroupedexpr;
        parser->rightassocfuncs[MC_TOK_FUNCTION] = mc_parser_parseliteralfunction;
        parser->rightassocfuncs[MC_TOK_LBRACKET] = mc_parser_parseliteralarray;
        parser->rightassocfuncs[MC_TOK_LBRACE] = mc_parser_parseliteralmap;
        parser->rightassocfuncs[MC_TOK_PLUSPLUS] = mc_parser_parseincdecprefixexpr;
        parser->rightassocfuncs[MC_TOK_MINUSMINUS] = mc_parser_parseincdecprefixexpr;
        #if 0
        parser->rightassocfuncs[MC_TOK_IF] = mc_parser_parseifstmt;
        #endif
    }
    {
        parser->leftassocfuncs[MC_TOK_PLUS] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_UNARYMINUS] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_SLASH] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_ASTERISK] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_PERCENT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_EQ] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_NOTEQ] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_LT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_LTE] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_GT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_GTE] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_LPAREN] = mc_parser_parsecallexpr;
        parser->leftassocfuncs[MC_TOK_LBRACKET] = mc_parser_parseindexexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGN] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNPLUS] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNMINUS] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNSLASH] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNASTERISK] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNPERCENT] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNBINAND] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNBINOR] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNBINXOR] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNLSHIFT] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNRSHIFT] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_DOT] = mc_parser_parsedotexpression;
        parser->leftassocfuncs[MC_TOK_AND] = mc_parser_parselogicalexpr;
        parser->leftassocfuncs[MC_TOK_OR] = mc_parser_parselogicalexpr;
        parser->leftassocfuncs[MC_TOK_BINAND] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_BINOR] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_BINXOR] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_LSHIFT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_RSHIFT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_QUESTION] = mc_parser_parseternaryexpr;
        parser->leftassocfuncs[MC_TOK_PLUSPLUS] = mc_parser_parseincdecpostfixexpr;
        parser->leftassocfuncs[MC_TOK_MINUSMINUS] = mc_parser_parseincdecpostfixexpr;
    }
    parser->depth = 0;
    return parser;
}

void mc_astparser_destroy(mcastparser_t* parser)
{
    if(!parser)
    {
        return;
    }
    mc_allocator_free(parser->pstate, parser);
}

mcptrlist_t* mc_astparser_parseall(mcastparser_t* parser, const char* input, mcastcompiledfile_t* file)
{
    bool ok;
    mcastexpression_t* expr;
    mcptrlist_t* statements;
    parser->depth = 0;
    ok = mc_lexer_init(&parser->lexer, parser->pstate, parser->errors, input, file);
    if(!ok)
    {
        return NULL;
    }
    mc_lexer_nexttoken(&parser->lexer);
    mc_lexer_nexttoken(&parser->lexer);
    statements = mc_ptrlist_make(parser->pstate, 0);
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
        expr = mc_astparser_parsestatement(parser);
        if(!expr)
        {
            goto err;
        }
        ok = mc_ptrlist_push(statements, expr);
        if(!ok)
        {
            mc_astexpr_destroy(expr);
            goto err;
        }
    }
    if(parser->errors->count > 0)
    {
        goto err;
    }
    return statements;
err:
    mc_ptrlist_destroy(statements, (mcitemdestroyfn_t)mc_astexpr_destroy);
    return NULL;
}

mcastexpression_t* mc_astparser_parsestatement(mcastparser_t* p)
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
                res = mc_parser_parsevarletstmt(p);
            }
            break;
        case MC_TOK_IF:
            {
                res = mc_parser_parseifstmt(p);
            }
            break;
        case MC_TOK_RETURN:
            {
                res = mc_parser_parsereturnstmt(p);
            }
            break;
        case MC_TOK_WHILE:
            {
                res = mc_parser_parseloopwhilestmt(p);
            }
            break;
        case MC_TOK_BREAK:
            {
                res = mc_parser_parsebreakstmt(p);
            }
            break;
        case MC_TOK_FOR:
            {
                res = mc_parser_parseloopforstmt(p);
            }
            break;
        case MC_TOK_FUNCTION:
            {
                if(mc_lexer_peektokenis(&p->lexer, MC_TOK_IDENT))
                {
                    res = mc_parser_parsefunctionstmt(p);
                }
                else
                {
                    res = mc_parser_parseexprstmt(p);
                }
            }
            break;
        case MC_TOK_LBRACE:
            {
                if(p->config->replmode && p->depth == 0)
                {
                    res = mc_parser_parseexprstmt(p);
                }
                else
                {
                    res = mc_parser_parseblockstmt(p);
                }
            }
            break;
        case MC_TOK_CONTINUE:
            {
                res = mc_parser_parsecontinuestmt(p);
            }
            break;
        case MC_TOK_IMPORT:
            {
                res = mc_parser_parseimportstmt(p);
            }
            break;
        case MC_TOK_RECOVER:
            {
                res = mc_parser_parserecoverstmt(p);
            }
            break;
        default:
            {
                res = mc_parser_parseexprstmt(p);
            }
            break;
    }
    if(res)
    {
        res->pos = pos;
    }
    return res;
}

mcastexpression_t* mc_parser_parsevarletstmt(mcastparser_t* p)
{
    bool assignable;
    mcastident_t* nameident;
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
    nameident = mc_astident_make(p->pstate, p->lexer.currtoken);
    if(!nameident)
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    #if 0
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_ASSIGN))
    #else
        if(!mc_lexer_currtokenis(&p->lexer, MC_TOK_ASSIGN))
    #endif
    {
        value = mc_astexpr_makeliteralnull(p->pstate);
        goto finish;
    }
    mc_lexer_nexttoken(&p->lexer);
    value = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!value)
    {
        goto err;
    }
    if(value->type == MC_EXPR_FUNCTIONLITERAL)
    {
        value->uexpr.exprlitfunction.name = mc_util_strdup(p->pstate, nameident->value);
        if(!value->uexpr.exprlitfunction.name)
        {
            goto err;
        }
    }
    finish:
    res = mc_astexpr_makedefineexpr(p->pstate, nameident, value, assignable);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astexpr_destroy(value);
    mc_astident_destroy(nameident);
    return NULL;
}

mcastexpression_t* mc_parser_parseifstmt(mcastparser_t* p)
{
    bool ok;
    mcptrlist_t* cases;
    mcastifcase_t* cond;
    mcastifcase_t* elif;
    mcastcodeblock_t* alternative;
    mcastexpression_t* res;
    cases = NULL;
    alternative = NULL;
    cases = mc_ptrlist_make(p->pstate, 0);
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
    cond = mc_astifcase_make(p->pstate, NULL, NULL);
    if(!cond)
    {
        goto err;
    }
    ok = mc_ptrlist_push(cases, cond);
    if(!ok)
    {
        mc_astifcase_destroy(cond);
        goto err;
    }
    cond->ifcond = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!cond->ifcond)
    {
        goto err;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    cond->consequence = mc_parser_parsecodeblock(p);
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
            elif = mc_astifcase_make(p->pstate, NULL, NULL);
            if(!elif)
            {
                goto err;
            }
            ok = mc_ptrlist_push(cases, elif);
            if(!ok)
            {
                mc_astifcase_destroy(elif);
                goto err;
            }
            elif->ifcond = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!elif->ifcond)
            {
                goto err;
            }
            if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
            {
                goto err;
            }
            mc_lexer_nexttoken(&p->lexer);
            elif->consequence = mc_parser_parsecodeblock(p);
            if(!elif->consequence)
            {
                goto err;
            }
        }
        else
        {
            alternative = mc_parser_parsecodeblock(p);
            if(!alternative)
            {
                goto err;
            }
        }
    }
    res = mc_astexpr_makeifexpr(p->pstate, cases, alternative);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_ptrlist_destroy(cases, (mcitemdestroyfn_t)mc_astifcase_destroy);
    mc_astcodeblock_destroy(alternative);
    return NULL;
}

mcastexpression_t* mc_parser_parsereturnstmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* expr;
    expr = NULL;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_currtokenis(&p->lexer, MC_TOK_SEMICOLON) && !mc_lexer_currtokenis(&p->lexer, MC_TOK_RBRACE) && !mc_lexer_currtokenis(&p->lexer, MC_TOK_EOF))
    {
        expr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
        if(!expr)
        {
            return NULL;
        }
    }
    res = mc_astexpr_makereturnexpr(p->pstate, expr);
    if(!res)
    {
        mc_astexpr_destroy(expr);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseexprstmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* expr;
    expr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!expr)
    {
        return NULL;
    }
    if(expr && (!p->config->replmode || p->depth > 0))
    {
        #if 0
        /* this is actually completely unnecessary */
        if(expr->type != MC_EXPR_ASSIGN && expr->type != MC_EXPR_CALL)
        {
            mc_errlist_addf(p->errors, MC_ERROR_PARSING, expr->pos, "Only assignments and function calls can be expression statements");
            mc_astexpr_destroy(expr);
            return NULL;
        }
        #endif
    }
    res = mc_astexpr_makeexprstmt(p->pstate, expr);
    if(!res)
    {
        mc_astexpr_destroy(expr);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseloopwhilestmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* test;
    mcastcodeblock_t* body;
    test = NULL;
    body = NULL;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    test = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!test)
    {
        goto err;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    body = mc_parser_parsecodeblock(p);
    if(!body)
    {
        goto err;
    }
    res = mc_astexpr_makewhileexpr(p->pstate, test, body);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astcodeblock_destroy(body);
    mc_astexpr_destroy(test);
    return NULL;
}

mcastexpression_t* mc_parser_parsebreakstmt(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    return mc_astexpr_makebreakexpr(p->pstate);
}

mcastexpression_t* mc_parser_parsecontinuestmt(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    return mc_astexpr_makecontinueexpr(p->pstate);
}

mcastexpression_t* mc_parser_parseblockstmt(mcastparser_t* p)
{
    mcastcodeblock_t* block;
    mcastexpression_t* res;
    block = mc_parser_parsecodeblock(p);
    if(!block)
    {
        return NULL;
    }
    res = mc_astexpr_makeblockexpr(p->pstate, block);
    if(!res)
    {
        mc_astcodeblock_destroy(block);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseimportstmt(mcastparser_t* p)
{
    char* processedname;
    mcastexpression_t* res;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_STRING))
    {
        return NULL;
    }
    processedname = mc_parser_processandcopystring(p->pstate, p->lexer.currtoken.literal, p->lexer.currtoken.len);
    if(!processedname)
    {
        mc_errlist_push(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "Error when parsing module name");
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    res = mc_astexpr_makeimportexpr(p->pstate, processedname);
    if(!res)
    {
        mc_allocator_free(p->pstate, processedname);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parserecoverstmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastident_t* eid;
    mcastcodeblock_t* body;
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
    eid = mc_astident_make(p->pstate, p->lexer.currtoken);
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
    body = mc_parser_parsecodeblock(p);
    if(!body)
    {
        goto err;
    }
    res = mc_astexpr_makerecoverexpr(p->pstate, eid, body);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astcodeblock_destroy(body);
    mc_astident_destroy(eid);
    return NULL;
}

mcastexpression_t* mc_parser_parseloopforstmt(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(mc_lexer_currtokenis(&p->lexer, MC_TOK_IDENT) && mc_lexer_peektokenis(&p->lexer, MC_TOK_IN))
    {
        return mc_parser_parseloopforeachstmt(p);
    }
    return mc_parser_parseloopforclassicstmt(p);
}

mcastexpression_t* mc_parser_parseloopforeachstmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* source;
    mcastcodeblock_t* body;
    mcastident_t* iteratorident;
    source = NULL;
    body = NULL;
    iteratorident = NULL;
    iteratorident = mc_astident_make(p->pstate, p->lexer.currtoken);
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
    source = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!source)
    {
        goto err;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    body = mc_parser_parsecodeblock(p);
    if(!body)
    {
        goto err;
    }
    res = mc_astexpr_makeforeachexpr(p->pstate, iteratorident, source, body);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astcodeblock_destroy(body);
    mc_astident_destroy(iteratorident);
    mc_astexpr_destroy(source);
    return NULL;
}

mcastexpression_t* mc_parser_parseloopforclassicstmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* init;
    mcastexpression_t* test;
    mcastexpression_t* update;
    mcastcodeblock_t* body;
    init = NULL;
    test = NULL;
    update = NULL;
    body = NULL;
    if(!mc_lexer_currtokenis(&p->lexer, MC_TOK_SEMICOLON))
    {
        init = mc_astparser_parsestatement(p);
        if(!init)
        {
            goto err;
        }
        if(init->type != MC_EXPR_STMTDEFINE && init->type != MC_EXPR_STMTEXPRESSION)
        {
            mc_errlist_addf(p->errors, MC_ERROR_PARSING, init->pos, "expected a definition or expression as 'for' loop init clause");
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
        test = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
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
        update = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
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
    body = mc_parser_parsecodeblock(p);
    if(!body)
    {
        goto err;
    }
    res = mc_astexpr_makeforloopexpr(p->pstate, init, test, update, body);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astexpr_destroy(init);
    mc_astexpr_destroy(test);
    mc_astexpr_destroy(update);
    mc_astcodeblock_destroy(body);
    return NULL;
}


mcastcodeblock_t* mc_parser_parsecodeblock(mcastparser_t* p)
{
    bool ok;
    mcastcodeblock_t* res;
    mcastexpression_t* expr;
    mcptrlist_t* statements;
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LBRACE))
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    p->depth++;
    statements = mc_ptrlist_make(p->pstate, 0);
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
        expr = mc_astparser_parsestatement(p);
        if(!expr)
        {
            goto err;
        }
        ok = mc_ptrlist_push(statements, expr);
        if(!ok)
        {
            mc_astexpr_destroy(expr);
            goto err;
        }
    }
    mc_lexer_nexttoken(&p->lexer);
    p->depth--;
    res = mc_astcodeblock_make(p->pstate, statements);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    p->depth--;
    mc_ptrlist_destroy(statements, (mcitemdestroyfn_t)mc_astexpr_destroy);
    return NULL;
}

mcastexpression_t* mc_parser_parseexpression(mcastparser_t* p, mcastprecedence_t prec)
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
        literal = mc_asttoken_dupliteralstring(p->pstate, &p->lexer.currtoken);
        mc_errlist_addf(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "no prefix parse function for \"%s\" found", literal);
        mc_allocator_free(p->pstate, literal);
        return NULL;
    }
    leftexpr = parserightassoc(p);
    if(!leftexpr)
    {
        return NULL;
    }
    leftexpr->pos = pos;
    while(!mc_lexer_currtokenis(&p->lexer, MC_TOK_SEMICOLON) && prec < mc_parser_getprecedence(p->lexer.currtoken.type))
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
            mc_astexpr_destroy(leftexpr);
            return NULL;
        }
        newleftexpr->pos = pos;
        leftexpr = newleftexpr;
    }
    return leftexpr;
}

mcastexpression_t* mc_parser_parseident(mcastparser_t* p)
{
    mcastident_t* ident;
    mcastexpression_t* res;
    ident = mc_astident_make(p->pstate, p->lexer.currtoken);
    if(!ident)
    {
        return NULL;
    }
    res = mc_astexpr_makeident(p->pstate, ident);
    if(!res)
    {
        mc_astident_destroy(ident);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    return res;
}

mcastexpression_t* mc_parser_parseliteralnumber(mcastparser_t* p)
{
    mcfloat_t number;
    long parsedlen;
    char* end;
    char* literal;
    number = 0;
    errno = 0;
    number = mc_util_strtod(p->lexer.currtoken.literal, p->lexer.currtoken.len, &end);
    #if 0
        fprintf(stderr, "literal=<%s> number=<%f>\n", p->lexer.currtoken.literal, number);
    #endif
    parsedlen = end - p->lexer.currtoken.literal;
    if(errno || parsedlen != p->lexer.currtoken.len)
    {
        literal = mc_asttoken_dupliteralstring(p->pstate, &p->lexer.currtoken);
        mc_errlist_addf(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "failed to parse number literal \"%s\"", literal);
        mc_allocator_free(p->pstate, literal);
        return NULL;
    }    
    mc_lexer_nexttoken(&p->lexer);
    return mc_astexpr_makeliteralnumber(p->pstate, number);
}

mcastexpression_t* mc_parser_parseliteralbool(mcastparser_t* p)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeliteralbool(p->pstate, p->lexer.currtoken.type == MC_TOK_TRUE);
    mc_lexer_nexttoken(&p->lexer);
    return res;
}

mcastexpression_t* mc_parser_parseliteralstring(mcastparser_t* p)
{
    size_t len;
    char* processedliteral;
    mcastexpression_t* res;
    processedliteral = mc_parser_processandcopystring(p->pstate, p->lexer.currtoken.literal, p->lexer.currtoken.len);
    if(!processedliteral)
    {
        mc_errlist_push(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "Error when parsing string literal");
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    len = mc_util_strlen(processedliteral);
    res = mc_astexpr_makeliteralstring(p->pstate, processedliteral, len);
    if(!res)
    {
        mc_allocator_free(p->pstate, processedliteral);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseliteraltemplatestring(mcastparser_t* p)
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
    processedliteral = mc_parser_processandcopystring(p->pstate, p->lexer.currtoken.literal, p->lexer.currtoken.len);
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
    len = mc_util_strlen(processedliteral);
    leftstringexpr = mc_astexpr_makeliteralstring(p->pstate, processedliteral, len);
    if(!leftstringexpr)
    {
        goto err;
    }
    leftstringexpr->pos = pos;
    processedliteral = NULL;
    pos = p->lexer.currtoken.pos;
    templateexpr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!templateexpr)
    {
        goto err;
    }
    tostrcallexpr = mc_parser_makefunccallexpr(p->pstate, templateexpr, "tostring");
    if(!tostrcallexpr)
    {
        goto err;
    }
    tostrcallexpr->pos = pos;
    templateexpr = NULL;
    leftaddexpr = mc_astexpr_makeinfixexpr(p->pstate, MC_MATHOP_PLUS, leftstringexpr, tostrcallexpr);
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
    rightexpr = mc_parser_parseexpression(p, MC_ASTPREC_HIGHEST);
    if(!rightexpr)
    {
        goto err;
    }
    rightaddexpr = mc_astexpr_makeinfixexpr(p->pstate, MC_MATHOP_PLUS, leftaddexpr, rightexpr);
    if(!rightaddexpr)
    {
        goto err;
    }
    rightaddexpr->pos = pos;
    leftaddexpr = NULL;
    rightexpr = NULL;
    return rightaddexpr;
err:
    mc_astexpr_destroy(rightaddexpr);
    mc_astexpr_destroy(rightexpr);
    mc_astexpr_destroy(leftaddexpr);
    mc_astexpr_destroy(tostrcallexpr);
    mc_astexpr_destroy(templateexpr);
    mc_astexpr_destroy(leftstringexpr);
    mc_allocator_free(p->pstate, processedliteral);
    return NULL;
}

mcastexpression_t* mc_parser_parseliteralnull(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    return mc_astexpr_makeliteralnull(p->pstate);
}

mcastexpression_t* mc_parser_parseliteralarray(mcastparser_t* p)
{
    mcptrlist_t* array;
    mcastexpression_t* res;
    array = mc_parser_parseexprlist(p, MC_TOK_LBRACKET, MC_TOK_RBRACKET, true);
    if(!array)
    {
        return NULL;
    }
    res = mc_astexpr_makeliteralarray(p->pstate, array);
    if(!res)
    {
        mc_ptrlist_destroy(array, (mcitemdestroyfn_t)mc_astexpr_destroy);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseliteralmap(mcastparser_t* p)
{
    bool ok;
    size_t len;
    char* str;
    mcptrlist_t* keys;
    mcptrlist_t* values;
    mcastexpression_t* res;
    mcastexpression_t* key;
    mcastexpression_t* value;
    keys = mc_ptrlist_make(p->pstate, 0);
    values = mc_ptrlist_make(p->pstate, 0);
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
            str = mc_asttoken_dupliteralstring(p->pstate, &p->lexer.currtoken);
            len = mc_util_strlen(str);
            key = mc_astexpr_makeliteralstring(p->pstate, str, len);
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
            key = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
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
                    mc_errlist_addf(p->errors, MC_ERROR_PARSING, key->pos, "can only use primitive types as literal 'map' object keys");
                    mc_astexpr_destroy(key);
                    goto err;
                }
            }
        }
        ok = mc_ptrlist_push(keys, key);
        if(!ok)
        {
            mc_astexpr_destroy(key);
            goto err;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_COLON))
        {
            goto err;
        }
        mc_lexer_nexttoken(&p->lexer);
        value = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
        if(!value)
        {
            goto err;
        }
        ok = mc_ptrlist_push(values, value);
        if(!ok)
        {
            mc_astexpr_destroy(value);
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
    res = mc_astexpr_makeliteralmap(p->pstate, keys, values);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_ptrlist_destroy(keys, (mcitemdestroyfn_t)mc_astexpr_destroy);
    mc_ptrlist_destroy(values, (mcitemdestroyfn_t)mc_astexpr_destroy);
    return NULL;
}

mcastexpression_t* mc_parser_parseprefixexpr(mcastparser_t* p)
{
    mcastmathoptype_t op;
    mcastexpression_t* res;
    mcastexpression_t* right;
    op = mc_parser_tokentomathop(p->lexer.currtoken.type);
    mc_lexer_nexttoken(&p->lexer);
    right = mc_parser_parseexpression(p, MC_ASTPREC_PREFIX);
    if(!right)
    {
        return NULL;
    }
    res = mc_astexpr_makeprefixexpr(p->pstate, op, right);
    if(!res)
    {
        mc_astexpr_destroy(right);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseinfixexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastmathoptype_t op;
    mcastprecedence_t prec;
    mcastexpression_t* res;
    mcastexpression_t* right;
    op = mc_parser_tokentomathop(p->lexer.currtoken.type);
    prec = mc_parser_getprecedence(p->lexer.currtoken.type);
    mc_lexer_nexttoken(&p->lexer);
    right = mc_parser_parseexpression(p, prec);
    if(!right)
    {
        return NULL;
    }
    res = mc_astexpr_makeinfixexpr(p->pstate, op, left, right);
    if(!res)
    {
        mc_astexpr_destroy(right);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parsegroupedexpr(mcastparser_t* p)
{
    mcastexpression_t* expr;
    mc_lexer_nexttoken(&p->lexer);
    expr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!expr || !mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        mc_astexpr_destroy(expr);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    return expr;
}


bool mc_parser_parsefuncparams(mcastparser_t* p, mcptrlist_t* outparams)
{
    bool ok;
    mcastident_t* ident;
    mcastfuncparam_t* param;
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
    ident = mc_astident_make(p->pstate, p->lexer.currtoken);
    if(!ident)
    {
        return false;
    }
    param = mc_astfuncparam_make(p->pstate, ident);
    ok = mc_ptrlist_push(outparams, param);
    if(!ok)
    {
        mc_astident_destroy(ident);
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
        ident = mc_astident_make(p->pstate, p->lexer.currtoken);
        if(!ident)
        {
            return false;
        }
        param = mc_astfuncparam_make(p->pstate, ident);
        ok = mc_ptrlist_push(outparams, param);
        if(!ok)
        {
            mc_astfuncparam_destroy(param);
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

mcastexpression_t* mc_parser_parseliteralfunction(mcastparser_t* p)
{
    bool ok;
    mcptrlist_t* params;
    mcastcodeblock_t* body;
    mcastexpression_t* res;
    p->depth++;
    params = NULL;
    body = NULL;
    if(mc_lexer_currtokenis(&p->lexer, MC_TOK_FUNCTION))
    {
        mc_lexer_nexttoken(&p->lexer);
    }
    params = mc_ptrlist_make(p->pstate, 0);
    ok = mc_parser_parsefuncparams(p, params);
    if(!ok)
    {
        goto err;
    }
    body = mc_parser_parsecodeblock(p);
    if(!body)
    {
        goto err;
    }
    res = mc_astexpr_makeliteralfunction(p->pstate, params, body);
    if(!res)
    {
        goto err;
    }
    p->depth -= 1;
    return res;
err:
    mc_astcodeblock_destroy(body);
    mc_ptrlist_destroy(params, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
    p->depth -= 1;
    return NULL;
}

mcastexpression_t* mc_parser_parsefunctionstmt(mcastparser_t* p)
{
    mcastident_t* nameident;
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
    nameident = mc_astident_make(p->pstate, p->lexer.currtoken);
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
    value->uexpr.exprlitfunction.name = mc_util_strdup(p->pstate, nameident->value);
    if(!value->uexpr.exprlitfunction.name)
    {
        goto err;
    }
    res = mc_astexpr_makedefineexpr(p->pstate, nameident, value, false);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astexpr_destroy(value);
    mc_astident_destroy(nameident);
    return NULL;
}


mcastexpression_t* mc_parser_parsecallexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcptrlist_t* args;
    mcastexpression_t* res;
    mcastexpression_t* function;
    function = left;
    args = mc_parser_parseexprlist(p, MC_TOK_LPAREN, MC_TOK_RPAREN, false);
    if(!args)
    {
        return NULL;
    }
    res = mc_astexpr_makecallexpr(p->pstate, function, args);
    if(!res)
    {
        mc_ptrlist_destroy(args, (mcitemdestroyfn_t)mc_astexpr_destroy);
        return NULL;
    }
    return res;
}

mcptrlist_t* mc_parser_parseexprlist(mcastparser_t* p, mcasttoktype_t starttoken, mcasttoktype_t endtoken, bool trailingcommaallowed)
{
    bool ok;
    mcptrlist_t* res;
    mcastexpression_t* argexpr;
    if(!mc_lexer_expectcurrent(&p->lexer, starttoken))
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    res = mc_ptrlist_make(p->pstate, 0);
    if(mc_lexer_currtokenis(&p->lexer, endtoken))
    {
        mc_lexer_nexttoken(&p->lexer);
        return res;
    }
    argexpr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!argexpr)
    {
        goto err;
    }
    ok = mc_ptrlist_push(res, argexpr);
    if(!ok)
    {
        mc_astexpr_destroy(argexpr);
        goto err;
    }
    while(mc_lexer_currtokenis(&p->lexer, MC_TOK_COMMA))
    {
        mc_lexer_nexttoken(&p->lexer);
        if(trailingcommaallowed && mc_lexer_currtokenis(&p->lexer, endtoken))
        {
            break;
        }
        argexpr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
        if(!argexpr)
        {
            goto err;
        }
        ok = mc_ptrlist_push(res, argexpr);
        if(!ok)
        {
            mc_astexpr_destroy(argexpr);
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
    mc_ptrlist_destroy(res, (mcitemdestroyfn_t)mc_astexpr_destroy);
    return NULL;
}

mcastexpression_t* mc_parser_parseindexexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastexpression_t* res;
    mcastexpression_t* index;
    mc_lexer_nexttoken(&p->lexer);
    index = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!index)
    {
        return NULL;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RBRACKET))
    {
        mc_astexpr_destroy(index);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    res = mc_astexpr_makeindexexpr(p->pstate, left, index, false);
    if(!res)
    {
        mc_astexpr_destroy(index);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseassignexpr(mcastparser_t* p, mcastexpression_t* left)
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
    source = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!source)
    {
        goto err;
    }
    switch(assigntype)
    {
        case MC_TOK_ASSIGNPLUS:
        case MC_TOK_ASSIGNMINUS:
        case MC_TOK_ASSIGNSLASH:
        case MC_TOK_ASSIGNASTERISK:
        case MC_TOK_ASSIGNPERCENT:
        case MC_TOK_ASSIGNBINAND:
        case MC_TOK_ASSIGNBINOR:
        case MC_TOK_ASSIGNBINXOR:
        case MC_TOK_ASSIGNLSHIFT:
        case MC_TOK_ASSIGNRSHIFT:
            {
                op = mc_parser_tokentomathop(assigntype);
                leftcopy = mc_astexpr_copy(left);
                if(!leftcopy)
                {
                    goto err;
                }
                pos = source->pos;
                newsource = mc_astexpr_makeinfixexpr(p->pstate, op, leftcopy, source);
                if(!newsource)
                {
                    mc_astexpr_destroy(leftcopy);
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
    res = mc_astexpr_makeassignexpr(p->pstate, left, source, false);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astexpr_destroy(source);
    return NULL;
}

mcastexpression_t* mc_parser_parselogicalexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastmathoptype_t op;
    mcastprecedence_t prec;
    mcastexpression_t* res;
    mcastexpression_t* right;
    op = mc_parser_tokentomathop(p->lexer.currtoken.type);
    prec = mc_parser_getprecedence(p->lexer.currtoken.type);
    mc_lexer_nexttoken(&p->lexer);
    right = mc_parser_parseexpression(p, prec);
    if(!right)
    {
        return NULL;
    }
    res = mc_astexpr_makelogicalexpr(p->pstate, op, left, right);
    if(!res)
    {
        mc_astexpr_destroy(right);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseternaryexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastexpression_t* res;
    mcastexpression_t* ift;
    mcastexpression_t* iffalse;
    mc_lexer_nexttoken(&p->lexer);
    ift = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!ift)
    {
        return NULL;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_COLON))
    {
        mc_astexpr_destroy(ift);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    iffalse = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!iffalse)
    {
        mc_astexpr_destroy(ift);
        return NULL;
    }
    res = mc_astexpr_maketernaryexpr(p->pstate, left, ift, iffalse);
    if(!res)
    {
        mc_astexpr_destroy(ift);
        mc_astexpr_destroy(iffalse);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseincdecprefixexpr(mcastparser_t* p)
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
    op = mc_parser_tokentomathop(operationtype);
    dest = mc_parser_parseexpression(p, MC_ASTPREC_PREFIX);
    if(!dest)
    {
        goto err;
    }
    oneliteral = mc_astexpr_makeliteralnumber(p->pstate, 1);
    if(!oneliteral)
    {
        mc_astexpr_destroy(dest);
        goto err;
    }
    oneliteral->pos = pos;
    destcopy = mc_astexpr_copy(dest);
    if(!destcopy)
    {
        mc_astexpr_destroy(oneliteral);
        mc_astexpr_destroy(dest);
        goto err;
    }
    operation = mc_astexpr_makeinfixexpr(p->pstate, op, destcopy, oneliteral);
    if(!operation)
    {
        mc_astexpr_destroy(destcopy);
        mc_astexpr_destroy(dest);
        mc_astexpr_destroy(oneliteral);
        goto err;
    }
    operation->pos = pos;
    res = mc_astexpr_makeassignexpr(p->pstate, dest, operation, false);
    if(!res)
    {
        mc_astexpr_destroy(dest);
        mc_astexpr_destroy(operation);
        goto err;
    }
    return res;
err:
    mc_astexpr_destroy(source);
    return NULL;
}

mcastexpression_t* mc_parser_parseincdecpostfixexpr(mcastparser_t* p, mcastexpression_t* left)
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
    op = mc_parser_tokentomathop(operationtype);
    leftcopy = mc_astexpr_copy(left);
    if(!leftcopy)
    {
        goto err;
    }
    oneliteral = mc_astexpr_makeliteralnumber(p->pstate, 1);
    if(!oneliteral)
    {
        mc_astexpr_destroy(leftcopy);
        goto err;
    }
    oneliteral->pos = pos;
    operation = mc_astexpr_makeinfixexpr(p->pstate, op, leftcopy, oneliteral);
    if(!operation)
    {
        mc_astexpr_destroy(oneliteral);
        mc_astexpr_destroy(leftcopy);
        goto err;
    }
    operation->pos = pos;
    res = mc_astexpr_makeassignexpr(p->pstate, left, operation, true);
    if(!res)
    {
        mc_astexpr_destroy(operation);
        goto err;
    }
    return res;
err:
    mc_astexpr_destroy(source);
    return NULL;
}

mcastexpression_t* mc_parser_parsedotexpression(mcastparser_t* p, mcastexpression_t* left)
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
    str = mc_asttoken_dupliteralstring(p->pstate, &p->lexer.currtoken);
    len = mc_util_strlen(str);
    index = mc_astexpr_makeliteralstring(p->pstate, str, len);
    if(!index)
    {
        mc_allocator_free(p->pstate, str);
        return NULL;
    }
    index->pos = p->lexer.currtoken.pos;
    mc_lexer_nexttoken(&p->lexer);
    res = mc_astexpr_makeindexexpr(p->pstate, left, index, true);
    if(!res)
    {
        mc_astexpr_destroy(index);
        return NULL;
    }
    return res;
}

mcastprecedence_t mc_parser_getprecedence(mcasttoktype_t tk)
{
    switch(tk)
    {
        case MC_TOK_EQ:
        case MC_TOK_NOTEQ:
            return MC_ASTPREC_EQUALS;
        case MC_TOK_LT:
        case MC_TOK_LTE:
        case MC_TOK_GT:
        case MC_TOK_GTE:
            return MC_ASTPREC_LESSGREATER;
        case MC_TOK_PLUS:
        case MC_TOK_UNARYMINUS:
        case MC_TOK_UNARYBINNOT:
            return MC_ASTPREC_SUM;
        case MC_TOK_SLASH:
        case MC_TOK_ASTERISK:
        case MC_TOK_PERCENT:
            return MC_ASTPREC_PRODUCT;
        case MC_TOK_LPAREN:
        case MC_TOK_LBRACKET:
            return MC_ASTPREC_POSTFIX;
        case MC_TOK_ASSIGN:
        case MC_TOK_ASSIGNPLUS:
        case MC_TOK_ASSIGNMINUS:
        case MC_TOK_ASSIGNASTERISK:
        case MC_TOK_ASSIGNSLASH:
        case MC_TOK_ASSIGNPERCENT:
        case MC_TOK_ASSIGNBINAND:
        case MC_TOK_ASSIGNBINOR:
        case MC_TOK_ASSIGNBINXOR:
        case MC_TOK_ASSIGNLSHIFT:
        case MC_TOK_ASSIGNRSHIFT:
            return MC_ASTPREC_ASSIGN;
        case MC_TOK_DOT:
            return MC_ASTPREC_POSTFIX;
        case MC_TOK_AND:
            return MC_ASTPREC_LOGICALAND;
        case MC_TOK_OR:
            return MC_ASTPREC_LOGICALOR;
        case MC_TOK_BINOR:
            return MC_ASTPREC_BINOR;
        case MC_TOK_BINXOR:
            return MC_ASTPREC_BINXOR;
        case MC_TOK_BINAND:
            return MC_ASTPREC_BINAND;
        case MC_TOK_LSHIFT:
        case MC_TOK_RSHIFT:
            return MC_ASTPREC_SHIFT;
        case MC_TOK_QUESTION:
            return MC_ASTPREC_TERNARY;
        case MC_TOK_PLUSPLUS:
        case MC_TOK_MINUSMINUS:
            return MC_ASTPREC_INCDEC;
        default:
            break;
    }
    return MC_ASTPREC_LOWEST;
}

mcastmathoptype_t mc_parser_tokentomathop(mcasttoktype_t tk)
{
    switch(tk)
    {
        case MC_TOK_ASSIGN:
            return MC_MATHOP_ASSIGN;
        case MC_TOK_PLUS:
            return MC_MATHOP_PLUS;
        case MC_TOK_UNARYMINUS:
            return MC_MATHOP_MINUS;
        case MC_TOK_UNARYBINNOT:
            return MC_MATHOP_BINNOT;
        case MC_TOK_BANG:
            return MC_MATHOP_BANG;
        case MC_TOK_ASTERISK:
            return MC_MATHOP_ASTERISK;
        case MC_TOK_SLASH:
            return MC_MATHOP_SLASH;
        case MC_TOK_LT:
            return MC_MATHOP_LT;
        case MC_TOK_LTE:
            return MC_MATHOP_LTE;
        case MC_TOK_GT:
            return MC_MATHOP_GT;
        case MC_TOK_GTE:
            return MC_MATHOP_GTE;
        case MC_TOK_EQ:
            return MC_MATHOP_EQ;
        case MC_TOK_NOTEQ:
            return MC_MATHOP_NOTEQ;
        case MC_TOK_PERCENT:
            return MC_MATHOP_MODULUS;
        case MC_TOK_AND:
            return MC_MATHOP_LOGICALAND;
        case MC_TOK_OR:
            return MC_MATHOP_LOGICALOR;
        case MC_TOK_ASSIGNPLUS:
            return MC_MATHOP_PLUS;
        case MC_TOK_ASSIGNMINUS:
            return MC_MATHOP_MINUS;
        case MC_TOK_ASSIGNASTERISK:
            return MC_MATHOP_ASTERISK;
        case MC_TOK_ASSIGNSLASH:
            return MC_MATHOP_SLASH;
        case MC_TOK_ASSIGNPERCENT:
            return MC_MATHOP_MODULUS;
        case MC_TOK_ASSIGNBINAND:
            return MC_MATHOP_BINAND;
        case MC_TOK_ASSIGNBINOR:
            return MC_MATHOP_BINOR;
        case MC_TOK_ASSIGNBINXOR:
            return MC_MATHOP_BINXOR;
        case MC_TOK_ASSIGNLSHIFT:
            return MC_MATHOP_LSHIFT;
        case MC_TOK_ASSIGNRSHIFT:
            return MC_MATHOP_RSHIFT;
        case MC_TOK_BINAND:
            return MC_MATHOP_BINAND;
        case MC_TOK_BINOR:
            return MC_MATHOP_BINOR;
        case MC_TOK_BINXOR:
            return MC_MATHOP_BINXOR;
        case MC_TOK_LSHIFT:
            return MC_MATHOP_LSHIFT;
        case MC_TOK_RSHIFT:
            return MC_MATHOP_RSHIFT;
        case MC_TOK_PLUSPLUS:
            return MC_MATHOP_PLUS;
        case MC_TOK_MINUSMINUS:
            return MC_MATHOP_MINUS;
        default:
            {
                MC_ASSERT(false);
            }
            break;
    }
    return MC_MATHOP_NONE;

}

char mc_parser_getescapechar(char c)
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

char* mc_parser_processandcopystring(mcstate_t* state, const char* input, size_t len)
{
    size_t ini;
    size_t outi;
    char* output;
    output = (char*)mc_allocator_malloc(state, len + 1);
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
            output[outi] = mc_parser_getescapechar(input[ini]);
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

mcastexpression_t* mc_parser_makefunccallexpr(mcstate_t* state, mcastexpression_t* expr, const char* fname)
{
    bool ok;
    mcasttoken_t fntoken;
    mcastident_t* ident;
    mcptrlist_t* args;
    mcastexpression_t* ce;
    mcastexpression_t* functionidentexpr;
    mc_asttoken_init(&fntoken, MC_TOK_IDENT, fname, mc_util_strlen(fname));
    fntoken.pos = expr->pos;
    ident = mc_astident_make(state, fntoken);
    if(!ident)
    {
        return NULL;
    }
    ident->pos = fntoken.pos;
    functionidentexpr = mc_astexpr_makeident(state, ident);
    if(!functionidentexpr)
    {
        mc_astident_destroy(ident);
        return NULL;
    }
    functionidentexpr->pos = expr->pos;
    ident = NULL;
    args = mc_ptrlist_make(state, 0);
    if(!args)
    {
        mc_astexpr_destroy(functionidentexpr);
        return NULL;
    }
    ok = mc_ptrlist_push(args, expr);
    if(!ok)
    {
        mc_ptrlist_destroy(args, NULL);
        mc_astexpr_destroy(functionidentexpr);
        return NULL;
    }
    ce = mc_astexpr_makecallexpr(state, functionidentexpr, args);
    if(!ce)
    {
        mc_ptrlist_destroy(args, NULL);
        mc_astexpr_destroy(functionidentexpr);
        return NULL;
    }
    ce->pos = expr->pos;
    return ce;
}

mcastexpression_t* mc_optimizer_optexpression(mcastexpression_t* expr)
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

mcastexpression_t* mc_optimizer_optinfixexpr(mcastexpression_t* expr)
{
    bool leftisnumeric;
    bool rightisnumeric;
    bool leftisstring;
    bool rightisstring;
    mcfloat_t dnleft;
    mcfloat_t dnright;
    size_t len;
    mcstate_t* state;
    mcastexpression_t* res;
    mcastexpression_t* left;
    mcastexpression_t* right;
    mcastexpression_t* leftoptimized;
    mcastexpression_t* rightoptimized;
    state = expr->pstate;
    left = expr->uexpr.exprinfix.left;
    leftoptimized = mc_optimizer_optexpression(left);
    if(leftoptimized)
    {
        left = leftoptimized;
    }
    right = expr->uexpr.exprinfix.right;
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
        dnleft = left->type == MC_EXPR_NUMBERLITERAL ? left->uexpr.exprlitnumber : left->uexpr.exprlitbool;
        dnright = right->type == MC_EXPR_NUMBERLITERAL ? right->uexpr.exprlitnumber : right->uexpr.exprlitbool;
        switch(expr->uexpr.exprinfix.op)
        {
            case MC_MATHOP_PLUS:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_add(dnleft, dnright));
                }
                break;
            case MC_MATHOP_MINUS:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_sub(dnleft, dnright));
                }
                break;
            case MC_MATHOP_ASTERISK:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_mult(dnleft, dnright));
                }
                break;
            case MC_MATHOP_SLASH:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_div(dnleft, dnright));
                }
                break;
            case MC_MATHOP_LT:
                {
                    res = mc_astexpr_makeliteralbool(state, dnleft < dnright);
                }
                break;
            case MC_MATHOP_LTE:
                {
                    res = mc_astexpr_makeliteralbool(state, dnleft <= dnright);
                }
                break;
            case MC_MATHOP_GT:
                {
                    res = mc_astexpr_makeliteralbool(state, dnleft > dnright);
                }
                break;
            case MC_MATHOP_GTE:
                {
                    res = mc_astexpr_makeliteralbool(state, dnleft >= dnright);
                }
                break;
            case MC_MATHOP_EQ:
                {
                    res = mc_astexpr_makeliteralbool(state, MC_UTIL_CMPFLOAT(dnleft, dnright));
                }
                break;
            case MC_MATHOP_NOTEQ:
                {
                    res = mc_astexpr_makeliteralbool(state, !MC_UTIL_CMPFLOAT(dnleft, dnright));
                }
                break;
            case MC_MATHOP_MODULUS:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_mod(dnleft, dnright));
                }
                break;
            case MC_MATHOP_BINAND:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_binand(dnleft, dnright));
                }
                break;
            case MC_MATHOP_BINOR:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_binor(dnleft, dnright));
                }
                break;
            case MC_MATHOP_BINXOR:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_binxor(dnleft, dnright));
                }
                break;
            case MC_MATHOP_LSHIFT:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_binshiftleft(dnleft, dnright));
                }
                break;
            case MC_MATHOP_RSHIFT:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_binshiftright(dnleft, dnright));
                }
                break;
            default:
                {
                }
                break;
        }
    }
    else if(expr->uexpr.exprinfix.op == MC_MATHOP_PLUS && leftisstring && rightisstring)
    {
        /* TODO:FIXME: horrible method of joining strings!!!!!!! */
        char* resstr;
        const char* strleft;
        const char* strright;
        strleft = left->uexpr.exprlitstring.data;
        strright = right->uexpr.exprlitstring.data;
        resstr = mc_util_stringallocfmt(state, "%s%s", strleft, strright);
        len = mc_util_strlen(resstr);
        if(resstr)
        {
            res = mc_astexpr_makeliteralstring(state, resstr, len);
            if(!res)
            {
                mc_allocator_free(state, resstr);
            }
        }
    }
    mc_astexpr_destroy(leftoptimized);
    mc_astexpr_destroy(rightoptimized);
    if(res)
    {
        res->pos = expr->pos;
    }
    return res;
}

mcastexpression_t* mc_optimizer_optprefixexpr(mcastexpression_t* expr)
{
    mcastexpression_t* res;
    mcastexpression_t* right;
    mcastexpression_t* rightoptimized;
    right = expr->uexpr.exprprefix.right;
    rightoptimized = mc_optimizer_optexpression(right);
    if(rightoptimized)
    {
        right = rightoptimized;
    }
    res = NULL;
    if(expr->uexpr.exprprefix.op == MC_MATHOP_MINUS && right->type == MC_EXPR_NUMBERLITERAL)
    {
        res = mc_astexpr_makeliteralnumber(expr->pstate, -right->uexpr.exprlitnumber);
    }
    else if(expr->uexpr.exprprefix.op == MC_MATHOP_BANG && right->type == MC_EXPR_BOOLLITERAL)
    {
        res = mc_astexpr_makeliteralbool(expr->pstate, !right->uexpr.exprlitbool);
    }
    mc_astexpr_destroy(rightoptimized);
    if(res)
    {
        res->pos = expr->pos;
    }
    return res;
}

#define APPEND_BYTE(n) \
    do \
    { \
        val = (uint8_t)(operands[i] >> (n * 8)); \
        ok = mc_basicarray_push(res, &val); \
        if(!ok) \
        { \
            return 0; \
        } \
    } while(0)

int mc_compiler_gencode(mcinternopcode_t op, int operandscount, const uint64_t* operands, mcbasicarray_t* res)
{
    bool ok;
    int i;
    int width;
    int instrlen;
    uint8_t val;
    mcopdefinition_t vdef;
    mcopdefinition_t* def;
    def = mc_opdef_lookup(&vdef, op);
    if(!def)
    {
        return 0;
    }
    instrlen = 1;
    for(i = 0; i < def->numoperands; i++)
    {
        instrlen += def->operandwidths[i];
    }
    val = op;
    ok = false;
    ok = mc_basicarray_push(res, &val);
    if(!ok)
    {
        return 0;
    }
    for(i = 0; i < operandscount; i++)
    {
        width = def->operandwidths[i];
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
    return instrlen;
}
#undef APPEND_BYTE


int mc_compiler_emit(mcastcompiler_t* comp, mcinternopcode_t op, int operandscount, uint64_t* operands)
{
    bool ok;
    int i;
    int ip;
    int len;
    mcastlocation_t* srcpos;
    mcastscopecomp_t* compscope;
    ip = mc_compiler_getip(comp);
    len = mc_compiler_gencode(op, operandscount, operands, mc_compiler_getbytecode(comp));
    if(len == 0)
    {
        return -1;
    }
    for(i = 0; i < len; i++)
    {
        srcpos = (mcastlocation_t*)mc_basicarray_top(comp->srcposstack);
        MC_ASSERT(srcpos->line >= 0);
        MC_ASSERT(srcpos->column >= 0);
        ok = mc_basicarray_push(mc_compiler_getsrcpositions(comp), srcpos);
        if(!ok)
        {
            return -1;
        }
    }
    compscope = mc_compiler_getcompilationscope(comp);
    compscope->lastopcode = op;
    return ip;
}

mcastscopecomp_t* mc_compiler_getcompilationscope(mcastcompiler_t* comp)
{
    return comp->compilationscope;
}

bool mc_compiler_pushcompilationscope(mcastcompiler_t* comp)
{
    mcastscopecomp_t* nscope;
    mcastscopecomp_t* currentscope;
    currentscope = mc_compiler_getcompilationscope(comp);
    nscope = mc_astcompscope_make(comp->pstate, currentscope);
    if(!nscope)
    {
        return false;
    }
    mc_compiler_setcompilationscope(comp, nscope);
    return true;
}

void mc_compiler_popcompilationscope(mcastcompiler_t* comp)
{
    mcastscopecomp_t* currentscope;
    currentscope = mc_compiler_getcompilationscope(comp);
    MC_ASSERT(currentscope);
    mc_compiler_setcompilationscope(comp, currentscope->outer);
    mc_astcompscope_destroy(currentscope);
}

bool mc_compiler_pushsymtable(mcastcompiler_t* comp, int globaloffset)
{
    mcastscopefile_t* filescope;
    mcastsymtable_t* currenttable;
    filescope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    if(!filescope)
    {
        MC_ASSERT(false);
        return false;
    }
    currenttable = filescope->filesymtab;
    filescope->filesymtab = mc_symtable_make(comp->pstate, currenttable, comp->compglobalstore, globaloffset);
    if(!filescope->filesymtab)
    {
        filescope->filesymtab = currenttable;
        return false;
    }
    return true;
}

void mc_compiler_popsymtable(mcastcompiler_t* comp)
{
    mcastscopefile_t* filescope;
    mcastsymtable_t* currenttable;
    filescope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    if(!filescope)
    {
        MC_ASSERT(false);
        return;
    }
    currenttable = filescope->filesymtab;
    if(!currenttable)
    {
        MC_ASSERT(false);
        return;
    }
    filescope->filesymtab = currenttable->outer;
    mc_symtable_destroy(currenttable);
}

mcinternopcode_t mc_compiler_getlastopcode(mcastcompiler_t* comp)
{
    mcastscopecomp_t* currentscope;
    currentscope = mc_compiler_getcompilationscope(comp);
    return currentscope->lastopcode;
}



bool mc_compiler_docompilesource(mcastcompiler_t* comp, const char* code)
{
    bool ok;
    mcstate_t* state;
    mcptrlist_t* statements;
    mcastscopefile_t* filescope;
    state = comp->pstate;
    filescope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    MC_ASSERT(filescope);
    statements = mc_astparser_parseall(filescope->parser, code, filescope->file);
    if(!statements)
    {
        /* errors are added by parser */
        return false;
    }
    if(comp->pstate->config.dumpast)
    {
        mc_astprinter_printast(state, statements);
    }
    ok = mc_compiler_compilestmtlist(comp, statements);
    mc_ptrlist_destroy(statements, (mcitemdestroyfn_t)mc_astexpr_destroy);
    if(comp->pstate->config.dumpbytecode)
    {
        mc_printer_printbytecode(state->stderrprinter,
            (uint8_t*)mc_basicarray_data(comp->compilationscope->bytecode),
            (mcastlocation_t*)mc_basicarray_data(comp->compilationscope->scopesrcposlist),
            mc_basicarray_count(comp->compilationscope->bytecode), false);
    }
    return ok;
}

bool mc_compiler_compilestmtlist(mcastcompiler_t* comp, mcptrlist_t* statements)
{
    bool ok;
    size_t i;
    mcastexpression_t* expr;
    ok = true;
    for(i = 0; i < mc_ptrlist_count(statements); i++)
    {
        expr = (mcastexpression_t*)mc_ptrlist_get(statements, i);
        ok = mc_compiler_compilestatement(comp, expr);
        if(!ok)
        {
            break;
        }
    }
    return ok;
}

bool mc_compiler_compileimport(mcastcompiler_t* comp, mcastexpression_t* importstmt)
{
    bool ok;
    bool result;
    size_t i;
    size_t flen;
    char* code;
    char* filepath;
    char* namecopy;
    const char* modpath;
    const char* modname;
    const char* loadedname;
    const char* searchedpath;
    const char* filepathnoncanonicalised;
    mcprinter_t* filepathbuf;
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
    filescope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    modpath = importstmt->uexpr.exprimportstmt.path;
    modname = mc_util_getmodulename(modpath);
    for(i = 0; i < mc_ptrlist_count(filescope->loadedmodnames); i++)
    {
        loadedname = (const char*)mc_ptrlist_get(filescope->loadedmodnames, i);
        if(mc_util_strequal(loadedname, modname))
        {
            if(comp->pstate->config.fatalcomplaints)
            {
                mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, importstmt->pos, "Module \"%s\" was already imported", modname);
                result = false;
            }
            else
            {
                mc_state_complain(comp->pstate, importstmt->pos, "module \"%s\" already imported; ignoring 'import' statement", modname);
                result = true;
            }
            goto end;
        }
    }
    filepathbuf = mc_printer_make(comp->pstate, NULL);
    if(!filepathbuf)
    {
        result = false;
        goto end;
    }
    if(mc_util_pathisabsolute(modpath))
    {
        mc_printer_printf(filepathbuf, "%s.mc", modpath);
    }
    else
    {
        mc_printer_printf(filepathbuf, "%s%s.mc", filescope->file->dir_path, modpath);
    }

    if(mc_printer_failed(filepathbuf))
    {
        mc_printer_destroy(filepathbuf);
        result = false;
        goto end;
    }
    filepathnoncanonicalised = mc_printer_getstring(filepathbuf);
    filepath = mc_util_canonpath(comp->pstate, filepathnoncanonicalised);
    mc_printer_destroy(filepathbuf);
    if(!filepath)
    {
        result = false;
        goto end;
    }
    symtab = mc_compiler_getsymtable(comp);
    if(symtab->outer != NULL || mc_ptrlist_count(symtab->blockscopes) > 1)
    {
        mc_errlist_push(comp->errors, MC_ERROR_COMPILING, importstmt->pos, "Modules can only be imported in global scope");
        result = false;
        goto end;
    }
    for(i = 0; i < mc_ptrlist_count(comp->filescopelist); i++)
    {
        fs = (mcastscopefile_t*)mc_ptrlist_get(comp->filescopelist, i);
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
        searchedpath = mc_module_findfile(comp->pstate, filepath);
        code = mc_fsutil_fileread(comp->pstate, searchedpath, &flen);
        if(!code)
        {
            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, importstmt->pos, "Reading module file \"%s\" failed", filepath);
            result = false;
            goto end;
        }
        module = mc_module_make(comp->pstate, modname);
        if(!module)
        {
            result = false;
            goto end;
        }
        ok = mc_compiler_filescopepush(comp, searchedpath);
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
    for(i = 0; i < mc_ptrlist_count(module->symbols); i++)
    {
        symbol = (mcastsymbol_t*)mc_ptrlist_get(module->symbols, i);
        ok = mc_symtable_addmodsymbol(symtab, symbol);
        if(!ok)
        {
            result = false;
            goto end;
        }
    }
    namecopy = mc_util_strdup(comp->pstate, modname);
    if(!namecopy)
    {
        result = false;
        goto end;
    }
    ok = mc_ptrlist_push(filescope->loadedmodnames, namecopy);
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

bool mc_compiler_compilestatement(mcastcompiler_t* comp, mcastexpression_t* expr)
{
    bool ok;
    int ip;
    uint64_t opbuf[10];
    mcastscopecomp_t* compscope;
    mcastsymtable_t* symtab;
    ok = false;
    ip = -1;
    ok = mc_basicarray_push(comp->srcposstack, &expr->pos);
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
                ok = mc_compiler_compileexpression(comp, expr->uexpr.exprexpression);
                if(!ok)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, NULL);
                if(ip < 0)
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTDEFINE:
            {
                mcastsymbol_t* symbol;
                ok = mc_compiler_compileexpression(comp, expr->uexpr.exprdefine.value);
                if(!ok)
                {
                    return false;
                }
                symbol = mc_compiler_defsymbol(comp, expr->uexpr.exprdefine.name->pos, expr->uexpr.exprdefine.name->value, expr->uexpr.exprdefine.assignable, false);
                if(!symbol)
                {
                    return false;
                }
                ok = mc_compiler_storesymbol(comp, symbol, true);
                if(!ok)
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTIF:
            {
                size_t i;
                int afteraltip;
                int nextcasejumpip;
                int jumptoendip;
                int afterelifip;
                int* pos;
                mcastifcase_t* ifcase;
                mcastexprstmtif_t* ifstmt;
                mcbasicarray_t* jumptoendips;
                ifstmt = &expr->uexpr.exprifstmt;
                jumptoendips = mc_basicarray_make(comp->pstate, sizeof(int));
                if(!jumptoendips)
                {
                    goto statementiferror;
                }
                for(i = 0; i < mc_ptrlist_count(ifstmt->cases); i++)
                {
                    ifcase = (mcastifcase_t*)mc_ptrlist_get(ifstmt->cases, i);
                    ok = mc_compiler_compileexpression(comp, ifcase->ifcond);
                    if(!ok)
                    {
                        goto statementiferror;
                    }
                    opbuf[0] = 0xbeef;
                    nextcasejumpip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFFALSE, 1, opbuf);
                    ok = mc_compiler_compilecodeblock(comp, ifcase->consequence);
                    if(!ok)
                    {
                        goto statementiferror;
                    }
                    /* don't emit jump for the last statement */
                    if(i < (mc_ptrlist_count(ifstmt->cases) - 1) || ifstmt->alternative)
                    {
                        opbuf[0] = 0xbeef;
                        jumptoendip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
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
            break;
        case MC_EXPR_STMTRETURN:
            {
                if(compscope->outer == NULL)
                {
                    mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "Nothing to return from");
                    return false;
                }
                ip = -1;
                if(expr->uexpr.exprreturnvalue)
                {
                    ok = mc_compiler_compileexpression(comp, expr->uexpr.exprreturnvalue);
                    if(!ok)
                    {
                        return false;
                    }
                    ip = mc_compiler_emit(comp, MC_OPCODE_RETURNVALUE, 0, NULL);
                }
                else
                {
                    ip = mc_compiler_emit(comp, MC_OPCODE_RETURN, 0, NULL);
                }
                if(ip < 0)
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTLOOPWHILE:
            {
                int beforetestip;
                int aftertestip;
                int afterbodyip;
                int jumptoafterbodyip;
                mcastexprstmtwhile_t* loop;
                loop = &expr->uexpr.exprwhileloopstmt;
                beforetestip = mc_compiler_getip(comp);
                ok = mc_compiler_compileexpression(comp, loop->loopcond);
                if(!ok)
                {
                    return false;
                }
                aftertestip = mc_compiler_getip(comp);
                opbuf[0] = aftertestip + 6;
                ip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFTRUE, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                opbuf[0] = 0xdead;
                jumptoafterbodyip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
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
                ip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                afterbodyip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, jumptoafterbodyip + 1, afterbodyip);
            }
            break;
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
                ip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
            }
            break;
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
                ip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
            }
            break;
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
                mcastexprstmtforeach_t* foreach;
                foreach = &expr->uexpr.exprforeachloopstmt;
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
                ip = mc_compiler_emit(comp, MC_OPCODE_NUMBER, 1, opbuf);
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
                    sourcesymbol = mc_symtable_resolve(symtab, foreach->source->uexpr.exprident->value);
                    if(!sourcesymbol)
                    {
                        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, foreach->source->pos, "symbol \"%s\" could not be resolved", foreach->source->uexpr.exprident->value);
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
                jumptoafterupdateip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
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
                ip = mc_compiler_emit(comp, MC_OPCODE_NUMBER, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_ADD, 0, NULL);
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
                ok = mc_basicarray_push(comp->srcposstack, &foreach->source->pos);
                if(!ok)
                {
                    return false;
                }
                ok = mc_compiler_readsymbol(comp, sourcesymbol);
                if(!ok)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_FOREACHLEN, 0, NULL);
                if(ip < 0)
                {
                    return false;
                }
                mc_basicarray_pop(comp->srcposstack, NULL);
                ok = mc_compiler_readsymbol(comp, indexsymbol);
                if(!ok)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_COMPARE, 0, NULL);
                if(ip < 0)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_EQUAL, 0, NULL);
                if(ip < 0)
                {
                    return false;
                }
                aftertestip = mc_compiler_getip(comp);
                opbuf[0] = aftertestip + 6;
                ip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFFALSE, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                opbuf[0] = 0xdead;
                jumptoafterbodyip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
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
                ip = mc_compiler_emit(comp, MC_OPCODE_GETVALUEAT, 0, NULL);
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
                ip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                afterbodyip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, jumptoafterbodyip + 1, afterbodyip);
                mc_symtable_popblockscope(symtab);
            }
            break;
        case MC_EXPR_STMTLOOPFORCLASSIC:
            {
                int afterbodyip;
                int jumptoafterupdateip;
                int updateip;
                int afterupdateip;
                int aftertestip;
                int jumptoafterbodyip;
                mcastexprstmtforloop_t* loop;
                loop = &expr->uexpr.exprforloopstmt;
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
                    jumptoafterupdateip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
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
                    ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, NULL);
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
                    ip = mc_compiler_emit(comp, MC_OPCODE_TRUE, 0, NULL);
                    if(ip < 0)
                    {
                        return false;
                    }
                }
                aftertestip = mc_compiler_getip(comp);
                opbuf[0] = aftertestip + 6;
                ip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFTRUE, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                opbuf[0] = 0xdead;
                jumptoafterbodyip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
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
                ip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
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
                ok = mc_compiler_compilecodeblock(comp, expr->uexpr.exprblockstmt);
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
                int recip;
                int afterrecoverip;
                int afterjumptorecoverip;
                int jumptoafterrecoverip;
                mcastsymbol_t* errorsymbol;
                mcastexprstmtrecover_t* recover;
                recover = &expr->uexpr.exprrecoverstmt;
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
                recip = mc_compiler_emit(comp, MC_OPCODE_SETRECOVER, 1, opbuf);
                if(recip < 0)
                {
                    return false;
                }
                opbuf[0] = 0xbeef;
                jumptoafterrecoverip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
                if(jumptoafterrecoverip < 0)
                {
                    return false;
                }
                afterjumptorecoverip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, recip + 1, afterjumptorecoverip);
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
                if(!mc_compiler_lastopcodeis(comp, MC_OPCODE_RETURN) && !mc_compiler_lastopcodeis(comp, MC_OPCODE_RETURNVALUE))
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
            break;
    }
    mc_basicarray_pop(comp->srcposstack, NULL);
    return true;
}

bool mc_compiler_compileexpression(mcastcompiler_t* comp, mcastexpression_t* expr)
{
    bool ok;
    bool res;
    int ip;
    uint64_t opbuf[10];
    mcastexpression_t* exproptimized;
    mcastscopecomp_t* compscope;
    mcastsymtable_t* symtab;
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
    ok = mc_basicarray_push(comp->srcposstack, &expr->pos);
    if(!ok)
    {
        return false;
    }
    compscope = mc_compiler_getcompilationscope(comp);
    symtab = mc_compiler_getsymtable(comp);
    res = false;
    switch(expr->type)
    {
        case MC_EXPR_INFIX:
            {
                bool rearrange;
                mcinternopcode_t op;
                mcastexpression_t* left;
                mcastexpression_t* right;
                rearrange = false;
                op = MC_OPCODE_HALT;
                switch(expr->uexpr.exprinfix.op)
                {
                    case MC_MATHOP_PLUS:
                        op = MC_OPCODE_ADD;
                        break;
                    case MC_MATHOP_MINUS:
                        op = MC_OPCODE_SUB;
                        break;
                    case MC_MATHOP_ASTERISK:
                        op = MC_OPCODE_MUL;
                        break;
                    case MC_MATHOP_SLASH:
                        op = MC_OPCODE_DIV;
                        break;
                    case MC_MATHOP_MODULUS:
                        op = MC_OPCODE_MOD;
                        break;
                    case MC_MATHOP_EQ:
                        op = MC_OPCODE_EQUAL;
                        break;
                    case MC_MATHOP_NOTEQ:
                        op = MC_OPCODE_NOTEQUAL;
                        break;
                    case MC_MATHOP_GT:
                        op = MC_OPCODE_GREATERTHAN;
                        break;
                    case MC_MATHOP_GTE:
                        op = MC_OPCODE_GREATERTHANEQUAL;
                        break;
                    case MC_MATHOP_LT:
                        op = MC_OPCODE_GREATERTHAN;
                        rearrange = true;
                        break;
                    case MC_MATHOP_LTE:
                        op = MC_OPCODE_GREATERTHANEQUAL;
                        rearrange = true;
                        break;
                    case MC_MATHOP_BINOR:
                        op = MC_OPCODE_BINOR;
                        break;
                    case MC_MATHOP_BINXOR:
                        op = MC_OPCODE_BINXOR;
                        break;
                    case MC_MATHOP_BINAND:
                        op = MC_OPCODE_BINAND;
                        break;
                    case MC_MATHOP_LSHIFT:
                        op = MC_OPCODE_LSHIFT;
                        break;
                    case MC_MATHOP_RSHIFT:
                        op = MC_OPCODE_RSHIFT;
                        break;
                    default:
                        {
                            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "Unknown infix operator");
                            goto error;
                        }
                        break;
                }
                left = rearrange ? expr->uexpr.exprinfix.right : expr->uexpr.exprinfix.left;
                right = rearrange ? expr->uexpr.exprinfix.left : expr->uexpr.exprinfix.right;
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
                switch(expr->uexpr.exprinfix.op)
                {
                    case MC_MATHOP_EQ:
                    case MC_MATHOP_NOTEQ:
                        {
                            ip = mc_compiler_emit(comp, MC_OPCODE_COMPAREEQ, 0, NULL);
                            if(ip < 0)
                            {
                                goto error;
                            }
                        }
                        break;
                    case MC_MATHOP_GT:
                    case MC_MATHOP_GTE:
                    case MC_MATHOP_LT:
                    case MC_MATHOP_LTE:
                        {
                            ip = mc_compiler_emit(comp, MC_OPCODE_COMPARE, 0, NULL);
                            if(ip < 0)
                            {
                                goto error;
                            }
                        }
                        break;
                    default:
                        {
                        }
                        break;
                }
                ip = mc_compiler_emit(comp, op, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;

        case MC_EXPR_NUMBERLITERAL:
            {
                mcfloat_t number;
                number = expr->uexpr.exprlitnumber;
                opbuf[0] = mc_util_doubletouint64(number);
                ip = mc_compiler_emit(comp, MC_OPCODE_NUMBER, 1, opbuf);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;

        case MC_EXPR_STRINGLITERAL:
            {
                int pos = 0;
                int* posval;
                int* currentpos;
                mcvalue_t obj;
                currentpos = (int*)mc_genericdict_get(comp->stringconstposdict, expr->uexpr.exprlitstring.data);
                if(currentpos)
                {
                    pos = *currentpos;
                }
                else
                {
                    obj = mc_value_makestringlen(comp->pstate, expr->uexpr.exprlitstring.data, expr->uexpr.exprlitstring.length);
                    if(mc_value_isnull(obj))
                    {
                        goto error;
                    }
                    pos = mc_compiler_addconstant(comp, obj);
                    if(pos < 0)
                    {
                        goto error;
                    }
                    posval = (int*)mc_allocator_malloc(comp->pstate, sizeof(int));
                    if(!posval)
                    {
                        goto error;
                    }
                    *posval = pos;
                    ok = mc_genericdict_set(comp->stringconstposdict, expr->uexpr.exprlitstring.data, posval);
                    if(!ok)
                    {
                        mc_allocator_free(comp->pstate, posval);
                        goto error;
                    }
                }
                opbuf[0] = pos;
                ip = mc_compiler_emit(comp, MC_OPCODE_CONSTANT, 1, opbuf);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_NULLLITERAL:
            {
                ip = mc_compiler_emit(comp, MC_OPCODE_NULL, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_BOOLLITERAL:
            {
                ip = mc_compiler_emit(comp, expr->uexpr.exprlitbool ? MC_OPCODE_TRUE : MC_OPCODE_FALSE, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_ARRAYLITERAL:
            {
                size_t i;
                for(i = 0; i < mc_ptrlist_count(expr->uexpr.exprlitarray.litarritems); i++)
                {
                    ok = mc_compiler_compileexpression(comp, (mcastexpression_t*)mc_ptrlist_get(expr->uexpr.exprlitarray.litarritems, i));
                    if(!ok)
                    {
                        goto error;
                    }
                }
                opbuf[0] = mc_ptrlist_count(expr->uexpr.exprlitarray.litarritems);
                ip = mc_compiler_emit(comp, MC_OPCODE_ARRAY, 1, opbuf);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_MAPLITERAL:
            {
                size_t i;
                size_t len;
                mcastexpression_t* key;
                mcastexpression_t* val;
                mcastliteralmap_t* map;
                map = &expr->uexpr.exprlitmap;
                len = mc_ptrlist_count(map->keys);
                opbuf[0] = len;
                ip = mc_compiler_emit(comp, MC_OPCODE_MAPSTART, 1, opbuf);
                if(ip < 0)
                {
                    goto error;
                }
                for(i = 0; i < len; i++)
                {
                    key = (mcastexpression_t*)mc_ptrlist_get(map->keys, i);
                    val = (mcastexpression_t*)mc_ptrlist_get(map->values, i);
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
                ip = mc_compiler_emit(comp, MC_OPCODE_MAPEND, 1, opbuf);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_PREFIX:
            {
                mcinternopcode_t op;
                ok = mc_compiler_compileexpression(comp, expr->uexpr.exprprefix.right);
                if(!ok)
                {
                    goto error;
                }
                op = MC_OPCODE_HALT;
                switch(expr->uexpr.exprprefix.op)
                {
                    case MC_MATHOP_MINUS:
                        op = MC_OPCODE_MINUS;
                        break;
                    case MC_MATHOP_BINNOT:
                        op = MC_OPCODE_BINNOT;
                        break;
                    case MC_MATHOP_BANG:
                        op = MC_OPCODE_BANG;
                        break;
                    default:
                        {
                            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "Unknown prefix operator.");
                            goto error;
                        }
                        break;
                }
                ip = mc_compiler_emit(comp, op, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_IDENT:
            {
                mcastsymbol_t* symbol;
                mcastident_t* ident;
                ident = expr->uexpr.exprident;
                symbol = mc_symtable_resolve(symtab, ident->value);
                if(!symbol)
                {
                    #if 0
                        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, ident->pos, "compilation: failed to resolve symbol \"%s\"", ident->value);
                        goto error;
                    #else
                        symbol = mc_compiler_defsymbol(comp, ident->pos, ident->value, true, false);
                    #endif

                }
                ok = mc_compiler_readsymbol(comp, symbol);
                if(!ok)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_INDEX:
            {
                mcastexprindex_t* index;
                index = &expr->uexpr.exprindex;
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
                #if 1
                if(index->isdot)
                {
                    ip = mc_compiler_emit(comp, MC_OPCODE_GETDOTINDEX, 0, NULL);
                }
                else
                #endif
                {
                    ip = mc_compiler_emit(comp, MC_OPCODE_GETINDEX, 0, NULL);
                }
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_FUNCTIONLITERAL:
            {
                size_t i;
                int pos;
                int nlocals;
                mcvalue_t obj;
                mcptrlist_t* freesyms;
                mccompiledprogram_t* comp_res;
                mcastliteralfunction_t* fn;
                mcastsymbol_t* symbol;
                mcastsymbol_t* fnsymbol;
                mcastsymbol_t* thissymbol;
                mcastsymbol_t* paramsymbol;
                mcastfuncparam_t* param;
                fn = &expr->uexpr.exprlitfunction;
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
                    fnsymbol = mc_symtable_definefunctionname(symtab, fn->name, false);
                    if(!fnsymbol)
                    {
                        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "Cannot define symbol \"%s\"", fn->name);
                        goto error;
                    }
                }
                thissymbol = mc_symtable_definethis(symtab);
                if(!thissymbol)
                {
                    mc_errlist_push(comp->errors, MC_ERROR_COMPILING, expr->pos, "Cannot define \"this\" symbol");
                    goto error;
                }
                for(i = 0; i < mc_ptrlist_count(expr->uexpr.exprlitfunction.funcparamlist); i++)
                {
                    param = (mcastfuncparam_t*)mc_ptrlist_get(expr->uexpr.exprlitfunction.funcparamlist, i);
                    paramsymbol = mc_compiler_defsymbol(comp, param->ident->pos, param->ident->value, true, false);
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
                if(!mc_compiler_lastopcodeis(comp, MC_OPCODE_RETURNVALUE) && !mc_compiler_lastopcodeis(comp, MC_OPCODE_RETURN))
                {
                    ip = mc_compiler_emit(comp, MC_OPCODE_RETURN, 0, NULL);
                    if(ip < 0)
                    {
                        goto error;
                    }
                }
                freesyms = symtab->freesymbols;
                /* because it gets destroyed with compiler_pop_compilation_scope() */
                symtab->freesymbols = NULL;
                nlocals = symtab->maxnumdefinitions;
                comp_res = mc_astcompscope_orphanresult(compscope);
                if(!comp_res)
                {
                    mc_ptrlist_destroy(freesyms, (mcitemdestroyfn_t)mc_symbol_destroy);
                    goto error;
                }
                mc_compiler_popsymtable(comp);
                mc_compiler_popcompilationscope(comp);
                compscope = mc_compiler_getcompilationscope(comp);
                symtab = mc_compiler_getsymtable(comp);
                obj = mc_value_makefuncscript(comp->pstate, fn->name, comp_res, true, nlocals, mc_ptrlist_count(fn->funcparamlist), 0);
                if(mc_value_isnull(obj))
                {
                    mc_ptrlist_destroy(freesyms, (mcitemdestroyfn_t)mc_symbol_destroy);
                    mc_astcompresult_destroy(comp_res);
                    goto error;
                }
                for(i = 0; i < mc_ptrlist_count(freesyms); i++)
                {
                    symbol = (mcastsymbol_t*)mc_ptrlist_get(freesyms, i);
                    ok = mc_compiler_readsymbol(comp, symbol);
                    if(!ok)
                    {
                        mc_ptrlist_destroy(freesyms, (mcitemdestroyfn_t)mc_symbol_destroy);
                        goto error;
                    }
                }
                pos = mc_compiler_addconstant(comp, obj);
                if(pos < 0)
                {
                    mc_ptrlist_destroy(freesyms, (mcitemdestroyfn_t)mc_symbol_destroy);
                    goto error;
                }
                opbuf[0] = pos;
                opbuf[1] = mc_ptrlist_count(freesyms);
                ip = mc_compiler_emit(comp, MC_OPCODE_FUNCTION, 2, opbuf);
                if(ip < 0)
                {
                    mc_ptrlist_destroy(freesyms, (mcitemdestroyfn_t)mc_symbol_destroy);
                    goto error;
                }
                mc_ptrlist_destroy(freesyms, (mcitemdestroyfn_t)mc_symbol_destroy);
            }
            break;

        case MC_EXPR_CALL:
            {
                size_t i;
                mcastexpression_t* argexpr;
                ok = mc_compiler_compileexpression(comp, expr->uexpr.exprcall.function);
                if(!ok)
                {
                    goto error;
                }
                for(i = 0; i < mc_ptrlist_count(expr->uexpr.exprcall.args); i++)
                {
                    argexpr = (mcastexpression_t*)mc_ptrlist_get(expr->uexpr.exprcall.args, i);
                    ok = mc_compiler_compileexpression(comp, argexpr);
                    if(!ok)
                    {
                        goto error;
                    }
                }
                opbuf[0] = mc_ptrlist_count(expr->uexpr.exprcall.args);
                ip = mc_compiler_emit(comp, MC_OPCODE_CALL, 1, opbuf);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_ASSIGN:
            {
                mcastexprindex_t* index;
                mcastexprassign_t* assign;
                mcastident_t* ident;
                mcastsymbol_t* symbol;
                assign = &expr->uexpr.exprassign;
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
                ip = mc_compiler_emit(comp, MC_OPCODE_DUP, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
                ok = mc_basicarray_push(comp->srcposstack, &assign->dest->pos);
                if(!ok)
                {
                    goto error;
                }
                if(assign->dest->type == MC_EXPR_IDENT)
                {
                    ident = assign->dest->uexpr.exprident;
                    symbol = mc_symtable_resolve(symtab, ident->value);
                    if(!symbol)
                    {
                        #if 0
                            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, assign->dest->pos, "cannot assign to undeclared symbol \"%s\"", ident->value);
                            goto error;

                        #else
                            symbol = mc_compiler_defsymbol(comp, ident->pos, ident->value, true, false);
                            if(!symbol)
                            {
                                mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, assign->dest->pos, "failed to implicitly create symbol \"%s\"", ident->value);
                                goto error;
                            }
                        #endif
                    }
                    if(!symbol->assignable)
                    {
                        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, assign->dest->pos, "compilation: cannot assign to readonly symbol \"%s\"", ident->value);
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
                    index = &assign->dest->uexpr.exprindex;
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
                    ip = mc_compiler_emit(comp, MC_OPCODE_SETINDEX, 0, NULL);
                    if(ip < 0)
                    {
                        goto error;
                    }
                }
                if(assign->is_postfix)
                {
                    ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, NULL);
                    if(ip < 0)
                    {
                        goto error;
                    }
                }
                mc_basicarray_pop(comp->srcposstack, NULL);
            }
            break;

        case MC_EXPR_LOGICAL:
            {
                int afterrightip;
                int afterleftjumpip;
                mcastexprlogical_t* logi;
                logi = &expr->uexpr.exprlogical;
                ok = mc_compiler_compileexpression(comp, logi->left);
                if(!ok)
                {
                    goto error;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_DUP, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
                afterleftjumpip = 0;
                if(logi->op == MC_MATHOP_LOGICALAND)
                {
                    opbuf[0] = 0xbeef;
                    afterleftjumpip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFFALSE, 1, opbuf);
                }
                else
                {
                    opbuf[0] = 0xbeef;
                    afterleftjumpip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFTRUE, 1, opbuf);
                }
                if(afterleftjumpip < 0)
                {
                    goto error;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, NULL);
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
                mcastexprternary_t* ternary;
                ternary = &expr->uexpr.exprternary;
                ok = mc_compiler_compileexpression(comp, ternary->tercond);
                if(!ok)
                {
                    goto error;
                }
                opbuf[0] = 0xbeef;
                elsejumpip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFFALSE, 1, opbuf);
                ok = mc_compiler_compileexpression(comp, ternary->teriftrue);
                if(!ok)
                {
                    goto error;
                }
                opbuf[0] = 0xbeef;
                endjumpip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
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
    mc_basicarray_pop(comp->srcposstack, NULL);
    mc_astexpr_destroy(exproptimized);
    return res;
}

bool mc_compiler_compilecodeblock(mcastcompiler_t* comp, mcastcodeblock_t* block)
{
    bool ok;
    size_t i;
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
    if(mc_ptrlist_count(block->statements) == 0)
    {
        ip = mc_compiler_emit(comp, MC_OPCODE_NULL, 0, NULL);
        if(ip < 0)
        {
            return false;
        }
        ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, NULL);
        if(ip < 0)
        {
            return false;
        }
    }
    for(i = 0; i < mc_ptrlist_count(block->statements); i++)
    {
        expr = (mcastexpression_t*)mc_ptrlist_get(block->statements, i);
        ok = mc_compiler_compilestatement(comp, expr);
        if(!ok)
        {
            return false;
        }
    }
    mc_symtable_popblockscope(symtab);
    return true;
}

int mc_compiler_addconstant(mcastcompiler_t* comp, mcvalue_t obj)
{
    bool ok;
    int pos;
    ok = mc_basicarray_push(comp->constants, &obj);
    if(!ok)
    {
        return -1;
    }
    pos = mc_basicarray_count(comp->constants) - 1;
    return pos;
}

void mc_compiler_changeuint16operand(mcastcompiler_t* comp, int ip, uint16_t operand)
{
    uint8_t hi;
    uint8_t lo;
    mcbasicarray_t* bytecode;
    bytecode = mc_compiler_getbytecode(comp);
    if((ip + 1) >= (int)mc_basicarray_count(bytecode))
    {
        MC_ASSERT(false);
        return;
    }
    hi = operand >> 8;
    mc_basicarray_set(bytecode, ip, &hi);
    lo = operand;
    mc_basicarray_set(bytecode, ip + 1, &lo);
}

bool mc_compiler_lastopcodeis(mcastcompiler_t* comp, mcinternopcode_t op)
{
    mcinternopcode_t lastopcode;
    lastopcode = mc_compiler_getlastopcode(comp);
    return lastopcode == op;
}

bool mc_compiler_readsymbol(mcastcompiler_t* comp, mcastsymbol_t* symbol)
{
    int ip;
    uint64_t opbuf[10];
    ip = -1;
    if(symbol->type == MC_SYM_MODULEGLOBAL)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_GETMODULEGLOBAL, 1, opbuf);
    }
    else if(symbol->type == MC_SYM_GLOBALBUILTIN)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_GETGLOBALBUILTIN, 1, opbuf);
    }
    else if(symbol->type == MC_SYM_LOCAL)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_GETLOCAL, 1, opbuf);
    }
    else if(symbol->type == MC_SYM_FREE)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_GETFREE, 1, opbuf);
    }
    else if(symbol->type == MC_SYM_FUNCTION)
    {
        ip = mc_compiler_emit(comp, MC_OPCODE_CURRENTFUNCTION, 0, NULL);
    }
    else if(symbol->type == MC_SYM_THIS)
    {
        ip = mc_compiler_emit(comp, MC_OPCODE_GETTHIS, 0, NULL);
    }
    return ip >= 0;
}

bool mc_compiler_storesymbol(mcastcompiler_t* comp, mcastsymbol_t* symbol, bool define)
{
    int ip;
    uint64_t opbuf[10];
    ip = -1;
    if(symbol->type == MC_SYM_MODULEGLOBAL)
    {
        if(define)
        {
            opbuf[0] = symbol->index;
            ip = mc_compiler_emit(comp, MC_OPCODE_DEFINEMODULEGLOBAL, 1, opbuf);
        }
        else
        {
            opbuf[0] = symbol->index;
            ip = mc_compiler_emit(comp, MC_OPCODE_SETMODULEGLOBAL, 1, opbuf);
        }
    }
    else if(symbol->type == MC_SYM_LOCAL)
    {
        if(define)
        {
            opbuf[0] = symbol->index;
            ip = mc_compiler_emit(comp, MC_OPCODE_DEFINELOCAL, 1, opbuf);
        }
        else
        {
            opbuf[0] = symbol->index;
            ip = mc_compiler_emit(comp, MC_OPCODE_SETLOCAL, 1, opbuf);
        }
    }
    else if(symbol->type == MC_SYM_FREE)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_SETFREE, 1, opbuf);
    }
    return ip >= 0;
}

bool mc_compiler_pushbreakip(mcastcompiler_t* comp, int ip)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return mc_basicarray_push(compscope->ipstackbreak, &ip);
}

void mc_compiler_popbreakip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    if(mc_basicarray_count(compscope->ipstackbreak) == 0)
    {
        MC_ASSERT(false);
        return;
    }
    mc_basicarray_pop(compscope->ipstackbreak, NULL);
}

int mc_compiler_getbreakip(mcastcompiler_t* comp)
{
    int* res;
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    if(mc_basicarray_count(compscope->ipstackbreak) == 0)
    {
        return -1;
    }
    res = (int*)mc_basicarray_top(compscope->ipstackbreak);
    return *res;
}

bool mc_compiler_pushcontinueip(mcastcompiler_t* comp, int ip)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return mc_basicarray_push(compscope->ipstackcontinue, &ip);
}

void mc_compiler_popcontinueip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    if(mc_basicarray_count(compscope->ipstackcontinue) == 0)
    {
        MC_ASSERT(false);
        return;
    }
    mc_basicarray_pop(compscope->ipstackcontinue, NULL);
}

int mc_compiler_getcontinueip(mcastcompiler_t* comp)
{
    int* res;
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    if(mc_basicarray_count(compscope->ipstackcontinue) == 0)
    {
        MC_ASSERT(false);
        return -1;
    }
    res = (int*)mc_basicarray_top(compscope->ipstackcontinue);
    return *res;
}

int mc_compiler_getip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return mc_basicarray_count(compscope->bytecode);
}

mcbasicarray_t* mc_compiler_getsrcpositions(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return compscope->scopesrcposlist;
}

mcbasicarray_t* mc_compiler_getbytecode(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return compscope->bytecode;
}

mcastscopefile_t* mc_compiler_filescopemake(mcastcompiler_t* comp, mcastcompiledfile_t* file)
{
    mcastscopefile_t* filescope;
    filescope = (mcastscopefile_t*)mc_allocator_malloc(comp->pstate, sizeof(mcastscopefile_t));
    if(!filescope)
    {
        return NULL;
    }
    memset(filescope, 0, sizeof(mcastscopefile_t));
    filescope->pstate = comp->pstate;
    filescope->parser = mc_astparser_make(comp->pstate, comp->config, comp->errors);
    if(!filescope->parser)
    {
        goto err;
    }
    filescope->filesymtab = NULL;
    filescope->file = file;
    filescope->loadedmodnames = mc_ptrlist_make(comp->pstate, 0);
    if(!filescope->loadedmodnames)
    {
        goto err;
    }
    return filescope;
err:
    mc_compiler_filescopedestroy(filescope);
    return NULL;
}

void mc_compiler_filescopedestroy(mcastscopefile_t* scope)
{
    size_t i;
    void* name;
    for(i = 0; i < mc_ptrlist_count(scope->loadedmodnames); i++)
    {
        name = (void*)mc_ptrlist_get(scope->loadedmodnames, i);
        mc_allocator_free(scope->pstate, name);
    }
    mc_ptrlist_destroy(scope->loadedmodnames, NULL);
    mc_astparser_destroy(scope->parser);
    mc_allocator_free(scope->pstate, scope);
}

bool mc_compiler_filescopepush(mcastcompiler_t* comp, const char* filepath)
{
    bool ok;
    int globaloffset;
    mcastscopeblock_t* prevsttopscope;
    mcastsymtable_t* prevst;
    mcastcompiledfile_t* file;
    mcastscopefile_t* filescope;
    prevst = NULL;
    if(mc_ptrlist_count(comp->filescopelist) > 0)
    {
        prevst = mc_compiler_getsymtable(comp);
    }
    file = mc_compiledfile_make(comp->pstate, filepath);
    if(!file)
    {
        return false;
    }
    ok = mc_ptrlist_push(comp->files, file);
    if(!ok)
    {
        mc_compiledfile_destroy(file);
        return false;
    }
    filescope = mc_compiler_filescopemake(comp, file);
    if(!filescope)
    {
        return false;
    }
    ok = mc_ptrlist_push(comp->filescopelist, filescope);
    if(!ok)
    {
        mc_compiler_filescopedestroy(filescope);
        return false;
    }
    globaloffset = 0;
    if(prevst)
    {
        prevsttopscope = mc_symtable_getblockscope(prevst);
        globaloffset = prevsttopscope->offset + prevsttopscope->numdefinitions;
    }
    ok = mc_compiler_pushsymtable(comp, globaloffset);
    if(!ok)
    {
        mc_ptrlist_pop(comp->filescopelist);
        mc_compiler_filescopedestroy(filescope);
        return false;
    }
    return true;
}

void mc_compiler_filescopepop(mcastcompiler_t* comp)
{
    int poppednumdefs;
    mcastsymtable_t* poppedst;
    mcastsymtable_t* currentst;
    mcastscopefile_t* scope;
    mcastscopeblock_t* currentsttopscope;
    mcastscopeblock_t* poppedsttopscope;
    poppedst = mc_compiler_getsymtable(comp);
    poppedsttopscope = mc_symtable_getblockscope(poppedst);
    poppednumdefs = poppedsttopscope->numdefinitions;
    while(mc_compiler_getsymtable(comp))
    {
        mc_compiler_popsymtable(comp);
    }
    scope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    if(!scope)
    {
        MC_ASSERT(false);
        return;
    }
    mc_compiler_filescopedestroy(scope);
    mc_ptrlist_pop(comp->filescopelist);
    if(mc_ptrlist_count(comp->filescopelist) > 0)
    {
        currentst = mc_compiler_getsymtable(comp);
        currentsttopscope = mc_symtable_getblockscope(currentst);
        currentsttopscope->numdefinitions += poppednumdefs;
    }
}

void mc_compiler_setcompilationscope(mcastcompiler_t* comp, mcastscopecomp_t* scope)
{
    comp->compilationscope = scope;
}

module_t* mc_module_make(mcstate_t* state, const char* name)
{
    module_t* module;
    module = (module_t*)mc_allocator_malloc(state, sizeof(module_t));
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
    module->symbols = mc_ptrlist_make(state, 0);
    if(!module->symbols)
    {
        mc_module_destroy(module);
        return NULL;
    }
    return module;
}

const char* mc_module_findfile(mcstate_t* state, const char* filename)
{
    (void)state;
    return filename;
}

void mc_module_destroy(module_t* module)
{
    if(!module)
    {
        return;
    }
    mc_allocator_free(module->pstate, module->name);
    mc_ptrlist_destroy(module->symbols, (mcitemdestroyfn_t)mc_symbol_destroy);
    mc_allocator_free(module->pstate, module);
}

module_t* mc_module_copy(module_t* src)
{
    module_t* copy;
    copy = (module_t*)mc_allocator_malloc(src->pstate, sizeof(module_t));
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
    copy->symbols = mc_ptrlist_copy(src->symbols, (mcitemcopyfn_t)mc_symbol_copy, (mcitemdestroyfn_t)mc_symbol_destroy);
    if(!copy->symbols)
    {
        mc_module_destroy(copy);
        return NULL;
    }
    return copy;
}

const char* mc_util_getmodulename(const char* path)
{
    const char* lastslashpos;
    lastslashpos = strrchr(path, '/');
    if(lastslashpos)
    {
        return lastslashpos + 1;
    }
    return path;
}

bool mc_module_addsymbol(module_t* module, mcastsymbol_t* symbol)
{
    bool ok;
    mcastsymbol_t* modulesymbol;
    mcprinter_t* namebuf;
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
    modulesymbol = mc_symbol_make(module->pstate, mc_printer_getstring(namebuf), MC_SYM_MODULEGLOBAL, symbol->index, false);
    mc_printer_destroy(namebuf);
    if(!modulesymbol)
    {
        return false;
    }
    ok = mc_ptrlist_push(module->symbols, modulesymbol);
    if(!ok)
    {
        mc_symbol_destroy(modulesymbol);
        return false;
    }
    return true;
}

mcastsymbol_t* mc_compiler_defsymbol(mcastcompiler_t* comp, mcastlocation_t pos, const char* name, bool assignable, bool canshadow)
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

mcstate_t* mc_state_make(void)
{
    mcstate_t* state;
    state = (mcstate_t*)mc_memory_malloc(sizeof(mcstate_t));
    if(!state)
    {
        return NULL;
    }
    memset(state, 0, sizeof(mcstate_t));
    mc_state_setdefaultconfig(state);
    state->valuestack = mc_vallist_make(state, "valuestack", MC_CONF_VMVALSTACKSIZE);
    state->valthisstack = mc_vallist_make(state, "valthisstack",  MC_CONF_VMTHISSTACKSIZE);
    state->framestack = mc_framelist_make(state, MC_CONF_VMMAXFRAMES);
    mc_errlist_init(&state->errors);
    state->mem = mc_gcmemory_make(state);
    if(!state->mem)
    {
        goto err;
    }
    state->files = mc_ptrlist_make(state, 0);
    if(!state->files)
    {
        goto err;
    }
    mc_vm_init(state);
    state->vmglobalstore = mc_globalstore_make(state);
    if(!state->vmglobalstore)
    {
        goto err;
    }
    state->compiler = mc_compiler_make(state, &state->config, state->mem, &state->errors, state->files, state->vmglobalstore);
    if(!state->compiler)
    {
        goto err;
    }
    state->stdoutprinter = mc_printer_make(state, stdout);
    state->stderrprinter = mc_printer_make(state, stderr);
    return state;
err:
    mc_state_deinit(state);
    free(state);
    return NULL;
}

void mc_state_deinit(mcstate_t* state)
{
    mc_compiler_destroy(state->compiler);
    mc_globalstore_destroy(state->vmglobalstore);
    mc_gcmemory_destroy(state->mem);
    mc_ptrlist_destroy(state->files, (mcitemdestroyfn_t)mc_compiledfile_destroy);
    mc_errlist_deinit(&state->errors);
    mc_printer_destroy(state->stdoutprinter);
    mc_printer_destroy(state->stderrprinter);
    mc_vallist_destroy(state->valuestack);
    mc_framelist_destroy(state->framestack);
    mc_vallist_destroy(state->valthisstack);
}

void mc_state_reset(mcstate_t* state)
{
    mc_state_clearerrors(state);
    mc_vm_reset(state);
}

void mc_state_setdefaultconfig(mcstate_t* state)
{
    memset(&state->config, 0, sizeof(mcconfig_t));
    state->config.replmode = false;
    state->config.dumpast = false;
    state->config.dumpbytecode = false;
    state->config.fatalcomplaints = false;
}

void mc_state_destroy(mcstate_t* state)
{
    if(!state)
    {
        return;
    }
    mc_state_deinit(state);

    mc_allocator_free(state, state);
}

void mc_state_freeallocated(mcstate_t* state, void* ptr)
{
    mc_allocator_free(state, ptr);
}

void mc_state_printerrors(mcstate_t* state)
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

mccompiledprogram_t* mc_state_compilesource(mcstate_t* state, const char* code)
{
    mccompiledprogram_t* compres;
    mc_state_clearerrors(state);
    compres = mc_compiler_compilesource(state->compiler, code);
    if(state->errors.count > 0)
    {
        goto err;
    }
    return compres;
err:
    mc_astcompresult_destroy(compres);
    return NULL;
}

mcvalue_t mc_program_execute(mcstate_t* state, mccompiledprogram_t* program)
{
    bool ok;
    mcvalue_t res;
    if(program == NULL)
    {
        mc_errlist_push(&state->errors, MC_ERROR_USER, srcposinvalid, "program passed to execute was null.");
        return mc_value_makenull();
    }
    mc_state_reset(state);
    if(state != program->pstate)
    {
        mc_errlist_push(&state->errors, MC_ERROR_USER, srcposinvalid, "program was compiled with an incompatible instance");
        return mc_value_makenull();
    }
    ok = mc_vm_runexecfunc(state, program, mc_compiler_getconstants(state->compiler));
    if(!ok || state->errors.count > 0)
    {
        return mc_value_makenull();
    }
    MC_ASSERT(state->vsposition == 0);
    res = mc_vm_getlastpopped(state);
    if(res.type == MC_VAL_NONE)
    {
        return mc_value_makenull();
    }
    return res;
}

void mc_program_destroy(mccompiledprogram_t* program)
{
    if(!program)
    {
        return;
    }
    mc_astcompresult_destroy(program);
}

mcvalue_t mc_state_execcode(mcstate_t* state, const char* code)
{
    bool ok;
    mcvalue_t res;
    mccompiledprogram_t* compres;
    mc_state_reset(state);
    compres = mc_compiler_compilesource(state->compiler, code);
    if(!compres || state->errors.count > 0)
    {
        goto err;
    }
    ok = mc_vm_runexecfunc(state, compres, mc_compiler_getconstants(state->compiler));
    if(!ok || state->errors.count > 0)
    {
        goto err;
    }
    MC_ASSERT(state->vsposition == 0);
    res = mc_vm_getlastpopped(state);
    if(res.type == MC_VAL_NONE)
    {
        goto err;
    }
    mc_astcompresult_destroy(compres);
    return res;
err:
    mc_astcompresult_destroy(compres);
    return mc_value_makenull();
}


mcvalue_t mc_state_callfunctionbyname(mcstate_t* state, const char* fname, int argc, mcvalue_t* args)
{
    mcvalue_t res;
    mcvalue_t callee;
    mc_state_reset(state);
    callee = mc_state_getglobalobjectbyname(state, fname);
    if(callee.type == MC_VAL_NULL)
    {
        return mc_value_makenull();
    }
    res = mc_vm_callvalue(state, mc_compiler_getconstants(state->compiler), callee, argc, (mcvalue_t*)args);
    if(state->errors.count > 0)
    {
        return mc_value_makenull();
    }
    return res;
}

bool mc_state_haserrors(mcstate_t* state)
{
    return mc_state_errorcount(state) > 0;
}

int mc_state_errorcount(mcstate_t* state)
{
    return state->errors.count;
}

void mc_state_clearerrors(mcstate_t* state)
{
    mc_errlist_clear(&state->errors);
}

mcerror_t* mc_state_geterror(mcstate_t* state, int index)
{
    return (mcerror_t*)mc_errlist_get(&state->errors, index);
}

bool mc_state_setnativefunction(mcstate_t* state, const char* name, mcnativefn_t fn, void* data, size_t dlen)
{
    mcvalue_t obj;
    obj = mc_value_makefuncnative(state, name, fn, data, dlen);
    if(mc_value_isnull(obj))
    {
        return false;
    }
    return mc_state_setglobalconstant(state, name, obj);
}

bool mc_state_setglobalconstant(mcstate_t* state, const char* name, mcvalue_t obj)
{
    return mc_globalstore_setnamed(state->vmglobalstore, name, obj);
}

mcvalue_t mc_state_getglobalobjectbyname(mcstate_t* state, const char* name)
{
    bool ok;
    mcvalue_t res;
    mcastsymbol_t* symbol;
    mcastsymtable_t* st;
    st = mc_compiler_getsymtable(state->compiler);
    symbol = mc_symtable_resolve(st, name);
    if(!symbol)
    {
        mc_state_pusherrorf(state, MC_ERROR_USER, srcposinvalid, "Symbol \"%s\" is not defined", name);
        return mc_value_makenull();
    }
    res = mc_value_makenull();
    if(symbol->type == MC_SYM_MODULEGLOBAL)
    {
        res = mc_vm_getglobalbyindex(state, symbol->index);
    }
    else if(symbol->type == MC_SYM_GLOBALBUILTIN)
    {
        ok = false;
        res = mc_globalstore_getatindex(state->vmglobalstore, symbol->index, &ok);
        if(!ok)
        {
            mc_state_pusherrorf(state, MC_ERROR_USER, srcposinvalid, "Failed to get global object at %d", symbol->index);
            return mc_value_makenull();
        }
    }
    else
    {
        mc_state_pusherrorf(state, MC_ERROR_USER, srcposinvalid, "Value associated with symbol \"%s\" could not be loaded", name);
        return mc_value_makenull();
    }
    return res;
}

void mc_state_pusherrorfv(mcstate_t* state, mcerrtype_t type, mcastlocation_t pos, const char* fmt, va_list va)
{
    mc_errlist_addfv(&state->errors, type, pos, fmt, va);
}

void mc_state_pusherrorf(mcstate_t* state, mcerrtype_t type, mcastlocation_t pos, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    mc_state_pusherrorfv(state, type, pos, fmt, va);
    va_end(va);
}

void mc_state_complainv(mcstate_t *state, mcastlocation_t pos, const char *fmt, va_list va)
{
    int ncol;
    int nline;
    const char* fname;
    (void)state;
    fname = "unknown";
    ncol = 0;
    nline = 0;
    if(pos.file != NULL)
    {
        if(pos.file->path != NULL)
        {
            fname = pos.file->path;
        }
        nline = pos.line;
        ncol = pos.column;
    }
    fprintf(stderr, "**WARNING** [%s:%d:%d] ", fname, nline, ncol);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");
}

void mc_state_complain(mcstate_t *state, mcastlocation_t pos, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    mc_state_complainv(state, pos, fmt, va);
    va_end(va);
}

void mc_state_setruntimeerrorf(mcstate_t* state, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    mc_state_pusherrorfv(state, MC_ERROR_RUNTIME, srcposinvalid, fmt, va);
    va_end(va);
}


bool mc_value_isnumeric(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return type == MC_VAL_NUMBER || type == MC_VAL_BOOL;
}

bool mc_value_isnull(mcvalue_t obj)
{
    return mc_value_gettype(obj) == MC_VAL_NULL;
}

bool mc_value_iscallable(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return type == MC_VAL_FUNCNATIVE || type == MC_VAL_FUNCSCRIPT;
}

const char* mc_util_objtypename(mcvaltype_t type)
{
    switch(type)
    {
        case MC_VAL_NONE:
            return "NONE";
        case MC_VAL_ERROR:
            return "ERROR";
        case MC_VAL_NUMBER:
            return "NUMBER";
        case MC_VAL_BOOL:
            return "BOOL";
        case MC_VAL_STRING:
            return "STRING";
        case MC_VAL_NULL:
            return "NULL";
        case MC_VAL_FUNCNATIVE:
            return "NATIVE_FUNCTION";
        case MC_VAL_ARRAY:
            return "ARRAY";
        case MC_VAL_MAP:
            return "MAP";
        case MC_VAL_FUNCSCRIPT:
            return "FUNCTION";
        case MC_VAL_EXTERNAL:
            return "EXTERNAL";
        case MC_VAL_FREED:
            return "FREED";
        case MC_VAL_ANY:
            return "ANY";
        default:
            break;
    }
    return "NONE";
}

const char* mc_error_getmessage(mcerror_t* error)
{
    return error->message;
}

const char* mc_error_getfilepath(mcerror_t* error)
{
    if(!error->pos.file)
    {
        return NULL;
    }
    return error->pos.file->path;
}

const char* mc_error_getsourcelinecode(mcerror_t* error)
{
    const char* line;
    mcptrlist_t* lines;
    if(!error->pos.file)
    {
        return NULL;
    }
    lines = error->pos.file->lines;
    if(error->pos.line >= (int)mc_ptrlist_count(lines))
    {
        return NULL;
    }
    line = (const char*)mc_ptrlist_get(lines, error->pos.line);
    return line;
}

int mc_error_getsourcelinenumber(mcerror_t* error)
{
    if(error->pos.line < 0)
    {
        return -1;
    }
    return error->pos.line + 1;
}

int mc_error_getsourcecolumn(mcerror_t* error)
{
    if(error->pos.column < 0)
    {
        return -1;
    }
    return error->pos.column + 1;
}

mcerrtype_t mc_error_gettype(mcerror_t* error)
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
            break;
    }
    return MC_ERROR_NONE;
}

const char* mc_error_gettypestring(mcerror_t* error)
{
    return mc_util_errortypename(mc_error_gettype(error));
}

const char* mc_util_errortypename(mcerrtype_t type)
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
            break;
    }
    return "NONE";
}

char* mc_error_serializetostring(mcstate_t* state, mcerror_t* err)
{
    int j;
    int colnum;
    int linenum;
    const char* line;
    const char* typestr;
    const char* filename;
    mcprinter_t* pr;
    mctraceback_t* traceback;
    typestr = mc_error_gettypestring(err);
    filename = mc_error_getfilepath(err);
    line = mc_error_getsourcelinecode(err);
    linenum = mc_error_getsourcelinenumber(err);
    colnum = mc_error_getsourcecolumn(err);
    pr = mc_printer_make(state, NULL);
    if(!pr)
    {
        return NULL;
    }
    if(line)
    {
        mc_printer_puts(pr, line);
        mc_printer_puts(pr, "\n");
        if(colnum >= 0)
        {
            for(j = 0; j < (colnum - 1); j++)
            {
                mc_printer_puts(pr, " ");
            }
            mc_printer_puts(pr, "^\n");
        }
    }
    mc_printer_printf(pr, "%s ERROR in \"%s\" on %d:%d: %s\n", typestr, filename, linenum, colnum, mc_error_getmessage(err));
    traceback = mc_error_gettraceback(err);
    if(traceback)
    {
        mc_printer_printf(pr, "Traceback:\n");
        mc_printer_printtraceback(pr, (mctraceback_t*)mc_error_gettraceback(err));
    }
    if(mc_printer_failed(pr))
    {
        mc_printer_destroy(pr);
        return NULL;
    }
    return mc_printer_getstringanddestroy(pr, NULL);
}

mctraceback_t* mc_error_gettraceback(mcerror_t* error)
{
    return (mctraceback_t*)error->traceback;
}

int mc_traceback_getdepth(mctraceback_t* traceback)
{
    return mc_basicarray_count(traceback->items);
}

const char* mc_traceback_getsourcefilepath(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)mc_basicarray_get(traceback->items, depth);
    if(!item)
    {
        return NULL;
    }
    return mc_traceitem_getsourcefilepath(item);
}

const char* mc_traceback_getsourcelinecode(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)mc_basicarray_get(traceback->items, depth);
    if(!item)
    {
        return NULL;
    }
    return mc_traceitem_getsourceline(item);
}

int mc_traceback_getsourcelinenumber(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)mc_basicarray_get(traceback->items, depth);
    if(!item)
    {
        return -1;
    }
    return item->pos.line;
}

int mc_traceback_getsourcecolumn(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)mc_basicarray_get(traceback->items, depth);
    if(!item)
    {
        return -1;
    }
    return item->pos.column;
}

const char* mc_traceback_getfunctionname(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)mc_basicarray_get(traceback->items, depth);
    if(!item)
    {
        return "";
    }
    return item->trfuncname;
}


#include "astprint.h"

const char* mc_util_mathopstring(mcastmathoptype_t op)
{
    switch(op)
    {
        case MC_MATHOP_NONE:
            return "MC_MATHOP_NONE";
        case MC_MATHOP_ASSIGN:
            return "=";
        case MC_MATHOP_PLUS:
            return "+";
        case MC_MATHOP_MINUS:
            return "-";
        case MC_MATHOP_BANG:
            return "!";
        case MC_MATHOP_ASTERISK:
            return "*";
        case MC_MATHOP_SLASH:
            return "/";
        case MC_MATHOP_LT:
            return "<";
        case MC_MATHOP_GT:
            return ">";
        case MC_MATHOP_EQ:
            return "==";
        case MC_MATHOP_NOTEQ:
            return "!=";
        case MC_MATHOP_MODULUS:
            return "%";
        case MC_MATHOP_LOGICALAND:
            return "&&";
        case MC_MATHOP_LOGICALOR:
            return "||";
        case MC_MATHOP_BINAND:
            return "&";
        case MC_MATHOP_BINOR:
            return "|";
        case MC_MATHOP_BINXOR:
            return "^";
        case MC_MATHOP_LSHIFT:
            return "<<";
        case MC_MATHOP_RSHIFT:
            return ">>";
        default:
            break;
    }
    return "MC_MATHOP_UNKNOWN";
}

#if 1
    bool mc_argcheck_check(mcstate_t* state, bool generateerror, int argc, mcvalue_t* args, ...)
    {
        (void)state;
        (void)generateerror;
        (void)argc;
        (void)args;
        return true;
    }
#else
    #if defined(__cplusplus) || 1
        #define mc_argcheck_check(state, generateerror, argc, args, ...) true
    #else
        #define mc_argcheck_check(state, generateerror, argc, args, ...) \
            mc_argcheck_checkactual((state), (generateerror), (argc), (args), sizeof((mcvaltype_t[]){ __VA_ARGS__ }) / sizeof(mcvaltype_t), (mcvaltype_t[]){ __VA_ARGS__ })
    #endif
#endif

bool mc_argcheck_checkactual(mcstate_t* state, bool generateerror, int argc, mcvalue_t* args, int expectedargc, const mcvaltype_t* expectedtypes)
{
    int i;
    char* expectedtypestr;
    const char* typestr;
    mcvaltype_t type;
    mcvaltype_t expectedtype;
    mcvalue_t arg;
    if(argc != expectedargc)
    {
        if(generateerror)
        {
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, srcposinvalid, "Invalid number or arguments, got %d instead of %d", argc, expectedargc);
        }
        return false;
    }
    for(i = 0; i < argc; i++)
    {
        arg = args[i];
        type = mc_value_gettype(arg);
        expectedtype = expectedtypes[i];
        if(!(type & expectedtype))
        {
            if(generateerror)
            {
                typestr = mc_valtype_getname(type);
                expectedtypestr = mc_valtype_getunionname(state, expectedtype);
                if(!expectedtypestr)
                {
                    return false;
                }
                mc_state_pusherrorf(state, MC_ERROR_RUNTIME, srcposinvalid, "Invalid argument %d type, got %s, expected %s", i, typestr, expectedtypestr);
                mc_allocator_free(state, expectedtypestr);
            }
            return false;
        }
    }
    return true;
}


mcvalue_t mc_scriptfn_binnot(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcfloat_t dn;
    int64_t iv;
    (void)state;
    (void)data;
    (void)argc;
    dn = args[0].uval.valnumber;
    iv = (int64_t)dn;
    iv = ~iv;
    return mc_value_makenumber(iv);
}

mcvalue_t mc_scriptfn_ord(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    char ch;
    size_t len;
    const char* str;
    mcvalue_t sval;
    (void)state;
    (void)data;
    (void)argc;
    sval = args[0];
    str = mc_valstring_getdata(sval);
    len = mc_valstring_getlength(sval);
    ch = 0;
    if(len > 0)
    {
        ch = str[0];
    }
    return mc_value_makenumber(ch);
}

mcvalue_t mc_scriptfn_arrayjoin(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool havejoinee;
    int i;
    int slen;
    int alen;
    const char* str;
    mcvalue_t rt;
    mcvalue_t item;
    mcvalue_t array;
    mcvalue_t joinee;
    mcprinter_t pr;
    (void)state;
    (void)argc;
    (void)data;
    havejoinee = false;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    array= args[0];
    if(argc > 1)
    {
        havejoinee = true;
        joinee = args[1];
    }
    alen = mc_valarray_getlength(array);
    mc_printer_init(&pr, state, alen, NULL, true);
    for(i=0; i<alen; i++)
    {
        item = mc_valarray_getvalueat(array, i);
        mc_printer_printvalue(&pr, item);
        if(havejoinee)
        {
            if((i + 1) != alen)
            {
                mc_printer_printvalue(&pr, joinee);
            }
        }
    }
    str = pr.data;
    slen = pr.len;
    rt = mc_value_makestringlen(state, str, slen);
    mc_printer_release(&pr);
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
mcvalue_t mc_scriptfn_index(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int inplen;
    int searchlen;
    int startindex;
    char* result;
    const char* inpstr;
    const char* searchstr;
    mcvalue_t startval;
    mcvalue_t inpval;
    mcvalue_t searchval;
    (void)state;
    (void)data;
    (void)inplen;
    (void)searchlen;
    startindex = 0;
    if(argc == 3 && args[2].type == MC_VAL_NUMBER)
    {
        startval = args[2];
        startindex = mc_value_getnumber(startval);
    }
    if((argc == 2 || argc == 3) && args[0].type == MC_VAL_STRING && args[1].type == MC_VAL_STRING)
    {
        inpval = args[0];
        searchval = args[1];
        inpstr = mc_valstring_getdata(inpval);
        searchstr = mc_valstring_getdata(searchval);
        inplen = mc_valstring_getlength(inpval);
        searchlen = mc_valstring_getlength(searchval);
        result = (char*)strstr(inpstr + startindex, searchstr);
        if(result == NULL)
        {
            return mc_value_makenumber(-1);
        }
        return mc_value_makenumber(result - inpstr);
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
mcvalue_t mc_scriptfn_left(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int inplen;
    int startpos;
    char* result;
    const char* inpstr;
    mcvalue_t obj;
    mcvalue_t inpval;
    mcvalue_t posval;
    (void)data;
    if(argc == 2 && args[0].type == MC_VAL_STRING && args[1].type == MC_VAL_NUMBER)
    {
        inpval = args[0];
        posval = args[1];
        inpstr = mc_valstring_getdata(inpval);
        inplen = mc_valstring_getlength(inpval);
        startpos = mc_value_getnumber(posval);
        /*
        * If the requested startpos is longer than the string then return a new string
        * of the full length.
        */
        if(startpos > (int)inplen)
        {
            return mc_value_makestringlen(state, inpstr, inplen);
        }
        result = (char*)mc_memory_malloc(startpos + 1);
        if(result == NULL)
        {
            return mc_value_makenull();
        }
        strncpy(result, inpstr, startpos);
        result[startpos] = '\0';
        obj = mc_value_makestringlen(state, result, startpos);
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
mcvalue_t mc_scriptfn_right(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int inplen;
    int startpos;
    int strlength;
    char* result;
    const char* inpstr;
    mcvalue_t obj;
    mcvalue_t inpval;
    mcvalue_t idxval;
    (void)data;
    if(argc == 2 && args[0].type == MC_VAL_STRING && args[1].type == MC_VAL_NUMBER)
    {
        inpval = args[0];
        idxval = args[1];
        inpstr = mc_valstring_getdata(inpval);
        inplen = mc_valstring_getlength(inpval);
        startpos = mc_value_getnumber(idxval);
        /*
        * If the requested startpos is longer than the string then return a new string
        * of the full length.
        */
        if(startpos >= inplen)
        {
            return mc_value_makestringlen(state, inpstr, inplen);
        }
        result = (char*)mc_memory_malloc(startpos + 1);
        if(result == NULL)
        {
            return mc_value_makenull();
        }
        strlength = inplen;
        strncpy(result, inpstr + strlength - startpos, startpos);
        result[startpos] = '\0';
        obj = mc_value_makestringlen(state, result, startpos);
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
mcvalue_t mc_scriptfn_replace(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    size_t len;
    size_t newlen;
    size_t inplen;
    size_t searchlen;
    size_t replacelen;
    size_t count;
    char* ptr;
    char* result;
    const char* temp;
    const char* inpstr;
    const char* searchstr;
    const char* replacestr;
    mcvalue_t obj;
    mcvalue_t inpval;
    mcvalue_t searchval;
    mcvalue_t repval;
    (void)data;
    if(argc == 3 && args[0].type == MC_VAL_STRING && args[1].type == MC_VAL_STRING && args[2].type == MC_VAL_STRING)
    {
        inpval = args[0];
        searchval = args[1];
        repval = args[2];
        inpstr = mc_valstring_getdata(inpval);
        searchstr = mc_valstring_getdata(searchval);
        replacestr = mc_valstring_getdata(repval);
        inplen = mc_valstring_getlength(inpval);
        searchlen = mc_valstring_getlength(searchval);
        replacelen = mc_valstring_getlength(repval);
        count = 0;
        temp = inpstr;
        /* Count number of occurrences of searchstr in inpstr */
        while((temp = strstr(temp, searchstr)))
        {
            count++;
            temp += searchlen;
        }
        /* Allocate new string to store result */
        newlen = inplen + count * (replacelen - searchlen) + 1;
        result = (char*)mc_memory_malloc(newlen);
        if(result == NULL)
        {
            return mc_value_makenull();
        }
        /* Replace all instances of searchstr with replacestr */
        ptr = result;
        while((temp = strstr(inpstr, searchstr)))
        {
            len = temp - inpstr;
            memcpy(ptr, inpstr, len);
            ptr += len;
            memcpy(ptr, replacestr, replacelen);
            ptr += replacelen;
            inpstr = temp + searchlen;
        }
        /* Copy remaining part of inpstr */
        strcpy(ptr, inpstr);
        obj = mc_value_makestring(state, result);
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
mcvalue_t mc_scriptfn_replacefirst(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    size_t len;
    size_t newlen;
    size_t inplen;
    size_t searchlen;
    size_t replacelen;
    const char* inpstr;
    const char* temp;
    const char* searchstr;
    const char* replacestr;
    char* result;
    mcvalue_t obj;
    mcvalue_t inpval;
    mcvalue_t repval;
    mcvalue_t searchval;
    (void)data;
    if(argc == 3 && args[0].type == MC_VAL_STRING && args[1].type == MC_VAL_STRING && args[2].type == MC_VAL_STRING)
    {
        inpval = args[0];
        searchval = args[1];
        repval = args[2];
        inpstr = mc_valstring_getdata(inpval);
        searchstr = mc_valstring_getdata(searchval);
        replacestr = mc_valstring_getdata(repval);
        inplen = mc_valstring_getlength(inpval);
        searchlen = mc_valstring_getlength(searchval);
        replacelen = mc_valstring_getlength(repval);
        temp = strstr(inpstr, searchstr);
        if(temp == NULL)
        {
            return mc_value_makestringlen(state, inpstr, inplen);
        }
        /* Allocate new string to store result */
        newlen = inplen + (replacelen - searchlen) + 1;
        result = (char*)mc_memory_malloc(newlen + 1);
        if(result == NULL)
        {
            return mc_value_makenull();
        }
        /* Replace the first instance of searchstr with replacestr */
        len = temp - inpstr;
        memcpy(result, inpstr, len);
        strcpy(result + len, replacestr);
        strcpy(result + len + replacelen, temp + searchlen);
        obj = mc_value_makestringlen(state, result, len);
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
mcvalue_t mc_scriptfn_trim(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int i;
    int j;
    int k;
    int inplen;
    char* result;
    const char* inpstr;
    mcvalue_t obj;
    mcvalue_t inpval;
    (void)data;
    if(argc == 1 && args[0].type == MC_VAL_STRING)
    {
        inpval = args[0];
        inpstr = mc_valstring_getdata(inpval);
        inplen = mc_valstring_getlength(inpval);
        if(inplen == 0)
        {
            return mc_value_makestringlen(state, "", 0);
        }
        result = (char*)mc_memory_malloc(inplen + 1);
        if(result == NULL)
        {
            return mc_value_makestringlen(state, "", 0);
        }
        strncpy(result, inpstr, inplen);
        result[inplen] = '\0';
        i = 0;
        j = inplen - 1;
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
        k = 0;
        while(i <= j)
        {
            result[k] = result[i];
            k++;
            i++;
        }
        result[k] = '\0';
        obj = mc_value_makestringlen(state, result, k);
        free(result);
        return obj;
    }
    return mc_value_makenull();
}

mcvalue_t mc_scriptfn_lengthof(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int len;
    mcvalue_t arg;
    mcvaltype_t type;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_STRING | MC_VAL_ARRAY | MC_VAL_MAP))
    {
        return mc_value_makenull();
    }
    arg = args[0];
    type = arg.type;
    if(type == MC_VAL_STRING)
    {
        len = mc_valstring_getlength(arg);
        return mc_value_makenumber(len);
    }
    if(type == MC_VAL_ARRAY)
    {
        len = mc_valarray_getlength(arg);
        return mc_value_makenumber(len);
    }
    if(type == MC_VAL_MAP)
    {
        len = mc_valmap_getlength(arg);
        return mc_value_makenumber(len);
    }
    return mc_value_makenull();
}


mcvalue_t mc_scriptfn_typeof(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcvalue_t arg;
    mcvaltype_t type;
    const char* ts;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    arg = args[0];
    type = arg.type;
    ts = mc_valtype_getname(type);
    return mc_value_makestring(state, ts);
}

mcvalue_t mc_scriptfn_arrayfirst(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcvalue_t arg;
    (void)state;
    (void)data;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY))
    {
        return mc_value_makenull();
    }
    arg = args[0];
    return mc_valarray_getvalueat(arg, 0);
}

mcvalue_t mc_scriptfn_arraylast(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int len;
    mcvalue_t arg;
    (void)state;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY))
    {
        return mc_value_makenull();
    }
    arg = args[0];
    len = mc_valarray_getlength(arg);
    return mc_valarray_getvalueat(arg, len - 1);
}

mcvalue_t mc_scriptfn_arrayrest(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    int len;
    mcvalue_t arg;
    mcvalue_t res;
    mcvalue_t item;
    (void)state;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY))
    {
        return mc_value_makenull();
    }
    arg = args[0];
    len = mc_valarray_getlength(arg);
    if(len == 0)
    {
        return mc_value_makenull();
    }
    res = mc_value_makearray(state);
    if(mc_value_isnull(res))
    {
        return mc_value_makenull();
    }
    for(i = 1; i < len; i++)
    {
        item = mc_valarray_getvalueat(arg, i);
        ok = mc_valarray_push(res, item);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

mcvalue_t mc_scriptfn_reverse(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    int inplen;
    char* resbuf;
    const char* inpstr;
    mcvaltype_t type;
    mcvalue_t arg;
    mcvalue_t obj;
    mcvalue_t res;
    (void)state;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY | MC_VAL_STRING))
    {
        return mc_value_makenull();
    }
    arg = args[0];
    type = arg.type;
    if(type == MC_VAL_ARRAY)
    {
        inplen = mc_valarray_getlength(arg);
        res = mc_value_makearraycapacity(state, inplen);
        if(mc_value_isnull(res))
        {
            return mc_value_makenull();
        }
        for(i = 0; i < inplen; i++)
        {
            obj = mc_valarray_getvalueat(arg, i);
            ok = mc_valarray_setvalueat(res, inplen - i - 1, obj);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return res;
    }
    if(type == MC_VAL_STRING)
    {
        inpstr = mc_valstring_getdata(arg);
        inplen = mc_valstring_getlength(arg);
        res = mc_value_makestrcapacity(state, inplen);
        if(mc_value_isnull(res))
        {
            return mc_value_makenull();
        }
        resbuf = mc_valstring_getmutabledata(res);
        for(i = 0; i < inplen; i++)
        {
            resbuf[inplen - i - 1] = inpstr[i];
        }
        resbuf[inplen] = '\0';
        mc_string_setlength(res, inplen);
        return res;
    }
    return mc_value_makenull();
}

mcvalue_t mc_scriptfn_array(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    int capacity;
    mcvalue_t res;
    mcvalue_t objnull;
    (void)state;
    (void)argc;
    (void)data;
    if(argc == 1)
    {
        if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
        {
            return mc_value_makenull();
        }
        capacity = (int)mc_value_getnumber(args[0]);
        res = mc_value_makearraycapacity(state, capacity);
        if(mc_value_isnull(res))
        {
            return mc_value_makenull();
        }
        objnull = mc_value_makenull();
        for(i = 0; i < capacity; i++)
        {
            ok = mc_valarray_push(res, objnull);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return res;
    }
    if(argc == 2)
    {
        if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER, MC_VAL_ANY))
        {
            return mc_value_makenull();
        }
        capacity = (int)mc_value_getnumber(args[0]);
        res = mc_value_makearraycapacity(state, capacity);
        if(mc_value_isnull(res))
        {
            return mc_value_makenull();
        }
        for(i = 0; i < capacity; i++)
        {
            ok = mc_valarray_push(res, args[1]);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return res;
    }
    mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER);
    return mc_value_makenull();
}

mcvalue_t mc_scriptfn_arraypush(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    int len;
    (void)state;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    for(i=1; i<argc; i++)
    {
        ok = mc_valarray_push(args[0], args[i]);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    len = mc_valarray_getlength(args[0]);
    return mc_value_makenumber(len);
}

mcvalue_t mc_scriptfn_arraypop(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcvalue_t val;
    (void)state;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    val = mc_valarray_pop(args[0]);
    return val;
}

mcvalue_t mc_scriptfn_externalfn(mcstate_t* state, void *data, int argc, mcvalue_t *args)
{
    int *test;
    (void)state;
    (void)argc;
    (void)args;
    test = (int*)data;
    *test = 42;
    return mc_value_makenull();
}

mcvalue_t mc_scriptfn_vec2add(mcstate_t *state, void *data, int argc, mcvalue_t *args)
{
    mcfloat_t a_x;
    mcfloat_t a_y;
    mcfloat_t b_x;
    mcfloat_t b_y;
    mcvalue_t res;
    mcvalue_t keyx;
    mcvalue_t keyy;
    (void)state;
    (void)argc;
    (void)data;
    if (!mc_argcheck_check(state, true, argc, args, MC_VAL_MAP, MC_VAL_MAP))
    {
        return mc_value_makenull();
    }
    keyx = mc_value_makestring(state, "x");
    keyy = mc_value_makestring(state, "y");
    a_x = mc_value_getnumber(mc_valmap_getvalue(args[0], keyx));
    a_y = mc_value_getnumber(mc_valmap_getvalue(args[0], keyy));
    b_x = mc_value_getnumber(mc_valmap_getvalue(args[1], keyx));
    b_y = mc_value_getnumber(mc_valmap_getvalue(args[1], keyy));
    res = mc_value_makemap(state);
    if (mc_value_gettype(res) == MC_VAL_NULL)
    {
        return res;
    }
    mc_valmap_setvalue(res, keyx, mc_value_makenumber(a_x + b_x));
    mc_valmap_setvalue(res, keyy, mc_value_makenumber(a_y + b_y));
    return res;
}

mcvalue_t mc_scriptfn_vec2sub(mcstate_t *state, void *data, int argc, mcvalue_t *args)
{
    mcfloat_t a_x;
    mcfloat_t a_y;
    mcfloat_t b_y;
    mcfloat_t b_x;
    mcvalue_t res;
    mcvalue_t keyx;
    mcvalue_t keyy;
    (void)state;
    (void)argc;
    (void)data;
    if (!mc_argcheck_check(state, true, argc, args, MC_VAL_MAP, MC_VAL_MAP))
    {
        return mc_value_makenull();
    }
    keyx = mc_value_makestring(state, "x");
    keyy = mc_value_makestring(state, "y");
    a_x = mc_value_getnumber(mc_valmap_getvalue(args[0], keyx));
    a_y = mc_value_getnumber(mc_valmap_getvalue(args[0], keyy));
    b_x = mc_value_getnumber(mc_valmap_getvalue(args[1], keyx));
    b_y = mc_value_getnumber(mc_valmap_getvalue(args[1], keyy));
    res = mc_value_makemap(state);
    mc_valmap_setvalue(res, keyx, mc_value_makenumber(a_x - b_x));
    mc_valmap_setvalue(res, keyy, mc_value_makenumber(a_y - b_y));
    return res;
}

mcvalue_t mc_scriptfn_testcheckargs(mcstate_t* state, void *data, int argc, mcvalue_t *args)
{
    (void)state;
    (void)args;
    (void)argc;
    (void)data;
    if (!mc_argcheck_check(state, true, argc, args,
                  MC_VAL_NUMBER,
                  MC_VAL_ARRAY | MC_VAL_MAP,
                  MC_VAL_MAP,
                  MC_VAL_STRING,
                  MC_VAL_NUMBER | MC_VAL_BOOL,
                  MC_VAL_FUNCSCRIPT | MC_VAL_FUNCNATIVE,
                  MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makenumber(42);
}


mcvalue_t mc_scriptfn_maketestdict(mcstate_t *state, void *data, int argc, mcvalue_t *args)
{
    int i;
    int blen;
    int numitems;
    mcvalue_t res;
    mcvalue_t key;
    mcvalue_t val;
    const char *tname;
    char keybuf[64];
    (void)data;
    if (argc != 1)
    {
        mc_state_setruntimeerrorf(state, "Invalid type passed to maketestdict, got %d, expected 1", argc);
        return mc_value_makenull();
    }    
    if (args[0].type != MC_VAL_NUMBER)
    {
        tname = mc_util_objtypename(args[0].type);
        mc_state_setruntimeerrorf(state, "Invalid type passed to maketestdict, got %s", tname);
        return mc_value_makenull();
    }
    numitems = mc_value_getnumber(args[0]);
    res = mc_value_makemap(state);
    if (res.type == MC_VAL_NULL)
    {
        return mc_value_makenull();
    }
    for (i = 0; i < numitems; i++)
    {
        blen = sprintf(keybuf, "%d", i);
        key = mc_value_makestringlen(state, keybuf, blen);
        val = mc_value_makenumber(i);
        mc_valmap_setvalue(res, key, val);
    }
    return res;
}

mcvalue_t mc_scriptfn_squarearray(mcstate_t *state, void *data, int argc, mcvalue_t *args)
{
    int i;
    mcfloat_t num;
    mcvalue_t res;
    mcvalue_t resitem;    
    (void)data;
    res = mc_value_makearraycapacity(state, argc);
    for(i = 0; i < argc; i++)
    {
        if(mc_value_gettype(args[i]) != MC_VAL_NUMBER)
        {
            mc_state_setruntimeerrorf(state, "Invalid type passed to squarearray");
            return mc_value_makenull();
        }
        num = mc_value_getnumber(args[i]);
        resitem = mc_value_makenumber(num * num);
        mc_valarray_push(res, resitem);
    }
    return res;
}

mcvalue_t mc_scriptfn_print(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int i;
    mcvalue_t arg;
    mcprinter_t* pr;
    (void)data;
    pr = state->stdoutprinter;
    for(i = 0; i < argc; i++)
    {
        arg = args[i];
        mc_printer_printvalue(pr, arg);
    }
    return mc_value_makenull();
}

mcvalue_t mc_scriptfn_println(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcvalue_t o;
    o = mc_scriptfn_print(state, data, argc, args);
    mc_printer_putchar(state->stdoutprinter, '\n');
    return o;
}

mcvalue_t mc_scriptfn_filewritefile(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int slen;
    int written;
    const char* path;
    const char* string;
    (void)state;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_STRING, MC_VAL_STRING))
    {
        return mc_value_makenull();
    }
    path = mc_valstring_getdata(args[0]);
    string = mc_valstring_getdata(args[1]);
    slen = mc_valstring_getlength(args[1]);
    written = mc_fsutil_filewrite(state, path, string, slen);
    return mc_value_makenumber(written);
}

mcvalue_t mc_scriptfn_filereadfile(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    size_t flen;
    char* contents;
    const char* path;
    mcvalue_t res;
    (void)state;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_STRING))
    {
        return mc_value_makenull();
    }
    path = mc_valstring_getdata(args[0]);
    contents = mc_fsutil_fileread(state, path, &flen);
    if(!contents)
    {
        return mc_value_makenull();
    }
    res = mc_value_makestringlen(state, contents, flen);
    mc_allocator_free(state, contents);
    return res;
}

mcvalue_t mc_scriptfn_tostring(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int reslen;
    const char* resstr;
    mcvalue_t arg;
    mcvalue_t res;
    mcprinter_t pr;
    (void)state;
    (void)argc;
    (void)data;
    arg = args[0];
    mc_printer_init(&pr, state, 5, NULL, true);
    mc_printer_printvalue(&pr, arg);
    if(mc_printer_failed(&pr))
    {
        mc_printer_release(&pr);
        return mc_value_makenull();
    }
    resstr = mc_printer_getstring(&pr);
    reslen = mc_printer_getlength(&pr);
    res = mc_value_makestringlen(state, resstr, reslen);
    mc_printer_release(&pr);
    return res;
}

mcvalue_t mc_scriptfn_jsonstringify(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int reslen;
    const char* resstr;
    mcvalue_t arg;
    mcvalue_t res;
    mcprinter_t pr;
    (void)state;
    (void)argc;
    (void)data;
    arg = args[0];
    mc_printer_init(&pr, state, 5, NULL, true);
    pr.config.verbosefunc = false;
    pr.config.quotstring = true;
    mc_printer_printvalue(&pr, arg);
    if(mc_printer_failed(&pr))
    {
        mc_printer_release(&pr);
        return mc_value_makenull();
    }
    resstr = mc_printer_getstring(&pr);
    reslen = mc_printer_getlength(&pr);
    res = mc_value_makestringlen(state, resstr, reslen);
    mc_printer_release(&pr);
    return res;
}

mcvalue_t mc_scriptfn_tonum(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int stringlen;
    int parsedlen;
    mcfloat_t result;
    char* end;
    const char* string;
    (void)state;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_STRING | MC_VAL_NUMBER | MC_VAL_BOOL | MC_VAL_NULL))
    {
        return mc_value_makenull();
    }
    result = 0;
    string = "";
    if(mc_value_isnumeric(args[0]))
    {
        result = mc_value_getnumber(args[0]);
    }
    else if(mc_value_isnull(args[0]))
    {
        result = 0;
    }
    else if(args[0].type == MC_VAL_STRING)
    {
        stringlen = mc_valstring_getlength(args[0]);
        string = mc_valstring_getdata(args[0]);
        errno = 0;
        result = mc_util_strtod(string, stringlen, &end);
        if(errno == ERANGE && (result <= -HUGE_VAL || result >= HUGE_VAL))
        {
            goto err;
        }
        if(errno && errno != ERANGE)
        {
            goto err;
        }
        parsedlen = end - string;
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
    mc_state_pusherrorf(state, MC_ERROR_RUNTIME, srcposinvalid, "Cannot convert \"%s\" to number", string);
    return mc_value_makenull();
}


mcvalue_t mc_scriptfn_isnan(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcfloat_t val;
    bool b;
    (void)state;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    val = mc_value_getnumber(args[0]);
    b = false;
    if(val != val)
    {
        b = true;
    }
    return mc_value_makebool(b);
}

mcvalue_t mc_scriptfn_chr(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcfloat_t val;
    char c;
    (void)state;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    val = mc_value_getnumber(args[0]);
    c = (char)val;
    return mc_value_makestringlen(state, &c, 1);
}

mcvalue_t mc_scriptfn_range(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    int start;
    int end;
    int step;
    const char* typestr;
    const char* expectedstr;
    mcvaltype_t type;
    mcvalue_t res;
    mcvalue_t item;
    (void)data;
    for(i = 0; i < argc; i++)
    {
        type = args[i].type;
        if(type != MC_VAL_NUMBER)
        {
            typestr = mc_valtype_getname(type);
            expectedstr = mc_valtype_getname(MC_VAL_NUMBER);
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, srcposinvalid, "Invalid argument %d passed to range, got %s instead of %s", i, typestr, expectedstr);
            return mc_value_makenull();
        }
    }
    start = 0;
    end = 0;
    step = 1;
    if(argc == 1)
    {
        end = (int)mc_value_getnumber(args[0]);
    }
    else if(argc == 2)
    {
        start = (int)mc_value_getnumber(args[0]);
        end = (int)mc_value_getnumber(args[1]);
    }
    else if(argc == 3)
    {
        start = (int)mc_value_getnumber(args[0]);
        end = (int)mc_value_getnumber(args[1]);
        step = (int)mc_value_getnumber(args[2]);
    }
    else
    {
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, srcposinvalid, "Invalid number of arguments passed to range, got %d", argc);
        return mc_value_makenull();
    }
    if(step == 0)
    {
        mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "range step cannot be 0");
        return mc_value_makenull();
    }
    res = mc_value_makearray(state);
    if(mc_value_isnull(res))
    {
        return mc_value_makenull();
    }
    for(i = start; i < end; i += step)
    {
        item = mc_value_makenumber(i);
        ok = mc_valarray_push(res, item);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

mcvalue_t mc_scriptfn_keys(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    int len;
    mcvalue_t arg;
    mcvalue_t res;
    mcvalue_t key;
    (void)state;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_MAP))
    {
        return mc_value_makenull();
    }
    arg = args[0];
    res = mc_value_makearray(state);
    if(mc_value_isnull(res))
    {
        return mc_value_makenull();
    }
    len = mc_valmap_getlength(arg);
    for(i = 0; i < len; i++)
    {
        key = mc_valmap_getkeyat(arg, i);
        ok = mc_valarray_push(res, key);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

mcvalue_t mc_scriptfn_values(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    int len;
    mcvalue_t key;
    mcvalue_t arg;
    mcvalue_t res;
    (void)state;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_MAP))
    {
        return mc_value_makenull();
    }
    arg = args[0];
    res = mc_value_makearray(state);
    if(mc_value_isnull(res))
    {
        return mc_value_makenull();
    }
    len = mc_valmap_getlength(arg);
    for(i = 0; i < len; i++)
    {
        key = mc_valmap_getvalueat(arg, i);
        ok = mc_valarray_push(res, key);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

mcvalue_t mc_scriptfn_copy(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)state;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_copyflat(state, args[0]);
}

mcvalue_t mc_scriptfn_copydeep(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_copydeep(state, args[0]);
}

mcvalue_t mc_scriptfn_remove(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool res;
    int i;
    int ix;
    mcvalue_t obj;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    ix = -1;
    for(i = 0; i < mc_valarray_getlength(args[0]); i++)
    {
        obj = mc_valarray_getvalueat(args[0], i);
        if(mc_value_equals(obj, args[1]))
        {
            ix = i;
            break;
        }
    }
    if(ix == -1)
    {
        return mc_value_makebool(false);
    }
    res = mc_valarray_removevalueat(args[0], ix);
    return mc_value_makebool(res);
}

mcvalue_t mc_scriptfn_removeat(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool res;
    int ix;
    mcvaltype_t type;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    type= args[0].type;
    ix = (int)mc_value_getnumber(args[1]);
    switch(type)
    {
        case MC_VAL_ARRAY:
            {
                res = mc_valarray_removevalueat(args[0], ix);
                return mc_value_makebool(res);
            }
            break;
        default:
            {
            }
            break;
    }
    return mc_value_makebool(true);
}

mcvalue_t mc_scriptfn_error(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(argc == 1 && args[0].type == MC_VAL_STRING)
    {
        return mc_value_makeerror(state, mc_valstring_getdata(args[0]));
    }
    return mc_value_makeerror(state, "");
}

mcvalue_t mc_scriptfn_crash(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    if(argc == 1 && args[0].type == MC_VAL_STRING)
    {
        mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), mc_valstring_getdata(args[0]));
    }
    else
    {
        mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "");
    }
    return mc_value_makenull();
}

mcvalue_t mc_scriptfn_assert(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_BOOL))
    {
        return mc_value_makenull();
    }
    if(!mc_value_getbool(args[0]))
    {
        mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "assertion failed");
        return mc_value_makenull();
    }
    return mc_value_makebool(true);
}

mcvalue_t mc_scriptfn_randseed(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    int seed;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    seed = (int)mc_value_getnumber(args[0]);
    srand(seed);
    return mc_value_makebool(true);
}

mcvalue_t mc_scriptfn_random(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcfloat_t min;
    mcfloat_t max;
    mcfloat_t res;
    mcfloat_t range;
    (void)data;
    (void)state;
    (void)argc;
    res = (mcfloat_t)rand() / RAND_MAX;
    if(argc == 0)
    {
        return mc_value_makenumber(res);
    }
    if(argc == 2)
    {
        if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER, MC_VAL_NUMBER))
        {
            return mc_value_makenull();
        }
        min = mc_value_getnumber(args[0]);
        max = mc_value_getnumber(args[1]);
        if(min >= max)
        {
            mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "max is bigger than min");
            return mc_value_makenull();
        }
        range = max - min;
        res = min + (res * range);
        return mc_value_makenumber(res);
    }
    mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "Invalid number or arguments");
    return mc_value_makenull();
}

mcvalue_t mc_scriptfn_slice(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    int len;
    int index;
    char c;
    const char* str;
    const char* typestr;
    mcvalue_t res;
    mcvalue_t item;
    mcvaltype_t argtype;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_STRING | MC_VAL_ARRAY, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    argtype = args[0].type;
    index = (int)mc_value_getnumber(args[1]);
    if(argtype == MC_VAL_ARRAY)
    {
        len = mc_valarray_getlength(args[0]);
        if(index < 0)
        {
            index = len + index;
            if(index < 0)
            {
                index = 0;
            }
        }
        res = mc_value_makearraycapacity(state, len - index);
        if(mc_value_isnull(res))
        {
            return mc_value_makenull();
        }
        for(i = index; i < len; i++)
        {
            item = mc_valarray_getvalueat(args[0], i);
            ok = mc_valarray_push(res, item);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return res;
    }
    if(argtype == MC_VAL_STRING)
    {
        
        str = mc_valstring_getdata(args[0]);
        len = mc_valstring_getlength(args[0]);
        if(index < 0)
        {
            index = len + index;
            if(index < 0)
            {
                return mc_value_makestringlen(state, "", 0);
            }
        }
        if(index >= len)
        {
            return mc_value_makestringlen(state, "", 0);
        }
        res = mc_value_makestrcapacity(state, 0);
        if(mc_value_isnull(res))
        {
            return mc_value_makenull();
        }
        //bool mc_valstring_appendlen(mcvalue_t obj, const char *src, size_t len);
        for(i = index; i < len; i++)
        {
            c = str[i];
            mc_valstring_appendlen(res, &c, 1);
        }
        return res;
    }
    typestr = mc_valtype_getname(argtype);
    mc_state_pusherrorf(state, MC_ERROR_RUNTIME, srcposinvalid, "Invalid argument 0 passed to slice, got %s instead", typestr);
    return mc_value_makenull();
}

mcvalue_t mc_scriptfn_isstring(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == MC_VAL_STRING);
}

mcvalue_t mc_scriptfn_isarray(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == MC_VAL_ARRAY);
}

mcvalue_t mc_scriptfn_ismap(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == MC_VAL_MAP);
}

mcvalue_t mc_scriptfn_isnumber(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == MC_VAL_NUMBER);
}

mcvalue_t mc_scriptfn_isbool(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(args[0].type == MC_VAL_BOOL);
}

mcvalue_t mc_scriptfn_isnull(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(mc_value_gettype(args[0]) == MC_VAL_NULL);
}

mcvalue_t mc_scriptfn_isfunction(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(mc_value_gettype(args[0]) == MC_VAL_FUNCSCRIPT);
}

mcvalue_t mc_scriptfn_isexternal(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(mc_value_gettype(args[0]) == MC_VAL_EXTERNAL);
}

mcvalue_t mc_scriptfn_iserror(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(mc_value_gettype(args[0]) == MC_VAL_ERROR);
}

mcvalue_t mc_scriptfn_isnativefunction(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_makebool(mc_value_gettype(args[0]) == MC_VAL_FUNCNATIVE);
}

mcvalue_t mc_scriptfn_sqrt(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_getnumber(args[0]);
    res = sqrt(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_scriptfn_pow(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcfloat_t arg1;
    mcfloat_t arg2;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg1 = mc_value_getnumber(args[0]);
    arg2 = mc_value_getnumber(args[1]);
    res = pow(arg1, arg2);
    return mc_value_makenumber(res);
}

mcvalue_t mc_scriptfn_sin(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_getnumber(args[0]);
    res = sin(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_scriptfn_cos(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_getnumber(args[0]);
    res = cos(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_scriptfn_tan(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_getnumber(args[0]);
    res = tan(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_scriptfn_log(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_getnumber(args[0]);
    res = log(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_scriptfn_ceil(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_getnumber(args[0]);
    res = ceil(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_scriptfn_floor(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_getnumber(args[0]);
    res = floor(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_scriptfn_abs(mcstate_t* state, void* data, int argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_getnumber(args[0]);
    res = MC_UTIL_FABS(arg);
    return mc_value_makenumber(res);
}

mcptrlist_t* mc_util_splitstring(mcstate_t* state, const char* str, const char* delimiter)
{
    bool ok;
    size_t i;
    long len;
    char* rest;
    char* line;
    const char* lineend;
    const char* linestart;
    mcptrlist_t* res;
    res = mc_ptrlist_make(state, 0);
    rest = NULL;
    if(!str)
    {
        return res;
    }
    linestart = str;
    lineend = strstr(linestart, delimiter);
    while(lineend != NULL)
    {
        len = lineend - linestart;
        line = mc_util_strndup(state, linestart, len);
        if(!line)
        {
            goto err;
        }
        ok = mc_ptrlist_push(res, line);
        if(!ok)
        {
            mc_allocator_free(state, line);
            goto err;
        }
        linestart = lineend + 1;
        lineend = strstr(linestart, delimiter);
    }
    rest = mc_util_strdup(state, linestart);
    if(!rest)
    {
        goto err;
    }
    ok = mc_ptrlist_push(res, rest);
    if(!ok)
    {
        goto err;
    }
    return res;
err:
    mc_allocator_free(state, rest);
    if(res)
    {
        for(i = 0; i < mc_ptrlist_count(res); i++)
        {
            line = (char*)mc_ptrlist_get(res, i);
            mc_allocator_free(state, line);
        }
    }
    mc_ptrlist_destroy(res, NULL);
    return NULL;
}

char* mc_util_joinstringarray(mcstate_t* state, mcptrlist_t* items, const char* joinee, size_t jlen)
{
    size_t i;
    char* item;
    mcprinter_t* res;
    res = mc_printer_make(state, NULL);
    if(!res)
    {
        return NULL;
    }
    for(i = 0; i < mc_ptrlist_count(items); i++)
    {
        item = (char*)mc_ptrlist_get(items, i);
        mc_printer_puts(res, item);
        if(i < (mc_ptrlist_count(items) - 1))
        {
            mc_printer_putlen(res, joinee, jlen);
        }
    }
    return mc_printer_getstringanddestroy(res, NULL);
}

char* mc_util_canonpath(mcstate_t* state, const char* path)
{
    size_t i;
    char* joined;
    char* stritem;
    char* nextitem;
    void* item;
    const char* tmpstr;
    mcptrlist_t* split;
    if(!strchr(path, '/') || (!strstr(path, "/../") && !strstr(path, "./")))
    {
        return mc_util_strdup(state, path);
    }
    split = mc_util_splitstring(state, path, "/");
    if(!split)
    {
        return NULL;
    }
    for(i = 0; i < mc_ptrlist_count(split) - 1; i++)
    {
        stritem = (char*)mc_ptrlist_get(split, i);
        nextitem = (char*)mc_ptrlist_get(split, i + 1);
        if(mc_util_strequal(stritem, "."))
        {
            mc_allocator_free(state, stritem);
            mc_ptrlist_removeat(split, i);
            i = -1;
            continue;
        }
        if(mc_util_strequal(nextitem, ".."))
        {
            mc_allocator_free(state, stritem);
            mc_allocator_free(state, nextitem);
            mc_ptrlist_removeat(split, i);
            mc_ptrlist_removeat(split, i);
            i = -1;
        }
    }
    tmpstr = "/";
    joined = mc_util_joinstringarray(state, split, tmpstr, strlen(tmpstr));
    for(i = 0; i < mc_ptrlist_count(split); i++)
    {
        item = mc_ptrlist_get(split, i);
        mc_allocator_free(state, item);
    }
    mc_ptrlist_destroy(split, NULL);
    return joined;
}

bool mc_util_pathisabsolute(const char* path)
{
    return path[0] == '/';
}

bool mc_util_strequal(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
}

MCINLINE bool mc_callframe_init(mcvmframe_t* frame, mcvalue_t functionobj, int baseptr)
{
    mcobjfuncscript_t* function;
    if(mc_value_gettype(functionobj) != MC_VAL_FUNCSCRIPT)
    {
        return false;
    }
    function = mc_value_functiongetscriptfunction(functionobj);
    frame->function = functionobj;
    frame->bcposition = 0;
    frame->basepointer = baseptr;
    frame->sourcebcpos = 0;
    frame->bytecode = function->compiledprogcode->bytecode;
    frame->framesrcposlist = function->compiledprogcode->progsrcposlist;
    frame->bcsize = function->compiledprogcode->count;
    frame->recoverip = -1;
    frame->isrecovering = false;
    return true;
}

MCINLINE uint64_t mc_callframe_readuint64(mcvmframe_t* frame)
{
    uint64_t res;
    uint8_t* data;
    data = frame->bytecode + frame->bcposition;
    frame->bcposition += 8;
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

MCINLINE uint16_t mc_callframe_readuint16(mcvmframe_t* frame)
{
    uint8_t* data;
    data = frame->bytecode + frame->bcposition;
    frame->bcposition += 2;
    return (data[0] << 8) | data[1];
}

MCINLINE uint8_t mc_callframe_readuint8(mcvmframe_t* frame)
{
    uint8_t* data;
    data = frame->bytecode + frame->bcposition;
    frame->bcposition++;
    return data[0];
}

MCINLINE mcopcode_t mc_callframe_readopcode(mcvmframe_t* frame)
{
    frame->sourcebcpos = frame->bcposition;
    return (mcopcode_t)mc_callframe_readuint8(frame);
}

MCINLINE mcastlocation_t mc_callframe_getpos(mcvmframe_t* frame)
{
    if(frame->framesrcposlist)
    {
        return frame->framesrcposlist[frame->sourcebcpos];
    }
    return srcposinvalid;
}

mcgcmemory_t* mc_gcmemory_make(mcstate_t* state)
{
    int i;
    mcgcmemory_t* mem = (mcgcmemory_t*)mc_allocator_malloc(state, sizeof(mcgcmemory_t));
    if(!mem)
    {
        return NULL;
    }
    memset(mem, 0, sizeof(mcgcmemory_t));
    mem->pstate = state;
    mem->gcobjlist = mc_ptrlist_make(state, 0);
    if(!mem->gcobjlist)
    {
        goto error;
    }
    mem->gcobjlistback = mc_ptrlist_make(state, 0);
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
    for(i = 0; i < MC_CONF_GCMEMPOOLCOUNT; i++)
    {
        mcgcobjdatapool_t* pool = &mem->mempools[i];
        mem->mempools[i].count = 0;
        memset(pool, 0, sizeof(mcgcobjdatapool_t));
    }
    return mem;
error:
    mc_gcmemory_destroy(mem);
    return NULL;
}

void mc_gcmemory_destroy(mcgcmemory_t* mem)
{
    size_t i;
    size_t j;
    mcobjdata_t* obj;
    mcgcobjdatapool_t* pool;
    mcobjdata_t* data;
    if(!mem)
    {
        return;
    }
    mc_basicarray_destroy(mem->gcobjlistremains);
    mc_ptrlist_destroy(mem->gcobjlistback, NULL);
    for(i = 0; i < mc_ptrlist_count(mem->gcobjlist); i++)
    {
        obj = (mcobjdata_t*)mc_ptrlist_get(mem->gcobjlist, i);
        mc_objectdata_deinit(obj);
        mc_allocator_free(mem->pstate, obj);
    }
    mc_ptrlist_destroy(mem->gcobjlist, NULL);
    for(i = 0; i < MC_CONF_GCMEMPOOLCOUNT; i++)
    {
        pool = &mem->mempools[i];
        for(j = 0; j < (size_t)pool->count; j++)
        {
            data = pool->data[j];
            mc_objectdata_deinit(data);
            mc_allocator_free(mem->pstate, data);
        }
        memset(pool, 0, sizeof(mcgcobjdatapool_t));
    }
    for(i = 0; i < (size_t)mem->onlydatapool.count; i++)
    {
        mc_allocator_free(mem->pstate, mem->onlydatapool.data[i]);
    }
    mc_allocator_free(mem->pstate, mem);
}

mcobjdata_t* mc_gcmemory_allocobjectdata(mcstate_t* state)
{
    bool ok;
    mcobjdata_t* data;
    data = NULL;
    state->mem->allocssincesweep++;
    if(state->mem->onlydatapool.count > 0)
    {
        data = state->mem->onlydatapool.data[state->mem->onlydatapool.count - 1];
        state->mem->onlydatapool.count--;
    }
    else
    {
        data = (mcobjdata_t*)mc_allocator_malloc(state, sizeof(mcobjdata_t));
        if(!data)
        {
            return NULL;
        }
    }
    memset(data, 0, sizeof(mcobjdata_t));
    data->pstate = state;
    MC_ASSERT(mc_ptrlist_count(state->mem->gcobjlistback) >= mc_ptrlist_count(state->mem->gcobjlist));
    /*
    * we want to make sure that appending to gcobjlistback never fails in sweep
    * so this only reserves space there.
    */
    ok = mc_ptrlist_push(state->mem->gcobjlistback, data);
    if(!ok)
    {
        mc_allocator_free(state, data);
        return NULL;
    }
    ok = mc_ptrlist_push(state->mem->gcobjlist, data);
    if(!ok)
    {
        mc_allocator_free(state, data);
        return NULL;
    }
    data->mem = state->mem;
    return data;
}

mcobjdata_t* mc_gcmemory_getdatafrompool(mcstate_t* state, mcvaltype_t type)
{
    bool ok;
    mcobjdata_t* data;
    mcgcobjdatapool_t* pool;
    pool = mc_state_gcgetpoolfortype(state, type);
    if(!pool || pool->count <= 0)
    {
        return NULL;
    }
    data = pool->data[pool->count - 1];
    MC_ASSERT(mc_ptrlist_count(state->mem->gcobjlistback) >= mc_ptrlist_count(state->mem->gcobjlist));
    /*
    * we want to make sure that appending to gcobjlistback never fails in sweep
    * so this only reserves space there.
    */
    ok = mc_ptrlist_push(state->mem->gcobjlistback, data);
    if(!ok)
    {
        return NULL;
    }
    ok = mc_ptrlist_push(state->mem->gcobjlist, data);
    if(!ok)
    {
        return NULL;
    }
    pool->count--;
    return data;
}

void mc_state_gcunmarkall(mcstate_t* state)
{
    size_t i;
    mcobjdata_t* data;
    for(i = 0; i < mc_ptrlist_count(state->mem->gcobjlist); i++)
    {
        data = (mcobjdata_t*)mc_ptrlist_get(state->mem->gcobjlist, i);
        data->gcmark = false;
    }
}

void mc_state_gcmarkobjlist(mcvalue_t* objects, size_t count)
{
    size_t i;
    mcvalue_t obj;
    for(i = 0; i < count; i++)
    {
        obj = objects[i];
        mc_state_gcmarkobject(obj);
    }
}

void mc_state_gcmarkobject(mcvalue_t obj)
{
    int i;
    int len;
    mcvalue_t key;
    mcvalue_t val;
    mcvalue_t freeval;
    mcobjdata_t* data;
    mcobjdata_t* valdata;
    mcobjdata_t* keydata;
    mcobjdata_t* freevaldata;
    mcobjfuncscript_t* function;
    if(!mc_value_isallocated(obj))
    {
        return;
    }
    data = mc_value_getallocateddata(obj);
    if(data->gcmark)
    {
        return;
    }
    data->gcmark = true;
    switch(obj.type)
    {
        case MC_VAL_MAP:
            {
                len = mc_valmap_getlength(obj);
                for(i = 0; i < len; i++)
                {
                    key = mc_valmap_getkeyat(obj, i);
                    if(mc_value_isallocated(key))
                    {
                        keydata = mc_value_getallocateddata(key);
                        if(!keydata->gcmark)
                        {
                            mc_state_gcmarkobject(key);
                        }
                    }
                    val = mc_valmap_getvalueat(obj, i);
                    if(mc_value_isallocated(val))
                    {
                        valdata = mc_value_getallocateddata(val);
                        if(!valdata->gcmark)
                        {
                            mc_state_gcmarkobject(val);
                        }
                    }
                }
            }
            break;
        case MC_VAL_ARRAY:
            {
                len = mc_valarray_getlength(obj);
                for(i = 0; i < len; i++)
                {
                    val = mc_valarray_getvalueat(obj, i);
                    if(mc_value_isallocated(val))
                    {
                        valdata = mc_value_getallocateddata(val);
                        if(!valdata->gcmark)
                        {
                            mc_state_gcmarkobject(val);
                        }
                    }
                }
            }
            break;
        case MC_VAL_FUNCSCRIPT:
            {
                function = mc_value_functiongetscriptfunction(obj);
                for(i = 0; i < function->freevalscount; i++)
                {
                    freeval = mc_value_functiongetfreevalat(obj, i);
                    mc_state_gcmarkobject(freeval);
                    if(mc_value_isallocated(freeval))
                    {
                        freevaldata = mc_value_getallocateddata(freeval);
                        if(!freevaldata->gcmark)
                        {
                            mc_state_gcmarkobject(freeval);
                        }
                    }
                }
            }
            break;
        default:
            {
            }
            break;
    }
}

void mc_state_gcsweep(mcstate_t* state)
{
    bool ok;
    size_t i;
    mcobjdata_t* data;
    mcptrlist_t* objstemp;
    mcgcobjdatapool_t* pool;
    mc_state_gcmarkobjlist((mcvalue_t*)mc_basicarray_data(state->mem->gcobjlistremains), mc_basicarray_count(state->mem->gcobjlistremains));
    MC_ASSERT(mc_ptrlist_count(state->mem->gcobjlistback) >= mc_ptrlist_count(state->mem->gcobjlist));
    mc_ptrlist_clear(state->mem->gcobjlistback);
    for(i = 0; i < mc_ptrlist_count(state->mem->gcobjlist); i++)
    {
        data = (mcobjdata_t*)mc_ptrlist_get(state->mem->gcobjlist, i);
        if(data->gcmark)
        {
            /*
            * this should never fail because gcobjlistback's size should be equal to objects
            */
            ok = mc_ptrlist_push(state->mem->gcobjlistback, data);
            (void)ok;
            MC_ASSERT(ok);
        }
        else
        {
            if(mc_state_gccandatabeputinpool(state, data))
            {
                pool = mc_state_gcgetpoolfortype(state, data->type);
                pool->data[pool->count] = data;
                pool->count++;
            }
            else
            {
                mc_objectdata_deinit(data);
                if(state->mem->onlydatapool.count < MC_CONF_GCMEMPOOLSIZE)
                {
                    state->mem->onlydatapool.data[state->mem->onlydatapool.count] = data;
                    state->mem->onlydatapool.count++;
                }
                else
                {
                    mc_allocator_free(state, data);
                }
            }
        }
    }
    objstemp = state->mem->gcobjlist;
    state->mem->gcobjlist = state->mem->gcobjlistback;
    state->mem->gcobjlistback = objstemp;
    state->mem->allocssincesweep = 0;
}


int mc_state_gcshouldsweep(mcstate_t* state)
{
    return state->mem->allocssincesweep > MC_CONF_GCMEMSWEEPINTERVAL;
}

mcgcobjdatapool_t* mc_state_gcgetpoolfortype(mcstate_t* state, mcvaltype_t type)
{
    switch(type)
    {
        case MC_VAL_ARRAY:
            return &state->mem->mempools[0];
        case MC_VAL_MAP:
            return &state->mem->mempools[1];
        case MC_VAL_STRING:
            return &state->mem->mempools[2];
        default:
            break;
    }
    return NULL;
}

bool mc_state_gccandatabeputinpool(mcstate_t* state, mcobjdata_t* data)
{
    mcvalue_t obj;
    mcgcobjdatapool_t* pool;
    obj = mc_object_makedatafrom(data->type, data);
    /*
    * this is to ensure that large objects won't be kept in pool indefinitely
    */
    switch(data->type)
    {
        case MC_VAL_ARRAY:
            {
                if(mc_valarray_getlength(obj) > 1024)
                {
                    return false;
                }
            }
            break;
        case MC_VAL_MAP:
            {
                if(mc_valmap_getlength(obj) > 1024)
                {
                    return false;
                }
            }
            break;
        case MC_VAL_STRING:
            {
                #if 0
                if(!data->uvobj.valstring.isallocated || data->uvobj.valstring.capacity > 4096)
                {
                    return false;
                }
                #endif
            }
            break;
        default:
            {
            }
            break;
    }
    pool= mc_state_gcgetpoolfortype(state, data->type);
    if(!pool || pool->count >= MC_CONF_GCMEMPOOLSIZE)
    {
        return false;
    }
    return true;
}

mcglobalstore_t* mc_globalstore_make(mcstate_t* state)
{
    mcglobalstore_t* store;
    store = (mcglobalstore_t*)mc_allocator_malloc(state, sizeof(mcglobalstore_t));
    if(!store)
    {
        return NULL;
    }
    memset(store, 0, sizeof(mcglobalstore_t));
    store->pstate = state;
    store->symbols = mc_genericdict_make(state, (mcitemcopyfn_t)mc_symbol_copy, (mcitemdestroyfn_t)mc_symbol_destroy);
    if(!store->symbols)
    {
        goto err;
    }
    store->objects = mc_basicarray_make(state, sizeof(mcvalue_t));
    if(!store->objects)
    {
        goto err;
    }
    return store;
err:
    mc_globalstore_destroy(store);
    return NULL;
}

void mc_globalstore_destroy(mcglobalstore_t* store)
{
    if(!store)
    {
        return;
    }
    mc_genericdict_destroyitemsanddict(store->symbols);
    mc_basicarray_destroy(store->objects);
    mc_allocator_free(store->pstate, store);
}

mcastsymbol_t* mc_globalstore_getsymbol(mcglobalstore_t* store, const char* name)
{
    return (mcastsymbol_t*)mc_genericdict_get(store->symbols, name);
}

bool mc_globalstore_setnamed(mcglobalstore_t* store, const char* name, mcvalue_t object)
{
    bool ok;
    int ix;
    mcastsymbol_t* symbol;
    mcastsymbol_t* existingsymbol;
    existingsymbol = mc_globalstore_getsymbol(store, name);
    if(existingsymbol)
    {
        ok = mc_basicarray_set(store->objects, existingsymbol->index, &object);
        return ok;
    }
    ix = mc_basicarray_count(store->objects);
    ok = mc_basicarray_push(store->objects, &object);
    if(!ok)
    {
        return false;
    }
    symbol = mc_symbol_make(store->pstate, name, MC_SYM_GLOBALBUILTIN, ix, false);
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

mcvalue_t mc_globalstore_getatindex(mcglobalstore_t* store, int ix, bool* outok)
{
    mcvalue_t* res;
    res = (mcvalue_t*)mc_basicarray_get(store->objects, ix);
    if(!res)
    {
        *outok = false;
        return mc_value_makenull();
    }
    *outok = true;
    return *res;
}

mcvalue_t* mc_globalstore_getdata(mcglobalstore_t* store)
{
    return (mcvalue_t*)mc_basicarray_data(store->objects);
}

int mc_globalstore_getcount(mcglobalstore_t* store)
{
    return mc_basicarray_count(store->objects);
}

bool mc_value_ishashable(mcvalue_t obj)
{
    mcvaltype_t type = mc_value_gettype(obj);
    switch(type)
    {
        case MC_VAL_STRING:
            return true;
        case MC_VAL_NUMBER:
            return true;
        case MC_VAL_BOOL:
            return true;
        default:
            break;
    }
    return false;
}

mcastsymbol_t* mc_symbol_make(mcstate_t* state, const char* name, mcastsymtype_t type, int index, bool assignable)
{
    mcastsymbol_t* symbol;
    symbol = (mcastsymbol_t*)mc_allocator_malloc(state, sizeof(mcastsymbol_t));
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

void mc_symbol_destroy(mcastsymbol_t* symbol)
{
    if(!symbol)
    {
        return;
    }
    mc_allocator_free(symbol->pstate, symbol->name);
    mc_allocator_free(symbol->pstate, symbol);
}

mcastsymbol_t* mc_symbol_copy(mcastsymbol_t* symbol)
{
    return mc_symbol_make(symbol->pstate, symbol->name, symbol->type, symbol->index, symbol->assignable);
}

mcastsymtable_t* mc_symtable_make(mcstate_t* state, mcastsymtable_t* outer, mcglobalstore_t* gstore, int mgo)
{
    bool ok;
    mcastsymtable_t* table;
    table = (mcastsymtable_t*)mc_allocator_malloc(state, sizeof(mcastsymtable_t));
    if(!table)
    {
        return NULL;
    }
    memset(table, 0, sizeof(mcastsymtable_t));
    table->pstate = state;
    table->maxnumdefinitions = 0;
    table->outer = outer;
    table->symglobalstore = gstore;
    table->modglobaloffset = mgo;
    table->blockscopes = mc_ptrlist_make(state, 0);
    if(!table->blockscopes)
    {
        goto err;
    }
    table->freesymbols = mc_ptrlist_make(state, 0);
    if(!table->freesymbols)
    {
        goto err;
    }
    table->modglobalsymbols = mc_ptrlist_make(state, 0);
    if(!table->modglobalsymbols)
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

void mc_symtable_destroy(mcastsymtable_t* table)
{
    mcstate_t* state;
    if(!table)
    {
        return;
    }
    while(mc_ptrlist_count(table->blockscopes) > 0)
    {
        mc_symtable_popblockscope(table);
    }
    mc_ptrlist_destroy(table->blockscopes, NULL);
    mc_ptrlist_destroy(table->modglobalsymbols, (mcitemdestroyfn_t)mc_symbol_destroy);
    mc_ptrlist_destroy(table->freesymbols, (mcitemdestroyfn_t)mc_symbol_destroy);
    state = table->pstate;
    memset(table, 0, sizeof(mcastsymtable_t));
    mc_allocator_free(state, table);
}

mcastsymtable_t* mc_symtable_copy(mcastsymtable_t* table)
{
    mcastsymtable_t* copy;
    copy = (mcastsymtable_t*)mc_allocator_malloc(table->pstate, sizeof(mcastsymtable_t));
    if(!copy)
    {
        return NULL;
    }
    memset(copy, 0, sizeof(mcastsymtable_t));
    copy->pstate = table->pstate;
    copy->outer = table->outer;
    copy->symglobalstore = table->symglobalstore;
    copy->blockscopes = mc_ptrlist_copy(table->blockscopes, (mcitemcopyfn_t)mc_astblockscope_copy, (mcitemdestroyfn_t)mc_astblockscope_destroy);
    if(!copy->blockscopes)
    {
        goto err;
    }
    copy->freesymbols = mc_ptrlist_copy(table->freesymbols, (mcitemcopyfn_t)mc_symbol_copy, (mcitemdestroyfn_t)mc_symbol_destroy);
    if(!copy->freesymbols)
    {
        goto err;
    }
    copy->modglobalsymbols = mc_ptrlist_copy(table->modglobalsymbols, (mcitemcopyfn_t)mc_symbol_copy, (mcitemdestroyfn_t)mc_symbol_destroy);
    if(!copy->modglobalsymbols)
    {
        goto err;
    }
    copy->maxnumdefinitions = table->maxnumdefinitions;
    copy->modglobaloffset = table->modglobaloffset;
    return copy;
err:
    mc_symtable_destroy(copy);
    return NULL;
}

bool mc_symtable_addmodsymbol(mcastsymtable_t* st, mcastsymbol_t* symbol)
{
    bool ok;
    mcastsymbol_t* copy;
    if(symbol->type != MC_SYM_MODULEGLOBAL)
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

mcastsymbol_t* mc_symtable_define(mcastsymtable_t* table, const char* name, bool assignable)
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
    globalsymbol = mc_globalstore_getsymbol(table->symglobalstore, name);
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
    symboltype = table->outer == NULL ? MC_SYM_MODULEGLOBAL : MC_SYM_LOCAL;
    ix = mc_symtable_nextsymindex(table);
    symbol = mc_symbol_make(table->pstate, name, symboltype, ix, assignable);
    if(!symbol)
    {
        return NULL;
    }
    globalsymboladded = false;
    ok = false;
    if(symboltype == MC_SYM_MODULEGLOBAL && mc_ptrlist_count(table->blockscopes) == 1)
    {
        globalsymbolcopy = mc_symbol_copy(symbol);
        if(!globalsymbolcopy)
        {
            mc_symbol_destroy(symbol);
            return NULL;
        }
        ok = mc_ptrlist_push(table->modglobalsymbols, globalsymbolcopy);
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
            globalsymbolcopy = (mcastsymbol_t*)mc_ptrlist_pop(table->modglobalsymbols);
            mc_symbol_destroy(globalsymbolcopy);
        }
        mc_symbol_destroy(symbol);
        return NULL;
    }
    topscope = (mcastscopeblock_t*)mc_ptrlist_top(table->blockscopes);
    topscope->numdefinitions++;
    definitionscount = mc_symtable_getnumdefs(table);
    if(definitionscount > table->maxnumdefinitions)
    {
        table->maxnumdefinitions = definitionscount;
    }
    return symbol;
}

mcastsymbol_t* mc_symtable_defineanddestroyold(mcastsymtable_t* st, mcastsymbol_t* original)
{
    bool ok;
    mcastsymbol_t* copy;
    mcastsymbol_t* symbol;
    copy = mc_symbol_make(st->pstate, original->name, original->type, original->index, original->assignable);
    if(!copy)
    {
        return NULL;
    }
    ok = mc_ptrlist_push(st->freesymbols, copy);
    if(!ok)
    {
        mc_symbol_destroy(copy);
        return NULL;
    }
    symbol = mc_symbol_make(st->pstate, original->name, MC_SYM_FREE, mc_ptrlist_count(st->freesymbols) - 1, original->assignable);
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

mcastsymbol_t* mc_symtable_definefunctionname(mcastsymtable_t* st, const char* name, bool assignable)
{
    bool ok;
    mcastsymbol_t* symbol;
    /* module symbol */
    if(strchr(name, ':'))
    {
        return NULL;
    }
    symbol = mc_symbol_make(st->pstate, name, MC_SYM_FUNCTION, 0, assignable);
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

mcastsymbol_t* mc_symtable_definethis(mcastsymtable_t* st)
{
    bool ok;
    mcastsymbol_t* symbol;
    symbol = mc_symbol_make(st->pstate, "this", MC_SYM_THIS, 0, false);
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

mcastsymbol_t* mc_symtable_resolve(mcastsymtable_t* table, const char* name)
{
    int i;
    mcastsymbol_t* symbol;
    mcastscopeblock_t* scope;
    symbol = NULL;
    scope = NULL;
    symbol = mc_globalstore_getsymbol(table->symglobalstore, name);
    if(symbol)
    {
        return symbol;
    }

    for(i = mc_ptrlist_count(table->blockscopes) - 1; i >= 0; i--)
    {
        scope = (mcastscopeblock_t*)mc_ptrlist_get(table->blockscopes, i);
        symbol = (mcastsymbol_t*)mc_genericdict_get(scope->store, name);
        if(symbol)
        {
            break;
        }
    }
    if(symbol && symbol->type == MC_SYM_THIS)
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
        if(symbol->type == MC_SYM_MODULEGLOBAL || symbol->type == MC_SYM_GLOBALBUILTIN)
        {
            return symbol;
        }
        symbol = mc_symtable_defineanddestroyold(table, symbol);
    }
    return symbol;
}

bool mc_symtable_isdefined(mcastsymtable_t* table, const char* name)
{
    /* todo: rename to something more obvious */
    mcastsymbol_t* symbol;
    mcastscopeblock_t* topscope;
    symbol = mc_globalstore_getsymbol(table->symglobalstore, name);
    if(symbol)
    {
        return true;
    }
    topscope = (mcastscopeblock_t*)mc_ptrlist_top(table->blockscopes);
    symbol = (mcastsymbol_t*)mc_genericdict_get(topscope->store, name);
    if(symbol)
    {
        return true;
    }
    return false;
}

bool mc_symtable_pushblockscope(mcastsymtable_t* table)
{
    bool ok;
    int blockscopeoffset;
    mcastscopeblock_t* newscope;
    mcastscopeblock_t* prevblockscope;
    blockscopeoffset = 0;
    prevblockscope = (mcastscopeblock_t*)mc_ptrlist_top(table->blockscopes);
    if(prevblockscope)
    {
        blockscopeoffset = table->modglobaloffset + prevblockscope->offset + prevblockscope->numdefinitions;
    }
    else
    {
        blockscopeoffset = table->modglobaloffset;
    }
    newscope = mc_astblockscope_make(table->pstate, blockscopeoffset);
    if(!newscope)
    {
        return false;
    }
    ok = mc_ptrlist_push(table->blockscopes, newscope);
    if(!ok)
    {
        mc_astblockscope_destroy(newscope);
        return false;
    }
    return true;
}

void mc_symtable_popblockscope(mcastsymtable_t* table)
{
    mcastscopeblock_t* topscope;
    topscope = (mcastscopeblock_t*)mc_ptrlist_top(table->blockscopes);
    mc_ptrlist_pop(table->blockscopes);
    mc_astblockscope_destroy(topscope);
}

mcastscopeblock_t* mc_symtable_getblockscope(mcastsymtable_t* table)
{
    mcastscopeblock_t* topscope;
    topscope = (mcastscopeblock_t*)mc_ptrlist_top(table->blockscopes);
    return topscope;
}

bool mc_symtable_ismodglobalscope(mcastsymtable_t* table)
{
    return table->outer == NULL;
}

bool mc_symtable_istopblockscope(mcastsymtable_t* table)
{
    return mc_ptrlist_count(table->blockscopes) == 1;
}

bool mc_symtable_istopglobalscope(mcastsymtable_t* table)
{
    return mc_symtable_ismodglobalscope(table) && mc_symtable_istopblockscope(table);
}

size_t mc_symtable_getmodglobalsymcount(mcastsymtable_t* table)
{
    return mc_ptrlist_count(table->modglobalsymbols);
}

mcastsymbol_t* mc_symtable_getmodglobalsymat(mcastsymtable_t* table, int ix)
{
    return (mcastsymbol_t*)mc_ptrlist_get(table->modglobalsymbols, ix);
}

mcastscopeblock_t* mc_astblockscope_make(mcstate_t* state, int offset)
{
    mcastscopeblock_t* newscope;
    newscope = (mcastscopeblock_t*)mc_allocator_malloc(state, sizeof(mcastscopeblock_t));
    if(!newscope)
    {
        return NULL;
    }
    memset(newscope, 0, sizeof(mcastscopeblock_t));
    newscope->pstate = state;
    newscope->store = mc_genericdict_make(state, (mcitemcopyfn_t)mc_symbol_copy, (mcitemdestroyfn_t)mc_symbol_destroy);
    if(!newscope->store)
    {
        mc_astblockscope_destroy(newscope);
        return NULL;
    }
    newscope->numdefinitions = 0;
    newscope->offset = offset;
    return newscope;
}

void mc_astblockscope_destroy(mcastscopeblock_t* scope)
{
    mc_genericdict_destroyitemsanddict(scope->store);
    mc_allocator_free(scope->pstate, scope);
}

mcastscopeblock_t* mc_astblockscope_copy(mcastscopeblock_t* scope)
{
    mcastscopeblock_t* copy;
    copy = (mcastscopeblock_t*)mc_allocator_malloc(scope->pstate, sizeof(mcastscopeblock_t));
    if(!copy)
    {
        return NULL;
    }
    memset(copy, 0, sizeof(mcastscopeblock_t));
    copy->pstate = scope->pstate;
    copy->numdefinitions = scope->numdefinitions;
    copy->offset = scope->offset;
    copy->store = mc_genericdict_copy(scope->store);
    if(!copy->store)
    {
        mc_astblockscope_destroy(copy);
        return NULL;
    }
    return copy;
}

bool mc_symtable_setsymbol(mcastsymtable_t* table, mcastsymbol_t* symbol)
{
    mcastscopeblock_t* topscope;
    mcastsymbol_t* existing;
    topscope = (mcastscopeblock_t*)mc_ptrlist_top(table->blockscopes);
    existing = (mcastsymbol_t*)mc_genericdict_get(topscope->store, symbol->name);
    if(existing)
    {
        mc_symbol_destroy(existing);
    }
    return mc_genericdict_set(topscope->store, symbol->name, symbol);
}

int mc_symtable_nextsymindex(mcastsymtable_t* table)
{
    int ix;
    mcastscopeblock_t* topscope;
    topscope = (mcastscopeblock_t*)mc_ptrlist_top(table->blockscopes);
    ix = topscope->offset + topscope->numdefinitions;
    return ix;
}

int mc_symtable_getnumdefs(mcastsymtable_t* table)
{
    int i;
    int count;
    mcastscopeblock_t* scope;
    count = 0;
    for(i = mc_ptrlist_count(table->blockscopes) - 1; i >= 0; i--)
    {
        scope = (mcastscopeblock_t*)mc_ptrlist_get(table->blockscopes, i);
        count += scope->numdefinitions;
    }
    return count;
}

void mc_asttoken_init(mcasttoken_t* tok, mcasttoktype_t type, const char* literal, int len)
{
    tok->type = type;
    tok->literal = literal;
    tok->len = len;
}

char* mc_asttoken_dupliteralstring(mcstate_t* state, mcasttoken_t* tok)
{
    return mc_util_strndup(state, tok->literal, tok->len);
}

const char* mc_asttoken_typename(mcasttoktype_t type)
{
    switch(type)
    {
        case MC_TOK_EOF:
            return "EOF";
        case MC_TOK_ASSIGN:
            return "=";
        case MC_TOK_ASSIGNPLUS:
            return "+=";
        case MC_TOK_ASSIGNMINUS:
            return "-=";
        case MC_TOK_ASSIGNASTERISK:
            return "*=";
        case MC_TOK_ASSIGNSLASH:
            return "/=";
        case MC_TOK_ASSIGNPERCENT:
            return "%=";
        case MC_TOK_ASSIGNBINAND:
            return "&=";
        case MC_TOK_ASSIGNBINOR:
            return "|=";
        case MC_TOK_ASSIGNBINXOR:
            return "^=";
        case MC_TOK_ASSIGNLSHIFT:
            return "<<=";
        case MC_TOK_ASSIGNRSHIFT:
            return ">>=";
        case MC_TOK_QUESTION:
            return "?";
        case MC_TOK_PLUS:
            return "+";
        case MC_TOK_PLUSPLUS:
            return "++";
        case MC_TOK_UNARYMINUS:
            return "-";
        case MC_TOK_MINUSMINUS:
            return "--";
        case MC_TOK_BANG:
            return "!";
        case MC_TOK_ASTERISK:
            return "*";
        case MC_TOK_SLASH:
            return "/";
        case MC_TOK_LT:
            return "<";
        case MC_TOK_LTE:
            return "<=";
        case MC_TOK_GT:
            return ">";
        case MC_TOK_GTE:
            return ">=";
        case MC_TOK_EQ:
            return "==";
        case MC_TOK_NOTEQ:
            return "!=";
        case MC_TOK_AND:
            return "&&";
        case MC_TOK_OR:
            return "||";
        case MC_TOK_BINAND:
            return "&";
        case MC_TOK_BINOR:
            return "|";
        case MC_TOK_BINXOR:
            return "^";
        case MC_TOK_LSHIFT:
            return "<<";
        case MC_TOK_RSHIFT:
            return ">>";
        case MC_TOK_COMMA:
            return ",";
        case MC_TOK_SEMICOLON:
            return ";";
        case MC_TOK_COLON:
            return ":";
        case MC_TOK_LPAREN:
            return "(";
        case MC_TOK_RPAREN:
            return ")";
        case MC_TOK_LBRACE:
            return "{";
        case MC_TOK_RBRACE:
            return "}";
        case MC_TOK_LBRACKET:
            return "[";
        case MC_TOK_RBRACKET:
            return "]";
        case MC_TOK_DOT:
            return ".";
        case MC_TOK_PERCENT:
            return "%";
        case MC_TOK_FUNCTION:
            return "FUNCTION";
        case MC_TOK_CONST:
            return "CONST";
        case MC_TOK_VAR:
            return "VAR";
        case MC_TOK_TRUE:
            return "TRUE";
        case MC_TOK_FALSE:
            return "FALSE";
        case MC_TOK_IF:
            return "IF";
        case MC_TOK_ELSE:
            return "ELSE";
        case MC_TOK_RETURN:
            return "RETURN";
        case MC_TOK_WHILE:
            return "WHILE";
        case MC_TOK_BREAK:
            return "BREAK";
        case MC_TOK_FOR:
            return "FOR";
        case MC_TOK_IN:
            return "IN";
        case MC_TOK_CONTINUE:
            return "CONTINUE";
        case MC_TOK_NULL:
            return "NULL";
        case MC_TOK_IMPORT:
            return "IMPORT";
        case MC_TOK_RECOVER:
            return "RECOVER";
        case MC_TOK_IDENT:
            return "IDENT";
        case MC_TOK_NUMBER:
            return "NUMBER";
        case MC_TOK_STRING:
            return "STRING";
        case MC_TOK_TEMPLATESTRING:
            return "TEMPLATE_STRING";
        default:
            break;
    }
    return "ILLEGAL";
}

mctraceback_t* mc_traceback_make(mcstate_t* state)
{
    mctraceback_t* traceback;
    traceback = (mctraceback_t*)mc_allocator_malloc(state, sizeof(mctraceback_t));
    if(!traceback)
    {
        return NULL;
    }
    memset(traceback, 0, sizeof(mctraceback_t));
    traceback->pstate = state;
    traceback->items = mc_basicarray_make(state, sizeof(mctraceitem_t));
    if(!traceback->items)
    {
        mc_traceback_destroy(traceback);
        return NULL;
    }
    return traceback;
}

void mc_traceback_destroy(mctraceback_t* traceback)
{
    size_t i;
    mctraceitem_t* item;
    if(!traceback)
    {
        return;
    }
    for(i = 0; i < mc_basicarray_count(traceback->items); i++)
    {
        item = (mctraceitem_t*)mc_basicarray_get(traceback->items, i);
        mc_allocator_free(traceback->pstate, item->trfuncname);
    }
    mc_basicarray_destroy(traceback->items);
    mc_allocator_free(traceback->pstate, traceback);
}

bool mc_traceback_push(mctraceback_t* traceback, const char* fname, mcastlocation_t pos)
{
    bool ok;
    mctraceitem_t item;
    item.trfuncname = mc_util_strdup(traceback->pstate, fname);
    if(!item.trfuncname)
    {
        return false;
    }
    item.pos = pos;
    ok = mc_basicarray_push(traceback->items, &item);
    if(!ok)
    {
        mc_allocator_free(traceback->pstate, item.trfuncname);
        return false;
    }
    return true;
}

bool mc_traceback_vmpush(mctraceback_t* traceback, mcstate_t* state)
{
    bool ok;
    int i;
    mcvmframe_t* frame;
    for(i = state->framecount - 1; i >= 0; i--)
    {
        frame = mc_framelist_get(state->framestack, i);
        ok = mc_traceback_push(traceback, mc_value_functiongetname(frame->function), mc_callframe_getpos(frame));
        if(!ok)
        {
            return false;
        }
    }
    return true;
}

bool mc_printer_printtraceback(mcprinter_t* pr, mctraceback_t* traceback)
{
    int i;
    int depth;
    const char* filename;
    mctraceitem_t* item;
    depth = mc_basicarray_count(traceback->items);
    for(i = 0; i < depth; i++)
    {
        item = (mctraceitem_t*)mc_basicarray_get(traceback->items, i);
        filename = mc_traceitem_getsourcefilepath(item);
        if(item->pos.line >= 0 && item->pos.column >= 0)
        {
            mc_printer_printf(pr, "%s in %s on %d:%d\n", item->trfuncname, filename, item->pos.line, item->pos.column);
        }
        else
        {
            mc_printer_printf(pr, "%s\n", item->trfuncname);
        }
    }
    return !mc_printer_failed(pr);
}

const char* mc_traceitem_getsourceline(mctraceitem_t* item)
{
    const char* line;
    mcptrlist_t* lines;
    if(!item->pos.file)
    {
        return NULL;
    }
    lines = item->pos.file->lines;
    if((size_t)item->pos.line >= (size_t)mc_ptrlist_count(lines))
    {
        return NULL;
    }
    line = (const char*)mc_ptrlist_get(lines, item->pos.line);
    return line;
}

const char* mc_traceitem_getsourcefilepath(mctraceitem_t* item)
{
    if(!item->pos.file)
    {
        return NULL;
    }
    return item->pos.file->path;
}

bool mc_vm_init(mcstate_t* state)
{
    int i;
    mcvalue_t keyobj;
    state->globalvalcount = 0;
    state->vsposition = 0;
    state->thisstpos = 0;
    state->framecount = 0;
    state->lastpopped = mc_value_makenull();
    state->running = false;
    for(i = 0; i < MC_CONF_MAXOPEROVERLOADS; i++)
    {
        state->operoverloadkeys[i] = mc_value_makenull();
    }
#define SET_OPERATOR_OVERLOAD_KEY(op, key)\
    do\
    {\
        keyobj = mc_value_makestring(state, key);\
        if(mc_value_isnull(keyobj))\
        {\
            goto err;\
        }\
        state->operoverloadkeys[op] = keyobj;\
    } while(0)
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_ADD, "__operator_add__");
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_SUB, "__operator_sub__");
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_MUL, "__operator_mul__");
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_DIV, "__operator_div__");
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_MOD, "__operator_mod__");
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_BINOR, "__operator_or__");
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_BINXOR, "__operator_xor__");
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_BINAND, "__operator_and__");
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_LSHIFT, "__operator_lshift__");
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_RSHIFT, "__operator_rshift__");
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_MINUS, "__operator_minus__");
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_BINNOT, "__operator_binnot__");
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_BANG, "__operator_bang__");
    SET_OPERATOR_OVERLOAD_KEY(MC_OPCODE_COMPARE, "__cmp__");
#undef SET_OPERATOR_OVERLOAD_KEY
    return true;
    err:
        return false;
}

void mc_vm_reset(mcstate_t* state)
{
    state->vsposition = 0;
    state->thisstpos = 0;
    while(state->framecount > 0)
    {
        mc_vm_popframe(state);
    }
}

bool mc_vm_runexecfunc(mcstate_t* state, mccompiledprogram_t* comp_res, mcbasicarray_t* constants)
{
    bool res;
    int oldsp;
    int oldthissp;
    int oldframescount;
    mcvalue_t mainfn;
    (void)oldsp;
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    oldsp = state->vsposition;
#endif
    oldthissp = state->thisstpos;
    oldframescount = state->framecount;
    mainfn = mc_value_makefuncscript(state, "main", comp_res, false, 0, 0, 0);
    if(mc_value_isnull(mainfn))
    {
        return false;
    }
    mc_vm_stackpush(state, mainfn);
    res = mc_function_execfunction(state, mainfn, constants);
    while(state->framecount > oldframescount)
    {
        mc_vm_popframe(state);
    }
    MC_ASSERT(state->vsposition == oldsp);
    state->thisstpos = oldthissp;
    return res;
}

mcvalue_t mc_vm_callvalue(mcstate_t* state, mcbasicarray_t* constants, mcvalue_t callee, int argc, mcvalue_t* args)
{
    bool ok;
    int i;
    int oldsp;
    int oldthissp;
    int oldframescount;
    mcvaltype_t type;
    (void)oldsp;
    type = mc_value_gettype(callee);
    if(type == MC_VAL_FUNCSCRIPT)
    {
        #if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
            oldsp = state->vsposition;
        #endif
        oldthissp = state->thisstpos;
        oldframescount = state->framecount;
        mc_vm_stackpush(state, callee);
        for(i = 0; i < argc; i++)
        {
            mc_vm_stackpush(state, args[i]);
        }
        ok = mc_function_execfunction(state, callee, constants);
        if(!ok)
        {
            return mc_value_makenull();
        }
        while(state->framecount > oldframescount)
        {
            mc_vm_popframe(state);
        }
        MC_ASSERT(state->vsposition == oldsp);
        state->thisstpos = oldthissp;
        return mc_vm_getlastpopped(state);
    }
    if(type == MC_VAL_FUNCNATIVE)
    {
        return mc_vm_callnativefunction(state, callee, srcposinvalid, argc, args);
    }
    mc_errlist_push(&state->errors, MC_ERROR_USER, srcposinvalid, "Object is not callable");
    return mc_value_makenull();
}

MCINLINE bool mc_vmdo_tryoverloadoperator(mcstate_t* state, mcvalue_t left, mcvalue_t right, mcinternopcode_t op, bool* outoverloadfound)
{
    int numoper;
    mcvalue_t key;
    mcvalue_t callee;
    mcvaltype_t lefttype;
    mcvaltype_t righttype;
    *outoverloadfound = false;
    lefttype = mc_value_gettype(left);
    righttype = mc_value_gettype(right);
    if(lefttype != MC_VAL_MAP && righttype != MC_VAL_MAP)
    {
        *outoverloadfound = false;
        return true;
    }
    numoper = 2;
    if(op == MC_OPCODE_MINUS || op == MC_OPCODE_BINNOT || op == MC_OPCODE_BANG)
    {
        numoper = 1;
    }
    key = state->operoverloadkeys[op];
    callee = mc_value_makenull();
    if(lefttype == MC_VAL_MAP)
    {
        callee = mc_valmap_getvalue(left, key);
    }
    if(!mc_value_iscallable(callee))
    {
        if(righttype == MC_VAL_MAP)
        {
            callee = mc_valmap_getvalue(right, key);
        }

        if(!mc_value_iscallable(callee))
        {
            *outoverloadfound = false;
            return true;
        }
    }
    *outoverloadfound = true;
    mc_vm_stackpush(state, callee);
    mc_vm_stackpush(state, left);
    if(numoper == 2)
    {
        mc_vm_stackpush(state, right);
    }
    return mc_vmdo_callobject(state, callee, numoper);
}

mcvalue_t mc_vm_getlastpopped(mcstate_t* state)
{
    return state->lastpopped;
}

bool mc_vm_haserrors(mcstate_t* state)
{
    return state->errors.count > 0;
}

bool mc_vm_setglobalbyindex(mcstate_t* state, int ix, mcvalue_t val)
{
    #if 1
    if(ix >= MC_CONF_VMMAXGLOBALS)
    {
        MC_ASSERT(false);
        mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Global write out of range");
        return false;
    }
    #endif
    state->globalvalstack[ix] = val;
    if(ix >= state->globalvalcount)
    {
        state->globalvalcount = ix + 1;
    }
    return true;
}

mcvalue_t mc_vm_getglobalbyindex(mcstate_t* state, int ix)
{
    #if 1
    if(ix >= MC_CONF_VMMAXGLOBALS)
    {
        MC_ASSERT(false);
        mc_errlist_push(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Global read out of range");
        return mc_value_makenull();
    }
    #endif
    return state->globalvalstack[ix];
}

void mc_vm_setstackpos(mcstate_t* state, int nsp)
{
    int count;
    size_t bytescount;
    (void)count;
    (void)bytescount;
    if(nsp > state->vsposition)
    {
        /* to avoid gcing freed objects */
        count = nsp - state->vsposition;
        bytescount = count * sizeof(mcvalue_t);
        //memset(state->valuestack->listitems + state->vsposition, 0, bytescount);
    }
    state->vsposition = nsp;
}

void mc_vm_stackpush(mcstate_t* vm, mcvalue_t obj)
{
    int numlocals;
    mcvmframe_t* frame;
    mcobjfuncscript_t* currentfunction;
    (void)numlocals;
    (void)frame;
    (void)currentfunction;
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(vm->currframe)
    {
        frame = vm->currframe;
        currentfunction = mc_value_functiongetscriptfunction(frame->function);
        numlocals = currentfunction->numlocals;
        MC_ASSERT(vm->vsposition >= (frame->basepointer + numlocals));
    }
#endif
    mc_vallist_set(vm->valuestack, vm->vsposition, obj);
    vm->vsposition++;
}

mcvalue_t mc_vm_stackpop(mcstate_t* vm)
{
    int numlocals;
    mcvalue_t res;
    mcvmframe_t* frame;
    mcobjfuncscript_t* currentfunction;
    (void)numlocals;
    (void)frame;
    (void)currentfunction;
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(vm->vsposition == 0)
    {
        mc_errlist_push(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(vm->currframe), "Stack underflow");
        MC_ASSERT(false);
        return mc_value_makenull();
    }
    if(vm->currframe)
    {
        frame = vm->currframe;
        currentfunction = mc_value_functiongetscriptfunction(frame->function);
        numlocals = currentfunction->numlocals;
        MC_ASSERT((vm->vsposition - 1) >= (frame->basepointer + numlocals));
    }
#endif
    vm->vsposition--;
    res = mc_vallist_get(vm->valuestack, vm->vsposition);
    vm->lastpopped = res;
    return res;
}

mcvalue_t mc_vm_stackget(mcstate_t* vm, int nthitem)
{
    int ix;
    ix = vm->vsposition - 1 - nthitem;
    return mc_vallist_get(vm->valuestack, ix);
}

void mc_vm_thisstackpush(mcstate_t* vm, mcvalue_t obj)
{
    mc_vallist_set(vm->valthisstack, vm->thisstpos, obj);
    vm->thisstpos++;
}

mcvalue_t mc_vm_thisstackpop(mcstate_t* vm)
{
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(vm->thisstpos == 0)
    {
        mc_errlist_push(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(vm->currframe), "this stack underflow");
        MC_ASSERT(false);
        return mc_value_makenull();
    }
#endif
    vm->thisstpos--;
    return mc_vallist_get(vm->valthisstack, vm->thisstpos);
}

mcvalue_t mc_vm_thisstackget(mcstate_t* vm, int nthitem)
{
    int ix;
    ix = vm->thisstpos - 1 - nthitem;
    return mc_vallist_get(vm->valthisstack, ix);
}

bool mc_vm_pushframe(mcstate_t* vm, mcvmframe_t frame)
{
    mcobjfuncscript_t* framefunction;
    mc_framelist_set(vm->framestack, vm->framecount, frame);
    vm->currframe = mc_framelist_get(vm->framestack, vm->framecount);
    vm->framecount++;
    framefunction = mc_value_functiongetscriptfunction(frame.function);
    mc_vm_setstackpos(vm, frame.basepointer + framefunction->numlocals);
    return true;
}

bool mc_vm_popframe(mcstate_t* vm)
{
    mc_vm_setstackpos(vm, vm->currframe->basepointer - 1);
    if(vm->framecount <= 0)
    {
        MC_ASSERT(false);
        vm->currframe = NULL;
        return false;
    }
    vm->framecount--;
    if(vm->framecount == 0)
    {
        vm->currframe = NULL;
        return false;
    }
    vm->currframe = mc_framelist_get(vm->framestack, vm->framecount - 1);
    return true;
}

void mc_vm_rungc(mcstate_t* vm, mcbasicarray_t* constants)
{
    int i;
    mcvmframe_t* frame;
    mc_state_gcunmarkall(vm);
    mc_state_gcmarkobjlist(mc_globalstore_getdata(vm->vmglobalstore), mc_globalstore_getcount(vm->vmglobalstore));
    mc_state_gcmarkobjlist((mcvalue_t*)mc_basicarray_data(constants), mc_basicarray_count(constants));
    mc_state_gcmarkobjlist(vm->globalvalstack, vm->globalvalcount);
    for(i = 0; i < vm->framecount; i++)
    {
        frame = mc_framelist_get(vm->framestack, i);
        mc_state_gcmarkobject(frame->function);
    }
    mc_state_gcmarkobjlist(vm->valuestack->listitems, vm->vsposition);
    mc_state_gcmarkobjlist(vm->valthisstack->listitems, vm->thisstpos);
    mc_state_gcmarkobject(vm->lastpopped);
    mc_state_gcmarkobjlist(vm->operoverloadkeys, MC_CONF_MAXOPEROVERLOADS);
    mc_state_gcsweep(vm);
}

MCINLINE bool mc_vmdo_callobject(mcstate_t* vm, mcvalue_t callee, int nargs)
{
    bool ok;
    const char* calleetypename;
    mcvaltype_t calleetype;
    mcvmframe_t calleeframe;
    mcvalue_t res;
    mcvalue_t* stackpos;
    mcobjfuncscript_t* calleefunction;
    calleetype = mc_value_gettype(callee);
    if(calleetype == MC_VAL_FUNCSCRIPT)
    {
        calleefunction = mc_value_functiongetscriptfunction(callee);
        if(nargs != calleefunction->numargs)
        {
            #if 1
            mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(vm->currframe), "Invalid number of arguments to \"%s\", expected %d, got %d",
                              mc_value_functiongetname(callee), calleefunction->numargs, nargs);
            return false;
            #endif
        }
        ok = mc_callframe_init(&calleeframe, callee, vm->vsposition - nargs);
        if(!ok)
        {
            mc_errlist_push(&vm->errors, MC_ERROR_RUNTIME, srcposinvalid, "Frame init failed in mc_vmdo_callobject");
            return false;
        }
        ok = mc_vm_pushframe(vm, calleeframe);
        if(!ok)
        {
            mc_errlist_push(&vm->errors, MC_ERROR_RUNTIME, srcposinvalid, "Pushing frame failed in mc_vmdo_callobject");
            return false;
        }
    }
    else if(calleetype == MC_VAL_FUNCNATIVE)
    {
        stackpos = vm->valuestack->listitems + vm->vsposition - nargs;
        res = mc_vm_callnativefunction(vm, callee, mc_callframe_getpos(vm->currframe), nargs, stackpos);
        if(mc_vm_haserrors(vm))
        {
            return false;
        }
        mc_vm_setstackpos(vm, vm->vsposition - nargs - 1);
        mc_vm_stackpush(vm, res);
    }
    else
    {
        calleetypename = mc_valtype_getname(calleetype);
        mc_errlist_addf(&vm->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(vm->currframe), "%s object is not callable", calleetypename);
        return false;
    }
    return true;
}

mcvalue_t mc_vm_callnativefunction(mcstate_t* vm, mcvalue_t callee, mcastlocation_t srcpos, int argc, mcvalue_t* args)
{
    mcvaltype_t restype;
    mcvalue_t res;
    mcerror_t* err; 
    mctraceback_t* traceback;
    mcobjfuncnative_t* nativefun;
    nativefun = mc_value_functiongetnativefunction(callee);
    res = nativefun->natptrfn(vm, nativefun->userdata, argc, args);
    if(mc_util_unlikely(vm->errors.count > 0))
    {
        err = mc_errlist_getlast(&vm->errors);
        err->pos = srcpos;
        err->traceback = mc_traceback_make(vm);
        if(err->traceback)
        {
            mc_traceback_push(err->traceback, nativefun->name, srcposinvalid);
        }
        return mc_value_makenull();
    }
    restype = mc_value_gettype(res);
    if(mc_util_unlikely(restype == MC_VAL_ERROR))
    {
        traceback = mc_traceback_make(vm);
        if(traceback)
        {
            /* error builtin is treated in a special way */
            if(!MC_UTIL_STREQ(nativefun->name, "error"))
            {
                mc_traceback_push(traceback, nativefun->name, srcposinvalid);
            }
            mc_traceback_vmpush(traceback, vm);
            mc_value_errorsettraceback(res, traceback);
        }
    }
    return res;
}

bool mc_vm_checkassign(mcstate_t* state, mcvalue_t oldvalue, mcvalue_t nvalue)
{
    mcvaltype_t nvaluetype;
    mcvaltype_t oldvaluetype;
    (void)state;
    oldvaluetype = mc_value_gettype(oldvalue);
    nvaluetype = mc_value_gettype(nvalue);
    if(oldvaluetype == MC_VAL_NULL || nvaluetype == MC_VAL_NULL)
    {
        return true;
    }
    #if 0
    if(oldvaluetype != nvaluetype)
    {
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Trying to assign variable of type %s to %s",
                          mc_valtype_getname(nvaluetype), mc_valtype_getname(oldvaluetype));
        return false;
    }
    #endif
    return true;
}

bool mc_valstring_appendvalue(mcvalue_t destval, mcvalue_t val)
{
    bool ok;
    int vlen;
    const char* vstr;
    if(val.type == MC_VAL_NUMBER)
    {
        mc_valstring_appendformat(destval, "%g", mc_value_getnumber(val));
        return true;
    }
    if(val.type == MC_VAL_STRING)
    {
        vlen = mc_valstring_getlength(val);
        vstr = mc_valstring_getdata(val);
        ok = mc_valstring_appendlen(destval, vstr, vlen);
        if(!ok)
        {
            return false;
        }
    
        return true;
    }
    return false;
}

MCINLINE bool mc_vmdo_opaddstring(mcstate_t* state, mcvalue_t valleft, mcvalue_t valright, mcvaltype_t righttype, mcopcode_t opcode)
{
    mcvalue_t nstring;
    (void)opcode;
    nstring = mc_value_makestrcapacity(state, 0);
    mc_vm_stackpush(state, nstring);
    if(!mc_valstring_appendvalue(nstring, valleft))
    {
        mc_vm_stackpush(state, valleft);
        return false;
    }
    if(!mc_valstring_appendvalue(nstring, valright))
    {
        mc_vm_stackpush(state, valright);
        return false;
    }
    return true;
}

MCINLINE bool mc_vmdo_math(mcstate_t* state, mcopcode_t opcode)
{
    bool ok;
    bool overloadfound;
    mcfloat_t res;
    mcfloat_t dnright;
    mcfloat_t dnleft;
    const char* opcodename;
    const char* lefttypename;
    const char* righttypename;
    mcvalue_t valright;
    mcvalue_t valleft;
    mcvaltype_t lefttype;
    mcvaltype_t righttype;
    valright = mc_vm_stackpop(state);
    valleft = mc_vm_stackpop(state);
    lefttype = mc_value_gettype(valleft);
    righttype = mc_value_gettype(valright);
    if(mc_value_isnumeric(valleft) && mc_value_isnumeric(valright))
    {
        dnright = mc_value_getnumber(valright);
        dnleft = mc_value_getnumber(valleft);
        res = 0;
        switch(opcode)
        {
            case MC_OPCODE_ADD:
                {
                    res = mc_mathutil_add(dnleft, dnright);
                }
                break;
            case MC_OPCODE_SUB:
                {
                    res = mc_mathutil_sub(dnleft, dnright);
                }
                break;
            case MC_OPCODE_MUL:
                {
                    res = mc_mathutil_mult(dnleft, dnright);
                }
                break;
            case MC_OPCODE_DIV:
                {
                    res = mc_mathutil_div(dnleft, dnright);
                }
                break;
            case MC_OPCODE_MOD:
                {
                    res = mc_mathutil_mod(dnleft, dnright);
                }
                break;
            case MC_OPCODE_BINOR:
                {
                    res = mc_mathutil_binor(dnleft, dnright);
                }
                break;
            case MC_OPCODE_BINXOR:
                {
                    res = mc_mathutil_binxor(dnleft, dnright);
                }
                break;
            case MC_OPCODE_BINAND:
                {
                    res = mc_mathutil_binand(dnleft, dnright);
                }
                break;
            case MC_OPCODE_LSHIFT:
                {
                    res = mc_mathutil_binshiftleft(dnleft, dnright);
                }
                break;
            case MC_OPCODE_RSHIFT:
                {
                    res = mc_mathutil_binshiftright(dnleft, dnright);
                }
                break;
            default:
                {
                    assert(false);
                }
                break;
        }
        mc_vm_stackpush(state, mc_value_makenumber(res));
        return true;
    }
    else if(lefttype == MC_VAL_STRING && opcode == MC_OPCODE_ADD)
    {
        if(mc_vmdo_opaddstring(state, valleft, valright, righttype, opcode))
        {
            return true;
        }
    }
    overloadfound = false;
    ok = mc_vmdo_tryoverloadoperator(state, valleft, valright, opcode, &overloadfound);
    if(!ok)
    {
        return false;
    }
    if(!overloadfound)
    {
        opcodename = mc_opdef_getname(opcode);
        lefttypename = mc_valtype_getname(lefttype);
        righttypename = mc_valtype_getname(righttype);
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Invalid operand types for %s, got %s and %s",
                          opcodename, lefttypename, righttypename);
        return false;
    }
    return true;
}

MCINLINE bool mc_vmdo_getindexpartial(mcstate_t* state, mcvalue_t left, mcvaltype_t lefttype, mcvalue_t index, mcvaltype_t indextype)
{
    int leftlen;
    int ix;
    char resstr[2];
    const char* str;
    const char* indextypename;
    const char* lefttypename;
    mcvalue_t res;
    if(lefttype != MC_VAL_ARRAY && lefttype != MC_VAL_MAP && lefttype != MC_VAL_STRING)
    {
        lefttypename = mc_valtype_getname(lefttype);
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Type %s is not indexable", lefttypename);
        return false;
    }
    res = mc_value_makenull();
    if(lefttype == MC_VAL_ARRAY)
    {
        if(indextype != MC_VAL_NUMBER)
        {
            lefttypename = mc_valtype_getname(lefttype);
            indextypename = mc_valtype_getname(indextype);
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Cannot index %s with %s", lefttypename, indextypename);
            return false;
        }
        ix = (int)mc_value_getnumber(index);
        if(ix < 0)
        {
            ix = mc_valarray_getlength(left) + ix;
        }
        if(ix >= 0 && ix < mc_valarray_getlength(left))
        {
            res = mc_valarray_getvalueat(left, ix);
        }
    }
    else if(lefttype == MC_VAL_MAP)
    {
        res = mc_valmap_getvalue(left, index);
    }
    else if(lefttype == MC_VAL_STRING)
    {
        if(indextype != MC_VAL_NUMBER)
        {
            lefttypename = mc_valtype_getname(lefttype);
            indextypename = mc_valtype_getname(indextype);
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Cannot index %s with %s", lefttypename, indextypename);
            return false;
        }
        str = mc_valstring_getdata(left);
        leftlen = mc_valstring_getlength(left);
        ix = (int)mc_value_getnumber(index);
        if(ix >= 0 && ix < leftlen)
        {
            resstr[0] = str[ix];
            res = mc_value_makestringlen(state, resstr, 1);
        }
    }
    mc_vm_stackpush(state, res);
    return true;
}

MCINLINE bool mc_vmdo_getindexfull(mcstate_t* state)
{
    mcvaltype_t lefttype;
    mcvaltype_t indextype;
    mcvalue_t left;
    mcvalue_t index;
    index = mc_vm_stackpop(state);
    left = mc_vm_stackpop(state);
    lefttype = mc_value_gettype(left);
    indextype = mc_value_gettype(index);
    return mc_vmdo_getindexpartial(state, left, lefttype, index, indextype);
}

bool mc_vm_findclassfor(mcstate_t* state)
{
    (void)state;
    return false;
}

MCINLINE bool mc_vmdo_getdotindex(mcstate_t* state)
{
    mcvaltype_t lefttype;
    mcvaltype_t indextype;
    mcvalue_t left;
    mcvalue_t index;
    index = mc_vm_stackpop(state);
    left = mc_vm_stackpop(state);
    lefttype = mc_value_gettype(left);
    indextype = mc_value_gettype(index);
    if(indextype == MC_VAL_STRING)
    {
        /* TODO: find member function, if any */
    }
    return mc_vmdo_getindexpartial(state, left, lefttype, index, indextype);
}

MCINLINE bool mc_vmdo_setindexpartial(mcstate_t* state, mcvalue_t left, mcvaltype_t lefttype, mcvalue_t index, mcvaltype_t indextype, mcvalue_t nvalue)
{
    bool ok;
    int alen;
    int ix;
    const char* indextypename;
    const char* lefttypename;
    mcvalue_t oldvalue;
    if(lefttype != MC_VAL_ARRAY && lefttype != MC_VAL_MAP)
    {
        lefttypename = mc_valtype_getname(lefttype);
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "type %s is not indexable", lefttypename);
        return false;
    }
    if(lefttype == MC_VAL_ARRAY)
    {
        if(indextype != MC_VAL_NUMBER)
        {
            lefttypename = mc_valtype_getname(lefttype);
            indextypename = mc_valtype_getname(indextype);
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "cannot index %s with %s", lefttypename, indextypename);
            return false;
        }
        ix = (int)mc_value_getnumber(index);                        
        ok = mc_valarray_setvalueat(left, ix, nvalue);
        alen = mc_valarray_getlength(left);
        if(!ok)
        {
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "failed to set array index %d (of %d)", ix, alen);
            return false;
        }
    }
    else if(lefttype == MC_VAL_MAP)
    {
        oldvalue = mc_valmap_getvalue(left, index);
        if(!mc_vm_checkassign(state, oldvalue, nvalue))
        {
            return false;
        }
        ok = mc_valmap_setvalue(left, index, nvalue);
        if(!ok)
        {
            return false;
        }
    }
    return true;
}

MCINLINE bool mc_vmdo_setindexfull(mcstate_t* state)
{
    mcvalue_t index;
    mcvalue_t left;
    mcvalue_t nvalue;
    mcvaltype_t lefttype;
    mcvaltype_t indextype;
    index = mc_vm_stackpop(state);
    left = mc_vm_stackpop(state);
    nvalue = mc_vm_stackpop(state);
    lefttype = mc_value_gettype(left);
    indextype = mc_value_gettype(index);
    return mc_vmdo_setindexpartial(state, left, lefttype, index, indextype, nvalue);
}

MCINLINE bool mc_vmdo_getvalueatfull(mcstate_t* state)
{
    int ix;
    int leftlen;
    char resstr[2];
    const char* lefttypename;
    const char* indextypename;
    const char* str;
    mcvaltype_t lefttype;
    mcvaltype_t indextype;
    mcvalue_t index;
    mcvalue_t left;
    mcvalue_t res;
    index = mc_vm_stackpop(state);
    left = mc_vm_stackpop(state);
    lefttype = mc_value_gettype(left);
    indextype= mc_value_gettype(index);
    if(lefttype != MC_VAL_ARRAY && lefttype != MC_VAL_MAP && lefttype != MC_VAL_STRING)
    {
        lefttypename = mc_valtype_getname(lefttype);
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Type %s is not indexable", lefttypename);
        return false;
    }
    res = mc_value_makenull();
    if(indextype != MC_VAL_NUMBER)
    {
        lefttypename = mc_valtype_getname(lefttype);
        indextypename = mc_valtype_getname(indextype);
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Cannot index %s with %s", lefttypename, indextypename);
        return false;
    }
    ix = (int)mc_value_getnumber(index);
    if(lefttype == MC_VAL_ARRAY)
    {
        res = mc_valarray_getvalueat(left, ix);
    }
    else if(lefttype == MC_VAL_MAP)
    {
        res = mc_valmap_getkvpairat(state, left, ix);
    }
    else if(lefttype == MC_VAL_STRING)
    {
        str = mc_valstring_getdata(left);
        leftlen = mc_valstring_getlength(left);
        ix = (int)mc_value_getnumber(index);
        if(ix >= 0 && ix < leftlen)
        {
            resstr[0] = str[ix];
            res = mc_value_makestringlen(state, resstr, 1);
        }
    }
    mc_vm_stackpush(state, res);
    return true;
}

MCINLINE bool mc_vmdo_makefunction(mcstate_t* state, mcbasicarray_t* constants)
{
    int i;
    uint8_t numfree;
    uint16_t constantix;
    const char* fname;
    const char* tname;
    mcvaltype_t constanttype;
    mcvalue_t freeval;
    mcvalue_t functionobj;
    mcvalue_t* constant;
    mcobjfuncscript_t* constfun;
    constantix = mc_callframe_readuint16(state->currframe);
    numfree = mc_callframe_readuint8(state->currframe);
    constant = (mcvalue_t*)mc_basicarray_get(constants, constantix);
    if(!constant)
    {
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Constant %d not found", constantix);
        return false;
    }
    constanttype = mc_value_gettype(*constant);
    if(constanttype != MC_VAL_FUNCSCRIPT)
    {
        tname = mc_valtype_getname(constanttype);
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "%s is not a function", tname);
        return false;
    }
    constfun = mc_value_functiongetscriptfunction(*constant);
    fname = mc_value_functiongetname(*constant);
    functionobj = mc_value_makefuncscript(state, fname, constfun->compiledprogcode, false, constfun->numlocals, constfun->numargs, numfree);
    if(mc_value_isnull(functionobj))
    {
        return false;
    }
    for(i = 0; i < numfree; i++)
    {
        freeval = mc_vallist_get(state->valuestack, state->vsposition - numfree + i);
        mc_value_functionsetfreevalat(functionobj, i, freeval);
    }
    mc_vm_setstackpos(state, state->vsposition - numfree);
    mc_vm_stackpush(state, functionobj);
    return true;
}

MCINLINE bool mc_vmdo_docmpvalue(mcstate_t* state, mcopcode_t opcode)
{
    bool ok;
    bool isoverloaded;
    const char* lefttname;
    const char* righttname;
    mcvalcmpresult_t cres;
    mcvalue_t res;
    mcvalue_t left;
    mcvalue_t right;
    right = mc_vm_stackpop(state);
    left = mc_vm_stackpop(state);
    isoverloaded = false;
    ok = mc_vmdo_tryoverloadoperator(state, left, right, MC_OPCODE_COMPARE, &isoverloaded);
    //fprintf(stderr, "docmpvalue: ok=%d isoverloaded=%d\n", ok, isoverloaded);
    if(!ok)
    {
        return false;
    }
    if(!isoverloaded)
    {
        ok = mc_value_compare(left, right, &cres);
        #if 0
        fprintf(stderr, "compare: ok=%d cres.result=%g\n", ok, cres.result);
        #endif
        if((ok == true) || (opcode == MC_OPCODE_COMPAREEQ))
        {
            res = mc_value_makenumber(cres.result);
            mc_vm_stackpush(state, res);
        }
        else
        {
            righttname = mc_valtype_getname(mc_value_gettype(right));
            lefttname = mc_valtype_getname(mc_value_gettype(left));
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "cannot compare %s and %s", lefttname, righttname);
            return false;
        }
    }
    return true;
}

MCINLINE bool mc_vmdo_docmpvalgreater(mcstate_t* state, mcopcode_t opcode)
{
    bool resval;
    mcfloat_t comparisonres;
    mcvalue_t res;
    mcvalue_t value;
    value = mc_vm_stackpop(state);
    comparisonres = mc_value_getnumber(value);
    resval = false;
    switch(opcode)
    {
        case MC_OPCODE_EQUAL:
            {
                resval = MC_UTIL_CMPFLOAT(comparisonres, 0);
            }
            break;
        case MC_OPCODE_NOTEQUAL:
            {
                resval = !MC_UTIL_CMPFLOAT(comparisonres, 0);
            }
            break;
        case MC_OPCODE_GREATERTHAN:
            {
                resval = comparisonres > 0;
            }
            break;
        case MC_OPCODE_GREATERTHANEQUAL:
            {
                resval = comparisonres > 0 || MC_UTIL_CMPFLOAT(comparisonres, 0);
            }
            break;
        default:
            {
                MC_ASSERT(false);
            }
            break;
    }
    res = mc_value_makebool(resval);
    mc_vm_stackpush(state, res);
    return true;
}

MCINLINE bool mc_vmdo_makearray(mcstate_t* state)
{
    bool ok;
    int i;
    uint16_t count;
    mcvalue_t item;
    mcvalue_t arrayobj;
    mcvalue_t* items;
    count = mc_callframe_readuint16(state->currframe);
    arrayobj = mc_value_makearraycapacity(state, count);
    if(mc_value_isnull(arrayobj))
    {
        return false;
    }
    items = state->valuestack->listitems + state->vsposition - count;
    for(i = 0; i < count; i++)
    {
        item = items[i];
        ok = mc_valarray_push(arrayobj, item);
        if(!ok)
        {
            return false;
        }
    }
    mc_vm_setstackpos(state, state->vsposition - count);
    mc_vm_stackpush(state, arrayobj);
    return true;
}

MCINLINE bool mc_vmdo_makemapstart(mcstate_t* state)
{
    uint16_t count;
    mcvalue_t mapobj;
    count = mc_callframe_readuint16(state->currframe);
    mapobj = mc_value_makemapcapacity(state, count);
    if(mc_value_isnull(mapobj))
    {
        return false;
    }
    mc_vm_thisstackpush(state, mapobj);
    return true;
}

MCINLINE bool mc_vmdo_makemapend(mcstate_t* state)
{
    bool ok;
    int i;
    uint16_t kvpcount;
    uint16_t itemscount;
    const char* keytypename;
    mcvaltype_t keytype;
    mcvalue_t key;
    mcvalue_t val;
    mcvalue_t mapobj;
    mcvalue_t* kvpairs;
    kvpcount = mc_callframe_readuint16(state->currframe);
    itemscount = kvpcount * 2;
    mapobj = mc_vm_thisstackpop(state);
    kvpairs = state->valuestack->listitems + state->vsposition - itemscount;
    for(i = 0; i < itemscount; i += 2)
    {
        key = kvpairs[i];
        if(!mc_value_ishashable(key))
        {
            keytype = mc_value_gettype(key);
            keytypename = mc_valtype_getname(keytype);
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Key of type %s is not hashable", keytypename);
            return false;
        }
        val = kvpairs[i + 1];
        ok = mc_valmap_setvalue(mapobj, key, val);
        if(!ok)
        {
            return false;
        }
    }
    mc_vm_setstackpos(state, state->vsposition - itemscount);
    mc_vm_stackpush(state, mapobj);
    return true;
}

#if 1
    #define mc_vmmac_break() \
        goto readnextop
#else
    #define mc_vmmac_break() \
        break
#endif

void mc_vmutil_getopinfo(mcopcode_t opc, const char** oname)
{
    mcopdefinition_t vdef;
    mcopdefinition_t* def;
    *oname = "!invalid!";
    def = mc_opdef_lookup(&vdef, opc);
    if(def != NULL)
    {
        *oname = def->name;
    }
}

bool mc_function_execfunction(mcstate_t* state, mcvalue_t function, mcbasicarray_t* constants)
{
    bool ok;
    int fri;
    int recoverframeix;
    int prevcode;
    int opcode;
    mcvmframe_t createdframe;
    mcvalue_t errobj;
    mcvmframe_t* frame;
    mcerror_t* err;    
    const char* oname;
    mcobjfuncscript_t* functionfunction;
    if(state->running)
    {
        mc_errlist_push(&state->errors, MC_ERROR_USER, srcposinvalid, "VM is already executing code");
        return false;
    }
    /* naming is hard */
    functionfunction = mc_value_functiongetscriptfunction(function);
    ok = false;
    ok = mc_callframe_init(&createdframe, function, state->vsposition - functionfunction->numargs);
    if(!ok)
    {
        fprintf(stderr, "failed to init frames!\n");
        return false;
    }
    ok = mc_vm_pushframe(state, createdframe);
    if(!ok)
    {
        mc_errlist_push(&state->errors, MC_ERROR_USER, srcposinvalid, "Pushing frame failed");
        return false;
    }
    fprintf(stderr, "**executing function**\n");
    state->running = true;
    state->lastpopped = mc_value_makenull();
    while(state->currframe->bcposition < state->currframe->bcsize)
    {
        readnextop:
        prevcode = opcode;
        opcode = mc_callframe_readopcode(state->currframe);
        #if 0
            mc_vmutil_getopinfo(opcode, &oname);
            fprintf(stderr, "opcode=%d (%s)\n", opcode, oname);
        #endif
        switch(opcode)
        {
            default:
                {
                    const char* prevname;
                    const char* thisname;
                    mc_vmutil_getopinfo(opcode, &thisname);
                    mc_vmutil_getopinfo(prevcode, &prevname);
                    mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Unknown opcode: %d (%s) (previous opcode was %d (%s))", opcode, thisname, prevcode, prevname);
                    MC_ASSERT(false);
                    goto onexecerror;
                }
                break;
            case MC_OPCODE_HALT:
                {
                    goto onexecfinish;
                }
                break;
            case MC_OPCODE_RETURNVALUE:
                {
                    mcvalue_t res;
                    res = mc_vm_stackpop(state);
                    ok = mc_vm_popframe(state);
                    if(!ok)
                    {
                        goto onexecfinish;
                    }
                    mc_vm_stackpush(state, res);
                }
                break;
            case MC_OPCODE_RETURN:
                {
                    ok = mc_vm_popframe(state);
                    mc_vm_stackpush(state, mc_value_makenull());
                    if(!ok)
                    {
                        mc_vm_stackpop(state);
                        goto onexecfinish;
                    }
                }
                break;
            case MC_OPCODE_CONSTANT:
                {
                    uint16_t constantix;
                    mcvalue_t* constant;
                    constantix = mc_callframe_readuint16(state->currframe);
                    constant = (mcvalue_t*)mc_basicarray_get(constants, constantix);
                    if(!constant)
                    {
                        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Constant at %d not found", constantix);
                        goto onexecerror;
                    }
                    mc_vm_stackpush(state, *constant);
                }
                mc_vmmac_break();
            case MC_OPCODE_ADD:
            case MC_OPCODE_SUB:
            case MC_OPCODE_MUL:
            case MC_OPCODE_DIV:
            case MC_OPCODE_MOD:
            case MC_OPCODE_BINOR:
            case MC_OPCODE_BINXOR:
            case MC_OPCODE_BINAND:
            case MC_OPCODE_LSHIFT:
            case MC_OPCODE_RSHIFT:
                {
                    if(!mc_vmdo_math(state, opcode))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_POP:
                {
                    mc_vm_stackpop(state);
                }
                mc_vmmac_break();
            case MC_OPCODE_TRUE:
                {
                    mc_vm_stackpush(state, mc_value_makebool(true));
                }
                mc_vmmac_break();
            case MC_OPCODE_FALSE:
                {
                    mc_vm_stackpush(state, mc_value_makebool(false));
                }
                mc_vmmac_break();
            case MC_OPCODE_COMPARE:
            case MC_OPCODE_COMPAREEQ:
                {
                    if(!mc_vmdo_docmpvalue(state, opcode))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_EQUAL:
            case MC_OPCODE_NOTEQUAL:
            case MC_OPCODE_GREATERTHAN:
            case MC_OPCODE_GREATERTHANEQUAL:
                {
                    if(!mc_vmdo_docmpvalgreater(state, opcode))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_MINUS:
                {
                    bool overloadfound;
                    mcfloat_t val;
                    const char* opertname;
                    mcvalue_t res;
                    mcvaltype_t opertype;
                    mcvalue_t operand;
                    operand = mc_vm_stackpop(state);
                    opertype = mc_value_gettype(operand);
                    if(opertype == MC_VAL_NUMBER)
                    {
                        val = mc_value_getnumber(operand);
                        res = mc_value_makenumber(-val);
                        mc_vm_stackpush(state, res);
                    }
                    else
                    {
                        overloadfound = false;
                        ok = mc_vmdo_tryoverloadoperator(state, operand, mc_value_makenull(), MC_OPCODE_MINUS, &overloadfound);
                        if(!ok)
                        {
                            goto onexecerror;
                        }
                        if(!overloadfound)
                        {
                            opertname = mc_valtype_getname(opertype);
                            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Invalid operand type for MINUS, got %s", opertname);
                            goto onexecerror;
                        }
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_BINNOT:
                {
                    bool overloadfound;
                    int64_t val;
                    const char* opertname;
                    mcvalue_t res;
                    mcvaltype_t opertype;
                    mcvalue_t operand;
                    operand = mc_vm_stackpop(state);
                    opertype = mc_value_gettype(operand);
                    if(opertype == MC_VAL_NUMBER)
                    {
                        val = mc_value_getnumber(operand);
                        res = mc_value_makenumber(~val);
                        mc_vm_stackpush(state, res);
                    }
                    else
                    {
                        overloadfound = false;
                        ok = mc_vmdo_tryoverloadoperator(state, operand, mc_value_makenull(), MC_OPCODE_BINNOT, &overloadfound);
                        if(!ok)
                        {
                            goto onexecerror;
                        }
                        if(!overloadfound)
                        {
                            opertname = mc_valtype_getname(opertype);
                            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Invalid operand type for BINNOT, got %s", opertname);
                            goto onexecerror;
                        }
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_BANG:
                {
                    bool overloadfound;
                    mcvalue_t res;
                    mcvalue_t operand;
                    mcvaltype_t type;
                    operand = mc_vm_stackpop(state);
                    type = mc_value_gettype(operand);
                    if(type == MC_VAL_BOOL)
                    {
                        res = mc_value_makebool(!mc_value_getbool(operand));
                        mc_vm_stackpush(state, res);
                    }
                    else if(type == MC_VAL_NULL)
                    {
                        res = mc_value_makebool(true);
                        mc_vm_stackpush(state, res);
                    }
                    else
                    {
                        overloadfound = false;
                        ok = mc_vmdo_tryoverloadoperator(state, operand, mc_value_makenull(), MC_OPCODE_BANG, &overloadfound);
                        if(!ok)
                        {
                            goto onexecerror;
                        }
                        if(!overloadfound)
                        {
                            res = mc_value_makebool(false);
                            mc_vm_stackpush(state, res);
                        }
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_JUMP:
                {
                    uint16_t pos;
                    pos = mc_callframe_readuint16(state->currframe);
                    state->currframe->bcposition = pos;
                }
                mc_vmmac_break();
            case MC_OPCODE_JUMPIFFALSE:
                {
                    uint16_t pos;
                    mcvalue_t test;
                    pos = mc_callframe_readuint16(state->currframe);
                    test = mc_vm_stackpop(state);
                    if(!mc_value_getbool(test))
                    {
                        state->currframe->bcposition = pos;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_JUMPIFTRUE:
                {
                    uint16_t pos;
                    mcvalue_t test;
                    pos = mc_callframe_readuint16(state->currframe);
                    test = mc_vm_stackpop(state);
                    if(mc_value_getbool(test))
                    {
                        state->currframe->bcposition = pos;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_NULL:
                {
                    mc_vm_stackpush(state, mc_value_makenull());
                }
                mc_vmmac_break();
            case MC_OPCODE_DEFINEMODULEGLOBAL:
                {
                    uint16_t ix;
                    mcvalue_t value;
                    ix = mc_callframe_readuint16(state->currframe);
                    value = mc_vm_stackpop(state);
                    mc_vm_setglobalbyindex(state, ix, value);
                }
                mc_vmmac_break();
            case MC_OPCODE_SETMODULEGLOBAL:
                {
                    uint16_t ix;
                    mcvalue_t nvalue;
                    mcvalue_t oldvalue;
                    ix = mc_callframe_readuint16(state->currframe);
                    nvalue = mc_vm_stackpop(state);
                    oldvalue= mc_vm_getglobalbyindex(state, ix);
                    if(!mc_vm_checkassign(state, oldvalue, nvalue))
                    {
                        goto onexecerror;
                    }
                    mc_vm_setglobalbyindex(state, ix, nvalue);
                }
                mc_vmmac_break();
            case MC_OPCODE_GETMODULEGLOBAL:
                {
                    uint16_t ix;
                    mcvalue_t global;
                    ix = mc_callframe_readuint16(state->currframe);
                    global = state->globalvalstack[ix];
                    mc_vm_stackpush(state, global);
                }
                mc_vmmac_break();
            case MC_OPCODE_ARRAY:
                {
                    if(!mc_vmdo_makearray(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_MAPSTART:
                {
                    if(!mc_vmdo_makemapstart(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_MAPEND:
                {
                    if(!mc_vmdo_makemapend(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_GETVALUEAT:
                {
                    if(!mc_vmdo_getvalueatfull(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_CALL:
                {
                    uint8_t nargs;
                    mcvalue_t callee;
                    nargs = mc_callframe_readuint8(state->currframe);
                    callee = mc_vm_stackget(state, nargs);
                    ok = mc_vmdo_callobject(state, callee, nargs);
                    if(!ok)
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_DEFINELOCAL:
                {
                    uint8_t pos;
                    pos = mc_callframe_readuint8(state->currframe);
                    mc_vallist_set(state->valuestack, state->currframe->basepointer + pos, mc_vm_stackpop(state));
                }
                mc_vmmac_break();
            case MC_OPCODE_SETLOCAL:
                {
                    uint8_t pos;
                    mcvalue_t nvalue;
                    mcvalue_t oldvalue;
                    pos = mc_callframe_readuint8(state->currframe);
                    nvalue = mc_vm_stackpop(state);
                    oldvalue = mc_vallist_get(state->valuestack, state->currframe->basepointer + pos);
                    if(!mc_vm_checkassign(state, oldvalue, nvalue))
                    {
                        goto onexecerror;
                    }
                    mc_vallist_set(state->valuestack, state->currframe->basepointer + pos, nvalue);
                }
                mc_vmmac_break();
            case MC_OPCODE_GETLOCAL:
                {
                    uint8_t pos;
                    mcvalue_t val;
                    pos = mc_callframe_readuint8(state->currframe);
                    val = mc_vallist_get(state->valuestack, state->currframe->basepointer + pos);
                    mc_vm_stackpush(state, val);
                }
                mc_vmmac_break();
            case MC_OPCODE_GETGLOBALBUILTIN:
                {
                    uint16_t ix;
                    mcvalue_t val;
                    ix = mc_callframe_readuint16(state->currframe);
                    ok = false;
                    val = mc_globalstore_getatindex(state->vmglobalstore, ix, &ok);
                    if(!ok)
                    {
                        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Global value %d not found", ix);
                        goto onexecerror;
                    }
                    mc_vm_stackpush(state, val);
                }
                mc_vmmac_break();

            case MC_OPCODE_FUNCTION:
                {
                    if(!mc_vmdo_makefunction(state, constants))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_GETFREE:
                {
                    uint8_t freeix;
                    mcvalue_t val;
                    freeix = mc_callframe_readuint8(state->currframe);
                    val = mc_value_functiongetfreevalat(state->currframe->function, freeix);
                    mc_vm_stackpush(state, val);
                }
                mc_vmmac_break();
            case MC_OPCODE_SETFREE:
                {
                    uint8_t freeix;
                    mcvalue_t val;
                    freeix = mc_callframe_readuint8(state->currframe);
                    val = mc_vm_stackpop(state);
                    mc_value_functionsetfreevalat(state->currframe->function, freeix, val);
                }
                mc_vmmac_break();
            case MC_OPCODE_CURRENTFUNCTION:
                {
                    mcvalue_t currentfunction;
                    currentfunction = state->currframe->function;
                    mc_vm_stackpush(state, currentfunction);
                }
                mc_vmmac_break();
            case MC_OPCODE_GETTHIS:
                {
                    mcvalue_t obj;
                    obj = mc_vm_thisstackget(state, 0);
                    mc_vm_stackpush(state, obj);
                }
                mc_vmmac_break();
            case MC_OPCODE_GETDOTINDEX:
                {
                    if(!mc_vmdo_getdotindex(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_GETINDEX:
                {
                    if(!mc_vmdo_getindexfull(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_SETINDEX:
                {
                    if(!mc_vmdo_setindexfull(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            case MC_OPCODE_DUP:
                {
                    mcvalue_t val;
                    val = mc_vm_stackget(state, 0);
                    mc_vm_stackpush(state, val);
                }
                mc_vmmac_break();
            case MC_OPCODE_FOREACHLEN:
                {
                    int len;
                    const char* tname;
                    mcvalue_t val;
                    mcvaltype_t type;
                    val = mc_vm_stackpop(state);
                    len = 0;
                    type = mc_value_gettype(val);
                    if(type == MC_VAL_ARRAY)
                    {
                        len = mc_valarray_getlength(val);
                    }
                    else if(type == MC_VAL_MAP)
                    {
                        len = mc_valmap_getlength(val);
                    }
                    else if(type == MC_VAL_STRING)
                    {
                        len = mc_valstring_getlength(val);
                    }
                    else
                    {
                        tname = mc_valtype_getname(type);
                        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->currframe), "Cannot get length of %s", tname);
                        goto onexecerror;
                    }
                    mc_vm_stackpush(state, mc_value_makenumber(len));
                }
                mc_vmmac_break();
            case MC_OPCODE_NUMBER:
                {
                    uint64_t val;
                    mcfloat_t dval;
                    mcvalue_t obj;
                    val = mc_callframe_readuint64(state->currframe);
                    dval = mc_util_uint64todouble(val);
                    obj = mc_value_makenumber(dval);
                    mc_vm_stackpush(state, obj);
                }
                mc_vmmac_break();
            case MC_OPCODE_SETRECOVER:
                {
                    uint16_t recip;
                    recip = mc_callframe_readuint16(state->currframe);
                    state->currframe->recoverip = recip;
                }
                mc_vmmac_break();
        }
    onexecerror:
        if(state->errors.count > 0)
        {
            err = mc_errlist_getlast(&state->errors);
            if(err->type == MC_ERROR_RUNTIME && state->errors.count == 1)
            {
                recoverframeix = -1;
                for(fri = state->framecount - 1; fri >= 0; fri--)
                {
                    frame = mc_framelist_get(state->framestack, fri);
                    if(frame->recoverip >= 0 && !frame->isrecovering)
                    {
                        recoverframeix = fri;
                        break;
                    }
                }
                if(recoverframeix < 0)
                {
                    goto onexecfinish;
                }
                if(!err->traceback)
                {
                    err->traceback = mc_traceback_make(state);
                }
                if(err->traceback)
                {
                    mc_traceback_vmpush(err->traceback, state);
                }
                while(state->framecount > (recoverframeix + 1))
                {
                    mc_vm_popframe(state);
                }
                errobj = mc_value_makeerror(state, err->message);
                if(!mc_value_isnull(errobj))
                {
                    mc_value_errorsettraceback(errobj, err->traceback);
                    err->traceback = NULL;
                }
                mc_vm_stackpush(state, errobj);
                state->currframe->bcposition = state->currframe->recoverip;
                state->currframe->isrecovering = true;
                mc_errlist_clear(&state->errors);
            }
            else
            {
                goto onexecfinish;
            }
        }
        if(mc_state_gcshouldsweep(state))
        {
            mc_vm_rungc(state, constants);
        }
    }

onexecfinish:
    if(state->errors.count > 0)
    {
        err = mc_errlist_getlast(&state->errors);
        if(!err->traceback)
        {
            err->traceback = mc_traceback_make(state);
        }
        if(err->traceback)
        {
            mc_traceback_vmpush(err->traceback, state);
        }
    }
    mc_vm_rungc(state, constants);
    state->running = false;
    return state->errors.count == 0;
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
            optprs_fprintmaybearg(out, "--", flag->longname, mc_util_strlen(flag->longname), needval, maybeval, "=");
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
    mccompiledprogram_t* program;
    ok = false;
    program = mc_state_compilesource(state, source);
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
    size_t len;
    char* code;
    code = mc_util_readfile(filename, &len);
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
    argvobj = mc_value_makearray(state);
    for(i=beginat; i<argc; i++)
    {
        strval = mc_value_makestring(state, argv[i]);
        mc_valarray_push(argvobj, strval);
    }
    mc_state_setglobalconstant(state, "ARGV", argvobj);
}

void mc_cli_installbuiltins(mcstate_t* state)
{
    static struct
    {
        const char* name;
        mcnativefn_t fn;
    } nativefunctions[] = {
        { "maketestdict", mc_scriptfn_maketestdict },
        { "squarearray", mc_scriptfn_squarearray },
        { "lengthof", mc_scriptfn_lengthof },
        { "typeof", mc_scriptfn_typeof },        
        { "binnot", mc_scriptfn_binnot},
        { "ord", mc_scriptfn_ord},
        { "println", mc_scriptfn_println },
        { "print", mc_scriptfn_print },
        { "readfile", mc_scriptfn_filereadfile },
        { "writefile", mc_scriptfn_filewritefile },
        { "arrayfirst", mc_scriptfn_arrayfirst },
        { "arraylast", mc_scriptfn_arraylast },
        { "arrayrest", mc_scriptfn_arrayrest },
        { "arrayjoin", mc_scriptfn_arrayjoin},
        { "arraypush", mc_scriptfn_arraypush },
        { "arraypop", mc_scriptfn_arraypop },
        { "remove", mc_scriptfn_remove },
        { "removeat", mc_scriptfn_removeat },
        { "tostring", mc_scriptfn_tostring },
        { "tonum", mc_scriptfn_tonum },
        { "strtoint", mc_scriptfn_tonum },
        { "isNaN", mc_scriptfn_isnan },
        { "range", mc_scriptfn_range },
        { "keys", mc_scriptfn_keys },
        { "values", mc_scriptfn_values },
        { "copy", mc_scriptfn_copy },
        { "deepcopy", mc_scriptfn_copydeep },
        { "chr", mc_scriptfn_chr },
        { "reverse", mc_scriptfn_reverse },
        { "array", mc_scriptfn_array },
        { "error", mc_scriptfn_error },
        { "crash", mc_scriptfn_crash },
        { "assert", mc_scriptfn_assert },
        { "randomseed", mc_scriptfn_randseed },
        { "random", mc_scriptfn_random },
        { "slice", mc_scriptfn_slice },

        /* Custom */
        { "indexof", mc_scriptfn_index },
        { "left", mc_scriptfn_left },
        { "right", mc_scriptfn_right },
        { "replace", mc_scriptfn_replace },
        { "replacefirst", mc_scriptfn_replacefirst },
        { "trim", mc_scriptfn_trim },

        /* Type checks */
        { "isstring", mc_scriptfn_isstring },
        { "isarray", mc_scriptfn_isarray },
        { "ismap", mc_scriptfn_ismap },
        { "isnumber", mc_scriptfn_isnumber },
        { "isbool", mc_scriptfn_isbool },
        { "isnull", mc_scriptfn_isnull },
        { "isfunction", mc_scriptfn_isfunction },
        { "isexternal", mc_scriptfn_isexternal },
        { "iserror", mc_scriptfn_iserror },
        { "isnativefunction", mc_scriptfn_isnativefunction },

        /* Math */
        { "sqrt", mc_scriptfn_sqrt },
        { "pow", mc_scriptfn_pow },
        { "sin", mc_scriptfn_sin },
        { "cos", mc_scriptfn_cos },
        { "tan", mc_scriptfn_tan },
        { "log", mc_scriptfn_log },
        { "ceil", mc_scriptfn_ceil },
        { "floor", mc_scriptfn_floor },
        { "abs", mc_scriptfn_abs },
        {NULL, NULL}
    };
    int i;
    for(i=0; nativefunctions[i].name != NULL; i++)
    {
        mc_state_setnativefunction(state, nativefunctions[i].name, nativefunctions[i].fn, NULL, 0);
    }
}

/*
TODO:
kind-of ruby-ish. so sue me.
to add:
File.read: data = File.read("somefile"[, how-much-in-bytes])
File.write: File.write("somefile", arrayjoin(somestuff, "\n"))
File.exists
...

*/

void mc_cli_installjsondummy(mcstate_t* state)
{
    mcvalue_t jmap;
    jmap = mc_value_makemap(state);
    mc_valmap_setvalstring(jmap, "stringify", mc_value_makefuncnative(state, "stringify", mc_scriptfn_jsonstringify, NULL, 0));
    mc_state_setglobalconstant(state, "JSON", jmap);
}

void mc_cli_installjsconsole(mcstate_t* state)
{
    mcvalue_t jmap;
    jmap = mc_value_makemap(state);
    mc_valmap_setvalstring(jmap, "log", mc_value_makefuncnative(state, "log", mc_scriptfn_println, NULL, 0));
    mc_state_setglobalconstant(state, "console", jmap);
}

void mc_cli_installfauxjavascript(mcstate_t* state)
{
    mc_cli_installjsondummy(state);
    mc_cli_installjsconsole(state);
}

void mc_cli_installfileio(mcstate_t* state)
{
    mcvalue_t map;
    map = mc_value_makemap(state);
    mc_valmap_setvalstring(map, "read", mc_value_makefuncnative(state, "read", mc_scriptfn_filereadfile, NULL, 0));
    mc_valmap_setvalstring(map, "write", mc_value_makefuncnative(state, "write", mc_scriptfn_filewritefile, NULL, 0));
    mc_state_setglobalconstant(state, "File", map);
}

void mc_cli_installotherstuff(mcstate_t* state)
{
    mc_state_setglobalconstant(state, "test", mc_value_makenumber(42));
    mc_state_setnativefunction(state, "external_fn_test", mc_scriptfn_externalfn, &g_extfnvar, sizeof(g_extfnvar));
    mc_state_setnativefunction(state, "test_check_args", mc_scriptfn_testcheckargs, NULL, 0);
    mc_state_setnativefunction(state, "vec2_add", mc_scriptfn_vec2add, NULL, 0);
    mc_state_setnativefunction(state, "vec2_sub", mc_scriptfn_vec2sub, NULL, 0);
    mc_cli_installfileio(state);
}

#define printtypesize(typ) \
    mc_cli_printtypesize(#typ, sizeof(typ))

void mc_cli_printtypesize(const char* name, size_t sz)
{
    printf("%ld\t%s\n", sz, name);
}

void mc_cli_printtypesizes()
{
    printtypesize(mcgenericdict_t);
    printtypesize(mcvaldict_t);
    printtypesize(mcbasicarray_t);
    printtypesize(mcptrlist_t);
    printtypesize(mcprintconfig_t);
    printtypesize(mcprinter_t);
    printtypesize(mcerror_t);
    printtypesize(mctraceback_t);
    printtypesize(mcastcompiledfile_t);
    printtypesize(mcastexpression_t);
    printtypesize(mccompiledprogram_t);
    printtypesize(mcstate_t);
    printtypesize(mcgcmemory_t);
    printtypesize(mcglobalstore_t);
    printtypesize(mcobjdata_t);
    printtypesize(mcerrlist_t);
    printtypesize(mcastparser_t);
    printtypesize(mcconfig_t);
    printtypesize(mcastsymtable_t);
    printtypesize(mcastcompiler_t);
    printtypesize(mcastsymbol_t);
    printtypesize(mcvalue_t);
    printtypesize(mcvalcmpresult_t);
    printtypesize(mcastlocation_t);
    printtypesize(mcasttoken_t);
    printtypesize(mcastcodeblock_t);
    printtypesize(mcastliteralmap_t);
    printtypesize(mcastliteralarray_t);
    printtypesize(mcastliteralstring_t);
    printtypesize(mcastexprprefix_t);
    printtypesize(mcastexprinfix_t);
    printtypesize(mcastifcase_t);
    printtypesize(mcastliteralfunction_t);
    printtypesize(mcastexprcall_t);
    printtypesize(mcastexprindex_t);
    printtypesize(mcastexprassign_t);
    printtypesize(mcastexprlogical_t);
    printtypesize(mcastexprternary_t);
    printtypesize(mcastident_t);
    printtypesize(mcastfuncparam_t);
    printtypesize(mcastexprdefine_t);
    printtypesize(mcastexprstmtif_t);
    printtypesize(mcastexprstmtwhile_t);
    printtypesize(mcastexprstmtforeach_t);
    printtypesize(mcastexprstmtforloop_t);
    printtypesize(mcastexprstmtimport_t);
    printtypesize(mcastexprstmtrecover_t);
    printtypesize(mcobjfuncscript_t);
    printtypesize(mcobjfuncnative_t);
    printtypesize(mcobjuserdata_t);
    printtypesize(mcobjerror_t);
    printtypesize(mcobjstring_t);
    printtypesize(mcopdefinition_t);
    printtypesize(mcastscopeblock_t);
    printtypesize(mcastscopefile_t);
    printtypesize(mcastscopecomp_t);
    printtypesize(mcgcobjdatapool_t);
    printtypesize(mcastlexer_t);
    printtypesize(mcastlexprevinfo_t);
    printtypesize(mcvmframe_t);
    printtypesize(mctraceitem_t);
    printtypesize(module_t);
    printtypesize(mcstoddiyfp_t);
    printtypesize(mcstoddiyfpconv_t);
    printtypesize(mcvalunion_t);
    printtypesize(mcobjunion_t);
    printtypesize(mcexprunion_t);
    printtypesize(mcfuncfvunion_t);
    printtypesize(mcfuncnameunion_t);

}

static optlongflags_t longopts[] =
{
    {"help", 'h', OPTPARSE_NONE, "this help"},
    {"printsizes", 't', OPTPARSE_NONE, "print type sizes"},
    {"eval", 'e', OPTPARSE_REQUIRED, "evaluate a single line of code"},
    {"dumpast", 'a', OPTPARSE_NONE, "dump AST after parsing"},
    {"dumpbc", 'd', OPTPARSE_NONE, "dump bytecode after compiling"},
    {0, 0, (optargtype_t)0, NULL}
};

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
    state = mc_state_make();
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
        else if(co == 'a')
        {
            state->config.dumpast = true;
        }
        else if(co == 'd')
        {
            state->config.dumpbytecode = true;
        }
        else if(co == 't')
        {
            mc_cli_printtypesizes();
            goto finished;
        }
    }
    /*
    * IMPORTANT:
    * when '-e' is specified, the structure of ARGV would /not/ include the callee (argv[0]),
    * which in scripts, would be the script file. i.e., in 'foo.mc' ARGV[0] would be 'foo.mc', et cetera.
    * this merely fills that spot whenever '-e' is being used.
    * don't remove the angle brackets either; so it doesn't get errornously picked up
    * as an option in some other place.
    * the down-cast from const is perfectly legal here; the string is never modified.
    */
    if(evalcode != NULL)
    {
        nargv[0] = (char*)"<-e>";
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
    mc_cli_installbuiltins(state);
    mc_cli_installotherstuff(state);
    mc_cli_installfauxjavascript(state);
    if(evalcode != NULL)
    {
        mc_cli_compileandrunsource(state, &tmp, evalcode);
    }
    else
    {
        if(nargc > 0)
        {
            if(!mc_cli_compileandrunfile(state, nargv[0]))
            {
                ok = false;
            }
        }
        else
        {
            /*
            if(nargc == 0)
            {
                mc_cli_runrepl(state);
            }
            */
            ok = false;
        }
    }
    finished:
    mc_state_destroy(state);
    fprintf(stderr, "ok=%d\n", ok);
    if(ok)
    {
        return 0;
    }
    return 1;
}

