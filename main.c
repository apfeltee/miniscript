
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
#define MC_CONF_MINVMVALSTACKSIZE (4)
#define MC_CONF_MINVMFRAMES (4)

#define MC_CONF_MAXVMGLOBALS (4)
#define MC_CONF_MINNATIVETHISSTACKSIZE (1024)
#define MC_CONF_MINVMTHISSTACKSIZE (MC_CONF_MINVMVALSTACKSIZE)
#define MC_CONF_GCMEMPOOLSIZE (2048/16)
#define MC_CONF_GCMEMPOOLCOUNT (3)
#define MC_CONF_GCMEMSWEEPINTERVAL (128)
#define MC_CONF_MAXERRORCOUNT (4)
#define MC_CONF_MAXERRORMSGLENGTH (128)
#define MC_CONF_GENERICDICTINVALIDIX (UINT_MAX)
#define MC_CONF_VALDICTINVALIDIX (UINT_MAX)
#define MC_CONF_GENERICDICTINITSIZE (32)

#define MC_CONF_MAXOPEROVERLOADS (25)

#ifdef _MSC_VER
    #define __attribute__(x)
#endif


#if (defined(__GNUC__) || defined(__clang__)) && !defined(__STRICT_ANSI__)
    #define MC_INLINE static inline
    #define MC_FORCEINLINE MC_INLINE __attribute__((always_inline))
#else
    #define MC_INLINE static
    #define MC_FORCEINLINE MC_INLINE
#endif

#if defined(__STRICT_ANSI__)
    #define inline
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

#define MC_ASSERT(x) mc_util_assert((x), #x, __FILE__, __LINE__, NULL)
#define MC_ASSERTF(x, ...) mc_util_assert((x), #x, __FILE__, __LINE__, __VA_ARGS__)

#if defined(__GNUC__) || defined(__clang__)
    #define mc_util_likely(x)   (__builtin_expect(!!(x), 1))
    #define mc_util_unlikely(x) (__builtin_expect(!!(x), 0))
#else
    #define mc_util_likely(x)   (x)
    #define mc_util_unlikely(x) (x)
#endif

#define mc_value_gettype(v) (v).valtype

#if 1
    #define MC_UTIL_INCCAPACITY(capacity) (((capacity) < 8) ? 8 : ((capacity) * 2))
#else
    #define MC_UTIL_INCCAPACITY(capacity) ((capacity) + 15) / 16 * 16;
#endif


#define mc_value_getallocateddata(object) (object).uval.odata


typedef double mcfloat_t;
typedef uint16_t mcinternopcode_t;
typedef uint32_t mcshiftint_t;

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

typedef struct mcstoddiyfp_t mcstoddiyfp_t;
typedef struct mcptrdict_t mcptrdict_t;
typedef struct mcvaldict_t mcvaldict_t;
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
typedef struct mcexecstate_t mcexecstate_t;
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
typedef struct mcastexprcodeblock_t mcastexprcodeblock_t;
typedef struct mcastexprliteralmap_t mcastexprliteralmap_t;
typedef struct mcastexprliteralarray_t mcastexprliteralarray_t;
typedef struct mcastexprliteralstring_t mcastexprliteralstring_t;
typedef struct mcastexprprefix_t mcastexprprefix_t;
typedef struct mcastexprinfix_t mcastexprinfix_t;
typedef struct mcastexprifcase_t mcastexprifcase_t;
typedef struct mcastexprliteralfunction_t mcastexprliteralfunction_t;
typedef struct mcastexprcall_t mcastexprcall_t;
typedef struct mcastexprindex_t mcastexprindex_t;
typedef struct mcastexprassign_t mcastexprassign_t;
typedef struct mcastexprlogical_t mcastexprlogical_t;
typedef struct mcastexprternary_t mcastexprternary_t;
typedef struct mcastexprident_t mcastexprident_t;
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
typedef struct mcmodule_t mcmodule_t;

typedef union mcstoddiyfpconv_t mcstoddiyfpconv_t;
typedef union mcvalunion_t mcvalunion_t;
typedef union mcobjunion_t mcobjunion_t;
typedef union mcexprunion_t mcexprunion_t;
typedef union mcfuncfvunion_t mcfuncfvunion_t;
typedef union mcfuncnameunion_t mcfuncnameunion_t;
typedef struct /**/mcvallist_t mcvallist_t;
typedef struct /**/mcframelist_t mcframelist_t;
typedef struct /**/mcclass_t mcclass_t;
typedef struct /**/mcfield_t mcfield_t;
typedef struct mcconsolecolor_t mcconsolecolor_t;

typedef mcvalue_t (*mcnativefn_t)(mcstate_t*, void*, mcvalue_t, size_t, mcvalue_t*);
typedef size_t (*mcitemhashfn_t)(void*);
typedef bool (*mcitemcomparefn_t)(void*, void*);
typedef void (*mcitemdestroyfn_t)(void*);
typedef void* (*mcitemcopyfn_t)(void*);
typedef void (*mcitemdeinitfn_t)(void*);
typedef mcastexpression_t* (*mcastrightassocparsefn_t)(mcastparser_t*);
typedef mcastexpression_t* (*mcleftassocparsefn_t)(mcastparser_t*, mcastexpression_t*);



struct mcconsolecolor_t
{
    int targetfds[5];
    size_t fdcount;
    bool ispiped;
};


/**
 * \brief The execution environment for an instance of the script engine.
 */

union mcvalunion_t
{
    mcobjdata_t* odata;
    mcfloat_t valnumber;
    int valbool;
};

struct mcvalue_t
{
    mcvaltype_t valtype;
    bool isallocated;
    mcvalunion_t uval;
};

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


struct mcptrlist_t
{
    mcstate_t* pstate;
    unsigned char* listitems;
    unsigned char* allocdata;
    unsigned int listcount;
    unsigned int listcapacity;
    size_t typesize;
    bool caplocked;
    bool isptr;
};


struct mcvaldict_t
{
    mcstate_t* pstate;
    size_t keytypesize;
    size_t valtypesize;
    unsigned int* vdcells;
    unsigned long* vdhashes;
    char** vdkeys;
    mcvalue_t** vdvalues;
    unsigned int* vdcellindices;
    unsigned int vdcount;
    unsigned int vditemcapacity;
    unsigned int vdcellcapacity;
    mcitemhashfn_t funchashfn;
    mcitemcomparefn_t funckeyequalsfn;
};

struct mcptrdict_t
{
    mcstate_t* pstate;
    unsigned int* gdcells;
    unsigned long* gdhashes;
    char** gdkeys;
    void** gdvalues;
    unsigned int* gdcellindices;
    unsigned int gdcount;
    unsigned int gditemcapacity;
    unsigned int gdcellcapacity;
    mcitemcopyfn_t funccopyfn;
    mcitemdestroyfn_t funcdestroyfn;
};


struct mcfield_t
{
    const char* name;
    bool ispseudo;
    mcnativefn_t fndest;
};

struct mcclass_t
{
    const char* classname;
    mcclass_t* parentclass;
    mcvalue_t constructor;
    mcptrlist_t* members;
};


union mcstoddiyfpconv_t
{
    double d;
    uint64_t u64;
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

struct mcasttoken_t
{
    mcasttoktype_t toktype;
    const char* tokstrdata;
    int tokstrlen;
    mcastlocation_t pos;
};

struct mcastexprcodeblock_t
{
    mcstate_t* pstate;
    mcptrlist_t* statements;
};

struct mcastexprliteralmap_t
{
    mcptrlist_t* litmapkeys;
    mcptrlist_t* litmapvalues;
};

struct mcastexprliteralarray_t
{
    mcptrlist_t* litarritems;
};

struct mcastexprliteralstring_t
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

struct mcastexprifcase_t
{
    mcstate_t* pstate;
    mcastexpression_t* ifcond;
    mcastexprcodeblock_t* consequence;
};

struct mcastexprliteralfunction_t
{
    char* name;
    mcptrlist_t* funcparamlist;
    mcastexprcodeblock_t* body;
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

struct mcastexprident_t
{
    mcstate_t* pstate;
    char* value;
    mcastlocation_t pos;
};

struct mcastfuncparam_t
{
    mcstate_t* pstate;
    mcastexprident_t* ident;
};

struct mcastexprdefine_t
{
    mcastexprident_t* name;
    mcastexpression_t* value;
    bool assignable;
};

struct mcastexprstmtif_t
{
    mcptrlist_t* cases;
    mcastexprcodeblock_t* alternative;
};

struct mcastexprstmtwhile_t
{
    mcastexpression_t* loopcond;
    mcastexprcodeblock_t* body;
};

struct mcastexprstmtforeach_t
{
    mcastexprident_t* iterator;
    mcastexpression_t* source;
    mcastexprcodeblock_t* body;
};

struct mcastexprstmtforloop_t
{
    mcastexpression_t* init;
    mcastexpression_t* loopcond;
    mcastexpression_t* update;
    mcastexprcodeblock_t* body;
};

struct mcastexprstmtimport_t
{
    char* path;
};

struct mcastexprstmtrecover_t
{
    mcastexprident_t* errident;
    mcastexprcodeblock_t* body;
};

union mcexprunion_t
{
    mcastexprident_t* exprident;
    mcfloat_t exprlitnumber;
    bool exprlitbool;
    mcastexprliteralarray_t exprlitarray;
    mcastexprstmtimport_t exprimportstmt;
    mcastexprliteralstring_t exprlitstring;
    mcastexprliteralmap_t exprlitmap;
    mcastexprprefix_t exprprefix;
    mcastexprcall_t exprcall;
    mcastexprstmtif_t exprifstmt;
    mcastexprstmtwhile_t exprwhileloopstmt;
    mcastexprcodeblock_t* exprblockstmt;
    mcastexprstmtrecover_t exprrecoverstmt;
    mcastexprinfix_t exprinfix;
    mcastexprliteralfunction_t exprlitfunction;
    mcastexprindex_t exprindex;
    mcastexprassign_t exprassign;
    mcastexprlogical_t exprlogical;
    mcastexprternary_t exprternary;
    mcastexprdefine_t exprdefine;
    mcastexprstmtforeach_t exprforeachloopstmt;
    mcastexprstmtforloop_t exprforloopstmt;
    mcastexpression_t* exprreturnvalue;
    mcastexpression_t* exprexpression;
};

struct mcastexpression_t
{
    mcstate_t* pstate;
    mcastexprtype_t exprtype;
    mcastlocation_t pos;
    mcexprunion_t uexpr;
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
    void* userpointer;
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
    mcvaltype_t odtype;
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
    mcastsymtype_t symtype;
    char* name;
    int index;
    bool assignable;
};

struct mcastscopeblock_t
{
    mcstate_t* pstate;
    mcptrdict_t* scopestore;
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
    mcptrlist_t* gcobjlistremains;
    mcgcobjdatapool_t onlydatapool;
    mcgcobjdatapool_t mempools[MC_CONF_GCMEMPOOLCOUNT];
};

struct mccompiledprogram_t
{
    mcstate_t* pstate;
    uint16_t* bytecode;
    mcastlocation_t* progsrcposlist;
    int count;
};

struct mcastscopecomp_t
{
    mcstate_t* pstate;
    mcastscopecomp_t* outer;
    mcptrlist_t* compiledscopebytecode;
    mcptrlist_t* scopesrcposlist;
    mcptrlist_t* ipstackbreak;
    mcptrlist_t* ipstackcontinue;
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
    mcerrtype_t errtype;
    char message[MC_CONF_MAXERRORMSGLENGTH];
    mcastlocation_t pos;
    mctraceback_t* traceback;
};

struct mcerrlist_t
{
    mcerror_t errors[MC_CONF_MAXERRORCOUNT];
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
    uint16_t* bytecode;
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
    mcptrlist_t* tbitems;
};

struct mcconfig_t
{
    bool dumpast;
    bool dumpbytecode;
    bool printinstructions;
    bool fatalcomplaints;
    bool exitaftercompiling;
    /* allows redefinition of symbols */
    bool replmode;
};

struct mcexecstate_t
{
    size_t vsposition;
    mcvmframe_t* currframe;
    mcvallist_t* valuestack;
    mcvallist_t* valthisstack;
    mcvallist_t* nativethisstack;
    size_t thisstpos;
    mcframelist_t* framestack;
    mcvalue_t lastpopped;

};

struct mcstate_t
{
    mcconfig_t config;
    mcerrlist_t errors;
    mcgcmemory_t* mem;
    mcglobalstore_t* vmglobalstore;
    mcvallist_t* globalvalstack;
    size_t globalvalcount;

    bool running;
    mcvalue_t operoverloadkeys[MC_CONF_MAXOPEROVERLOADS];
    mcptrlist_t* files;
    mcastcompiler_t* compiler;
    mcexecstate_t execstate;
    mcprinter_t* stdoutprinter;
    mcprinter_t* stderrprinter;
    mcclass_t* stdobjobject;
    mcclass_t* stdobjnumber;
    mcclass_t* stdobjstring;
    mcclass_t* stdobjarray;
    mcclass_t* stdobjmap;
    mcclass_t* stdobjfunction;
};

struct mcglobalstore_t
{
    mcstate_t* pstate;
    mcptrdict_t* storedsymbols;
    mcvallist_t* storedobjects;
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
    StringBuffer* destbuf;
};

struct mcastprinter_t
{
    mcstate_t* pstate;
    mcprinter_t* pdest;
    bool pseudolisp;
};

struct mcmodule_t
{
    mcstate_t* pstate;
    char* name;
    mcptrlist_t* modsymbols;
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
    mcvallist_t* constants;
    mcastscopecomp_t* compilationscope;
    mcptrlist_t* filescopelist;
    mcptrlist_t* srcposstack;
    mcptrdict_t* modules;
    mcptrdict_t* stringconstposdict;
};

#include "prot.inc"

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

MC_INLINE void mc_util_assertva(bool x, const char* exprstr, const char* file, int line, const char* fmt, va_list va)
{
    if(!x)
    {
        fprintf(stderr, "ASSERTION FAILED at %s:%d: %s", file, line, exprstr);
        if(fmt != NULL)
        {
            fprintf(stderr, ": ");
            vfprintf(stderr, fmt, va);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
        abort();
    }
}

MC_INLINE void mc_util_assert(bool x, const char* exprstr, const char* file, int line, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    mc_util_assertva(x, exprstr, file, line, fmt, va);
    va_end(va);
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
    size_t printedsz;
    FILE* fp;
    (void)state;
    fp = fopen(path, "w");
    if(!fp)
    {
        return 0;
    }
    printedsz = fwrite(string, 1, stringsize, fp);
    fclose(fp);
    return printedsz;
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


size_t mc_util_hashdata(const void* ptr, size_t len)
{
    /* djb2 */
    size_t i;
    size_t hash;
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

size_t mc_util_hashdouble(mcfloat_t val)
{
    /* djb2 */
    size_t hash;
    uint32_t* valptr;
    valptr = (uint32_t*)&val;
    hash = 5381;
    hash = ((hash << 5) + hash) + valptr[0];
    hash = ((hash << 5) + hash) + valptr[1];
    return hash;
}

size_t mc_util_upperpowoftwo(size_t v)
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

MC_FORCEINLINE mcvalue_t mc_object_makedatafrom(mcvaltype_t type, mcobjdata_t* data)
{
    mcvalue_t object;
    memset(&object, 0, sizeof(mcvalue_t));
    object.valtype = type;
    data->odtype = type;
    object.isallocated = true;
    object.uval.odata = data;
    return object;
}

MC_FORCEINLINE mcvalue_t mc_value_makeempty(mcvaltype_t t)
{
    mcvalue_t o = {};
    o.valtype = t;
    o.isallocated = false;
    return o;
}

MC_FORCEINLINE mcvalue_t mc_value_makenumber(mcfloat_t val)
{
    mcvalue_t o;
    o = mc_value_makeempty(MC_VAL_NUMBER);
    o.uval.valnumber = val;
    return o;
}

MC_FORCEINLINE mcvalue_t mc_value_makebool(bool val)
{
    mcvalue_t o;
    o = mc_value_makeempty(MC_VAL_BOOL);
    o.uval.valbool = val;
    return o;
}

MC_FORCEINLINE mcvalue_t mc_value_makenull()
{
    mcvalue_t o;
    o = mc_value_makeempty(MC_VAL_NULL);
    return o;
}

MC_FORCEINLINE mcshiftint_t mc_mathutil_binshiftleft(mcfloat_t dnleft, mcfloat_t dnright)
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
    ivleft = (mcshiftint_t)dnleft;
    ivright = (mcshiftint_t)dnright;
    ivright &= 0x1f;
    return (ivleft << ivright);
}

MC_FORCEINLINE mcshiftint_t mc_mathutil_binshiftright(mcfloat_t dnleft, mcfloat_t dnright)
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
    return (ivleft >> ivright);
}

MC_FORCEINLINE mcfloat_t mc_mathutil_binor(mcfloat_t dnleft, mcfloat_t dnright)
{
    int64_t ivleft;
    int64_t ivright;
    ivleft = (int64_t)dnleft;
    ivright = (int64_t)dnright;
    return (mcfloat_t)(ivleft | ivright);
}

MC_FORCEINLINE mcfloat_t mc_mathutil_binand(mcfloat_t dnleft, mcfloat_t dnright)
{
    int64_t ivleft;
    int64_t ivright;
    ivleft = (int64_t)dnleft;
    ivright = (int64_t)dnright;
    return (mcfloat_t)(ivleft & ivright);
}

MC_FORCEINLINE mcfloat_t mc_mathutil_binxor(mcfloat_t dnleft, mcfloat_t dnright)
{
    int64_t ivleft;
    int64_t ivright;
    ivleft = (int64_t)dnleft;
    ivright = (int64_t)dnright;
    return (mcfloat_t)(ivleft ^ ivright);
}

MC_FORCEINLINE mcfloat_t mc_mathutil_add(mcfloat_t dnleft, mcfloat_t dnright)
{
    return dnleft + dnright;
}

MC_FORCEINLINE mcfloat_t mc_mathutil_sub(mcfloat_t dnleft, mcfloat_t dnright)
{
    return dnleft - dnright;
}

MC_FORCEINLINE mcfloat_t mc_mathutil_mult(mcfloat_t dnleft, mcfloat_t dnright)
{
    return dnleft * dnright;
}

MC_FORCEINLINE mcfloat_t mc_mathutil_div(mcfloat_t dnleft, mcfloat_t dnright)
{
    return dnleft / dnright;
}

MC_FORCEINLINE mcfloat_t mc_mathutil_mod(mcfloat_t dnleft, mcfloat_t dnright)
{
    return fmod(dnleft, dnright);
}

#include "listptrlist.h"
#include "listvallist.h"
#include "listframelist.h"
#include "dictptrdict.h"
#include "dictvaldict.h"

mcprinter_t* mc_printer_make(mcstate_t* state, FILE* ofh)
{
    mcprinter_t* pr;
    pr = (mcprinter_t*)mc_allocator_malloc(state, sizeof(mcprinter_t));
    if(pr == NULL)
    {
        return NULL;
    }
    if(!mc_printer_init(pr, state, ofh, false))
    {
        return NULL;
    }
    return pr;
}

bool mc_printer_init(mcprinter_t* pr, mcstate_t* state, FILE* ofh, bool onstack)
{
    memset(pr, 0, sizeof(mcprinter_t));
    pr->pstate = state;
    pr->failed = false;
    pr->destfile = ofh;
    pr->onstack = onstack;
    pr->destbuf = NULL;
    pr->config.verbosefunc = true;
    pr->config.quotstring = false;
    pr->config.shouldflush = false;
    if(pr->destfile == NULL)
    {
        pr->destbuf = dyn_strbuf_makeempty(0);
    }
    return true;
}

void mc_printer_release(mcprinter_t* pr, bool took)
{
    if(pr == NULL)
    {
        return;
    }
    if(pr->destbuf != NULL)
    {
        if(!took)
        {
            dyn_strbuf_destroy(pr->destbuf);
        }
    }
}

void mc_printer_destroy(mcprinter_t* pr)
{
    mc_printer_release(pr, true);
    if(!pr->onstack)
    {
        mc_memory_free(pr);
    }
}

MC_FORCEINLINE bool mc_printer_putlen(mcprinter_t* pr, const char* str, size_t len)
{
    if(pr->failed)
    {
        return false;
    }
    if(len == 0)
    {
        return true;
    }
    if(pr->destfile == NULL)
    {
        dyn_strbuf_appendstrn(pr->destbuf, str, len);
    }
    else
    {
        fwrite(str, sizeof(char), len, pr->destfile);
        if(pr->config.shouldflush)
        {
            fflush(pr->destfile);
        }
    }
    return true;
}

MC_FORCEINLINE bool mc_printer_puts(mcprinter_t* pr, const char* str)
{
    return mc_printer_putlen(pr, str, mc_util_strlen(str));
}

MC_FORCEINLINE bool mc_printer_putchar(mcprinter_t* pr, int b)
{
    char ch;
    ch = b;
    return mc_printer_putlen(pr, &ch, 1);
}

MC_FORCEINLINE bool mc_printer_printfv(mcprinter_t* pr, const char* fmt, va_list va)
{
    if(pr->failed)
    {
        return false;
    }
    if(pr->destfile == NULL)
    {
        dyn_strbuf_appendformatv(pr->destbuf, fmt, va);
    }
    else
    {
        vfprintf(pr->destfile, fmt, va);
        if(pr->config.shouldflush)
        {
            fflush(pr->destfile);
        }
    }
    return true;
}

MC_INLINE bool mc_printer_printf(mcprinter_t* pr, const char* fmt, ...)
{
    bool r;
    va_list va;
    va_start(va, fmt);
    r = mc_printer_printfv(pr, fmt, va);
    va_end(va);
    return r;
}

MC_FORCEINLINE void mc_printer_printescapedchar(mcprinter_t* pr, int ch)
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

MC_FORCEINLINE void mc_printer_printescapedstring(mcprinter_t* pr, const char* str, size_t len)
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

MC_FORCEINLINE const char* mc_printer_getstring(mcprinter_t* pr)
{
    if(pr->failed)
    {
        return NULL;
    }
    if(pr->destfile != NULL)
    {
        return NULL;
    }
    return pr->destbuf->data;
}

MC_FORCEINLINE size_t mc_printer_getlength(mcprinter_t* pr)
{
    if(pr->failed)
    {
        return 0;
    }
    if(pr->destfile != NULL)
    {
        return 0;
    }
    return pr->destbuf->length;
}

MC_FORCEINLINE char* mc_printer_getstringanddestroy(mcprinter_t* pr, size_t* lendest)
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
    res = pr->destbuf->data;
    if(lendest != NULL)
    {
        *lendest = pr->destbuf->length;
    }
    pr->destbuf = NULL;
    mc_printer_destroy(pr);
    return res;
}

#define mc_printer_printvalue(pr, val, accurate) \
    mc_printer_printvalue_actual(mc_value_gettype(val), pr, val, accurate)

void mc_printer_printnumberfloat(mcprinter_t* pr, mcfloat_t flt)
{
    int64_t inum;
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

bool mc_printutil_bcreadoperands(mcopdefinition_t* def, const uint16_t* instr, uint64_t outoperands[2])
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

void mc_printer_printbytecode(mcprinter_t* pr, uint16_t* code, mcastlocation_t* sposlist, size_t codesize, bool simple)
{
    bool ok;
    int i;
    uint16_t op;
    size_t pos;
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
        MC_ASSERT(def != NULL);
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
    str = mc_value_getstringdata(obj);
    len = mc_value_getstringlength(obj);
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
    actualary = mc_value_arraygetactualarray(obj);
    alen = mc_value_arraygetlength(obj);
    mc_printer_puts(pr, "[");
    for(i = 0; i < alen; i++)
    {
        recursion = false;
        iobj = mc_value_arraygetvalue(obj, i);
        if(mc_value_gettype(iobj) == MC_VAL_ARRAY)
        {
            otherary = mc_value_arraygetactualarray(iobj);
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
            mc_printer_printvalue(pr, iobj, false);
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
        mc_printer_printvalue(pr, key, false);
        mc_printer_puts(pr, ": ");
        mc_printer_printvalue(pr, val, false);
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
    mc_error_printusererror(pr, obj);
}


void mc_consolecolor_init(mcconsolecolor_t* mcc, int fd)
{
    size_t i;
    mcc->targetfds[0] = fd;
    mcc->fdcount = 1;
    mcc->ispiped = false;
    if(fd == -1)
    {
        mcc->ispiped = true;
    }
    else
    {
        for(i=0;i<mcc->fdcount; i++)
        {
            if(!isatty(mcc->targetfds[i]))
            {
                mcc->ispiped = true;
                break;
            }
        }
    }
}

const char* mc_consolecolor_get(mcconsolecolor_t* mcc, char code)
{
    if(mcc->ispiped)
    {
        return "";
    }
    switch(code)
    {
        case 0:
        case '0':
            /* reset */
            return "\x1B[0m";
        case 'r':
            /* red */
            return "\x1B[31m";
        case 'g':
            /* green */
            return "\x1B[32m";
        case 'y':
            /* yellow */
            return "\x1B[33m";
        case 'b':
            /* blue */
            return "\x1B[34m";
        case 'm':
            /* magenta */
            return "\x1B[35m";
        case 'c':
            /* cyan */
            return "\x1B[36m";
    }
    return "";
}



void mc_printer_printvalue_actual(int vt, mcprinter_t* pr, mcvalue_t obj, bool accurate)
{
    mcvaltype_t type;
    (void)vt;
    (void)accurate;
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
                number = mc_value_asnumber(obj);
                mc_printer_printnumberfloat(pr, number);
            }
            break;
        case MC_VAL_BOOL:
            {
                mc_printer_puts(pr, mc_value_asbool(obj) ? "true" : "false");
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
                mc_printer_puts(pr, "FUNCNATIVE");
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
    int needsz;
    int printedsz;
    char* res;
    va_list args;
    (void)printedsz;
    va_start(args, format);
    needsz = vsnprintf(NULL, 0, format, args);
    va_end(args);
    va_start(args, format);
    res = (char*)mc_allocator_malloc(state, needsz + 1);
    if(!res)
    {
        return NULL;
    }
    printedsz = vsprintf(res, format, args);
    va_end(args);
    MC_ASSERT(printedsz == needsz);
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

MC_FORCEINLINE bool mc_value_isallocated(mcvalue_t object)
{
    return object.isallocated;
}

MC_FORCEINLINE bool mc_value_isnumeric(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return type == MC_VAL_NUMBER || type == MC_VAL_BOOL;
}

MC_FORCEINLINE bool mc_value_isnumber(mcvalue_t o)
{
    return (mc_value_gettype(o) == MC_VAL_NUMBER || mc_value_gettype(o) == MC_VAL_BOOL);
}

MC_FORCEINLINE bool mc_value_isnull(mcvalue_t obj)
{
    return mc_value_gettype(obj) == MC_VAL_NULL;
}

MC_FORCEINLINE bool mc_value_isfuncnative(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return (type == MC_VAL_FUNCNATIVE);
}

MC_FORCEINLINE bool mc_value_isfuncscript(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return (type == MC_VAL_FUNCSCRIPT);
}

MC_FORCEINLINE bool mc_value_iscallable(mcvalue_t obj)
{
    return (mc_value_isfuncnative(obj) || mc_value_isfuncscript(obj));
}

MC_FORCEINLINE bool mc_value_isstring(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return (type == MC_VAL_STRING);
}

MC_FORCEINLINE bool mc_value_ishashable(mcvalue_t obj)
{
    mcvaltype_t type = mc_value_gettype(obj);
    switch(type)
    {
        case MC_VAL_STRING:
        case MC_VAL_NUMBER:
        case MC_VAL_BOOL:
            return true;
        default:
            break;
    }
    return false;
}

MC_FORCEINLINE bool mc_value_compare(mcvalue_t a, mcvalue_t b, mcvalcmpresult_t* cres)
{
    const char* astring;
    const char* bstring;
    int alen;
    int blen;
    intptr_t adataval;
    intptr_t bdataval;
    mcfloat_t dnleft;
    mcfloat_t dnright;
    size_t ahash;
    size_t bhash;
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
        dnleft = mc_value_asnumber(a);
        dnright = mc_value_asnumber(b);
        cres->result = (dnleft - dnright);
        return true;
    }
    if(atype == btype && atype == MC_VAL_STRING)
    {
        alen = mc_value_getstringlength(a);
        blen = mc_value_getstringlength(b);
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
        astring = mc_value_getstringdata(a);
        bstring = mc_value_getstringdata(b);
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

MC_FORCEINLINE bool mc_value_equals(mcvalue_t a, mcvalue_t b)
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


mcvalue_t mc_value_copydeepfuncscript(mcstate_t* state, mcvalue_t obj, mcvaldict_t* targetdict)
{
    bool ok;
    int i;
    uint16_t* bytecodecopy;
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
    bytecodecopy = (uint16_t*)mc_allocator_malloc(state, sizeof(uint16_t) * function->compiledprogcode->count);
    if(!bytecodecopy)
    {
        return mc_value_makenull();
    }
    memcpy(bytecodecopy, function->compiledprogcode->bytecode, sizeof(uint16_t) * function->compiledprogcode->count);
    srcpositionscopy = (mcastlocation_t*)mc_allocator_malloc(state, sizeof(mcastlocation_t) * function->compiledprogcode->count);
    if(!srcpositionscopy)
    {
        mc_memory_free(bytecodecopy);
        return mc_value_makenull();
    }
    memcpy(srcpositionscopy, function->compiledprogcode->progsrcposlist, sizeof(mcastlocation_t) * function->compiledprogcode->count);
    comprescopy = mc_astcompresult_make(state, bytecodecopy, srcpositionscopy, function->compiledprogcode->count);
    /*
    * todo: add compilation result copy function
    */
    if(!comprescopy)
    {
        mc_memory_free(srcpositionscopy);
        mc_memory_free(bytecodecopy);
        return mc_value_makenull();
    }
    copy = mc_value_makefuncscript(state, mc_value_functiongetname(obj), comprescopy, true, function->numlocals, function->numargs, 0);
    if(mc_value_isnull(copy))
    {
        mc_astcompresult_destroy(comprescopy);
        return mc_value_makenull();
    }
    ok = mc_valdict_setkv(targetdict, &obj, &copy);
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
        freevalcopy = mc_value_copydeepintern(state, freeval, targetdict);
        if(!mc_value_isnull(freeval) && mc_value_isnull(freevalcopy))
        {
            return mc_value_makenull();
        }
        mc_value_functionsetfreevalat(copy, i, freevalcopy);
    }
    return copy;
}

mcvalue_t mc_value_copydeeparray(mcstate_t* state, mcvalue_t obj, mcvaldict_t* targetdict)
{
    bool ok;
    int i;
    int len;
    mcvalue_t copy;
    mcvalue_t item;
    mcvalue_t itemcopy;
    len = mc_value_arraygetlength(obj);
    copy = mc_value_makearraycapacity(state, len);
    if(mc_value_isnull(copy))
    {
        return mc_value_makenull();
    }
    ok = mc_valdict_setkv(targetdict, &obj, &copy);
    if(!ok)
    {
        return mc_value_makenull();
    }
    for(i = 0; i < len; i++)
    {
        item = mc_value_arraygetvalue(obj, i);
        itemcopy = mc_value_copydeepintern(state, item, targetdict);
        if(!mc_value_isnull(item) && mc_value_isnull(itemcopy))
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

mcvalue_t mc_value_copydeepmap(mcstate_t* state, mcvalue_t obj, mcvaldict_t* targetdict)
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
    ok = mc_valdict_setkv(targetdict, &obj, &copy);
    if(!ok)
    {
        return mc_value_makenull();
    }
    for(i = 0; i < mc_valmap_getlength(obj); i++)
    {
        key = mc_valmap_getkeyat(obj, i);
        val = mc_valmap_getvalueat(obj, i);
        keycopy = mc_value_copydeepintern(state, key, targetdict);
        if(!mc_value_isnull(key) && mc_value_isnull(keycopy))
        {
            return mc_value_makenull();
        }
        valcopy = mc_value_copydeepintern(state, val, targetdict);
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

mcvalue_t mc_value_copydeepintern(mcstate_t* state, mcvalue_t obj, mcvaldict_t* targetdict)
{
    mcvaltype_t type;
    mcvalue_t copy;
    mcvalue_t* copyptr;
    copyptr = (mcvalue_t*)mc_valdict_get(targetdict, &obj);
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
                bool ok;
                int len;
                const char* str;
                str = mc_value_getstringdata(obj);
                len = mc_value_getstringlength(obj);
                copy = mc_value_makestringlen(state, str, len);
                ok = mc_valdict_setkv(targetdict, &obj, &copy);
                if(!ok)
                {
                    return mc_value_makenull();
                }
                return copy;
            }
            break;
        case MC_VAL_FUNCSCRIPT:
            {
                return mc_value_copydeepfuncscript(state, obj, targetdict);
            }
            break;
        case MC_VAL_ARRAY:
            {
                return mc_value_copydeeparray(state, obj, targetdict);
            }
            break;
        case MC_VAL_MAP:
            {
                return mc_value_copydeepmap(state, obj, targetdict);
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

mcvalue_t mc_value_copydeep(mcstate_t* state, mcvalue_t obj)
{
    mcvalue_t res;
    mcvaldict_t* targetdict;
    targetdict = mc_valdict_makedefault(state, sizeof(mcvalue_t), sizeof(mcvalue_t));
    if(!targetdict)
    {
        return mc_value_makenull();
    }
    res = mc_value_copydeepintern(state, obj, targetdict);
    mc_valdict_destroy(targetdict);
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
                str = mc_value_getstringdata(obj);
                len = mc_value_getstringlength(obj);
                copy = mc_value_makestringlen(state, str, len);
            }
            break;
        case MC_VAL_ARRAY:
            {
                int i;
                int len;
                mcvalue_t item;
                len = mc_value_arraygetlength(obj);
                copy = mc_value_makearraycapacity(state, len);
                if(mc_value_isnull(copy))
                {
                    return mc_value_makenull();
                }
                for(i = 0; i < len; i++)
                {
                    item = mc_value_arraygetvalue(obj, i);
                    ok = mc_value_arraypush(copy, item);
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


mcgcmemory_t* mc_value_getmem(mcvalue_t obj)
{
    mcobjdata_t* data;
    data = mc_value_getallocateddata(obj);
    return data->mem;
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

bool mc_value_asbool(mcvalue_t obj)
{
    if(mc_value_isnumber(obj))
    {
        return obj.uval.valnumber;
    }
    return obj.uval.valbool;
}

mcfloat_t mc_value_asnumber(mcvalue_t obj)
{
    if(mc_value_isnumber(obj))
    {
        if(mc_value_gettype(obj) == MC_VAL_BOOL)
        {
            return obj.uval.valbool;
        }
        return obj.uval.valnumber;
    }
    return obj.uval.valnumber;
}

MC_INLINE const char* mc_value_getstringdata(mcvalue_t object)
{
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    return mc_value_getstringdataintern(object);
}

int mc_value_getstringlength(mcvalue_t object)
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

MC_INLINE char* mc_valstring_getmutabledata(mcvalue_t object)
{
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    return mc_value_getstringdataintern(object);
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

bool mc_valstring_appendvalue(mcvalue_t destval, mcvalue_t val)
{
    bool ok;
    int vlen;
    const char* vstr;
    if(mc_value_gettype(val) == MC_VAL_NUMBER)
    {
        mc_valstring_appendformat(destval, "%g", mc_value_asnumber(val));
        return true;
    }
    if(mc_value_gettype(val) == MC_VAL_STRING)
    {
        vlen = mc_value_getstringlength(val);
        vstr = mc_value_getstringdata(val);
        ok = mc_valstring_appendlen(destval, vstr, vlen);
        if(!ok)
        {
            return false;
        }
    
        return true;
    }
    return false;
}

size_t mc_valstring_gethash(mcvalue_t obj)
{
    size_t len;
    const char* str;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_STRING);
    data = mc_value_getallocateddata(obj);
    if(data->uvobj.valstring.hash == 0)
    {
        len = mc_value_getstringlength(obj);
        str = mc_value_getstringdata(obj);
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

MC_INLINE mcobjfuncnative_t* mc_value_functiongetnativefunction(mcvalue_t obj)
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

mcvalue_t mc_value_arraygetvalue(mcvalue_t object, size_t ix)
{
    mcvalue_t* res;
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
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

bool mc_value_arraysetvalue(mcvalue_t object, size_t ix, mcvalue_t val)
{
    size_t len;
    size_t toadd;
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    len = mc_vallist_count(array);
    if((ix >= len) || (len == 0))
    {
        toadd = len+1;
        #if 0
            fprintf(stderr, "ix=%d toadd=%d len=%d\n", ix, toadd, len);
        #endif
        while(toadd != (ix+2))
        {
            mc_value_arraypush(object, mc_value_makenull());
            toadd++;
        }
    }
    return mc_vallist_set(array, ix, val);
}

bool mc_value_arraypush(mcvalue_t object, mcvalue_t val)
{
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    return mc_vallist_push(array, val);
}

int mc_value_arraygetlength(mcvalue_t object)
{
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    return mc_vallist_count(array);
}

mcvalue_t mc_valarray_pop(mcvalue_t object)
{
    mcvalue_t dest;
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    if(mc_vallist_pop(array, &dest))
    {
        return dest;
    }
    return mc_value_makenull();
}

bool mc_value_arrayremoveat(mcvalue_t object, int ix)
{
    mcvallist_t* array;
    array = mc_value_arraygetactualarray(object);
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

void mc_errlist_init(mcerrlist_t* errors)
{
    memset(errors, 0, sizeof(mcerrlist_t));
    errors->count = 0;
}

void mc_errlist_deinit(mcerrlist_t* errors)
{
    mc_errlist_clear(errors);
}

void mc_errlist_pushmessage(mcerrlist_t* errors, mcerrtype_t type, mcastlocation_t pos, const char* message)
{
    int len;
    int tocopy;
    mcerror_t err;
    if(errors->count >= MC_CONF_MAXERRORCOUNT)
    {
        return;
    }
    memset(&err, 0, sizeof(mcerror_t));
    err.errtype = type;
    len = mc_util_strlen(message);
    tocopy = len;
    if(tocopy >= (MC_CONF_MAXERRORMSGLENGTH - 1))
    {
        tocopy = MC_CONF_MAXERRORMSGLENGTH - 1;
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
    int needsz;
    int printedsz;
    char res[MC_CONF_MAXERRORMSGLENGTH];
    va_list vcopy;
    (void)needsz;
    (void)printedsz;
    va_copy(vcopy, va);
    needsz = vsnprintf(NULL, 0, format, vcopy);
    printedsz = vsnprintf(res, MC_CONF_MAXERRORMSGLENGTH, format, va);
    MC_ASSERT(needsz == printedsz);
    mc_errlist_pushmessage(errors, type, pos, res);
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

mcvalue_t mc_value_makefuncnative(mcstate_t* state, const char* name, mcnativefn_t fn, void* data)
{
    mcobjdata_t* obj;
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
        obj->uvobj.valnativefunc.userpointer = data;
    }
    return mc_object_makedatafrom(MC_VAL_FUNCNATIVE, obj);
}

mcvalue_t mc_value_makearray(mcstate_t* state)
{
    return mc_value_makearraycapacity(state, 8);
}

mcvalue_t mc_value_makearraycapacity(mcstate_t* state, size_t capacity)
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
    data->uvobj.valarray = mc_vallist_make(state, NULL, capacity);
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

mcvalue_t mc_value_makemapcapacity(mcstate_t* state, size_t capacity)
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
    mc_valdict_sethashfunction(data->uvobj.valmap, (mcitemhashfn_t)mc_value_callbackhash);
    mc_valdict_setequalsfunction(data->uvobj.valmap, (mcitemcomparefn_t)mc_value_callbackequals);
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
    int needsz;
    int printedsz;
    char* res;
    va_list args;
    mcvalue_t resobj;
    (void)printedsz;
    va_start(args, fmt);
    needsz = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    va_start(args, fmt);
    res = (char*)mc_memory_malloc(needsz + 1);
    if(!res)
    {
        return mc_value_makenull();
    }
    printedsz = vsprintf(res, fmt, args);
    MC_ASSERT(printedsz == needsz);
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


void mc_objectdata_deinit(mcobjdata_t* data)
{
    switch(data->odtype)
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
                    mc_memory_free(data->uvobj.valscriptfunc.unamev.fallocname);
                    mc_astcompresult_destroy(data->uvobj.valscriptfunc.compiledprogcode);
                }
                if(mc_objfunction_freevalsareallocated(&data->uvobj.valscriptfunc))
                {
                    mc_memory_free(data->uvobj.valscriptfunc.ufv.freevalsallocated);
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
                mc_memory_free(data->uvobj.valnativefunc.name);
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
                mc_memory_free(data->uvobj.valerror.message);
                mc_traceback_destroy(data->uvobj.valerror.traceback);
            }
            break;
        default:
            {
            }
            break;
    }
    data->odtype = MC_VAL_FREED;
}


bool mc_value_callbackequals(mcvalue_t* aptr, mcvalue_t* bptr)
{
    mcvalue_t a;
    mcvalue_t b;
    a = *aptr;
    b = *bptr;
    return mc_value_equals(a, b);
}

size_t mc_value_callbackhash(mcvalue_t* objptr)
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
                dval = mc_value_asnumber(obj);
                return mc_util_hashdouble(dval);
            }
            break;
        case MC_VAL_BOOL:
            {
                bval = mc_value_asbool(obj);
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

mcvallist_t* mc_value_arraygetactualarray(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valarray;
}

bool mc_objfunction_freevalsareallocated(mcobjfuncscript_t* fun)
{
    return fun->freevalscount >= MC_UTIL_STATICARRAYSIZE(fun->ufv.freevalsstack);
}

MC_INLINE char* mc_value_getstringdataintern(mcvalue_t object)
{
    mcobjdata_t* data;
    data = mc_value_getallocateddata(object);
    MC_ASSERT(data->odtype == MC_VAL_STRING);
    return data->uvobj.valstring.strbuf->data;
}


#include "modapi.h"

#include "cclex.h"

#include "cccomp.h"

#include "ccapi.h"

#include "ccparse.h"

#include "optimizer.h"

bool mc_util_strequal(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
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
            mc_memory_free(stritem);
            mc_ptrlist_removeat(split, i);
            i = -1;
            continue;
        }
        if(mc_util_strequal(nextitem, ".."))
        {
            mc_memory_free(stritem);
            mc_memory_free(nextitem);
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
        mc_memory_free(item);
    }
    mc_ptrlist_destroy(split, NULL);
    return joined;
}

bool mc_util_pathisabsolute(const char* path)
{
    return path[0] == '/';
}

mcclass_t* mc_class_make(mcstate_t* state, const char* name, bool istop)
{
    mcclass_t* cl;
    cl = (mcclass_t*)mc_memory_malloc(sizeof(mcclass_t));
    cl->parentclass = NULL;
    cl->classname = name;
    cl->constructor = mc_value_makenull();
    cl->members = mc_ptrlist_make(state, sizeof(void*), true);
    if(!istop)
    {
        cl->parentclass = state->stdobjobject;
    }
    return cl;
}

void mc_class_destroy(mcstate_t* state, mcclass_t* cl)
{
    size_t i;
    mcfield_t* memb;
    (void)state;
    for(i=0; i<cl->members->listcount; i++)
    {
        memb = (mcfield_t*)mc_ptrlist_get(cl->members, i);
        mc_memory_free(memb);
    }
    mc_ptrlist_destroy(cl->members, NULL);
    mc_memory_free(cl);
}

void mc_class_addfunction(mcstate_t* state, mcclass_t* cl, const char* name, bool ispseudo, mcnativefn_t fn)
{
    mcfield_t* bt;
    (void)state;
    bt = (mcfield_t*)mc_memory_malloc(sizeof(mcfield_t));
    bt->name = name;
    bt->ispseudo = ispseudo;
    bt->fndest = fn;
    mc_ptrlist_push(cl->members, bt);
}

void mc_class_addmember(mcstate_t* state, mcclass_t* cl, const char* name, mcnativefn_t fn)
{
    return mc_class_addfunction(state, cl, name, false, fn);
}

void mc_class_addpseudo(mcstate_t* state, mcclass_t* cl, const char* name, mcnativefn_t fn)
{
    return mc_class_addfunction(state, cl, name, true, fn);
}

mcstate_t* mc_state_make()
{
    mcstate_t* state;
    state = (mcstate_t*)mc_memory_malloc(sizeof(mcstate_t));
    if(!state)
    {
        return NULL;
    }
    memset(state, 0, sizeof(mcstate_t));
    mc_state_setdefaultconfig(state);
    state->execstate.valuestack = mc_vallist_make(state, "valuestack", MC_CONF_MINVMVALSTACKSIZE);
    state->execstate.valthisstack = mc_vallist_make(state, "valthisstack",  MC_CONF_MINVMTHISSTACKSIZE);
    state->execstate.nativethisstack = mc_vallist_make(state, "nativethisstack",  MC_CONF_MINNATIVETHISSTACKSIZE);
    state->globalvalstack = mc_vallist_make(state, "globalvalstack", MC_CONF_MAXVMGLOBALS);
    state->execstate.framestack = mc_framelist_make(state, MC_CONF_MINVMFRAMES);
    mc_errlist_init(&state->errors);
    state->mem = mc_gcmemory_make(state);
    if(!state->mem)
    {
        goto err;
    }
    state->files = mc_ptrlist_make(state, sizeof(void*), true);
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
    mc_state_makestdclasses(state);
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
    mc_vallist_destroy(state->execstate.valuestack);
    mc_vallist_destroy(state->globalvalstack);
    mc_framelist_destroy(state->execstate.framestack);
    mc_vallist_destroy(state->execstate.valthisstack);
    mc_vallist_destroy(state->execstate.nativethisstack);
    mc_class_destroy(state, state->stdobjobject);
    mc_class_destroy(state, state->stdobjnumber);
    mc_class_destroy(state, state->stdobjstring);
    mc_class_destroy(state, state->stdobjarray);
    mc_class_destroy(state, state->stdobjmap);
    mc_class_destroy(state, state->stdobjfunction);
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
    state->config.exitaftercompiling = false;
    state->config.printinstructions = false;
}

void mc_state_destroy(mcstate_t* state)
{
    if(!state)
    {
        return;
    }
    mc_state_deinit(state);
    mc_memory_free(state);
}

void mc_state_freeallocated(mcstate_t* state, void* ptr)
{
    (void)state;
    mc_memory_free(ptr);
}

void mc_state_printerrors(mcstate_t* state)
{
    int i;
    int ecnt;
    mcerror_t* err;
    ecnt = mc_state_errorcount(state);
    for(i = 0; i < ecnt; i++)
    {
        err = mc_state_geterror(state, i);
        mc_error_printerror(state, state->stderrprinter, err);
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
        mc_errlist_addf(&state->errors, MC_ERROR_USER, srcposinvalid, "program passed to execute was null.");
        return mc_value_makenull();
    }
    mc_state_reset(state);
    if(state != program->pstate)
    {
        mc_errlist_addf(&state->errors, MC_ERROR_USER, srcposinvalid, "program was compiled with an incompatible instance");
        return mc_value_makenull();
    }
    ok = mc_vm_runexecfunc(state, program, mc_compiler_getconstants(state->compiler));
    if(!ok || state->errors.count > 0)
    {
        return mc_value_makenull();
    }
    //MC_ASSERT(state->execstate.vsposition == 0);
    res = mc_vm_getlastpopped(state);
    if(mc_value_gettype(res) == MC_VAL_NONE)
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
    MC_ASSERT(state->execstate.vsposition == 0);
    res = mc_vm_getlastpopped(state);
    if(mc_value_gettype(res) == MC_VAL_NONE)
    {
        goto err;
    }
    mc_astcompresult_destroy(compres);
    return res;
err:
    mc_astcompresult_destroy(compres);
    return mc_value_makenull();
}

mcvalue_t mc_state_callfunctionbyname(mcstate_t* state, const char* fname, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcvalue_t res;
    mcvalue_t callee;
    mc_state_reset(state);
    callee = mc_state_getglobalobjectbyname(state, fname);
    if(mc_value_gettype(callee) == MC_VAL_NULL)
    {
        return mc_value_makenull();
    }
    res = mc_vm_callvalue(state, mc_compiler_getconstants(state->compiler), callee, thisval, argc, (mcvalue_t*)args);
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

bool mc_state_setnativefunction(mcstate_t* state, const char* name, mcnativefn_t fn, void* data)
{
    mcvalue_t obj;
    obj = mc_value_makefuncnative(state, name, fn, data);
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
        mc_state_pusherrorf(state, MC_ERROR_USER, srcposinvalid, "symbol \"%s\" is not defined", name);
        return mc_value_makenull();
    }
    res = mc_value_makenull();
    if(symbol->symtype == MC_SYM_MODULEGLOBAL)
    {
        res = mc_vm_getglobalbyindex(state, symbol->index);
    }
    else if(symbol->symtype == MC_SYM_GLOBALBUILTIN)
    {
        ok = false;
        res = mc_globalstore_getatindex(state->vmglobalstore, symbol->index, &ok);
        if(!ok)
        {
            mc_state_pusherrorf(state, MC_ERROR_USER, srcposinvalid, "failed to get global object at %d", symbol->index);
            return mc_value_makenull();
        }
    }
    else
    {
        mc_state_pusherrorf(state, MC_ERROR_USER, srcposinvalid, "value associated with symbol \"%s\" could not be loaded", name);
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
    switch(error->errtype)
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

bool mc_error_printtraceback(mcprinter_t* pr, mctraceback_t* traceback, mcconsolecolor_t* mcc)
{
    int i;
    int depth;
    const char* cblue;
    const char* cyell;
    const char* creset;
    const char* filename;
    mctraceitem_t* item;
    cblue = mc_consolecolor_get(mcc, 'b');
    cyell = mc_consolecolor_get(mcc, 'y');
    creset = mc_consolecolor_get(mcc, '0');
    depth = mc_ptrlist_count(traceback->tbitems);
    for(i = 0; i < depth; i++)
    {
        item = (mctraceitem_t*)mc_ptrlist_get(traceback->tbitems, i);
        filename = mc_traceitem_getsourcefilepath(item);
        mc_printer_printf(pr, "  function %s%s%s", cblue, item->trfuncname, creset);
        if(item->pos.line >= 0 && item->pos.column >= 0)
        {
            mc_printer_printf(pr, " in %s%s:%d:%d%s", cyell, filename, item->pos.line, item->pos.column, creset);
        }
        else
        {
        }
        mc_printer_printf(pr, "\n");
    }
    return !pr->failed;
}

bool mc_error_printusererror(mcprinter_t* pr, mcvalue_t obj)
{
    const char* cred;
    const char* creset;
    mcconsolecolor_t mcc;
    mctraceback_t* traceback;
    mc_consolecolor_init(&mcc, fileno(stdout));
    cred = mc_consolecolor_get(&mcc, 'r');
    creset = mc_consolecolor_get(&mcc, '0');
    mc_printer_printf(pr, "%sERROR: %s%\n", cred, mc_value_errorgetmessage(obj), creset);
    traceback = mc_value_errorgettraceback(obj);
    MC_ASSERT(traceback != NULL);
    if(traceback)
    {
        mc_printer_printf(pr, "%sTraceback:%s\n", cred, creset);
        mc_error_printtraceback(pr, traceback, &mcc);
    }
    return true;
}

bool mc_error_printerror(mcstate_t* state, mcprinter_t* pr, mcerror_t* err)
{
    int j;
    int colnum;
    int linenum;
    const char* line;
    const char* typestr;
    const char* filename;
    const char* cred;
    const char* cblue;
    const char* creset;
    mctraceback_t* traceback;
    mcconsolecolor_t mcc;
    (void)state;
    mc_consolecolor_init(&mcc, fileno(stdout));
    cred = mc_consolecolor_get(&mcc, 'r');
    cblue = mc_consolecolor_get(&mcc, 'b');
    creset = mc_consolecolor_get(&mcc, '0');
    typestr = mc_error_gettypestring(err);
    filename = mc_error_getfilepath(err);
    line = mc_error_getsourcelinecode(err);
    linenum = mc_error_getsourcelinenumber(err);
    colnum = mc_error_getsourcecolumn(err);
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
    mc_printer_printf(pr, "%s%s ERROR%s in \"%s\" on %s%d:%d:%s %s\n", cred, typestr, creset, filename, cblue, linenum, colnum, creset, mc_error_getmessage(err));
    traceback = mc_error_gettraceback(err);
    if(traceback)
    {
        mc_printer_printf(pr, "traceback:\n");
        mc_error_printtraceback(pr, (mctraceback_t*)mc_error_gettraceback(err), &mcc);
    }
    return true;
}

mctraceback_t* mc_error_gettraceback(mcerror_t* error)
{
    return (mctraceback_t*)error->traceback;
}

int mc_traceback_getdepth(mctraceback_t* traceback)
{
    return mc_ptrlist_count(traceback->tbitems);
}

const char* mc_traceback_getsourcefilepath(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)mc_ptrlist_get(traceback->tbitems, depth);
    if(!item)
    {
        return NULL;
    }
    return mc_traceitem_getsourcefilepath(item);
}

const char* mc_traceback_getsourcelinecode(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)mc_ptrlist_get(traceback->tbitems, depth);
    if(!item)
    {
        return NULL;
    }
    return mc_traceitem_getsourceline(item);
}

int mc_traceback_getsourcelinenumber(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)mc_ptrlist_get(traceback->tbitems, depth);
    if(!item)
    {
        return -1;
    }
    return item->pos.line;
}

int mc_traceback_getsourcecolumn(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)mc_ptrlist_get(traceback->tbitems, depth);
    if(!item)
    {
        return -1;
    }
    return item->pos.column;
}

const char* mc_traceback_getfunctionname(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)mc_ptrlist_get(traceback->tbitems, depth);
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
        case MC_MATHOP_LTE:
            return "<=";
        case MC_MATHOP_GTE:
            return ">=";
        case MC_MATHOP_BINNOT:
            return "~";
    }
    return "MC_MATHOP_UNKNOWN";
}

#if 1
    bool mc_argcheck_check(mcstate_t* state, bool generateerror, size_t argc, mcvalue_t* args, ...)
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

bool mc_argcheck_checkactual(mcstate_t* state, bool generateerror, size_t argc, mcvalue_t* args, size_t expectedargc, const mcvaltype_t* expectedtypes)
{
    size_t i;
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
                mc_memory_free(expectedtypestr);
            }
            return false;
        }
    }
    return true;
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
    res = mc_ptrlist_make(state, sizeof(void*), true);
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
            mc_memory_free(line);
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
    mc_memory_free(rest);
    if(res)
    {
        for(i = 0; i < mc_ptrlist_count(res); i++)
        {
            line = (char*)mc_ptrlist_get(res, i);
            mc_memory_free(line);
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

MC_FORCEINLINE bool mc_callframe_init(mcvmframe_t* frame, mcvalue_t functionobj, int64_t baseptr)
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


MC_FORCEINLINE uint16_t mc_callframe_readuint8(mcvmframe_t* frame)
{
    #if 0
        uint16_t* data;
        data = &frame->bytecode[frame->bcposition];
        frame->bcposition++;
        return data[0];
    #else
        uint16_t data;
        data = frame->bytecode[frame->bcposition];
        frame->bcposition++;
        return data;
    #endif
}

MC_FORCEINLINE uint16_t mc_callframe_readuint16(mcvmframe_t* frame)
{
    uint16_t* data;
    data = frame->bytecode + frame->bcposition;
    frame->bcposition += 2;
    return (data[0] << 8) | data[1];
}

MC_FORCEINLINE uint64_t mc_callframe_readuint64(mcvmframe_t* frame)
{
    uint64_t res;
    uint16_t* data;
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


MC_FORCEINLINE mcopcode_t mc_callframe_readopcode(mcvmframe_t* frame)
{
    frame->sourcebcpos = frame->bcposition;
    return (mcopcode_t)mc_callframe_readuint8(frame);
}

MC_FORCEINLINE mcastlocation_t mc_callframe_getpos(mcvmframe_t* frame)
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
    mem->gcobjlist = mc_ptrlist_make(state, sizeof(void*), true);
    if(!mem->gcobjlist)
    {
        goto error;
    }
    mem->gcobjlistback = mc_ptrlist_make(state, sizeof(void*), true);
    if(!mem->gcobjlistback)
    {
        goto error;
    }
    mem->gcobjlistremains = mc_ptrlist_make(state, sizeof(mcvalue_t), false);
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
    mcobjdata_t* data;
    mcgcobjdatapool_t* pool;
    if(!mem)
    {
        return;
    }
    mc_ptrlist_destroy(mem->gcobjlistremains, NULL);
    mc_ptrlist_destroy(mem->gcobjlistback, NULL);
    for(i = 0; i < mc_ptrlist_count(mem->gcobjlist); i++)
    {
        obj = (mcobjdata_t*)mc_ptrlist_get(mem->gcobjlist, i);
        mc_objectdata_deinit(obj);
        memset(obj, 0, sizeof(mcobjdata_t));
        mc_memory_free(obj);
    }
    mc_ptrlist_destroy(mem->gcobjlist, NULL);
    for(i = 0; i < MC_CONF_GCMEMPOOLCOUNT; i++)
    {
        pool = &mem->mempools[i];
        for(j = 0; j < (size_t)pool->count; j++)
        {
            data = pool->data[j];
            mc_objectdata_deinit(data);
            memset(data, 0, sizeof(mcobjdata_t));
            mc_memory_free(data);
        }
        memset(pool, 0, sizeof(mcgcobjdatapool_t));
    }
    for(i = 0; i < (size_t)mem->onlydatapool.count; i++)
    {
        mc_memory_free(mem->onlydatapool.data[i]);
    }
    mc_memory_free(mem);
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
        mc_memory_free(data);
        return NULL;
    }
    ok = mc_ptrlist_push(state->mem->gcobjlist, data);
    if(!ok)
    {
        mc_memory_free(data);
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
    switch(mc_value_gettype(obj))
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
                len = mc_value_arraygetlength(obj);
                for(i = 0; i < len; i++)
                {
                    val = mc_value_arraygetvalue(obj, i);
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
    mc_state_gcmarkobjlist((mcvalue_t*)mc_ptrlist_data(state->mem->gcobjlistremains), mc_ptrlist_count(state->mem->gcobjlistremains));
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
                pool = mc_state_gcgetpoolfortype(state, data->odtype);
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
                    mc_memory_free(data);
                    data = NULL;
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


bool mc_state_gcdisablefor(mcvalue_t obj)
{
    bool ok;
    mcobjdata_t* data;
    if(!mc_value_isallocated(obj))
    {
        return false;
    }
    data = mc_value_getallocateddata(obj);
    if(mc_ptrlist_contains(data->mem->gcobjlistremains, &obj))
    {
        return false;
    }
    ok = mc_ptrlist_push(data->mem->gcobjlistremains, &obj);
    return ok;
}

void mc_state_gcenablefor(mcvalue_t obj)
{
    mcobjdata_t* data;
    if(!mc_value_isallocated(obj))
    {
        return;
    }
    data = mc_value_getallocateddata(obj);
    mc_ptrlist_removeitem(data->mem->gcobjlistremains, &obj);
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
    obj = mc_object_makedatafrom(data->odtype, data);
    /*
    * this is to ensure that large objects won't be kept in pool indefinitely
    */
    switch(data->odtype)
    {
        case MC_VAL_ARRAY:
            {
                if(mc_value_arraygetlength(obj) > 1024)
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
    pool= mc_state_gcgetpoolfortype(state, data->odtype);
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
    store->storedsymbols = mc_ptrdict_make(state, (mcitemcopyfn_t)mc_symbol_copy, (mcitemdestroyfn_t)mc_symbol_destroy);
    if(!store->storedsymbols)
    {
        goto err;
    }
    store->storedobjects = mc_vallist_make(state, "globalstore", 0);
    if(!store->storedobjects)
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
    mc_ptrdict_destroyitemsanddict(store->storedsymbols);
    mc_vallist_destroy(store->storedobjects);
    mc_memory_free(store);
    store = NULL;
}

mcastsymbol_t* mc_globalstore_getsymbol(mcglobalstore_t* store, const char* name)
{
    return (mcastsymbol_t*)mc_ptrdict_get(store->storedsymbols, name);
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
        ok = mc_vallist_set(store->storedobjects, existingsymbol->index, object);
        return ok;
    }
    ix = mc_vallist_count(store->storedobjects);
    ok = mc_vallist_push(store->storedobjects, object);
    if(!ok)
    {
        return false;
    }
    symbol = mc_symbol_make(store->pstate, name, MC_SYM_GLOBALBUILTIN, ix, false);
    if(!symbol)
    {
        goto err;
    }
    ok = mc_ptrdict_set(store->storedsymbols, name, symbol);
    if(!ok)
    {
        mc_symbol_destroy(symbol);
        goto err;
    }
    return true;
err:
    mc_vallist_pop(store->storedobjects, NULL);
    return false;
}

mcvalue_t mc_globalstore_getatindex(mcglobalstore_t* store, int ix, bool* outok)
{
    mcvalue_t* res;
    res = (mcvalue_t*)mc_vallist_getp(store->storedobjects, ix);
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
    return (mcvalue_t*)mc_vallist_data(store->storedobjects);
}

int mc_globalstore_getcount(mcglobalstore_t* store)
{
    return mc_vallist_count(store->storedobjects);
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
        mc_memory_free(symbol);
        symbol = NULL;
        return NULL;
    }
    symbol->symtype = type;
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
    mc_memory_free(symbol->name);
    symbol->name = NULL;
    mc_memory_free(symbol);
    symbol = NULL;
}

mcastsymbol_t* mc_symbol_copy(mcastsymbol_t* symbol)
{
    return mc_symbol_make(symbol->pstate, symbol->name, symbol->symtype, symbol->index, symbol->assignable);
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
    table->blockscopes = mc_ptrlist_make(state, sizeof(void*), true);
    if(!table->blockscopes)
    {
        goto err;
    }
    table->freesymbols = mc_ptrlist_make(state, sizeof(void*), true);
    if(!table->freesymbols)
    {
        goto err;
    }
    table->modglobalsymbols = mc_ptrlist_make(state, sizeof(void*), true);
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
    memset(table, 0, sizeof(mcastsymtable_t));
    mc_memory_free(table);
    table = NULL;
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
    if(symbol->symtype != MC_SYM_MODULEGLOBAL)
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
            globalsymbolcopy = (mcastsymbol_t*)mc_ptrlist_popret(table->modglobalsymbols);
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
    copy = mc_symbol_make(st->pstate, original->name, original->symtype, original->index, original->assignable);
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
        symbol = (mcastsymbol_t*)mc_ptrdict_get(scope->scopestore, name);
        if(symbol)
        {
            break;
        }
    }
    if(symbol && symbol->symtype == MC_SYM_THIS)
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
        if(symbol->symtype == MC_SYM_MODULEGLOBAL || symbol->symtype == MC_SYM_GLOBALBUILTIN)
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
    symbol = (mcastsymbol_t*)mc_ptrdict_get(topscope->scopestore, name);
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
    mc_ptrlist_pop(table->blockscopes, NULL);
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
    newscope->scopestore = mc_ptrdict_make(state, (mcitemcopyfn_t)mc_symbol_copy, (mcitemdestroyfn_t)mc_symbol_destroy);
    if(!newscope->scopestore)
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
    mc_ptrdict_destroyitemsanddict(scope->scopestore);
    mc_memory_free(scope);
    scope = NULL;
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
    copy->scopestore = mc_ptrdict_copy(scope->scopestore);
    if(!copy->scopestore)
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
    existing = (mcastsymbol_t*)mc_ptrdict_get(topscope->scopestore, symbol->name);
    if(existing)
    {
        mc_symbol_destroy(existing);
    }
    return mc_ptrdict_set(topscope->scopestore, symbol->name, symbol);
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
    tok->toktype = type;
    tok->tokstrdata = literal;
    tok->tokstrlen = len;
}

char* mc_asttoken_dupliteralstring(mcstate_t* state, mcasttoken_t* tok)
{
    return mc_util_strndup(state, tok->tokstrdata, tok->tokstrlen);
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
    traceback->tbitems = mc_ptrlist_make(state, sizeof(mctraceitem_t), false);
    if(!traceback->tbitems)
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
    for(i = 0; i < mc_ptrlist_count(traceback->tbitems); i++)
    {
        item = (mctraceitem_t*)mc_ptrlist_get(traceback->tbitems, i);
        mc_memory_free(item->trfuncname);
        item->trfuncname = NULL;
    }
    mc_ptrlist_destroy(traceback->tbitems, NULL);
    mc_memory_free(traceback);
    traceback = NULL;
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
    ok = mc_ptrlist_push(traceback->tbitems, &item);
    if(!ok)
    {
        mc_memory_free(item.trfuncname);
        item.trfuncname = NULL;
        return false;
    }
    return true;
}

bool mc_traceback_vmpush(mctraceback_t* traceback, mcstate_t* state)
{
    bool ok;
    int i;
    mcvmframe_t* frame;
    for(i = state->execstate.framestack->listcount - 1; i >= 0; i--)
    {
        frame = mc_framelist_get(state->execstate.framestack, i);
        ok = mc_traceback_push(traceback, mc_value_functiongetname(frame->function), mc_callframe_getpos(frame));
        if(!ok)
        {
            return false;
        }
    }
    return true;
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
    state->execstate.vsposition = 0;
    state->execstate.thisstpos = 0;
    state->execstate.lastpopped = mc_value_makenull();
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
    state->execstate.vsposition = 0;
    state->execstate.thisstpos = 0;
    while(state->execstate.framestack->listcount > 0)
    {
        mc_vm_popframe(state);
    }
}

bool mc_vm_runexecfunc(mcstate_t* state, mccompiledprogram_t* comp_res, mcvallist_t* constants)
{
    bool res;
    size_t oldsp;
    size_t oldthissp;
    size_t oldframescount;
    mcvalue_t mainfn;
    (void)oldsp;
    oldsp = state->execstate.vsposition;
    oldthissp = state->execstate.thisstpos;
    oldframescount = state->execstate.framestack->listcount;
    mainfn = mc_value_makefuncscript(state, "main", comp_res, false, 0, 0, 0);
    if(mc_value_isnull(mainfn))
    {
        return false;
    }
    mc_vm_stackpush(state, mainfn);
    res = mc_function_execfunction(state, mainfn, constants, false);
    while(state->execstate.framestack->listcount > oldframescount)
    {
        mc_vm_popframe(state);
    }
    //MC_ASSERT(state->execstate.vsposition == oldsp);
    state->execstate.thisstpos = oldthissp;
    return res;
}




MC_FORCEINLINE bool mc_vmdo_tryoverloadoperator(mcstate_t* state, mcvalue_t left, mcvalue_t right, mcinternopcode_t op, bool* outoverloadfound)
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

MC_FORCEINLINE mcvalue_t mc_vm_getlastpopped(mcstate_t* state)
{
    return state->execstate.lastpopped;
}

MC_FORCEINLINE bool mc_vm_haserrors(mcstate_t* state)
{
    return state->errors.count > 0;
}

MC_FORCEINLINE bool mc_vm_setglobalbyindex(mcstate_t* state, size_t ix, mcvalue_t val)
{
    mc_vallist_set(state->globalvalstack, ix, val);
    if(ix >= state->globalvalcount)
    {
        state->globalvalcount = ix + 1;
    }
    return true;
}

MC_FORCEINLINE mcvalue_t mc_vm_getglobalbyindex(mcstate_t* state, size_t ix)
{
    return mc_vallist_get(state->globalvalstack, ix);
}

MC_FORCEINLINE void mc_vm_setstackpos(mcstate_t* state, size_t nsp)
{
    #if 0
    size_t i;
    size_t count;
    size_t bytescount;
    if(nsp > state->execstate.vsposition)
    {
        /* to avoid gcing freed objects */
        count = nsp - state->execstate.vsposition;
        bytescount = (count - 0) * sizeof(mcvalue_t);
        for(i=(state->execstate.vsposition - 0); (i != bytescount) && (i < state->execstate.valuestack->listcapacity); i++)
        {
            //memset(&state->execstate.valuestack->listitems[i], 0, sizeof(mcvalue_t));
            state->execstate.valuestack->listitems[i].valtype = MC_VAL_NULL;
        }
    }
    #endif
    state->execstate.vsposition = nsp;
}

MC_FORCEINLINE void mc_vm_stackpush(mcstate_t* state, mcvalue_t obj)
{
    int numlocals;
    mcvmframe_t* frame;
    mcobjfuncscript_t* currentfunction;
    (void)numlocals;
    (void)frame;
    (void)currentfunction;
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(state->execstate.currframe)
    {
        frame = state->execstate.currframe;
        currentfunction = mc_value_functiongetscriptfunction(frame->function);
        numlocals = currentfunction->numlocals;
        MC_ASSERT((size_t)state->execstate.vsposition >= (size_t)(frame->basepointer + numlocals));
    }
#endif
    #if 1
        mc_vallist_set(state->execstate.valuestack, state->execstate.vsposition, obj);
    #else
        mc_vallist_push(state->execstate.valuestack, obj);
    #endif
    state->execstate.vsposition++;
}

MC_FORCEINLINE mcvalue_t mc_vm_stackpop(mcstate_t* state)
{
    int numlocals;
    mcvalue_t res;
    mcvmframe_t* frame;
    mcobjfuncscript_t* currentfunction;
    (void)numlocals;
    (void)frame;
    (void)currentfunction;
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(state->execstate.vsposition == 0)
    {
        mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "stack underflow");
        MC_ASSERT(false);
        return mc_value_makenull();
    }
    if(state->execstate.currframe)
    {
        frame = state->execstate.currframe;
        currentfunction = mc_value_functiongetscriptfunction(frame->function);
        numlocals = currentfunction->numlocals;
        MC_ASSERT((state->execstate.vsposition - 1) >= (frame->basepointer + numlocals));
    }
#endif
    state->execstate.vsposition--;
    res = mc_vallist_get(state->execstate.valuestack, state->execstate.vsposition);
    state->execstate.lastpopped = res;
    return res;
}

MC_FORCEINLINE mcvalue_t mc_vm_stackget(mcstate_t* state, size_t nthitem)
{
    size_t ix;
    ix = state->execstate.vsposition - 1 - nthitem;
    return mc_vallist_get(state->execstate.valuestack, ix);
}

MC_FORCEINLINE void mc_vm_thisstackpush(mcstate_t* state, mcvalue_t obj)
{
    mc_vallist_set(state->execstate.valthisstack, state->execstate.thisstpos, obj);
    state->execstate.thisstpos++;
}

MC_FORCEINLINE mcvalue_t mc_vm_thisstackpop(mcstate_t* state)
{
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(state->execstate.thisstpos == 0)
    {
        mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "'this' stack underflow");
        MC_ASSERT(false);
        return mc_value_makenull();
    }
#endif
    state->execstate.thisstpos--;
    return mc_vallist_get(state->execstate.valthisstack, state->execstate.thisstpos);
}

MC_FORCEINLINE mcvalue_t mc_vm_thisstackget(mcstate_t* state, int nthitem)
{
    int ix;
    ix = state->execstate.thisstpos - 1 - nthitem;
    return mc_vallist_get(state->execstate.valthisstack, ix);
}

MC_FORCEINLINE bool mc_vm_pushframe(mcstate_t* state, mcvmframe_t frame)
{
    int add;
    mcobjfuncscript_t* framefunction;
    add = 0;
    #if 1
        mc_framelist_set(state->execstate.framestack, state->execstate.framestack->listcount, frame);
        add = 1;
    #else
        mc_framelist_push(state->execstate.framestack, frame);
    #endif
    state->execstate.currframe = mc_framelist_get(state->execstate.framestack, state->execstate.framestack->listcount);
    state->execstate.framestack->listcount += add;
    framefunction = mc_value_functiongetscriptfunction(frame.function);
    mc_vm_setstackpos(state, frame.basepointer + framefunction->numlocals);
    return true;
}

MC_FORCEINLINE bool mc_vm_popframe(mcstate_t* state)
{
    mc_vm_setstackpos(state, state->execstate.currframe->basepointer - 1);
    if(state->execstate.framestack->listcount <= 0)
    {
        MC_ASSERT(false);
        state->execstate.currframe = NULL;
        return false;
    }
    state->execstate.framestack->listcount--;
    if(state->execstate.framestack->listcount == 0)
    {
        state->execstate.currframe = NULL;
        return false;
    }
    state->execstate.currframe = mc_framelist_get(state->execstate.framestack, state->execstate.framestack->listcount - 1);
    return true;
}

MC_INLINE void mc_vm_rungc(mcstate_t* state, mcvallist_t* constants)
{
    size_t i;
    mcvmframe_t* frame;
    mc_state_gcunmarkall(state);
    mc_state_gcmarkobjlist(mc_globalstore_getdata(state->vmglobalstore), mc_globalstore_getcount(state->vmglobalstore));
    mc_state_gcmarkobjlist((mcvalue_t*)mc_vallist_data(constants), mc_vallist_count(constants));
    mc_state_gcmarkobjlist(mc_vallist_data(state->globalvalstack), state->globalvalcount);
    for(i = 0; i < state->execstate.framestack->listcount; i++)
    {
        frame = mc_framelist_get(state->execstate.framestack, i);
        mc_state_gcmarkobject(frame->function);
    }
    mc_state_gcmarkobjlist(mc_vallist_data(state->execstate.valuestack), state->execstate.vsposition);
    mc_state_gcmarkobjlist(mc_vallist_data(state->execstate.valthisstack), state->execstate.thisstpos);
    mc_state_gcmarkobject(state->execstate.lastpopped);
    mc_state_gcmarkobjlist(state->operoverloadkeys, MC_CONF_MAXOPEROVERLOADS);
    mc_state_gcsweep(state);
}

MC_FORCEINLINE bool mc_vmdo_callobject(mcstate_t* state, mcvalue_t callee, int nargs)
{
    bool ok;
    const char* calleetypename;
    mcvaltype_t calleetype;
    mcvmframe_t calleeframe;
    mcvalue_t res;
    mcvalue_t tmpval;
    mcvalue_t selfval;
    mcvalue_t* stackpos;
    mcobjfuncscript_t* calleefunction;
    calleetype = mc_value_gettype(callee);
    selfval = mc_value_makenull();
    if(mc_value_isfuncnative(callee))
    {
        if(!mc_vallist_pop(state->execstate.nativethisstack, &tmpval))
        {
            #if 0
                mc_printer_printf(state->stderrprinter, "failed to pop native 'this' for = <");
                mc_printer_printvalue(state->stderrprinter, callee, true);
                mc_printer_printf(state->stderrprinter, ">\n");
                #if 0
                    mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "failed to pop native 'this'");
                #endif
            #endif
        }
        selfval = tmpval;
    }
    #if 0
    {
        mc_printer_printf(state->stderrprinter, "selfval = <<<");
        mc_printer_printvalue(state->stderrprinter, selfval, true);
        mc_printer_printf(state->stderrprinter, ">>>\n");
    }
    #endif
    if(calleetype == MC_VAL_FUNCSCRIPT)
    {
        calleefunction = mc_value_functiongetscriptfunction(callee);
        if(nargs != calleefunction->numargs)
        {
            #if 0
            mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "invalid number of arguments to \"%s\": expected %d, got %d",
                              mc_value_functiongetname(callee), calleefunction->numargs, nargs);
            return false;
            #endif
        }
        ok = mc_callframe_init(&calleeframe, callee, state->execstate.vsposition - nargs);
        if(!ok)
        {
            mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "frame init failed in mc_vmdo_callobject");
            return false;
        }
        ok = mc_vm_pushframe(state, calleeframe);
        if(!ok)
        {
            mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "pushing frame failed in mc_vmdo_callobject");
            return false;
        }
    }
    else if(calleetype == MC_VAL_FUNCNATIVE)
    {
        #if 0
        if(!mc_value_isnull(selfval))
        {
            mc_vm_stackpop(state);
        }
        #endif
        stackpos = mc_vallist_data(state->execstate.valuestack) + state->execstate.vsposition - nargs;
        res = mc_vm_callnativefunction(state, callee, mc_callframe_getpos(state->execstate.currframe), selfval, nargs, stackpos);
        if(mc_vm_haserrors(state))
        {
            return false;
        }
        mc_vm_setstackpos(state, state->execstate.vsposition - nargs - 1);
        mc_vm_stackpush(state, res);
    }
    else
    {
        calleetypename = mc_valtype_getname(calleetype);
        mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "%s object is not callable", calleetypename);
        return false;
    }
    return true;
}

MC_FORCEINLINE mcvalue_t mc_vm_callnativefunction(mcstate_t* state, mcvalue_t callee, mcastlocation_t srcpos, mcvalue_t selfval, size_t argc, mcvalue_t* args)
{
    mcvaltype_t restype;
    mcvalue_t res;
    mcerror_t* err; 
    mctraceback_t* traceback;
    mcobjfuncnative_t* nativefun;
    nativefun = mc_value_functiongetnativefunction(callee);
    res = nativefun->natptrfn(state, nativefun->userpointer, selfval, argc, args);
    if(mc_util_unlikely(state->errors.count > 0))
    {
        err = mc_errlist_getlast(&state->errors);
        err->pos = srcpos;
        err->traceback = mc_traceback_make(state);
        if(err->traceback)
        {
            mc_traceback_push(err->traceback, nativefun->name, srcposinvalid);
        }
        return mc_value_makenull();
    }
    restype = mc_value_gettype(res);
    if(mc_util_unlikely(restype == MC_VAL_ERROR))
    {
        traceback = mc_traceback_make(state);
        if(traceback)
        {
            /* error builtin is treated in a special way */
            if(!MC_UTIL_STREQ(nativefun->name, "error"))
            {
                mc_traceback_push(traceback, nativefun->name, srcposinvalid);
            }
            mc_traceback_vmpush(traceback, state);
            mc_value_errorsettraceback(res, traceback);
        }
    }
    return res;
}

MC_FORCEINLINE bool mc_vm_checkassign(mcstate_t* state, mcvalue_t oldvalue, mcvalue_t nvalue)
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
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "trying to assign variable of type %s to %s",
                          mc_valtype_getname(nvaluetype), mc_valtype_getname(oldvaluetype));
        return false;
    }
    #endif
    return true;
}


MC_FORCEINLINE bool mc_vmdo_opaddstring(mcstate_t* state, mcvalue_t valleft, mcvalue_t valright, mcvaltype_t righttype, mcopcode_t opcode)
{
    mcvalue_t nstring;
    (void)opcode;
    (void)righttype;
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

MC_FORCEINLINE bool mc_vmdo_math(mcstate_t* state, mcopcode_t opcode)
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
    if(lefttype == MC_VAL_STRING && opcode == MC_OPCODE_ADD)
    {
        if(mc_vmdo_opaddstring(state, valleft, valright, righttype, opcode))
        {
            return true;
        }
    }
    else if((mc_value_isnumeric(valleft) || mc_value_isnull(valleft)) && (mc_value_isnumeric(valright) || mc_value_isnull(valright)))
    {
        dnright = mc_value_asnumber(valright);
        dnleft = mc_value_asnumber(valleft);
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
            /*
            // TODO: shifting, signedness: how does nodejs do it?
            // enabling checks for <0 breaks sha1.mc!
            */
            case MC_OPCODE_LSHIFT:
                {
                    #if 0
                    if((dnleft < 0) || (dnright < 0))
                    {
                        res = (int64_t)dnleft << (int64_t)dnright;
                    }
                    else
                    #endif
                    {
                        res = mc_mathutil_binshiftleft(dnleft, dnright);
                    }
                }
                break;
            case MC_OPCODE_RSHIFT:
                {
                    #if 0
                    if((dnleft < 0) || (dnright < 0))
                    {
                        res = (int64_t)dnleft >> (int64_t)dnright;
                    }
                    else
                    #endif
                    {
                        res = mc_mathutil_binshiftright(dnleft, dnright);
                    }
                }
                break;
            default:
                {
                    MC_ASSERT(false);
                }
                break;
        }
        mc_vm_stackpush(state, mc_value_makenumber(res));
        return true;
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
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "invalid operand types for %s: got %s and %s",
                          opcodename, lefttypename, righttypename);
        return false;
    }
    return true;
}

MC_FORCEINLINE mcclass_t* mc_vmdo_findclassforintern(mcstate_t* state, mcvaltype_t typ)
{
    (void)state;
    (void)typ;
    switch(typ)
    {
        case MC_VAL_NUMBER:
            {
                return state->stdobjnumber;
            }
            break;
        case MC_VAL_STRING:
            {
                return state->stdobjstring;
            }
            break;
        case MC_VAL_ARRAY:
            {
                return state->stdobjarray;
            }
            break;
        case MC_VAL_MAP:
            {
                return state->stdobjmap;
            }
            break;
        case MC_VAL_FUNCNATIVE:
        case MC_VAL_FUNCSCRIPT:
            {
                return state->stdobjfunction;
            }
            break;
        default:
            {
            }
            break;
    }
    return NULL;
}

MC_FORCEINLINE mcclass_t* mc_vmdo_findclassfor(mcstate_t* state, mcvaltype_t typ)
{
    mcclass_t* cl;
    cl = mc_vmdo_findclassforintern(state, typ);
    if(cl != NULL)
    {
        
    }
    return cl;
}

MC_INLINE mcfield_t* mc_vmdo_getclassmember(mcstate_t* state, mcclass_t* cl, const char* name)
{
    size_t i;
    mcfield_t* memb;
    (void)state;
    for(i=0; i<cl->members->listcount; i++)
    {
        memb = (mcfield_t*)mc_ptrlist_get(cl->members, i);
        if(strcmp(memb->name, name) == 0)
        {
            return memb;
        }
    }
    if(cl->parentclass != NULL)
    {
        return mc_vmdo_getclassmember(state, cl->parentclass, name);
    }
    return NULL;
}

MC_FORCEINLINE bool mc_vmdo_findclassmembervalue(mcstate_t* state, mcvalue_t left, mcvalue_t index, mcvalue_t setval)
{
    mcvalue_t fnval;
    mcvalue_t retv;
    const char* idxname;
    mcfield_t* vdest;
    (void)state;
    (void)left;
    (void)index;
    (void)setval;
    mcclass_t* cl;
    cl = mc_vmdo_findclassfor(state, mc_value_gettype(left));
    if(cl != NULL)
    {
        idxname = mc_value_getstringdata(index);
        vdest = mc_vmdo_getclassmember(state, cl, idxname);
        if(vdest == NULL)
        {
            return false;
        }
        else
        {
            fnval = mc_value_makefuncnative(state, vdest->name, vdest->fndest, NULL);
            if(vdest->ispseudo)
            {
                retv = mc_vm_callnativefunction(state, fnval, mc_callframe_getpos(state->execstate.currframe), left, 0, NULL);
                mc_vm_stackpush(state, retv);
                return true;
            }
            else
            {
                retv = fnval;
                mc_vm_stackpush(state, retv);
                return true;
            }
        }
    }
    return false;
}

MC_FORCEINLINE bool mc_vmdo_getindexpartial(mcstate_t* state, mcvalue_t left, mcvaltype_t lefttype, mcvalue_t index, mcvaltype_t indextype, bool fromdot)
{
    int leftlen;
    int ix;
    char resstr[2];
    const char* str;
    const char* indextypename;
    const char* lefttypename;
    mcvalue_t res;
    (void)fromdot;
    if(mc_value_isstring(index))
    {
        if(mc_vmdo_findclassmembervalue(state, left, index, mc_value_makenull()))
        {
            #if 0
            if(mc_value_isfuncnative(callee))
            #endif
            {
                mc_vallist_push(state->execstate.nativethisstack, left);
            }
            #if 0
                mc_printer_printf(state->stderrprinter, "getindexpartial:left=<");
                mc_printer_printvalue(state->stderrprinter, left, true);
                mc_printer_printf(state->stderrprinter, ">\n");
            #endif
            return true;
        }
    }
    if(lefttype != MC_VAL_ARRAY && lefttype != MC_VAL_MAP && lefttype != MC_VAL_STRING)
    {
        lefttypename = mc_valtype_getname(lefttype);
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "getindexpartial: type %s is not indexable", lefttypename);
        return false;
    }
    res = mc_value_makenull();
    if(lefttype == MC_VAL_ARRAY)
    {
        if(indextype != MC_VAL_NUMBER)
        {
            lefttypename = mc_valtype_getname(lefttype);
            indextypename = mc_valtype_getname(indextype);
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "cannot get partial index of %s with %s", lefttypename, indextypename);
            return false;
        }
        ix = (int)mc_value_asnumber(index);
        if(ix < 0)
        {
            ix = mc_value_arraygetlength(left) + ix;
        }
        if(ix >= 0 && ix < mc_value_arraygetlength(left))
        {
            res = mc_value_arraygetvalue(left, ix);
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
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "cannot index %s with %s", lefttypename, indextypename);
            return false;
        }
        str = mc_value_getstringdata(left);
        leftlen = mc_value_getstringlength(left);
        ix = (int)mc_value_asnumber(index);
        if(ix >= 0 && ix < leftlen)
        {
            resstr[0] = str[ix];
            res = mc_value_makestringlen(state, resstr, 1);
        }
    }
    mc_vm_stackpush(state, res);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_getindexfull(mcstate_t* state)
{
    mcvaltype_t lefttype;
    mcvaltype_t indextype;
    mcvalue_t left;
    mcvalue_t index;
    index = mc_vm_stackpop(state);
    left = mc_vm_stackpop(state);
    lefttype = mc_value_gettype(left);
    indextype = mc_value_gettype(index);
    return mc_vmdo_getindexpartial(state, left, lefttype, index, indextype, false);
}

MC_FORCEINLINE bool mc_vmdo_getdotindex(mcstate_t* state)
{
    mcvaltype_t lefttype;
    mcvaltype_t indextype;
    mcvalue_t left;
    mcvalue_t index;
    index = mc_vm_stackpop(state);
    left = mc_vm_stackpop(state);
    lefttype = mc_value_gettype(left);
    indextype = mc_value_gettype(index);
    return mc_vmdo_getindexpartial(state, left, lefttype, index, indextype, true);
}

MC_FORCEINLINE bool mc_vmdo_setindexpartial(mcstate_t* state, mcvalue_t left, mcvaltype_t lefttype, mcvalue_t index, mcvaltype_t indextype, mcvalue_t nvalue)
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
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "setindexpartial: type %s is not indexable", lefttypename);
        return false;
    }
    if(lefttype == MC_VAL_ARRAY)
    {
        if(indextype != MC_VAL_NUMBER)
        {
            lefttypename = mc_valtype_getname(lefttype);
            indextypename = mc_valtype_getname(indextype);
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "cannot set index of %s with %s", lefttypename, indextypename);
            return false;
        }
        ix = (int)mc_value_asnumber(index);                        
        ok = mc_value_arraysetvalue(left, ix, nvalue);
        alen = mc_value_arraygetlength(left);
        if(!ok)
        {
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "failed to set array index %d (of %d)", ix, alen);
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

MC_FORCEINLINE bool mc_vmdo_setindexfull(mcstate_t* state)
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

MC_FORCEINLINE bool mc_vmdo_getvalueatfull(mcstate_t* state)
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
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "getvalueatfull: type %s is not indexable", lefttypename);
        return false;
    }
    res = mc_value_makenull();
    if(indextype != MC_VAL_NUMBER)
    {
        lefttypename = mc_valtype_getname(lefttype);
        indextypename = mc_valtype_getname(indextype);
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "cannot get full index %s with %s", lefttypename, indextypename);
        return false;
    }
    ix = (int)mc_value_asnumber(index);
    if(lefttype == MC_VAL_ARRAY)
    {
        res = mc_value_arraygetvalue(left, ix);
    }
    else if(lefttype == MC_VAL_MAP)
    {
        res = mc_valmap_getkvpairat(state, left, ix);
    }
    else if(lefttype == MC_VAL_STRING)
    {
        str = mc_value_getstringdata(left);
        leftlen = mc_value_getstringlength(left);
        ix = (int)mc_value_asnumber(index);
        if(ix >= 0 && ix < leftlen)
        {
            resstr[0] = str[ix];
            res = mc_value_makestringlen(state, resstr, 1);
        }
    }
    mc_vm_stackpush(state, res);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_makefunction(mcstate_t* state, mcvallist_t* constants)
{
    int i;
    uint16_t numfree;
    uint16_t constantix;
    const char* fname;
    const char* tname;
    mcvaltype_t constanttype;
    mcvalue_t freeval;
    mcvalue_t functionobj;
    mcvalue_t* constant;
    mcobjfuncscript_t* constfun;
    constantix = mc_callframe_readuint16(state->execstate.currframe);
    numfree = mc_callframe_readuint8(state->execstate.currframe);
    constant = mc_vallist_getp(constants, constantix);
    if(!constant)
    {
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "constant %d not found", constantix);
        return false;
    }
    constanttype = mc_value_gettype(*constant);
    if(constanttype != MC_VAL_FUNCSCRIPT)
    {
        tname = mc_valtype_getname(constanttype);
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "%s is not a function", tname);
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
        freeval = mc_vallist_get(state->execstate.valuestack, state->execstate.vsposition - numfree + i);
        mc_value_functionsetfreevalat(functionobj, i, freeval);
    }
    mc_vm_setstackpos(state, state->execstate.vsposition - numfree);
    mc_vm_stackpush(state, functionobj);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_docmpvalue(mcstate_t* state, mcopcode_t opcode)
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
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "cannot compare %s and %s", lefttname, righttname);
            return false;
        }
    }
    return true;
}

MC_FORCEINLINE bool mc_vmdo_docmpvalgreater(mcstate_t* state, mcopcode_t opcode)
{
    bool resval;
    mcfloat_t comparisonres;
    mcvalue_t res;
    mcvalue_t value;
    value = mc_vm_stackpop(state);
    comparisonres = mc_value_asnumber(value);
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

MC_FORCEINLINE bool mc_vmdo_makearray(mcstate_t* state)
{
    bool ok;
    int i;
    uint16_t count;
    mcvalue_t item;
    mcvalue_t arrayobj;
    mcvalue_t* items;
    count = mc_callframe_readuint16(state->execstate.currframe);
    arrayobj = mc_value_makearraycapacity(state, count);
    if(mc_value_isnull(arrayobj))
    {
        return false;
    }
    items = mc_vallist_data(state->execstate.valuestack) + state->execstate.vsposition - count;
    for(i = 0; i < count; i++)
    {
        item = items[i];
        ok = mc_value_arraypush(arrayobj, item);
        if(!ok)
        {
            return false;
        }
    }
    mc_vm_setstackpos(state, state->execstate.vsposition - count);
    mc_vm_stackpush(state, arrayobj);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_makemapstart(mcstate_t* state)
{
    uint16_t count;
    mcvalue_t mapobj;
    count = mc_callframe_readuint16(state->execstate.currframe);
    mapobj = mc_value_makemapcapacity(state, count);
    if(mc_value_isnull(mapobj))
    {
        return false;
    }
    mc_vm_thisstackpush(state, mapobj);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_makemapend(mcstate_t* state)
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
    kvpcount = mc_callframe_readuint16(state->execstate.currframe);
    itemscount = kvpcount * 2;
    mapobj = mc_vm_thisstackpop(state);
    kvpairs = mc_vallist_data(state->execstate.valuestack) + state->execstate.vsposition - itemscount;
    for(i = 0; i < itemscount; i += 2)
    {
        key = kvpairs[i];
        if(!mc_value_ishashable(key))
        {
            keytype = mc_value_gettype(key);
            keytypename = mc_valtype_getname(keytype);
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "key of type %s is not hashable", keytypename);
            return false;
        }
        val = kvpairs[i + 1];
        ok = mc_valmap_setvalue(mapobj, key, val);
        if(!ok)
        {
            return false;
        }
    }
    mc_vm_setstackpos(state, state->execstate.vsposition - itemscount);
    mc_vm_stackpush(state, mapobj);
    return true;
}

#if defined(__GNUC__) || defined(__CLANG__) || defined(__TINYC__)
    #define MC_CONF_USECOMPUTEDGOTOS 1
#else
    #define MC_CONF_USECOMPUTEDGOTOS 0
#endif

#if defined(MC_CONF_USECOMPUTEDGOTOS) && (MC_CONF_USECOMPUTEDGOTOS == 1)
    #define MAKELABEL(opname) LABEL_##opname
    #define mcvm_case(opn) LABEL_##opn
    #define mc_vmmac_break() \
        goto readnextop
#else
    #define mcvm_case(opn) case opn
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

bool mc_function_execfunction(mcstate_t* state, mcvalue_t function, mcvallist_t* constants, bool nested)
{
    bool ok;
    int fri;
    int prevcode;
    int opcode;
    size_t recoverframeix;
    mcvmframe_t createdframe;
    mcvalue_t errobj;
    mcvmframe_t* frame;
    mcerror_t* err;    
    const char* oname;
    mcobjfuncscript_t* functionfunction;
    (void)oname;
    (void)prevcode;
    #if defined(MC_CONF_USECOMPUTEDGOTOS) && (MC_CONF_USECOMPUTEDGOTOS == 1)
        static void* dispatchtable[] = {
            &&MAKELABEL(MC_OPCODE_HALT),
            &&MAKELABEL(MC_OPCODE_CONSTANT),
            &&MAKELABEL(MC_OPCODE_ADD),
            &&MAKELABEL(MC_OPCODE_SUB),
            &&MAKELABEL(MC_OPCODE_MUL),
            &&MAKELABEL(MC_OPCODE_DIV),
            &&MAKELABEL(MC_OPCODE_MOD),
            &&MAKELABEL(MC_OPCODE_POP),
            &&MAKELABEL(MC_OPCODE_BINOR),
            &&MAKELABEL(MC_OPCODE_BINXOR),
            &&MAKELABEL(MC_OPCODE_BINAND),
            &&MAKELABEL(MC_OPCODE_LSHIFT),
            &&MAKELABEL(MC_OPCODE_RSHIFT),
            &&MAKELABEL(MC_OPCODE_BANG),
            &&MAKELABEL(MC_OPCODE_COMPARE),
            &&MAKELABEL(MC_OPCODE_TRUE),
            &&MAKELABEL(MC_OPCODE_FALSE),
            &&MAKELABEL(MC_OPCODE_COMPAREEQ),
            &&MAKELABEL(MC_OPCODE_EQUAL),
            &&MAKELABEL(MC_OPCODE_NOTEQUAL),
            &&MAKELABEL(MC_OPCODE_GREATERTHAN),
            &&MAKELABEL(MC_OPCODE_GREATERTHANEQUAL),
            &&MAKELABEL(MC_OPCODE_MINUS),
            &&MAKELABEL(MC_OPCODE_BINNOT),
            &&MAKELABEL(MC_OPCODE_JUMP),
            &&MAKELABEL(MC_OPCODE_JUMPIFFALSE),
            &&MAKELABEL(MC_OPCODE_JUMPIFTRUE),
            &&MAKELABEL(MC_OPCODE_NULL),
            &&MAKELABEL(MC_OPCODE_GETMODULEGLOBAL),
            &&MAKELABEL(MC_OPCODE_SETMODULEGLOBAL),
            &&MAKELABEL(MC_OPCODE_DEFINEMODULEGLOBAL),
            &&MAKELABEL(MC_OPCODE_ARRAY),
            &&MAKELABEL(MC_OPCODE_MAPSTART),
            &&MAKELABEL(MC_OPCODE_MAPEND),
            &&MAKELABEL(MC_OPCODE_GETTHIS),
            &&MAKELABEL(MC_OPCODE_GETINDEX),
            &&MAKELABEL(MC_OPCODE_SETINDEX),
            &&MAKELABEL(MC_OPCODE_GETDOTINDEX),
            &&MAKELABEL(MC_OPCODE_GETVALUEAT),
            &&MAKELABEL(MC_OPCODE_CALL),
            &&MAKELABEL(MC_OPCODE_RETURNVALUE),
            &&MAKELABEL(MC_OPCODE_RETURN),
            &&MAKELABEL(MC_OPCODE_GETLOCAL),
            &&MAKELABEL(MC_OPCODE_DEFINELOCAL),
            &&MAKELABEL(MC_OPCODE_SETLOCAL),
            &&MAKELABEL(MC_OPCODE_GETGLOBALBUILTIN),
            &&MAKELABEL(MC_OPCODE_FUNCTION),
            &&MAKELABEL(MC_OPCODE_GETFREE),
            &&MAKELABEL(MC_OPCODE_SETFREE),
            &&MAKELABEL(MC_OPCODE_CURRENTFUNCTION),
            &&MAKELABEL(MC_OPCODE_DUP),
            &&MAKELABEL(MC_OPCODE_NUMBER),
            &&MAKELABEL(MC_OPCODE_FOREACHLEN),
            &&MAKELABEL(MC_OPCODE_SETRECOVER),
        };
    #endif
    #if 0
    if(mc_util_unlikely(state->running))
    {
        mc_errlist_addf(&state->errors, MC_ERROR_USER, srcposinvalid, "state is already executing code");
        return false;
    }
    #endif
    /* naming is hard */
    functionfunction = mc_value_functiongetscriptfunction(function);
    ok = false;
    ok = mc_callframe_init(&createdframe, function, state->execstate.vsposition - functionfunction->numargs);
    if(!ok)
    {
        fprintf(stderr, "failed to init frames!\n");
        return false;
    }
    ok = mc_vm_pushframe(state, createdframe);
    if(!ok)
    {
        mc_errlist_addf(&state->errors, MC_ERROR_USER, srcposinvalid, "pushing frame failed");
        return false;
    }
    fprintf(stderr, "**executing function**\n");
    state->running = true;
    state->execstate.lastpopped = mc_value_makenull();
    //while(state->execstate.currframe->bcposition < state->execstate.currframe->bcsize)
    while(true)
    {
        readnextop:
        prevcode = opcode;
        if(state->execstate.currframe == NULL)
        {
            goto onexecfinish;
        }
        opcode = mc_callframe_readopcode(state->execstate.currframe);
        if(mc_util_unlikely(state->config.printinstructions))
        {
            mc_vmutil_getopinfo((mcopcode_t)opcode, &oname);
            fprintf(stderr, "opcode=%d (%s)\n", opcode, oname);
        }
        #if defined(MC_CONF_USECOMPUTEDGOTOS) && (MC_CONF_USECOMPUTEDGOTOS == 1)
            goto* dispatchtable[opcode];
        #else
        switch(opcode)
        #endif
        {
            #if !defined(MC_CONF_USECOMPUTEDGOTOS) || (MC_CONF_USECOMPUTEDGOTOS == 0)
            default:
                {
                    const char* prevname;
                    const char* thisname;
                    mc_vmutil_getopinfo((mcopcode_t)opcode, &thisname);
                    mc_vmutil_getopinfo((mcopcode_t)prevcode, &prevname);
                    mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "unknown opcode: %d (%s) (previous opcode was %d (%s))", opcode, thisname, prevcode, prevname);
                    MC_ASSERT(false);
                    goto onexecerror;
                }
                break;
            #endif
            mcvm_case(MC_OPCODE_HALT):
                {
                    goto onexecfinish;
                }
            mcvm_case(MC_OPCODE_RETURNVALUE):
                {
                    mcvalue_t res;
                    res = mc_vm_stackpop(state);
                    ok = mc_vm_popframe(state);
                    if(!ok)
                    {
                        goto onexecfinish;
                    }
                    mc_vm_stackpush(state, res);
                    if(nested)
                    {
                        goto onexecfinish;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_RETURN):
                {
                    ok = mc_vm_popframe(state);
                    mc_vm_stackpush(state, mc_value_makenull());
                    if(!ok)
                    {
                        mc_vm_stackpop(state);
                        goto onexecfinish;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_CONSTANT):
                {
                    uint16_t constantix;
                    mcvalue_t* constant;
                    constantix = mc_callframe_readuint16(state->execstate.currframe);
                    constant = (mcvalue_t*)mc_vallist_getp(constants, constantix);
                    if(!constant)
                    {
                        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "constant at %d not found", constantix);
                        goto onexecerror;
                    }
                    mc_vm_stackpush(state, *constant);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_ADD):
            mcvm_case(MC_OPCODE_SUB):
            mcvm_case(MC_OPCODE_MUL):
            mcvm_case(MC_OPCODE_DIV):
            mcvm_case(MC_OPCODE_MOD):
            mcvm_case(MC_OPCODE_BINOR):
            mcvm_case(MC_OPCODE_BINXOR):
            mcvm_case(MC_OPCODE_BINAND):
            mcvm_case(MC_OPCODE_LSHIFT):
            mcvm_case(MC_OPCODE_RSHIFT):
                {
                    if(!mc_vmdo_math(state, (mcopcode_t)opcode))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_POP):
                {
                    mc_vm_stackpop(state);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_TRUE):
                {
                    mc_vm_stackpush(state, mc_value_makebool(true));
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_FALSE):
                {
                    mc_vm_stackpush(state, mc_value_makebool(false));
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_COMPARE):
            mcvm_case(MC_OPCODE_COMPAREEQ):
                {
                    if(!mc_vmdo_docmpvalue(state, (mcopcode_t)opcode))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_EQUAL):
            mcvm_case(MC_OPCODE_NOTEQUAL):
            mcvm_case(MC_OPCODE_GREATERTHAN):
            mcvm_case(MC_OPCODE_GREATERTHANEQUAL):
                {
                    if(!mc_vmdo_docmpvalgreater(state, (mcopcode_t)opcode))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_MINUS):
                {
                    bool overloadfound;
                    mcfloat_t val;
                    const char* opertname;
                    mcvalue_t res;
                    mcvaltype_t opertype;
                    mcvalue_t operand;
                    operand = mc_vm_stackpop(state);
                    opertype = mc_value_gettype(operand);
                    if(mc_util_likely(opertype == MC_VAL_NUMBER))
                    {
                        val = mc_value_asnumber(operand);
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
                            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "invalid operand type for MINUS, got %s", opertname);
                            goto onexecerror;
                        }
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_BINNOT):
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
                        val = mc_value_asnumber(operand);
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
                            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "invalid operand type for BINNOT, got %s", opertname);
                            goto onexecerror;
                        }
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_BANG):
                {
                    bool overloadfound;
                    mcvalue_t res;
                    mcvalue_t operand;
                    mcvaltype_t type;
                    operand = mc_vm_stackpop(state);
                    type = mc_value_gettype(operand);
                    if(type == MC_VAL_BOOL)
                    {
                        res = mc_value_makebool(!mc_value_asbool(operand));
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
            mcvm_case(MC_OPCODE_JUMP):
                {
                    uint16_t pos;
                    pos = mc_callframe_readuint16(state->execstate.currframe);
                    state->execstate.currframe->bcposition = pos;
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_JUMPIFFALSE):
                {
                    uint16_t pos;
                    mcvalue_t test;
                    pos = mc_callframe_readuint16(state->execstate.currframe);
                    test = mc_vm_stackpop(state);
                    if(!mc_value_asbool(test))
                    {
                        state->execstate.currframe->bcposition = pos;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_JUMPIFTRUE):
                {
                    uint16_t pos;
                    mcvalue_t test;
                    pos = mc_callframe_readuint16(state->execstate.currframe);
                    test = mc_vm_stackpop(state);
                    if(mc_value_asbool(test))
                    {
                        state->execstate.currframe->bcposition = pos;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_NULL):
                {
                    mc_vm_stackpush(state, mc_value_makenull());
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_DEFINEMODULEGLOBAL):
                {
                    uint16_t ix;
                    mcvalue_t value;
                    ix = mc_callframe_readuint16(state->execstate.currframe);
                    value = mc_vm_stackpop(state);
                    mc_vm_setglobalbyindex(state, ix, value);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_SETMODULEGLOBAL):
                {
                    uint16_t ix;
                    mcvalue_t nvalue;
                    mcvalue_t oldvalue;
                    ix = mc_callframe_readuint16(state->execstate.currframe);
                    nvalue = mc_vm_stackpop(state);
                    oldvalue= mc_vm_getglobalbyindex(state, ix);
                    if(!mc_vm_checkassign(state, oldvalue, nvalue))
                    {
                        goto onexecerror;
                    }
                    mc_vm_setglobalbyindex(state, ix, nvalue);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_GETMODULEGLOBAL):
                {
                    uint16_t ix;
                    mcvalue_t global;
                    ix = mc_callframe_readuint16(state->execstate.currframe);
                    global = mc_vallist_get(state->globalvalstack, ix);
                    mc_vm_stackpush(state, global);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_ARRAY):
                {
                    if(!mc_vmdo_makearray(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_MAPSTART):
                {
                    if(!mc_vmdo_makemapstart(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_MAPEND):
                {
                    if(!mc_vmdo_makemapend(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_GETVALUEAT):
                {
                    if(!mc_vmdo_getvalueatfull(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_CALL):
                {
                    uint16_t nargs;
                    mcvalue_t callee;
                    nargs = mc_callframe_readuint8(state->execstate.currframe);
                    callee = mc_vm_stackget(state, nargs);
                    ok = mc_vmdo_callobject(state, callee, nargs);
                    if(!ok)
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_DEFINELOCAL):
                {
                    uint16_t pos;
                    pos = mc_callframe_readuint8(state->execstate.currframe);
                    mc_vallist_set(state->execstate.valuestack, state->execstate.currframe->basepointer + pos, mc_vm_stackpop(state));
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_SETLOCAL):
                {
                    uint16_t pos;
                    mcvalue_t nvalue;
                    mcvalue_t oldvalue;
                    pos = mc_callframe_readuint8(state->execstate.currframe);
                    nvalue = mc_vm_stackpop(state);
                    oldvalue = mc_vallist_get(state->execstate.valuestack, state->execstate.currframe->basepointer + pos);
                    if(!mc_vm_checkassign(state, oldvalue, nvalue))
                    {
                        goto onexecerror;
                    }
                    mc_vallist_set(state->execstate.valuestack, state->execstate.currframe->basepointer + pos, nvalue);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_GETLOCAL):
                {
                    size_t finalpos;
                    size_t pos;
                    mcvalue_t val;
                    pos = mc_callframe_readuint8(state->execstate.currframe);
                    finalpos = state->execstate.currframe->basepointer + pos;
                    val = mc_vallist_get(state->execstate.valuestack, finalpos);
                    #if 0
                    {
                        mcprinter_t* pr = state->stderrprinter;
                        mc_printer_printf(pr, "GETLOCAL: finalpos=%ld val=<<<", finalpos);
                        mc_printer_printvalue(pr, val, true);
                        mc_printer_printf(pr, ">>>\n");
                    }
                    #endif
                    mc_vm_stackpush(state, val);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_GETGLOBALBUILTIN):
                {
                    uint16_t ix;
                    mcvalue_t val;
                    ix = mc_callframe_readuint16(state->execstate.currframe);
                    ok = false;
                    val = mc_globalstore_getatindex(state->vmglobalstore, ix, &ok);
                    if(!ok)
                    {
                        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "global value %d not found", ix);
                        goto onexecerror;
                    }
                    mc_vm_stackpush(state, val);
                }
                mc_vmmac_break();

            mcvm_case(MC_OPCODE_FUNCTION):
                {
                    if(!mc_vmdo_makefunction(state, constants))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_GETFREE):
                {
                    uint16_t freeix;
                    mcvalue_t val;
                    freeix = mc_callframe_readuint8(state->execstate.currframe);
                    val = mc_value_functiongetfreevalat(state->execstate.currframe->function, freeix);
                    mc_vm_stackpush(state, val);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_SETFREE):
                {
                    uint16_t freeix;
                    mcvalue_t val;
                    freeix = mc_callframe_readuint8(state->execstate.currframe);
                    val = mc_vm_stackpop(state);
                    mc_value_functionsetfreevalat(state->execstate.currframe->function, freeix, val);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_CURRENTFUNCTION):
                {
                    mcvalue_t currentfunction;
                    currentfunction = state->execstate.currframe->function;
                    mc_vm_stackpush(state, currentfunction);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_GETTHIS):
                {
                    mcvalue_t obj;
                    obj = mc_vm_thisstackget(state, 0);
                    mc_vm_stackpush(state, obj);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_GETDOTINDEX):
                {
                    if(!mc_vmdo_getdotindex(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_GETINDEX):
                {
                    if(!mc_vmdo_getindexfull(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_SETINDEX):
                {
                    if(!mc_vmdo_setindexfull(state))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_DUP):
                {
                    mcvalue_t val;
                    val = mc_vm_stackget(state, 0);
                    mc_vm_stackpush(state, val);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_FOREACHLEN):
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
                        len = mc_value_arraygetlength(val);
                    }
                    else if(type == MC_VAL_MAP)
                    {
                        len = mc_valmap_getlength(val);
                    }
                    else if(type == MC_VAL_STRING)
                    {
                        len = mc_value_getstringlength(val);
                    }
                    else
                    {
                        tname = mc_valtype_getname(type);
                        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "cannot get length of %s", tname);
                        goto onexecerror;
                    }
                    mc_vm_stackpush(state, mc_value_makenumber(len));
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_NUMBER):
                {
                    uint64_t val;
                    mcfloat_t dval;
                    mcvalue_t obj;
                    val = mc_callframe_readuint64(state->execstate.currframe);
                    dval = mc_util_uint64todouble(val);
                    obj = mc_value_makenumber(dval);
                    mc_vm_stackpush(state, obj);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_SETRECOVER):
                {
                    uint16_t recip;
                    recip = mc_callframe_readuint16(state->execstate.currframe);
                    state->execstate.currframe->recoverip = recip;
                }
                mc_vmmac_break();
        }
    onexecerror:
        if(state->errors.count > 0)
        {
            err = mc_errlist_getlast(&state->errors);
            if(err->errtype == MC_ERROR_RUNTIME && state->errors.count == 1)
            {
                recoverframeix = -1;
                for(fri = state->execstate.framestack->listcount - 1; fri >= 0; fri--)
                {
                    frame = mc_framelist_get(state->execstate.framestack, fri);
                    if(frame->recoverip >= 0 && !frame->isrecovering)
                    {
                        recoverframeix = fri;
                        break;
                    }
                }
                if((int)recoverframeix < 0)
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
                while(state->execstate.framestack->listcount > (recoverframeix + 1))
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
                state->execstate.currframe->bcposition = state->execstate.currframe->recoverip;
                state->execstate.currframe->isrecovering = true;
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





mcvalue_t mc_scriptfn_typeof(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcvalue_t arg;
    mcvaltype_t type;
    const char* ts;
    (void)data;
    (void)state;
    (void)thisval;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    arg = args[0];
    type = mc_value_gettype(arg);
    ts = mc_valtype_getname(type);
    return mc_value_makestring(state, ts);
}

mcvalue_t mc_scriptfn_arrayfirst(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcvalue_t arg;
    (void)state;
    (void)data;
    (void)thisval;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY))
    {
        return mc_value_makenull();
    }
    arg = args[0];
    return mc_value_arraygetvalue(arg, 0);
}

mcvalue_t mc_scriptfn_arraylast(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int len;
    mcvalue_t arg;
    (void)state;
    (void)argc;
    (void)thisval;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY))
    {
        return mc_value_makenull();
    }
    arg = args[0];
    len = mc_value_arraygetlength(arg);
    return mc_value_arraygetvalue(arg, len - 1);
}

mcvalue_t mc_scriptfn_arrayrest(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    bool ok;
    int i;
    int len;
    mcvalue_t arg;
    mcvalue_t res;
    mcvalue_t item;
    (void)state;
    (void)argc;
    (void)thisval;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY))
    {
        return mc_value_makenull();
    }
    arg = args[0];
    len = mc_value_arraygetlength(arg);
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
        item = mc_value_arraygetvalue(arg, i);
        ok = mc_value_arraypush(res, item);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

mcvalue_t mc_scriptfn_reverse(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
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
    (void)thisval;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY | MC_VAL_STRING))
    {
        return mc_value_makenull();
    }
    arg = args[0];
    type = mc_value_gettype(arg);
    if(type == MC_VAL_ARRAY)
    {
        inplen = mc_value_arraygetlength(arg);
        res = mc_value_makearraycapacity(state, inplen);
        if(mc_value_isnull(res))
        {
            return mc_value_makenull();
        }
        for(i = 0; i < inplen; i++)
        {
            obj = mc_value_arraygetvalue(arg, i);
            ok = mc_value_arraysetvalue(res, inplen - i - 1, obj);
            if(!ok)
            {
                return mc_value_makenull();
            }
        }
        return res;
    }
    if(type == MC_VAL_STRING)
    {
        inpstr = mc_value_getstringdata(arg);
        inplen = mc_value_getstringlength(arg);
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

mcvalue_t mc_scriptfn_makearray(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    bool ok;
    int i;
    int capacity;
    mcvalue_t res;
    mcvalue_t objnull;
    (void)state;
    (void)argc;
    (void)thisval;
    (void)data;
    if(argc == 1)
    {
        if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
        {
            return mc_value_makenull();
        }
        capacity = (int)mc_value_asnumber(args[0]);
        res = mc_value_makearraycapacity(state, capacity);
        if(mc_value_isnull(res))
        {
            return mc_value_makenull();
        }
        objnull = mc_value_makenull();
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
        if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER, MC_VAL_ANY))
        {
            return mc_value_makenull();
        }
        capacity = (int)mc_value_asnumber(args[0]);
        res = mc_value_makearraycapacity(state, capacity);
        if(mc_value_isnull(res))
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
    mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER);
    return mc_value_makenull();
}

mcvalue_t mc_scriptfn_externalfn(mcstate_t* state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args)
{
    int *test;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)args;
    test = (int*)data;
    *test = 42;
    return mc_value_makenull();
}

mcvalue_t mc_scriptfn_vec2add(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args)
{
    mcfloat_t a_x;
    mcfloat_t a_y;
    mcfloat_t b_x;
    mcfloat_t b_y;
    mcvalue_t res;
    mcvalue_t keyx;
    mcvalue_t keyy;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    if (!mc_argcheck_check(state, true, argc, args, MC_VAL_MAP, MC_VAL_MAP))
    {
        return mc_value_makenull();
    }
    keyx = mc_value_makestring(state, "x");
    keyy = mc_value_makestring(state, "y");
    a_x = mc_value_asnumber(mc_valmap_getvalue(args[0], keyx));
    a_y = mc_value_asnumber(mc_valmap_getvalue(args[0], keyy));
    b_x = mc_value_asnumber(mc_valmap_getvalue(args[1], keyx));
    b_y = mc_value_asnumber(mc_valmap_getvalue(args[1], keyy));
    res = mc_value_makemap(state);
    if (mc_value_gettype(res) == MC_VAL_NULL)
    {
        return res;
    }
    mc_valmap_setvalue(res, keyx, mc_value_makenumber(a_x + b_x));
    mc_valmap_setvalue(res, keyy, mc_value_makenumber(a_y + b_y));
    return res;
}

mcvalue_t mc_scriptfn_vec2sub(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args)
{
    mcfloat_t a_x;
    mcfloat_t a_y;
    mcfloat_t b_y;
    mcfloat_t b_x;
    mcvalue_t res;
    mcvalue_t keyx;
    mcvalue_t keyy;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    if (!mc_argcheck_check(state, true, argc, args, MC_VAL_MAP, MC_VAL_MAP))
    {
        return mc_value_makenull();
    }
    keyx = mc_value_makestring(state, "x");
    keyy = mc_value_makestring(state, "y");
    a_x = mc_value_asnumber(mc_valmap_getvalue(args[0], keyx));
    a_y = mc_value_asnumber(mc_valmap_getvalue(args[0], keyy));
    b_x = mc_value_asnumber(mc_valmap_getvalue(args[1], keyx));
    b_y = mc_value_asnumber(mc_valmap_getvalue(args[1], keyy));
    res = mc_value_makemap(state);
    mc_valmap_setvalue(res, keyx, mc_value_makenumber(a_x - b_x));
    mc_valmap_setvalue(res, keyy, mc_value_makenumber(a_y - b_y));
    return res;
}

mcvalue_t mc_scriptfn_testcheckargs(mcstate_t* state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args)
{
    (void)state;
    (void)args;
    (void)thisval;
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


mcvalue_t mc_scriptfn_maketestdict(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args)
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
    (void)thisval;
    if (argc != 1)
    {
        mc_state_setruntimeerrorf(state, "invalid type passed to maketestdict, got %d, expected 1", argc);
        return mc_value_makenull();
    }    
    if (mc_value_gettype(args[0]) != MC_VAL_NUMBER)
    {
        tname = mc_util_objtypename(mc_value_gettype(args[0]));
        mc_state_setruntimeerrorf(state, "invalid type passed to maketestdict, got %s", tname);
        return mc_value_makenull();
    }
    numitems = mc_value_asnumber(args[0]);
    res = mc_value_makemap(state);
    if (mc_value_gettype(res) == MC_VAL_NULL)
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

mcvalue_t mc_scriptfn_squarearray(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args)
{
    size_t i;
    mcfloat_t num;
    mcvalue_t res;
    mcvalue_t resitem;    
    (void)data;
    (void)thisval;
    res = mc_value_makearraycapacity(state, argc);
    for(i = 0; i < argc; i++)
    {
        if(mc_value_gettype(args[i]) != MC_VAL_NUMBER)
        {
            mc_state_setruntimeerrorf(state, "invalid type passed to squarearray");
            return mc_value_makenull();
        }
        num = mc_value_asnumber(args[i]);
        resitem = mc_value_makenumber(num * num);
        mc_value_arraypush(res, resitem);
    }
    return res;
}

mcvalue_t mc_scriptfn_print(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    size_t i;
    mcvalue_t arg;
    mcprinter_t* pr;
    (void)data;
    (void)thisval;
    (void)thisval;
    pr = state->stdoutprinter;
    for(i = 0; i < argc; i++)
    {
        arg = args[i];
        mc_printer_printvalue(pr, arg, false);
    }
    return mc_value_makenull();
}

mcvalue_t mc_scriptfn_println(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcvalue_t o;
    (void)thisval;
    o = mc_scriptfn_print(state, data, thisval, argc, args);
    mc_printer_putchar(state->stdoutprinter, '\n');
    return o;
}

mcvalue_t mc_nsfnfile_writefile(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int slen;
    int printedsz;
    const char* path;
    const char* string;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_STRING, MC_VAL_STRING))
    {
        return mc_value_makenull();
    }
    path = mc_value_getstringdata(args[0]);
    string = mc_value_getstringdata(args[1]);
    slen = mc_value_getstringlength(args[1]);
    printedsz = mc_fsutil_filewrite(state, path, string, slen);
    return mc_value_makenumber(printedsz);
}

mcvalue_t mc_nsfnfile_readfile(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    size_t flen;
    char* contents;
    const char* path;
    mcvalue_t res;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_STRING))
    {
        return mc_value_makenull();
    }
    path = mc_value_getstringdata(args[0]);
    contents = mc_fsutil_fileread(state, path, &flen);
    if(!contents)
    {
        return mc_value_makenull();
    }
    res = mc_value_makestringlen(state, contents, flen);
    mc_memory_free(contents);
    contents = NULL;
    return res;
}

mcvalue_t mc_scriptfn_tostring(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int reslen;
    const char* resstr;
    mcvalue_t arg;
    mcvalue_t res;
    mcprinter_t pr;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    arg = args[0];
    mc_printer_init(&pr, state, NULL, true);
    mc_printer_printvalue(&pr, arg, false);
    if(pr.failed)
    {
        mc_printer_release(&pr, true);
        return mc_value_makenull();
    }
    resstr = mc_printer_getstring(&pr);
    reslen = mc_printer_getlength(&pr);
    res = mc_value_makestringlen(state, resstr, reslen);
    mc_printer_release(&pr, false);
    return res;
}

mcvalue_t mc_nsfnjson_stringify(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int reslen;
    const char* resstr;
    mcvalue_t arg;
    mcvalue_t res;
    mcprinter_t pr;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    arg = args[0];
    mc_printer_init(&pr, state, NULL, true);
    pr.config.verbosefunc = false;
    pr.config.quotstring = true;
    mc_printer_printvalue(&pr, arg, false);
    if(pr.failed)
    {
        mc_printer_release(&pr, true);
        return mc_value_makenull();
    }
    resstr = mc_printer_getstring(&pr);
    reslen = mc_printer_getlength(&pr);
    res = mc_value_makestringlen(state, resstr, reslen);
    mc_printer_release(&pr, true);
    return res;
}


mcvalue_t mc_objfnnumber_chr(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcfloat_t val;
    char c;
    (void)state;
    (void)argc;
    (void)data;
    (void)args;
    val = mc_value_asnumber(thisval);
    c = (char)val;
    return mc_value_makestringlen(state, &c, 1);
}
 
mcvalue_t mc_objfnstring_length(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    size_t len;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    len = mc_value_getstringlength(thisval);
    return mc_value_makenumber(len);
}

/**
 * \brief Searches a string an instance of another string in it and returns the index of the first occurance.  If no occurance is found a -1 is returned.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return The index of the found string or -1 if it's not found.
 */
mcvalue_t mc_objfnstring_indexof(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int inplen;
    int searchlen;
    int startindex;
    char tmpch;
    char* result;
    const char* inpstr;
    const char* searchstr;
    mcvalue_t searchval;
    mcvaltype_t searchtype;
    (void)state;
    (void)data;
    (void)inplen;
    (void)searchlen;
    (void)argc;
    startindex = 0;
    searchval = args[0];
    searchtype = mc_value_gettype(searchval);
    if(searchtype == MC_VAL_NULL)
    {
        return mc_value_makenumber(-1);
    }
    searchstr = NULL;
    searchlen = 0;
    inpstr = mc_value_getstringdata(thisval);
    inplen = mc_value_getstringlength(thisval);
    MC_ASSERT((searchtype == MC_VAL_NUMBER) || (searchtype == MC_VAL_STRING));
    if(searchtype == MC_VAL_NUMBER)
    {
        tmpch = mc_value_asnumber(searchval);
        inpstr = &tmpch;
        inplen = 1;
    }
    else if(searchtype == MC_VAL_STRING)
    {
        searchstr = mc_value_getstringdata(searchval);
        searchlen = mc_value_getstringlength(searchval);
    }

    result = (char*)strstr(inpstr + startindex, searchstr);
    if(result == NULL)
    {
        return mc_value_makenumber(-1);
    }
    return mc_value_makenumber(result - inpstr);
}

mcvalue_t mc_objfnstring_charcodefirst(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    char ch;
    size_t len;
    const char* str;
    mcvalue_t sval;
    (void)state;
    (void)data;
    (void)thisval;
    (void)argc;
    (void)args;
    sval = thisval;
    str = mc_value_getstringdata(sval);
    len = mc_value_getstringlength(sval);
    ch = 0;
    if(len > 0)
    {
        ch = str[0];
    }
    return mc_value_makenumber(ch);
}


mcvalue_t mc_objfnstring_charcodeat(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    char ch;
    long idx;
    size_t len;
    const char* str;
    mcvalue_t sval;
    (void)state;
    (void)data;
    (void)thisval;
    (void)argc;
    (void)args;
    sval = thisval;
    str = mc_value_getstringdata(sval);
    len = mc_value_getstringlength(sval);
    idx = mc_value_asnumber(args[0]);
    if(idx >= (long)len)
    {
        return mc_value_makenull();
    }
    ch = str[idx];
    return mc_value_makenumber(ch);
}


mcvalue_t mc_objfnstring_charat(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    char ch;
    long idx;
    size_t len;
    const char* str;
    mcvalue_t sval;
    (void)state;
    (void)data;
    (void)thisval;
    (void)argc;
    (void)args;
    sval = thisval;
    str = mc_value_getstringdata(sval);
    len = mc_value_getstringlength(sval);
    idx = mc_value_asnumber(args[0]);
    if(idx >= (long)len)
    {
        return mc_value_makenull();
    }
    ch = str[idx];
    return mc_value_makestringlen(state, &ch, 1);
}

mcvalue_t mc_objfnstring_getself(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    const char* str;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    str = mc_value_getstringdata(thisval);
    fprintf(stderr, "objfnstring_getself: str=\"%s\"\n", str);
    return thisval;
}

mcvalue_t mc_objfnstring_tonumber(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int stringlen;
    int parsedlen;
    mcfloat_t result;
    char* end;
    const char* string;
    (void)state;
    (void)argc;
    (void)data;
    (void)args;
    result = 0;
    string = "";
    stringlen = mc_value_getstringlength(thisval);
    string = mc_value_getstringdata(thisval);
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
    return mc_value_makenumber(result);
err:
    mc_state_pusherrorf(state, MC_ERROR_RUNTIME, srcposinvalid, "cannot convert \"%s\" to number", string);
    return mc_value_makenull();
}

/**
 * \brief Returns the specified number of characters from the left hand side of the string.  If more characters exist than the length of the string the entire string is returned.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return The section of the string from the left-hand side.
 */
mcvalue_t mc_objfnstring_left(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int inplen;
    int startpos;
    char* result;
    const char* inpstr;
    mcvalue_t obj;
    mcvalue_t inpval;
    mcvalue_t posval;
    (void)data;
    if(argc > 0 && mc_value_gettype(args[0]) == MC_VAL_NUMBER)
    {
        inpval = thisval;
        posval = args[0];
        inpstr = mc_value_getstringdata(inpval);
        inplen = mc_value_getstringlength(inpval);
        startpos = mc_value_asnumber(posval);
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
mcvalue_t mc_objfnstring_right(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
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
    (void)thisval;
    if(argc > 0 && mc_value_gettype(args[1]) == MC_VAL_NUMBER)
    {
        inpval = thisval;
        idxval = args[0];
        inpstr = mc_value_getstringdata(inpval);
        inplen = mc_value_getstringlength(inpval);
        startpos = mc_value_asnumber(idxval);
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
mcvalue_t mc_objfnstring_replaceall(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
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
    (void)argc;
    if(mc_value_gettype(args[0]) == MC_VAL_STRING && mc_value_gettype(args[1]) == MC_VAL_STRING)
    {
        inpval = thisval;
        searchval = args[0];
        repval = args[1];
        inpstr = mc_value_getstringdata(inpval);
        searchstr = mc_value_getstringdata(searchval);
        replacestr = mc_value_getstringdata(repval);
        inplen = mc_value_getstringlength(inpval);
        searchlen = mc_value_getstringlength(searchval);
        replacelen = mc_value_getstringlength(repval);
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
mcvalue_t mc_objfnstring_replacefirst(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
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
    (void)argc;
    if(mc_value_gettype(args[0]) == MC_VAL_STRING && mc_value_gettype(args[1]) == MC_VAL_STRING)
    {
        inpval = thisval;
        searchval = args[0];
        repval = args[1];
        inpstr = mc_value_getstringdata(inpval);
        searchstr = mc_value_getstringdata(searchval);
        replacestr = mc_value_getstringdata(repval);
        inplen = mc_value_getstringlength(inpval);
        searchlen = mc_value_getstringlength(searchval);
        replacelen = mc_value_getstringlength(repval);
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
mcvalue_t mc_objfnstring_trim(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
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
    (void)argc;
    (void)args;
    inpval = thisval;
    inpstr = mc_value_getstringdata(inpval);
    inplen = mc_value_getstringlength(inpval);
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

mcvalue_t mc_objfnarray_length(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    size_t len;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    len = mc_value_arraygetlength(thisval);
    return mc_value_makenumber(len);
}

/*

    size_t vsposition;
    mcvmframe_t* currframe;
    mcvallist_t* valuestack;
    mcvallist_t* valthisstack;
    mcvallist_t* nativethisstack;
    size_t thisstpos;
    mcframelist_t* framestack;
    mcvalue_t lastpopped;

*/
void mc_vm_savestate(mcstate_t* state, mcexecstate_t* est)
{
    est->thisstpos = state->execstate.thisstpos;
    est->vsposition = state->execstate.vsposition;
    est->currframe = state->execstate.currframe;
}

void mc_vm_restorestate(mcstate_t* state, mcexecstate_t* est)
{
    state->execstate.thisstpos = est->thisstpos;
    state->execstate.vsposition = est->vsposition;
    state->execstate.currframe = est->currframe;
}

/*
struct mcvmframe_t
{
    mcvalue_t function;
    int64_t bcposition;
    int64_t basepointer;
    mcastlocation_t* framesrcposlist;
    uint16_t* bytecode;
    int64_t sourcebcpos;
    int64_t bcsize;
    int64_t recoverip;
    bool isrecovering;

*/

mcvalue_t mc_vm_callvalue(mcstate_t* state, mcvallist_t* constants, mcvalue_t callee, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    bool ok;
    size_t i;
    size_t oldsp;
    size_t oldthissp;
    size_t oldframescount;
    mcvmframe_t tempframe;
    mcvalue_t retv;
    mcexecstate_t est;
    mcvaltype_t type;
    (void)oldsp;
    (void)oldframescount;
    type = mc_value_gettype(callee);
    if(type == MC_VAL_FUNCSCRIPT)
    {
        mc_callframe_init(&tempframe, callee, 0);
        //mc_callframe_init(&tempframe, callee, state->execstate.vsposition - 1);
        mc_vm_savestate(state, &est);
        oldsp = state->execstate.vsposition;
        oldthissp = state->execstate.thisstpos;
        mc_vm_pushframe(state, tempframe);
        oldframescount = state->execstate.framestack->listcount;
        mc_vm_stackpush(state, callee);
        for(i = 0; i < argc; i++)
        {
            mc_vm_stackpush(state, args[i]);
        }
        ok = mc_function_execfunction(state, callee, constants, true);
        if(!ok)
        {
            mc_vm_restorestate(state, &est);
            return mc_value_makenull();
        }
        #if 0
        while(state->execstate.framestack->listcount > oldframescount)
        {
            mc_vm_popframe(state);
        }
        #endif
        //MC_ASSERTF(state->execstate.vsposition == oldsp, "oldsp=%ld state->execstate.vsposition=%ld", oldsp, state->execstate.vsposition);
        state->execstate.thisstpos = oldthissp;
        retv = mc_vm_getlastpopped(state);
        mc_vm_restorestate(state, &est);
        return retv;
    }
    if(type == MC_VAL_FUNCNATIVE)
    {
        return mc_vm_callnativefunction(state, callee, srcposinvalid, thisval, argc, args);
    }
    mc_errlist_addf(&state->errors, MC_ERROR_USER, srcposinvalid, "object is not callable");
    return mc_value_makenull();
}

mcvalue_t mc_objfnarray_map(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    size_t i;
    size_t len;
    mcvalue_t val;
    mcvalue_t res;
    mcvalue_t callee;
    mcvalue_t pseudothis;
    mcvalue_t vargs[3];
    mcvalue_t narr;
    mcvallist_t* ary;
    mcvallist_t* nary;
    mcprinter_t* pr;
    (void)state;
    (void)data;
    (void)argc;
    (void)pr;
    pseudothis = mc_value_makenull();
    pr = state->stderrprinter;
    //callee = mc_value_copyflat(state, args[0]);
    callee = args[0];
    narr = mc_value_makearray(state);
    nary = mc_value_arraygetactualarray(narr);
    ary = mc_value_arraygetactualarray(thisval);
    len = ary->listcount;
    for(i=0; i<len; i++)
    {
        val = ary->listitems[i];
        vargs[0] = val;
        vargs[1] = mc_value_makenull();
        res = mc_vm_callvalue(state, mc_compiler_getconstants(state->compiler), callee, thisval, 1, vargs);        
        #if 0
        {
            mc_printer_printf(pr, "map(): value at %ld =<<<", i);
            mc_printer_printvalue(pr, res, true);
            mc_printer_printf(pr, ">>> (before: <<<");
            mc_printer_printvalue(pr, val, true);
            mc_printer_printf(pr,">>>)\n");
        }
        #endif
        mc_vallist_push(nary, res);
    }
    return narr;
}


mcvalue_t mc_objfnarray_push(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    bool ok;
    size_t i;
    size_t len;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    for(i=0; i<argc; i++)
    {
        ok = mc_value_arraypush(thisval, args[i]);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    len = mc_value_arraygetlength(thisval);
    return mc_value_makenumber(len);
}

mcvalue_t mc_objfnarray_pop(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcvalue_t val;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    val = mc_valarray_pop(thisval);
    return val;
}

mcvalue_t mc_objfnarray_join(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
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
    (void)thisval;
    (void)argc;
    (void)data;
    havejoinee = false;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    array= thisval;
    if(argc > 0)
    {
        havejoinee = true;
        joinee = args[0];
    }
    alen = mc_value_arraygetlength(array);
    mc_printer_init(&pr, state, NULL, true);
    for(i=0; i<alen; i++)
    {
        item = mc_value_arraygetvalue(array, i);
        mc_printer_printvalue(&pr, item, false);
        if(havejoinee)
        {
            if((i + 1) != alen)
            {
                mc_printer_printvalue(&pr, joinee, false);
            }
        }
    }
    str = pr.destbuf->data;
    slen = pr.destbuf->length;
    rt = mc_value_makestringlen(state, str, slen);
    mc_printer_release(&pr, true);
    return rt;
}

mcvalue_t mc_objfnmap_length(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    size_t len;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    len = mc_valmap_getlength(thisval);
    return mc_value_makenumber(len);
}

mcvalue_t mc_objfnutil_istype(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args, mcvaltype_t vt)
{
    (void)data;
    (void)state;
    (void)argc;
    (void)args;
    return mc_value_makebool(mc_value_gettype(thisval) == vt);
}

mcvalue_t mc_objfnobject_iscallable(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    (void)args;
    return mc_value_makebool(mc_value_iscallable(thisval));
}

mcvalue_t mc_objfnobject_isstring(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, MC_VAL_STRING);
}

mcvalue_t mc_objfnobject_isarray(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, MC_VAL_ARRAY);
}

mcvalue_t mc_objfnobject_ismap(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, MC_VAL_MAP);
}

mcvalue_t mc_objfnobject_isnumber(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, MC_VAL_NUMBER);
}

mcvalue_t mc_objfnobject_isbool(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, MC_VAL_BOOL);
}

mcvalue_t mc_objfnobject_isnull(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, MC_VAL_NULL);
}

mcvalue_t mc_objfnobject_isfuncscript(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, MC_VAL_FUNCSCRIPT);
}

mcvalue_t mc_objfnobject_isexternal(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, MC_VAL_EXTERNAL);
}

mcvalue_t mc_objfnobject_iserror(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, MC_VAL_ERROR);
}

mcvalue_t mc_objfnobject_isfuncnative(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, MC_VAL_FUNCNATIVE);
}

void mc_state_makestdclasses(mcstate_t* state)
{
    {
        state->stdobjobject = mc_class_make(state, "Object", true);
        mc_class_addmember(state, state->stdobjobject, "isString", mc_objfnobject_isstring);
        mc_class_addmember(state, state->stdobjobject, "isNumber", mc_objfnobject_isnumber);
        mc_class_addmember(state, state->stdobjobject, "isArray", mc_objfnobject_isarray);
        mc_class_addmember(state, state->stdobjobject, "isMap", mc_objfnobject_ismap);
        mc_class_addmember(state, state->stdobjobject, "isFuncNative", mc_objfnobject_isfuncnative);
        mc_class_addmember(state, state->stdobjobject, "isFuncScript", mc_objfnobject_isfuncscript);
        mc_class_addmember(state, state->stdobjobject, "isExternal", mc_objfnobject_isexternal);
        mc_class_addmember(state, state->stdobjobject, "isError", mc_objfnobject_iserror);
        mc_class_addmember(state, state->stdobjobject, "isNull", mc_objfnobject_isnull);
        mc_class_addmember(state, state->stdobjobject, "isBool", mc_objfnobject_isbool);
        mc_class_addmember(state, state->stdobjobject, "isCallable", mc_objfnobject_iscallable);

    }
    {
        state->stdobjnumber = mc_class_make(state, "Number", false);
        mc_class_addmember(state, state->stdobjnumber, "chr", mc_objfnnumber_chr);
        
    }
    {
        state->stdobjstring = mc_class_make(state, "String", false);
        mc_class_addpseudo(state, state->stdobjstring, "length", mc_objfnstring_length);
        mc_class_addmember(state, state->stdobjstring, "getself", mc_objfnstring_getself);
        mc_class_addmember(state, state->stdobjstring, "toNumber", mc_objfnstring_tonumber);
        mc_class_addmember(state, state->stdobjstring, "ord", mc_objfnstring_charcodefirst);
        mc_class_addmember(state, state->stdobjstring, "charCodeAt", mc_objfnstring_charcodeat);
        mc_class_addmember(state, state->stdobjstring, "charAt", mc_objfnstring_charat);
        mc_class_addmember(state, state->stdobjstring, "indexOf", mc_objfnstring_indexof);
        mc_class_addmember(state, state->stdobjstring, "left", mc_objfnstring_left);
        mc_class_addmember(state, state->stdobjstring, "right", mc_objfnstring_right);
        mc_class_addmember(state, state->stdobjstring, "replace", mc_objfnstring_replaceall);
        mc_class_addmember(state, state->stdobjstring, "replacefirst", mc_objfnstring_replacefirst);
        mc_class_addmember(state, state->stdobjstring, "trim", mc_objfnstring_trim);


    }
    {
        state->stdobjarray = mc_class_make(state, "Array", false);
        mc_class_addpseudo(state, state->stdobjarray, "length", mc_objfnarray_length);
        mc_class_addmember(state, state->stdobjarray, "push", mc_objfnarray_push);
        mc_class_addmember(state, state->stdobjarray, "pop", mc_objfnarray_pop);
        mc_class_addmember(state, state->stdobjarray, "join", mc_objfnarray_join);
        mc_class_addmember(state, state->stdobjarray, "map", mc_objfnarray_map);
    }
    {
        state->stdobjmap = mc_class_make(state, "Map", false);
        mc_class_addpseudo(state, state->stdobjmap, "length", mc_objfnmap_length);
    }
    {
        state->stdobjfunction = mc_class_make(state, "Function", false);
    }
}

mcvalue_t mc_scriptfn_isnan(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcfloat_t val;
    bool b;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    val = mc_value_asnumber(args[0]);
    b = false;
    if(val != val)
    {
        b = true;
    }
    return mc_value_makebool(b);
}

mcvalue_t mc_scriptfn_range(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    bool ok;
    size_t ai;
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
    (void)thisval;
    for(ai = 0; ai < argc; ai++)
    {
        type = mc_value_gettype(args[ai]);
        if(type != MC_VAL_NUMBER)
        {
            typestr = mc_valtype_getname(type);
            expectedstr = mc_valtype_getname(MC_VAL_NUMBER);
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, srcposinvalid, "invalid argument %d passed to range, got %s instead of %s", ai, typestr, expectedstr);
            return mc_value_makenull();
        }
    }
    start = 0;
    end = 0;
    step = 1;
    if(argc == 1)
    {
        end = (int)mc_value_asnumber(args[0]);
    }
    else if(argc == 2)
    {
        start = (int)mc_value_asnumber(args[0]);
        end = (int)mc_value_asnumber(args[1]);
    }
    else if(argc == 3)
    {
        start = (int)mc_value_asnumber(args[0]);
        end = (int)mc_value_asnumber(args[1]);
        step = (int)mc_value_asnumber(args[2]);
    }
    else
    {
        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, srcposinvalid, "invalid number of arguments passed to range, got %d", argc);
        return mc_value_makenull();
    }
    if(step == 0)
    {
        mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "range step cannot be 0");
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
        ok = mc_value_arraypush(res, item);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

mcvalue_t mc_scriptfn_keys(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
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
    (void)thisval;
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
        ok = mc_value_arraypush(res, key);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

mcvalue_t mc_scriptfn_values(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
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
    (void)thisval;
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
        ok = mc_value_arraypush(res, key);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

mcvalue_t mc_scriptfn_copy(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_copyflat(state, args[0]);
}

mcvalue_t mc_scriptfn_copydeep(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    return mc_value_copydeep(state, args[0]);
}

mcvalue_t mc_scriptfn_remove(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    bool res;
    int i;
    int ix;
    mcvalue_t obj;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY, MC_VAL_ANY))
    {
        return mc_value_makenull();
    }
    ix = -1;
    for(i = 0; i < mc_value_arraygetlength(args[0]); i++)
    {
        obj = mc_value_arraygetvalue(args[0], i);
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
    res = mc_value_arrayremoveat(args[0], ix);
    return mc_value_makebool(res);
}

mcvalue_t mc_scriptfn_removeat(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    bool res;
    int ix;
    mcvaltype_t type;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_ARRAY, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    type= mc_value_gettype(args[0]);
    ix = (int)mc_value_asnumber(args[1]);
    switch(type)
    {
        case MC_VAL_ARRAY:
            {
                res = mc_value_arrayremoveat(args[0], ix);
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

mcvalue_t mc_scriptfn_error(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    (void)data;
    (void)thisval;
    if(argc == 1 && mc_value_gettype(args[0]) == MC_VAL_STRING)
    {
        return mc_value_makeerror(state, mc_value_getstringdata(args[0]));
    }
    return mc_value_makeerror(state, "");
}

mcvalue_t mc_scriptfn_crash(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    (void)data;
    (void)thisval;
    if(argc == 1 && mc_value_gettype(args[0]) == MC_VAL_STRING)
    {
        mc_errlist_pushmessage(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), mc_value_getstringdata(args[0]));
    }
    else
    {
        mc_errlist_pushmessage(&state->errors, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "");
    }
    return mc_value_makenull();
}

mcvalue_t mc_scriptfn_assert(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_BOOL))
    {
        return mc_value_makenull();
    }
    if(!mc_value_asbool(args[0]))
    {
        mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "assertion failed");
        return mc_value_makenull();
    }
    return mc_value_makebool(true);
}

mcvalue_t mc_scriptfn_randseed(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int seed;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    seed = (int)mc_value_asnumber(args[0]);
    srand(seed);
    return mc_value_makebool(true);
}

mcvalue_t mc_scriptfn_random(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcfloat_t min;
    mcfloat_t max;
    mcfloat_t res;
    mcfloat_t range;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
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
        min = mc_value_asnumber(args[0]);
        max = mc_value_asnumber(args[1]);
        if(min >= max)
        {
            mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "max is bigger than min");
            return mc_value_makenull();
        }
        range = max - min;
        res = min + (res * range);
        return mc_value_makenumber(res);
    }
    mc_errlist_addf(&state->errors, MC_ERROR_RUNTIME, srcposinvalid, "invalid number or arguments");
    return mc_value_makenull();
}

mcvalue_t mc_scriptutil_slicearray(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int i;
    int len;
    int index;
    bool ok;
    mcvalue_t res;
    mcvalue_t item;
    (void)data;
    (void)argc;
    (void)thisval;
    index = (int)mc_value_asnumber(args[1]);
    len = mc_value_arraygetlength(args[0]);
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
        item = mc_value_arraygetvalue(args[0], i);
        ok = mc_value_arraypush(res, item);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return res;
}

mcvalue_t mc_scriptutil_slicestring(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int i;
    int len;
    int index;
    char c;
    mcvalue_t res;
    const char* str;
    (void)data;
    (void)argc;
    (void)thisval;
    index = (int)mc_value_asnumber(args[1]);
    str = mc_value_getstringdata(args[0]);
    len = mc_value_getstringlength(args[0]);
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
    res = mc_value_makestrcapacity(state, 10);
    if(mc_value_isnull(res))
    {
        return mc_value_makenull();
    }
    for(i = index; i < len; i++)
    {
        c = str[i];
        mc_valstring_appendlen(res, &c, 1);
    }
    return res;
}

mcvalue_t mc_scriptfn_slice(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    const char* typestr;
    mcvaltype_t argtype;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_STRING | MC_VAL_ARRAY, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    argtype = mc_value_gettype(args[0]);
    if(argtype == MC_VAL_ARRAY)
    {
        return mc_scriptutil_slicearray(state, data, thisval, argc, args);
    }
    if(argtype == MC_VAL_STRING)
    {
        return mc_scriptutil_slicestring(state, data, thisval, argc, args);
    }
    typestr = mc_valtype_getname(argtype);
    mc_state_pusherrorf(state, MC_ERROR_RUNTIME, srcposinvalid, "invalid argument 0 passed to slice, got %s instead", typestr);
    return mc_value_makenull();
}




mcvalue_t mc_nsfnmath_sqrt(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_asnumber(args[0]);
    res = sqrt(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_nsfnmath_pow(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcfloat_t arg1;
    mcfloat_t arg2;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg1 = mc_value_asnumber(args[0]);
    arg2 = mc_value_asnumber(args[1]);
    res = pow(arg1, arg2);
    return mc_value_makenumber(res);
}

mcvalue_t mc_nsfnmath_sin(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_asnumber(args[0]);
    res = sin(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_nsfnmath_cos(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_asnumber(args[0]);
    res = cos(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_nsfnmath_tan(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_asnumber(args[0]);
    res = tan(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_nsfnmath_log(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_asnumber(args[0]);
    res = log(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_nsfnmath_ceil(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_asnumber(args[0]);
    res = ceil(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_nsfnmath_floor(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_asnumber(args[0]);
    res = floor(arg);
    return mc_value_makenumber(res);
}

mcvalue_t mc_nsfnmath_abs(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_NUMBER))
    {
        return mc_value_makenull();
    }
    arg = mc_value_asnumber(args[0]);
    res = MC_UTIL_FABS(arg);
    return mc_value_makenumber(res);
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
        { "typeof", mc_scriptfn_typeof },        
        { "println", mc_scriptfn_println },
        { "print", mc_scriptfn_print },
        { "arrayfirst", mc_scriptfn_arrayfirst },
        { "arraylast", mc_scriptfn_arraylast },
        { "arrayrest", mc_scriptfn_arrayrest },
        { "remove", mc_scriptfn_remove },
        { "removeat", mc_scriptfn_removeat },
        { "tostring", mc_scriptfn_tostring },
        { "isNaN", mc_scriptfn_isnan },
        { "range", mc_scriptfn_range },
        { "keys", mc_scriptfn_keys },
        { "values", mc_scriptfn_values },
        { "copy", mc_scriptfn_copy },
        { "deepcopy", mc_scriptfn_copydeep },
        { "reverse", mc_scriptfn_reverse },
        { "array", mc_scriptfn_makearray },
        { "error", mc_scriptfn_error },
        { "crash", mc_scriptfn_crash },
        { "assert", mc_scriptfn_assert },
        { "randomseed", mc_scriptfn_randseed },
        { "random", mc_scriptfn_random },
        { "slice", mc_scriptfn_slice },

        {NULL, NULL}
    };
    int i;
    for(i=0; nativefunctions[i].name != NULL; i++)
    {
        mc_state_setnativefunction(state, nativefunctions[i].name, nativefunctions[i].fn, NULL);
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
    mc_valmap_setvalstring(jmap, "stringify", mc_value_makefuncnative(state, "stringify", mc_nsfnjson_stringify, NULL));
    mc_state_setglobalconstant(state, "JSON", jmap);
}

void mc_cli_installjsconsole(mcstate_t* state)
{
    mcvalue_t jmap;
    jmap = mc_value_makemap(state);
    mc_valmap_setvalstring(jmap, "log", mc_value_makefuncnative(state, "log", mc_scriptfn_println, NULL));
    mc_state_setglobalconstant(state, "console", jmap);
}

void mc_cli_installmath(mcstate_t* state)
{
    mcvalue_t jmap;
    jmap = mc_value_makemap(state);
    mc_valmap_setvalstring(jmap, "sqrt", mc_value_makefuncnative(state, "sqrt", mc_nsfnmath_sqrt, NULL));
    mc_valmap_setvalstring(jmap, "pow", mc_value_makefuncnative(state, "pow", mc_nsfnmath_pow, NULL));
    mc_valmap_setvalstring(jmap, "sin", mc_value_makefuncnative(state, "sin", mc_nsfnmath_sin, NULL));
    mc_valmap_setvalstring(jmap, "cos", mc_value_makefuncnative(state, "cos", mc_nsfnmath_cos, NULL));
    mc_valmap_setvalstring(jmap, "tan", mc_value_makefuncnative(state, "tan", mc_nsfnmath_tan, NULL));
    mc_valmap_setvalstring(jmap, "log", mc_value_makefuncnative(state, "log", mc_nsfnmath_log, NULL));
    mc_valmap_setvalstring(jmap, "ceil", mc_value_makefuncnative(state, "ceil", mc_nsfnmath_ceil, NULL));
    mc_valmap_setvalstring(jmap, "floor", mc_value_makefuncnative(state, "floor", mc_nsfnmath_floor, NULL));
    mc_valmap_setvalstring(jmap, "abs", mc_value_makefuncnative(state, "abs", mc_nsfnmath_abs, NULL));
    mc_state_setglobalconstant(state, "Math", jmap);
}

void mc_cli_installfauxjavascript(mcstate_t* state)
{
    mc_cli_installjsondummy(state);
    mc_cli_installjsconsole(state);
    mc_cli_installmath(state);
}

void mc_cli_installfileio(mcstate_t* state)
{
    mcvalue_t map;
    map = mc_value_makemap(state);
    mc_valmap_setvalstring(map, "read", mc_value_makefuncnative(state, "read", mc_nsfnfile_readfile, NULL));
    mc_valmap_setvalstring(map, "write", mc_value_makefuncnative(state, "write", mc_nsfnfile_writefile, NULL));
    mc_valmap_setvalstring(map, "put", mc_value_makefuncnative(state, "put", mc_nsfnfile_writefile, NULL));
    mc_state_setglobalconstant(state, "File", map);
}

static int g_extfnvar;

void mc_cli_installotherstuff(mcstate_t* state)
{
    mc_state_setglobalconstant(state, "test", mc_value_makenumber(42));
    mc_state_setnativefunction(state, "external_fn_test", mc_scriptfn_externalfn, &g_extfnvar);
    mc_state_setnativefunction(state, "test_check_args", mc_scriptfn_testcheckargs, NULL);
    mc_state_setnativefunction(state, "vec2_add", mc_scriptfn_vec2add, NULL);
    mc_state_setnativefunction(state, "vec2_sub", mc_scriptfn_vec2sub, NULL);
    mc_cli_installfileio(state);
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

bool mc_cli_compileandrunsource(mcstate_t* state, mcvalue_t* vdest, const char* source)
{
    bool ok;
    mcvalue_t tmp;
    mccompiledprogram_t* program;
    ok = false;
    program = mc_state_compilesource(state, source);
    if(state->config.exitaftercompiling)
    {
        mc_program_destroy(program);
        return true;
    }
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
        mc_value_arraypush(argvobj, strval);
    }
    mc_state_setglobalconstant(state, "ARGV", argvobj);
}

#define printtypesize(typ) \
    mc_cli_printtypesize(#typ, sizeof(typ))

void mc_cli_printtypesize(const char* name, size_t sz)
{
    printf("%ld\t%s\n", sz, name);
}

void mc_cli_printtypesizes()
{
    printtypesize(mcptrdict_t);
    printtypesize(mcvaldict_t);
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
    printtypesize(mcastexprcodeblock_t);
    printtypesize(mcastexprliteralmap_t);
    printtypesize(mcastexprliteralarray_t);
    printtypesize(mcastexprliteralstring_t);
    printtypesize(mcastexprprefix_t);
    printtypesize(mcastexprinfix_t);
    printtypesize(mcastexprifcase_t);
    printtypesize(mcastexprliteralfunction_t);
    printtypesize(mcastexprcall_t);
    printtypesize(mcastexprindex_t);
    printtypesize(mcastexprassign_t);
    printtypesize(mcastexprlogical_t);
    printtypesize(mcastexprternary_t);
    printtypesize(mcastexprident_t);
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
    printtypesize(mcmodule_t);
    printtypesize(mcstoddiyfpconv_t);
    printtypesize(mcvalunion_t);
    printtypesize(mcobjunion_t);
    printtypesize(mcexprunion_t);
    printtypesize(mcfuncfvunion_t);
    printtypesize(mcfuncnameunion_t);
    printtypesize(int);
    printtypesize(uint16_t);

}

static optlongflags_t longopts[] =
{
    {"help", 'h', OPTPARSE_NONE, "this help"},
    {"printsizes", 't', OPTPARSE_NONE, "print type sizes"},
    {"eval", 'e', OPTPARSE_REQUIRED, "evaluate a single line of code"},
    {"dumpast", 'a', OPTPARSE_NONE, "dump AST after parsing"},
    {"dumpbc", 'd', OPTPARSE_NONE, "dump bytecode after compiling"},
    {"exitcompile", 'x', OPTPARSE_NONE, "exit after compiling (for debugging)"},
    {"printins", 'p', OPTPARSE_NONE, "print each instruction as it is being executed"},
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
        else if(co == 'x')
        {
            state->config.exitaftercompiling = true;
        }
        else if(co == 'p')
        {
            state->config.printinstructions = true;
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

