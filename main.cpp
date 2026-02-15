
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
#include <new>
#if defined(__unix__)
    #include <unistd.h>
    #include <sys/time.h>
#endif

#include "mem.h"
#include "optparse.h"
#include "strbuf.h"
#include "utilos.h"
#include "fnmatch.h"

#include "stod.h"


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
#define MC_CONF_MINVMVALSTACKSIZE (4)
#define MC_CONF_MINVMFRAMES (4)

#define MC_CONF_MAXVMGLOBALS (4)
#define MC_CONF_MINNATIVETHISSTACKSIZE (1024)
#define MC_CONF_MINVMTHISSTACKSIZE (MC_CONF_MINVMVALSTACKSIZE)
#define MC_CONF_GCMEMPOOLSIZE (2048/16)
#define MC_CONF_GCMEMPOOLCOUNT (3)
#define MC_CONF_GCMEMSWEEPINTERVAL (128)
#define MC_CONF_MAXERRORCOUNT (4)
#define MC_CONF_MAXERRORMSGLENGTH (64)
#define MC_CONF_GENERICDICTINVALIDIX (UINT_MAX)
#define MC_CONF_VALDICTINVALIDIX (UINT_MAX)
#define MC_CONF_GENERICDICTINITSIZE (32)

#define MC_CONF_MAXOPEROVERLOADS (25)

#if !defined(__GNUC__)
    #define __attribute__(x)
#endif


#if (defined(__GNUC__) || defined(__clang__))
    #define MC_INLINE static inline
    #define MC_FORCEINLINE MC_INLINE __attribute__((always_inline))
#else
    #define MC_INLINE static
    #define MC_FORCEINLINE MC_INLINE
#endif


#define MC_UTIL_STATICARRAYSIZE(array) ((int)(sizeof(array) / sizeof(array[0])))
#define MC_UTIL_FABS(n) fabs(n)

#if 0
    #define MC_UTIL_CMPFLOAT(a, b) (MC_UTIL_FABS((a) - (b)) < DBL_EPSILON)
#else
    #define MC_UTIL_CMPFLOAT(a, b) ((a) == (b))
#endif

#define MC_ASSERT(x) mc_util_assert((x), #x, __FILE__, __LINE__, nullptr)

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
typedef struct mcprintconfig_t mcprintconfig_t;
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

typedef struct mcobjuserdata_t mcobjuserdata_t;
typedef struct mcobjerror_t mcobjerror_t;
typedef struct mcobjstring_t mcobjstring_t;
typedef struct mcobjmap_t mcobjmap_t;
typedef struct mcobjarray_t mcobjarray_t;


typedef struct mcopdefinition_t mcopdefinition_t;

typedef struct mcastscopeblock_t mcastscopeblock_t;
typedef struct mcastscopefile_t mcastscopefile_t;
typedef struct mcastscopecomp_t mcastscopecomp_t;

typedef struct mcgcobjdatapool_t mcgcobjdatapool_t;


typedef struct mcvmframe_t mcvmframe_t;
typedef struct mctraceitem_t mctraceitem_t;
typedef struct mcmodule_t mcmodule_t;

typedef union mcvalunion_t mcvalunion_t;
typedef union mcobjunion_t mcobjunion_t;
typedef union mcexprunion_t mcexprunion_t;
typedef struct /**/mcclass_t mcclass_t;
typedef struct /**/mcfield_t mcfield_t;
typedef struct mcconsolecolor_t mcconsolecolor_t;
typedef struct mcobjfunction_t mcobjfunction_t;

class PtrList;
class Printer;
class PtrDict;
class AstLexer;
class AstLexInfo;

typedef mcvalue_t (*mcnativefn_t)(mcstate_t*, void*, mcvalue_t, size_t, mcvalue_t*);
typedef size_t (*mcitemhashfn_t)(void*);
typedef bool (*mcitemcomparefn_t)(void*, void*);
typedef void (*mcitemdestroyfn_t)(void*);
typedef void* (*mcitemcopyfn_t)(void*);
typedef void (*mcitemdeinitfn_t)(void*);
typedef mcastexpression_t* (*mcastrightassocparsefn_t)(mcastparser_t*);
typedef mcastexpression_t* (*mcleftassocparsefn_t)(mcastparser_t*, mcastexpression_t*);



MC_FORCEINLINE mcvalue_t mc_value_makenull();

void optprs_fprintmaybearg(FILE *out, const char *begin, const char *flagname, size_t flaglen, bool needval, bool maybeval, const char *delim);
void optprs_fprintusage(FILE *out, optlongflags_t *flags);
void optprs_printusage(char *argv[], optlongflags_t *flags, bool fail);
uint64_t stod_leading_zeros64(uint64_t x);
double stod_diyfp2d(mcstoddiyfp_t v);
mcstoddiyfp_t stod_diyfp_shift_left(mcstoddiyfp_t v, unsigned shift);
mcstoddiyfp_t stod_diyfp_shift_right(mcstoddiyfp_t v, unsigned shift);
mcstoddiyfp_t stod_diyfp_mul(mcstoddiyfp_t lhs, mcstoddiyfp_t rhs);
mcstoddiyfp_t stod_diyfp_normalize(mcstoddiyfp_t v);
uint64_t stod_read_uint64(const unsigned char *start, size_t length, size_t *ndigits);
mcstoddiyfp_t stod_cached_power_dec(int exp, int *dec_exp);
mcstoddiyfp_t stod_adjust_pow10(int exp);
int stod_diyfp_sgnd_size(int order);
double stod_strtod(const unsigned char **start, const unsigned char *end, int literal);
char *mc_util_readhandle(FILE *hnd, size_t *dlen);
char *mc_util_readfile(const char *filename, size_t *dlen);
char *mc_fsutil_fileread(mcstate_t *state, const char *filename, size_t *flen);
size_t mc_fsutil_filewrite(mcstate_t *state, const char *path, const char *string, size_t stringsize);
size_t mc_util_strlen(const char *str);
char *mc_util_strndup(const char *string, size_t n);
char *mc_util_strdup(const char *string);
bool mc_util_strequal(const char *a, const char *b);
bool mc_util_strnequal(const char *a, const char *b, size_t len);
char *mc_util_canonpath(mcstate_t *state, const char *path);
bool mc_util_pathisabsolute(const char *path);
size_t mc_util_hashdata(const void *ptr, size_t len);
size_t mc_util_hashdouble(mcfloat_t val);
size_t mc_util_upperpowoftwo(size_t v);
mcfloat_t mc_util_strtod(const char *str, size_t slen, char **endptr);
PtrList *mc_util_splitstring(const char *str, const char *delimiter);
char *mc_util_joinstringarray(mcstate_t *state, PtrList *items, const char *joinee, size_t jlen);
uint64_t mc_util_doubletouint64(mcfloat_t val);
mcfloat_t mc_util_uint64todouble(uint64_t val);
const char *mc_valtype_getname(mcvaltype_t type);
const char *mc_value_objtypename(mcvaltype_t type);


mcvalue_t mc_value_makestrcapacity(mcstate_t *state, int capacity);
mcvalue_t mc_value_makestringlen(mcstate_t *state, const char *string, size_t len);
mcvalue_t mc_value_makestring(mcstate_t *state, const char *string);
mcvalue_t mc_value_makefuncnative(mcstate_t *state, const char *name, mcnativefn_t fn, void *data);
mcvalue_t mc_value_makearray(mcstate_t *state);
mcvalue_t mc_value_makearraycapacity(mcstate_t *state, size_t capacity);
mcvalue_t mc_value_makemap(mcstate_t *state);
mcvalue_t mc_value_makemapcapacity(mcstate_t *state, size_t capacity);
mcvalue_t mc_value_makeerror(mcstate_t *state, const char *error);
mcvalue_t mc_value_makeerrornocopy(mcstate_t *state, char *error);
mcvalue_t mc_value_makefuncscript(mcstate_t *state, const char *name, mccompiledprogram_t *cres, bool ownsdt, int nlocals, int nargs, int fvc);
mcvalue_t mc_value_makeuserobject(mcstate_t *state, void *data);
mcobjuserdata_t *mc_value_userdatagetdata(mcvalue_t object);
bool mc_value_userdatasetdata(mcvalue_t object, void *extdata);
bool mc_value_userdatasetdestroyfunction(mcvalue_t object, mcitemdestroyfn_t dfn);
bool mc_value_userdatasetcopyfunction(mcvalue_t object, mcitemcopyfn_t copyfn);
bool mc_value_asbool(mcvalue_t obj);
mcfloat_t mc_value_asnumber(mcvalue_t obj);
int mc_value_stringgetlength(mcvalue_t object);
void mc_value_stringsetlength(mcvalue_t object, int len);
bool mc_value_stringappendlen(mcvalue_t obj, const char *src, size_t len);
bool mc_value_stringappend(mcvalue_t obj, const char *src);
bool mc_value_stringappendvalue(mcvalue_t destval, mcvalue_t val);
size_t mc_value_stringgethash(mcvalue_t obj);
bool mc_value_stringrehash(mcvalue_t obj);
mcobjfunction_t *mc_value_asscriptfunction(mcvalue_t object);
const char *mc_value_functiongetname(mcvalue_t obj);
mcvalue_t mc_value_functiongetfreevalat(mcvalue_t obj, int ix);
void mc_value_functionsetfreevalat(mcvalue_t obj, int ix, mcvalue_t val);
mcvalue_t *mc_value_functiongetfreevals(mcvalue_t obj);
const char *mc_value_errorgetmessage(mcvalue_t object);
void mc_value_errorsettraceback(mcvalue_t object, mctraceback_t *traceback);
mctraceback_t *mc_value_errorgettraceback(mcvalue_t object);
mcvalue_t mc_value_arraygetvalue(mcvalue_t object, size_t ix);
bool mc_value_arraysetvalue(mcvalue_t object, size_t ix, mcvalue_t val);
bool mc_value_arraypush(mcvalue_t object, mcvalue_t val);
int mc_value_arraygetlength(mcvalue_t object);
mcvalue_t mc_valarray_pop(mcvalue_t object);
bool mc_value_arrayremoveat(mcvalue_t object, int ix);
int mc_value_mapgetlength(mcvalue_t object);
mcvalue_t mc_value_mapgetkeyat(mcvalue_t object, int ix);
mcvalue_t mc_value_mapgetvalueat(mcvalue_t object, int ix);
bool mc_value_mapsetvalueat(mcvalue_t object, int ix, mcvalue_t val);
mcvalue_t mc_value_mapgetkvpairat(mcstate_t *state, mcvalue_t object, int ix);
bool mc_value_mapsetvalue(mcvalue_t object, mcvalue_t key, mcvalue_t val);
bool mc_value_mapsetvaluestring(mcvalue_t object, const char *strkey, mcvalue_t val);
mcvalue_t mc_value_mapgetvalue(mcvalue_t object, mcvalue_t key);
bool mc_value_mapgetvaluechecked(mcvalue_t object, mcvalue_t key, mcvalue_t *dest);
bool mc_value_maphaskey(mcvalue_t object, mcvalue_t key);
void mc_objectdata_deinit(mcobjdata_t *data);
bool mc_value_callbackequals(mcvalue_t *aptr, mcvalue_t *bptr);
size_t mc_value_callbackhash(mcvalue_t *objptr);
mcvalue_t mc_value_copydeep(mcstate_t *state, mcvalue_t obj);
mcvalue_t mc_value_copyflat(mcstate_t *state, mcvalue_t obj);

bool mc_printutil_bcreadoperands(mcopdefinition_t *def, const uint16_t *instr, uint64_t outoperands[2]);
void mc_printer_printoneinstruc(Printer *pr, uint16_t *code, uint16_t op, size_t *pos, mcastlocation_t *sposlist, bool simple);
void mc_printer_printbytecode(Printer *pr, uint16_t *code, mcastlocation_t *sposlist, size_t codesize, bool simple);
void mc_printer_printobjstring(Printer *pr, mcvalue_t obj);
void mc_printer_printobjfuncscript(Printer *pr, mcvalue_t obj);
void mc_printer_printobjarray(Printer *pr, mcvalue_t obj);
void mc_printer_printobjmap(Printer *pr, mcvalue_t obj);
void mc_printer_printobjerror(Printer *pr, mcvalue_t obj);
void mc_consolecolor_init(mcconsolecolor_t *mcc, int fd);
const char *mc_consolecolor_get(mcconsolecolor_t *mcc, char code);
void mc_printer_printvalue(Printer *pr, mcvalue_t obj, bool accurate);
mcastlocation_t mc_astlocation_make(mcastcompiledfile_t *file, int line, int column);
mcgcmemory_t *mc_value_getmem(mcvalue_t obj);
char *mc_valtype_getunionname(mcstate_t *state, mcvaltype_t type);

mcmodule_t *mc_module_make(mcstate_t *state, const char *name);
void mc_module_destroy(mcmodule_t *module);
bool mc_module_addsymbol(mcmodule_t *module, mcastsymbol_t *symbol);
const char *mc_util_getmodulename(const char *path);
const char *mc_module_findfile(mcstate_t *state, const char *filename);
mcmodule_t *mc_module_copy(mcmodule_t *src);


mcastexpression_t *mc_astexpr_makedefineexpr(mcstate_t *state, mcastexprident_t *name, mcastexpression_t *value, bool assignable);;
mcastexpression_t *mc_astexpr_makeifexpr(mcstate_t *state, PtrList *cases, mcastexprcodeblock_t *alternative);;
mcastexpression_t *mc_astexpr_makereturnexpr(mcstate_t *state, mcastexpression_t *value);;
mcastexpression_t *mc_astexpr_makeexprstmt(mcstate_t *state, mcastexpression_t *value);;
mcastexpression_t *mc_astexpr_makewhileexpr(mcstate_t *state, mcastexpression_t *test, mcastexprcodeblock_t *body);;
mcastexpression_t *mc_astexpr_makebreakexpr(mcstate_t *state);;
mcastexpression_t *mc_astexpr_makeforeachexpr(mcstate_t *state, mcastexprident_t *iterator, mcastexpression_t *source, mcastexprcodeblock_t *body);;
mcastexpression_t *mc_astexpr_makeforloopexpr(mcstate_t *state, mcastexpression_t *init, mcastexpression_t *test, mcastexpression_t *update, mcastexprcodeblock_t *body);;
mcastexpression_t *mc_astexpr_makecontinueexpr(mcstate_t *state);;
mcastexpression_t *mc_astexpr_makeblockexpr(mcstate_t *state, mcastexprcodeblock_t *block);;
mcastexpression_t *mc_astexpr_makeimportexpr(mcstate_t *state, char *path);;
mcastexpression_t *mc_astexpr_makerecoverexpr(mcstate_t *state, mcastexprident_t *eid, mcastexprcodeblock_t *body);;
mcastexpression_t *mc_astexpr_makeexpression(mcstate_t *state, mcastexprtype_t type);;
mcastexpression_t* mc_astexpr_makeident(mcstate_t* state, mcastexprident_t* ident);
mcastexpression_t* mc_astexpr_makeliteralnumber(mcstate_t* state, mcfloat_t val);
mcastexpression_t* mc_astexpr_makeliteralbool(mcstate_t* state, bool val);
mcastexpression_t* mc_astexpr_makeliteralstring(mcstate_t* state, char* value, size_t len);
mcastexpression_t* mc_astexpr_makeliteralnull(mcstate_t* state);
mcastexpression_t* mc_astexpr_makeliteralarray(mcstate_t* state, PtrList* values);
mcastexpression_t* mc_astexpr_makeliteralmap(mcstate_t* state, PtrList* keys, PtrList* values);
mcastexpression_t* mc_astexpr_makeprefixexpr(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* right);
mcastexpression_t* mc_astexpr_makeinfixexpr(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* left, mcastexpression_t* right);
mcastexpression_t* mc_astexpr_makeliteralfunction(mcstate_t* state, PtrList* params, mcastexprcodeblock_t* body);
mcastexpression_t* mc_astexpr_makecallexpr(mcstate_t* state, mcastexpression_t* function, PtrList* args);
mcastexpression_t* mc_astexpr_makeindexexpr(mcstate_t* state, mcastexpression_t* left, mcastexpression_t* index, bool isdot);
mcastexpression_t* mc_astexpr_makeassignexpr(mcstate_t* state, mcastexpression_t* dest, mcastexpression_t* source, bool is_postfix);
mcastexpression_t* mc_astexpr_makelogicalexpr(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* left, mcastexpression_t* right);
mcastexpression_t* mc_astexpr_maketernaryexpr(mcstate_t* state, mcastexpression_t* test, mcastexpression_t* ift, mcastexpression_t* iffalse);
mcastexpression_t* mc_astexpr_makedefineexpr(mcstate_t* state, mcastexprident_t* name, mcastexpression_t* value, bool assignable);
mcastexpression_t* mc_astexpr_makeifexpr(mcstate_t* state, PtrList* cases, mcastexprcodeblock_t* alternative);
mcastexpression_t* mc_astexpr_makereturnexpr(mcstate_t* state, mcastexpression_t* value);
mcastexpression_t* mc_astexpr_makeexprstmt(mcstate_t* state, mcastexpression_t* value);
mcastexpression_t* mc_astexpr_makewhileexpr(mcstate_t* state, mcastexpression_t* test, mcastexprcodeblock_t* body);
mcastexpression_t* mc_astexpr_makebreakexpr(mcstate_t* state);
mcastexpression_t* mc_astexpr_makeforeachexpr(mcstate_t* state, mcastexprident_t* iterator, mcastexpression_t* source, mcastexprcodeblock_t* body);
mcastexpression_t* mc_astexpr_makeforloopexpr(mcstate_t* state, mcastexpression_t* init, mcastexpression_t* test, mcastexpression_t* update, mcastexprcodeblock_t* body);
mcastexpression_t* mc_astexpr_makecontinueexpr(mcstate_t* state);
mcastexpression_t* mc_astexpr_makeblockexpr(mcstate_t* state, mcastexprcodeblock_t* block);
mcastexpression_t* mc_astexpr_makeimportexpr(mcstate_t* state, char* path);
mcastexpression_t* mc_astexpr_makerecoverexpr(mcstate_t* state, mcastexprident_t* eid, mcastexprcodeblock_t* body);
mcastexpression_t* mc_astexpr_makeexpression(mcstate_t* state, mcastexprtype_t type);
void mc_astcodeblock_destroy(mcastexprcodeblock_t *block);
mcastexprcodeblock_t *mc_astcodeblock_copy(mcastexprcodeblock_t *block);
mcastfuncparam_t *mc_astfuncparam_copy(mcastfuncparam_t *param);
void mc_astfuncparam_destroy(mcastfuncparam_t *param);
mcastexprident_t *mc_astident_make(mcstate_t *state, mcasttoken_t tok);
mcastexprident_t *mc_astident_copy(mcastexprident_t *ident);
void mc_astident_destroy(mcastexprident_t *ident);
void mc_astifcase_destroy(mcastexprifcase_t *cond);
mcastexprifcase_t *mc_astifcase_copy(mcastexprifcase_t *ifcase);
mcastexpression_t *mc_astexpr_makeexpression(mcstate_t *state, mcastexprtype_t type);
mcastexpression_t *mc_astparser_parsestatement(mcastparser_t *p);
mcastexpression_t *mc_parser_parsevarletstmt(mcastparser_t *p);
mcastexpression_t *mc_parser_parseifstmt(mcastparser_t *p);
mcastexpression_t *mc_parser_parsereturnstmt(mcastparser_t *p);
mcastexpression_t *mc_parser_parseexprstmt(mcastparser_t *p);
mcastexpression_t *mc_parser_parseloopwhilestmt(mcastparser_t *p);
mcastexpression_t *mc_parser_parsebreakstmt(mcastparser_t *p);
mcastexpression_t *mc_parser_parsecontinuestmt(mcastparser_t *p);
mcastexpression_t *mc_parser_parseblockstmt(mcastparser_t *p);
mcastexpression_t *mc_parser_parseimportstmt(mcastparser_t *p);
mcastexpression_t *mc_parser_parserecoverstmt(mcastparser_t *p);
mcastexpression_t *mc_parser_parseloopforloopstmt(mcastparser_t *p);
mcastexpression_t *mc_parser_parseloopforeachstmt(mcastparser_t *p);
mcastexpression_t *mc_parser_parseloopforcstylestmt(mcastparser_t *p);
mcastexprcodeblock_t *mc_parser_parsecodeblock(mcastparser_t *p);
mcastexpression_t *mc_parser_parseexpression(mcastparser_t *p, mcastprecedence_t prec);
mcastexpression_t *mc_parser_parseident(mcastparser_t *p);
mcastexpression_t *mc_parser_parseliteralnumber(mcastparser_t *p);
mcastexpression_t *mc_parser_parseliteralbool(mcastparser_t *p);
mcastexpression_t *mc_parser_parseliteralstring(mcastparser_t *p);
mcastexpression_t *mc_parser_parseliteraltemplatestring(mcastparser_t *p);
mcastexpression_t *mc_parser_parseliteralnull(mcastparser_t *p);
mcastexpression_t *mc_parser_parseliteralarray(mcastparser_t *p);
mcastexpression_t *mc_parser_parseliteralmap(mcastparser_t *p);
mcastexpression_t *mc_parser_parseprefixexpr(mcastparser_t *p);
mcastexpression_t *mc_parser_parseinfixexpr(mcastparser_t *p, mcastexpression_t *left);
mcastexpression_t *mc_parser_parsegroupedexpr(mcastparser_t *p);
mcastexpression_t *mc_parser_parseliteralfunction(mcastparser_t *p);
mcastexpression_t *mc_parser_parsefunctionstmt(mcastparser_t *p);
mcastexpression_t *mc_parser_parsecallexpr(mcastparser_t *p, mcastexpression_t *left);
PtrList *mc_parser_parseexprlist(mcastparser_t *p, mcasttoktype_t starttoken, mcasttoktype_t endtoken, bool trailingcommaallowed);
mcastexpression_t *mc_parser_parseindexexpr(mcastparser_t *p, mcastexpression_t *left);
mcastexpression_t *mc_parser_parseassignexpr(mcastparser_t *p, mcastexpression_t *left);
mcastexpression_t *mc_parser_parselogicalexpr(mcastparser_t *p, mcastexpression_t *left);
mcastexpression_t *mc_parser_parseternaryexpr(mcastparser_t *p, mcastexpression_t *left);
mcastexpression_t *mc_parser_parseincdecprefixexpr(mcastparser_t *p);
mcastexpression_t *mc_parser_parseincdecpostfixexpr(mcastparser_t *p, mcastexpression_t *left);
mcastexpression_t *mc_parser_parsedotexpression(mcastparser_t *p, mcastexpression_t *left);
mcastexpression_t *mc_optimizer_optinfixexpr(mcastexpression_t *expr);
mcastexpression_t *mc_optimizer_optprefixexpr(mcastexpression_t *expr);
void mc_astcompscope_destroy(mcastscopecomp_t *scope);
mccompiledprogram_t *mc_astcompresult_make(mcstate_t *state, uint16_t *bytecode, mcastlocation_t *srcposlist, int count);
void mc_astcompresult_destroy(mccompiledprogram_t *res);
void mc_compiler_deinit(mcastcompiler_t *comp);
mcopdefinition_t *mc_opdef_lookup(mcopdefinition_t *def, mcinternopcode_t op);
mcastscopecomp_t *mc_compiler_getcompilationscope(mcastcompiler_t *comp);
bool mc_compiler_pushcompilationscope(mcastcompiler_t *comp);
void mc_compiler_popcompilationscope(mcastcompiler_t *comp);
bool mc_compiler_compilestmtlist(mcastcompiler_t *comp, PtrList *statements);
bool mc_compiler_compileexpression(mcastcompiler_t *comp, mcastexpression_t *expr);
bool mc_compiler_compilecodeblock(mcastcompiler_t *comp, mcastexprcodeblock_t *block);
int mc_compiler_addconstant(mcastcompiler_t *comp, mcvalue_t obj);
void mc_compiler_changeuint16operand(mcastcompiler_t *comp, int ip, uint16_t operand);
bool mc_compiler_lastopcodeis(mcastcompiler_t *comp, mcinternopcode_t op);
bool mc_compiler_readsymbol(mcastcompiler_t *comp, mcastsymbol_t *symbol);
bool mc_compiler_storesymbol(mcastcompiler_t *comp, mcastsymbol_t *symbol, bool define);
bool mc_compiler_pushbreakip(mcastcompiler_t *comp, int ip);
void mc_compiler_popbreakip(mcastcompiler_t *comp);
int mc_compiler_getbreakip(mcastcompiler_t *comp);
bool mc_compiler_pushcontinueip(mcastcompiler_t *comp, int ip);
void mc_compiler_popcontinueip(mcastcompiler_t *comp);
int mc_compiler_getcontinueip(mcastcompiler_t *comp);
int mc_compiler_getip(mcastcompiler_t *comp);

PtrList *mc_compiler_getsrcpositions(mcastcompiler_t *comp);
PtrList *mc_compiler_getbytecode(mcastcompiler_t *comp);
mcastscopefile_t *mc_compiler_filescopemake(mcastcompiler_t *comp, mcastcompiledfile_t *file);
void mc_compiler_filescopedestroy(mcastscopefile_t *scope);
bool mc_compiler_filescopepush(mcastcompiler_t *comp, const char *filepath);
void mc_compiler_filescopepop(mcastcompiler_t *comp);
void mc_compiler_setcompilationscope(mcastcompiler_t *comp, mcastscopecomp_t *scope);
mcastcompiledfile_t *mc_compiledfile_make(mcstate_t *state, const char *path);
void mc_compiledfile_destroy(mcastcompiledfile_t *file);
mcastcompiler_t *mc_compiler_make(mcstate_t *state, mcconfig_t *config, mcgcmemory_t *mem, mcerrlist_t *errors, PtrList *files, mcglobalstore_t *gstore);
void mc_compiler_destroy(mcastcompiler_t *comp);
mccompiledprogram_t *mc_compiler_compilesource(mcastcompiler_t *comp, const char *code, const char *filename);
mcastsymtable_t *mc_compiler_getsymtable(mcastcompiler_t *comp);
void mc_compiler_setsymtable(mcastcompiler_t *comp, mcastsymtable_t *table);
mcclass_t *mc_class_make(mcstate_t *state, const char *name, bool istop);
void mc_class_destroy(mcstate_t *state, mcclass_t *cl);
void mc_class_addfunction(mcstate_t *state, mcclass_t *cl, const char *name, bool ispseudo, mcnativefn_t fn);
void mc_class_addmember(mcstate_t *state, mcclass_t *cl, const char *name, mcnativefn_t fn);
void mc_class_addpseudo(mcstate_t *state, mcclass_t *cl, const char *name, mcnativefn_t fn);
mcstate_t *mc_state_make(void);
void mc_state_deinit(mcstate_t *state);
void mc_state_reset(mcstate_t *state);
void mc_state_setdefaultconfig(mcstate_t *state);
void mc_state_destroy(mcstate_t *state);
void mc_state_printerrors(mcstate_t *state);
mccompiledprogram_t *mc_state_compilesource(mcstate_t *state, const char *code, const char *filename);
mcvalue_t mc_program_execute(mcstate_t *state, mccompiledprogram_t *program);
void mc_program_destroy(mccompiledprogram_t *program);
mcvalue_t mc_state_execcode(mcstate_t *state, const char *code, const char *filename);
mcvalue_t mc_state_callfunctionbyname(mcstate_t *state, const char *fname, mcvalue_t thisval, size_t argc, mcvalue_t *args);
bool mc_state_haserrors(mcstate_t *state);
int mc_state_errorcount(mcstate_t *state);
void mc_state_clearerrors(mcstate_t *state);
mcerror_t *mc_state_geterror(mcstate_t *state, int index);
bool mc_state_setnativefunction(mcstate_t *state, const char *name, mcnativefn_t fn, void *data);
bool mc_state_setglobalconstant(mcstate_t *state, const char *name, mcvalue_t obj);
mcvalue_t mc_state_getglobalobjectbyname(mcstate_t *state, const char *name);
bool mc_error_printtraceback(Printer *pr, mctraceback_t *traceback, mcconsolecolor_t *mcc);
bool mc_error_printusererror(Printer *pr, mcvalue_t obj);
bool mc_error_printerror(mcstate_t *state, Printer *pr, mcerror_t *err);
int mc_traceback_getdepth(mctraceback_t *traceback);
const char *mc_traceback_getsourcefilepath(mctraceback_t *traceback, int depth);
const char *mc_traceback_getsourcelinecode(mctraceback_t *traceback, int depth);
int mc_traceback_getsourcelinenumber(mctraceback_t *traceback, int depth);
int mc_traceback_getsourcecolumn(mctraceback_t *traceback, int depth);
const char *mc_traceback_getfunctionname(mctraceback_t *traceback, int depth);
void mc_astprinter_printast(mcstate_t *state, PtrList *statements);
void mc_astprint_stmtlist(mcastprinter_t *apr, PtrList *statements);
void mc_astprint_printfuncliteral(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printcall(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printarrayliteral(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printstringliteral(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printmapliteral(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printprefixexpr(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printinfixexpr(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printindexexpr(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printassignexpr(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printlogicalexpr(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printternaryexpr(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printdefineexpr(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printifexpr(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printwhileexpr(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printforclassicexpr(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printforeachexpr(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printimportexpr(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_printrecoverexpr(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_expression(mcastprinter_t *apr, mcastexpression_t *astexpr);
void mc_astprint_codeblock(mcastprinter_t *apr, mcastexprcodeblock_t *blockexpr);
const char *mc_util_mathopstring(mcastmathoptype_t op);
bool mc_argcheck_check(mcstate_t *state, bool generateerror, size_t argc, mcvalue_t *args, ...);
bool mc_argcheck_checkactual(mcstate_t *state, bool generateerror, size_t argc, mcvalue_t *args, size_t expectedargc, const mcvaltype_t *expectedtypes);
mcgcmemory_t *mc_gcmemory_make(mcstate_t *state);
void mc_gcmemory_destroy(mcgcmemory_t *mem);
mcobjdata_t *mc_gcmemory_allocobjectdata(mcstate_t *state);
mcobjdata_t *mc_gcmemory_getdatafrompool(mcstate_t *state, mcvaltype_t type);
void mc_state_gcunmarkall(mcstate_t *state);
void mc_state_gcmarkobjlist(mcvalue_t *objects, size_t count);
void mc_state_gcmarkobject(mcvalue_t obj);
void mc_state_gcsweep(mcstate_t *state);
int mc_state_gcshouldsweep(mcstate_t *state);
bool mc_state_gcdisablefor(mcvalue_t obj);
void mc_state_gcenablefor(mcvalue_t obj);
mcgcobjdatapool_t *mc_state_gcgetpoolfortype(mcstate_t *state, mcvaltype_t type);
bool mc_state_gccandatabeputinpool(mcstate_t *state, mcobjdata_t *data);
mcglobalstore_t *mc_globalstore_make(mcstate_t *state);
void mc_globalstore_destroy(mcglobalstore_t *store);
mcastsymbol_t *mc_globalstore_getsymbol(mcglobalstore_t *store, const char *name);
bool mc_globalstore_setnamed(mcglobalstore_t *store, const char *name, mcvalue_t object);
mcvalue_t mc_globalstore_getatindex(mcglobalstore_t *store, int ix, bool *outok);
mcvalue_t *mc_globalstore_getdata(mcglobalstore_t *store);
int mc_globalstore_getcount(mcglobalstore_t *store);

mcastscopeblock_t *mc_astblockscope_make(mcstate_t *state, int offset);
void mc_astblockscope_destroy(mcastscopeblock_t *scope);
mcastscopeblock_t *mc_astblockscope_copy(mcastscopeblock_t *scope);
bool mc_symtable_setsymbol(mcastsymtable_t *table, mcastsymbol_t *symbol);
int mc_symtable_nextsymindex(mcastsymtable_t *table);
int mc_symtable_getnumdefs(mcastsymtable_t *table);
void mc_asttoken_init(mcasttoken_t *tok, mcasttoktype_t type, const char *literal, int len);
char *mc_asttoken_dupliteralstring(mcasttoken_t *tok);
const char *mc_asttoken_typename(mcasttoktype_t type);
mctraceback_t *mc_traceback_make(mcstate_t *state);
void mc_traceback_destroy(mctraceback_t *traceback);
bool mc_traceback_push(mctraceback_t *traceback, const char *fname, mcastlocation_t pos);
bool mc_traceback_vmpush(mctraceback_t *traceback, mcstate_t *state);
const char *mc_traceitem_getsourceline(mctraceitem_t *item);
const char *mc_traceitem_getsourcefilepath(mctraceitem_t *item);
bool mc_vm_init(mcstate_t *state);
void mc_vm_reset(mcstate_t *state);
mcvalue_t mc_vm_getglobalbyindex(mcstate_t *state, size_t ix);
void mc_vm_stackpush(mcstate_t *state, mcvalue_t obj);
bool mc_vm_popframe(mcstate_t *state);
void mc_vmutil_getopinfo(mcopcode_t opc, const char **oname);
mcvalue_t mc_scriptfn_typeof(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_arrayfirst(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_arraylast(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_arrayrest(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_reverse(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_makearray(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_externalfn(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_vec2add(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_vec2sub(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_testcheckargs(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_maketestdict(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_squarearray(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_print(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_println(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_tostring(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnjson_stringify(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnnumber_chr(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_length(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_indexof(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_charcodefirst(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_charcodeat(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_charat(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_getself(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_tonumber(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_left(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_right(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_replaceall(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_replacefirst(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_trim(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_matchhelper(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args, bool icase);
mcvalue_t mc_objfnstring_matchglobcase(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_matchglobicase(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_tolower(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnstring_toupper(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnarray_length(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
void mc_vm_savestate(mcstate_t *state, mcexecstate_t *est);
void mc_vm_restorestate(mcstate_t *state, mcexecstate_t *est);
mcvalue_t mc_objfnarray_map(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnarray_push(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnarray_pop(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnarray_join(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnmap_length(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnutil_istype(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args, mcvaltype_t vt);
mcvalue_t mc_objfnobject_iscallable(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnobject_isstring(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnobject_isarray(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnobject_ismap(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnobject_isnumber(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnobject_isbool(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnobject_isnull(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnobject_isfuncscript(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnobject_isexternal(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnobject_iserror(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_objfnobject_isfuncnative(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
void mc_state_makestdclasses(mcstate_t *state);
mcvalue_t mc_scriptfn_isnan(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_range(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_keys(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_values(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_copy(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_copydeep(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_remove(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_removeat(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_error(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_crash(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_assert(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_randseed(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_random(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptutil_slicearray(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptutil_slicestring(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_scriptfn_slice(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnmath_sqrt(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnmath_pow(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnmath_sin(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnmath_cos(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnmath_tan(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnmath_log(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnmath_ceil(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnmath_floor(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnmath_abs(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnfile_writefile(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnfile_readfile(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnfile_join(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnfile_isdirectory(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnfile_isfile(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnfile_stat(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfndir_readdir(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
mcvalue_t mc_nsfnvm_hadrecovered(mcstate_t *state, void *data, mcvalue_t thisval, size_t argc, mcvalue_t *args);
void mc_cli_installbuiltins(mcstate_t *state);
void mc_cli_installjsondummy(mcstate_t *state);
void mc_cli_installjsconsole(mcstate_t *state);
void mc_cli_installmath(mcstate_t *state);
void mc_cli_installfauxjavascript(mcstate_t *state);
void mc_cli_installfileio(mcstate_t *state);
void mc_cli_installdir(mcstate_t *state);
void mc_cli_installvmvar(mcstate_t *state);
void mc_cli_installotherstuff(mcstate_t *state);
bool mc_cli_compileandrunsource(mcstate_t *state, mcvalue_t *vdest, const char *source, const char *filename);
bool mc_cli_compileandrunfile(mcstate_t *state, const char *filename);
void mc_cli_installargv(mcstate_t *state, int argc, char **argv, int beginat);
void mc_cli_printtypesize(const char *name, size_t sz);
void mc_cli_printtypesizes(void);
int main(int argc, char *argv[]);
/* mem.c */
void *mc_memory_malloc(size_t sz);
void *mc_memory_realloc(void *p, size_t nsz);
void *mc_memory_calloc(size_t count, size_t typsize);
void mc_memory_free(void *ptr);

/* utilos.c */
char *osfn_utilstrndup(const char *src, size_t len);
char *osfn_utilstrdup(const char *src);
bool fslib_diropen(FSDirReader *rd, const char *path);
bool fslib_dirread(FSDirReader *rd, FSDirItem *itm);
bool fslib_dirclose(FSDirReader *rd);
int osfn_fdopen(const char *path, int flags, int mode);
int osfn_fdcreat(const char *path, int mode);
int osfn_fdclose(int fd);
size_t osfn_fdread(int fd, void *buf, size_t count);
size_t osfn_fdwrite(int fd, const void *buf, size_t count);
size_t osfn_fdput(int fd, const void *buf, size_t count);
int osfn_chmod(const char *path, int mode);
char *osfn_realpath(const char *path, char *respath);
char *osfn_dirname(const char *path);
char *osfn_fallbackbasename(const char *opath);
char *osfn_basename(const char *path);
int osfn_isatty(int fd);
int osfn_symlink(const char *path1, const char *path2);
int osfn_symlinkat(const char *path1, int fd, const char *path2);
char *osfn_getcwd(char *buf, size_t size);
int osfn_lstat(const char *path, struct stat *buf);
int osfn_truncate(const char *path, size_t length);
unsigned int osfn_sleep(unsigned int seconds);
int osfn_gettimeofday(struct timeval *tp, void *tzp);
int osfn_mkdir(const char *path, size_t mode);
int osfn_chdir(const char *path);
bool osfn_fileexists(const char *filepath);
bool osfn_stat(const char *path, struct stat *st);
bool osfn_statisa(const char *path, struct stat *st, char kind);
bool osfn_pathcheck(const char *path, char mode);
bool osfn_pathisfile(const char *path);
bool osfn_pathisdirectory(const char *path);
/* fnmatch.h */
/* mem.h */
/* optparse.h */
void optprs_fprintusage(FILE *out, optlongflags_t *flags);
/* stod.h */
uint64_t stod_leading_zeros64(uint64_t x);
double stod_diyfp2d(mcstoddiyfp_t v);
mcstoddiyfp_t stod_diyfp_shift_left(mcstoddiyfp_t v, unsigned shift);
mcstoddiyfp_t stod_diyfp_shift_right(mcstoddiyfp_t v, unsigned shift);
mcstoddiyfp_t stod_diyfp_mul(mcstoddiyfp_t lhs, mcstoddiyfp_t rhs);
mcstoddiyfp_t stod_diyfp_normalize(mcstoddiyfp_t v);
uint64_t stod_read_uint64(const unsigned char *start, size_t length, size_t *ndigits);
mcstoddiyfp_t stod_cached_power_dec(int exp, int *dec_exp);
mcstoddiyfp_t stod_adjust_pow10(int exp);
int stod_diyfp_sgnd_size(int order);
double stod_strtod(const unsigned char **start, const unsigned char *end, int literal);



/* must come before any other function/class body */
template<typename... ArgsT>
MC_INLINE void mc_util_assert(bool x, const char* exprstr, const char* file, int line, const char* fmt, ArgsT&&... args)
{
    static auto tmpfprintf = fprintf;
    if(!x)
    {
        fprintf(stderr, "ASSERTION FAILED at %s:%d: %s", file, line, exprstr);
        if(fmt != nullptr)
        {
            fprintf(stderr, ": ");
            tmpfprintf(stderr, fmt, args...);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
        abort();
    }
}




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


template<typename ValType>
class GenericList
{
    public:
        static void destroy(GenericList* list)
        {
            if(list != nullptr)
            {
                mc_memory_free(list->m_listitems);
                mc_memory_free(list);
                list = nullptr;
            }
        }

        static GenericList* copy(GenericList* list)
        {
            size_t i;
            GenericList* nlist;
            nlist = Memory::make<GenericList>(0, list->m_nullvalue);
            for(i=0; i<list->m_listcount; i++)
            {
                nlist->push(list->m_listitems[i]);
            }
            return nlist;
        }


    public:
        size_t m_listcapacity;
        size_t m_listcount;
        ValType* m_listitems;
        ValType m_nullvalue;

    private:
        inline bool removeAtIntern(unsigned int ix)
        {
            size_t tomovebytes;
            void* src;
            void* dest;
            if(ix == (m_listcount - 1))
            {
                m_listcount--;
                return true;
            }
            tomovebytes = (m_listcount - 1 - ix) * sizeof(ValType);
            dest = m_listitems + (ix * sizeof(ValType));
            src = m_listitems + ((ix + 1) * sizeof(ValType));
            memmove(dest, src, tomovebytes);
            m_listcount--;
            return true;
        }

        inline void ensureCapacity(size_t needsize, ValType fillval, bool first)
        {
            size_t i;
            size_t ncap;
            size_t oldcap;
            (void)first;
            if(m_listcapacity < needsize)
            {
                oldcap = m_listcapacity;
                ncap = MC_UTIL_INCCAPACITY(m_listcapacity + needsize);
                m_listcapacity = ncap;
                if(m_listitems == nullptr)
                {
                    m_listitems = (ValType*)mc_memory_malloc(sizeof(ValType) * ncap);
                }
                else
                {
                    m_listitems = (ValType*)mc_memory_realloc(m_listitems, sizeof(ValType) * ncap);
                }
                for(i = oldcap; i < ncap; i++)
                {
                    m_listitems[i] = fillval;
                }
            }
        }
        
    public:
        inline GenericList(size_t initialsize, ValType nullval)
        {
            m_listcount = 0;
            m_listcapacity = 0;
            m_listitems = nullptr;
            m_nullvalue = nullval;
            if(initialsize > 0)
            {
                this->ensureCapacity(initialsize, m_nullvalue, true);
            }
        }

        inline ~GenericList()
        {
            GenericList::destroy(this);
        }

        inline size_t count()
        {
            return m_listcount;
        }

        inline ValType* data()
        {
            return m_listitems;
        }

        inline ValType get(size_t idx)
        {
            return m_listitems[idx];
        }

        inline ValType* getp(size_t idx)
        {
            return &m_listitems[idx];
        }

        inline ValType* set(size_t idx, ValType val)
        {
            size_t need;
            need = idx + 1;
            if(((idx == 0) || (m_listcapacity == 0)) || (idx >= m_listcapacity))
            {
                this->ensureCapacity(need, m_nullvalue, false);
            }
            if(idx > m_listcount)
            {
                m_listcount = idx;
            }
            m_listitems[idx] = val;
            return &m_listitems[idx];
        }

        inline bool push(ValType value)
        {
            size_t oldcap;
            if(m_listcapacity < m_listcount + 1)
            {
                oldcap = m_listcapacity;
                m_listcapacity = MC_UTIL_INCCAPACITY(oldcap);
                if(m_listitems == nullptr)
                {
                    m_listitems = (ValType*)mc_memory_malloc(sizeof(ValType) * m_listcapacity);
                }
                else
                {
                    m_listitems = (ValType*)mc_memory_realloc(m_listitems, sizeof(ValType) * m_listcapacity);
                }
            }
            m_listitems[m_listcount] = value;
            m_listcount++;
            return true;
        }

        inline bool pop(ValType* dest)
        {
            if(m_listcount > 0)
            {
                *dest = m_listitems[m_listcount - 1];
                m_listcount--;
                return true;
            }
            return false;
        }

        inline bool removeAt(unsigned int ix)
        {
            if(ix >= m_listcount)
            {
                return false;
            }
            if(ix == 0)
            {
                m_listitems += sizeof(ValType);
                m_listcapacity--;
                m_listcount--;
                return true;
            }
            return this->removeAtIntern(ix);
        }

        inline void setEmpty()
        {
            if((m_listcapacity > 0) && (m_listitems != nullptr))
            {
                memset(m_listitems, 0, sizeof(ValType) * m_listcapacity);
            }
            m_listcount = 0;
            m_listcapacity = 0;
        }
};


class PtrList
{
    public:
        enum {
            InitialSize = 32,
        };

    public:
        static inline bool initCapacity(PtrList* list, unsigned int capacity, size_t tsz, bool isp)
        {
            if(isp)
            {
                tsz = sizeof(void*);
            }
            list->m_ptrlisttypesize = tsz;
            list->m_ptrlistisptr = isp;
            if(capacity > 0)
            {
                list->m_ptrlistallocdata = (unsigned char*)mc_memory_malloc(capacity * list->m_ptrlisttypesize);
                list->m_ptrlistitems = list->m_ptrlistallocdata;
                if(!list->m_ptrlistallocdata)
                {
                    return false;
                }
            }
            else
            {
                list->m_ptrlistallocdata = nullptr;
                list->m_ptrlistitems = nullptr;
            }
            list->m_listcapacity = capacity;
            list->m_ptrlistcount = 0;
            list->m_ptrlistcaplocked = false;
            return true;
        }

        static inline void deInit(PtrList* list)
        {
            mc_memory_free(list->m_ptrlistallocdata);
        }

        static inline void destroy(PtrList* list, mcitemdestroyfn_t dfn)
        {
            if(list != nullptr)
            {    
                if(dfn)
                {
                    clearAndDestroy(list, dfn);
                }
                deInit(list);
                mc_memory_free(list);
            }
        }

        static inline void clearAndDestroy(PtrList* list, mcitemdestroyfn_t dfn)
        {
            size_t i;
            void* item;
            for(i = 0; i < list->count(); i++)
            {
                item = list->get(i);
                dfn(item);
            }
            list->clear();
        }

        static inline void orphanData(PtrList* list)
        {
            PtrList::initCapacity(list, 0, list->m_ptrlisttypesize, list->m_ptrlistisptr);
        }

    protected:
        unsigned char* m_ptrlistitems;
        unsigned char* m_ptrlistallocdata;
        unsigned int m_ptrlistcount;
        unsigned int m_listcapacity;
        size_t m_ptrlisttypesize;
        bool m_ptrlistcaplocked;
        bool m_ptrlistisptr;

    public:
        inline PtrList(size_t tsz, bool isp): PtrList(PtrList::InitialSize, tsz, isp)
        {
        }

        inline PtrList(unsigned int capacity, size_t tsz, bool isp)
        {
            bool ok;
            ok = initCapacity(this, capacity, tsz, isp);
            assert(ok);
        }

        inline ~PtrList()
        {
            PtrList::destroy(this, nullptr);
        }

        inline PtrList* copy(mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
        {
            bool ok;
            size_t i;
            void* item;
            void* itemcopy;
            PtrList* arrcopy;
            arrcopy = Memory::make<PtrList>(m_ptrlisttypesize, m_ptrlistisptr);
            if(copyfn)
            {
                for(i = 0; i < this->count(); i++)
                {
                    item = (void*)this->get(i);
                    itemcopy = item;
                    if(copyfn)
                    {
                        itemcopy = copyfn(item);
                    }
                    if(item && !itemcopy)
                    {
                        goto listcopyfailed;
                    }
                    ok = arrcopy->push(itemcopy);
                    if(!ok)
                    {
                        goto listcopyfailed;
                    }
                }
                return arrcopy;
            }
            else
            {
                arrcopy->m_listcapacity = m_listcapacity;
                arrcopy->m_ptrlistcount = m_ptrlistcount;
                arrcopy->m_ptrlisttypesize = m_ptrlisttypesize;
                arrcopy->m_ptrlistcaplocked = m_ptrlistcaplocked;
                if(m_ptrlistallocdata)
                {
                    arrcopy->m_ptrlistallocdata = (unsigned char*)mc_memory_malloc(m_listcapacity * m_ptrlisttypesize);
                    if(!arrcopy->m_ptrlistallocdata)
                    {
                        mc_memory_free(arrcopy);
                        return nullptr;
                    }
                    arrcopy->m_ptrlistitems = arrcopy->m_ptrlistallocdata;
                    memcpy(arrcopy->m_ptrlistallocdata, m_ptrlistitems, m_listcapacity * m_ptrlisttypesize);
                }
                else
                {
                    arrcopy->m_ptrlistallocdata = nullptr;
                    arrcopy->m_ptrlistitems = nullptr;
                }
            }
            return arrcopy;
        listcopyfailed:
            PtrList::destroy(arrcopy, dfn);
            return nullptr;
        }

        inline bool push(void* value)
        {
            unsigned int ncap;
            unsigned char* newdata;
            if(m_ptrlistcount >= m_listcapacity)
            {
                MC_ASSERT(!m_ptrlistcaplocked);
                if(m_ptrlistcaplocked)
                {
                    return false;
                }
                ncap = MC_UTIL_INCCAPACITY(m_listcapacity);
                newdata = (unsigned char*)mc_memory_realloc(m_ptrlistallocdata, ncap * m_ptrlisttypesize);
                if(!newdata)
                {
                    return false;
                }
                m_ptrlistallocdata = newdata;
                m_ptrlistitems = m_ptrlistallocdata;
                m_listcapacity = ncap;
            }
            if(value)
            {
                if(m_ptrlistisptr)
                {
                    ((void**)m_ptrlistitems)[m_ptrlistcount] = value;
                }
                else
                {
                    memcpy(m_ptrlistitems + (m_ptrlistcount * m_ptrlisttypesize), value, m_ptrlisttypesize);
                }
            }
            m_ptrlistcount++;
            return true;
        }

        inline bool pop(void** outvalue)
        {
            void* res;
            if(m_ptrlistcount <= 0)
            {
                return false;
            }
            if(outvalue != nullptr)
            {
                res = this->get(m_ptrlistcount - 1);
                if(m_ptrlistisptr)
                {
                    *outvalue = res;
                }
                else
                {
                    memcpy(*outvalue, res, m_ptrlisttypesize);
                }
            }
            this->removeAt(m_ptrlistcount - 1);
            return true;
        }

        inline void* popReturn()
        {
            void* dest;
            if(this->pop(&dest))
            {
                return dest;
            }
            return nullptr;
        }

        inline void* top()
        {
            if(m_ptrlistcount <= 0)
            {
                return nullptr;
            }
            return this->get(m_ptrlistcount - 1);
        }

        inline bool set(unsigned int ix, void* value)
        {
            size_t offset;
            if(ix >= m_ptrlistcount)
            {
                MC_ASSERT(false);
                return false;
            }
            if(m_ptrlistisptr)
            {
                ((void**)m_ptrlistitems)[ix] = value;
            }
            else
            {
                offset = ix * m_ptrlisttypesize;
                memmove(m_ptrlistitems + offset, value, m_ptrlisttypesize);
            }
            return true;
        }

        inline void* get(unsigned int ix)
        {
            size_t offset;
            if(ix >= m_ptrlistcount)
            {
                MC_ASSERT(false);
                return nullptr;
            }
            if(m_ptrlistisptr)
            {
                return ((void**)m_ptrlistitems)[ix];
            }
            offset = ix * m_ptrlisttypesize;
            return m_ptrlistitems + offset;
        }

        inline size_t count()
        {
            return m_ptrlistcount;
        }

        inline bool removeAt(unsigned int ix)
        {
            size_t tomovebytes;
            void* dest;
            void* src;
            if(ix >= m_ptrlistcount)
            {
                return false;
            }
            if(!m_ptrlistisptr)
            {
                if(ix == 0)
                {
                    m_ptrlistitems += m_ptrlisttypesize;
                    m_listcapacity--;
                    m_ptrlistcount--;
                    return true;
                }
                if(ix == (m_ptrlistcount - 1))
                {
                    m_ptrlistcount--;
                    return true;
                }
                tomovebytes = (m_ptrlistcount - 1 - ix) * m_ptrlisttypesize;
                dest = m_ptrlistitems + (ix * m_ptrlisttypesize);
                src = m_ptrlistitems + ((ix + 1) * m_ptrlisttypesize);
                memmove(dest, src, tomovebytes);
            }
            m_ptrlistcount--;
            return true;
        }

        inline void clear()
        {
            m_ptrlistcount = 0;
        }

        inline void* data()
        {
            return m_ptrlistitems;
        }

        inline void* getConstAt(unsigned int ix)
        {
            size_t offset;
            if(ix >= m_ptrlistcount)
            {
                MC_ASSERT(false);
                return nullptr;
            }
            offset = ix * m_ptrlisttypesize;
            return m_ptrlistitems + offset;
        }

        inline int getIndexOf(void* ptr)
        {
            size_t i;
            for(i = 0; i < this->count(); i++)
            {
                if(this->getConstAt(i) == ptr)
                {
                    return i;
                }
            }
            return -1;
        }

        inline bool removeItem(void* ptr)
        {
            int ix;
            ix = this->getIndexOf(ptr);
            if(ix < 0)
            {
                return false;
            }
            return this->removeAt(ix);
        }

        inline bool contains(void* ptr)
        {
            return this->getIndexOf(ptr) >= 0;
        }
};

template<typename TypeKeyT, typename TypeValueT>
class GenericDict
{
    public:
        static inline bool initDict(GenericDict* dict, unsigned int initialcapacity)
        {
            unsigned int i;
            dict->m_vdcells = nullptr;
            dict->m_vdkeys = nullptr;
            dict->m_vdvalues = nullptr;
            dict->m_vdcellindices = nullptr;
            dict->m_vdhashes = nullptr;
            dict->m_vdcount = 0;
            dict->m_vdcellcapacity = initialcapacity;
            dict->m_vditemcapacity = (unsigned int)(initialcapacity * 0.7f);
            dict->m_funckeyequalsfn = nullptr;
            dict->m_funchashfn = nullptr;
            dict->m_vdcells = (unsigned int*)mc_memory_malloc(dict->m_vdcellcapacity * sizeof(unsigned int));
            dict->m_vdkeys = (char**)mc_memory_malloc(dict->m_vditemcapacity * sizeof(TypeKeyT));
            dict->m_vdvalues = (void**)mc_memory_malloc(dict->m_vditemcapacity * sizeof(TypeValueT));
            dict->m_vdcellindices = (unsigned int*)mc_memory_malloc(dict->m_vditemcapacity * sizeof(unsigned int));
            dict->m_vdhashes = (long unsigned int*)mc_memory_malloc(dict->m_vditemcapacity * sizeof(unsigned long));
            if(dict->m_vdcells == nullptr || dict->m_vdkeys == nullptr || dict->m_vdvalues == nullptr || dict->m_vdcellindices == nullptr || dict->m_vdhashes == nullptr)
            {
                goto dictallocfailed;
            }
            for(i = 0; i < dict->m_vdcellcapacity; i++)
            {
                dict->m_vdcells[i] = MC_CONF_VALDICTINVALIDIX;
            }
            return true;
        dictallocfailed:
            mc_memory_free(dict->m_vdcells);
            mc_memory_free(dict->m_vdkeys);
            mc_memory_free(dict->m_vdvalues);
            mc_memory_free(dict->m_vdcellindices);
            mc_memory_free(dict->m_vdhashes);
            return false;
        }

        static inline void deinit(GenericDict* dict)
        {
            dict->m_vdcount = 0;
            dict->m_vditemcapacity = 0;
            dict->m_vdcellcapacity = 0;
            mc_memory_free(dict->m_vdcells);
            mc_memory_free(dict->m_vdkeys);
            mc_memory_free(dict->m_vdvalues);
            mc_memory_free(dict->m_vdcellindices);
            mc_memory_free(dict->m_vdhashes);
            dict->m_vdcells = nullptr;
            dict->m_vdkeys = nullptr;
            dict->m_vdvalues = nullptr;
            dict->m_vdcellindices = nullptr;
            dict->m_vdhashes = nullptr;
        }


        static inline void destroy(GenericDict* dict)
        {
            if(dict != nullptr)
            {
                GenericDict::deinit(dict);
                mc_memory_free(dict);
            }
        }

    public:
        unsigned int* m_vdcells;
        unsigned long* m_vdhashes;
        char** m_vdkeys;
        void** m_vdvalues;
        unsigned int* m_vdcellindices;
        unsigned int m_vdcount;
        unsigned int m_vditemcapacity;
        unsigned int m_vdcellcapacity;
        mcitemhashfn_t m_funchashfn;
        mcitemcomparefn_t m_funckeyequalsfn;

    public:
        inline GenericDict(): GenericDict(MC_CONF_GENERICDICTINITSIZE)
        {
        }

        inline GenericDict(unsigned int mincapacity)
        {
            bool ok;
            unsigned int capacity;
            capacity = mc_util_upperpowoftwo(mincapacity * 2);
            ok = GenericDict::initDict(this, capacity);
            assert(ok);
        }

        inline void setHashFunction(mcitemhashfn_t hashfn)
        {
            m_funchashfn = hashfn;
        }

        inline void setEqualsFunction(mcitemcomparefn_t equalsfn)
        {
            m_funckeyequalsfn = equalsfn;
        }

        inline bool setKVIntern(unsigned int cellix, unsigned long hash, void* key, void* value)
        {
            bool ok;
            bool found;
            unsigned int lastix;
            if(m_vdcount >= m_vditemcapacity)
            {
                ok = this->growAndRehash();
                if(!ok)
                {
                    return false;
                }
                cellix = this->getCellIndex(key, hash, &found);
            }
            lastix = m_vdcount;
            m_vdcount++;
            m_vdcells[cellix] = lastix;
            this->setKeyAt(lastix, key);
            this->setValueAt(lastix, value);
            m_vdcellindices[lastix] = cellix;
            m_vdhashes[lastix] = hash;
            return true;
        }

        inline bool setKV(void* key, void* value)
        {
            bool found;
            unsigned long hash;
            unsigned int cellix;
            unsigned int itemix;
            hash = this->hashKey(key);
            found = false;
            cellix = this->getCellIndex(key, hash, &found);
            if(found)
            {
                itemix = m_vdcells[cellix];
                this->setValueAt(itemix, value);
                return true;
            }
            return this->setKVIntern(cellix, hash, key, value);
        }

        inline void* get(void* key)
        {
            bool found;
            unsigned int itemix;
            unsigned long hash;
            unsigned long cellix;
            if(m_vdcount == 0)
            {
                return nullptr;
            }
            hash = this->hashKey(key);
            found = false;
            cellix = this->getCellIndex(key, hash, &found);
            if(!found)
            {
                return nullptr;
            }
            itemix = m_vdcells[cellix];
            return this->getValueAt(itemix);
        }

        inline void* getKeyAt(unsigned int ix)
        {
            if(ix >= m_vdcount)
            {
                return nullptr;
            }
            return (char*)m_vdkeys + (sizeof(TypeKeyT) * ix);
        }

        inline void* getValueAt(unsigned int ix)
        {
            if(ix >= m_vdcount)
            {
                return nullptr;
            }
            return (char*)m_vdvalues + (sizeof(TypeValueT) * ix);
        }

        inline unsigned int getCapacity()
        {
            return m_vditemcapacity;
        }

        inline bool setValueAt(unsigned int ix, void* value)
        {
            size_t offset;
            if(ix >= m_vdcount)
            {
                return false;
            }
            offset = ix * sizeof(TypeValueT);
            memcpy((char*)m_vdvalues + offset, value, sizeof(TypeValueT));
            return true;
        }

        inline int count()
        {
            return m_vdcount;
        }

        inline bool removeByKey(void* key)
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
            hash = this->hashKey(key);
            found = false;
            cell = this->getCellIndex(key, hash, &found);
            if(!found)
            {
                return false;
            }
            itemix = m_vdcells[cell];
            lastitemix = m_vdcount - 1;
            if(itemix < lastitemix)
            {
                lastkey = this->getKeyAt(lastitemix);
                this->setKeyAt(itemix, lastkey);
                lastvalue = this->getKeyAt(lastitemix);
                this->setValueAt(itemix, lastvalue);
                m_vdcellindices[itemix] = m_vdcellindices[lastitemix];
                m_vdhashes[itemix] = m_vdhashes[lastitemix];
                m_vdcells[m_vdcellindices[itemix]] = itemix;
            }
            m_vdcount--;
            i = cell;
            j = i;
            for(x = 0; x < (m_vdcellcapacity - 1); x++)
            {
                j = (j + 1) & (m_vdcellcapacity - 1);
                if(m_vdcells[j] == MC_CONF_VALDICTINVALIDIX)
                {
                    break;
                }
                k = (unsigned int)(m_vdhashes[m_vdcells[j]]) & (m_vdcellcapacity - 1);
                if((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j)))
                {
                    m_vdcellindices[m_vdcells[j]] = i;
                    m_vdcells[i] = m_vdcells[j];
                    i = j;
                }
            }
            m_vdcells[i] = MC_CONF_VALDICTINVALIDIX;
            return true;
        }

        inline void clear()
        {
            unsigned int i;
            m_vdcount = 0;
            for(i = 0; i < m_vdcellcapacity; i++)
            {
                m_vdcells[i] = MC_CONF_VALDICTINVALIDIX;
            }
        }

        inline unsigned int getCellIndex(void* key, unsigned long hash, bool* outfound)
        {
            bool areequal;
            unsigned int i;
            unsigned int ix;
            unsigned int cell;
            unsigned int cellix;
            unsigned long hashtocheck;
            void* keytocheck;
            *outfound = false;
            cellix = (unsigned int)hash & (m_vdcellcapacity - 1);
            for(i = 0; i < m_vdcellcapacity; i++)
            {
                ix = (cellix + i) & (m_vdcellcapacity - 1);
                cell = m_vdcells[ix];
                if(cell == MC_CONF_VALDICTINVALIDIX)
                {
                    return ix;
                }
                hashtocheck = m_vdhashes[cell];
                if(hash != hashtocheck)
                {
                    continue;
                }
                keytocheck = this->getKeyAt(cell);
                areequal = this->keysAreEqual(key, keytocheck);
                if(areequal)
                {
                    *outfound = true;
                    return ix;
                }
            }
            return MC_CONF_VALDICTINVALIDIX;
        }

        inline bool growAndRehash()
        {
            bool ok;
            unsigned int i;
            unsigned ncap;
            char* key;
            void* value;
            ncap = MC_UTIL_INCCAPACITY(m_vdcellcapacity);    
            GenericDict newdict(ncap);
            newdict.m_funckeyequalsfn = m_funckeyequalsfn;
            newdict.m_funchashfn = m_funchashfn;
            for(i = 0; i < m_vdcount; i++)
            {
                key = (char*)this->getKeyAt(i);
                value = this->getValueAt(i);
                ok = newdict.setKV(key, value);
                if(!ok)
                {
                    GenericDict::deinit(&newdict);
                    return false;
                }
            }
            GenericDict::deinit(this);
            *this = newdict;
            return true;
        }

        inline bool setKeyAt(unsigned int ix, void* key)
        {
            size_t offset;
            if(ix >= m_vdcount)
            {
                return false;
            }
            offset = ix * sizeof(TypeKeyT);
            memcpy((char*)m_vdkeys + offset, key, sizeof(TypeKeyT));
            return true;
        }

        inline bool keysAreEqual(void* a, void* b)
        {
            if(m_funckeyequalsfn)
            {
                return m_funckeyequalsfn(a, b);
            }
            return memcmp(a, b, sizeof(TypeKeyT)) == 0;
        }

        inline unsigned long hashKey(void* key)
        {
            if(m_funchashfn)
            {
                return m_funchashfn(key);
            }
            return mc_util_hashdata(key, sizeof(TypeKeyT));
        }
};


class PtrDict
{
    public:
        static bool initValues(PtrDict* dict, unsigned int initialcapacity, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
        {
            unsigned int i;
            dict->gdcells = nullptr;
            dict->gdkeys = nullptr;
            dict->gdvalues = nullptr;
            dict->gdcellindices = nullptr;
            dict->gdhashes = nullptr;
            dict->gdcount = 0;
            dict->gdcellcapacity = 0;
            dict->gdcellcapacity += initialcapacity;
            dict->gditemcapacity = (unsigned int)(initialcapacity * 0.7f);
            dict->funccopyfn = copyfn;
            dict->funcdestroyfn = dfn;
            dict->gdcells = (unsigned int*)mc_memory_malloc(dict->gdcellcapacity * sizeof(*dict->gdcells));
            dict->gdkeys = (char**)mc_memory_malloc(dict->gditemcapacity * sizeof(*dict->gdkeys));
            dict->gdvalues = (void**)mc_memory_malloc(dict->gditemcapacity * sizeof(*dict->gdvalues));
            dict->gdcellindices = (unsigned int*)mc_memory_malloc(dict->gditemcapacity * sizeof(*dict->gdcellindices));
            dict->gdhashes = (long unsigned int*)mc_memory_malloc(dict->gditemcapacity * sizeof(*dict->gdhashes));
            if(dict->gdcells == nullptr || dict->gdkeys == nullptr || dict->gdvalues == nullptr || dict->gdcellindices == nullptr || dict->gdhashes == nullptr)
            {
                goto dictallocfailed;
            }
            for(i = 0; i < dict->gdcellcapacity; i++)
            {
                dict->gdcells[i] = MC_CONF_GENERICDICTINVALIDIX;
            }
            return true;
        dictallocfailed:
            mc_memory_free(dict->gdcells);
            mc_memory_free(dict->gdkeys);
            mc_memory_free(dict->gdvalues);
            mc_memory_free(dict->gdcellindices);
            mc_memory_free(dict->gdhashes);
            return false;
        }

        static void deinitValues(PtrDict* dict, bool freekeys)
        {
            unsigned int i;
            if(freekeys)
            {
                for(i = 0; i < dict->gdcount; i++)
                {
                    if(dict->gdkeys[i] != nullptr)
                    {
                        mc_memory_free(dict->gdkeys[i]);
                        dict->gdkeys[i] = nullptr;
                    }
                }
            }
            dict->gdcount = 0;
            dict->gditemcapacity = 0;
            dict->gdcellcapacity = 0;
            mc_memory_free(dict->gdcells);
            mc_memory_free(dict->gdkeys);
            mc_memory_free(dict->gdvalues);
            mc_memory_free(dict->gdcellindices);
            mc_memory_free(dict->gdhashes);
            dict->gdcells = nullptr;
            dict->gdkeys = nullptr;
            dict->gdvalues = nullptr;
            dict->gdcellindices = nullptr;
            dict->gdhashes = nullptr;
        }

        static void destroy(PtrDict* dict)
        {
            if(dict != nullptr)
            {
                PtrDict::deinitValues(dict, true);
                mc_memory_free(dict);
            }
        }

        static void destroyItemsAndDict(PtrDict* dict)
        {
            unsigned int i;
            if(dict != nullptr)
            {    
                if(dict->funcdestroyfn)
                {
                    for(i = 0; i < dict->gdcount; i++)
                    {
                        dict->funcdestroyfn(dict->gdvalues[i]);
                    }
                }
                PtrDict::destroy(dict);
            }
        }

    public:
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

    public:
        PtrDict(): PtrDict(nullptr, nullptr)
        {
        }

        PtrDict(size_t cap, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
        {
            bool ok;
            ok = PtrDict::initValues(this, cap, copyfn, dfn);
            assert(ok);
        }

        PtrDict(mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
        {
            bool ok;
            ok = PtrDict::initValues(this, MC_CONF_GENERICDICTINITSIZE, copyfn, dfn);
            assert(ok);
        }

        bool growAndRehash()
        {
            bool ok;
            unsigned int i;
            char* key;
            void* value;
            size_t ncap;
            ncap = MC_UTIL_INCCAPACITY(this->gdcellcapacity);
            PtrDict newdict(ncap, this->funccopyfn, this->funcdestroyfn);
            for(i = 0; i < this->gdcount; i++)
            {
                key = this->gdkeys[i];
                value = this->gdvalues[i];
                ok = newdict.setActual(key, key, value);
                if(!ok)
                {
                    PtrDict::deinitValues(&newdict, false);
                    return false;
                }
            }
            PtrDict::deinitValues(this, false);
            *this = newdict;
            return true;
        }

        unsigned int getCellIndex(const char* key, unsigned long hash, bool* outfound)
        {
            unsigned int i;
            unsigned int ix;
            unsigned int cell;
            unsigned int cellix;
            unsigned long hashtocheck;
            const char* keytocheck;
            *outfound = false;
            cellix = (unsigned int)hash & (this->gdcellcapacity - 1);
            for(i = 0; i < this->gdcellcapacity; i++)
            {
                ix = (cellix + i) & (this->gdcellcapacity - 1);
                cell = this->gdcells[ix];
                if(cell == MC_CONF_GENERICDICTINVALIDIX)
                {
                    return ix;
                }
                hashtocheck = this->gdhashes[cell];
                if(hash != hashtocheck)
                {
                    continue;
                }
                keytocheck = this->gdkeys[cell];
                if(strcmp(key, keytocheck) == 0)
                {
                    *outfound = true;
                    return ix;
                }
            }
            return MC_CONF_GENERICDICTINVALIDIX;
        }

        bool setIntern(unsigned int cellix, unsigned long hash, const char* ckey, char* mkey, void* value)
        {
            bool ok;
            bool found;
            char* keycopy;
            if(this->gdcount >= this->gditemcapacity)
            {
                ok = this->growAndRehash();
                if(!ok)
                {
                    return false;
                }
                cellix = this->getCellIndex(ckey, hash, &found);
            }
            if(mkey)
            {
                this->gdkeys[this->gdcount] = mkey;
            }
            else
            {
                keycopy = mc_util_strdup(ckey);
                if(!keycopy)
                {
                    return false;
                }
                this->gdkeys[this->gdcount] = keycopy;
            }
            this->gdcells[cellix] = this->gdcount;
            this->gdvalues[this->gdcount] = value;
            this->gdcellindices[this->gdcount] = cellix;
            this->gdhashes[this->gdcount] = hash;
            this->gdcount++;
            return true;
        }

        bool setActual(const char* ckey, char* mkey, void* value)
        {
            bool found;
            unsigned int cellix;
            unsigned int itemix;
            unsigned long hash;
            hash = mc_util_hashdata(ckey, mc_util_strlen(ckey));
            found = false;
            cellix = this->getCellIndex(ckey, hash, &found);
            if(found)
            {
                itemix = this->gdcells[cellix];
                this->gdvalues[itemix] = value;
                return true;
            }
            return this->setIntern(cellix, hash, ckey, mkey, value);
        }

        bool set(const char* key, void* value)
        {
            return this->setActual(key, nullptr, value);
        }

        void* get(const char* key)
        {
            bool found;
            unsigned int itemix;
            unsigned long hash;
            unsigned long cellix;
            hash = mc_util_hashdata(key, mc_util_strlen(key));
            found = false;
            cellix = this->getCellIndex(key, hash, &found);
            if(found == false)
            {
                return nullptr;
            }
            itemix = this->gdcells[cellix];
            return this->gdvalues[itemix];
        }

        void* getValueAt(unsigned int ix)
        {
            if(ix >= this->gdcount)
            {
                return nullptr;
            }
            return this->gdvalues[ix];
        }

        const char* getKeyAt(unsigned int ix)
        {
            if(ix >= this->gdcount)
            {
                return nullptr;
            }
            return this->gdkeys[ix];
        }

        size_t count()
        {
            return this->gdcount;
        }

        bool removeByKey(const char* key)
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
            cell = this->getCellIndex(key, hash, &found);
            if(!found)
            {
                return false;
            }
            itemix = this->gdcells[cell];
            mc_memory_free(this->gdkeys[itemix]);
            lastitemix = this->gdcount - 1;
            if(itemix < lastitemix)
            {
                this->gdkeys[itemix] = this->gdkeys[lastitemix];
                this->gdvalues[itemix] = this->gdvalues[lastitemix];
                this->gdcellindices[itemix] = this->gdcellindices[lastitemix];
                this->gdhashes[itemix] = this->gdhashes[lastitemix];
                this->gdcells[this->gdcellindices[itemix]] = itemix;
            }
            this->gdcount--;
            i = cell;
            j = i;
            for(x = 0; x < (this->gdcellcapacity - 1); x++)
            {
                j = (j + 1) & (this->gdcellcapacity - 1);
                if(this->gdcells[j] == MC_CONF_GENERICDICTINVALIDIX)
                {
                    break;
                }
                k = (unsigned int)(this->gdhashes[this->gdcells[j]]) & (this->gdcellcapacity - 1);
                if((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j)))
                {
                    this->gdcellindices[this->gdcells[j]] = i;
                    this->gdcells[i] = this->gdcells[j];
                    i = j;
                }
            }
            this->gdcells[i] = MC_CONF_GENERICDICTINVALIDIX;
            return true;
        }

        PtrDict* copy()
        {
            bool ok;
            size_t i;
            void* item;
            void* itemcopy;
            const char* key;
            PtrDict* dictcopy;
            if(!this->funccopyfn || !this->funcdestroyfn)
            {
                return nullptr;
            }
            dictcopy = Memory::make<PtrDict>(this->funccopyfn, this->funcdestroyfn);
            if(!dictcopy)
            {
                return nullptr;
            }
            for(i = 0; i < this->count(); i++)
            {
                key = this->getKeyAt(i);
                item = this->getValueAt(i);
                itemcopy = dictcopy->funccopyfn(item);
                if(item && !itemcopy)
                {
                    PtrDict::destroyItemsAndDict(dictcopy);
                    return nullptr;
                }
                ok = dictcopy->set(key, itemcopy);
                if(!ok)
                {
                    dictcopy->funcdestroyfn(itemcopy);
                    PtrDict::destroyItemsAndDict(dictcopy);
                    return nullptr;
                }
            }
            return dictcopy;
        }
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
    PtrList* members;
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
    PtrList* statements;
};

struct mcastexprliteralmap_t
{
    PtrList* litmapkeys;
    PtrList* litmapvalues;
};

struct mcastexprliteralarray_t
{
    PtrList* litarritems;
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
    PtrList* funcparamlist;
    mcastexprcodeblock_t* body;
};

struct mcastexprcall_t
{
    mcastexpression_t* function;
    PtrList* args;
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
    PtrList* cases;
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
    public:
        static mcastexpression_t* copyExpression(mcastexpression_t* expr)
        {
            mcastexpression_t* res;
            if(!expr)
            {
                return nullptr;
            }
            res = nullptr;
            switch(expr->exprtype)
            {
                case MC_EXPR_NONE:
                    {
                        MC_ASSERT(false);
                    }
                    break;
                case MC_EXPR_IDENT:
                    {
                        mcastexprident_t* ident;
                        ident = mc_astident_copy(expr->uexpr.exprident);
                        if(!ident)
                        {
                            return nullptr;
                        }
                        res = mc_astexpr_makeident(expr->pstate, ident);
                        if(!res)
                        {
                            mc_astident_destroy(ident);
                            return nullptr;
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
                        stringcopy = mc_util_strndup(expr->uexpr.exprlitstring.data, expr->uexpr.exprlitstring.length);
                        if(!stringcopy)
                        {
                            return nullptr;
                        }
                        res = mc_astexpr_makeliteralstring(expr->pstate, stringcopy, expr->uexpr.exprlitstring.length);
                        if(!res)
                        {
                            mc_memory_free(stringcopy);
                            return nullptr;
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
                        PtrList* valuescopy;
                        valuescopy = expr->uexpr.exprlitarray.litarritems->copy((mcitemcopyfn_t)copyExpression, (mcitemdestroyfn_t)destroyExpression);
                        if(!valuescopy)
                        {
                            return nullptr;
                        }
                        res = mc_astexpr_makeliteralarray(expr->pstate, valuescopy);
                        if(!res)
                        {
                            PtrList::destroy(valuescopy, (mcitemdestroyfn_t)destroyExpression);
                            return nullptr;
                        }
                    }
                    break;

                case MC_EXPR_MAPLITERAL:
                    {
                        PtrList* keyscopy;
                        PtrList* valuescopy;
                        keyscopy = expr->uexpr.exprlitmap.litmapkeys->copy((mcitemcopyfn_t)copyExpression, (mcitemdestroyfn_t)destroyExpression);
                        valuescopy = expr->uexpr.exprlitmap.litmapvalues->copy((mcitemcopyfn_t)copyExpression, (mcitemdestroyfn_t)destroyExpression);
                        if(!keyscopy || !valuescopy)
                        {
                            PtrList::destroy(keyscopy, (mcitemdestroyfn_t)destroyExpression);
                            PtrList::destroy(valuescopy, (mcitemdestroyfn_t)destroyExpression);
                            return nullptr;
                        }
                        res = mc_astexpr_makeliteralmap(expr->pstate, keyscopy, valuescopy);
                        if(!res)
                        {
                            PtrList::destroy(keyscopy, (mcitemdestroyfn_t)destroyExpression);
                            PtrList::destroy(valuescopy, (mcitemdestroyfn_t)destroyExpression);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_PREFIX:
                    {
                        mcastexpression_t* rightcopy;
                        rightcopy = copyExpression(expr->uexpr.exprprefix.right);
                        if(!rightcopy)
                        {
                            return nullptr;
                        }
                        res = mc_astexpr_makeprefixexpr(expr->pstate, expr->uexpr.exprprefix.op, rightcopy);
                        if(!res)
                        {
                            destroyExpression(rightcopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_INFIX:
                    {
                        mcastexpression_t* leftcopy;
                        mcastexpression_t* rightcopy;
                        leftcopy = copyExpression(expr->uexpr.exprinfix.left);
                        rightcopy = copyExpression(expr->uexpr.exprinfix.right);
                        if(!leftcopy || !rightcopy)
                        {
                            destroyExpression(leftcopy);
                            destroyExpression(rightcopy);
                            return nullptr;
                        }
                        res = mc_astexpr_makeinfixexpr(expr->pstate, expr->uexpr.exprinfix.op, leftcopy, rightcopy);
                        if(!res)
                        {
                            destroyExpression(leftcopy);
                            destroyExpression(rightcopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_FUNCTIONLITERAL:
                    {
                        char* namecopy;
                        PtrList* pacopy;
                        mcastexprcodeblock_t* bodycopy;
                        pacopy = expr->uexpr.exprlitfunction.funcparamlist->copy((mcitemcopyfn_t)mc_astfuncparam_copy, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                        bodycopy = mc_astcodeblock_copy(expr->uexpr.exprlitfunction.body);
                        namecopy = mc_util_strdup(expr->uexpr.exprlitfunction.name);
                        if(!pacopy || !bodycopy)
                        {
                            PtrList::destroy(pacopy, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                            mc_astcodeblock_destroy(bodycopy);
                            mc_memory_free(namecopy);
                            return nullptr;
                        }
                        res = mc_astexpr_makeliteralfunction(expr->pstate, pacopy, bodycopy);
                        if(!res)
                        {
                            PtrList::destroy(pacopy, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                            mc_astcodeblock_destroy(bodycopy);
                            mc_memory_free(namecopy);
                            return nullptr;
                        }
                        res->uexpr.exprlitfunction.name = namecopy;
                    }
                    break;
                case MC_EXPR_CALL:
                    {
                        mcastexpression_t* fcopy;
                        PtrList* argscopy;
                        fcopy = copyExpression(expr->uexpr.exprcall.function);
                        argscopy = expr->uexpr.exprcall.args->copy((mcitemcopyfn_t)copyExpression, (mcitemdestroyfn_t)destroyExpression);
                        if(!fcopy || !argscopy)
                        {
                            destroyExpression(fcopy);
                            PtrList::destroy(expr->uexpr.exprcall.args, (mcitemdestroyfn_t)destroyExpression);
                            return nullptr;
                        }
                        res = mc_astexpr_makecallexpr(expr->pstate, fcopy, argscopy);
                        if(!res)
                        {
                            destroyExpression(fcopy);
                            PtrList::destroy(expr->uexpr.exprcall.args, (mcitemdestroyfn_t)destroyExpression);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_INDEX:
                    {
                        mcastexpression_t* leftcopy;
                        mcastexpression_t* indexcopy;
                        leftcopy = copyExpression(expr->uexpr.exprindex.left);
                        indexcopy = copyExpression(expr->uexpr.exprindex.index);
                        if(!leftcopy || !indexcopy)
                        {
                            destroyExpression(leftcopy);
                            destroyExpression(indexcopy);
                            return nullptr;
                        }
                        res = mc_astexpr_makeindexexpr(expr->pstate, leftcopy, indexcopy, false);
                        if(!res)
                        {
                            destroyExpression(leftcopy);
                            destroyExpression(indexcopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_ASSIGN:
                    {
                        mcastexpression_t* destcopy;
                        mcastexpression_t* sourcecopy;
                        destcopy = copyExpression(expr->uexpr.exprassign.dest);
                        sourcecopy = copyExpression(expr->uexpr.exprassign.source);
                        if(!destcopy || !sourcecopy)
                        {
                            destroyExpression(destcopy);
                            destroyExpression(sourcecopy);
                            return nullptr;
                        }
                        res = mc_astexpr_makeassignexpr(expr->pstate, destcopy, sourcecopy, expr->uexpr.exprassign.is_postfix);
                        if(!res)
                        {
                            destroyExpression(destcopy);
                            destroyExpression(sourcecopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_LOGICAL:
                    {
                        mcastexpression_t* leftcopy;
                        mcastexpression_t* rightcopy;
                        leftcopy = copyExpression(expr->uexpr.exprlogical.left);
                        rightcopy = copyExpression(expr->uexpr.exprlogical.right);
                        if(!leftcopy || !rightcopy)
                        {
                            destroyExpression(leftcopy);
                            destroyExpression(rightcopy);
                            return nullptr;
                        }
                        res = mc_astexpr_makelogicalexpr(expr->pstate, expr->uexpr.exprlogical.op, leftcopy, rightcopy);
                        if(!res)
                        {
                            destroyExpression(leftcopy);
                            destroyExpression(rightcopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_TERNARY:
                    {
                        mcastexpression_t* testcopy;
                        mcastexpression_t* iftruecopy;
                        mcastexpression_t* iffalsecopy;
                        testcopy = copyExpression(expr->uexpr.exprternary.tercond);
                        iftruecopy = copyExpression(expr->uexpr.exprternary.teriftrue);
                        iffalsecopy = copyExpression(expr->uexpr.exprternary.teriffalse);
                        if(!testcopy || !iftruecopy || !iffalsecopy)
                        {
                            destroyExpression(testcopy);
                            destroyExpression(iftruecopy);
                            destroyExpression(iffalsecopy);
                            return nullptr;
                        }
                        res = mc_astexpr_maketernaryexpr(expr->pstate, testcopy, iftruecopy, iffalsecopy);
                        if(!res)
                        {
                            destroyExpression(testcopy);
                            destroyExpression(iftruecopy);
                            destroyExpression(iffalsecopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_STMTDEFINE:
                    {
                        mcastexpression_t* valuecopy;
                        valuecopy = copyExpression(expr->uexpr.exprdefine.value);
                        if(!valuecopy)
                        {
                            return nullptr;
                        }
                        res = mc_astexpr_makedefineexpr(expr->pstate, mc_astident_copy(expr->uexpr.exprdefine.name), valuecopy, expr->uexpr.exprdefine.assignable);
                        if(!res)
                        {
                            destroyExpression(valuecopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_STMTIF:
                    {
                        PtrList* casescopy;
                        mcastexprcodeblock_t* alternativecopy;
                        casescopy = expr->uexpr.exprifstmt.cases->copy((mcitemcopyfn_t)mc_astifcase_copy, (mcitemdestroyfn_t)mc_astifcase_destroy);
                        alternativecopy = mc_astcodeblock_copy(expr->uexpr.exprifstmt.alternative);
                        if(!casescopy || !alternativecopy)
                        {
                            PtrList::destroy(casescopy, (mcitemdestroyfn_t)mc_astifcase_destroy);
                            mc_astcodeblock_destroy(alternativecopy);
                            return nullptr;
                        }
                        res = mc_astexpr_makeifexpr(expr->pstate, casescopy, alternativecopy);
                        if(res)
                        {
                            PtrList::destroy(casescopy, (mcitemdestroyfn_t)mc_astifcase_destroy);
                            mc_astcodeblock_destroy(alternativecopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_STMTRETURN:
                    {
                        mcastexpression_t* valuecopy;
                        valuecopy = copyExpression(expr->uexpr.exprreturnvalue);
                        if(!valuecopy)
                        {
                            return nullptr;
                        }
                        res = mc_astexpr_makereturnexpr(expr->pstate, valuecopy);
                        if(!res)
                        {
                            destroyExpression(valuecopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_STMTEXPRESSION:
                    {
                        mcastexpression_t* valuecopy;
                        valuecopy = copyExpression(expr->uexpr.exprexpression);
                        if(!valuecopy)
                        {
                            return nullptr;
                        }
                        res = mc_astexpr_makeexprstmt(expr->pstate, valuecopy);
                        if(!res)
                        {
                            destroyExpression(valuecopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_STMTLOOPWHILE:
                    {
                        mcastexpression_t* testcopy;
                        mcastexprcodeblock_t* bodycopy;
                        testcopy = copyExpression(expr->uexpr.exprwhileloopstmt.loopcond);
                        bodycopy = mc_astcodeblock_copy(expr->uexpr.exprwhileloopstmt.body);
                        if(!testcopy || !bodycopy)
                        {
                            destroyExpression(testcopy);
                            mc_astcodeblock_destroy(bodycopy);
                            return nullptr;
                        }
                        res = mc_astexpr_makewhileexpr(expr->pstate, testcopy, bodycopy);
                        if(!res)
                        {
                            destroyExpression(testcopy);
                            mc_astcodeblock_destroy(bodycopy);
                            return nullptr;
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
                        mcastexprcodeblock_t* bodycopy;
                        sourcecopy = copyExpression(expr->uexpr.exprforeachloopstmt.source);
                        bodycopy = mc_astcodeblock_copy(expr->uexpr.exprforeachloopstmt.body);
                        if(!sourcecopy || !bodycopy)
                        {
                            destroyExpression(sourcecopy);
                            mc_astcodeblock_destroy(bodycopy);
                            return nullptr;
                        }
                        res = mc_astexpr_makeforeachexpr(expr->pstate, mc_astident_copy(expr->uexpr.exprforeachloopstmt.iterator), sourcecopy, bodycopy);
                        if(!res)
                        {
                            destroyExpression(sourcecopy);
                            mc_astcodeblock_destroy(bodycopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_STMTLOOPFORCLASSIC:
                    {
                        mcastexpression_t* initcopy;
                        mcastexpression_t* testcopy;
                        mcastexpression_t* updatecopy;
                        mcastexprcodeblock_t* bodycopy;
                        initcopy= copyExpression(expr->uexpr.exprforloopstmt.init);
                        testcopy = copyExpression(expr->uexpr.exprforloopstmt.loopcond);
                        updatecopy = mcastexpression_t::copyExpression(expr->uexpr.exprforloopstmt.update);
                        bodycopy = mc_astcodeblock_copy(expr->uexpr.exprforloopstmt.body);
                        if(!initcopy || !testcopy || !updatecopy || !bodycopy)
                        {
                            destroyExpression(initcopy);
                            destroyExpression(testcopy);
                            destroyExpression(updatecopy);
                            mc_astcodeblock_destroy(bodycopy);
                            return nullptr;
                        }
                        res = mc_astexpr_makeforloopexpr(expr->pstate, initcopy, testcopy, updatecopy, bodycopy);
                        if(!res)
                        {
                            destroyExpression(initcopy);
                            destroyExpression(testcopy);
                            destroyExpression(updatecopy);
                            mc_astcodeblock_destroy(bodycopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_STMTBLOCK:
                    {
                        mcastexprcodeblock_t* blockcopy;
                        blockcopy = mc_astcodeblock_copy(expr->uexpr.exprblockstmt);
                        if(!blockcopy)
                        {
                            return nullptr;
                        }
                        res = mc_astexpr_makeblockexpr(expr->pstate, blockcopy);
                        if(!res)
                        {
                            mc_astcodeblock_destroy(blockcopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_STMTIMPORT:
                    {
                        char* pathcopy;
                        pathcopy = mc_util_strdup(expr->uexpr.exprimportstmt.path);
                        if(!pathcopy)
                        {
                            return nullptr;
                        }
                        res = mc_astexpr_makeimportexpr(expr->pstate, pathcopy);
                        if(!res)
                        {
                            mc_memory_free(pathcopy);
                            return nullptr;
                        }
                    }
                    break;
                case MC_EXPR_STMTRECOVER:
                    {
                        mcastexprcodeblock_t* bodycopy;
                        mcastexprident_t* erroridentcopy;
                        bodycopy = mc_astcodeblock_copy(expr->uexpr.exprrecoverstmt.body);
                        erroridentcopy = mc_astident_copy(expr->uexpr.exprrecoverstmt.errident);
                        if(!bodycopy || !erroridentcopy)
                        {
                            mc_astcodeblock_destroy(bodycopy);
                            mc_astident_destroy(erroridentcopy);
                            return nullptr;
                        }
                        res = mc_astexpr_makerecoverexpr(expr->pstate, erroridentcopy, bodycopy);
                        if(!res)
                        {
                            mc_astcodeblock_destroy(bodycopy);
                            mc_astident_destroy(erroridentcopy);
                            return nullptr;
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
                return nullptr;
            }
            res->pos = expr->pos;
            return res;
        }

        static void destroyExpression(mcastexpression_t* expr)
        {
            if(expr != nullptr)
            {
                switch(expr->exprtype)
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
                            mc_memory_free(expr->uexpr.exprlitstring.data);
                        }
                        break;
                    case MC_EXPR_NULLLITERAL:
                        {
                        }
                        break;
                    case MC_EXPR_ARRAYLITERAL:
                        {
                            PtrList::destroy(expr->uexpr.exprlitarray.litarritems, (mcitemdestroyfn_t)destroyExpression);
                        }
                        break;
                    case MC_EXPR_MAPLITERAL:
                        {
                            PtrList::destroy(expr->uexpr.exprlitmap.litmapkeys, (mcitemdestroyfn_t)destroyExpression);
                            PtrList::destroy(expr->uexpr.exprlitmap.litmapvalues, (mcitemdestroyfn_t)destroyExpression);
                        }
                        break;
                    case MC_EXPR_PREFIX:
                        {
                            destroyExpression(expr->uexpr.exprprefix.right);
                        }
                        break;
                    case MC_EXPR_INFIX:
                        {
                            destroyExpression(expr->uexpr.exprinfix.left);
                            destroyExpression(expr->uexpr.exprinfix.right);
                        }
                        break;
                    case MC_EXPR_FUNCTIONLITERAL:
                        {
                            mcastexprliteralfunction_t* fn;
                            fn = &expr->uexpr.exprlitfunction;
                            mc_memory_free(fn->name);
                            PtrList::destroy(fn->funcparamlist, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                            mc_astcodeblock_destroy(fn->body);
                        }
                        break;
                    case MC_EXPR_CALL:
                        {
                            PtrList::destroy(expr->uexpr.exprcall.args, (mcitemdestroyfn_t)destroyExpression);
                            destroyExpression(expr->uexpr.exprcall.function);
                        }
                        break;
                    case MC_EXPR_INDEX:
                        {
                            destroyExpression(expr->uexpr.exprindex.left);
                            destroyExpression(expr->uexpr.exprindex.index);
                        }
                        break;
                    case MC_EXPR_ASSIGN:
                        {
                            destroyExpression(expr->uexpr.exprassign.dest);
                            destroyExpression(expr->uexpr.exprassign.source);
                        }
                        break;
                    case MC_EXPR_LOGICAL:
                        {
                            destroyExpression(expr->uexpr.exprlogical.left);
                            destroyExpression(expr->uexpr.exprlogical.right);
                        }
                        break;
                    case MC_EXPR_TERNARY:
                        {
                            destroyExpression(expr->uexpr.exprternary.tercond);
                            destroyExpression(expr->uexpr.exprternary.teriftrue);
                            destroyExpression(expr->uexpr.exprternary.teriffalse);
                        }
                        break;
                    case MC_EXPR_STMTDEFINE:
                        {
                            mc_astident_destroy(expr->uexpr.exprdefine.name);
                            destroyExpression(expr->uexpr.exprdefine.value);
                        }
                        break;
                    case MC_EXPR_STMTIF:
                        {
                            PtrList::destroy(expr->uexpr.exprifstmt.cases, (mcitemdestroyfn_t)mc_astifcase_destroy);
                            mc_astcodeblock_destroy(expr->uexpr.exprifstmt.alternative);
                        }
                        break;
                    case MC_EXPR_STMTRETURN:
                        {
                            destroyExpression(expr->uexpr.exprreturnvalue);
                        }
                        break;
                    case MC_EXPR_STMTEXPRESSION:
                        {
                            destroyExpression(expr->uexpr.exprexpression);
                        }
                        break;
                    case MC_EXPR_STMTLOOPWHILE:
                        {
                            destroyExpression(expr->uexpr.exprwhileloopstmt.loopcond);
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
                            destroyExpression(expr->uexpr.exprforeachloopstmt.source);
                            mc_astcodeblock_destroy(expr->uexpr.exprforeachloopstmt.body);
                        }
                        break;
                    case MC_EXPR_STMTLOOPFORCLASSIC:
                        {
                            destroyExpression(expr->uexpr.exprforloopstmt.init);
                            destroyExpression(expr->uexpr.exprforloopstmt.loopcond);
                            destroyExpression(expr->uexpr.exprforloopstmt.update);
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
                            mc_memory_free(expr->uexpr.exprimportstmt.path);
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
                mc_memory_free(expr);
            }
        }

    public:
        mcstate_t* pstate;
        mcastexprtype_t exprtype;
        mcastlocation_t pos;
        mcexprunion_t uexpr;

    public:

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

struct mcobjmap_t
{
    GenericDict<mcvalue_t, mcvalue_t>* actualmap;
};

struct mcobjarray_t
{
    GenericList<mcvalue_t>* actualarray;
};

struct mcobjfunction_t
{
    public:
        union
        {
            struct
            {
                union
                {
                    mcvalue_t* freevalsallocated;
                    mcvalue_t freevalsstack[2];
                } ufv;
                
                union
                {
                    char* fallocname;
                    const char* fconstname;
                } unamev;
                mccompiledprogram_t* compiledprogcode;
                int numlocals;
                int numargs;
                int freevalscount;
                bool ownsdata;
            } valscriptfunc;

            struct
            {
                char* natfnname;
                mcnativefn_t natptrfn;
                void* userpointer;
            } valnativefunc;
        } funcdata;

    public:
        bool freeValuesAreAllocated()
        {
            return this->funcdata.valscriptfunc.freevalscount >= MC_UTIL_STATICARRAYSIZE(this->funcdata.valscriptfunc.ufv.freevalsstack);
        }

};

union mcobjunion_t
{
    mcobjstring_t valstring;
    mcobjerror_t valerror;
    mcobjarray_t* valarray;
    mcobjmap_t* valmap;
    mcobjfunction_t valfunc;
    mcobjuserdata_t valuserobject;
};

struct mcobjdata_t
{
    mcstate_t* pstate;
    mcgcmemory_t* mem;
    mcvaltype_t odtype;
    bool gcmark;
    mcobjunion_t uvobj;
};

struct mcopdefinition_t
{
    const char* name;
    int numoperands;
    int operandwidths[2];
};

struct mcastsymbol_t
{
    public:
        mcstate_t* pstate;
        mcastsymtype_t symtype;
        char* name;
        int index;
        bool assignable;

    public:
        static mcastsymbol_t* make(mcstate_t* state, const char* name, mcastsymtype_t type, int index, bool assignable)
        {
            mcastsymbol_t* symbol;
            symbol = Memory::make<mcastsymbol_t>();
            if(!symbol)
            {
                return nullptr;
            }
            memset(symbol, 0, sizeof(mcastsymbol_t));
            symbol->pstate = state;
            symbol->name = mc_util_strdup(name);
            if(!symbol->name)
            {
                Memory::destroy(symbol);
                symbol = nullptr;
                return nullptr;
            }
            symbol->symtype = type;
            symbol->index = index;
            symbol->assignable = assignable;
            return symbol;
        }

        static void destroy(mcastsymbol_t* symbol)
        {
            if(symbol != nullptr)
            {
                mc_memory_free(symbol->name);
                symbol->name = nullptr;
                mc_memory_free(symbol);
                symbol = nullptr;
            }
        }

        static mcastsymbol_t* copy(mcastsymbol_t* symbol)
        {
            return mcastsymbol_t::make(symbol->pstate, symbol->name, symbol->symtype, symbol->index, symbol->assignable);
        }
};

struct mcastscopeblock_t
{
    mcstate_t* pstate;
    PtrDict* scopestore;
    int offset;
    int numdefinitions;
};

struct mcastsymtable_t
{
    public:
        mcstate_t* pstate;
        mcastsymtable_t* outer;
        mcglobalstore_t* symglobalstore;
        PtrList* blockscopes;
        PtrList* freesymbols;
        PtrList* modglobalsymbols;
        int maxnumdefinitions;
        int modglobaloffset;

    public:
        static mcastsymtable_t* make(mcstate_t* state, mcastsymtable_t* outer, mcglobalstore_t* gstore, int mgo)
        {
            bool ok;
            mcastsymtable_t* table;
            table = Memory::make<mcastsymtable_t>();
            if(!table)
            {
                return nullptr;
            }
            memset(table, 0, sizeof(mcastsymtable_t));
            table->pstate = state;
            table->maxnumdefinitions = 0;
            table->outer = outer;
            table->symglobalstore = gstore;
            table->modglobaloffset = mgo;
            table->blockscopes = Memory::make<PtrList>(sizeof(void*), true);
            if(!table->blockscopes)
            {
                goto err;
            }
            table->freesymbols = Memory::make<PtrList>(sizeof(void*), true);
            if(!table->freesymbols)
            {
                goto err;
            }
            table->modglobalsymbols = Memory::make<PtrList>(sizeof(void*), true);
            if(!table->modglobalsymbols)
            {
                goto err;
            }
            ok = table->scopeBlockPush();
            if(!ok)
            {
                goto err;
            }
            return table;
        err:
            mcastsymtable_t::destroy(table);
            return nullptr;
        }

        static void destroy(mcastsymtable_t* table)
        {
            if(table != nullptr)
            {
                while(table->blockscopes->count() > 0)
                {
                    table->scopeBlockPop();
                }
                PtrList::destroy(table->blockscopes, nullptr);
                PtrList::destroy(table->modglobalsymbols, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
                PtrList::destroy(table->freesymbols, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
                memset(table, 0, sizeof(mcastsymtable_t));
                mc_memory_free(table);
                table = nullptr;
            }
        }

        mcastsymtable_t* copy()
        {
            mcastsymtable_t* copy;
            copy = Memory::make<mcastsymtable_t>();
            if(!copy)
            {
                return nullptr;
            }
            memset(copy, 0, sizeof(mcastsymtable_t));
            copy->pstate = this->pstate;
            copy->outer = this->outer;
            copy->symglobalstore = this->symglobalstore;
            copy->blockscopes = this->blockscopes->copy((mcitemcopyfn_t)mc_astblockscope_copy, (mcitemdestroyfn_t)mc_astblockscope_destroy);
            if(!copy->blockscopes)
            {
                goto err;
            }
            copy->freesymbols = this->freesymbols->copy((mcitemcopyfn_t)mcastsymbol_t::copy, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
            if(!copy->freesymbols)
            {
                goto err;
            }
            copy->modglobalsymbols = this->modglobalsymbols->copy((mcitemcopyfn_t)mcastsymbol_t::copy, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
            if(!copy->modglobalsymbols)
            {
                goto err;
            }
            copy->maxnumdefinitions = this->maxnumdefinitions;
            copy->modglobaloffset = this->modglobaloffset;
            return copy;
        err:
            mcastsymtable_t::destroy(copy);
            return nullptr;
        }

        bool addModuleSymbol(mcastsymbol_t* symbol)
        {
            bool ok;
            mcastsymbol_t* copy;
            if(symbol->symtype != MC_SYM_MODULEGLOBAL)
            {
                MC_ASSERT(false);
                return false;
            }
            if(this->isDefined(symbol->name))
            {
                /* todo: make sure it should be true in this case */
                return true;
            }
            copy = mcastsymbol_t::copy(symbol);
            if(!copy)
            {
                return false;
            }
            ok = mc_symtable_setsymbol(this, copy);
            if(!ok)
            {
                mcastsymbol_t::destroy(copy);
                return false;
            }
            return true;
        }

        mcastsymbol_t* defineSymbol(const char* name, bool assignable)
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
            globalsymbol = mc_globalstore_getsymbol(this->symglobalstore, name);
            if(globalsymbol)
            {
                return nullptr;
            }
            /* module symbol */
            if(strchr(name, ':'))
            {
                return nullptr;
            }
            /* "this" is reserved */
            #if 1
            if(mc_util_strequal(name, "this"))
            {
                return this->defineThis();
            }
            #endif
            symboltype = this->outer == nullptr ? MC_SYM_MODULEGLOBAL : MC_SYM_LOCAL;
            ix = mc_symtable_nextsymindex(this);
            symbol = mcastsymbol_t::make(this->pstate, name, symboltype, ix, assignable);
            if(!symbol)
            {
                return nullptr;
            }
            globalsymboladded = false;
            ok = false;
            if(symboltype == MC_SYM_MODULEGLOBAL && this->blockscopes->count() == 1)
            {
                globalsymbolcopy = mcastsymbol_t::copy(symbol);
                if(!globalsymbolcopy)
                {
                    mcastsymbol_t::destroy(symbol);
                    return nullptr;
                }
                ok = this->modglobalsymbols->push(globalsymbolcopy);
                if(!ok)
                {
                    mcastsymbol_t::destroy(globalsymbolcopy);
                    mcastsymbol_t::destroy(symbol);
                    return nullptr;
                }
                globalsymboladded = true;
            }
            ok = mc_symtable_setsymbol(this, symbol);
            if(!ok)
            {
                if(globalsymboladded)
                {
                    globalsymbolcopy = (mcastsymbol_t*)this->modglobalsymbols->popReturn();
                    mcastsymbol_t::destroy(globalsymbolcopy);
                }
                mcastsymbol_t::destroy(symbol);
                return nullptr;
            }
            topscope = (mcastscopeblock_t*)this->blockscopes->top();
            topscope->numdefinitions++;
            definitionscount = mc_symtable_getnumdefs(this);
            if(definitionscount > this->maxnumdefinitions)
            {
                this->maxnumdefinitions = definitionscount;
            }
            return symbol;
        }

        mcastsymbol_t* defineAndDestroyOld(mcastsymbol_t* original)
        {
            bool ok;
            mcastsymbol_t* copy;
            mcastsymbol_t* symbol;
            copy = mcastsymbol_t::make(this->pstate, original->name, original->symtype, original->index, original->assignable);
            if(!copy)
            {
                return nullptr;
            }
            ok = this->freesymbols->push(copy);
            if(!ok)
            {
                mcastsymbol_t::destroy(copy);
                return nullptr;
            }
            symbol = mcastsymbol_t::make(this->pstate, original->name, MC_SYM_FREE, this->freesymbols->count() - 1, original->assignable);
            if(!symbol)
            {
                return nullptr;
            }
            ok = mc_symtable_setsymbol(this, symbol);
            if(!ok)
            {
                mcastsymbol_t::destroy(symbol);
                return nullptr;
            }
            return symbol;
        }

        mcastsymbol_t* defineFunctionName(const char* name, bool assignable)
        {
            bool ok;
            mcastsymbol_t* symbol;
            /* module symbol */
            if(strchr(name, ':'))
            {
                return nullptr;
            }
            symbol = mcastsymbol_t::make(this->pstate, name, MC_SYM_FUNCTION, 0, assignable);
            if(!symbol)
            {
                return nullptr;
            }
            ok = mc_symtable_setsymbol(this, symbol);
            if(!ok)
            {
                mcastsymbol_t::destroy(symbol);
                return nullptr;
            }
            return symbol;
        }

        mcastsymbol_t* defineThis()
        {
            bool ok;
            mcastsymbol_t* symbol;
            symbol = mcastsymbol_t::make(this->pstate, "this", MC_SYM_THIS, 0, false);
            if(!symbol)
            {
                return nullptr;
            }
            ok = mc_symtable_setsymbol(this, symbol);
            if(!ok)
            {
                mcastsymbol_t::destroy(symbol);
                return nullptr;
            }
            return symbol;
        }

        mcastsymbol_t* resolve(const char* name)
        {
            int i;
            mcastsymbol_t* symbol;
            mcastscopeblock_t* scope;
            symbol = nullptr;
            scope = nullptr;
            symbol = mc_globalstore_getsymbol(this->symglobalstore, name);
            if(symbol)
            {
                return symbol;
            }

            for(i = this->blockscopes->count() - 1; i >= 0; i--)
            {
                scope = (mcastscopeblock_t*)this->blockscopes->get(i);
                symbol = (mcastsymbol_t*)scope->scopestore->get(name);
                if(symbol)
                {
                    break;
                }
            }
            if(symbol && symbol->symtype == MC_SYM_THIS)
            {
                symbol = this->defineAndDestroyOld(symbol);
            }
            if(!symbol && this->outer)
            {
                symbol = this->outer->resolve(name);
                if(!symbol)
                {
                    return nullptr;
                }
                if(symbol->symtype == MC_SYM_MODULEGLOBAL || symbol->symtype == MC_SYM_GLOBALBUILTIN)
                {
                    return symbol;
                }
                symbol = this->defineAndDestroyOld(symbol);
            }
            return symbol;
        }

        bool isDefined(const char* name)
        {
            /* todo: rename to something more obvious */
            mcastsymbol_t* symbol;
            mcastscopeblock_t* topscope;
            symbol = mc_globalstore_getsymbol(this->symglobalstore, name);
            if(symbol)
            {
                return true;
            }
            topscope = (mcastscopeblock_t*)this->blockscopes->top();
            symbol = (mcastsymbol_t*)topscope->scopestore->get(name);
            if(symbol)
            {
                return true;
            }
            return false;
        }

        bool scopeBlockPush()
        {
            bool ok;
            int blockscopeoffset;
            mcastscopeblock_t* newscope;
            mcastscopeblock_t* prevblockscope;
            blockscopeoffset = 0;
            prevblockscope = (mcastscopeblock_t*)this->blockscopes->top();
            if(prevblockscope)
            {
                blockscopeoffset = this->modglobaloffset + prevblockscope->offset + prevblockscope->numdefinitions;
            }
            else
            {
                blockscopeoffset = this->modglobaloffset;
            }
            newscope = mc_astblockscope_make(this->pstate, blockscopeoffset);
            if(!newscope)
            {
                return false;
            }
            ok = this->blockscopes->push(newscope);
            if(!ok)
            {
                mc_astblockscope_destroy(newscope);
                return false;
            }
            return true;
        }

        void scopeBlockPop()
        {
            mcastscopeblock_t* topscope;
            topscope = (mcastscopeblock_t*)this->blockscopes->top();
            this->blockscopes->pop(nullptr);
            mc_astblockscope_destroy(topscope);
        }

        mcastscopeblock_t* scopeBlockGet()
        {
            mcastscopeblock_t* topscope;
            topscope = (mcastscopeblock_t*)this->blockscopes->top();
            return topscope;
        }

        bool isModuleGlobalScope()
        {
            return this->outer == nullptr;
        }

        bool isTopBlockScope()
        {
            return this->blockscopes->count() == 1;
        }

        bool isTopGlobalScope()
        {
            return this->isModuleGlobalScope() && this->isTopBlockScope();
        }

        size_t getModuleGlobalSymCount()
        {
            return this->modglobalsymbols->count();
        }

        mcastsymbol_t* getModuleGlobalSymAt(int ix)
        {
            return (mcastsymbol_t*)this->modglobalsymbols->get(ix);
        }
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
    PtrList* gcobjlist;
    PtrList* gcobjlistback;
    PtrList* gcobjlistremains;
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
    PtrList* compiledscopebytecode;
    PtrList* scopesrcposlist;
    PtrList* ipstackbreak;
    PtrList* ipstackcontinue;
    mcinternopcode_t lastopcode;
};

struct mcastcompiledfile_t
{
    mcstate_t* pstate;
    char* dir_path;
    char* path;
    PtrList* lines;
};

struct mcerror_t
{
    public:
        static const char* mc_util_errortypename(mcerrtype_t type)
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

    public:
        mcerrtype_t m_errtype = MC_ERROR_NONE;
        char m_message[MC_CONF_MAXERRORMSGLENGTH] = {};
        mcastlocation_t m_pos = {};
        mctraceback_t* m_traceback = nullptr;

    public:
        mcerror_t()
        {
        }

        const char* getMessage()
        {
            return this->m_message;
        }

        const char* getFile()
        {
            if(!this->m_pos.file)
            {
                return nullptr;
            }
            return this->m_pos.file->path;
        }

        const char* getSourceLineCode()
        {
            const char* line;
            PtrList* lines;
            if(!this->m_pos.file)
            {
                return nullptr;
            }
            lines = this->m_pos.file->lines;
            if(this->m_pos.line >= (int)lines->count())
            {
                return nullptr;
            }
            line = (const char*)lines->get(this->m_pos.line);
            return line;
        }

        int getSourceLineNumber()
        {
            if(this->m_pos.line < 0)
            {
                return -1;
            }
            return this->m_pos.line + 1;
        }

        int getSourceColumn()
        {
            if(this->m_pos.column < 0)
            {
                return -1;
            }
            return this->m_pos.column + 1;
        }

        mcerrtype_t getType()
        {
            switch(this->m_errtype)
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

        const char* getTypeString()
        {
            return mc_util_errortypename(this->getType());
        }

        mctraceback_t*getTraceback()
        {
            return (mctraceback_t*)this->m_traceback;
        }

};

struct mcerrlist_t
{
    public:
        mcerror_t m_erroritems[MC_CONF_MAXERRORCOUNT];
        int count;

    public:
        static void init(mcerrlist_t* tgt)
        {
            tgt->count = 0;
        }

        static void deinit(mcerrlist_t* tgt)
        {
            tgt->clear();
        }

        void pushMessage(mcerrtype_t type, mcastlocation_t pos, const char* message)
        {
            int len;
            int tocopy;
            mcerror_t err;
            if(this->count >= MC_CONF_MAXERRORCOUNT)
            {
            }
            else
            {
                err.m_errtype = type;
                len = mc_util_strlen(message);
                tocopy = len;
                if(tocopy >= (MC_CONF_MAXERRORMSGLENGTH - 1))
                {
                    tocopy = MC_CONF_MAXERRORMSGLENGTH - 1;
                }
                memcpy(err.m_message, message, tocopy);
                err.m_message[tocopy] = '\0';
                err.m_pos = pos;
                err.m_traceback = nullptr;
                this->m_erroritems[this->count] = err;
                this->count++;
            }
        }

        template<typename... ArgsT>
        void pushFormat(mcerrtype_t type, mcastlocation_t pos, const char* format, ArgsT&&... args)
        {
            static auto tmpsnprintf = snprintf;
            int needsz;
            int printedsz;
            char res[MC_CONF_MAXERRORMSGLENGTH];
            (void)needsz;
            (void)printedsz;
            needsz = tmpsnprintf(nullptr, 0, format, args...);
            printedsz = tmpsnprintf(res, MC_CONF_MAXERRORMSGLENGTH, format, args...);
            MC_ASSERT(needsz == printedsz);
            this->pushMessage(type, pos, res);
        }


        void clear()
        {
            int i;
            mcerror_t* error;
            for(i = 0; i < this->count; i++)
            {
                error = this->get(i);
                if(error->m_traceback)
                {
                    mc_traceback_destroy(error->m_traceback);
                }
            }
            this->count = 0;
        }

        mcerror_t* get(int ix)
        {
            if(ix >= this->count)
            {
                return nullptr;
            }
            return &this->m_erroritems[ix];
        }

        mcerror_t* getLast()
        {
            if(this->count <= 0)
            {
                return nullptr;
            }
            return &this->m_erroritems[this->count - 1];
        }
};

class AstLexInfo
{
    public:
        mcstate_t* pstate;
        mcerrlist_t* m_errors;
        const char* m_inputsource;
        int m_inputlength;
        int m_position;
        int m_nextposition;
        char m_ch;
        size_t m_line;
        size_t m_column;
        mcastcompiledfile_t* m_file;
        bool m_failed;
        bool m_continuetplstring;
        mcasttoken_t m_prevtoken;
        mcasttoken_t m_currtoken;
        mcasttoken_t m_peektoken;
};

class AstLexer: public AstLexInfo
{
    public:
        static bool charIsLetter(char ch)
        {
            return (
                (('a' <= ch) && (ch <= 'z')) || (('A' <= ch) && (ch <= 'Z')) || (ch == '_')
            );
        }

        static bool charIsDigit(char ch)
        {
            return (
                (ch >= '0') && (ch <= '9')
            );
        }


        static bool charIsOneOf(char ch, const char* allowed, int allowedlen)
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

        static bool init(AstLexer* lex, mcstate_t* state, mcerrlist_t* errs, const char* input, mcastcompiledfile_t* file)
        {
            bool ok;
            lex->pstate = state;
            lex->m_errors = errs;
            lex->m_inputsource = input;
            lex->m_inputlength = (int)mc_util_strlen(input);
            lex->m_position = 0;
            lex->m_nextposition = 0;
            lex->m_ch = '\0';
            if(file)
            {
                lex->m_line = file->lines->count();
            }
            else
            {
                lex->m_line = 0;
            }
            lex->m_column = -1;
            lex->m_file = file;
            ok = lex->addLineAt(0);
            if(!ok)
            {
                return false;
            }
            ok = lex->readChar();
            if(!ok)
            {
                return false;
            }
            lex->m_failed = false;
            lex->m_continuetplstring = false;
            memset(&lex->m_prevstate, 0, sizeof(lex->m_prevstate));
            mc_asttoken_init(&lex->m_prevtoken, MC_TOK_INVALID, nullptr, 0);
            mc_asttoken_init(&lex->m_currtoken, MC_TOK_INVALID, nullptr, 0);
            mc_asttoken_init(&lex->m_peektoken, MC_TOK_INVALID, nullptr, 0);
            return true;
        }

    public:
        AstLexInfo m_prevstate;

    public:
        bool failed()
        {
            return this->m_failed;
        }

        void conttplstring()
        {
            this->m_continuetplstring = true;
        }

        bool currentTokenIs(mcasttoktype_t type)
        {
            return this->m_currtoken.toktype == type;
        }

        bool peekTokenIs(mcasttoktype_t type)
        {
            return this->m_peektoken.toktype == type;
        }

        bool nextToken()
        {
            this->m_prevtoken = this->m_currtoken;
            this->m_currtoken = this->m_peektoken;
            this->m_peektoken = this->nextTokenInternal();
            return !this->m_failed;
        }

        bool previousToken()
        {
            if(this->m_prevtoken.toktype == MC_TOK_INVALID)
            {
                return false;
            }
            this->m_peektoken = this->m_currtoken;
            this->m_currtoken = this->m_prevtoken;
            mc_asttoken_init(&this->m_prevtoken, MC_TOK_INVALID, nullptr, 0);
            this->m_ch = this->m_prevstate.m_ch;
            this->m_column = this->m_prevstate.m_column;
            this->m_line = this->m_prevstate.m_line;
            this->m_position = this->m_prevstate.m_position;
            this->m_nextposition = this->m_prevstate.m_nextposition;
            return true;
        }

        mcasttoken_t nextTokenInternal()
        {
            char c;
            mcasttoken_t outtok;
            this->m_prevstate.m_ch = this->m_ch;
            this->m_prevstate.m_column = this->m_column;
            this->m_prevstate.m_line = this->m_line;
            this->m_prevstate.m_position = this->m_position;
            this->m_prevstate.m_nextposition = this->m_nextposition;
            while(true)
            {
                if(!this->m_continuetplstring)
                {
                    this->skipSpace();
                }
                outtok.toktype = MC_TOK_INVALID;
                outtok.tokstrdata = this->m_inputsource + this->m_position;
                outtok.tokstrlen = 1;
                outtok.pos = mc_astlocation_make(this->m_file, this->m_line, this->m_column);
                c = this->m_continuetplstring ? '`' : this->m_ch;
                switch(c)
                {
                    case '\0':
                        {
                            mc_asttoken_init(&outtok, MC_TOK_EOF, "EOF", 3);
                        }
                        break;
                    case '=':
                        {
                            if(this->peekChar() == '=')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_EQ, "==", 2);
                                this->readChar();
                            }
                            else
                            {
                                mc_asttoken_init(&outtok, MC_TOK_ASSIGN, "=", 1);
                            }
                        }
                        break;
                    case '&':
                        {
                            if(this->peekChar() == '&')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_AND, "&&", 2);
                                this->readChar();
                            }
                            else if(this->peekChar() == '=')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_ASSIGNBINAND, "&=", 2);
                                this->readChar();
                            }
                            else
                            {
                                mc_asttoken_init(&outtok, MC_TOK_BINAND, "&", 1);
                            }
                        }
                        break;
                    case '|':
                        {
                            if(this->peekChar() == '|')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_OR, "||", 2);
                                this->readChar();
                            }
                            else if(this->peekChar() == '=')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_ASSIGNBINOR, "|=", 2);
                                this->readChar();
                            }
                            else
                            {
                                mc_asttoken_init(&outtok, MC_TOK_BINOR, "|", 1);
                            }
                        }
                        break;
                    case '^':
                        {
                            if(this->peekChar() == '=')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_ASSIGNBINXOR, "^=", 2);
                                this->readChar();
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
                            if(this->peekChar() == '=')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_ASSIGNPLUS, "+=", 2);
                                this->readChar();
                            }
                            else if(this->peekChar() == '+')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_PLUSPLUS, "++", 2);
                                this->readChar();
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
                            if(this->peekChar() == '=')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_ASSIGNMINUS, "-=", 2);
                                this->readChar();
                            }
                            else if(this->peekChar() == '-')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_MINUSMINUS, "--", 2);
                                this->readChar();
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
                            if(this->peekChar() == '=')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_NOTEQ, "!=", 2);
                                this->readChar();
                            }
                            else
                            {
                                mc_asttoken_init(&outtok, MC_TOK_BANG, "!", 1);
                            }
                        }
                        break;
                    case '*':
                        {
                            if(this->peekChar() == '=')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_ASSIGNASTERISK, "*=", 2);
                                this->readChar();
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
                            if(this->peekChar() == '/')
                            {
                                this->readChar();
                                while(this->m_ch != '\n' && this->m_ch != '\0')
                                {
                                    this->readChar();
                                }
                                continue;
                            }
                            if(this->peekChar() == '=')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_ASSIGNSLASH, "/=", 2);
                                this->readChar();
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
                            if(this->peekChar() == '=')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_LTE, "<=", 2);
                                this->readChar();
                            }
                            else if(this->peekChar() == '<')
                            {
                                this->readChar();
                                if(this->peekChar() == '=')
                                {
                                    mc_asttoken_init(&outtok, MC_TOK_ASSIGNLSHIFT, "<<=", 3);
                                    this->readChar();
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
                            if(this->peekChar() == '=')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_GTE, ">=", 2);
                                this->readChar();
                            }
                            else if(this->peekChar() == '>')
                            {
                                this->readChar();
                                if(this->peekChar() == '=')
                                {
                                    mc_asttoken_init(&outtok, MC_TOK_ASSIGNRSHIFT, ">>=", 3);
                                    this->readChar();
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
                            if(this->peekChar() == '=')
                            {
                                mc_asttoken_init(&outtok, MC_TOK_ASSIGNPERCENT, "%=", 2);
                                this->readChar();
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
                            this->readChar();
                            str = this->scanString('"', false, nullptr, &len);
                            if(str)
                            {
                                mc_asttoken_init(&outtok, MC_TOK_STRING, str, len);
                            }
                            else
                            {
                                mc_asttoken_init(&outtok, MC_TOK_INVALID, nullptr, 0);
                            }
                        }
                        break;
                    case '\'':
                        {
                            int len;
                            const char* str;
                            this->readChar();
                            str = this->scanString('\'', false, nullptr, &len);
                            if(str)
                            {
                                mc_asttoken_init(&outtok, MC_TOK_STRING, str, len);
                            }
                            else
                            {
                                mc_asttoken_init(&outtok, MC_TOK_INVALID, nullptr, 0);
                            }
                        }
                        break;
                    case '`':
                        {
                            int len;
                            bool templatefound;
                            const char* str;
                            if(!this->m_continuetplstring)
                            {
                                this->readChar();
                            }
                            templatefound = false;
                            str = this->scanString('`', true, &templatefound, &len);
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
                                mc_asttoken_init(&outtok, MC_TOK_INVALID, nullptr, 0);
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
                            if(charIsLetter(this->m_ch) || (this->m_ch == '$'))
                            {
                                identlen = 0;
                                ident = this->scanIdent(&identlen);
                                type = this->lookupIdent(ident, identlen);
                                mc_asttoken_init(&outtok, type, ident, identlen);
                                return outtok;
                            }
                            if(charIsDigit(this->m_ch))
                            {
                                numberlen = 0;
                                number = this->scanNumber(&numberlen);
                                mc_asttoken_init(&outtok, MC_TOK_NUMBER, number, numberlen);
                                return outtok;
                            }
                        }
                        break;
                }
                this->readChar();
                if(this->failed())
                {
                    mc_asttoken_init(&outtok, MC_TOK_INVALID, nullptr, 0);
                }
                this->m_continuetplstring = false;
                return outtok;
            }
            /* NB. never reached; but keep the compiler from complaining. */
            return outtok;
        }

        bool expectCurrent(mcasttoktype_t type)
        {
            const char* actualtypestr;
            const char* expectedtypestr;
            if(this->failed())
            {
                return false;
            }
            if(!this->currentTokenIs(type))
            {
                expectedtypestr = mc_asttoken_typename(type);
                actualtypestr = mc_asttoken_typename(this->m_currtoken.toktype);
                this->m_errors->pushFormat(MC_ERROR_PARSING, this->m_currtoken.pos, "expected token \"%s\", but got \"%s\"", expectedtypestr, actualtypestr);
                return false;
            }
            return true;
        }

        bool readChar()
        {
            bool ok; 
            if(this->m_nextposition >= this->m_inputlength)
            {
                this->m_ch = '\0';
            }
            else
            {
                this->m_ch = this->m_inputsource[this->m_nextposition];
            }
            this->m_position = this->m_nextposition;
            this->m_nextposition++;
            if(this->m_ch == '\n')
            {
                this->m_line++;
                this->m_column = -1;
                ok = this->addLineAt(this->m_nextposition);
                if(!ok)
                {
                    this->m_failed = true;
                    return false;
                }
            }
            else
            {
                this->m_column++;
            }
            return true;
        }

        char peekChar()
        {
            if(this->m_nextposition >= this->m_inputlength)
            {
                return '\0';
            }
            return this->m_inputsource[this->m_nextposition];
        }

        const char* scanIdent(int* outlen)
        {
            int len;
            int position;
            len = 0;
            /*if(this->ch == '$')
            {
                this->readChar();
            }
            */
            position = this->m_position;
            while(charIsDigit(this->m_ch) || charIsLetter(this->m_ch) ||(this->m_ch == '$') || (this->m_ch == ':'))
            {
                if(this->m_ch == ':')
                {
                    if(this->peekChar() != ':')
                    {
                        goto end;
                    }
                    this->readChar();
                }
                this->readChar();
            }
        end:
            len = this->m_position - position;
            *outlen = len;
            return this->m_inputsource + position;
        }

        const char* scanNumber(int* outlen)
        {
            int len;
            int position;
            static const char allowed[] = ".xXaAbBcCdDeEfF";
            position = this->m_position;
            while(charIsDigit(this->m_ch) || charIsOneOf(this->m_ch, allowed, MC_UTIL_STATICARRAYSIZE(allowed) - 1))
            {
                this->readChar();
            }
            len = this->m_position - position;
            *outlen = len;
            return this->m_inputsource + position;
        }

        const char* scanString(char delimiter, bool istemplate, bool* outtemplatefound, int* outlen)
        {
            bool escaped;
            int len;
            int position;
            *outlen = 0;
            escaped = false;
            position = this->m_position;
            while(true)
            {
                if(this->m_ch == '\0')
                {
                    return nullptr;
                }
                if(this->m_ch == delimiter && !escaped)
                {
                    break;
                }
                if(istemplate && !escaped && this->m_ch == '$' && this->peekChar() == '{')
                {
                    *outtemplatefound = true;
                    break;
                }
                escaped = false;
                if(this->m_ch == '\\')
                {
                    escaped = true;
                }
                this->readChar();
            }
            len = this->m_position - position;
            *outlen = len;
            return this->m_inputsource + position;
        }

        mcasttoktype_t lookupIdent(const char* ident, int len)
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
                { nullptr, (mcasttoktype_t)0}
            };
            for(i = 0; keywords[i].value != nullptr; i++)
            {
                klen = mc_util_strlen(keywords[i].value);
                if(klen == len && mc_util_strnequal(ident, keywords[i].value, len))
                {
                    return keywords[i].type;
                }
            }
            return MC_TOK_IDENT;
        }

        void skipSpace()
        {
            char ch;
            ch = this->m_ch;
            while(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
            {
                this->readChar();
                ch = this->m_ch;
            }
        }

        bool addLineAt(int offset)
        {
            bool ok;
            size_t linelen;
            char* line;
            const char* linestart;
            const char* newlineptr;
            if(!this->m_file)
            {
                return true;
            }
            if(this->m_line < this->m_file->lines->count())
            {
                return true;
            }
            linestart = this->m_inputsource + offset;
            newlineptr = strchr(linestart, '\n');
            line = nullptr;
            if(!newlineptr)
            {
                line = mc_util_strdup(linestart);
            }
            else
            {
                linelen = newlineptr - linestart;
                line = mc_util_strndup(linestart, linelen);
            }
            if(!line)
            {
                this->m_failed = true;
                return false;
            }
            ok = this->m_file->lines->push(line);
            if(!ok)
            {
                this->m_failed = true;
                mc_memory_free(line);
                return false;
            }
            return true;
        }


};

struct mcastparser_t
{
    mcstate_t* pstate;
    mcconfig_t* config;
    AstLexer lexer;
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
    PtrList* tbitems;
};

struct mcconfig_t
{
    bool dumpast;
    bool dumpbytecode;
    bool printinstructions;
    bool fatalcomplaints;
    bool exitaftercompiling;
    bool strictmode;
    /* allows redefinition of symbols */
    bool replmode;
};

struct mcexecstate_t
{
    size_t vsposition;
    mcvmframe_t* currframe;
    GenericList<mcvalue_t>* valuestack;
    GenericList<mcvalue_t>* valthisstack;
    GenericList<mcvalue_t>* nativethisstack;
    size_t thisstpos;
    GenericList<mcvmframe_t>* framestack;
    mcvalue_t lastpopped;

};

struct mcstate_t
{
    mcconfig_t config;
    mcerrlist_t errors;
    mcgcmemory_t* mem;
    mcglobalstore_t* vmglobalstore;
    GenericList<mcvalue_t>* globalvalstack;
    size_t globalvalcount;
    bool hadrecovered;
    bool running;
    mcvalue_t operoverloadkeys[MC_CONF_MAXOPEROVERLOADS];
    PtrList* files;
    mcastcompiler_t* compiler;
    mcexecstate_t execstate;
    Printer* stdoutprinter;
    Printer* stderrprinter;
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
    PtrDict* storedsymbols;
    GenericList<mcvalue_t>* storedobjects;
};

struct mcprintconfig_t
{
    bool verbosefunc;
    bool quotstring;
    bool shouldflush;
};

class Printer
{
    public:
        static Printer* make(mcstate_t* state, FILE* ofh)
        {
            Printer* pr;
            pr = Memory::make<Printer>();
            if(pr == nullptr)
            {
                return nullptr;
            }
            if(!Printer::initFromStack(pr, state, ofh, false))
            {
                return nullptr;
            }
            return pr;
        }

        static void destroy(Printer* pr)
        {
            releaseFromPtr(pr, true);
            if(!pr->m_prisstack)
            {
                mc_memory_free(pr);
            }
        }

        static bool initFromStack(Printer* pr, mcstate_t* state, FILE* ofh, bool onstack)
        {
            memset(pr, 0, sizeof(Printer));
            pr->m_prpstate = state;
            pr->m_prfailed = false;
            pr->m_prdestrfile = ofh;
            pr->m_prisstack = onstack;
            pr->m_prdestbuf = nullptr;
            pr->m_prconfig.verbosefunc = true;
            pr->m_prconfig.quotstring = false;
            pr->m_prconfig.shouldflush = false;
            if(pr->m_prdestrfile == nullptr)
            {
                pr->m_prdestbuf = Memory::make<StringBuffer>(0);
            }
            return true;
        }

        static void releaseFromPtr(Printer* pr, bool took)
        {
            if(pr != nullptr)
            {
                if(pr->m_prdestbuf != nullptr)
                {
                    if(!took)
                    {
                        StringBuffer::destroy(pr->m_prdestbuf);
                    }
                }
            }
        }


    public:
        mcstate_t* m_prpstate;
        mcprintconfig_t m_prconfig;
        bool m_prfailed;
        bool m_prisstack;
        FILE* m_prdestrfile;
        StringBuffer* m_prdestbuf;

    public:
        inline bool put(const char* str, size_t len)
        {
            if(m_prfailed)
            {
                return false;
            }
            if(len == 0)
            {
                return true;
            }
            if(m_prdestrfile == nullptr)
            {
                m_prdestbuf->append(str, len);
            }
            else
            {
                fwrite(str, sizeof(char), len, m_prdestrfile);
                if(m_prconfig.shouldflush)
                {
                    fflush(m_prdestrfile);
                }
            }
            return true;
        }

        inline bool put(const char* str)
        {
            return this->put(str, mc_util_strlen(str));
        }

        inline bool putChar(int b)
        {
            char ch;
            ch = b;
            return this->put(&ch, 1);
        }

        template<typename... ArgsT>
        inline bool format(const char* fmt, ArgsT&&... args)
        {
            static auto tmprintf = fprintf;
            if(m_prfailed)
            {
                return false;
            }
            if(m_prdestrfile == nullptr)
            {
                m_prdestbuf->appendFormat(fmt, args...);
            }
            else
            {
                tmprintf(m_prdestrfile, fmt, args...);
                if(m_prconfig.shouldflush)
                {
                    fflush(m_prdestrfile);
                }
            }
            return true;
        }

        inline void printEscapedChar(int ch)
        {
            switch(ch)
            {
                case '\'':
                    {
                        this->put("\\\'");
                    }
                    break;
                case '\"':
                    {
                        this->put("\\\"");
                    }
                    break;
                case '\\':
                    {
                        this->put("\\\\");
                    }
                    break;
                case '\b':
                    {
                        this->put("\\b");
                    }
                    break;
                case '\f':
                    {
                        this->put("\\f");
                    }
                    break;
                case '\n':
                    {
                        this->put("\\n");
                    }
                    break;
                case '\r':
                    {
                        this->put("\\r");
                    }
                    break;
                case '\t':
                    {
                        this->put("\\t");
                    }
                    break;
                case 0:
                    {
                        this->put("\\0");
                    }
                    break;
                default:
                    {
                        this->format("\\x%02x", (unsigned char)ch);
                    }
                    break;
            }
        }

        inline void printEscapedString(const char* str, size_t len)
        {
            int ch;
            size_t i;
            this->put("\"");
            for(i=0; i<len; i++)
            {
                ch = str[i];
                if((ch < 32) || (ch > 127) || (ch == '\"') || (ch == '\\'))
                {
                    this->printEscapedChar(ch);
                }
                else
                {
                    this->putChar(ch);
                }
            }
            this->put("\"");
        }

        inline const char* getString()
        {
            if(m_prfailed)
            {
                return nullptr;
            }
            if(m_prdestrfile != nullptr)
            {
                return nullptr;
            }
            return m_prdestbuf->data();
        }

        inline size_t getLength()
        {
            if(m_prfailed)
            {
                return 0;
            }
            if(m_prdestrfile != nullptr)
            {
                return 0;
            }
            return m_prdestbuf->length();
        }

        inline char* getStringAndDestroy(size_t* lendest)
        {
            char* res;
            if(m_prfailed)
            {
                Printer::destroy(this);
                return nullptr;
            }
            if(m_prdestrfile != nullptr)
            {
                return nullptr;
            }
            res = m_prdestbuf->data();
            if(lendest != nullptr)
            {
                *lendest = m_prdestbuf->length();
            }
            m_prdestbuf = nullptr;
            Printer::destroy(this);
            return res;
        }

        inline void printNumFloat(mcfloat_t flt)
        {
            int64_t inum;
            inum = (int64_t)flt;
            if(flt == inum)
            {
                #if 1
                    this->format("%" PRIiFAST64 "", inum);
                #else
                    this->format("%ld", inum);
                #endif
            }
            else
            {
                this->format("%g", flt);
            }
        }
};

struct mcastprinter_t
{
    mcstate_t* pstate;
    Printer* pdest;
    bool pseudolisp;
};

struct mcmodule_t
{
    mcstate_t* pstate;
    char* name;
    PtrList* modsymbols;
};

struct mcastscopefile_t
{
    mcstate_t* pstate;
    mcastparser_t* parser;
    mcastsymtable_t* filesymtab;
    mcastcompiledfile_t* file;
    PtrList* loadedmodnames;
};

struct mcastcompiler_t
{
    mcstate_t* pstate;
    mcconfig_t* config;
    mcgcmemory_t* mem;
    mcerrlist_t* errors;
    PtrList* files;
    mcglobalstore_t* compglobalstore;
    GenericList<mcvalue_t>* constants;
    mcastscopecomp_t* compilationscope;
    PtrList* filescopelist;
    PtrList* srcposstack;
    PtrDict* modules;
    PtrDict* stringconstposdict;
};

/* endheader */
const mcastlocation_t srcposinvalid = { nullptr, -1, -1 };

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
        return nullptr;
    }
    if((rawtold = ftell(hnd)) == -1)
    {
        return nullptr;
    }
    toldlen = rawtold;
    if(fseek(hnd, 0, SEEK_SET) == -1)
    {
        return nullptr;
    }
    buf = (char*)mc_memory_malloc(toldlen + 1);
    memset(buf, 0, toldlen+1);
    if(buf != nullptr)
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
        if(dlen != nullptr)
        {
            *dlen = actuallen;
        }
        return buf;
    }
    return nullptr;
}

char* mc_util_readfile(const char* filename, size_t* dlen)
{
    char* b;
    FILE* fh;
    if((fh = fopen(filename, "rb")) == nullptr)
    {
        return nullptr;
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

size_t mc_util_strlen(const char* str)
{
    size_t len;
    if(str == nullptr)
    {
        return 0;
    }
    for(len=0; str[len] != 0; len++)
    {
    }
    return len;
}

char* mc_util_strndup(const char* string, size_t n)
{
    char* outputstring;
    outputstring = (char*)mc_memory_malloc(n + 1);
    if(!outputstring)
    {
        return nullptr;
    }
    outputstring[n] = '\0';
    memcpy(outputstring, string, n);
    return outputstring;
}

char* mc_util_strdup(const char* string)
{
    if(!string)
    {
        return nullptr;
    }
    return mc_util_strndup(string, mc_util_strlen(string));
}


bool mc_util_strequal(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
}

bool mc_util_strnequal(const char* a, const char* b, size_t len)
{
    return strncmp(a, b, len) == 0;
}


char* mc_util_canonpath(mcstate_t* state, const char* path)
{
    size_t i;
    char* joined;
    char* stritem;
    char* nextitem;
    void* item;
    const char* tmpstr;
    PtrList* split;
    if(!strchr(path, '/') || (!strstr(path, "/../") && !strstr(path, "./")))
    {
        return mc_util_strdup(path);
    }
    split = mc_util_splitstring(path, "/");
    if(!split)
    {
        return nullptr;
    }
    for(i = 0; i < split->count() - 1; i++)
    {
        stritem = (char*)split->get(i);
        nextitem = (char*)split->get(i + 1);
        if(mc_util_strequal(stritem, "."))
        {
            mc_memory_free(stritem);
            split->removeAt(i);
            i = -1;
            continue;
        }
        if(mc_util_strequal(nextitem, ".."))
        {
            mc_memory_free(stritem);
            mc_memory_free(nextitem);
            split->removeAt(i);
            split->removeAt(i);
            i = -1;
        }
    }
    tmpstr = "/";
    joined = mc_util_joinstringarray(state, split, tmpstr, strlen(tmpstr));
    for(i = 0; i < split->count(); i++)
    {
        item = split->get(i);
        mc_memory_free(item);
    }
    PtrList::destroy(split, nullptr);
    return joined;
}

bool mc_util_pathisabsolute(const char* path)
{
    return path[0] == '/';
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
    if(endptr != nullptr)
    {
        *endptr = (char*)end;
    }
    return stod_strtod(&p, end, true);
}

PtrList* mc_util_splitstring(const char* str, const char* delimiter)
{
    bool ok;
    size_t i;
    long len;
    char* rest;
    char* line;
    const char* lineend;
    const char* linestart;
    PtrList* res;
    res = Memory::make<PtrList>(sizeof(void*), true);
    rest = nullptr;
    if(!str)
    {
        return res;
    }
    linestart = str;
    lineend = strstr(linestart, delimiter);
    while(lineend != nullptr)
    {
        len = lineend - linestart;
        line = mc_util_strndup(linestart, len);
        if(!line)
        {
            goto err;
        }
        ok = res->push(line);
        if(!ok)
        {
            mc_memory_free(line);
            goto err;
        }
        linestart = lineend + 1;
        lineend = strstr(linestart, delimiter);
    }
    rest = mc_util_strdup(linestart);
    if(!rest)
    {
        goto err;
    }
    ok = res->push(rest);
    if(!ok)
    {
        goto err;
    }
    return res;
err:
    mc_memory_free(rest);
    if(res)
    {
        for(i = 0; i < res->count(); i++)
        {
            line = (char*)res->get(i);
            mc_memory_free(line);
        }
    }
    PtrList::destroy(res, nullptr);
    return nullptr;
}

char* mc_util_joinstringarray(mcstate_t* state, PtrList* items, const char* joinee, size_t jlen)
{
    size_t i;
    char* item;
    Printer* res;
    res = Printer::make(state, nullptr);
    if(!res)
    {
        return nullptr;
    }
    for(i = 0; i < items->count(); i++)
    {
        item = (char*)items->get(i);
        res->put(item);
        if(i < (items->count() - 1))
        {
            res->put(joinee, jlen);
        }
    }
    return res->getStringAndDestroy(nullptr);
}

template<typename... ArgsT>
char* mc_util_stringallocfmt(const char* format, ArgsT&&... args)
{
    int needsz;
    int printedsz;
    char* res;
    (void)printedsz;
    needsz = snprintf(nullptr, 0, format, args...);
    res = (char*)mc_memory_malloc(needsz + 1);
    if(!res)
    {
        return nullptr;
    }
    printedsz = sprintf(res, format, args...);
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

template<typename... ArgsT>
void mc_util_complain(mcastlocation_t pos, const char *fmt, ArgsT&&... args)
{
    static auto tmpfprintf = fprintf;
    int ncol;
    int nline;
    const char* fname;
    fname = "unknown";
    ncol = 0;
    nline = 0;
    if(pos.file != nullptr)
    {
        if(pos.file->path != nullptr)
        {
            fname = pos.file->path;
        }
        nline = pos.line;
        ncol = pos.column;
    }
    fprintf(stderr, "**WARNING** [%s:%d:%d] ", fname, nline, ncol);
    tmpfprintf(stderr, fmt, args...);
    fprintf(stderr, "\n");
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

#define mc_mathutil_add(dnleft, dnright) \
    ((dnleft) + dnright)

#define mc_mathutil_sub(dnleft, dnright) \
    ((dnleft) - (dnright))

#define mc_mathutil_mult(dnleft, dnright) \
    ((dnleft) * (dnright))

#define mc_mathutil_div(dnleft, dnright) \
    ((dnleft) / (dnright))

#define mc_mathutil_mod(dnleft, dnright) \
    (fmod((dnleft), (dnright)))


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


const char* mc_value_objtypename(mcvaltype_t type)
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
            return "nullptr";
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

MC_FORCEINLINE bool mc_value_ismap(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return (type == MC_VAL_MAP);
}

MC_FORCEINLINE bool mc_value_isarray(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return (type == MC_VAL_ARRAY);
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
    data->uvobj.valstring.strbuf = Memory::make<StringBuffer>(capacity);
    return mc_object_makedatafrom(MC_VAL_STRING, data);
}

template<typename... ArgsT>
mcvalue_t mc_value_makestrformat(mcstate_t* state, const char* fmt, ArgsT&&... args)
{
    mcvalue_t res;
    mcobjdata_t* data;
    data = mc_gcmemory_getdatafrompool(state, MC_VAL_STRING);
    res = mc_value_makestrcapacity(state, 0);
    if(mc_value_isnull(res))
    {
        return mc_value_makenull();
    }
    data->uvobj.valstring.strbuf->appendFormat(fmt, args...);
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
    ok = mc_value_stringappendlen(res, string, len);
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
    obj->uvobj.valfunc.funcdata.valnativefunc.natfnname = mc_util_strdup(name);
    if(!obj->uvobj.valfunc.funcdata.valnativefunc.natfnname)
    {
        return mc_value_makenull();
    }
    obj->uvobj.valfunc.funcdata.valnativefunc.natptrfn = fn;
    if(data)
    {
        obj->uvobj.valfunc.funcdata.valnativefunc.userpointer = data;
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
        data->uvobj.valarray->actualarray->setEmpty();
        return mc_object_makedatafrom(MC_VAL_ARRAY, data);
    }
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->uvobj.valarray = Memory::make<mcobjarray_t>();
    data->uvobj.valarray->actualarray = Memory::make<GenericList<mcvalue_t>>(capacity, mc_value_makenull());
    if(!data->uvobj.valarray->actualarray)
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
        data->uvobj.valmap->actualmap->clear();
        return mc_object_makedatafrom(MC_VAL_MAP, data);
    }
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->uvobj.valmap = Memory::make<mcobjmap_t>();
    data->uvobj.valmap->actualmap = Memory::make<GenericDict<mcvalue_t, mcvalue_t>>(capacity);
    if(!data->uvobj.valmap->actualmap)
    {
        return mc_value_makenull();
    }
    data->uvobj.valmap->actualmap->setHashFunction((mcitemhashfn_t)mc_value_callbackhash);
    data->uvobj.valmap->actualmap->setEqualsFunction((mcitemcomparefn_t)mc_value_callbackequals);
    return mc_object_makedatafrom(MC_VAL_MAP, data);
}

mcvalue_t mc_value_makeerror(mcstate_t* state, const char* error)
{
    char* errorstr;
    mcvalue_t res;
    errorstr = mc_util_strdup(error);
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
    data->uvobj.valerror.traceback = nullptr;
    return mc_object_makedatafrom(MC_VAL_ERROR, data);
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
        data->uvobj.valfunc.funcdata.valscriptfunc.unamev.fallocname = name ? mc_util_strdup(name) : mc_util_strdup("anonymous");
        if(!data->uvobj.valfunc.funcdata.valscriptfunc.unamev.fallocname)
        {
            return mc_value_makenull();
        }
    }
    else
    {
        data->uvobj.valfunc.funcdata.valscriptfunc.unamev.fconstname = name ? name : "anonymous";
    }
    data->uvobj.valfunc.funcdata.valscriptfunc.compiledprogcode = cres;
    data->uvobj.valfunc.funcdata.valscriptfunc.ownsdata = ownsdt;
    data->uvobj.valfunc.funcdata.valscriptfunc.numlocals = nlocals;
    data->uvobj.valfunc.funcdata.valscriptfunc.numargs = nargs;
    if(fvc >= MC_UTIL_STATICARRAYSIZE(data->uvobj.valfunc.funcdata.valscriptfunc.ufv.freevalsstack))
    {
        data->uvobj.valfunc.funcdata.valscriptfunc.ufv.freevalsallocated = (mcvalue_t*)mc_memory_malloc(sizeof(mcvalue_t) * fvc);
        if(!data->uvobj.valfunc.funcdata.valscriptfunc.ufv.freevalsallocated)
        {
            return mc_value_makenull();
        }
    }
    data->uvobj.valfunc.funcdata.valscriptfunc.freevalscount = fvc;
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
    obj->uvobj.valuserobject.datadestroyfn = nullptr;
    obj->uvobj.valuserobject.datacopyfn = nullptr;
    return mc_object_makedatafrom(MC_VAL_EXTERNAL, obj);
}

GenericList<mcvalue_t>* mc_value_arraygetactualarray(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valarray->actualarray;
}

MC_INLINE char* mc_value_stringgetdataintern(mcvalue_t object)
{
    mcobjdata_t* data;
    data = mc_value_getallocateddata(object);
    MC_ASSERT(data->odtype == MC_VAL_STRING);
    return data->uvobj.valstring.strbuf->data();
}

mcobjuserdata_t* mc_value_userdatagetdata(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_EXTERNAL);
    data = mc_value_getallocateddata(object);
    return &data->uvobj.valuserobject;
}

bool mc_value_userdatasetdata(mcvalue_t object, void* extdata)
{
    mcobjuserdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_EXTERNAL);
    data = mc_value_userdatagetdata(object);
    if(!data)
    {
        return false;
    }
    data->data = extdata;
    return true;
}

bool mc_value_userdatasetdestroyfunction(mcvalue_t object, mcitemdestroyfn_t dfn)
{
    mcobjuserdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_EXTERNAL);
    data = mc_value_userdatagetdata(object);
    if(!data)
    {
        return false;
    }
    data->datadestroyfn = dfn;
    return true;
}

bool mc_value_userdatasetcopyfunction(mcvalue_t object, mcitemcopyfn_t copyfn)
{
    mcobjuserdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_EXTERNAL);
    data = mc_value_userdatagetdata(object);
    if(!data)
    {
        return false;
    }
    data->datacopyfn = copyfn;
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

MC_INLINE const char* mc_value_stringgetdata(mcvalue_t object)
{
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    return mc_value_stringgetdataintern(object);
}

int mc_value_stringgetlength(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valstring.strbuf->length();
}

void mc_value_stringsetlength(mcvalue_t object, int len)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    data = mc_value_getallocateddata(object);
    data->uvobj.valstring.strbuf->setLength(len);
    mc_value_stringrehash(object);
}


MC_INLINE char* mc_value_stringgetmutabledata(mcvalue_t object)
{
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    return mc_value_stringgetdataintern(object);
}

bool mc_value_stringappendlen(mcvalue_t obj, const char* src, size_t len)
{
    mcobjdata_t* data;
    mcobjstring_t* string;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_STRING);
    data = mc_value_getallocateddata(obj);
    string = &data->uvobj.valstring;
    string->strbuf->append(src, len);
    mc_value_stringrehash(obj);
    return true;
}

bool mc_value_stringappend(mcvalue_t obj, const char* src)
{
    return mc_value_stringappendlen(obj, src, strlen(src));
}

template<typename... ArgsT>
bool mc_value_stringappendformat(mcvalue_t obj, const char* fmt, ArgsT&&... args)
{
    mcobjdata_t* data;
    mcobjstring_t* string;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_STRING);
    data = mc_value_getallocateddata(obj);
    string = &data->uvobj.valstring;
    string->strbuf->appendFormat(fmt, args...);
    mc_value_stringrehash(obj);
    return true;
}


bool mc_value_stringappendvalue(mcvalue_t destval, mcvalue_t val)
{
    bool ok;
    int vlen;
    const char* vstr;
    if(mc_value_gettype(val) == MC_VAL_NUMBER)
    {
        mc_value_stringappendformat(destval, "%g", mc_value_asnumber(val));
        goto finished;
    }
    if(mc_value_gettype(val) == MC_VAL_STRING)
    {
        vlen = mc_value_stringgetlength(val);
        vstr = mc_value_stringgetdata(val);
        ok = mc_value_stringappendlen(destval, vstr, vlen);
        if(!ok)
        {
            return false;
        }
        goto finished;
    }
    return false;
    finished:
    mc_value_stringrehash(destval);
    return true;
}

size_t mc_value_stringgethash(mcvalue_t obj)
{
    size_t len;
    const char* str;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_STRING);
    data = mc_value_getallocateddata(obj);
    if(data->uvobj.valstring.hash == 0)
    {
        len = mc_value_stringgetlength(obj);
        str = mc_value_stringgetdata(obj);
        data->uvobj.valstring.hash = mc_util_hashdata(str, len);
    }
    return data->uvobj.valstring.hash;
}

bool mc_value_stringrehash(mcvalue_t obj)
{
    size_t len;
    const char* str;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_STRING);
    data = mc_value_getallocateddata(obj);
    len = mc_value_stringgetlength(obj);
    str = mc_value_stringgetdata(obj);
    data->uvobj.valstring.hash = mc_util_hashdata(str, len);
    return true;
}

mcobjfunction_t* mc_value_asscriptfunction(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(object);
    return &data->uvobj.valfunc;
}

MC_INLINE mcobjfunction_t* mc_value_asnativefunction(mcvalue_t obj)
{
    mcobjdata_t* data = mc_value_getallocateddata(obj);
    return &data->uvobj.valfunc;
}

const char* mc_value_functiongetname(mcvalue_t obj)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return nullptr;
    }
    if(data->uvobj.valfunc.funcdata.valscriptfunc.ownsdata)
    {
        return data->uvobj.valfunc.funcdata.valscriptfunc.unamev.fallocname;
    }
    return data->uvobj.valfunc.funcdata.valscriptfunc.unamev.fconstname;
}

mcvalue_t mc_value_functiongetfreevalat(mcvalue_t obj, int ix)
{
    mcobjdata_t* data;
    mcobjfunction_t* fun;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return mc_value_makenull();
    }
    fun = &data->uvobj.valfunc;
    MC_ASSERT(ix >= 0 && ix < fun->funcdata.valscriptfunc.freevalscount);
    if(ix < 0 || ix >= fun->funcdata.valscriptfunc.freevalscount)
    {
        return mc_value_makenull();
    }
    if(fun->freeValuesAreAllocated())
    {
        return fun->funcdata.valscriptfunc.ufv.freevalsallocated[ix];
    }
    return fun->funcdata.valscriptfunc.ufv.freevalsstack[ix];
}

void mc_value_functionsetfreevalat(mcvalue_t obj, int ix, mcvalue_t val)
{
    mcobjdata_t* data;
    mcobjfunction_t* fun;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(obj);
    MC_ASSERT(data);
    if(data != nullptr)
    {
        fun = &data->uvobj.valfunc;
        MC_ASSERT(ix >= 0 && ix < fun->funcdata.valscriptfunc.freevalscount);
        if(ix < 0 || ix >= fun->funcdata.valscriptfunc.freevalscount)
        {
        }
        else
        {
            if(fun->freeValuesAreAllocated())
            {
                fun->funcdata.valscriptfunc.ufv.freevalsallocated[ix] = val;
            }
            else
            {
                fun->funcdata.valscriptfunc.ufv.freevalsstack[ix] = val;
            }
        }
    }
}

mcvalue_t* mc_value_functiongetfreevals(mcvalue_t obj)
{
    mcobjdata_t* data;
    mcobjfunction_t* fun;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return nullptr;
    }
    fun = &data->uvobj.valfunc;
    if(fun->freeValuesAreAllocated())
    {
        return fun->funcdata.valscriptfunc.ufv.freevalsallocated;
    }
    return fun->funcdata.valscriptfunc.ufv.freevalsstack;
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
    if(mc_value_gettype(object) == MC_VAL_ERROR)
    {
        data = mc_value_getallocateddata(object);
        MC_ASSERT(data->uvobj.valerror.traceback == nullptr);
        data->uvobj.valerror.traceback = traceback;
    }
}

mctraceback_t* mc_value_errorgettraceback(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ERROR);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valerror.traceback;
}

mcvalue_t mc_value_arraygetvalue(mcvalue_t object, size_t ix)
{
    mcvalue_t* res;
    GenericList<mcvalue_t>* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    if(ix >= array->count())
    {
        return mc_value_makenull();
    }
    res = (mcvalue_t*)array->getp(ix);
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
    GenericList<mcvalue_t>* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    len = array->count();
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
    return array->set(ix, val);
}

bool mc_value_arraypush(mcvalue_t object, mcvalue_t val)
{
    GenericList<mcvalue_t>* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    return array->push(val);
}

int mc_value_arraygetlength(mcvalue_t object)
{
    GenericList<mcvalue_t>* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    return array->count();
}

mcvalue_t mc_valarray_pop(mcvalue_t object)
{
    mcvalue_t dest;
    GenericList<mcvalue_t>* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    if(array->pop(&dest))
    {
        return dest;
    }
    return mc_value_makenull();
}

bool mc_value_arrayremoveat(mcvalue_t object, int ix)
{
    GenericList<mcvalue_t>* array;
    array = mc_value_arraygetactualarray(object);
    return array->removeAt(ix);
}

int mc_value_mapgetlength(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valmap->actualmap->count();
}

mcvalue_t mc_value_mapgetkeyat(mcvalue_t object, int ix)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)data->uvobj.valmap->actualmap->getKeyAt(ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

mcvalue_t mc_value_mapgetvalueat(mcvalue_t object, int ix)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)data->uvobj.valmap->actualmap->getValueAt(ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

bool mc_value_mapsetvalueat(mcvalue_t object, int ix, mcvalue_t val)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    if(ix >= mc_value_mapgetlength(object))
    {
        return false;
    }
    data = mc_value_getallocateddata(object);
    return data->uvobj.valmap->actualmap->setValueAt(ix, &val);
}

mcvalue_t mc_value_mapgetkvpairat(mcstate_t* state, mcvalue_t object, int ix)
{
    mcvalue_t key;
    mcvalue_t val;
    mcvalue_t res;
    mcvalue_t valobj;
    mcvalue_t keyobj;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    if(ix >= data->uvobj.valmap->actualmap->count())
    {
        return mc_value_makenull();
    }
    key = mc_value_mapgetkeyat(object, ix);
    val = mc_value_mapgetvalueat(object, ix);
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
    mc_value_mapsetvalue(res, keyobj, key);
    valobj = mc_value_makestring(state, "value");
    if(mc_value_isnull(valobj))
    {
        return mc_value_makenull();
    }
    mc_value_mapsetvalue(res, valobj, val);
    return res;
}

bool mc_value_mapsetvalue(mcvalue_t object, mcvalue_t key, mcvalue_t val)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valmap->actualmap->setKV(&key, &val);
}

bool mc_value_mapsetvaluestring(mcvalue_t object, const char* strkey, mcvalue_t val)
{
    mcstate_t* state;
    mcvalue_t vkey;
    state = mc_value_getallocateddata(object)->pstate;
    vkey = mc_value_makestring(state, strkey);
    return mc_value_mapsetvalue(object, vkey, val);
}

mcvalue_t mc_value_mapgetvalue(mcvalue_t object, mcvalue_t key)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)data->uvobj.valmap->actualmap->get(&key);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

bool mc_value_mapgetvaluechecked(mcvalue_t object, mcvalue_t key, mcvalue_t* dest)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)data->uvobj.valmap->actualmap->get(&key);
    if(!res)
    {
        *dest = mc_value_makenull();
        return false;
    }
    *dest = *res;
    return true;
}

bool mc_value_maphaskey(mcvalue_t object, mcvalue_t key)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)data->uvobj.valmap->actualmap->get(&key);
    return res != nullptr;
}

void mc_objectdata_deinit(mcobjdata_t* data)
{
    switch(data->odtype)
    {
        case MC_VAL_FREED:
            {
                MC_ASSERT(false);
            }
            break;
        case MC_VAL_STRING:
            {
                StringBuffer::destroy(data->uvobj.valstring.strbuf);
            }
            break;
        case MC_VAL_FUNCSCRIPT:
            {
                if(data->uvobj.valfunc.funcdata.valscriptfunc.ownsdata)
                {
                    mc_memory_free(data->uvobj.valfunc.funcdata.valscriptfunc.unamev.fallocname);
                    mc_astcompresult_destroy(data->uvobj.valfunc.funcdata.valscriptfunc.compiledprogcode);
                }
                if(data->uvobj.valfunc.freeValuesAreAllocated())
                {
                    mc_memory_free(data->uvobj.valfunc.funcdata.valscriptfunc.ufv.freevalsallocated);
                }
            }
            break;
        case MC_VAL_ARRAY:
            {
                Memory::destroy(data->uvobj.valarray->actualarray);
                Memory::destroy(data->uvobj.valarray);
            }
            break;
        case MC_VAL_MAP:
            {
                Memory::destroy(data->uvobj.valmap->actualmap);
                Memory::destroy(data->uvobj.valmap);
            }
            break;
        case MC_VAL_FUNCNATIVE:
            {
                mc_memory_free(data->uvobj.valfunc.funcdata.valnativefunc.natfnname);
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
        alen = mc_value_stringgetlength(a);
        blen = mc_value_stringgetlength(b);
        #if 0
        fprintf(stderr, "mc_value_compare: alen=%d, blen=%d\n", alen, blen);
        #endif
        if(alen != blen)
        {
            cres->result = alen - blen;
            return false;
        }
        ahash = mc_value_stringgethash(a);
        bhash = mc_value_stringgethash(b);
        if(ahash != bhash)
        {
            cres->result = ahash - bhash;
            return false;
        }
        astring = mc_value_stringgetdata(a);
        bstring = mc_value_stringgetdata(b);
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

template<typename TypeKeyT, typename TypeValueT>
mcvalue_t mc_value_copydeepfuncscript(mcstate_t* state, mcvalue_t obj, GenericDict<TypeKeyT, TypeValueT>* targetdict)
{
    bool ok;
    int i;
    uint16_t* bytecodecopy;
    mcobjfunction_t* functioncopy;
    mcvalue_t copy;
    mcvalue_t freeval;
    mcvalue_t freevalcopy;
    mcobjfunction_t* function;
    mcastlocation_t* srcpositionscopy;
    mccompiledprogram_t* comprescopy;
    function = mc_value_asscriptfunction(obj);
    bytecodecopy = nullptr;
    srcpositionscopy = nullptr;
    comprescopy = nullptr;
    bytecodecopy = (uint16_t*)mc_memory_malloc(sizeof(uint16_t) * function->funcdata.valscriptfunc.compiledprogcode->count);
    if(!bytecodecopy)
    {
        return mc_value_makenull();
    }
    memcpy(bytecodecopy, function->funcdata.valscriptfunc.compiledprogcode->bytecode, sizeof(uint16_t) * function->funcdata.valscriptfunc.compiledprogcode->count);
    srcpositionscopy = (mcastlocation_t*)mc_memory_malloc(sizeof(mcastlocation_t) * function->funcdata.valscriptfunc.compiledprogcode->count);
    if(!srcpositionscopy)
    {
        mc_memory_free(bytecodecopy);
        return mc_value_makenull();
    }
    memcpy(srcpositionscopy, function->funcdata.valscriptfunc.compiledprogcode->progsrcposlist, sizeof(mcastlocation_t) * function->funcdata.valscriptfunc.compiledprogcode->count);
    comprescopy = mc_astcompresult_make(state, bytecodecopy, srcpositionscopy, function->funcdata.valscriptfunc.compiledprogcode->count);
    /*
    * todo: add compilation result copy function
    */
    if(!comprescopy)
    {
        mc_memory_free(srcpositionscopy);
        mc_memory_free(bytecodecopy);
        return mc_value_makenull();
    }
    copy = mc_value_makefuncscript(state, mc_value_functiongetname(obj), comprescopy, true, function->funcdata.valscriptfunc.numlocals, function->funcdata.valscriptfunc.numargs, 0);
    if(mc_value_isnull(copy))
    {
        mc_astcompresult_destroy(comprescopy);
        return mc_value_makenull();
    }
    ok = targetdict->setKV(&obj, &copy);
    if(!ok)
    {
        return mc_value_makenull();
    }
    functioncopy = mc_value_asscriptfunction(copy);
    if(function->freeValuesAreAllocated())
    {
        functioncopy->funcdata.valscriptfunc.ufv.freevalsallocated = (mcvalue_t*)mc_memory_malloc(sizeof(mcvalue_t) * function->funcdata.valscriptfunc.freevalscount);
        if(!functioncopy->funcdata.valscriptfunc.ufv.freevalsallocated)
        {
            return mc_value_makenull();
        }
    }
    functioncopy->funcdata.valscriptfunc.freevalscount = function->funcdata.valscriptfunc.freevalscount;
    for(i = 0; i < function->funcdata.valscriptfunc.freevalscount; i++)
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

template<typename TypeKeyT, typename TypeValueT>
mcvalue_t mc_value_copydeeparray(mcstate_t* state, mcvalue_t obj, GenericDict<TypeKeyT, TypeValueT>* targetdict)
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
    ok = targetdict->setKV(&obj, &copy);
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

template<typename TypeKeyT, typename TypeValueT>
mcvalue_t mc_value_copydeepmap(mcstate_t* state, mcvalue_t obj, GenericDict<TypeKeyT, TypeValueT>* targetdict)
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
    ok = targetdict->setKV(&obj, &copy);
    if(!ok)
    {
        return mc_value_makenull();
    }
    for(i = 0; i < mc_value_mapgetlength(obj); i++)
    {
        key = mc_value_mapgetkeyat(obj, i);
        val = mc_value_mapgetvalueat(obj, i);
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
        ok = mc_value_mapsetvalue(copy, keycopy, valcopy);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return copy;
}

template<typename TypeKeyT, typename TypeValueT>
mcvalue_t mc_value_copydeepintern(mcstate_t* state, mcvalue_t obj, GenericDict<TypeKeyT, TypeValueT>* targetdict)
{
    mcvaltype_t type;
    mcvalue_t copy;
    mcvalue_t* copyptr;
    copyptr = (mcvalue_t*)targetdict->get(&obj);
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
                str = mc_value_stringgetdata(obj);
                len = mc_value_stringgetlength(obj);
                copy = mc_value_makestringlen(state, str, len);
                ok = targetdict->setKV(&obj, &copy);
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
    auto targetdict = Memory::make<GenericDict<mcvalue_t, mcvalue_t>>();
    if(!targetdict)
    {
        return mc_value_makenull();
    }
    res = mc_value_copydeepintern(state, obj, targetdict);
    Memory::destroy(targetdict);
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
                str = mc_value_stringgetdata(obj);
                len = mc_value_stringgetlength(obj);
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
                for(i = 0; i < mc_value_mapgetlength(obj); i++)
                {
                    key = mc_value_mapgetkeyat(obj, i);
                    val = mc_value_mapgetvalueat(obj, i);
                    ok = mc_value_mapsetvalue(copy, key, val);
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
                copy = mc_value_makeuserobject(state, nullptr);
                if(mc_value_isnull(copy))
                {
                    return mc_value_makenull();
                }
                objext = mc_value_userdatagetdata(obj);
                datacopy = nullptr;
                if(objext->datacopyfn)
                {
                    datacopy = objext->datacopyfn(objext->data);
                }
                else
                {
                    datacopy = objext->data;
                }
                mc_value_userdatasetdata(copy, datacopy);
                mc_value_userdatasetdestroyfunction(copy, objext->datadestroyfn);
                mc_value_userdatasetcopyfunction(copy, objext->datacopyfn);
            }
            break;
    }
    return copy;
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

void mc_printer_printoneinstruc(Printer* pr, uint16_t* code, uint16_t op, size_t* pos, mcastlocation_t* sposlist, bool simple)
{
    bool ok;
    int i;
    mcfloat_t dval;
    uint64_t operands[2];
    mcopdefinition_t* def;
    mcopdefinition_t vdef;
    mcastlocation_t srcpos;
    def = mc_opdef_lookup(&vdef, op);
    MC_ASSERT(def != nullptr);
    if(sposlist)
    {
        srcpos = sposlist[*pos];
        if(simple)
        {
            pr->put("<");
        }
        pr->format("@%d:%d %04d %s", srcpos.line, srcpos.column, *pos, def->name);
        if(simple)
        {
            pr->put(">");
        }
    }
    else
    {
        pr->format("%04d %s", *pos, def->name);
    }
    (*pos)++;
    ok = mc_printutil_bcreadoperands(def, code + (*pos), operands);
    if(ok)
    {
        for(i = 0; i < def->numoperands; i++)
        {
            if(op == MC_OPCODE_NUMBER)
            {
                dval = mc_util_uint64todouble(operands[i]);
                pr->format(" %1.17g", dval);
            }
            else
            {
                pr->format(" %llu", operands[i]);
            }
            (*pos) += def->operandwidths[i];
        }
        if(simple)
        {
            pr->put(",");
        }
        else
        {
            pr->put("\n");
        }
    }
}

void mc_printer_printbytecode(Printer* pr, uint16_t* code, mcastlocation_t* sposlist, size_t codesize, bool simple)
{
    uint16_t op;
    size_t pos;
    pos = 0;
    while(pos < codesize)
    {
        op = code[pos];
        mc_printer_printoneinstruc(pr, code, op, &pos, sposlist, simple);
    }
}

void mc_printer_printobjstring(Printer* pr, mcvalue_t obj)
{
    size_t len;
    const char* str;
    str = mc_value_stringgetdata(obj);
    len = mc_value_stringgetlength(obj);
    if(pr->m_prconfig.quotstring)
    {
        pr->printEscapedString(str, len);
    }
    else
    {
        pr->put(str, len);
    }
}

void mc_printer_printobjfuncscript(Printer* pr, mcvalue_t obj)
{
    const char* fname;
    mcobjfunction_t* fn;
    fn = mc_value_asscriptfunction(obj);
    fname = mc_value_functiongetname(obj);
    pr->format("<scriptfunction '%s' locals=%d argc=%d fvc=%d", fname, fn->funcdata.valscriptfunc.numlocals, fn->funcdata.valscriptfunc.numargs, fn->funcdata.valscriptfunc.freevalscount);
    #if 0
    if(pr->m_prconfig.verbosefunc)
    {
        pr->put(" [");
        mc_printer_printbytecode(pr, fn->funcdata.valscriptfunc.compiledprogcode->bytecode, fn->funcdata.valscriptfunc.compiledprogcode->progsrcposlist, fn->funcdata.valscriptfunc.compiledprogcode->count, true);
        pr->put(" ]");
    }
    else
    #endif
    {
    }
    pr->put(">");
}

void mc_printer_printobjarray(Printer* pr, mcvalue_t obj)
{
    bool recursion;
    size_t i;
    size_t alen;
    bool prevquot;
    mcvalue_t iobj;
    GenericList<mcvalue_t>* actualary;
    GenericList<mcvalue_t>* otherary;
    actualary = mc_value_arraygetactualarray(obj);
    alen = mc_value_arraygetlength(obj);
    pr->put("[");
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
        prevquot = pr->m_prconfig.quotstring;
        pr->m_prconfig.quotstring = true;
        if(recursion)
        {
            pr->put("<recursion>");
        }
        else
        {
            mc_printer_printvalue(pr, iobj, false);
        }
        pr->m_prconfig.quotstring = prevquot;
        if(i < (alen - 1))
        {
            pr->put(", ");
        }
    }
    pr->put("]");
}

void mc_printer_printobjmap(Printer* pr, mcvalue_t obj)
{
    bool prevquot;
    size_t i;
    size_t alen;
    mcvalue_t key;
    mcvalue_t val;
    alen = mc_value_mapgetlength(obj);
    pr->put("{");
    for(i = 0; i < alen; i++)
    {
        key = mc_value_mapgetkeyat(obj, i);
        val = mc_value_mapgetvalueat(obj, i);
        prevquot = pr->m_prconfig.quotstring;
        pr->m_prconfig.quotstring = true;
        mc_printer_printvalue(pr, key, false);
        pr->put(": ");
        mc_printer_printvalue(pr, val, false);
        pr->m_prconfig.quotstring = prevquot;
        if(i < (alen - 1))
        {
            pr->put(", ");
        }
    }
    pr->put("}");
}


void mc_printer_printobjerror(Printer* pr, mcvalue_t obj)
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

void mc_printer_printvalue(Printer* pr, mcvalue_t obj, bool accurate)
{
    mcvaltype_t type;
    (void)accurate;
    type = mc_value_gettype(obj);
    switch(type)
    {
        case MC_VAL_FREED:
            {
                pr->put("FREED");
            }
            break;
        case MC_VAL_NONE:
            {
                pr->put("NONE");
            }
            break;
        case MC_VAL_NUMBER:
            {
                mcfloat_t number;
                number = mc_value_asnumber(obj);
                pr->printNumFloat(number);
            }
            break;
        case MC_VAL_BOOL:
            {
                pr->put(mc_value_asbool(obj) ? "true" : "false");
            }
            break;
        case MC_VAL_STRING:
            {
                mc_printer_printobjstring(pr, obj);
            }
            break;
        case MC_VAL_NULL:
            {
                pr->put("null");
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
                pr->put("FUNCNATIVE");
            }
            break;
        case MC_VAL_EXTERNAL:
            {
                pr->put("EXTERNAL");
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


mcgcmemory_t* mc_value_getmem(mcvalue_t obj)
{
    mcobjdata_t* data;
    data = mc_value_getallocateddata(obj);
    return data->mem;
}

char* mc_valtype_getunionname(mcstate_t* state, mcvaltype_t type)
{
    bool inbetween;
    Printer* res;
    if(type == MC_VAL_ANY || type == MC_VAL_NONE || type == MC_VAL_FREED)
    {
        return mc_util_strdup(mc_valtype_getname(type));
    }
    res = Printer::make(state, nullptr);
    if(!res)
    {
        return nullptr;
    }
    inbetween = false;
#define CHECK_TYPE(t)                                    \
    do                                                   \
    {                                                    \
        if((type & t) == t)                              \
        {                                                \
            if(inbetween)                               \
            {                                            \
                res->put("|");                 \
            }                                            \
            res->put(mc_valtype_getname(t)); \
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
    return res->getStringAndDestroy(nullptr);
}


mcmodule_t* mc_module_make(mcstate_t* state, const char* name)
{
    mcmodule_t* module;
    module = Memory::make<mcmodule_t>();
    if(!module)
    {
        return nullptr;
    }
    memset(module, 0, sizeof(mcmodule_t));
    module->pstate = state;
    module->name = mc_util_strdup(name);
    if(!module->name)
    {
        mc_module_destroy(module);
        return nullptr;
    }
    module->modsymbols = Memory::make<PtrList>(sizeof(void*), true);
    if(!module->modsymbols)
    {
        mc_module_destroy(module);
        return nullptr;
    }
    return module;
}

void mc_module_destroy(mcmodule_t* module)
{
    if(module != nullptr)
    {
        mc_memory_free(module->name);
        PtrList::destroy(module->modsymbols, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
        mc_memory_free(module);
    }
}

bool mc_module_addsymbol(mcmodule_t* module, mcastsymbol_t* symbol)
{
    bool ok;
    mcastsymbol_t* modulesymbol;
    Printer* namebuf;
    namebuf = Printer::make(module->pstate, nullptr);
    if(!namebuf)
    {
        return false;
    }
    ok = namebuf->format("%s::%s", module->name, symbol->name);
    if(!ok)
    {
        Printer::destroy(namebuf);
        return false;
    }
    modulesymbol = mcastsymbol_t::make(module->pstate, namebuf->getString(), MC_SYM_MODULEGLOBAL, symbol->index, false);
    Printer::destroy(namebuf);
    if(!modulesymbol)
    {
        return false;
    }
    ok = module->modsymbols->push(modulesymbol);
    if(!ok)
    {
        mcastsymbol_t::destroy(modulesymbol);
        return false;
    }
    return true;
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

const char* mc_module_findfile(mcstate_t* state, const char* filename)
{
    (void)state;
    return filename;
}

mcmodule_t* mc_module_copy(mcmodule_t* src)
{
    mcmodule_t* copy;
    copy = Memory::make<mcmodule_t>();
    if(!copy)
    {
        return nullptr;
    }
    memset(copy, 0, sizeof(mcmodule_t));
    copy->pstate = src->pstate;
    copy->name = mc_util_strdup(src->name);
    if(!copy->name)
    {
        mc_module_destroy(copy);
        return nullptr;
    }
    copy->modsymbols = src->modsymbols->copy((mcitemcopyfn_t)mcastsymbol_t::copy, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
    if(!copy->modsymbols)
    {
        mc_module_destroy(copy);
        return nullptr;
    }
    return copy;
}


mcastexpression_t* mc_astexpr_makeident(mcstate_t* state, mcastexprident_t* ident)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_IDENT);
    if(!res)
    {
        return nullptr;
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
        return nullptr;
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
        return nullptr;
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
        return nullptr;
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
        return nullptr;
    }
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralarray(mcstate_t* state, PtrList* values)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_ARRAYLITERAL);
    if(!res)
    {
        return nullptr;
    }
    res->uexpr.exprlitarray.litarritems = values;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralmap(mcstate_t* state, PtrList* keys, PtrList* values)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_MAPLITERAL);
    if(!res)
    {
        return nullptr;
    }
    res->uexpr.exprlitmap.litmapkeys = keys;
    res->uexpr.exprlitmap.litmapvalues = values;
    return res;
}

mcastexpression_t* mc_astexpr_makeprefixexpr(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* right)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_PREFIX);
    if(!res)
    {
        return nullptr;
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
        return nullptr;
    }
    res->uexpr.exprinfix.op = op;
    res->uexpr.exprinfix.left = left;
    res->uexpr.exprinfix.right = right;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralfunction(mcstate_t* state, PtrList* params, mcastexprcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_FUNCTIONLITERAL);
    if(!res)
    {
        return nullptr;
    }
    res->uexpr.exprlitfunction.name = nullptr;
    res->uexpr.exprlitfunction.funcparamlist = params;
    res->uexpr.exprlitfunction.body = body;
    return res;
}

mcastexpression_t* mc_astexpr_makecallexpr(mcstate_t* state, mcastexpression_t* function, PtrList* args)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_CALL);
    if(!res)
    {
        return nullptr;
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
        return nullptr;
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
        return nullptr;
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
        return nullptr;
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
        return nullptr;
    }
    res->uexpr.exprternary.tercond = test;
    res->uexpr.exprternary.teriftrue = ift;
    res->uexpr.exprternary.teriffalse = iffalse;
    return res;
}

mcastexpression_t* mc_parser_makefunccallexpr(mcstate_t* state, mcastexpression_t* expr, const char* fname)
{
    bool ok;
    mcasttoken_t fntoken;
    mcastexprident_t* ident;
    PtrList* args;
    mcastexpression_t* ce;
    mcastexpression_t* functionidentexpr;
    mc_asttoken_init(&fntoken, MC_TOK_IDENT, fname, mc_util_strlen(fname));
    fntoken.pos = expr->pos;
    ident = mc_astident_make(state, fntoken);
    if(!ident)
    {
        return nullptr;
    }
    ident->pos = fntoken.pos;
    functionidentexpr = mc_astexpr_makeident(state, ident);
    if(!functionidentexpr)
    {
        mc_astident_destroy(ident);
        return nullptr;
    }
    functionidentexpr->pos = expr->pos;
    ident = nullptr;
    args = Memory::make<PtrList>(sizeof(void*), true);
    if(!args)
    {
        mcastexpression_t::destroyExpression(functionidentexpr);
        return nullptr;
    }
    ok = args->push(expr);
    if(!ok)
    {
        PtrList::destroy(args, nullptr);
        mcastexpression_t::destroyExpression(functionidentexpr);
        return nullptr;
    }
    ce = mc_astexpr_makecallexpr(state, functionidentexpr, args);
    if(!ce)
    {
        PtrList::destroy(args, nullptr);
        mcastexpression_t::destroyExpression(functionidentexpr);
        return nullptr;
    }
    ce->pos = expr->pos;
    return ce;
}




mcastexpression_t* mc_astexpr_makedefineexpr(mcstate_t* state, mcastexprident_t* name, mcastexpression_t* value, bool assignable)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTDEFINE);
    if(!res)
    {
        return nullptr;
    }
    res->uexpr.exprdefine.name = name;
    res->uexpr.exprdefine.value = value;
    res->uexpr.exprdefine.assignable = assignable;
    return res;
}

mcastexpression_t* mc_astexpr_makeifexpr(mcstate_t* state, PtrList* cases, mcastexprcodeblock_t* alternative)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTIF);
    if(!res)
    {
        return nullptr;
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
        return nullptr;
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
        return nullptr;
    }
    res->uexpr.exprexpression = value;
    return res;
}

mcastexpression_t* mc_astexpr_makewhileexpr(mcstate_t* state, mcastexpression_t* test, mcastexprcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTLOOPWHILE);
    if(!res)
    {
        return nullptr;
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
        return nullptr;
    }
    return res;
}

mcastexpression_t* mc_astexpr_makeforeachexpr(mcstate_t* state, mcastexprident_t* iterator, mcastexpression_t* source, mcastexprcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTLOOPFOREACH);
    if(!res)
    {
        return nullptr;
    }
    res->uexpr.exprforeachloopstmt.iterator = iterator;
    res->uexpr.exprforeachloopstmt.source = source;
    res->uexpr.exprforeachloopstmt.body = body;
    return res;
}

mcastexpression_t* mc_astexpr_makeforloopexpr(mcstate_t* state, mcastexpression_t* init, mcastexpression_t* test, mcastexpression_t* update, mcastexprcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTLOOPFORCLASSIC);
    if(!res)
    {
        return nullptr;
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
        return nullptr;
    }
    return res;
}

mcastexpression_t* mc_astexpr_makeblockexpr(mcstate_t* state, mcastexprcodeblock_t* block)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTBLOCK);
    if(!res)
    {
        return nullptr;
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
        return nullptr;
    }
    res->uexpr.exprimportstmt.path = path;
    return res;
}

mcastexpression_t* mc_astexpr_makerecoverexpr(mcstate_t* state, mcastexprident_t* eid, mcastexprcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTRECOVER);
    if(!res)
    {
        return nullptr;
    }
    res->uexpr.exprrecoverstmt.errident = eid;
    res->uexpr.exprrecoverstmt.body = body;
    return res;
}

mcastexprcodeblock_t* mc_astcodeblock_make(mcstate_t* state, PtrList* statements)
{
    mcastexprcodeblock_t* block;
    block = Memory::make<mcastexprcodeblock_t>();
    if(!block)
    {
        return nullptr;
    }
    block->pstate = state;
    block->statements = statements;
    return block;
}

void mc_astcodeblock_destroy(mcastexprcodeblock_t* block)
{
    if(block != nullptr)
    {
        PtrList::destroy(block->statements, (mcitemdestroyfn_t)mcastexpression_t::destroyExpression);
        mc_memory_free(block);
    }
}

mcastexprcodeblock_t* mc_astcodeblock_copy(mcastexprcodeblock_t* block)
{
    mcastexprcodeblock_t* res;
    PtrList* statementscopy;
    if(!block)
    {
        return nullptr;
    }
    statementscopy = block->statements->copy((mcitemcopyfn_t)mcastexpression_t::copyExpression, (mcitemdestroyfn_t)mcastexpression_t::destroyExpression);
    if(!statementscopy)
    {
        return nullptr;
    }
    res = mc_astcodeblock_make(block->pstate, statementscopy);
    if(!res)
    {
        PtrList::destroy(statementscopy, (mcitemdestroyfn_t)mcastexpression_t::destroyExpression);
        return nullptr;
    }
    return res;
}

mcastfuncparam_t* mc_astfuncparam_make(mcstate_t* state, mcastexprident_t* ident)
{
    mcastfuncparam_t* res;
    res = Memory::make<mcastfuncparam_t>();
    if(!res)
    {
        return nullptr;
    }
    res->pstate = state;
    res->ident = ident;
    if(!res->ident->value)
    {
        mc_memory_free(res);
        return nullptr;
    }
    return res;
}

mcastfuncparam_t* mc_astfuncparam_copy(mcastfuncparam_t* param)
{
    mcastfuncparam_t* res;
    res = Memory::make<mcastfuncparam_t>();
    if(!res)
    {
        return nullptr;
    }
    res->pstate = param->pstate;
    res->ident = mc_astident_copy(param->ident);
    if(!res->ident->value)
    {
        mc_memory_free(res);
        return nullptr;
    }
    return res;
}

void mc_astfuncparam_destroy(mcastfuncparam_t* param)
{
    if(param != nullptr)
    {
        mc_astident_destroy(param->ident);
        mc_memory_free(param);
    }
}

mcastexprident_t* mc_astident_make(mcstate_t* state, mcasttoken_t tok)
{
    mcastexprident_t* res = Memory::make<mcastexprident_t>();
    if(!res)
    {
        return nullptr;
    }
    res->pstate = state;
    res->value = mc_asttoken_dupliteralstring(&tok);
    if(!res->value)
    {
        mc_memory_free(res);
        return nullptr;
    }
    res->pos = tok.pos;
    return res;
}

mcastexprident_t* mc_astident_copy(mcastexprident_t* ident)
{
    mcastexprident_t* res = Memory::make<mcastexprident_t>();
    if(!res)
    {
        return nullptr;
    }
    res->pstate = ident->pstate;
    res->value = mc_util_strdup(ident->value);
    if(!res->value)
    {
        mc_memory_free(res);
        return nullptr;
    }
    res->pos = ident->pos;
    return res;
}

void mc_astident_destroy(mcastexprident_t* ident)
{
    if(ident != nullptr)
    {
        mc_memory_free(ident->value);
        ident->value = nullptr;
        ident->pos = srcposinvalid;
        mc_memory_free(ident);
    }
}

mcastexprifcase_t* mc_astifcase_make(mcstate_t* state, mcastexpression_t* test, mcastexprcodeblock_t* consequence)
{
    mcastexprifcase_t* res;
    res = Memory::make<mcastexprifcase_t>();
    if(!res)
    {
        return nullptr;
    }
    res->pstate = state;
    res->ifcond = test;
    res->consequence = consequence;
    return res;
}

void mc_astifcase_destroy(mcastexprifcase_t* cond)
{
    if(cond != nullptr)
    {
        mcastexpression_t::destroyExpression(cond->ifcond);
        mc_astcodeblock_destroy(cond->consequence);
        mc_memory_free(cond);
    }
}

mcastexprifcase_t* mc_astifcase_copy(mcastexprifcase_t* ifcase)
{
    mcastexpression_t* testcopy;
    mcastexprcodeblock_t* consequencecopy;
    mcastexprifcase_t* ifcasecopy;
    if(!ifcase)
    {
        return nullptr;
    }
    testcopy = nullptr;
    consequencecopy = nullptr;
    ifcasecopy = nullptr;
    testcopy = mcastexpression_t::copyExpression(ifcase->ifcond);
    if(!testcopy)
    {
        goto err;
    }
    #if 1
    consequencecopy = mc_astcodeblock_copy(ifcase->consequence);
    #else
    consequencecopy = ifcase->consequence;    
    #endif
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
    mcastexpression_t::destroyExpression(testcopy);
    mc_astcodeblock_destroy(consequencecopy);
    mc_astifcase_destroy(ifcasecopy);
    return nullptr;
}

mcastexpression_t* mc_astexpr_makeexpression(mcstate_t* state, mcastexprtype_t type)
{
    mcastexpression_t* res = Memory::make<mcastexpression_t>();
    if(!res)
    {
        return nullptr;
    }
    res->pstate = state;
    res->exprtype = type;
    res->pos = srcposinvalid;
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
        case 'e':
            return 27;
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

char* mc_parser_processandcopystring(const char* input, size_t len)
{
    size_t ini;
    size_t outi;
    char* output;
    output = (char*)mc_memory_malloc(len + 1);
    if(!output)
    {
        return nullptr;
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
    mc_memory_free(output);
    return nullptr;
}

mcastparser_t* mc_astparser_make(mcstate_t* state, mcconfig_t* config, mcerrlist_t* errors)
{
    mcastparser_t* parser;
    parser = Memory::make<mcastparser_t>();
    if(!parser)
    {
        return nullptr;
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
        #if 1
        parser->rightassocfuncs[MC_TOK_RECOVER] = mc_parser_parserecoverstmt;
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
    if(parser != nullptr)
    {
        mc_memory_free(parser);
    }
}

PtrList* mc_astparser_parseall(mcastparser_t* parser, const char* input, mcastcompiledfile_t* file)
{
    bool ok;
    mcastexpression_t* expr;
    PtrList* statements;
    parser->depth = 0;
    ok = AstLexer::init(&parser->lexer, parser->pstate, parser->errors, input, file);
    if(!ok)
    {
        return nullptr;
    }
    parser->lexer.nextToken();
    parser->lexer.nextToken();
    statements = Memory::make<PtrList>(sizeof(void*), true);
    if(!statements)
    {
        return nullptr;
    }
    while(!parser->lexer.currentTokenIs(MC_TOK_EOF))
    {
        if(parser->lexer.currentTokenIs(MC_TOK_SEMICOLON))
        {
            parser->lexer.nextToken();
            continue;
        }
        expr = mc_astparser_parsestatement(parser);
        if(!expr)
        {
            goto err;
        }
        ok = statements->push(expr);
        if(!ok)
        {
            mcastexpression_t::destroyExpression(expr);
            goto err;
        }
    }
    if(parser->errors->count > 0)
    {
        goto err;
    }
    return statements;
err:
    PtrList::destroy(statements, (mcitemdestroyfn_t)mcastexpression_t::destroyExpression);
    return nullptr;
}

mcastexpression_t* mc_astparser_parsestatement(mcastparser_t* p)
{
    mcastlocation_t pos;
    mcastexpression_t* res;
    pos = p->lexer.m_currtoken.pos;
    res = nullptr;
    switch(p->lexer.m_currtoken.toktype)
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
                res = mc_parser_parseloopforloopstmt(p);
            }
            break;
        case MC_TOK_FUNCTION:
            {
                if(p->lexer.peekTokenIs(MC_TOK_IDENT))
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
    mcastexprident_t* nameident;
    mcastexpression_t* value;
    mcastexpression_t* res;
    nameident = nullptr;
    value = nullptr;
    assignable = p->lexer.currentTokenIs(MC_TOK_VAR);
    p->lexer.nextToken();
    if(!p->lexer.expectCurrent(MC_TOK_IDENT))
    {
        goto err;
    }
    nameident = mc_astident_make(p->pstate, p->lexer.m_currtoken);
    if(!nameident)
    {
        goto err;
    }
    p->lexer.nextToken();
    #if 0
        if(!p->lexer.expectCurrent(MC_TOK_ASSIGN))
    #else
        if(!p->lexer.currentTokenIs(MC_TOK_ASSIGN))
    #endif
    {
        value = mc_astexpr_makeliteralnull(p->pstate);
        goto finish;
    }
    p->lexer.nextToken();
    value = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!value)
    {
        goto err;
    }
    if(value->exprtype == MC_EXPR_FUNCTIONLITERAL)
    {
        value->uexpr.exprlitfunction.name = mc_util_strdup(nameident->value);
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
    mcastexpression_t::destroyExpression(value);
    mc_astident_destroy(nameident);
    return nullptr;
}

mcastexpression_t* mc_parser_parseifstmt(mcastparser_t* p)
{
    bool ok;
    PtrList* cases;
    mcastexprifcase_t* cond;
    mcastexprifcase_t* elif;
    mcastexprcodeblock_t* alternative;
    mcastexpression_t* res;
    cases = nullptr;
    alternative = nullptr;
    cases = Memory::make<PtrList>(sizeof(void*), true);
    if(!cases)
    {
        goto err;
    }
    p->lexer.nextToken();
    if(!p->lexer.expectCurrent(MC_TOK_LPAREN))
    {
        goto err;
    }
    p->lexer.nextToken();
    cond = mc_astifcase_make(p->pstate, nullptr, nullptr);
    if(!cond)
    {
        goto err;
    }
    ok = cases->push(cond);
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
    if(!p->lexer.expectCurrent(MC_TOK_RPAREN))
    {
        goto err;
    }
    p->lexer.nextToken();
    cond->consequence = mc_parser_parsecodeblock(p);
    if(!cond->consequence)
    {
        goto err;
    }
    while(p->lexer.currentTokenIs(MC_TOK_ELSE))
    {
        p->lexer.nextToken();
        if(p->lexer.currentTokenIs(MC_TOK_IF))
        {
            p->lexer.nextToken();
            if(!p->lexer.expectCurrent(MC_TOK_LPAREN))
            {
                goto err;
            }
            p->lexer.nextToken();
            elif = mc_astifcase_make(p->pstate, nullptr, nullptr);
            if(!elif)
            {
                goto err;
            }
            ok = cases->push(elif);
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
            if(!p->lexer.expectCurrent(MC_TOK_RPAREN))
            {
                goto err;
            }
            p->lexer.nextToken();
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
    PtrList::destroy(cases, (mcitemdestroyfn_t)mc_astifcase_destroy);
    mc_astcodeblock_destroy(alternative);
    return nullptr;
}

mcastexpression_t* mc_parser_parsereturnstmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* expr;
    expr = nullptr;
    p->lexer.nextToken();
    if(!p->lexer.currentTokenIs(MC_TOK_SEMICOLON) && !p->lexer.currentTokenIs(MC_TOK_RBRACE) && !p->lexer.currentTokenIs(MC_TOK_EOF))
    {
        expr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
        if(!expr)
        {
            return nullptr;
        }
    }
    res = mc_astexpr_makereturnexpr(p->pstate, expr);
    if(!res)
    {
        mcastexpression_t::destroyExpression(expr);
        return nullptr;
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
        return nullptr;
    }
    if(expr && (!p->config->replmode || p->depth > 0))
    {
        #if 0
        /* this is actually completely unnecessary */
        if(expr->exprtype != MC_EXPR_ASSIGN && expr->exprtype != MC_EXPR_CALL)
        {
            p->errors->pushFormat(MC_ERROR_PARSING, expr->pos, "only assignments and function calls can be expression statements");
            mcastexpression_t::destroyExpression(expr);
            return nullptr;
        }
        #endif
    }
    res = mc_astexpr_makeexprstmt(p->pstate, expr);
    if(!res)
    {
        mcastexpression_t::destroyExpression(expr);
        return nullptr;
    }
    return res;
}

mcastexpression_t* mc_parser_parseloopwhilestmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* test;
    mcastexprcodeblock_t* body;
    test = nullptr;
    body = nullptr;
    p->lexer.nextToken();
    if(!p->lexer.expectCurrent(MC_TOK_LPAREN))
    {
        goto err;
    }
    p->lexer.nextToken();
    test = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!test)
    {
        goto err;
    }
    if(!p->lexer.expectCurrent(MC_TOK_RPAREN))
    {
        goto err;
    }
    p->lexer.nextToken();
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
    mcastexpression_t::destroyExpression(test);
    return nullptr;
}

mcastexpression_t* mc_parser_parsebreakstmt(mcastparser_t* p)
{
    p->lexer.nextToken();
    return mc_astexpr_makebreakexpr(p->pstate);
}

mcastexpression_t* mc_parser_parsecontinuestmt(mcastparser_t* p)
{
    p->lexer.nextToken();
    return mc_astexpr_makecontinueexpr(p->pstate);
}

mcastexpression_t* mc_parser_parseblockstmt(mcastparser_t* p)
{
    mcastexprcodeblock_t* block;
    mcastexpression_t* res;
    block = mc_parser_parsecodeblock(p);
    if(!block)
    {
        return nullptr;
    }
    res = mc_astexpr_makeblockexpr(p->pstate, block);
    if(!res)
    {
        mc_astcodeblock_destroy(block);
        return nullptr;
    }
    return res;
}

mcastexpression_t* mc_parser_parseimportstmt(mcastparser_t* p)
{
    char* processedname;
    mcastexpression_t* res;
    p->lexer.nextToken();
    if(!p->lexer.expectCurrent(MC_TOK_STRING))
    {
        return nullptr;
    }
    processedname = mc_parser_processandcopystring(p->lexer.m_currtoken.tokstrdata, p->lexer.m_currtoken.tokstrlen);
    if(!processedname)
    {
        p->errors->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.pos, "error when parsing module name");
        return nullptr;
    }
    p->lexer.nextToken();
    res = mc_astexpr_makeimportexpr(p->pstate, processedname);
    if(!res)
    {
        mc_memory_free(processedname);
        return nullptr;
    }
    return res;
}

mcastexpression_t* mc_parser_parserecoverstmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexprident_t* eid;
    mcastexprcodeblock_t* body;
    eid = nullptr;
    body = nullptr;
    p->lexer.nextToken();
    if(!p->lexer.expectCurrent(MC_TOK_LPAREN))
    {
        return nullptr;
    }
    p->lexer.nextToken();
    if(!p->lexer.expectCurrent(MC_TOK_IDENT))
    {
        return nullptr;
    }
    eid = mc_astident_make(p->pstate, p->lexer.m_currtoken);
    if(!eid)
    {
        return nullptr;
    }
    p->lexer.nextToken();
    if(!p->lexer.expectCurrent(MC_TOK_RPAREN))
    {
        goto err;
    }
    p->lexer.nextToken();
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
    return nullptr;
}

mcastexpression_t* mc_parser_parseloopforloopstmt(mcastparser_t* p)
{
    p->lexer.nextToken();
    if(!p->lexer.expectCurrent(MC_TOK_LPAREN))
    {
        return nullptr;
    }
    p->lexer.nextToken();
    if(p->lexer.currentTokenIs(MC_TOK_IDENT) && p->lexer.peekTokenIs(MC_TOK_IN))
    {
        return mc_parser_parseloopforeachstmt(p);
    }
    return mc_parser_parseloopforcstylestmt(p);
}

mcastexpression_t* mc_parser_parseloopforeachstmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* source;
    mcastexprcodeblock_t* body;
    mcastexprident_t* iteratorident;
    source = nullptr;
    body = nullptr;
    iteratorident = nullptr;
    iteratorident = mc_astident_make(p->pstate, p->lexer.m_currtoken);
    if(!iteratorident)
    {
        goto err;
    }
    p->lexer.nextToken();
    if(!p->lexer.expectCurrent(MC_TOK_IN))
    {
        goto err;
    }
    p->lexer.nextToken();
    source = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!source)
    {
        goto err;
    }
    if(!p->lexer.expectCurrent(MC_TOK_RPAREN))
    {
        goto err;
    }
    p->lexer.nextToken();
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
    mcastexpression_t::destroyExpression(source);
    return nullptr;
}

mcastexpression_t* mc_parser_parseloopforcstylestmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* init;
    mcastexpression_t* test;
    mcastexpression_t* update;
    mcastexprcodeblock_t* body;
    init = nullptr;
    test = nullptr;
    update = nullptr;
    body = nullptr;
    if(!p->lexer.currentTokenIs(MC_TOK_SEMICOLON))
    {
        init = mc_astparser_parsestatement(p);
        if(!init)
        {
            goto err;
        }
        if(init->exprtype != MC_EXPR_STMTDEFINE && init->exprtype != MC_EXPR_STMTEXPRESSION)
        {
            p->errors->pushFormat(MC_ERROR_PARSING, init->pos, "expected a definition or expression as 'for' loop init clause");
            goto err;
        }
        if(!p->lexer.expectCurrent(MC_TOK_SEMICOLON))
        {
            goto err;
        }
    }
    p->lexer.nextToken();
    if(!p->lexer.currentTokenIs(MC_TOK_SEMICOLON))
    {
        test = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
        if(!test)
        {
            goto err;
        }
        if(!p->lexer.expectCurrent(MC_TOK_SEMICOLON))
        {
            goto err;
        }
    }
    p->lexer.nextToken();
    if(!p->lexer.currentTokenIs(MC_TOK_RPAREN))
    {
        update = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
        if(!update)
        {
            goto err;
        }
        if(!p->lexer.expectCurrent(MC_TOK_RPAREN))
        {
            goto err;
        }
    }
    p->lexer.nextToken();
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
    mcastexpression_t::destroyExpression(init);
    mcastexpression_t::destroyExpression(test);
    mcastexpression_t::destroyExpression(update);
    mc_astcodeblock_destroy(body);
    return nullptr;
}

mcastexprcodeblock_t* mc_parser_parsecodeblock(mcastparser_t* p)
{
    bool ok;
    mcastexprcodeblock_t* res;
    mcastexpression_t* expr;
    PtrList* statements;
    if(!p->lexer.expectCurrent(MC_TOK_LBRACE))
    {
        return nullptr;
    }
    p->lexer.nextToken();
    p->depth++;
    statements = Memory::make<PtrList>(sizeof(void*), true);
    if(!statements)
    {
        goto err;
    }
    while(!p->lexer.currentTokenIs(MC_TOK_RBRACE))
    {
        if(p->lexer.currentTokenIs(MC_TOK_EOF))
        {
            p->errors->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.pos, "unexpected EOF");
            goto err;
        }
        if(p->lexer.currentTokenIs(MC_TOK_SEMICOLON))
        {
            p->lexer.nextToken();
            continue;
        }
        expr = mc_astparser_parsestatement(p);
        if(!expr)
        {
            goto err;
        }
        ok = statements->push(expr);
        if(!ok)
        {
            mcastexpression_t::destroyExpression(expr);
            goto err;
        }
    }
    p->lexer.nextToken();
    p->depth--;
    res = mc_astcodeblock_make(p->pstate, statements);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    p->depth--;
    PtrList::destroy(statements, (mcitemdestroyfn_t)mcastexpression_t::destroyExpression);
    return nullptr;
}

mcastexpression_t* mc_parser_parseexpression(mcastparser_t* p, mcastprecedence_t prec)
{
    char* literal;
    mcastlocation_t pos;
    mcleftassocparsefn_t parseleftassoc;
    mcastrightassocparsefn_t parserightassoc;
    mcastexpression_t* newleftexpr;
    mcastexpression_t* leftexpr;
    pos = p->lexer.m_currtoken.pos;
    if(p->lexer.m_currtoken.toktype == MC_TOK_INVALID)
    {
        p->errors->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.pos, "illegal token");
        return nullptr;
    }
    parserightassoc = p->rightassocfuncs[p->lexer.m_currtoken.toktype];
    if(!parserightassoc)
    {
        literal = mc_asttoken_dupliteralstring(&p->lexer.m_currtoken);
        p->errors->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.pos, "no prefix parse function for \"%s\" found", literal);
        mc_memory_free(literal);
        return nullptr;
    }
    leftexpr = parserightassoc(p);
    if(!leftexpr)
    {
        return nullptr;
    }
    leftexpr->pos = pos;
    while(!p->lexer.currentTokenIs(MC_TOK_SEMICOLON) && prec < mc_parser_getprecedence(p->lexer.m_currtoken.toktype))
    {
        parseleftassoc = p->leftassocfuncs[p->lexer.m_currtoken.toktype];
        if(!parseleftassoc)
        {
            return leftexpr;
        }
        pos = p->lexer.m_currtoken.pos;
        newleftexpr = parseleftassoc(p, leftexpr);
        if(!newleftexpr)
        {
            mcastexpression_t::destroyExpression(leftexpr);
            return nullptr;
        }
        newleftexpr->pos = pos;
        leftexpr = newleftexpr;
    }
    return leftexpr;
}

mcastexpression_t* mc_parser_parseident(mcastparser_t* p)
{
    mcastexprident_t* ident;
    mcastexpression_t* res;
    ident = mc_astident_make(p->pstate, p->lexer.m_currtoken);
    if(!ident)
    {
        return nullptr;
    }
    res = mc_astexpr_makeident(p->pstate, ident);
    if(!res)
    {
        mc_astident_destroy(ident);
        return nullptr;
    }
    p->lexer.nextToken();
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
    number = mc_util_strtod(p->lexer.m_currtoken.tokstrdata, p->lexer.m_currtoken.tokstrlen, &end);
    #if 0
        fprintf(stderr, "literal=<%s> number=<%f>\n", p->lexer.m_currtoken.tokstrdata, number);
    #endif
    parsedlen = end - p->lexer.m_currtoken.tokstrdata;
    if(errno || parsedlen != p->lexer.m_currtoken.tokstrlen)
    {
        literal = mc_asttoken_dupliteralstring(&p->lexer.m_currtoken);
        p->errors->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.pos, "failed to parse number literal \"%s\"", literal);
        mc_memory_free(literal);
        return nullptr;
    }    
    p->lexer.nextToken();
    return mc_astexpr_makeliteralnumber(p->pstate, number);
}

mcastexpression_t* mc_parser_parseliteralbool(mcastparser_t* p)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeliteralbool(p->pstate, p->lexer.m_currtoken.toktype == MC_TOK_TRUE);
    p->lexer.nextToken();
    return res;
}

mcastexpression_t* mc_parser_parseliteralstring(mcastparser_t* p)
{
    size_t len;
    char* processedliteral;
    mcastexpression_t* res;
    processedliteral = mc_parser_processandcopystring(p->lexer.m_currtoken.tokstrdata, p->lexer.m_currtoken.tokstrlen);
    if(!processedliteral)
    {
        p->errors->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.pos, "error parsing string literal");
        return nullptr;
    }
    p->lexer.nextToken();
    len = mc_util_strlen(processedliteral);
    res = mc_astexpr_makeliteralstring(p->pstate, processedliteral, len);
    if(!res)
    {
        mc_memory_free(processedliteral);
        return nullptr;
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
    processedliteral = nullptr;
    leftstringexpr = nullptr;
    templateexpr = nullptr;
    tostrcallexpr = nullptr;
    leftaddexpr = nullptr;
    rightexpr = nullptr;
    rightaddexpr = nullptr;
    processedliteral = mc_parser_processandcopystring(p->lexer.m_currtoken.tokstrdata, p->lexer.m_currtoken.tokstrlen);
    if(!processedliteral)
    {
        p->errors->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.pos, "error parsing string literal");
        return nullptr;
    }
    p->lexer.nextToken();
    if(!p->lexer.expectCurrent(MC_TOK_LBRACE))
    {
        goto err;
    }
    p->lexer.nextToken();
    pos = p->lexer.m_currtoken.pos;
    len = mc_util_strlen(processedliteral);
    leftstringexpr = mc_astexpr_makeliteralstring(p->pstate, processedliteral, len);
    if(!leftstringexpr)
    {
        goto err;
    }
    leftstringexpr->pos = pos;
    processedliteral = nullptr;
    pos = p->lexer.m_currtoken.pos;
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
    templateexpr = nullptr;
    leftaddexpr = mc_astexpr_makeinfixexpr(p->pstate, MC_MATHOP_PLUS, leftstringexpr, tostrcallexpr);
    if(!leftaddexpr)
    {
        goto err;
    }
    leftaddexpr->pos = pos;
    leftstringexpr = nullptr;
    tostrcallexpr = nullptr;
    if(!p->lexer.expectCurrent(MC_TOK_RBRACE))
    {
        goto err;
    }
    p->lexer.previousToken();
    p->lexer.conttplstring();
    p->lexer.nextToken();
    p->lexer.nextToken();
    pos = p->lexer.m_currtoken.pos;
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
    leftaddexpr = nullptr;
    rightexpr = nullptr;
    return rightaddexpr;
err:
    mcastexpression_t::destroyExpression(rightaddexpr);
    mcastexpression_t::destroyExpression(rightexpr);
    mcastexpression_t::destroyExpression(leftaddexpr);
    mcastexpression_t::destroyExpression(tostrcallexpr);
    mcastexpression_t::destroyExpression(templateexpr);
    mcastexpression_t::destroyExpression(leftstringexpr);
    mc_memory_free(processedliteral);
    return nullptr;
}

mcastexpression_t* mc_parser_parseliteralnull(mcastparser_t* p)
{
    p->lexer.nextToken();
    return mc_astexpr_makeliteralnull(p->pstate);
}

mcastexpression_t* mc_parser_parseliteralarray(mcastparser_t* p)
{
    PtrList* array;
    mcastexpression_t* res;
    array = mc_parser_parseexprlist(p, MC_TOK_LBRACKET, MC_TOK_RBRACKET, true);
    if(!array)
    {
        return nullptr;
    }
    res = mc_astexpr_makeliteralarray(p->pstate, array);
    if(!res)
    {
        PtrList::destroy(array, (mcitemdestroyfn_t)mcastexpression_t::destroyExpression);
        return nullptr;
    }
    return res;
}

mcastexpression_t* mc_parser_parseliteralmap(mcastparser_t* p)
{
    bool ok;
    size_t len;
    char* str;
    PtrList* keys;
    PtrList* values;
    mcastexpression_t* res;
    mcastexpression_t* key;
    mcastexpression_t* value;
    keys = Memory::make<PtrList>(sizeof(void*), true);
    values = Memory::make<PtrList>(sizeof(void*), true);
    if(!keys || !values)
    {
        goto err;
    }
    p->lexer.nextToken();
    while(!p->lexer.currentTokenIs(MC_TOK_RBRACE))
    {
        key = nullptr;
        if(p->lexer.currentTokenIs(MC_TOK_IDENT))
        {
            str = mc_asttoken_dupliteralstring(&p->lexer.m_currtoken);
            len = mc_util_strlen(str);
            key = mc_astexpr_makeliteralstring(p->pstate, str, len);
            if(!key)
            {
                mc_memory_free(str);
                goto err;
            }
            key->pos = p->lexer.m_currtoken.pos;
            p->lexer.nextToken();
        }
        else
        {
            key = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!key)
            {
                goto err;
            }
            switch(key->exprtype)
            {
                case MC_EXPR_STRINGLITERAL:
                case MC_EXPR_NUMBERLITERAL:
                case MC_EXPR_BOOLLITERAL:
                    {
                    }
                    break;
                default:
                    {
                        p->errors->pushFormat(MC_ERROR_PARSING, key->pos, "can only use primitive types as literal 'map' object keys");
                        mcastexpression_t::destroyExpression(key);
                        goto err;
                    }
                    break;
            }
        }
        ok = keys->push(key);
        if(!ok)
        {
            mcastexpression_t::destroyExpression(key);
            goto err;
        }
        if(!p->lexer.expectCurrent(MC_TOK_COLON))
        {
            goto err;
        }
        p->lexer.nextToken();
        value = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
        if(!value)
        {
            goto err;
        }
        ok = values->push(value);
        if(!ok)
        {
            mcastexpression_t::destroyExpression(value);
            goto err;
        }
        if(p->lexer.currentTokenIs(MC_TOK_RBRACE))
        {
            break;
        }
        if(!p->lexer.expectCurrent(MC_TOK_COMMA))
        {
            goto err;
        }
        p->lexer.nextToken();
    }
    p->lexer.nextToken();
    res = mc_astexpr_makeliteralmap(p->pstate, keys, values);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    PtrList::destroy(keys, (mcitemdestroyfn_t)mcastexpression_t::destroyExpression);
    PtrList::destroy(values, (mcitemdestroyfn_t)mcastexpression_t::destroyExpression);
    return nullptr;
}

mcastexpression_t* mc_parser_parseprefixexpr(mcastparser_t* p)
{
    mcastmathoptype_t op;
    mcastexpression_t* res;
    mcastexpression_t* right;
    op = mc_parser_tokentomathop(p->lexer.m_currtoken.toktype);
    p->lexer.nextToken();
    right = mc_parser_parseexpression(p, MC_ASTPREC_PREFIX);
    if(!right)
    {
        return nullptr;
    }
    res = mc_astexpr_makeprefixexpr(p->pstate, op, right);
    if(!res)
    {
        mcastexpression_t::destroyExpression(right);
        return nullptr;
    }
    return res;
}

mcastexpression_t* mc_parser_parseinfixexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastmathoptype_t op;
    mcastprecedence_t prec;
    mcastexpression_t* res;
    mcastexpression_t* right;
    op = mc_parser_tokentomathop(p->lexer.m_currtoken.toktype);
    prec = mc_parser_getprecedence(p->lexer.m_currtoken.toktype);
    p->lexer.nextToken();
    right = mc_parser_parseexpression(p, prec);
    if(!right)
    {
        return nullptr;
    }
    res = mc_astexpr_makeinfixexpr(p->pstate, op, left, right);
    if(!res)
    {
        mcastexpression_t::destroyExpression(right);
        return nullptr;
    }
    return res;
}

mcastexpression_t* mc_parser_parsegroupedexpr(mcastparser_t* p)
{
    mcastexpression_t* expr;
    p->lexer.nextToken();
    expr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!expr || !p->lexer.expectCurrent(MC_TOK_RPAREN))
    {
        mcastexpression_t::destroyExpression(expr);
        return nullptr;
    }
    p->lexer.nextToken();
    return expr;
}


bool mc_parser_parsefuncparams(mcastparser_t* p, PtrList* outparams)
{
    bool ok;
    mcastexprident_t* ident;
    mcastfuncparam_t* param;
    if(!p->lexer.expectCurrent(MC_TOK_LPAREN))
    {
        return false;
    }
    p->lexer.nextToken();
    if(p->lexer.currentTokenIs(MC_TOK_RPAREN))
    {
        p->lexer.nextToken();
        return true;
    }
    if(!p->lexer.expectCurrent(MC_TOK_IDENT))
    {
        return false;
    }
    ident = mc_astident_make(p->pstate, p->lexer.m_currtoken);
    if(!ident)
    {
        return false;
    }
    param = mc_astfuncparam_make(p->pstate, ident);
    ok = outparams->push(param);
    if(!ok)
    {
        mc_astident_destroy(ident);
        return false;
    }
    p->lexer.nextToken();
    while(p->lexer.currentTokenIs(MC_TOK_COMMA))
    {
        p->lexer.nextToken();
        if(!p->lexer.expectCurrent(MC_TOK_IDENT))
        {
            return false;
        }
        ident = mc_astident_make(p->pstate, p->lexer.m_currtoken);
        if(!ident)
        {
            return false;
        }
        param = mc_astfuncparam_make(p->pstate, ident);
        ok = outparams->push(param);
        if(!ok)
        {
            mc_astfuncparam_destroy(param);
            return false;
        }
        p->lexer.nextToken();
    }
    if(!p->lexer.expectCurrent(MC_TOK_RPAREN))
    {
        return false;
    }
    p->lexer.nextToken();
    return true;
}

mcastexpression_t* mc_parser_parseliteralfunction(mcastparser_t* p)
{
    bool ok;
    PtrList* params;
    mcastexprcodeblock_t* body;
    mcastexpression_t* res;
    p->depth++;
    params = nullptr;
    body = nullptr;
    if(p->lexer.currentTokenIs(MC_TOK_FUNCTION))
    {
        p->lexer.nextToken();
    }
    params = Memory::make<PtrList>(sizeof(void*), true);
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
    PtrList::destroy(params, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
    p->depth -= 1;
    return nullptr;
}

mcastexpression_t* mc_parser_parsefunctionstmt(mcastparser_t* p)
{
    mcastexprident_t* nameident;
    mcastexpression_t* res;
    mcastexpression_t* value;
    mcastlocation_t pos;
    nameident = nullptr;
    value = nullptr;
    pos = p->lexer.m_currtoken.pos;
    p->lexer.nextToken();
    if(!p->lexer.expectCurrent(MC_TOK_IDENT))
    {
        goto err;
    }
    nameident = mc_astident_make(p->pstate, p->lexer.m_currtoken);
    if(!nameident)
    {
        goto err;
    }
    p->lexer.nextToken();
    value = mc_parser_parseliteralfunction(p);
    if(!value)
    {
        goto err;
    }
    value->pos = pos;
    value->uexpr.exprlitfunction.name = mc_util_strdup(nameident->value);
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
    mcastexpression_t::destroyExpression(value);
    mc_astident_destroy(nameident);
    return nullptr;
}


mcastexpression_t* mc_parser_parsecallexpr(mcastparser_t* p, mcastexpression_t* left)
{
    PtrList* args;
    mcastexpression_t* res;
    mcastexpression_t* function;
    function = left;
    args = mc_parser_parseexprlist(p, MC_TOK_LPAREN, MC_TOK_RPAREN, false);
    if(!args)
    {
        return nullptr;
    }
    res = mc_astexpr_makecallexpr(p->pstate, function, args);
    if(!res)
    {
        PtrList::destroy(args, (mcitemdestroyfn_t)mcastexpression_t::destroyExpression);
        return nullptr;
    }
    return res;
}

PtrList* mc_parser_parseexprlist(mcastparser_t* p, mcasttoktype_t starttoken, mcasttoktype_t endtoken, bool trailingcommaallowed)
{
    bool ok;
    PtrList* res;
    mcastexpression_t* argexpr;
    if(!p->lexer.expectCurrent(starttoken))
    {
        return nullptr;
    }
    p->lexer.nextToken();
    res = Memory::make<PtrList>(sizeof(void*), true);
    if(p->lexer.currentTokenIs(endtoken))
    {
        p->lexer.nextToken();
        return res;
    }
    argexpr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!argexpr)
    {
        goto err;
    }
    ok = res->push(argexpr);
    if(!ok)
    {
        mcastexpression_t::destroyExpression(argexpr);
        goto err;
    }
    while(p->lexer.currentTokenIs(MC_TOK_COMMA))
    {
        p->lexer.nextToken();
        if(trailingcommaallowed && p->lexer.currentTokenIs(endtoken))
        {
            break;
        }
        argexpr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
        if(!argexpr)
        {
            goto err;
        }
        ok = res->push(argexpr);
        if(!ok)
        {
            mcastexpression_t::destroyExpression(argexpr);
            goto err;
        }
    }
    if(!p->lexer.expectCurrent(endtoken))
    {
        goto err;
    }
    p->lexer.nextToken();
    return res;
err:
    PtrList::destroy(res, (mcitemdestroyfn_t)mcastexpression_t::destroyExpression);
    return nullptr;
}

mcastexpression_t* mc_parser_parseindexexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastexpression_t* res;
    mcastexpression_t* index;
    p->lexer.nextToken();
    index = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!index)
    {
        return nullptr;
    }
    if(!p->lexer.expectCurrent(MC_TOK_RBRACKET))
    {
        mcastexpression_t::destroyExpression(index);
        return nullptr;
    }
    p->lexer.nextToken();
    res = mc_astexpr_makeindexexpr(p->pstate, left, index, false);
    if(!res)
    {
        mcastexpression_t::destroyExpression(index);
        return nullptr;
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
    source = nullptr;
    assigntype = p->lexer.m_currtoken.toktype;
    p->lexer.nextToken();
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
                leftcopy = mcastexpression_t::copyExpression(left);
                if(!leftcopy)
                {
                    goto err;
                }
                pos = source->pos;
                newsource = mc_astexpr_makeinfixexpr(p->pstate, op, leftcopy, source);
                if(!newsource)
                {
                    mcastexpression_t::destroyExpression(leftcopy);
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
    mcastexpression_t::destroyExpression(source);
    return nullptr;
}

mcastexpression_t* mc_parser_parselogicalexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastmathoptype_t op;
    mcastprecedence_t prec;
    mcastexpression_t* res;
    mcastexpression_t* right;
    op = mc_parser_tokentomathop(p->lexer.m_currtoken.toktype);
    prec = mc_parser_getprecedence(p->lexer.m_currtoken.toktype);
    p->lexer.nextToken();
    right = mc_parser_parseexpression(p, prec);
    if(!right)
    {
        return nullptr;
    }
    res = mc_astexpr_makelogicalexpr(p->pstate, op, left, right);
    if(!res)
    {
        mcastexpression_t::destroyExpression(right);
        return nullptr;
    }
    return res;
}

mcastexpression_t* mc_parser_parseternaryexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastexpression_t* res;
    mcastexpression_t* ift;
    mcastexpression_t* iffalse;
    p->lexer.nextToken();
    ift = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!ift)
    {
        return nullptr;
    }
    if(!p->lexer.expectCurrent(MC_TOK_COLON))
    {
        mcastexpression_t::destroyExpression(ift);
        return nullptr;
    }
    p->lexer.nextToken();
    iffalse = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!iffalse)
    {
        mcastexpression_t::destroyExpression(ift);
        return nullptr;
    }
    res = mc_astexpr_maketernaryexpr(p->pstate, left, ift, iffalse);
    if(!res)
    {
        mcastexpression_t::destroyExpression(ift);
        mcastexpression_t::destroyExpression(iffalse);
        return nullptr;
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
    source = nullptr;
    operationtype = p->lexer.m_currtoken.toktype;
    pos = p->lexer.m_currtoken.pos;
    p->lexer.nextToken();
    op = mc_parser_tokentomathop(operationtype);
    dest = mc_parser_parseexpression(p, MC_ASTPREC_PREFIX);
    if(!dest)
    {
        goto err;
    }
    oneliteral = mc_astexpr_makeliteralnumber(p->pstate, 1);
    if(!oneliteral)
    {
        mcastexpression_t::destroyExpression(dest);
        goto err;
    }
    oneliteral->pos = pos;
    destcopy = mcastexpression_t::copyExpression(dest);
    if(!destcopy)
    {
        mcastexpression_t::destroyExpression(oneliteral);
        mcastexpression_t::destroyExpression(dest);
        goto err;
    }
    operation = mc_astexpr_makeinfixexpr(p->pstate, op, destcopy, oneliteral);
    if(!operation)
    {
        mcastexpression_t::destroyExpression(destcopy);
        mcastexpression_t::destroyExpression(dest);
        mcastexpression_t::destroyExpression(oneliteral);
        goto err;
    }
    operation->pos = pos;
    res = mc_astexpr_makeassignexpr(p->pstate, dest, operation, false);
    if(!res)
    {
        mcastexpression_t::destroyExpression(dest);
        mcastexpression_t::destroyExpression(operation);
        goto err;
    }
    return res;
err:
    mcastexpression_t::destroyExpression(source);
    return nullptr;
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
    source = nullptr;
    operationtype = p->lexer.m_currtoken.toktype;
    pos = p->lexer.m_currtoken.pos;
    p->lexer.nextToken();
    op = mc_parser_tokentomathop(operationtype);
    leftcopy = mcastexpression_t::copyExpression(left);
    if(!leftcopy)
    {
        goto err;
    }
    oneliteral = mc_astexpr_makeliteralnumber(p->pstate, 1);
    if(!oneliteral)
    {
        mcastexpression_t::destroyExpression(leftcopy);
        goto err;
    }
    oneliteral->pos = pos;
    operation = mc_astexpr_makeinfixexpr(p->pstate, op, leftcopy, oneliteral);
    if(!operation)
    {
        mcastexpression_t::destroyExpression(oneliteral);
        mcastexpression_t::destroyExpression(leftcopy);
        goto err;
    }
    operation->pos = pos;
    res = mc_astexpr_makeassignexpr(p->pstate, left, operation, true);
    if(!res)
    {
        mcastexpression_t::destroyExpression(operation);
        goto err;
    }
    return res;
err:
    mcastexpression_t::destroyExpression(source);
    return nullptr;
}

mcastexpression_t* mc_parser_parsedotexpression(mcastparser_t* p, mcastexpression_t* left)
{
    size_t len;
    char* str;
    mcastexpression_t* res;
    mcastexpression_t* index;
    p->lexer.nextToken();
    if(!p->lexer.expectCurrent(MC_TOK_IDENT))
    {
        return nullptr;
    }
    str = mc_asttoken_dupliteralstring(&p->lexer.m_currtoken);
    len = mc_util_strlen(str);
    index = mc_astexpr_makeliteralstring(p->pstate, str, len);
    if(!index)
    {
        mc_memory_free(str);
        return nullptr;
    }
    index->pos = p->lexer.m_currtoken.pos;
    p->lexer.nextToken();
    res = mc_astexpr_makeindexexpr(p->pstate, left, index, true);
    if(!res)
    {
        mcastexpression_t::destroyExpression(index);
        return nullptr;
    }
    return res;
}


mcastexpression_t* mc_optimizer_optexpression(mcastexpression_t* expr)
{
    switch(expr->exprtype)
    {
        case MC_EXPR_INFIX:
            return mc_optimizer_optinfixexpr(expr);
        case MC_EXPR_PREFIX:
            return mc_optimizer_optprefixexpr(expr);
        default:
            break;
    }
    return nullptr;
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
    res = nullptr;
    leftisnumeric = left->exprtype == MC_EXPR_NUMBERLITERAL || left->exprtype == MC_EXPR_BOOLLITERAL;
    rightisnumeric = right->exprtype == MC_EXPR_NUMBERLITERAL || right->exprtype == MC_EXPR_BOOLLITERAL;
    leftisstring = left->exprtype == MC_EXPR_STRINGLITERAL;
    rightisstring = right->exprtype == MC_EXPR_STRINGLITERAL;
    if(leftisnumeric && rightisnumeric)
    {
        dnleft = left->exprtype == MC_EXPR_NUMBERLITERAL ? left->uexpr.exprlitnumber : left->uexpr.exprlitbool;
        dnright = right->exprtype == MC_EXPR_NUMBERLITERAL ? right->uexpr.exprlitnumber : right->uexpr.exprlitbool;
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
        resstr = mc_util_stringallocfmt("%s%s", strleft, strright);
        len = mc_util_strlen(resstr);
        if(resstr)
        {
            res = mc_astexpr_makeliteralstring(state, resstr, len);
            if(!res)
            {
                mc_memory_free(resstr);
            }
        }
    }
    mcastexpression_t::destroyExpression(leftoptimized);
    mcastexpression_t::destroyExpression(rightoptimized);
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
    res = nullptr;
    if(expr->uexpr.exprprefix.op == MC_MATHOP_MINUS && right->exprtype == MC_EXPR_NUMBERLITERAL)
    {
        res = mc_astexpr_makeliteralnumber(expr->pstate, -right->uexpr.exprlitnumber);
    }
    else if(expr->uexpr.exprprefix.op == MC_MATHOP_BANG && right->exprtype == MC_EXPR_BOOLLITERAL)
    {
        res = mc_astexpr_makeliteralbool(expr->pstate, !right->uexpr.exprlitbool);
    }
    mcastexpression_t::destroyExpression(rightoptimized);
    if(res)
    {
        res->pos = expr->pos;
    }
    return res;
}


mcastscopecomp_t* mc_astcompscope_make(mcstate_t* state, mcastscopecomp_t* outer)
{
    mcastscopecomp_t* scope;
    scope = Memory::make<mcastscopecomp_t>();
    if(!scope)
    {
        return nullptr;
    }
    memset(scope, 0, sizeof(mcastscopecomp_t));
    scope->pstate = state;
    scope->outer = outer;
    scope->compiledscopebytecode = Memory::make<PtrList>(sizeof(uint16_t), false);
    if(!scope->compiledscopebytecode)
    {
        goto scopemakefailed;
    }
    scope->scopesrcposlist = Memory::make<PtrList>(sizeof(mcastlocation_t), false);
    if(!scope->scopesrcposlist)
    {
        goto scopemakefailed;
    }
    scope->ipstackbreak = Memory::make<PtrList>(sizeof(int), false);
    if(!scope->ipstackbreak)
    {
        goto scopemakefailed;
    }
    scope->ipstackcontinue = Memory::make<PtrList>(sizeof(int), false);
    if(!scope->ipstackcontinue)
    {
        goto scopemakefailed;
    }
    return scope;
scopemakefailed:
    mc_astcompscope_destroy(scope);
    return nullptr;
}

void mc_astcompscope_destroy(mcastscopecomp_t* scope)
{
    PtrList::destroy(scope->ipstackcontinue, nullptr);
    PtrList::destroy(scope->ipstackbreak, nullptr);
    PtrList::destroy(scope->compiledscopebytecode, nullptr);
    PtrList::destroy(scope->scopesrcposlist, nullptr);
    mc_memory_free(scope);
}

mccompiledprogram_t* mc_astcompscope_orphanresult(mcastscopecomp_t* scope)
{
    uint16_t* bcdata;
    mcastlocation_t* astlocdata;
    mccompiledprogram_t* res;
    bcdata = (uint16_t*)scope->compiledscopebytecode->data();
    astlocdata = (mcastlocation_t*)scope->scopesrcposlist->data();
    res = mc_astcompresult_make(scope->pstate, bcdata, astlocdata, scope->compiledscopebytecode->count());
    if(!res)
    {
        return nullptr;
    }
    PtrList::orphanData(scope->compiledscopebytecode);
    PtrList::orphanData(scope->scopesrcposlist);
    return res;
}

mccompiledprogram_t* mc_astcompresult_make(mcstate_t* state, uint16_t* bytecode, mcastlocation_t* srcposlist, int count)
{
    mccompiledprogram_t* res;
    res = Memory::make<mccompiledprogram_t>();
    if(!res)
    {
        return nullptr;
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
    if(res != nullptr)
    {
        mc_memory_free(res->bytecode);
        mc_memory_free(res->progsrcposlist);
        mc_memory_free(res);
    }
}


bool mc_compiler_init(mcastcompiler_t* comp, mcstate_t* state, mcconfig_t* cfg, mcgcmemory_t* mem, mcerrlist_t* errors, PtrList* files, mcglobalstore_t* gstor)
{
    bool ok;
    const char* filename;
    memset(comp, 0, sizeof(mcastcompiler_t));
    comp->pstate = state;
    comp->config = cfg;
    comp->mem = mem;
    comp->errors = errors;
    comp->files = files;
    comp->compglobalstore = gstor;
    comp->filescopelist = Memory::make<PtrList>(sizeof(void*), true);
    if(!comp->filescopelist)
    {
        goto compilerinitfailed;
    }
    comp->constants = Memory::make<GenericList<mcvalue_t>>(0, mc_value_makenull());
    if(!comp->constants)
    {
        goto compilerinitfailed;
    }
    comp->srcposstack = Memory::make<PtrList>(sizeof(mcastlocation_t), false);
    if(!comp->srcposstack)
    {
        goto compilerinitfailed;
    }
    comp->modules = Memory::make<PtrDict>((mcitemcopyfn_t)mc_module_copy, (mcitemdestroyfn_t)mc_module_destroy);
    if(!comp->modules)
    {
        goto compilerinitfailed;
    }
    ok = mc_compiler_pushcompilationscope(comp);
    if(!ok)
    {
        goto compilerinitfailed;
    }
    filename = "<none>";
    if(files->count() > 0)
    {
        filename = (const char*)files->top();
    }
    #if 1
    ok = mc_compiler_filescopepush(comp, filename);
    if(!ok)
    {
        goto compilerinitfailed;
    }
    #endif
    comp->stringconstposdict = Memory::make<PtrDict>(nullptr, nullptr);
    if(!comp->stringconstposdict)
    {
        goto compilerinitfailed;
    }

    return true;
compilerinitfailed:
    mc_compiler_deinit(comp);
    return false;
}

void mc_compiler_deinit(mcastcompiler_t* comp)
{
    size_t i;
    int* val;
    if(comp != nullptr)
    {
        for(i = 0; i < comp->stringconstposdict->count(); i++)
        {
            val = (int*)comp->stringconstposdict->getValueAt(i);
            mc_memory_free(val);
        }
        Memory::destroy(comp->stringconstposdict);
        while(comp->filescopelist->count() > 0)
        {
            mc_compiler_filescopepop(comp);
        }
        while(mc_compiler_getcompilationscope(comp))
        {
            mc_compiler_popcompilationscope(comp);
        }
        PtrDict::destroyItemsAndDict(comp->modules);
        PtrList::destroy(comp->srcposstack, nullptr);
        Memory::destroy(comp->constants);
        PtrList::destroy(comp->filescopelist, nullptr);
        memset(comp, 0, sizeof(mcastcompiler_t));
    }
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
    PtrDict* modulescopy;
    GenericList<mcvalue_t>* constantscopy;
    PtrList* srcloadedmodulenames;
    PtrList* copyloadedmodulenames;
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
    //MC_ASSERT(src->filescopelist->count() == 1);
    MC_ASSERT(srcst->outer == nullptr);
    srcstocopy = srcst->copy();
    if(!srcstocopy)
    {
        goto compilercopyfailed;
    }
    copyst = mc_compiler_getsymtable(copy);
    mcastsymtable_t::destroy(copyst);
    copyst = nullptr;
    mc_compiler_setsymtable(copy, srcstocopy);
    modulescopy = src->modules->copy();
    if(!modulescopy)
    {
        goto compilercopyfailed;
    }
    PtrDict::destroyItemsAndDict(copy->modules);
    copy->modules = modulescopy;
    constantscopy = GenericList<mcvalue_t>::copy(src->constants);
    if(!constantscopy)
    {
        goto compilercopyfailed;
    }
    Memory::destroy(copy->constants);
    copy->constants = constantscopy;
    for(i = 0; i < src->stringconstposdict->count(); i++)
    {
        key = src->stringconstposdict->getKeyAt(i);
        val = (int*)src->stringconstposdict->getValueAt(i);
        valcopy = (int*)mc_memory_malloc(sizeof(int));
        if(!valcopy)
        {
            goto compilercopyfailed;
        }
        *valcopy = *val;
        ok = copy->stringconstposdict->set(key, valcopy);
        if(!ok)
        {
            mc_memory_free(valcopy);
            goto compilercopyfailed;
        }
    }
    srcfilescope = (mcastscopefile_t*)src->filescopelist->top();
    copyfilescope = (mcastscopefile_t*)copy->filescopelist->top();
    srcloadedmodulenames = srcfilescope->loadedmodnames;
    copyloadedmodulenames = copyfilescope->loadedmodnames;
    for(i = 0; i < srcloadedmodulenames->count(); i++)
    {
        loadedname = (const char*)srcloadedmodulenames->get(i);
        loadednamecopy = mc_util_strdup(loadedname);
        if(!loadednamecopy)
        {
            goto compilercopyfailed;
        }
        ok = copyloadedmodulenames->push(loadednamecopy);
        if(!ok)
        {
            mc_memory_free(loadednamecopy);
            goto compilercopyfailed;
        }
    }
    return true;
compilercopyfailed:
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
        makecase(def, MC_OPCODE_HALT, "MC_OPCODE_HALT", 0, 0, 0);
        makecase(def, MC_OPCODE_CONSTANT, "MC_OPCODE_CONSTANT", 1, 2, 0);
        makecase(def, MC_OPCODE_ADD, "MC_OPCODE_ADD", 0, 0, 0);
        makecase(def, MC_OPCODE_POP, "MC_OPCODE_POP", 0, 0, 0);
        makecase(def, MC_OPCODE_SUB, "MC_OPCODE_SUB", 0, 0, 0);
        makecase(def, MC_OPCODE_MUL, "MC_OPCODE_MUL", 0, 0, 0);
        makecase(def, MC_OPCODE_DIV, "MC_OPCODE_DIV", 0, 0, 0);
        makecase(def, MC_OPCODE_MOD, "MC_OPCODE_MOD", 0, 0, 0);
        makecase(def, MC_OPCODE_TRUE, "MC_OPCODE_TRUE", 0, 0, 0);
        makecase(def, MC_OPCODE_FALSE, "MC_OPCODE_FALSE", 0, 0, 0);
        makecase(def, MC_OPCODE_COMPARE, "MC_OPCODE_COMPARE", 0, 0, 0);
        makecase(def, MC_OPCODE_COMPAREEQ, "MC_OPCODE_COMPAREEQ", 0, 0, 0);
        makecase(def, MC_OPCODE_EQUAL, "MC_OPCODE_EQUAL", 0, 0, 0);
        makecase(def, MC_OPCODE_NOTEQUAL, "MC_OPCODE_NOTEQUAL", 0, 0, 0);
        makecase(def, MC_OPCODE_GREATERTHAN, "MC_OPCODE_GREATERTHAN", 0, 0, 0);
        makecase(def, MC_OPCODE_GREATERTHANEQUAL, "MC_OPCODE_GREATERTHANEQUAL", 0, 0, 0);
        makecase(def, MC_OPCODE_MINUS, "MC_OPCODE_MINUS", 0, 0, 0);
        makecase(def, MC_OPCODE_BINNOT, "MC_OPCODE_BINNOT", 0, 0, 0);
        makecase(def, MC_OPCODE_BANG, "MC_OPCODE_BANG", 0, 0, 0);
        makecase(def, MC_OPCODE_JUMP, "MC_OPCODE_JUMP", 1, 2, 0);
        makecase(def, MC_OPCODE_JUMPIFFALSE, "MC_OPCODE_JUMPIFFALSE", 1, 2, 0);
        makecase(def, MC_OPCODE_JUMPIFTRUE, "MC_OPCODE_JUMPIFTRUE", 1, 2, 0);
        makecase(def, MC_OPCODE_NULL, "MC_OPCODE_NULL", 0, 0, 0);
        makecase(def, MC_OPCODE_GETMODULEGLOBAL, "MC_OPCODE_GETMODULEGLOBAL", 1, 2, 0);
        makecase(def, MC_OPCODE_SETMODULEGLOBAL, "MC_OPCODE_SETMODULEGLOBAL", 1, 2, 0);
        makecase(def, MC_OPCODE_DEFINEMODULEGLOBAL, "MC_OPCODE_DEFINEMODULEGLOBAL", 1, 2, 0);
        makecase(def, MC_OPCODE_ARRAY, "MC_OPCODE_ARRAY", 1, 2, 0);
        makecase(def, MC_OPCODE_MAPSTART, "MC_OPCODE_MAPSTART", 1, 2, 0);
        makecase(def, MC_OPCODE_MAPEND, "MC_OPCODE_MAPEND", 1, 2, 0);
        makecase(def, MC_OPCODE_GETTHIS, "MC_OPCODE_GETTHIS", 0, 0, 0);
        makecase(def, MC_OPCODE_GETINDEX, "MC_OPCODE_GETINDEX", 0, 0, 0);
        makecase(def, MC_OPCODE_SETINDEX, "MC_OPCODE_SETINDEX", 0, 0, 0);
        makecase(def, MC_OPCODE_GETDOTINDEX, "MC_OPCODE_GETDOTINDEX", 0, 0, 0);
        makecase(def, MC_OPCODE_GETVALUEAT, "MC_OPCODE_GETVALUEAT", 0, 0, 0);
        makecase(def, MC_OPCODE_CALL, "MC_OPCODE_CALL", 1, 1, 0);
        makecase(def, MC_OPCODE_RETURNVALUE, "MC_OPCODE_RETURNVALUE", 0, 0, 0);
        makecase(def, MC_OPCODE_RETURN, "MC_OPCODE_RETURN", 0, 0, 0);
        makecase(def, MC_OPCODE_GETLOCAL, "MC_OPCODE_GETLOCAL", 1, 1, 0);
        makecase(def, MC_OPCODE_DEFINELOCAL, "MC_OPCODE_DEFINELOCAL", 1, 1, 0);
        makecase(def, MC_OPCODE_SETLOCAL, "MC_OPCODE_SETLOCAL", 1, 1, 0);
        makecase(def, MC_OPCODE_GETGLOBALBUILTIN, "MC_OPCODE_GETGLOBALBUILTIN", 1, 2, 0);
        makecase(def, MC_OPCODE_FUNCTION, "MC_OPCODE_FUNCTION", 2, 2, 1);
        makecase(def, MC_OPCODE_GETFREE, "MC_OPCODE_GETFREE", 1, 1, 0);
        makecase(def, MC_OPCODE_SETFREE, "MC_OPCODE_SETFREE", 1, 1, 0);
        makecase(def, MC_OPCODE_CURRENTFUNCTION, "MC_OPCODE_CURRENTFUNCTION", 0, 0, 0);
        makecase(def, MC_OPCODE_DUP, "MC_OPCODE_DUP", 0, 0, 0);
        makecase(def, MC_OPCODE_NUMBER, "MC_OPCODE_NUMBER", 1, 8, 0);
        makecase(def, MC_OPCODE_FOREACHLEN, "MC_OPCODE_FOREACHLEN", 0, 0, 0);
        makecase(def, MC_OPCODE_SETRECOVER, "MC_OPCODE_SETRECOVER", 1, 2, 0);
        makecase(def, MC_OPCODE_BINOR, "MC_OPCODE_BINOR", 0, 0, 0);
        makecase(def, MC_OPCODE_BINXOR, "MC_OPCODE_BINXOR", 0, 0, 0);
        makecase(def, MC_OPCODE_BINAND, "MC_OPCODE_BINAND", 0, 0, 0);
        makecase(def, MC_OPCODE_LSHIFT, "MC_OPCODE_LSHIFT", 0, 0, 0);
        makecase(def, MC_OPCODE_RSHIFT, "MC_OPCODE_RSHIFT", 0, 0, 0);
        makecase(def, MC_OPCODE_MAX, "MC_OPCODE_MAX", 0, 0, 0);
        default:
            {
                return nullptr; 
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


#define APPEND_BYTE(n) \
    do \
    { \
        val = (uint16_t)(operands[i] >> (n * 8)); \
        ok = res->push(&val); \
        if(!ok) \
        { \
            return 0; \
        } \
    } while(0)

int mc_compiler_gencode(mcinternopcode_t op, int operandscount, const uint64_t* operands, PtrList* res)
{
    bool ok;
    int i;
    int width;
    int instrlen;
    uint16_t val;
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
    ok = res->push(&val);
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
        srcpos = (mcastlocation_t*)comp->srcposstack->top();
        /*
        MC_ASSERT(srcpos->line >= 0);
        MC_ASSERT(srcpos->column >= 0);
        */
        ok = mc_compiler_getsrcpositions(comp)->push(srcpos);
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
    filescope = (mcastscopefile_t*)comp->filescopelist->top();
    if(!filescope)
    {
        MC_ASSERT(false);
        return false;
    }
    currenttable = filescope->filesymtab;
    filescope->filesymtab = mcastsymtable_t::make(comp->pstate, currenttable, comp->compglobalstore, globaloffset);
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
    filescope = (mcastscopefile_t*)comp->filescopelist->top();
    if(filescope != nullptr)
    {
        currenttable = filescope->filesymtab;
        if(currenttable != nullptr)
        {
            filescope->filesymtab = currenttable->outer;
            mcastsymtable_t::destroy(currenttable);
        }
    }
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
    PtrList* statements;
    mcastscopefile_t* filescope;
    state = comp->pstate;
    filescope = (mcastscopefile_t*)comp->filescopelist->top();
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
    PtrList::destroy(statements, (mcitemdestroyfn_t)mcastexpression_t::destroyExpression);
    if(comp->pstate->config.dumpbytecode)
    {
        mc_printer_printbytecode(state->stderrprinter,
            (uint16_t*)comp->compilationscope->compiledscopebytecode->data(),
            (mcastlocation_t*)comp->compilationscope->scopesrcposlist->data(),
            comp->compilationscope->compiledscopebytecode->count(), false);
    }
    return ok;
}

bool mc_compiler_compilestmtlist(mcastcompiler_t* comp, PtrList* statements)
{
    bool ok;
    size_t i;
    mcastexpression_t* expr;
    ok = true;
    for(i = 0; i < statements->count(); i++)
    {
        expr = (mcastexpression_t*)statements->get(i);
        ok = mc_compiler_compileexpression(comp, expr);
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
    Printer* filepathbuf;
    mcastsymtable_t* symtab;
    mcastscopefile_t* fs;
    mcmodule_t* module;
    mcastsymtable_t* st;
    mcastscopefile_t* filescope;
    mcastsymbol_t* symbol;
    /* todo: split into smaller functions */
    result = false;
    filepath = nullptr;
    code = nullptr;
    filescope = (mcastscopefile_t*)comp->filescopelist->top();
    modpath = importstmt->uexpr.exprimportstmt.path;
    modname = mc_util_getmodulename(modpath);
    for(i = 0; i < filescope->loadedmodnames->count(); i++)
    {
        loadedname = (const char*)filescope->loadedmodnames->get(i);
        if(mc_util_strequal(loadedname, modname))
        {
            if(comp->pstate->config.fatalcomplaints)
            {
                comp->errors->pushFormat(MC_ERROR_COMPILING, importstmt->pos, "module \"%s\" was already imported", modname);
                result = false;
            }
            else
            {
                mc_util_complain(importstmt->pos, "module \"%s\" already imported; ignoring 'import' statement", modname);
                result = true;
            }
            goto end;
        }
    }
    filepathbuf = Printer::make(comp->pstate, nullptr);
    if(!filepathbuf)
    {
        result = false;
        goto end;
    }
    if(mc_util_pathisabsolute(modpath))
    {
        filepathbuf->format("%s.mc", modpath);
    }
    else
    {
        filepathbuf->format("%s%s.mc", filescope->file->dir_path, modpath);
    }

    if(filepathbuf->m_prfailed)
    {
        Printer::destroy(filepathbuf);
        result = false;
        goto end;
    }
    filepathnoncanonicalised = filepathbuf->getString();
    filepath = mc_util_canonpath(comp->pstate, filepathnoncanonicalised);
    Printer::destroy(filepathbuf);
    if(!filepath)
    {
        result = false;
        goto end;
    }
    symtab = mc_compiler_getsymtable(comp);
    if(symtab->outer != nullptr || symtab->blockscopes->count() > 1)
    {
        comp->errors->pushFormat(MC_ERROR_COMPILING, importstmt->pos, "modules can only be imported in global scope");
        result = false;
        goto end;
    }
    for(i = 0; i < comp->filescopelist->count(); i++)
    {
        fs = (mcastscopefile_t*)comp->filescopelist->get(i);
        if(mc_util_strequal(fs->file->path, filepath))
        {
            comp->errors->pushFormat(MC_ERROR_COMPILING, importstmt->pos, "cyclic reference of file \"%s\"", filepath);
            result = false;
            goto end;
        }
    }
    module = (mcmodule_t*)comp->modules->get(filepath);
    if(!module)
    {
        /* todo: create new module function */
        searchedpath = mc_module_findfile(comp->pstate, filepath);
        code = mc_fsutil_fileread(comp->pstate, searchedpath, &flen);
        if(!code)
        {
            comp->errors->pushFormat(MC_ERROR_COMPILING, importstmt->pos, "reading module file \"%s\" failed", filepath);
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
        for(i = 0; i < st->getModuleGlobalSymCount(); i++)
        {
            symbol = st->getModuleGlobalSymAt(i);
            mc_module_addsymbol(module, symbol);
        }
        mc_compiler_filescopepop(comp);
        ok = comp->modules->set(filepath, module);
        if(!ok)
        {
            mc_module_destroy(module);
            result = false;
            goto end;
        }
    }
    for(i = 0; i < module->modsymbols->count(); i++)
    {
        symbol = (mcastsymbol_t*)module->modsymbols->get(i);
        ok = symtab->addModuleSymbol(symbol);
        if(!ok)
        {
            result = false;
            goto end;
        }
    }
    namecopy = mc_util_strdup(modname);
    if(!namecopy)
    {
        result = false;
        goto end;
    }
    ok = filescope->loadedmodnames->push(namecopy);
    if(!ok)
    {
        mc_memory_free(namecopy);
        result = false;
        goto end;
    }
    result = true;
end:
    mc_memory_free(filepath);
    mc_memory_free(code);
    return result;
}

mcastsymbol_t* mc_compiler_defsymbol(mcastcompiler_t* comp, mcastlocation_t pos, const char* name, bool assignable, bool canshadow)
{
    mcastsymbol_t* symbol;
    mcastsymbol_t* currentsymbol;
    mcastsymtable_t* symtab;
    symtab = mc_compiler_getsymtable(comp);
    if(!canshadow && !symtab->isTopGlobalScope())
    {
        currentsymbol = symtab->resolve(name);
        if(currentsymbol)
        {
            comp->errors->pushFormat(MC_ERROR_COMPILING, pos, "symbol \"%s\" is already defined", name);
            return nullptr;
        }
    }
    symbol = symtab->defineSymbol(name, assignable);
    if(!symbol)
    {
        comp->errors->pushFormat(MC_ERROR_COMPILING, pos, "cannot define symbol \"%s\"", name);
        return nullptr;
    }
    return symbol;
}

bool mc_compiler_compiledefine(mcastcompiler_t* comp, mcastexpression_t* expr)
{
    bool ok;
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
    return true;
}

bool mc_compiler_compileifstmt(mcastcompiler_t* comp, uint64_t* opbuf, mcastexpression_t* expr)
{
    bool ok;
    size_t i;
    int afteraltip;
    int nextcasejumpip;
    int jumptoendip;
    int afterelifip;
    int* pos;
    mcastexprifcase_t* ifcase;
    mcastexprstmtif_t* ifstmt;
    PtrList* jumptoendips;
    ifstmt = &expr->uexpr.exprifstmt;
    jumptoendips = Memory::make<PtrList>(sizeof(int), false);
    if(!jumptoendips)
    {
        goto statementiferror;
    }
    for(i = 0; i < ifstmt->cases->count(); i++)
    {
        ifcase = (mcastexprifcase_t*)ifstmt->cases->get(i);
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
        if(i < (ifstmt->cases->count() - 1) || ifstmt->alternative)
        {
            opbuf[0] = 0xbeef;
            jumptoendip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
            ok = jumptoendips->push(&jumptoendip);
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
    for(i = 0; i < jumptoendips->count(); i++)
    {
        pos = (int*)jumptoendips->get(i);
        mc_compiler_changeuint16operand(comp, *pos + 1, afteraltip);
    }
    PtrList::destroy(jumptoendips, nullptr);
    return true;
statementiferror:
    PtrList::destroy(jumptoendips, nullptr);
    return false;
}

bool mc_compiler_compilereturnstmt(mcastcompiler_t* comp, mcastscopecomp_t* compscope, mcastexpression_t* expr)
{
    bool ok;
    int ip;
    if(compscope->outer == nullptr)
    {
        comp->errors->pushFormat( MC_ERROR_COMPILING, expr->pos, "nothing to return from");
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
        ip = mc_compiler_emit(comp, MC_OPCODE_RETURNVALUE, 0, nullptr);
    }
    else
    {
        ip = mc_compiler_emit(comp, MC_OPCODE_RETURN, 0, nullptr);
    }
    if(ip < 0)
    {
        return false;
    }
    return true;
}

bool mc_compiler_compilewhilestmt(mcastcompiler_t* comp, uint64_t* opbuf, mcastexpression_t* expr)
{
    bool ok;
    int ip;
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
    return true;
}

bool mc_compiler_compilebreakstmt(mcastcompiler_t* comp, uint64_t* opbuf, mcastexpression_t* expr)
{
    int ip;
    int breakip;
    breakip = mc_compiler_getbreakip(comp);
    if(breakip < 0)
    {
        comp->errors->pushFormat(MC_ERROR_COMPILING, expr->pos, "nothing to break from.");
        return false;
    }
    opbuf[0] = breakip;
    ip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
    if(ip < 0)
    {
        return false;
    }
    return true;
}

bool mc_compiler_compilecontinuestmt(mcastcompiler_t* comp, uint64_t* opbuf, mcastexpression_t* expr)
{
    int ip;
    int continueip;
    continueip = mc_compiler_getcontinueip(comp);
    if(continueip < 0)
    {
        comp->errors->pushFormat(MC_ERROR_COMPILING, expr->pos, "nothing to continue from.");
        return false;
    }
    opbuf[0] = continueip;
    ip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
    if(ip < 0)
    {
        return false;
    }
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
    exproptimized = nullptr;
    #if 0
    exproptimized = mc_optimizer_optexpression(expr);
    if(exproptimized != nullptr)
    {
        expr = exproptimized;
    }
    #endif
    ok = comp->srcposstack->push(&expr->pos);
    if(!ok)
    {
        return false;
    }
    compscope = mc_compiler_getcompilationscope(comp);
    symtab = mc_compiler_getsymtable(comp);
    res = false;
    switch(expr->exprtype)
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
                            comp->errors->pushFormat(MC_ERROR_COMPILING, expr->pos, "unknown infix operator");
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
                            ip = mc_compiler_emit(comp, MC_OPCODE_COMPAREEQ, 0, nullptr);
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
                            ip = mc_compiler_emit(comp, MC_OPCODE_COMPARE, 0, nullptr);
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
                ip = mc_compiler_emit(comp, op, 0, nullptr);
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
                currentpos = (int*)comp->stringconstposdict->get(expr->uexpr.exprlitstring.data);
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
                    posval = (int*)mc_memory_malloc(sizeof(int));
                    if(!posval)
                    {
                        goto error;
                    }
                    *posval = pos;
                    ok = comp->stringconstposdict->set(expr->uexpr.exprlitstring.data, posval);
                    if(!ok)
                    {
                        mc_memory_free(posval);
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
                ip = mc_compiler_emit(comp, MC_OPCODE_NULL, 0, nullptr);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_BOOLLITERAL:
            {
                ip = mc_compiler_emit(comp, expr->uexpr.exprlitbool ? MC_OPCODE_TRUE : MC_OPCODE_FALSE, 0, nullptr);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_ARRAYLITERAL:
            {
                size_t i;
                for(i = 0; i < expr->uexpr.exprlitarray.litarritems->count(); i++)
                {
                    ok = mc_compiler_compileexpression(comp, (mcastexpression_t*)expr->uexpr.exprlitarray.litarritems->get(i));
                    if(!ok)
                    {
                        goto error;
                    }
                }
                opbuf[0] = expr->uexpr.exprlitarray.litarritems->count();
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
                mcastexprliteralmap_t* map;
                map = &expr->uexpr.exprlitmap;
                len = map->litmapkeys->count();
                opbuf[0] = len;
                ip = mc_compiler_emit(comp, MC_OPCODE_MAPSTART, 1, opbuf);
                if(ip < 0)
                {
                    goto error;
                }
                for(i = 0; i < len; i++)
                {
                    key = (mcastexpression_t*)map->litmapkeys->get(i);
                    val = (mcastexpression_t*)map->litmapvalues->get(i);
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
                            comp->errors->pushFormat(MC_ERROR_COMPILING, expr->pos, "unknown prefix operator.");
                            goto error;
                        }
                        break;
                }
                ip = mc_compiler_emit(comp, op, 0, nullptr);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_IDENT:
            {
                mcastsymbol_t* symbol;
                mcastexprident_t* ident;
                ident = expr->uexpr.exprident;
                symbol = symtab->resolve(ident->value);
                if(!symbol)
                {
                    if(comp->pstate->config.strictmode)
                    {
                        comp->errors->pushFormat(MC_ERROR_COMPILING, ident->pos, "compilation: failed to resolve symbol \"%s\"", ident->value);
                        goto error;
                    }
                    else
                    {
                        symbol = mc_compiler_defsymbol(comp, ident->pos, ident->value, true, false);
                    }
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
                    ip = mc_compiler_emit(comp, MC_OPCODE_GETDOTINDEX, 0, nullptr);
                }
                else
                #endif
                {
                    ip = mc_compiler_emit(comp, MC_OPCODE_GETINDEX, 0, nullptr);
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
                PtrList* freesyms;
                mccompiledprogram_t* comp_res;
                mcastexprliteralfunction_t* fn;
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
                    fnsymbol = symtab->defineFunctionName(fn->name, false);
                    if(!fnsymbol)
                    {
                        comp->errors->pushFormat(MC_ERROR_COMPILING, expr->pos, "cannot define function name as \"%s\"", fn->name);
                        goto error;
                    }
                }
                thissymbol = symtab->defineThis();
                if(!thissymbol)
                {
                    comp->errors->pushFormat(MC_ERROR_COMPILING, expr->pos, "cannot define \"this\" symbol");
                    goto error;
                }
                for(i = 0; i < expr->uexpr.exprlitfunction.funcparamlist->count(); i++)
                {
                    param = (mcastfuncparam_t*)expr->uexpr.exprlitfunction.funcparamlist->get(i);
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
                    ip = mc_compiler_emit(comp, MC_OPCODE_RETURN, 0, nullptr);
                    if(ip < 0)
                    {
                        goto error;
                    }
                }
                freesyms = symtab->freesymbols;
                /* because it gets destroyed with compiler_pop_compilation_scope() */
                symtab->freesymbols = nullptr;
                nlocals = symtab->maxnumdefinitions;
                comp_res = mc_astcompscope_orphanresult(compscope);
                if(!comp_res)
                {
                    PtrList::destroy(freesyms, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
                    goto error;
                }
                mc_compiler_popsymtable(comp);
                mc_compiler_popcompilationscope(comp);
                compscope = mc_compiler_getcompilationscope(comp);
                symtab = mc_compiler_getsymtable(comp);
                obj = mc_value_makefuncscript(comp->pstate, fn->name, comp_res, true, nlocals, fn->funcparamlist->count(), 0);
                if(mc_value_isnull(obj))
                {
                    PtrList::destroy(freesyms, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
                    mc_astcompresult_destroy(comp_res);
                    goto error;
                }
                for(i = 0; i < freesyms->count(); i++)
                {
                    symbol = (mcastsymbol_t*)freesyms->get(i);
                    ok = mc_compiler_readsymbol(comp, symbol);
                    if(!ok)
                    {
                        PtrList::destroy(freesyms, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
                        goto error;
                    }
                }
                pos = mc_compiler_addconstant(comp, obj);
                if(pos < 0)
                {
                    PtrList::destroy(freesyms, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
                    goto error;
                }
                opbuf[0] = pos;
                opbuf[1] = freesyms->count();
                ip = mc_compiler_emit(comp, MC_OPCODE_FUNCTION, 2, opbuf);
                if(ip < 0)
                {
                    PtrList::destroy(freesyms, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
                    goto error;
                }
                PtrList::destroy(freesyms, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
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
                for(i = 0; i < expr->uexpr.exprcall.args->count(); i++)
                {
                    argexpr = (mcastexpression_t*)expr->uexpr.exprcall.args->get(i);
                    ok = mc_compiler_compileexpression(comp, argexpr);
                    if(!ok)
                    {
                        goto error;
                    }
                }
                opbuf[0] = expr->uexpr.exprcall.args->count();
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
                mcastexprident_t* ident;
                mcastsymbol_t* symbol;
                assign = &expr->uexpr.exprassign;
                if(assign->dest->exprtype != MC_EXPR_IDENT && assign->dest->exprtype != MC_EXPR_INDEX)
                {
                    comp->errors->pushFormat(MC_ERROR_COMPILING, assign->dest->pos, "expression is not assignable");
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
                ip = mc_compiler_emit(comp, MC_OPCODE_DUP, 0, nullptr);
                if(ip < 0)
                {
                    goto error;
                }
                ok = comp->srcposstack->push(&assign->dest->pos);
                if(!ok)
                {
                    goto error;
                }
                if(assign->dest->exprtype == MC_EXPR_IDENT)
                {
                    ident = assign->dest->uexpr.exprident;
                    symbol = symtab->resolve(ident->value);
                    if(!symbol)
                    {
                        if(comp->pstate->config.strictmode)
                        {
                            comp->errors->pushFormat(MC_ERROR_COMPILING, assign->dest->pos, "cannot assign to undeclared symbol \"%s\"", ident->value);
                            goto error;
                        }
                        else
                        {
                            symbol = mc_compiler_defsymbol(comp, ident->pos, ident->value, true, false);
                            if(!symbol)
                            {
                                comp->errors->pushFormat(MC_ERROR_COMPILING, assign->dest->pos, "failed to implicitly create symbol \"%s\"", ident->value);
                                goto error;
                            }
                        }
                    }
                    if(!symbol->assignable)
                    {
                        comp->errors->pushFormat(MC_ERROR_COMPILING, assign->dest->pos, "compilation: cannot assign to readonly symbol \"%s\"", ident->value);
                        goto error;
                    }
                    ok = mc_compiler_storesymbol(comp, symbol, false);
                    if(!ok)
                    {
                        goto error;
                    }
                }
                else if(assign->dest->exprtype == MC_EXPR_INDEX)
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
                    ip = mc_compiler_emit(comp, MC_OPCODE_SETINDEX, 0, nullptr);
                    if(ip < 0)
                    {
                        goto error;
                    }
                }
                if(assign->is_postfix)
                {
                    ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, nullptr);
                    if(ip < 0)
                    {
                        goto error;
                    }
                }
                comp->srcposstack->pop(nullptr);
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
                ip = mc_compiler_emit(comp, MC_OPCODE_DUP, 0, nullptr);
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
                ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, nullptr);
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

        case MC_EXPR_STMTEXPRESSION:
            {
                ok = mc_compiler_compileexpression(comp, expr->uexpr.exprexpression);
                if(!ok)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, nullptr);
                if(ip < 0)
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTDEFINE:
            {
                if(!mc_compiler_compiledefine(comp, expr))
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTIF:
            {
                if(!mc_compiler_compileifstmt(comp, opbuf, expr))
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTRETURN:
            {
                if(!mc_compiler_compilereturnstmt(comp, compscope, expr))
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTLOOPWHILE:
            {
                if(!mc_compiler_compilewhilestmt(comp, opbuf, expr))
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTBREAK:
            {
                if(!mc_compiler_compilebreakstmt(comp, opbuf, expr))
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTCONTINUE:
            {
                if(!mc_compiler_compilecontinuestmt(comp, opbuf, expr))
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
                ok = symtab->scopeBlockPush();
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
                sourcesymbol = nullptr;
                if(foreach->source->exprtype == MC_EXPR_IDENT)
                {
                    sourcesymbol = symtab->resolve(foreach->source->uexpr.exprident->value);
                    if(!sourcesymbol)
                    {
                        comp->errors->pushFormat(MC_ERROR_COMPILING, foreach->source->pos, "symbol \"%s\" could not be resolved", foreach->source->uexpr.exprident->value);
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
                ip = mc_compiler_emit(comp, MC_OPCODE_ADD, 0, nullptr);
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
                ok = comp->srcposstack->push(&foreach->source->pos);
                if(!ok)
                {
                    return false;
                }
                ok = mc_compiler_readsymbol(comp, sourcesymbol);
                if(!ok)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_FOREACHLEN, 0, nullptr);
                if(ip < 0)
                {
                    return false;
                }
                comp->srcposstack->pop(nullptr);
                ok = mc_compiler_readsymbol(comp, indexsymbol);
                if(!ok)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_COMPARE, 0, nullptr);
                if(ip < 0)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_EQUAL, 0, nullptr);
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
                ip = mc_compiler_emit(comp, MC_OPCODE_GETVALUEAT, 0, nullptr);
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
                symtab->scopeBlockPop();
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
                ok = symtab->scopeBlockPush();
                if(!ok)
                {
                    return false;
                }
                /* Init */
                jumptoafterupdateip = 0;
                ok = false;
                if(loop->init)
                {
                    ok = mc_compiler_compileexpression(comp, loop->init);
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
                    ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, nullptr);
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
                    ip = mc_compiler_emit(comp, MC_OPCODE_TRUE, 0, nullptr);
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
                symtab->scopeBlockPop();
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
                if(comp->pstate->config.strictmode)
                {
                    if(symtab->isModuleGlobalScope())
                    {
                        comp->errors->pushFormat(MC_ERROR_COMPILING, expr->pos, "recover statement cannot be defined in global scope");
                        return false;
                    }
                }
                #if 0
                if(!symtab->isTopBlockScope())
                {
                    comp->errors->pushFormat(MC_ERROR_COMPILING, expr->pos, "recover statement cannot be defined within other statements");
                    return false;
                }
                #endif
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
                ok = symtab->scopeBlockPush();
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
                    #if 0
                        comp->errors->pushFormat(MC_ERROR_COMPILING, expr->pos, "recover body must end with a return statement");
                        return false;
                    #else
                        mc_util_complain(expr->pos, "recover body should end with a return statement");
                    #endif
                }
                symtab->scopeBlockPop();
                afterrecoverip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, jumptoafterrecoverip + 1, afterrecoverip);
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
    comp->srcposstack->pop(nullptr);
    mcastexpression_t::destroyExpression(exproptimized);
    return res;
}

bool mc_compiler_compilecodeblock(mcastcompiler_t* comp, mcastexprcodeblock_t* block)
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
    ok = symtab->scopeBlockPush();
    if(!ok)
    {
        return false;
    }
    if(block->statements->count() == 0)
    {
        ip = mc_compiler_emit(comp, MC_OPCODE_NULL, 0, nullptr);
        if(ip < 0)
        {
            return false;
        }
        ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, nullptr);
        if(ip < 0)
        {
            return false;
        }
    }
    for(i = 0; i < block->statements->count(); i++)
    {
        expr = (mcastexpression_t*)block->statements->get(i);
        ok = mc_compiler_compileexpression(comp, expr);
        if(!ok)
        {
            return false;
        }
    }
    symtab->scopeBlockPop();
    return true;
}

int mc_compiler_addconstant(mcastcompiler_t* comp, mcvalue_t obj)
{
    bool ok;
    int pos;
    ok = comp->constants->push(obj);
    if(!ok)
    {
        return -1;
    }
    pos = comp->constants->count() - 1;
    return pos;
}

void mc_compiler_changeuint16operand(mcastcompiler_t* comp, int ip, uint16_t operand)
{
    uint16_t hi;
    uint16_t lo;
    PtrList* bytecode;
    bytecode = mc_compiler_getbytecode(comp);
    if((ip + 1) >= (int)bytecode->count())
    {
    }
    else
    {
        hi = operand >> 8;
        bytecode->set(ip, &hi);
        lo = operand;
        bytecode->set(ip + 1, &lo);
    }
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
    if(symbol == nullptr)
    {
        return false;
    }
    if(symbol->symtype == MC_SYM_MODULEGLOBAL)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_GETMODULEGLOBAL, 1, opbuf);
    }
    else if(symbol->symtype == MC_SYM_GLOBALBUILTIN)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_GETGLOBALBUILTIN, 1, opbuf);
    }
    else if(symbol->symtype == MC_SYM_LOCAL)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_GETLOCAL, 1, opbuf);
    }
    else if(symbol->symtype == MC_SYM_FREE)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_GETFREE, 1, opbuf);
    }
    else if(symbol->symtype == MC_SYM_FUNCTION)
    {
        ip = mc_compiler_emit(comp, MC_OPCODE_CURRENTFUNCTION, 0, nullptr);
    }
    else if(symbol->symtype == MC_SYM_THIS)
    {
        ip = mc_compiler_emit(comp, MC_OPCODE_GETTHIS, 0, nullptr);
    }
    return ip >= 0;
}

bool mc_compiler_storesymbol(mcastcompiler_t* comp, mcastsymbol_t* symbol, bool define)
{
    int ip;
    uint64_t opbuf[10];
    ip = -1;
    if(symbol->symtype == MC_SYM_MODULEGLOBAL)
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
    else if(symbol->symtype == MC_SYM_LOCAL)
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
    else if(symbol->symtype == MC_SYM_FREE)
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
    return compscope->ipstackbreak->push(&ip);
}

void mc_compiler_popbreakip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    if(compscope->ipstackbreak->count() == 0)
    {
    }
    else
    {
        compscope->ipstackbreak->pop(nullptr);
    }
}

int mc_compiler_getbreakip(mcastcompiler_t* comp)
{
    int* res;
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    if(compscope->ipstackbreak->count() == 0)
    {
        return -1;
    }
    res = (int*)compscope->ipstackbreak->top();
    return *res;
}

bool mc_compiler_pushcontinueip(mcastcompiler_t* comp, int ip)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return compscope->ipstackcontinue->push(&ip);
}

void mc_compiler_popcontinueip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    if(compscope->ipstackcontinue->count() == 0)
    {
        MC_ASSERT(false);
    }
    compscope->ipstackcontinue->pop(nullptr);
}

int mc_compiler_getcontinueip(mcastcompiler_t* comp)
{
    int* res;
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    if(compscope->ipstackcontinue->count() == 0)
    {
        MC_ASSERT(false);
        return -1;
    }
    res = (int*)compscope->ipstackcontinue->top();
    return *res;
}

int mc_compiler_getip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return compscope->compiledscopebytecode->count();
}

PtrList* mc_compiler_getsrcpositions(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return compscope->scopesrcposlist;
}

PtrList* mc_compiler_getbytecode(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return compscope->compiledscopebytecode;
}


mcastscopefile_t* mc_compiler_filescopemake(mcastcompiler_t* comp, mcastcompiledfile_t* file)
{
    mcastscopefile_t* filescope;
    filescope = Memory::make<mcastscopefile_t>();
    if(!filescope)
    {
        return nullptr;
    }
    memset(filescope, 0, sizeof(mcastscopefile_t));
    filescope->pstate = comp->pstate;
    filescope->parser = mc_astparser_make(comp->pstate, comp->config, comp->errors);
    if(!filescope->parser)
    {
        goto err;
    }
    filescope->filesymtab = nullptr;
    filescope->file = file;
    filescope->loadedmodnames = Memory::make<PtrList>(sizeof(void*), true);
    if(!filescope->loadedmodnames)
    {
        goto err;
    }
    return filescope;
err:
    mc_compiler_filescopedestroy(filescope);
    return nullptr;
}

void mc_compiler_filescopedestroy(mcastscopefile_t* scope)
{
    size_t i;
    void* name;
    for(i = 0; i < scope->loadedmodnames->count(); i++)
    {
        name = (void*)scope->loadedmodnames->get(i);
        mc_memory_free(name);
    }
    PtrList::destroy(scope->loadedmodnames, nullptr);
    mc_astparser_destroy(scope->parser);
    mc_memory_free(scope);
}

bool mc_compiler_filescopepush(mcastcompiler_t* comp, const char* filepath)
{
    bool ok;
    int globaloffset;
    mcastscopeblock_t* prevsttopscope;
    mcastsymtable_t* prevst;
    mcastcompiledfile_t* file;
    mcastscopefile_t* filescope;
    prevst = nullptr;
    if(comp->filescopelist->count() > 0)
    {
        prevst = mc_compiler_getsymtable(comp);
    }
    file = mc_compiledfile_make(comp->pstate, filepath);
    if(!file)
    {
        return false;
    }
    ok = comp->files->push(file);
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
    ok = comp->filescopelist->push(filescope);
    if(!ok)
    {
        mc_compiler_filescopedestroy(filescope);
        return false;
    }
    globaloffset = 0;
    if(prevst)
    {
        prevsttopscope = prevst->scopeBlockGet();
        globaloffset = prevsttopscope->offset + prevsttopscope->numdefinitions;
    }
    ok = mc_compiler_pushsymtable(comp, globaloffset);
    if(!ok)
    {
        comp->filescopelist->pop(nullptr);
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
    poppedsttopscope = poppedst->scopeBlockGet();
    poppednumdefs = poppedsttopscope->numdefinitions;
    while(mc_compiler_getsymtable(comp))
    {
        mc_compiler_popsymtable(comp);
    }
    scope = (mcastscopefile_t*)comp->filescopelist->top();
    if(!scope)
    {
        MC_ASSERT(false);
    }
    mc_compiler_filescopedestroy(scope);
    comp->filescopelist->pop(nullptr);
    if(comp->filescopelist->count() > 0)
    {
        currentst = mc_compiler_getsymtable(comp);
        currentsttopscope = currentst->scopeBlockGet();
        currentsttopscope->numdefinitions += poppednumdefs;
    }
}

void mc_compiler_setcompilationscope(mcastcompiler_t* comp, mcastscopecomp_t* scope)
{
    comp->compilationscope = scope;
}

mcastcompiledfile_t* mc_compiledfile_make(mcstate_t* state, const char* path)
{
    size_t len;
    const char* lastslashpos;
    mcastcompiledfile_t* file;
    file = Memory::make<mcastcompiledfile_t>();
    if(!file)
    {
        return nullptr;
    }
    memset(file, 0, sizeof(mcastcompiledfile_t));
    file->pstate = state;
    lastslashpos = strrchr(path, '/');
    if(lastslashpos)
    {
        len = lastslashpos - path + 1;
        file->dir_path = mc_util_strndup(path, len);
    }
    else
    {
        file->dir_path = mc_util_strdup("");
    }
    if(!file->dir_path)
    {
        goto error;
    }
    file->path = mc_util_strdup(path);
    if(!file->path)
    {
        goto error;
    }
    file->lines = Memory::make<PtrList>(sizeof(void*), true);
    if(!file->lines)
    {
        goto error;
    }
    return file;
error:
    mc_compiledfile_destroy(file);
    return nullptr;
}

void mc_compiledfile_destroy(mcastcompiledfile_t* file)
{
    size_t i;
    void* item;
    if(file != nullptr)
    {
        for(i = 0; i < file->lines->count(); i++)
        {
            item = (void*)file->lines->get(i);
            mc_memory_free(item);
        }
        PtrList::destroy(file->lines, nullptr);
        mc_memory_free(file->dir_path);
        mc_memory_free(file->path);
        mc_memory_free(file);
    }
}

mcastcompiler_t* mc_compiler_make(mcstate_t* state, mcconfig_t* config, mcgcmemory_t* mem, mcerrlist_t* errors, PtrList* files, mcglobalstore_t* gstore)
{
    bool ok;
    mcastcompiler_t* comp = Memory::make<mcastcompiler_t>();
    if(!comp)
    {
        return nullptr;
    }
    ok = mc_compiler_init(comp, state, config, mem, errors, files, gstore);
    if(!ok)
    {
        mc_memory_free(comp);
        return nullptr;
    }
    comp->pstate = state; 
    return comp;
}

void mc_compiler_destroy(mcastcompiler_t* comp)
{
    if(comp != nullptr)
    {
        mc_compiler_deinit(comp);
        mc_memory_free(comp);
    }
}

mccompiledprogram_t* mc_compiler_compilesource(mcastcompiler_t* comp, const char* code, const char* filename)
{
    bool ok;
    mcastcompiler_t compshallowcopy;
    mcastscopecomp_t* compscope;
    mccompiledprogram_t* res;
    compscope = mc_compiler_getcompilationscope(comp);
    MC_ASSERT(comp->srcposstack->count() == 0);
    MC_ASSERT(compscope->compiledscopebytecode->count() == 0);
    MC_ASSERT(compscope->ipstackbreak->count() == 0);
    MC_ASSERT(compscope->ipstackcontinue->count() == 0);
    comp->srcposstack->clear();
    compscope->compiledscopebytecode->clear();
    compscope->scopesrcposlist->clear();
    compscope->ipstackbreak->clear();
    compscope->ipstackcontinue->clear();
    ok = mc_compiler_filescopepush(comp, filename);
    if(!ok)
    {
        goto compilefailed;
    }
    ok = mc_compiler_initshallowcopy(&compshallowcopy, comp);
    if(!ok)
    {
        return nullptr;
    }
    ok = mc_compiler_docompilesource(comp, code);
    if(!ok)
    {
        goto compilefailed;
    }
    mc_compiler_emit(comp, MC_OPCODE_HALT, 0, 0);
    /* might've changed */
    compscope = mc_compiler_getcompilationscope(comp);
    MC_ASSERT(compscope->outer == nullptr);
    compscope = mc_compiler_getcompilationscope(comp);
    res = mc_astcompscope_orphanresult(compscope);
    if(!res)
    {
        goto compilefailed;
    }
    mc_compiler_deinit(&compshallowcopy);
    return res;
compilefailed:
    mc_compiler_deinit(comp);
    *comp = compshallowcopy;
    return nullptr;
}

mcastsymtable_t* mc_compiler_getsymtable(mcastcompiler_t* comp)
{
    mcastscopefile_t* filescope = (mcastscopefile_t*)comp->filescopelist->top();
    if(!filescope)
    {
        MC_ASSERT(false);
        return nullptr;
    }
    return filescope->filesymtab;
}

void mc_compiler_setsymtable(mcastcompiler_t* comp, mcastsymtable_t* table)
{
    mcastscopefile_t* filescope = (mcastscopefile_t*)comp->filescopelist->top();
    if(!filescope)
    {
        MC_ASSERT(false);
    }
    filescope->filesymtab = table;
}

GenericList<mcvalue_t>* mc_compiler_getconstants(mcastcompiler_t* comp)
{
    return comp->constants;
}

mcclass_t* mc_class_make(mcstate_t* state, const char* name, bool istop)
{
    mcclass_t* cl;
    cl = Memory::make<mcclass_t>();
    cl->parentclass = nullptr;
    cl->classname = name;
    cl->constructor = mc_value_makenull();
    cl->members = Memory::make<PtrList>(sizeof(void*), true);
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
    for(i=0; i<cl->members->count(); i++)
    {
        memb = (mcfield_t*)cl->members->get(i);
        mc_memory_free(memb);
    }
    PtrList::destroy(cl->members, nullptr);
    mc_memory_free(cl);
}

void mc_class_addfunction(mcstate_t* state, mcclass_t* cl, const char* name, bool ispseudo, mcnativefn_t fn)
{
    mcfield_t* bt;
    (void)state;
    bt = Memory::make<mcfield_t>();
    bt->name = name;
    bt->ispseudo = ispseudo;
    bt->fndest = fn;
    cl->members->push(bt);
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
    state = Memory::make<mcstate_t>();
    if(!state)
    {
        return nullptr;
    }
    //memset(state, 0, sizeof(mcstate_t));
    mc_state_setdefaultconfig(state);
    state->execstate.valuestack = Memory::make<GenericList<mcvalue_t>>(MC_CONF_MINVMVALSTACKSIZE, mc_value_makenull());
    state->execstate.valthisstack = Memory::make<GenericList<mcvalue_t>>(MC_CONF_MINVMTHISSTACKSIZE, mc_value_makenull());
    state->execstate.nativethisstack = Memory::make<GenericList<mcvalue_t>>(MC_CONF_MINNATIVETHISSTACKSIZE, mc_value_makenull());
    state->globalvalstack = Memory::make<GenericList<mcvalue_t>>(MC_CONF_MAXVMGLOBALS, mc_value_makenull());
    state->execstate.framestack = Memory::make<GenericList<mcvmframe_t>>(MC_CONF_MINVMFRAMES, mcvmframe_t{});
    mcerrlist_t::init(&state->errors);
    state->mem = mc_gcmemory_make(state);
    if(!state->mem)
    {
        goto err;
    }
    state->files = Memory::make<PtrList>(sizeof(void*), true);
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
    state->stdoutprinter = Printer::make(state, stdout);
    state->stderrprinter = Printer::make(state, stderr);
    mc_state_makestdclasses(state);
    return state;
err:
    mc_state_deinit(state);
    free(state);
    return nullptr;
}

void mc_state_deinit(mcstate_t* state)
{
    mc_compiler_destroy(state->compiler);
    mc_globalstore_destroy(state->vmglobalstore);
    mc_gcmemory_destroy(state->mem);
    PtrList::destroy(state->files, (mcitemdestroyfn_t)mc_compiledfile_destroy);
    mcerrlist_t::deinit(&state->errors);
    Printer::destroy(state->stdoutprinter);
    Printer::destroy(state->stderrprinter);
    Memory::destroy(state->execstate.valuestack);
    Memory::destroy(state->globalvalstack);
    Memory::destroy(state->execstate.framestack);
    Memory::destroy(state->execstate.valthisstack);
    Memory::destroy(state->execstate.nativethisstack);
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
    state->config.strictmode = false;
}

void mc_state_destroy(mcstate_t* state)
{
    if(state != nullptr)
    {
        mc_state_deinit(state);
        mc_memory_free(state);
    }
}


template<typename... ArgsT>
void mc_state_pusherrorf(mcstate_t* state, mcerrtype_t type, mcastlocation_t pos, const char* fmt, ArgsT&&... args)
{
    state->errors.pushFormat(type, pos, fmt, args...);
}



template<typename... ArgsT>
void mc_state_setruntimeerrorf(mcstate_t* state, const char* fmt, ArgsT&&... args)
{
    mc_state_pusherrorf(state, MC_ERROR_RUNTIME, srcposinvalid, fmt, args...);
}



bool mc_error_printtraceback(Printer* pr, mctraceback_t* traceback, mcconsolecolor_t* mcc)
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
    depth = traceback->tbitems->count();
    for(i = 0; i < depth; i++)
    {
        item = (mctraceitem_t*)traceback->tbitems->get(i);
        filename = mc_traceitem_getsourcefilepath(item);
        pr->format("  function %s%s%s", cblue, item->trfuncname, creset);
        if(item->pos.line >= 0 && item->pos.column >= 0)
        {
            pr->format(" in %s%s:%d:%d%s", cyell, filename, item->pos.line, item->pos.column, creset);
        }
        else
        {
        }
        pr->format("\n");
    }
    return !pr->m_prfailed;
}

bool mc_error_printusererror(Printer* pr, mcvalue_t obj)
{
    const char* cred;
    const char* creset;
    mcconsolecolor_t mcc;
    mctraceback_t* traceback;
    mc_consolecolor_init(&mcc, fileno(stdout));
    cred = mc_consolecolor_get(&mcc, 'r');
    creset = mc_consolecolor_get(&mcc, '0');
    pr->format("%sERROR:%s %s\n", cred, creset, mc_value_errorgetmessage(obj));
    traceback = mc_value_errorgettraceback(obj);
    MC_ASSERT(traceback != nullptr);
    if(traceback)
    {
        pr->format("%sTraceback:%s\n", cred, creset);
        mc_error_printtraceback(pr, traceback, &mcc);
    }
    return true;
}

bool mc_error_printerror(mcstate_t* state, Printer* pr, mcerror_t* err)
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
    typestr = err->getTypeString();
    filename = err->getFile();
    line = err->getSourceLineCode();
    linenum = err->getSourceLineNumber();
    colnum = err->getSourceColumn();
    if(line)
    {
        pr->put(line);
        pr->put("\n");
        if(colnum >= 0)
        {
            for(j = 0; j < (colnum - 1); j++)
            {
                pr->put(" ");
            }
            pr->put("^\n");
        }
    }
    pr->format("%s%s ERROR%s in \"%s\" on %s%d:%d:%s %s\n", cred, typestr, creset, filename, cblue, linenum, colnum, creset, err->getMessage());
    traceback = err->getTraceback();
    if(traceback)
    {
        pr->format("traceback:\n");
        mc_error_printtraceback(pr, (mctraceback_t*)err->getTraceback(), &mcc);
    }
    return true;
}



int mc_traceback_getdepth(mctraceback_t* traceback)
{
    return traceback->tbitems->count();
}

const char* mc_traceback_getsourcefilepath(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)traceback->tbitems->get(depth);
    if(!item)
    {
        return nullptr;
    }
    return mc_traceitem_getsourcefilepath(item);
}

const char* mc_traceback_getsourcelinecode(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)traceback->tbitems->get(depth);
    if(!item)
    {
        return nullptr;
    }
    return mc_traceitem_getsourceline(item);
}

int mc_traceback_getsourcelinenumber(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)traceback->tbitems->get(depth);
    if(!item)
    {
        return -1;
    }
    return item->pos.line;
}

int mc_traceback_getsourcecolumn(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)traceback->tbitems->get(depth);
    if(!item)
    {
        return -1;
    }
    return item->pos.column;
}

const char* mc_traceback_getfunctionname(mctraceback_t* traceback, int depth)
{
    mctraceitem_t* item;
    item = (mctraceitem_t*)traceback->tbitems->get(depth);
    if(!item)
    {
        return "";
    }
    return item->trfuncname;
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

mccompiledprogram_t* mc_state_compilesource(mcstate_t* state, const char* code, const char* filename)
{
    mccompiledprogram_t* compres;
    mc_state_clearerrors(state);
    compres = mc_compiler_compilesource(state->compiler, code, filename);
    if(state->errors.count > 0)
    {
        goto err;
    }
    return compres;
err:
    mc_astcompresult_destroy(compres);
    return nullptr;
}


bool mc_vm_runexecfunc(mcstate_t* state, mccompiledprogram_t* comp_res, GenericList<mcvalue_t>* constants);

mcvalue_t mc_program_execute(mcstate_t* state, mccompiledprogram_t* program)
{
    bool ok;
    mcvalue_t res;
    if(program == nullptr)
    {
        state->errors.pushFormat(MC_ERROR_USER, srcposinvalid, "program passed to execute was null.");
        return mc_value_makenull();
    }
    mc_state_reset(state);
    if(state != program->pstate)
    {
        state->errors.pushFormat(MC_ERROR_USER, srcposinvalid, "program was compiled with an incompatible instance");
        return mc_value_makenull();
    }
    ok = mc_vm_runexecfunc(state, program, mc_compiler_getconstants(state->compiler));
    if(!ok || state->errors.count > 0)
    {
        return mc_value_makenull();
    }
    //MC_ASSERT(state->execstate.vsposition == 0);
    res = state->execstate.lastpopped;
    if(mc_value_gettype(res) == MC_VAL_NONE)
    {
        return mc_value_makenull();
    }
    return res;
}

void mc_program_destroy(mccompiledprogram_t* program)
{
    if(program != nullptr)
    {
        mc_astcompresult_destroy(program);
    }
}

mcvalue_t mc_state_execcode(mcstate_t* state, const char* code, const char* filename)
{
    bool ok;
    mcvalue_t res;
    mccompiledprogram_t* compres;
    mc_state_reset(state);
    compres = mc_compiler_compilesource(state->compiler, code, filename);
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
    res = state->execstate.lastpopped;
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

mcvalue_t mc_vm_callvalue(mcstate_t* state, GenericList<mcvalue_t>* constants, mcvalue_t callee, mcvalue_t thisval, size_t argc, mcvalue_t* args);


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
    state->errors.clear();
}

mcerror_t* mc_state_geterror(mcstate_t* state, int index)
{
    return (mcerror_t*)state->errors.get(index);
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
    symbol = st->resolve(name);
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


void mc_astprinter_printast(mcstate_t* state, PtrList* statements)
{
    mcastprinter_t apr;
    apr.pstate = state;
    apr.pseudolisp = false;
    apr.pdest = state->stderrprinter;
    state->stderrprinter->m_prconfig.quotstring = true;
    fprintf(stderr, "---AST dump begin---\n");
    mc_astprint_stmtlist(&apr, statements);
    fprintf(stderr, "\n---AST dump end---\n");
    state->stderrprinter->m_prconfig.quotstring = false;
}

void mc_astprint_stmtlist(mcastprinter_t* apr, PtrList* statements)
{
    int i;
    int count;
    mcastexpression_t* subex;
    count = statements->count();
    for(i = 0; i < count; i++)
    {
        subex = (mcastexpression_t*)statements->get(i);
        mc_astprint_expression(apr, subex);
        if(i < (count - 1))
        {
            apr->pdest->put("\n");
        }
    }
}

void mc_astprint_printfuncliteral(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    size_t i;
    mcastfuncparam_t* param;
    mcastexprliteralfunction_t* ex;
    ex = &astexpr->uexpr.exprlitfunction;
    if(apr->pseudolisp)
    {
        apr->pdest->format("(deffunction '(");
    }
    else
    {
        apr->pdest->put("function(");
    }
    for(i = 0; i < ex->funcparamlist->count(); i++)
    {
        param = (mcastfuncparam_t*)ex->funcparamlist->get(i);
        apr->pdest->put(param->ident->value);
        if(i < (ex->funcparamlist->count() - 1))
        {
            apr->pdest->put(", ");
        }
    }
    apr->pdest->put(") ");
    mc_astprint_codeblock(apr, ex->body);
}

void mc_astprint_printcall(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    size_t i;
    mcastexprcall_t* ex;
    mcastexpression_t* arg;
    ex = &astexpr->uexpr.exprcall;
    mc_astprint_expression(apr, ex->function);
    apr->pdest->put("(");
    for(i = 0; i < ex->args->count(); i++)
    {
        arg = (mcastexpression_t*)ex->args->get(i);
        mc_astprint_expression(apr, arg);
        if(i < (ex->args->count() - 1))
        {
            apr->pdest->put(", ");
        }
    }
    apr->pdest->put(")");
}

void mc_astprint_printarrayliteral(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    size_t i;
    size_t len;
    mcastexprliteralarray_t* ex;
    mcastexpression_t* itemex;
    PtrList* vl;
    ex = &astexpr->uexpr.exprlitarray;
    vl = ex->litarritems;
    len = vl->count();
    apr->pdest->put("[");
    for(i = 0; i < len; i++)
    {
        itemex = (mcastexpression_t*)vl->get(i);
        mc_astprint_expression(apr, itemex);
        if(i < (len - 1))
        {
            apr->pdest->put(", ");
        }
    }
    apr->pdest->put("]");
}

void mc_astprint_printstringliteral(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    size_t slen;
    const char* sdata;
    mcastexprliteralstring_t* ex;
    ex = &astexpr->uexpr.exprlitstring;
    sdata = ex->data;
    slen = ex->length;
    if(apr->pdest->m_prconfig.quotstring)
    {
        apr->pdest->printEscapedString(sdata, slen);
    }
    else
    {
        apr->pdest->put(sdata, slen);
    }
}

void mc_astprint_printmapliteral(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    size_t i;
    mcastexpression_t* keyexpr;
    mcastexpression_t* valexpr;
    mcastexprliteralmap_t* ex;
    ex = &astexpr->uexpr.exprlitmap;
    apr->pdest->put("{");
    for(i = 0; i < ex->litmapkeys->count(); i++)
    {
        keyexpr = (mcastexpression_t*)ex->litmapkeys->get(i);
        valexpr = (mcastexpression_t*)ex->litmapvalues->get(i);
        mc_astprint_expression(apr, keyexpr);
        apr->pdest->put(" : ");
        mc_astprint_expression(apr, valexpr);
        if(i < (ex->litmapkeys->count() - 1))
        {
            apr->pdest->put(", ");
        }
    }
    apr->pdest->put("}");
}

void mc_astprint_printprefixexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprprefix_t* ex;
    ex = &astexpr->uexpr.exprprefix;
    apr->pdest->put("(");
    apr->pdest->put(mc_util_mathopstring(ex->op));
    mc_astprint_expression(apr, ex->right);
    apr->pdest->put(")");
}

void mc_astprint_printinfixexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprinfix_t* ex;
    ex = &astexpr->uexpr.exprinfix;
    apr->pdest->put("(");
    mc_astprint_expression(apr, ex->left);
    apr->pdest->put(" ");
    apr->pdest->put(mc_util_mathopstring(ex->op));
    apr->pdest->put(" ");
    mc_astprint_expression(apr, ex->right);
    apr->pdest->put(")");
}

void mc_astprint_printindexexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    bool prevquot;
    mcastexprindex_t* ex;
    ex = &astexpr->uexpr.exprindex;
    apr->pdest->put("(");
    mc_astprint_expression(apr, ex->left);
    if(ex->isdot)
    {
        apr->pdest->put(".");
        prevquot = apr->pdest->m_prconfig.quotstring;
        apr->pdest->m_prconfig.quotstring = false;
        mc_astprint_expression(apr, ex->index);
        apr->pdest->m_prconfig.quotstring = prevquot;
    }
    else
    {
        apr->pdest->put("[");
        mc_astprint_expression(apr, ex->index);
        apr->pdest->put("]");
    }
    apr->pdest->put(")");
}

void mc_astprint_printassignexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprassign_t* ex;
    ex = &astexpr->uexpr.exprassign;
    mc_astprint_expression(apr, ex->dest);
    apr->pdest->put(" = ");
    mc_astprint_expression(apr, ex->source);
}

void mc_astprint_printlogicalexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprlogical_t* ex;
    ex = &astexpr->uexpr.exprlogical;
    mc_astprint_expression(apr, ex->left);
    apr->pdest->put(" ");
    apr->pdest->put(mc_util_mathopstring(ex->op));
    apr->pdest->put(" ");
    mc_astprint_expression(apr, ex->right);
}

void mc_astprint_printternaryexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprternary_t* ex;
    ex = &astexpr->uexpr.exprternary;
    mc_astprint_expression(apr, ex->tercond);
    apr->pdest->put(" ? ");
    mc_astprint_expression(apr, ex->teriftrue);
    apr->pdest->put(" : ");
    mc_astprint_expression(apr, ex->teriffalse);
}

void mc_astprint_printdefineexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprdefine_t* ex;
    ex = &astexpr->uexpr.exprdefine;
    if(ex->assignable)
    {
        apr->pdest->put("var ");
    }
    else
    {
        apr->pdest->put("const ");
    }
    apr->pdest->put(ex->name->value);
    apr->pdest->put(" = ");
    if(ex->value)
    {
        mc_astprint_expression(apr, ex->value);
    }
}

void mc_astprint_printifexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    size_t i;
    mcastexprifcase_t* ifcase;
    mcastexprstmtif_t* ex;
    ex = &astexpr->uexpr.exprifstmt;
    ifcase = (mcastexprifcase_t*)ex->cases->get(0);
    apr->pdest->put("if (");
    mc_astprint_expression(apr, ifcase->ifcond);
    apr->pdest->put(") ");
    mc_astprint_codeblock(apr, ifcase->consequence);
    for(i = 1; i < ex->cases->count(); i++)
    {
        mcastexprifcase_t* elifcase = (mcastexprifcase_t*)ex->cases->get(i);
        apr->pdest->put(" elif (");
        mc_astprint_expression(apr, elifcase->ifcond);
        apr->pdest->put(") ");
        mc_astprint_codeblock(apr, elifcase->consequence);
    }
    if(ex->alternative)
    {
        apr->pdest->put(" else ");
        mc_astprint_codeblock(apr, ex->alternative);
    }
}

void mc_astprint_printwhileexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprstmtwhile_t* ex;
    ex = &astexpr->uexpr.exprwhileloopstmt;
    apr->pdest->put("while (");
    mc_astprint_expression(apr, ex->loopcond);
    apr->pdest->put(")");
    mc_astprint_codeblock(apr, ex->body);
}

void mc_astprint_printforclassicexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprstmtforloop_t* ex;
    ex = &astexpr->uexpr.exprforloopstmt;
    apr->pdest->put("for (");
    if(ex->init)
    {
        mc_astprint_expression(apr, ex->init);
        apr->pdest->put(" ");
    }
    else
    {
        apr->pdest->put(";");
    }
    if(ex->loopcond)
    {
        mc_astprint_expression(apr, ex->loopcond);
        apr->pdest->put("; ");
    }
    else
    {
        apr->pdest->put(";");
    }
    if(ex->update)
    {
        mc_astprint_expression(apr, ex->update);
    }
    apr->pdest->put(")");
    mc_astprint_codeblock(apr, ex->body);
}

void mc_astprint_printforeachexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprstmtforeach_t* ex;
    ex = &astexpr->uexpr.exprforeachloopstmt;
    apr->pdest->put("for (");
    apr->pdest->format("%s", ex->iterator->value);
    apr->pdest->put(" in ");
    mc_astprint_expression(apr, ex->source);
    apr->pdest->put(")");
    mc_astprint_codeblock(apr, ex->body);
}

void mc_astprint_printimportexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprstmtimport_t* ex;
    ex = &astexpr->uexpr.exprimportstmt;
    apr->pdest->format("import \"%s\"", ex->path);
}

void mc_astprint_printrecoverexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprstmtrecover_t* ex;
    ex = &astexpr->uexpr.exprrecoverstmt;
    apr->pdest->format("recover (%s)", ex->errident->value);
    mc_astprint_codeblock(apr, ex->body);
}

void mc_astprint_expression(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    switch(astexpr->exprtype)
    {
        case MC_EXPR_IDENT:
            {
                mcastexprident_t* ex;
                ex = astexpr->uexpr.exprident;
                apr->pdest->put(ex->value);
            }
            break;
        case MC_EXPR_NUMBERLITERAL:
            {
                mcfloat_t fl;
                fl = astexpr->uexpr.exprlitnumber;
                apr->pdest->format("%1.17g", fl);
            }
            break;
        case MC_EXPR_BOOLLITERAL:
            {
                bool bl;
                bl = astexpr->uexpr.exprlitbool;
                apr->pdest->format("%s", bl ? "true" : "false");
            }
            break;
        case MC_EXPR_STRINGLITERAL:
            {
                mc_astprint_printstringliteral(apr, astexpr);
            }
            break;
        case MC_EXPR_NULLLITERAL:
            {
                apr->pdest->put("null");
            }
            break;
        case MC_EXPR_ARRAYLITERAL:
            {
                mc_astprint_printarrayliteral(apr, astexpr);
            }
            break;
        case MC_EXPR_MAPLITERAL:
            {
                mc_astprint_printmapliteral(apr, astexpr);
            }
            break;
        case MC_EXPR_PREFIX:
            {
                mc_astprint_printprefixexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_INFIX:
            {
                mc_astprint_printinfixexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_FUNCTIONLITERAL:
            {
                mc_astprint_printfuncliteral(apr, astexpr);
            }
            break;
        case MC_EXPR_CALL:
            {
                mc_astprint_printcall(apr, astexpr);
            }
            break;
        case MC_EXPR_INDEX:
            {
                mc_astprint_printindexexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_ASSIGN:
            {
                mc_astprint_printassignexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_LOGICAL:
            {
                mc_astprint_printlogicalexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_TERNARY:
            {
                mc_astprint_printternaryexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_STMTDEFINE:
            {
                mc_astprint_printdefineexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_STMTIF:
            {
                mc_astprint_printifexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_STMTRETURN:
            {
                mcastexpression_t* ex;
                ex = astexpr->uexpr.exprreturnvalue;
                if(ex)
                {
                    apr->pdest->put("return ");
                    mc_astprint_expression(apr, ex);
                    apr->pdest->put(";");
                }
                else
                {
                    apr->pdest->put("return;");
                }
            }
            break;
        case MC_EXPR_STMTEXPRESSION:
            {
                mcastexpression_t* ex;
                ex = astexpr->uexpr.exprexpression;
                if(ex)
                {
                    mc_astprint_expression(apr, ex);
                }
            }
            break;
        case MC_EXPR_STMTLOOPWHILE:
            {
                mc_astprint_printwhileexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_STMTLOOPFORCLASSIC:
            {
                mc_astprint_printforclassicexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_STMTLOOPFOREACH:
            {
                mc_astprint_printforeachexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_STMTBLOCK:
            {
                mcastexprcodeblock_t* ex;
                ex = astexpr->uexpr.exprblockstmt;
                mc_astprint_codeblock(apr, ex);
            }
            break;
        case MC_EXPR_STMTBREAK:
            {
                apr->pdest->put("break");
            }
            break;
        case MC_EXPR_STMTCONTINUE:
            {
                apr->pdest->put("continue");
            }
            break;
        case MC_EXPR_STMTIMPORT:
            {
                mc_astprint_printimportexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_NONE:
            {
                apr->pdest->put("MC_EXPR_NONE");
            }
            break;
        case MC_EXPR_STMTRECOVER:
            {
                mc_astprint_printrecoverexpr(apr, astexpr);
            }
            break;
        default:
            break;
    }
}

void mc_astprint_codeblock(mcastprinter_t* apr, mcastexprcodeblock_t* blockexpr)
{
    size_t i;
    size_t cnt;
    mcastexpression_t* istmt;
    cnt = blockexpr->statements->count();
    apr->pdest->put("{ ");
    for(i = 0; i < cnt; i++)
    {
        istmt = (mcastexpression_t*)blockexpr->statements->get(i);
        mc_astprint_expression(apr, istmt);
        apr->pdest->put("\n");
    }
    apr->pdest->put(" }");
}

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


MC_FORCEINLINE bool mc_callframe_init(mcvmframe_t* frame, mcvalue_t functionobj, int64_t baseptr)
{
    mcobjfunction_t* function;
    if(mc_value_gettype(functionobj) != MC_VAL_FUNCSCRIPT)
    {
        return false;
    }
    function = mc_value_asscriptfunction(functionobj);
    frame->function = functionobj;
    frame->bcposition = 0;
    frame->basepointer = baseptr;
    frame->sourcebcpos = 0;
    frame->bytecode = function->funcdata.valscriptfunc.compiledprogcode->bytecode;
    frame->framesrcposlist = function->funcdata.valscriptfunc.compiledprogcode->progsrcposlist;
    frame->bcsize = function->funcdata.valscriptfunc.compiledprogcode->count;
    frame->recoverip = -1;
    frame->isrecovering = false;
    return true;
}


MC_FORCEINLINE uint16_t mc_callframe_readuint8(mcvmframe_t* frame)
{
    uint16_t data;
    data = frame->bytecode[frame->bcposition];
    frame->bcposition++;
    return data;
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
    mcgcmemory_t* mem = Memory::make<mcgcmemory_t>();
    if(!mem)
    {
        return nullptr;
    }
    memset(mem, 0, sizeof(mcgcmemory_t));
    mem->pstate = state;
    mem->gcobjlist = Memory::make<PtrList>(sizeof(void*), true);
    if(!mem->gcobjlist)
    {
        goto error;
    }
    mem->gcobjlistback = Memory::make<PtrList>(sizeof(void*), true);
    if(!mem->gcobjlistback)
    {
        goto error;
    }
    mem->gcobjlistremains = Memory::make<PtrList>(sizeof(mcvalue_t), false);
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
    return nullptr;
}

void mc_gcmemory_destroy(mcgcmemory_t* mem)
{
    size_t i;
    size_t j;
    mcobjdata_t* obj;
    mcobjdata_t* data;
    mcgcobjdatapool_t* pool;
    if(mem != nullptr)
    {
        PtrList::destroy(mem->gcobjlistremains, nullptr);
        PtrList::destroy(mem->gcobjlistback, nullptr);
        for(i = 0; i < mem->gcobjlist->count(); i++)
        {
            obj = (mcobjdata_t*)mem->gcobjlist->get(i);
            mc_objectdata_deinit(obj);
            memset(obj, 0, sizeof(mcobjdata_t));
            mc_memory_free(obj);
        }
        PtrList::destroy(mem->gcobjlist, nullptr);
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
}

mcobjdata_t* mc_gcmemory_allocobjectdata(mcstate_t* state)
{
    bool ok;
    mcobjdata_t* data;
    data = nullptr;
    state->mem->allocssincesweep++;
    if(state->mem->onlydatapool.count > 0)
    {
        data = state->mem->onlydatapool.data[state->mem->onlydatapool.count - 1];
        state->mem->onlydatapool.count--;
    }
    else
    {
        data = Memory::make<mcobjdata_t>();
        if(!data)
        {
            return nullptr;
        }
    }
    memset(data, 0, sizeof(mcobjdata_t));
    data->pstate = state;
    MC_ASSERT(state->mem->gcobjlistback->count() >= state->mem->gcobjlist->count());
    /*
    * we want to make sure that appending to gcobjlistback never fails in sweep
    * so this only reserves space there.
    */
    ok = state->mem->gcobjlistback->push(data);
    if(!ok)
    {
        Memory::destroy(data);
        return nullptr;
    }
    ok = state->mem->gcobjlist->push(data);
    if(!ok)
    {
        Memory::destroy(data);
        return nullptr;
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
        return nullptr;
    }
    data = pool->data[pool->count - 1];
    MC_ASSERT(state->mem->gcobjlistback->count() >= state->mem->gcobjlist->count());
    /*
    * we want to make sure that appending to gcobjlistback never fails in sweep
    * so this only reserves space there.
    */
    ok = state->mem->gcobjlistback->push(data);
    if(!ok)
    {
        return nullptr;
    }
    ok = state->mem->gcobjlist->push(data);
    if(!ok)
    {
        return nullptr;
    }
    pool->count--;
    return data;
}

void mc_state_gcunmarkall(mcstate_t* state)
{
    size_t i;
    mcobjdata_t* data;
    for(i = 0; i < state->mem->gcobjlist->count(); i++)
    {
        data = (mcobjdata_t*)state->mem->gcobjlist->get(i);
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
    mcobjfunction_t* function;
    if(mc_value_isallocated(obj))
    {
        data = mc_value_getallocateddata(obj);
        if(!data->gcmark)
        {
            data->gcmark = true;
            switch(mc_value_gettype(obj))
            {
                case MC_VAL_MAP:
                    {
                        len = mc_value_mapgetlength(obj);
                        for(i = 0; i < len; i++)
                        {
                            key = mc_value_mapgetkeyat(obj, i);
                            if(mc_value_isallocated(key))
                            {
                                keydata = mc_value_getallocateddata(key);
                                if(!keydata->gcmark)
                                {
                                    mc_state_gcmarkobject(key);
                                }
                            }
                            val = mc_value_mapgetvalueat(obj, i);
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
                        break;
                        function = mc_value_asscriptfunction(obj);
                        for(i = 0; i < function->funcdata.valscriptfunc.freevalscount; i++)
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
    }
}

void mc_state_gcsweep(mcstate_t* state)
{
    bool ok;
    size_t i;
    mcobjdata_t* data;
    PtrList* objstemp;
    mcgcobjdatapool_t* pool;
    mc_state_gcmarkobjlist((mcvalue_t*)state->mem->gcobjlistremains->data(), state->mem->gcobjlistremains->count());
    MC_ASSERT(state->mem->gcobjlistback->count() >= state->mem->gcobjlist->count());
    state->mem->gcobjlistback->clear();
    for(i = 0; i < state->mem->gcobjlist->count(); i++)
    {
        data = (mcobjdata_t*)state->mem->gcobjlist->get(i);
        if(data->gcmark)
        {
            /*
            * this should never fail because gcobjlistback's size should be equal to objects
            */
            ok = state->mem->gcobjlistback->push(data);
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
                    Memory::destroy(data);
                    data = nullptr;
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
    if(data->mem->gcobjlistremains->contains(&obj))
    {
        return false;
    }
    ok = data->mem->gcobjlistremains->push(&obj);
    return ok;
}

void mc_state_gcenablefor(mcvalue_t obj)
{
    mcobjdata_t* data;
    if(mc_value_isallocated(obj))
    {
        data = mc_value_getallocateddata(obj);
        data->mem->gcobjlistremains->removeItem(&obj);
    }
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
    return nullptr;
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
                if(mc_value_mapgetlength(obj) > 1024)
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
    store = Memory::make<mcglobalstore_t>();
    if(!store)
    {
        return nullptr;
    }
    memset(store, 0, sizeof(mcglobalstore_t));
    store->pstate = state;
    store->storedsymbols = Memory::make<PtrDict>((mcitemcopyfn_t)mcastsymbol_t::copy, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
    if(!store->storedsymbols)
    {
        goto err;
    }
    store->storedobjects = Memory::make<GenericList<mcvalue_t>>(0, mc_value_makenull());
    if(!store->storedobjects)
    {
        goto err;
    }
    return store;
err:
    mc_globalstore_destroy(store);
    return nullptr;
}

void mc_globalstore_destroy(mcglobalstore_t* store)
{
    if(store != nullptr)
    {
        PtrDict::destroyItemsAndDict(store->storedsymbols);
        Memory::destroy(store->storedobjects);
        mc_memory_free(store);
        store = nullptr;
    }
}

mcastsymbol_t* mc_globalstore_getsymbol(mcglobalstore_t* store, const char* name)
{
    return (mcastsymbol_t*)store->storedsymbols->get(name);
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
        ok = store->storedobjects->set(existingsymbol->index, object);
        return ok;
    }
    ix = store->storedobjects->count();
    ok = store->storedobjects->push(object);
    if(!ok)
    {
        return false;
    }
    symbol = mcastsymbol_t::make(store->pstate, name, MC_SYM_GLOBALBUILTIN, ix, false);
    if(!symbol)
    {
        goto err;
    }
    ok = store->storedsymbols->set(name, symbol);
    if(!ok)
    {
        mcastsymbol_t::destroy(symbol);
        goto err;
    }
    return true;
err:
    store->storedobjects->pop(nullptr);
    return false;
}

mcvalue_t mc_globalstore_getatindex(mcglobalstore_t* store, int ix, bool* outok)
{
    mcvalue_t* res;
    res = (mcvalue_t*)store->storedobjects->getp(ix);
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
    return (mcvalue_t*)store->storedobjects->data();
}

int mc_globalstore_getcount(mcglobalstore_t* store)
{
    return store->storedobjects->count();
}




mcastscopeblock_t* mc_astblockscope_make(mcstate_t* state, int offset)
{
    mcastscopeblock_t* newscope;
    newscope = Memory::make<mcastscopeblock_t>();
    if(!newscope)
    {
        return nullptr;
    }
    memset(newscope, 0, sizeof(mcastscopeblock_t));
    newscope->pstate = state;
    newscope->scopestore = Memory::make<PtrDict>((mcitemcopyfn_t)mcastsymbol_t::copy, (mcitemdestroyfn_t)mcastsymbol_t::destroy);
    if(!newscope->scopestore)
    {
        mc_astblockscope_destroy(newscope);
        return nullptr;
    }
    newscope->numdefinitions = 0;
    newscope->offset = offset;
    return newscope;
}

void mc_astblockscope_destroy(mcastscopeblock_t* scope)
{
    PtrDict::destroyItemsAndDict(scope->scopestore);
    mc_memory_free(scope);
    scope = nullptr;
}

mcastscopeblock_t* mc_astblockscope_copy(mcastscopeblock_t* scope)
{
    mcastscopeblock_t* copy;
    copy = Memory::make<mcastscopeblock_t>();
    if(!copy)
    {
        return nullptr;
    }
    memset(copy, 0, sizeof(mcastscopeblock_t));
    copy->pstate = scope->pstate;
    copy->numdefinitions = scope->numdefinitions;
    copy->offset = scope->offset;
    copy->scopestore = scope->scopestore->copy();
    if(!copy->scopestore)
    {
        mc_astblockscope_destroy(copy);
        return nullptr;
    }
    return copy;
}

bool mc_symtable_setsymbol(mcastsymtable_t* table, mcastsymbol_t* symbol)
{
    mcastscopeblock_t* topscope;
    mcastsymbol_t* existing;
    topscope = (mcastscopeblock_t*)table->blockscopes->top();
    existing = (mcastsymbol_t*)topscope->scopestore->get(symbol->name);
    if(existing)
    {
        mcastsymbol_t::destroy(existing);
    }
    return topscope->scopestore->set(symbol->name, symbol);
}

int mc_symtable_nextsymindex(mcastsymtable_t* table)
{
    int ix;
    mcastscopeblock_t* topscope;
    topscope = (mcastscopeblock_t*)table->blockscopes->top();
    ix = topscope->offset + topscope->numdefinitions;
    return ix;
}

int mc_symtable_getnumdefs(mcastsymtable_t* table)
{
    int i;
    int count;
    mcastscopeblock_t* scope;
    count = 0;
    for(i = table->blockscopes->count() - 1; i >= 0; i--)
    {
        scope = (mcastscopeblock_t*)table->blockscopes->get(i);
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

char* mc_asttoken_dupliteralstring(mcasttoken_t* tok)
{
    return mc_util_strndup(tok->tokstrdata, tok->tokstrlen);
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
            return "nullptr";
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
    traceback = Memory::make<mctraceback_t>();
    if(!traceback)
    {
        return nullptr;
    }
    memset(traceback, 0, sizeof(mctraceback_t));
    traceback->pstate = state;
    traceback->tbitems = Memory::make<PtrList>(sizeof(mctraceitem_t), false);
    if(!traceback->tbitems)
    {
        mc_traceback_destroy(traceback);
        return nullptr;
    }
    return traceback;
}

void mc_traceback_destroy(mctraceback_t* traceback)
{
    size_t i;
    mctraceitem_t* item;
    if(traceback != nullptr)
    {
        for(i = 0; i < traceback->tbitems->count(); i++)
        {
            item = (mctraceitem_t*)traceback->tbitems->get(i);
            mc_memory_free(item->trfuncname);
            item->trfuncname = nullptr;
        }
        PtrList::destroy(traceback->tbitems, nullptr);
        mc_memory_free(traceback);
        traceback = nullptr;
    }
}

bool mc_traceback_push(mctraceback_t* traceback, const char* fname, mcastlocation_t pos)
{
    bool ok;
    mctraceitem_t item;
    item.trfuncname = mc_util_strdup(fname);
    if(!item.trfuncname)
    {
        return false;
    }
    item.pos = pos;
    ok = traceback->tbitems->push(&item);
    if(!ok)
    {
        mc_memory_free(item.trfuncname);
        item.trfuncname = nullptr;
        return false;
    }
    return true;
}

bool mc_traceback_vmpush(mctraceback_t* traceback, mcstate_t* state)
{
    bool ok;
    int i;
    mcvmframe_t* frame;
    for(i = state->execstate.framestack->count() - 1; i >= 0; i--)
    {
        frame = state->execstate.framestack->getp(i);
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
    PtrList* lines;
    if(!item->pos.file)
    {
        return nullptr;
    }
    lines = item->pos.file->lines;
    if((size_t)item->pos.line >= (size_t)lines->count())
    {
        return nullptr;
    }
    line = (const char*)lines->get(item->pos.line);
    return line;
}

const char* mc_traceitem_getsourcefilepath(mctraceitem_t* item)
{
    if(!item->pos.file)
    {
        return nullptr;
    }
    return item->pos.file->path;
}

bool mc_vm_init(mcstate_t* state)
{
    int i;
    mcvalue_t keyobj;
    state->hadrecovered = false;
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
    while(state->execstate.framestack->m_listcount > 0)
    {
        mc_vm_popframe(state);
    }
}

bool mc_function_execfunction(mcstate_t* state, mcvalue_t function, GenericList<mcvalue_t>* constants, bool nested);

bool mc_vm_runexecfunc(mcstate_t* state, mccompiledprogram_t* comp_res, GenericList<mcvalue_t>* constants)
{
    bool res;
    size_t oldsp;
    size_t oldthissp;
    size_t oldframescount;
    mcvalue_t mainfn;
    (void)oldsp;
    oldsp = state->execstate.vsposition;
    oldthissp = state->execstate.thisstpos;
    oldframescount = state->execstate.framestack->count();
    mainfn = mc_value_makefuncscript(state, "__main__", comp_res, false, 0, 0, 0);
    if(mc_value_isnull(mainfn))
    {
        return false;
    }
    mc_vm_stackpush(state, mainfn);
    res = mc_function_execfunction(state, mainfn, constants, false);
    while(state->execstate.framestack->count() > oldframescount)
    {
        mc_vm_popframe(state);
    }
    //MC_ASSERT(state->execstate.vsposition == oldsp);
    state->execstate.thisstpos = oldthissp;
    return res;
}


MC_FORCEINLINE bool mc_vm_haserrors(mcstate_t* state)
{
    return state->errors.count > 0;
}

MC_FORCEINLINE bool mc_vm_setglobalbyindex(mcstate_t* state, size_t ix, mcvalue_t val)
{
    state->globalvalstack->set(ix, val);
    if(ix >= state->globalvalcount)
    {
        state->globalvalcount = ix + 1;
    }
    return true;
}

MC_FORCEINLINE mcvalue_t mc_vmintern_getglobalbyindex(mcstate_t* state, size_t ix)
{
    return state->globalvalstack->get(ix);
}

mcvalue_t mc_vm_getglobalbyindex(mcstate_t* state, size_t ix)
{
    return mc_vmintern_getglobalbyindex(state, ix);
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
        for(i=(state->execstate.vsposition - 0); (i != bytescount) && (i < state->execstate.valuestack->m_listcapacity); i++)
        {
            //memset(&state->execstate.valuestack->m_listitems[i], 0, sizeof(mcvalue_t));
            state->execstate.valuestack->m_listitems[i].valtype = MC_VAL_NULL;
        }
    }
    #endif
    state->execstate.vsposition = nsp;
}

MC_FORCEINLINE void mc_vmintern_stackpush(mcstate_t* state, mcvalue_t obj)
{
    int numlocals;
    mcvmframe_t* frame;
    mcobjfunction_t* currentfunction;
    (void)numlocals;
    (void)frame;
    (void)currentfunction;
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(state->execstate.currframe)
    {
        frame = state->execstate.currframe;
        currentfunction = mc_value_asscriptfunction(frame->function);
        numlocals = currentfunction->funcdata.valscriptfunc.numlocals;
        MC_ASSERT((size_t)state->execstate.vsposition >= (size_t)(frame->basepointer + numlocals));
    }
#endif
    #if 1
        state->execstate.valuestack->set(state->execstate.vsposition, obj);
    #else
        state->execstate.valuestack->push(obj);
    #endif
    state->execstate.vsposition++;
}

void mc_vm_stackpush(mcstate_t* state, mcvalue_t obj)
{
    mc_vmintern_stackpush(state, obj);
}

MC_FORCEINLINE mcvalue_t mc_vm_stackpop(mcstate_t* state)
{
    int numlocals;
    mcvalue_t res;
    mcvmframe_t* frame;
    mcobjfunction_t* currentfunction;
    (void)numlocals;
    (void)frame;
    (void)currentfunction;
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(state->execstate.vsposition == 0)
    {
        state->errors.pushFormat(MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "stack underflow");
        MC_ASSERT(false);
        return mc_value_makenull();
    }
    if(state->execstate.currframe)
    {
        frame = state->execstate.currframe;
        currentfunction = mc_value_asscriptfunction(frame->function);
        numlocals = currentfunction->funcdata.valscriptfunc.numlocals;
        MC_ASSERT((state->execstate.vsposition - 1) >= (frame->basepointer + numlocals));
    }
#endif
    state->execstate.vsposition--;
    res = state->execstate.valuestack->get(state->execstate.vsposition);
    state->execstate.lastpopped = res;
    return res;
}

MC_FORCEINLINE mcvalue_t mc_vm_stackget(mcstate_t* state, size_t nthitem)
{
    size_t ix;
    ix = state->execstate.vsposition - 1 - nthitem;
    return state->execstate.valuestack->get(ix);
}

MC_FORCEINLINE void mc_vm_thisstackpush(mcstate_t* state, mcvalue_t obj)
{
    state->execstate.valthisstack->set(state->execstate.thisstpos, obj);
    state->execstate.thisstpos++;
}

MC_FORCEINLINE mcvalue_t mc_vm_thisstackpop(mcstate_t* state)
{
#if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
    if(state->execstate.thisstpos == 0)
    {
        state->errorspushFormat(MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "'this' stack underflow");
        MC_ASSERT(false);
        return mc_value_makenull();
    }
#endif
    state->execstate.thisstpos--;
    return state->execstate.valthisstack->get(state->execstate.thisstpos);
}

MC_FORCEINLINE mcvalue_t mc_vm_thisstackget(mcstate_t* state, size_t nthitem)
{
    size_t ix;
    size_t cnt;
    mcvalue_t val;
    (void)val;
    ix = state->execstate.thisstpos - 1 - nthitem;
    cnt = state->execstate.valthisstack->count();
    if((cnt == 0) || (ix > cnt))
    {
        //val = mc_value_makemap(state);
        //state->execstate.valthisstack->set(ix, val);
        //return mc_value_makemap(state);
        return mc_value_makenull();
        //return val;
    }
    return state->execstate.valthisstack->get(ix);
}

MC_FORCEINLINE bool mc_vm_pushframe(mcstate_t* state, mcvmframe_t frame)
{
    int add;
    mcobjfunction_t* framefunction;
    add = 0;
    state->execstate.framestack->set(state->execstate.framestack->m_listcount, frame);
    add = 1;
    state->execstate.currframe = state->execstate.framestack->getp(state->execstate.framestack->m_listcount);
    state->execstate.framestack->m_listcount += add;
    framefunction = mc_value_asscriptfunction(frame.function);
    mc_vm_setstackpos(state, frame.basepointer + framefunction->funcdata.valscriptfunc.numlocals);
    return true;
}


MC_FORCEINLINE bool mc_vmintern_popframe(mcstate_t* state)
{
    mc_vm_setstackpos(state, state->execstate.currframe->basepointer - 1);
    if(state->execstate.framestack->m_listcount <= 0)
    {
        MC_ASSERT(false);
        state->execstate.currframe = NULL;
        return false;
    }
    state->execstate.framestack->m_listcount--;
    if(state->execstate.framestack->m_listcount == 0)
    {
        state->execstate.currframe = NULL;
        return false;
    }
    state->execstate.currframe = state->execstate.framestack->getp(state->execstate.framestack->m_listcount - 1);
    return true;
}


bool mc_vm_popframe(mcstate_t* state)
{
    return mc_vmintern_popframe(state);
}

MC_INLINE void mc_vm_rungc(mcstate_t* state, GenericList<mcvalue_t>* constants)
{
    size_t i;
    mcvmframe_t* frame;
    mc_state_gcunmarkall(state);
    mc_state_gcmarkobjlist(mc_globalstore_getdata(state->vmglobalstore), mc_globalstore_getcount(state->vmglobalstore));
    mc_state_gcmarkobjlist((mcvalue_t*)constants->data(), constants->count());
    mc_state_gcmarkobjlist(state->globalvalstack->data(), state->globalvalcount);
    for(i = 0; i < state->execstate.framestack->count(); i++)
    {
        frame = state->execstate.framestack->getp(i);
        mc_state_gcmarkobject(frame->function);
    }
    mc_state_gcmarkobjlist(state->execstate.valuestack->data(), state->execstate.vsposition);
    mc_state_gcmarkobjlist(state->execstate.valthisstack->data(), state->execstate.thisstpos);
    mc_state_gcmarkobject(state->execstate.lastpopped);
    mc_state_gcmarkobjlist(state->operoverloadkeys, MC_CONF_MAXOPEROVERLOADS);
    mc_state_gcsweep(state);
}

MC_FORCEINLINE mcvalue_t mc_vm_callnativefunction(mcstate_t* state, mcvalue_t callee, mcastlocation_t srcpos, mcvalue_t selfval, size_t argc, mcvalue_t* args)
{
    mcvaltype_t restype;
    mcvalue_t res;
    mcerror_t* err; 
    mctraceback_t* traceback;
    mcobjfunction_t* nativefun;
    nativefun = mc_value_asnativefunction(callee);
    res = nativefun->funcdata.valnativefunc.natptrfn(state, nativefun->funcdata.valnativefunc.userpointer, selfval, argc, args);
    if(mc_util_unlikely(state->errors.count > 0))
    {
        err = state->errors.getLast();
        err->m_pos = srcpos;
        err->m_traceback = mc_traceback_make(state);
        if(err->m_traceback)
        {
            mc_traceback_push(err->m_traceback, nativefun->funcdata.valnativefunc.natfnname, srcposinvalid);
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
            if(!mc_util_strequal(nativefun->funcdata.valnativefunc.natfnname, "error"))
            {
                mc_traceback_push(traceback, nativefun->funcdata.valnativefunc.natfnname, srcposinvalid);
            }
            mc_traceback_vmpush(traceback, state);
            mc_value_errorsettraceback(res, traceback);
        }
    }
    return res;
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
    mcobjfunction_t* calleefunction;
    calleetype = mc_value_gettype(callee);
    selfval = mc_value_makenull();
    if(mc_value_isfuncnative(callee))
    {
        if(!state->execstate.nativethisstack->pop(&tmpval))
        {
            #if 0
                state->stderrprinter->format("failed to pop native 'this' for = <");
                mc_printer_printvalue(state->stderrprinter, callee, true);
                state->stderrprinter->format(">\n");
                #if 0
                    state->errors.pushFormat(MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "failed to pop native 'this'");
                #endif
            #endif
        }
        selfval = tmpval;
    }
    #if 0
    {
        state->stderrprinter->format("selfval = <<<");
        mc_printer_printvalue(state->stderrprinter, selfval, true);
        state->stderrprinter->format(">>>\n");
    }
    #endif
    if(calleetype == MC_VAL_FUNCSCRIPT)
    {
        calleefunction = mc_value_asscriptfunction(callee);
        if(nargs != calleefunction->funcdata.valscriptfunc.numargs)
        {
            #if 0
            state->errors.pushFormat(MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "invalid number of arguments to \"%s\": expected %d, got %d",
                              mc_value_functiongetname(callee), calleefunction->funcdata.valscriptfunc.numargs, nargs);
            return false;
            #endif
        }
        ok = mc_callframe_init(&calleeframe, callee, state->execstate.vsposition - nargs);
        if(!ok)
        {
            state->errors.pushFormat(MC_ERROR_RUNTIME, srcposinvalid, "frame init failed in mc_vmdo_callobject");
            return false;
        }
        ok = mc_vm_pushframe(state, calleeframe);
        if(!ok)
        {
            state->errors.pushFormat(MC_ERROR_RUNTIME, srcposinvalid, "pushing frame failed in mc_vmdo_callobject");
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
        stackpos = state->execstate.valuestack->data() + state->execstate.vsposition - nargs;
        res = mc_vm_callnativefunction(state, callee, mc_callframe_getpos(state->execstate.currframe), selfval, nargs, stackpos);
        if(mc_vm_haserrors(state))
        {
            return false;
        }
        mc_vm_setstackpos(state, state->execstate.vsposition - nargs - 1);
        mc_vmintern_stackpush(state, res);
    }
    else
    {
        calleetypename = mc_valtype_getname(calleetype);
        state->errors.pushFormat(MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "%s object is not callable", calleetypename);
        return false;
    }
    return true;
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
        callee = mc_value_mapgetvalue(left, key);
    }
    if(!mc_value_iscallable(callee))
    {
        if(righttype == MC_VAL_MAP)
        {
            callee = mc_value_mapgetvalue(right, key);
        }

        if(!mc_value_iscallable(callee))
        {
            *outoverloadfound = false;
            return true;
        }
    }
    *outoverloadfound = true;
    mc_vmintern_stackpush(state, callee);
    mc_vmintern_stackpush(state, left);
    if(numoper == 2)
    {
        mc_vmintern_stackpush(state, right);
    }
    return mc_vmdo_callobject(state, callee, numoper);
}


MC_FORCEINLINE bool mc_vm_checkassign(mcstate_t* state, mcvalue_t oldvalue, mcvalue_t nvalue)
{
    return true;
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
    mc_vmintern_stackpush(state, nstring);
    if(!mc_value_stringappendvalue(nstring, valleft))
    {
        mc_vmintern_stackpush(state, valleft);
        return false;
    }
    if(!mc_value_stringappendvalue(nstring, valright))
    {
        mc_vmintern_stackpush(state, valright);
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
        mc_vmintern_stackpush(state, mc_value_makenumber(res));
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
    return state->stdobjobject;
}

MC_FORCEINLINE mcclass_t* mc_vmdo_findclassfor(mcstate_t* state, mcvaltype_t typ)
{
    mcclass_t* cl;
    cl = mc_vmdo_findclassforintern(state, typ);
    if(cl != nullptr)
    {
        
    }
    return cl;
}

MC_INLINE mcfield_t* mc_vmdo_getclassmember(mcstate_t* state, mcclass_t* cl, const char* name)
{
    size_t i;
    mcfield_t* memb;
    (void)state;
    for(i=0; i<cl->members->count(); i++)
    {
        memb = (mcfield_t*)cl->members->get(i);
        if(strcmp(memb->name, name) == 0)
        {
            return memb;
        }
    }
    if(cl->parentclass != nullptr)
    {
        return mc_vmdo_getclassmember(state, cl->parentclass, name);
    }
    return nullptr;
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
    if(cl != nullptr)
    {
        idxname = mc_value_stringgetdata(index);
        vdest = mc_vmdo_getclassmember(state, cl, idxname);
        if(vdest == nullptr)
        {
            return false;
        }
        else
        {
            fnval = mc_value_makefuncnative(state, vdest->name, vdest->fndest, nullptr);
            if(vdest->ispseudo)
            {
                retv = mc_vm_callnativefunction(state, fnval, mc_callframe_getpos(state->execstate.currframe), left, 0, nullptr);
                mc_vmintern_stackpush(state, retv);
                return true;
            }
            else
            {
                retv = fnval;
                mc_vmintern_stackpush(state, retv);
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
    lefttypename = "unknown";
    if(lefttype == MC_VAL_MAP)
    {
        if(mc_value_mapgetvaluechecked(left, index, &res))
        {
            goto finished;
        }
    }
    if(mc_value_isstring(index))
    {
        if(mc_vmdo_findclassmembervalue(state, left, index, mc_value_makenull()))
        {
            #if 0
            if(mc_value_isfuncnative(callee))
            #endif
            {
                state->execstate.nativethisstack->push(left);
            }
            #if 0
                state->stderrprinter->format("getindexpartial:left=<");
                mc_printer_printvalue(state->stderrprinter, left, true);
                state->stderrprinter->format(">\n");
            #endif
            return true;
        }
        else
        {
            if(!mc_value_ismap(left))
            {
                res = mc_value_makenull();
                lefttypename = mc_valtype_getname(lefttype);
                mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "object type '%s' has no field '%s'", lefttypename, mc_value_stringgetdata(index));
                mc_vmintern_stackpush(state, res);
                return false;
            }
        }
    }
    if(lefttype != MC_VAL_ARRAY && lefttype != MC_VAL_MAP && lefttype != MC_VAL_STRING)
    {
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
    else if(lefttype == MC_VAL_STRING)
    {
        if(indextype != MC_VAL_NUMBER)
        {
            lefttypename = mc_valtype_getname(lefttype);
            indextypename = mc_valtype_getname(indextype);
            mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "cannot index %s with %s", lefttypename, indextypename);
            return false;
        }
        str = mc_value_stringgetdata(left);
        leftlen = mc_value_stringgetlength(left);
        ix = (int)mc_value_asnumber(index);
        if(ix >= 0 && ix < leftlen)
        {
            resstr[0] = str[ix];
            res = mc_value_makestringlen(state, resstr, 1);
        }
    }
    finished:
    mc_vmintern_stackpush(state, res);
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
        #if 0
        {
            int* p = nullptr;
            p[5] += 5;
        }
        #endif
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
        oldvalue = mc_value_mapgetvalue(left, index);
        if(!mc_vm_checkassign(state, oldvalue, nvalue))
        {
            return false;
        }
        ok = mc_value_mapsetvalue(left, index, nvalue);
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
        res = mc_value_mapgetkvpairat(state, left, ix);
    }
    else if(lefttype == MC_VAL_STRING)
    {
        str = mc_value_stringgetdata(left);
        leftlen = mc_value_stringgetlength(left);
        ix = (int)mc_value_asnumber(index);
        if(ix >= 0 && ix < leftlen)
        {
            resstr[0] = str[ix];
            res = mc_value_makestringlen(state, resstr, 1);
        }
    }
    mc_vmintern_stackpush(state, res);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_makefunction(mcstate_t* state, GenericList<mcvalue_t>* constants)
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
    mcobjfunction_t* constfun;
    constantix = mc_callframe_readuint16(state->execstate.currframe);
    numfree = mc_callframe_readuint8(state->execstate.currframe);
    constant = constants->getp(constantix);
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
    constfun = mc_value_asscriptfunction(*constant);
    fname = mc_value_functiongetname(*constant);
    functionobj = mc_value_makefuncscript(state, fname, constfun->funcdata.valscriptfunc.compiledprogcode, false, constfun->funcdata.valscriptfunc.numlocals, constfun->funcdata.valscriptfunc.numargs, numfree);
    if(mc_value_isnull(functionobj))
    {
        return false;
    }
    for(i = 0; i < numfree; i++)
    {
        freeval = state->execstate.valuestack->get(state->execstate.vsposition - numfree + i);
        mc_value_functionsetfreevalat(functionobj, i, freeval);
    }
    mc_vm_setstackpos(state, state->execstate.vsposition - numfree);
    mc_vmintern_stackpush(state, functionobj);
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
            mc_vmintern_stackpush(state, res);
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
    mc_vmintern_stackpush(state, res);
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
    items = state->execstate.valuestack->data() + state->execstate.vsposition - count;
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
    mc_vmintern_stackpush(state, arrayobj);
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
    kvpairs = state->execstate.valuestack->data() + state->execstate.vsposition - itemscount;
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
        ok = mc_value_mapsetvalue(mapobj, key, val);
        if(!ok)
        {
            return false;
        }
    }
    mc_vm_setstackpos(state, state->execstate.vsposition - itemscount);
    mc_vmintern_stackpush(state, mapobj);
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
    if(def != nullptr)
    {
        *oname = def->name;
    }
}

bool mc_function_execfunction(mcstate_t* state, mcvalue_t function, GenericList<mcvalue_t>* constants, bool nested)
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
    const char* funcname;
    mcobjfunction_t* targetfunction;
    opcode = 0;
    (void)oname;
    (void)prevcode;
    #if defined(MC_CONF_USECOMPUTEDGOTOS) && (MC_CONF_USECOMPUTEDGOTOS == 1)
        fprintf(stderr, "**using computed gotos**\n");
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
        state->error.pushFormat(MC_ERROR_USER, srcposinvalid, "state is already executing code");
        return false;
    }
    #endif
    /* naming is hard */
    targetfunction = mc_value_asscriptfunction(function);
    ok = false;
    ok = mc_callframe_init(&createdframe, function, state->execstate.vsposition - targetfunction->funcdata.valscriptfunc.numargs);
    if(!ok)
    {
        fprintf(stderr, "failed to init frames!\n");
        return false;
    }
    ok = mc_vm_pushframe(state, createdframe);
    if(!ok)
    {
        state->errors.pushFormat(MC_ERROR_USER, srcposinvalid, "pushing frame failed");
        return false;
    }
    {
        funcname = "unknown";
        if(targetfunction->funcdata.valscriptfunc.ownsdata)
        {
            funcname = targetfunction->funcdata.valscriptfunc.unamev.fconstname;
        }
        else
        {
            funcname = targetfunction->funcdata.valscriptfunc.unamev.fallocname;
        }
        fprintf(stderr, "**executing function '%s'**\n", funcname);
    }
    state->running = true;
    state->execstate.lastpopped = mc_value_makenull();
    //while(state->execstate.currframe->bcposition < state->execstate.currframe->bcsize)
    while(true)
    {
        readnextop:
        prevcode = opcode;
        if(state->execstate.currframe == nullptr)
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
                    ok = mc_vmintern_popframe(state);
                    if(!ok)
                    {
                        goto onexecfinish;
                    }
                    mc_vmintern_stackpush(state, res);
                    if(nested)
                    {
                        goto onexecfinish;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_RETURN):
                {
                    ok = mc_vmintern_popframe(state);
                    mc_vmintern_stackpush(state, mc_value_makenull());
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
                    constant = (mcvalue_t*)constants->getp(constantix);
                    if(!constant)
                    {
                        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "constant at %d not found", constantix);
                        goto onexecerror;
                    }
                    mc_vmintern_stackpush(state, *constant);
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
                    mc_vmintern_stackpush(state, mc_value_makebool(true));
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_FALSE):
                {
                    mc_vmintern_stackpush(state, mc_value_makebool(false));
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
                        mc_vmintern_stackpush(state, res);
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
                        mc_vmintern_stackpush(state, res);
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
                        mc_vmintern_stackpush(state, res);
                    }
                    else if(type == MC_VAL_NULL)
                    {
                        res = mc_value_makebool(true);
                        mc_vmintern_stackpush(state, res);
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
                            mc_vmintern_stackpush(state, res);
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
                    mc_vmintern_stackpush(state, mc_value_makenull());
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
                    oldvalue= mc_vmintern_getglobalbyindex(state, ix);
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
                    global = state->globalvalstack->get(ix);
                    mc_vmintern_stackpush(state, global);
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
                    state->execstate.valuestack->set(state->execstate.currframe->basepointer + pos, mc_vm_stackpop(state));
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_SETLOCAL):
                {
                    uint16_t pos;
                    mcvalue_t nvalue;
                    mcvalue_t oldvalue;
                    pos = mc_callframe_readuint8(state->execstate.currframe);
                    nvalue = mc_vm_stackpop(state);
                    oldvalue = state->execstate.valuestack->get(state->execstate.currframe->basepointer + pos);
                    if(!mc_vm_checkassign(state, oldvalue, nvalue))
                    {
                        goto onexecerror;
                    }
                    state->execstate.valuestack->set(state->execstate.currframe->basepointer + pos, nvalue);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_GETLOCAL):
                {
                    size_t finalpos;
                    size_t pos;
                    mcvalue_t val;
                    pos = mc_callframe_readuint8(state->execstate.currframe);
                    finalpos = state->execstate.currframe->basepointer + pos;
                    val = state->execstate.valuestack->get(finalpos);
                    #if 0
                    {
                        Printer* pr = state->stderrprinter;
                        pr->format("GETLOCAL: finalpos=%ld val=<<<", finalpos);
                        mc_printer_printvalue(pr, val, true);
                        pr->format(">>>\n");
                    }
                    #endif
                    mc_vmintern_stackpush(state, val);
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
                    mc_vmintern_stackpush(state, val);
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
                    mc_vmintern_stackpush(state, val);
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
                    mc_vmintern_stackpush(state, currentfunction);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_GETTHIS):
                {
                    mcvalue_t obj;
                    obj = mc_vm_thisstackget(state, 0);
                    mc_vmintern_stackpush(state, obj);
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
                    mc_vmintern_stackpush(state, val);
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
                        len = mc_value_mapgetlength(val);
                    }
                    else if(type == MC_VAL_STRING)
                    {
                        len = mc_value_stringgetlength(val);
                    }
                    else
                    {
                        tname = mc_valtype_getname(type);
                        mc_state_pusherrorf(state, MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "cannot get length of %s", tname);
                        goto onexecerror;
                    }
                    mc_vmintern_stackpush(state, mc_value_makenumber(len));
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
                    mc_vmintern_stackpush(state, obj);
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
        state->hadrecovered = false;
        if(state->errors.count > 0)
        {
            err = state->errors.getLast();
            if(err->m_errtype == MC_ERROR_RUNTIME && state->errors.count >= 1)
            {
                recoverframeix = -1;
                for(fri = state->execstate.framestack->count() - 1; fri >= 0; fri--)
                {
                    frame = state->execstate.framestack->getp(fri);
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
                if(!err->m_traceback)
                {
                    err->m_traceback = mc_traceback_make(state);
                }
                if(err->m_traceback)
                {
                    mc_traceback_vmpush(err->m_traceback, state);
                }
                while(state->execstate.framestack->count() > (recoverframeix + 1))
                {
                    mc_vmintern_popframe(state);
                }
                errobj = mc_value_makeerror(state, err->m_message);
                if(!mc_value_isnull(errobj))
                {
                    mc_value_errorsettraceback(errobj, err->m_traceback);
                    err->m_traceback = nullptr;
                }
                mc_vmintern_stackpush(state, errobj);
                state->execstate.currframe->bcposition = state->execstate.currframe->recoverip;
                state->execstate.currframe->isrecovering = true;
                state->errors.clear();
                state->hadrecovered = true;
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
        err = state->errors.getLast();
        if(!err->m_traceback)
        {
            err->m_traceback = mc_traceback_make(state);
        }
        if(err->m_traceback)
        {
            mc_traceback_vmpush(err->m_traceback, state);
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
        inpstr = mc_value_stringgetdata(arg);
        inplen = mc_value_stringgetlength(arg);
        res = mc_value_makestrcapacity(state, inplen);
        if(mc_value_isnull(res))
        {
            return mc_value_makenull();
        }
        resbuf = mc_value_stringgetmutabledata(res);
        for(i = 0; i < inplen; i++)
        {
            resbuf[inplen - i - 1] = inpstr[i];
        }
        resbuf[inplen] = '\0';
        mc_value_stringsetlength(res, inplen);
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
    a_x = mc_value_asnumber(mc_value_mapgetvalue(args[0], keyx));
    a_y = mc_value_asnumber(mc_value_mapgetvalue(args[0], keyy));
    b_x = mc_value_asnumber(mc_value_mapgetvalue(args[1], keyx));
    b_y = mc_value_asnumber(mc_value_mapgetvalue(args[1], keyy));
    res = mc_value_makemap(state);
    if (mc_value_gettype(res) == MC_VAL_NULL)
    {
        return res;
    }
    mc_value_mapsetvalue(res, keyx, mc_value_makenumber(a_x + b_x));
    mc_value_mapsetvalue(res, keyy, mc_value_makenumber(a_y + b_y));
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
    a_x = mc_value_asnumber(mc_value_mapgetvalue(args[0], keyx));
    a_y = mc_value_asnumber(mc_value_mapgetvalue(args[0], keyy));
    b_x = mc_value_asnumber(mc_value_mapgetvalue(args[1], keyx));
    b_y = mc_value_asnumber(mc_value_mapgetvalue(args[1], keyy));
    res = mc_value_makemap(state);
    mc_value_mapsetvalue(res, keyx, mc_value_makenumber(a_x - b_x));
    mc_value_mapsetvalue(res, keyy, mc_value_makenumber(a_y - b_y));
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
        tname = mc_value_objtypename(mc_value_gettype(args[0]));
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
        mc_value_mapsetvalue(res, key, val);
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
    Printer* pr;
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
    state->stdoutprinter->putChar('\n');
    return o;
}

mcvalue_t mc_scriptfn_tostring(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int reslen;
    const char* resstr;
    mcvalue_t arg;
    mcvalue_t res;
    Printer pr;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    arg = args[0];
    Printer::initFromStack(&pr, state, nullptr, true);
    mc_printer_printvalue(&pr, arg, false);
    if(pr.m_prfailed)
    {
        Printer::releaseFromPtr(&pr, true);
        return mc_value_makenull();
    }
    resstr = pr.getString();
    reslen = pr.getLength();
    res = mc_value_makestringlen(state, resstr, reslen);
    Printer::releaseFromPtr(&pr, false);
    return res;
}

mcvalue_t mc_nsfnjson_stringify(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int reslen;
    const char* resstr;
    mcvalue_t arg;
    mcvalue_t res;
    Printer pr;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    arg = args[0];
    Printer::initFromStack(&pr, state, nullptr, true);
    pr.m_prconfig.verbosefunc = false;
    pr.m_prconfig.quotstring = true;
    mc_printer_printvalue(&pr, arg, false);
    if(pr.m_prfailed)
    {
        Printer::releaseFromPtr(&pr, true);
        return mc_value_makenull();
    }
    resstr = pr.getString();
    reslen = pr.getLength();
    res = mc_value_makestringlen(state, resstr, reslen);
    Printer::releaseFromPtr(&pr, true);
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
    len = mc_value_stringgetlength(thisval);
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
    searchstr = nullptr;
    searchlen = 0;
    inpstr = mc_value_stringgetdata(thisval);
    inplen = mc_value_stringgetlength(thisval);
    MC_ASSERT((searchtype == MC_VAL_NUMBER) || (searchtype == MC_VAL_STRING));
    if(searchtype == MC_VAL_NUMBER)
    {
        tmpch = mc_value_asnumber(searchval);
        inpstr = &tmpch;
        inplen = 1;
    }
    else if(searchtype == MC_VAL_STRING)
    {
        searchstr = mc_value_stringgetdata(searchval);
        searchlen = mc_value_stringgetlength(searchval);
    }

    result = (char*)strstr(inpstr + startindex, searchstr);
    if(result == nullptr)
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
    str = mc_value_stringgetdata(sval);
    len = mc_value_stringgetlength(sval);
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
    str = mc_value_stringgetdata(sval);
    len = mc_value_stringgetlength(sval);
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
    str = mc_value_stringgetdata(sval);
    len = mc_value_stringgetlength(sval);
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
    str = mc_value_stringgetdata(thisval);
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
    stringlen = mc_value_stringgetlength(thisval);
    string = mc_value_stringgetdata(thisval);
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
        inpstr = mc_value_stringgetdata(inpval);
        inplen = mc_value_stringgetlength(inpval);
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
        if(result == nullptr)
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
        inpstr = mc_value_stringgetdata(inpval);
        inplen = mc_value_stringgetlength(inpval);
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
        if(result == nullptr)
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
        inpstr = mc_value_stringgetdata(inpval);
        searchstr = mc_value_stringgetdata(searchval);
        replacestr = mc_value_stringgetdata(repval);
        inplen = mc_value_stringgetlength(inpval);
        searchlen = mc_value_stringgetlength(searchval);
        replacelen = mc_value_stringgetlength(repval);
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
        if(result == nullptr)
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
        inpstr = mc_value_stringgetdata(inpval);
        searchstr = mc_value_stringgetdata(searchval);
        replacestr = mc_value_stringgetdata(repval);
        inplen = mc_value_stringgetlength(inpval);
        searchlen = mc_value_stringgetlength(searchval);
        replacelen = mc_value_stringgetlength(repval);
        temp = strstr(inpstr, searchstr);
        if(temp == nullptr)
        {
            return mc_value_makestringlen(state, inpstr, inplen);
        }
        /* Allocate new string to store result */
        newlen = inplen + (replacelen - searchlen) + 1;
        result = (char*)mc_memory_malloc(newlen + 1);
        if(result == nullptr)
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
    inpstr = mc_value_stringgetdata(inpval);
    inplen = mc_value_stringgetlength(inpval);
    if(inplen == 0)
    {
        return mc_value_makestringlen(state, "", 0);
    }
    result = (char*)mc_memory_malloc(inplen + 1);
    if(result == nullptr)
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

mcvalue_t mc_objfnstring_matchhelper(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args, bool icase)
{
    bool r;
    int flags;
    size_t inplen;
    size_t patlen;
    const char* inpstr;
    const char* patstr;
    mcvalue_t patval;
    mcvalue_t inpval;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    flags = 0;
    inpval = thisval;
    patval = args[0];
    inpstr = mc_value_stringgetdata(inpval);
    inplen = mc_value_stringgetlength(inpval);
    patstr = mc_value_stringgetdata(patval);
    patlen = mc_value_stringgetlength(patval);
    if(icase)
    {
        flags |= STRFNM_FLAG_CASEFOLD;
    }
    r  = strfnm_match(patstr, patlen, inpstr, inplen, flags);
    return mc_value_makebool(r);
}

mcvalue_t mc_objfnstring_matchglobcase(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    return mc_objfnstring_matchhelper(state, data, thisval, argc, args, false);
}

mcvalue_t mc_objfnstring_matchglobicase(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    return mc_objfnstring_matchhelper(state, data, thisval, argc, args, false);
}

mcvalue_t mc_objfnstring_tolower(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int inplen;
    const char* inpstr;
    mcvalue_t resstr;
    mcvalue_t inpval;
    (void)data;
    (void)argc;
    (void)args;
    inpval = thisval;
    inpstr = mc_value_stringgetdata(inpval);
    inplen = mc_value_stringgetlength(inpval);
    resstr = mc_value_makestringlen(state, inpstr, inplen);
    mc_value_getallocateddata(resstr)->uvobj.valstring.strbuf->toLowercase();
    return resstr;
}


mcvalue_t mc_objfnstring_toupper(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    int inplen;
    const char* inpstr;
    mcvalue_t resstr;
    mcvalue_t inpval;
    (void)data;
    (void)argc;
    (void)args;
    inpval = thisval;
    inpstr = mc_value_stringgetdata(inpval);
    inplen = mc_value_stringgetlength(inpval);
    resstr = mc_value_makestringlen(state, inpstr, inplen);
    mc_value_getallocateddata(resstr)->uvobj.valstring.strbuf->toUppercase();

    return resstr;
    
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

mcvalue_t mc_vm_callvalue(mcstate_t* state, GenericList<mcvalue_t>* constants, mcvalue_t callee, mcvalue_t thisval, size_t argc, mcvalue_t* args)
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
    if(constants == nullptr)
    {
        constants = mc_compiler_getconstants(state->compiler);
    }
    type = mc_value_gettype(callee);
    if(type == MC_VAL_FUNCSCRIPT)
    {
        //mc_callframe_init(&tempframe, callee, 0);
        mc_callframe_init(&tempframe, callee, state->execstate.vsposition - argc);
        mc_vm_savestate(state, &est);
        oldsp = state->execstate.vsposition;
        oldthissp = state->execstate.thisstpos;
        mc_vm_pushframe(state, tempframe);
        oldframescount = state->execstate.framestack->count();
        mc_vmintern_stackpush(state, callee);
        for(i = 0; i < argc; i++)
        {
            mc_vmintern_stackpush(state, args[i]);
        }
        ok = mc_function_execfunction(state, callee, constants, true);
        if(!ok)
        {
            mc_vm_restorestate(state, &est);
            return mc_value_makenull();
        }
        #if 1
        while(state->execstate.framestack->count() > oldframescount)
        {
            mc_vmintern_popframe(state);
        }
        #endif
        state->execstate.thisstpos = oldthissp;
        retv = state->execstate.lastpopped;
        mc_vm_restorestate(state, &est);
        return retv;
    }
    if(type == MC_VAL_FUNCNATIVE)
    {
        return mc_vm_callnativefunction(state, callee, srcposinvalid, thisval, argc, args);
    }
    state->errors.pushFormat(MC_ERROR_USER, srcposinvalid, "object is not callable");
    return mc_value_makenull();
}

mcvalue_t mc_objfnarray_map(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    size_t i;
    size_t len;
    mcvalue_t val;
    mcvalue_t res;
    mcvalue_t callee;
    mcvalue_t vargs[3];
    mcvalue_t narr;
    GenericList<mcvalue_t>* ary;
    GenericList<mcvalue_t>* nary;
    (void)state;
    (void)data;
    (void)argc;
    callee = args[0];
    narr = mc_value_makearray(state);
    nary = mc_value_arraygetactualarray(narr);
    ary = mc_value_arraygetactualarray(thisval);
    len = ary->count();
    for(i=0; i<len; i++)
    {
        val = ary->get(i);
        vargs[0] = val;
        vargs[1] = mc_value_makenumber(i);
        res = mc_vm_callvalue(state, nullptr, callee, thisval, 1, vargs);
        nary->push(res);
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
    Printer pr;
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
    Printer::initFromStack(&pr, state, nullptr, true);
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
    str = pr.m_prdestbuf->data();
    slen = pr.m_prdestbuf->length();
    rt = mc_value_makestringlen(state, str, slen);
    Printer::releaseFromPtr(&pr, true);
    return rt;
}

mcvalue_t mc_objfnmap_length(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    size_t len;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    len = mc_value_mapgetlength(thisval);
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
        mc_class_addmember(state, state->stdobjstring, "match", mc_objfnstring_matchglobcase);
        mc_class_addmember(state, state->stdobjstring, "imatch", mc_objfnstring_matchglobicase);
        mc_class_addmember(state, state->stdobjstring, "trim", mc_objfnstring_trim);
        mc_class_addmember(state, state->stdobjstring, "toLower", mc_objfnstring_tolower);
        mc_class_addmember(state, state->stdobjstring, "toUpper", mc_objfnstring_toupper);
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
        state->errors.pushFormat(MC_ERROR_RUNTIME, srcposinvalid, "range step cannot be 0");
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
    len = mc_value_mapgetlength(arg);
    for(i = 0; i < len; i++)
    {
        key = mc_value_mapgetkeyat(arg, i);
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
    len = mc_value_mapgetlength(arg);
    for(i = 0; i < len; i++)
    {
        key = mc_value_mapgetvalueat(arg, i);
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
        return mc_value_makeerror(state, mc_value_stringgetdata(args[0]));
    }
    return mc_value_makeerror(state, "");
}

mcvalue_t mc_scriptfn_crash(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    (void)data;
    (void)thisval;
    if(argc == 1 && mc_value_gettype(args[0]) == MC_VAL_STRING)
    {
        state->errors.pushMessage(MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), mc_value_stringgetdata(args[0]));
    }
    else
    {
        state->errors.pushMessage(MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), "");
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
        state->errors.pushFormat(MC_ERROR_RUNTIME, srcposinvalid, "assertion failed");
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
            state->errors.pushFormat(MC_ERROR_RUNTIME, srcposinvalid, "max is bigger than min");
            return mc_value_makenull();
        }
        range = max - min;
        res = min + (res * range);
        return mc_value_makenumber(res);
    }
    state->errors.pushFormat(MC_ERROR_RUNTIME, srcposinvalid, "invalid number or arguments");
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
    str = mc_value_stringgetdata(args[0]);
    len = mc_value_stringgetlength(args[0]);
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
        mc_value_stringappendlen(res, &c, 1);
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
    path = mc_value_stringgetdata(args[0]);
    string = mc_value_stringgetdata(args[1]);
    slen = mc_value_stringgetlength(args[1]);
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
    path = mc_value_stringgetdata(args[0]);
    contents = mc_fsutil_fileread(state, path, &flen);
    if(!contents)
    {
        return mc_value_makenull();
    }
    res = mc_value_makestringlen(state, contents, flen);
    mc_memory_free(contents);
    contents = nullptr;
    return res;
}

mcvalue_t mc_nsfnfile_join(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    size_t i;
    mcvalue_t res;
    mcvalue_t arg;
    (void)data;
    (void)thisval;
    res = mc_value_makestringlen(state, "", 0);
    for(i=0; i<argc; i++)
    {
        arg = args[i];
        mc_value_stringappendvalue(res, arg);
        if((i + 1) < argc)
        {
            mc_value_stringappendlen(res, "/", 1);
        }
    }
    return res;
}

mcvalue_t mc_nsfnfile_isdirectory(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    const char* path;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_STRING))
    {
        return mc_value_makenull();
    }
    path = mc_value_stringgetdata(args[0]);
    return mc_value_makebool(osfn_pathisdirectory(path));
}

mcvalue_t mc_nsfnfile_isfile(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    const char* path;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_STRING))
    {
        return mc_value_makenull();
    }
    path = mc_value_stringgetdata(args[0]);
    return mc_value_makebool(osfn_pathisfile(path));
}

mcvalue_t mc_nsfnfile_stat(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    mcvalue_t resmap;
    const char* path;
    const char* fullpath;
    struct stat st;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    char fpbuffer[1024];
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_STRING))
    {
        return mc_value_makenull();
    }
    path = mc_value_stringgetdata(args[0]);
    if(stat(path, &st) == 0)
    {
        resmap = mc_value_makemap(state);
        fullpath = osfn_realpath(path, fpbuffer);
        mc_value_mapsetvaluestring(resmap, "dev", mc_value_makenumber(st.st_dev));
        mc_value_mapsetvaluestring(resmap, "ino", mc_value_makenumber(st.st_ino));
        mc_value_mapsetvaluestring(resmap, "mode", mc_value_makenumber(st.st_mode));
        mc_value_mapsetvaluestring(resmap, "nlink", mc_value_makenumber(st.st_nlink));
        mc_value_mapsetvaluestring(resmap, "uid", mc_value_makenumber(st.st_uid));
        mc_value_mapsetvaluestring(resmap, "gid", mc_value_makenumber(st.st_gid));
        mc_value_mapsetvaluestring(resmap, "size", mc_value_makenumber(st.st_size));
        mc_value_mapsetvaluestring(resmap, "path", mc_value_makestring(state, fullpath));
        return resmap;
    }
    return mc_value_makenull();
}

mcvalue_t mc_nsfndir_readdir(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    bool isdot;
    bool joinpaths;
    const char* path;
    mcvalue_t res;
    mcvalue_t vjustname;
    mcvalue_t vfullpath;
    mcvalue_t vpath;
    mcvalue_t vrespath;
    FSDirItem ent;
    FSDirReader reader;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    joinpaths = false;
    if(!mc_argcheck_check(state, true, argc, args, MC_VAL_STRING, MC_VAL_BOOL))
    {
        return mc_value_makenull();
    }
    vpath = args[0];
    path = mc_value_stringgetdata(vpath);
    if(argc > 1)
    {
        joinpaths = mc_value_asbool(args[1]);
    }
    if(fslib_diropen(&reader, path))
    {
        res = mc_value_makearray(state);
        while(fslib_dirread(&reader, &ent))
        {
            isdot = ((strcmp(ent.name, ".") == 0) || (strcmp(ent.name, "..") == 0));
            if(isdot)
            {
                continue;
            }
            vjustname = mc_value_makestringlen(state, ent.name, strlen(ent.name));
            vrespath = vjustname;
            if(joinpaths)
            {
                vfullpath = mc_value_makestringlen(state, "", 0);
                mc_value_stringappendvalue(vfullpath, vpath);
                mc_value_stringappendlen(vfullpath, "/", 1);
                mc_value_stringappendvalue(vfullpath, vjustname);
                vrespath = vfullpath;
            }
            mc_value_arraypush(res, vrespath);
        }
        fslib_dirclose(&reader);
        return res;
    }
    state->errors.pushMessage(MC_ERROR_RUNTIME, mc_callframe_getpos(state->execstate.currframe), strerror(errno));
    return mc_value_makenull();
}

mcvalue_t mc_nsfnvm_hadrecovered(mcstate_t* state, void* data, mcvalue_t thisval, size_t argc, mcvalue_t* args)
{
    (void)data;
    (void)thisval;
    (void)argc;
    (void)args;
    return mc_value_makebool(state->hadrecovered);
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
        {nullptr, nullptr}
    };
    int i;
    for(i=0; nativefunctions[i].name != nullptr; i++)
    {
        mc_state_setnativefunction(state, nativefunctions[i].name, nativefunctions[i].fn, nullptr);
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

#define putnsfield(map, fnname, function) \
    mc_value_mapsetvaluestring(map, fnname, mc_value_makefuncnative(state, fnname, function, nullptr));


void mc_cli_installjsondummy(mcstate_t* state)
{
    mcvalue_t jmap;
    jmap = mc_value_makemap(state);
    putnsfield(jmap, "stringify", mc_nsfnjson_stringify);
    mc_state_setglobalconstant(state, "JSON", jmap);
}

void mc_cli_installjsconsole(mcstate_t* state)
{
    mcvalue_t jmap;
    jmap = mc_value_makemap(state);
    putnsfield(jmap, "log", mc_scriptfn_println);
    mc_state_setglobalconstant(state, "console", jmap);
}

void mc_cli_installmath(mcstate_t* state)
{
    mcvalue_t jmap;
    jmap = mc_value_makemap(state);
    putnsfield(jmap, "sqrt", mc_nsfnmath_sqrt);
    putnsfield(jmap, "pow", mc_nsfnmath_pow);
    putnsfield(jmap, "sin", mc_nsfnmath_sin);
    putnsfield(jmap, "cos", mc_nsfnmath_cos);
    putnsfield(jmap, "tan", mc_nsfnmath_tan);
    putnsfield(jmap, "log", mc_nsfnmath_log);
    putnsfield(jmap, "ceil", mc_nsfnmath_ceil);
    putnsfield(jmap, "floor", mc_nsfnmath_floor);
    putnsfield(jmap, "abs", mc_nsfnmath_abs);
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
    putnsfield(map, "read", mc_nsfnfile_readfile);
    putnsfield(map, "write", mc_nsfnfile_writefile);
    putnsfield(map, "put", mc_nsfnfile_writefile);
    putnsfield(map, "join", mc_nsfnfile_join);
    putnsfield(map, "isDirectory", mc_nsfnfile_isdirectory);
    putnsfield(map, "isFile", mc_nsfnfile_isfile);
    putnsfield(map, "stat", mc_nsfnfile_stat);
    mc_state_setglobalconstant(state, "File", map);
}

void mc_cli_installdir(mcstate_t* state)
{
    mcvalue_t map;
    map = mc_value_makemap(state);
    putnsfield(map, "read", mc_nsfndir_readdir);
    mc_state_setglobalconstant(state, "Dir", map);
}

void mc_cli_installvmvar(mcstate_t* state)
{
    mcvalue_t map;
    map = mc_value_makemap(state);
    putnsfield(map, "hadRecovered", mc_nsfnvm_hadrecovered);
    mc_state_setglobalconstant(state, "VM", map);
}


static int g_extfnvar;

void mc_cli_installotherstuff(mcstate_t* state)
{
    mc_state_setglobalconstant(state, "test", mc_value_makenumber(42));
    mc_state_setnativefunction(state, "external_fn_test", mc_scriptfn_externalfn, &g_extfnvar);
    mc_state_setnativefunction(state, "test_check_args", mc_scriptfn_testcheckargs, nullptr);
    mc_state_setnativefunction(state, "vec2_add", mc_scriptfn_vec2add, nullptr);
    mc_state_setnativefunction(state, "vec2_sub", mc_scriptfn_vec2sub, nullptr);
    mc_cli_installfileio(state);
    mc_cli_installdir(state);
    mc_cli_installvmvar(state);
}


bool mc_cli_compileandrunsource(mcstate_t* state, mcvalue_t* vdest, const char* source, const char* filename)
{
    bool ok;
    mcvalue_t tmp;
    mccompiledprogram_t* program;
    ok = false;
    program = mc_state_compilesource(state, source, filename);
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
    if(vdest != nullptr)
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
    if(code == nullptr)
    {
        return false;
    }
    ok = mc_cli_compileandrunsource(state, nullptr, code, filename);
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

#define printtypesize(typexpr...) \
    mc_cli_printtypesize(#typexpr, sizeof(typexpr))

void mc_cli_printtypesize(const char* name, size_t sz)
{
    printf("%ld\t%s\n", sz, name);
}

void mc_cli_printtypesizes()
{
    printtypesize(PtrDict);
    printtypesize(GenericDict<mcvalue_t, mcvalue_t>);
    printtypesize(PtrList);
    printtypesize(mcprintconfig_t);
    printtypesize(Printer);
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
    printtypesize(mcobjfunction_t);
    printtypesize(mcobjuserdata_t);
    printtypesize(mcobjerror_t);
    printtypesize(mcobjstring_t);
    printtypesize(mcopdefinition_t);
    printtypesize(mcastscopeblock_t);
    printtypesize(mcastscopefile_t);
    printtypesize(mcastscopecomp_t);
    printtypesize(mcgcobjdatapool_t);
    printtypesize(AstLexer);
    printtypesize(AstLexInfo);
    printtypesize(mcvmframe_t);
    printtypesize(mctraceitem_t);
    printtypesize(mcmodule_t);
    printtypesize(mcstoddiyfpconv_t);
    printtypesize(mcvalunion_t);
    printtypesize(mcobjunion_t);
    printtypesize(mcexprunion_t);
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
    {"strict", 's', OPTPARSE_NONE, "enable strict mode"},
    {0, 0, (optargtype_t)0, nullptr}
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
    evalcode = nullptr;
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
            optprs_printusage(argv, longopts, false);
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
        else if(co == 's')
        {
            state->config.strictmode = true;
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
    if(evalcode != nullptr)
    {
        nargv[0] = (char*)"<-e>";
        nargc++;
    }
    while(true)
    {
        arg = optprs_nextpositional(&options);
        if(arg == nullptr)
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
    if(evalcode != nullptr)
    {
        mc_cli_compileandrunsource(state, &tmp, evalcode, "<-e>");
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

