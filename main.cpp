
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
#define MC_CONF_MINVMGLOBALS (4)
#define MC_CONF_MINNATIVETHISSTACKSIZE (32)
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
#define MC_CONF_FREEVALSCOUNT (4)


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


#if 1
    #define MC_UTIL_INCCAPACITY(capacity) (((capacity) < 8) ? 8 : ((capacity) * 2))
#else
    #define MC_UTIL_INCCAPACITY(capacity) ((capacity) + 15) / 16 * 16;
#endif


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



typedef enum mcerrtype_t mcerrtype_t;


typedef enum mcastmathoptype_t mcastmathoptype_t;

typedef enum mcopcode_t mcopcode_t;
typedef enum mcastsymtype_t mcastsymtype_t;

typedef struct mcstoddiyfp_t mcstoddiyfp_t;

typedef struct mcexecstate_t mcexecstate_t;

typedef struct mcobjdata_t mcobjdata_t;

typedef struct mcconfig_t mcconfig_t;


typedef struct mcobjuserdata_t mcobjuserdata_t;
typedef struct mcobjerror_t mcobjerror_t;
typedef struct mcobjstring_t mcobjstring_t;
typedef struct mcobjmap_t mcobjmap_t;
typedef struct mcobjarray_t mcobjarray_t;


typedef struct mcopdefinition_t mcopdefinition_t;

typedef struct mcconsolecolor_t mcconsolecolor_t;
typedef struct mcobjfunction_t mcobjfunction_t;

class PtrList;
class Printer;
class PtrDict;
class AstLexer;
class AstInfo;
class AstSymTable;
class Error;
class ErrList;
class Traceback;
class Module;
class GCMemory;
class AstParser;
class AstSymbol;
class AstLocation;
class AstScopeComp;
class Value;
class AstSourceFile;
class AstCompiler;
class AstPrinter;
class State;
class VMFrame;
class CompiledProgram;
class AstExpression;
class Token;
class SymStore;
class ObjClass;
template<typename> class GenericList;

typedef Value (*mcnativefn_t)(State*, void*, Value, size_t, Value*);
typedef size_t (*mcitemhashfn_t)(void*);
typedef bool (*mcitemcomparefn_t)(void*, void*);
typedef void (*mcitemdestroyfn_t)(void*);
typedef void* (*mcitemcopyfn_t)(void*);
typedef void (*mcitemdeinitfn_t)(void*);
typedef AstExpression* (*mcastrightassocparsefn_t)(AstParser*);
typedef AstExpression* (*mcleftassocparsefn_t)(AstParser*, AstExpression*);

template<typename... ArgsT>
void mc_util_complain(AstLocation pos, const char *fmt, ArgsT&&... args);

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



/* protos */
MC_INLINE const char* mc_value_stringgetdata(Value object);

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
char *mc_fsutil_fileread(const char *filename, size_t *flen);
size_t mc_fsutil_filewrite(const char *path, const char *string, size_t stringsize);
size_t mc_util_strlen(const char *str);
char *mc_util_strndup(const char *string, size_t n);
char *mc_util_strdup(const char *string);
bool mc_util_strequal(const char *a, const char *b);
bool mc_util_strnequal(const char *a, const char *b, size_t len);
char *mc_util_canonpath(const char *path);
bool mc_util_pathisabsolute(const char *path);
size_t mc_util_hashdata(const void *ptr, size_t len);
size_t mc_util_hashdouble(mcfloat_t val);
size_t mc_util_upperpowoftwo(size_t v);
mcfloat_t mc_util_strtod(const char *str, size_t slen, char **endptr);
uint64_t mc_util_doubletouint64(mcfloat_t val);
mcfloat_t mc_util_uint64todouble(uint64_t val);

Value mc_value_makestrcapacity(State *state, int capacity);
Value mc_value_makestringlen(State *state, const char *string, size_t len);
Value mc_value_makestring(State *state, const char *string);
Value mc_value_makefuncnative(State *state, const char *name, mcnativefn_t fn, void *data);
Value mc_value_makearray(State *state);
Value mc_value_makearraycapacity(State *state, size_t capacity);
Value mc_value_makemap(State *state);
Value mc_value_makemapcapacity(State *state, size_t capacity);
Value mc_value_makeerror(State *state, const char *error);
Value mc_value_makeerrornocopy(State *state, char *error);
Value mc_value_makefuncscript(State *state, const char *name, CompiledProgram *cres, bool ownsdt, int nlocals, int nargs, int fvc);
Value mc_value_makeuserobject(State *state, void *data);
mcobjuserdata_t *mc_value_userdatagetdata(Value object);
bool mc_value_userdatasetdata(Value object, void *extdata);
bool mc_value_userdatasetdestroyfunction(Value object, mcitemdestroyfn_t dfn);
bool mc_value_userdatasetcopyfunction(Value object, mcitemcopyfn_t copyfn);


int mc_value_stringgetlength(Value object);
void mc_value_stringsetlength(Value object, int len);
bool mc_value_stringappendlen(Value obj, const char *src, size_t len);
bool mc_value_stringappend(Value obj, const char *src);
bool mc_value_stringappendvalue(Value destval, Value val);
size_t mc_value_stringgethash(Value obj);
bool mc_value_stringrehash(Value obj);

const char *mc_value_functiongetname(Value obj);
Value mc_value_functiongetfreevalat(Value obj, int ix);
void mc_value_functionsetfreevalat(Value obj, int ix, Value val);
Value *mc_value_functiongetfreevals(Value obj);
const char *mc_value_errorgetmessage(Value object);
void mc_value_errorsettraceback(Value object, Traceback *traceback);
Traceback *mc_value_errorgettraceback(Value object);
Value mc_value_arraygetvalue(Value object, size_t ix);
bool mc_value_arraysetvalue(Value object, size_t ix, Value val);
bool mc_value_arraypush(Value object, Value val);
int mc_value_arraygetlength(Value object);
Value mc_valarray_pop(Value object);
bool mc_value_arrayremoveat(Value object, int ix);
int mc_value_mapgetlength(Value object);
Value mc_value_mapgetkeyat(Value object, int ix);
Value mc_value_mapgetvalueat(Value object, int ix);
bool mc_value_mapsetvalueat(Value object, int ix, Value val);
Value mc_value_mapgetkvpairat(State *state, Value object, int ix);
bool mc_value_mapsetvalue(Value object, Value key, Value val);
bool mc_value_mapsetvaluestring(Value object, const char *strkey, Value val);
Value mc_value_mapgetvalue(Value object, Value key);
bool mc_value_mapgetvaluechecked(Value object, Value key, Value *dest);
bool mc_value_maphaskey(Value object, Value key);
void mc_objectdata_deinit(mcobjdata_t *data);


bool mc_printutil_bcreadoperands(mcopdefinition_t *def, const uint16_t *instr, uint64_t outoperands[2]);
void mc_printer_printoneinstruc(Printer *pr, uint16_t *code, uint16_t op, size_t *pos, AstLocation *sposlist, bool simple);
void mc_printer_printbytecode(Printer *pr, uint16_t *code, AstLocation *sposlist, size_t codesize, bool simple);
void mc_printer_printobjstring(Printer *pr, Value obj);
void mc_printer_printobjfuncscript(Printer *pr, Value obj);
void mc_printer_printobjarray(Printer *pr, Value obj);
void mc_printer_printobjmap(Printer *pr, Value obj);
void mc_printer_printobjerror(Printer *pr, Value obj);
void mc_consolecolor_init(mcconsolecolor_t *mcc, int fd);
const char *mc_consolecolor_get(mcconsolecolor_t *mcc, char code);
void mc_printer_printvalue(Printer *pr, Value obj, bool accurate);

MC_FORCEINLINE mcshiftint_t mc_mathutil_binshiftleft(mcfloat_t dnleft, mcfloat_t dnright);
MC_FORCEINLINE mcshiftint_t mc_mathutil_binshiftright(mcfloat_t dnleft, mcfloat_t dnright);
MC_FORCEINLINE mcfloat_t mc_mathutil_binor(mcfloat_t dnleft, mcfloat_t dnright);
MC_FORCEINLINE mcfloat_t mc_mathutil_binand(mcfloat_t dnleft, mcfloat_t dnright);
MC_FORCEINLINE mcfloat_t mc_mathutil_binxor(mcfloat_t dnleft, mcfloat_t dnright);
MC_FORCEINLINE mcfloat_t mc_mathutil_add(mcfloat_t dnleft, mcfloat_t dnright);
MC_FORCEINLINE mcfloat_t mc_mathutil_sub(mcfloat_t dnleft, mcfloat_t dnright);
MC_FORCEINLINE mcfloat_t mc_mathutil_mult(mcfloat_t dnleft, mcfloat_t dnright);
MC_FORCEINLINE mcfloat_t mc_mathutil_div(mcfloat_t dnleft, mcfloat_t dnright);
MC_FORCEINLINE mcfloat_t mc_mathutil_mod(mcfloat_t dnleft, mcfloat_t dnright);



Value mc_program_execute(State *state, CompiledProgram *program);
Value mc_state_execcode(State *state, const char *code, const char *filename);

bool mc_error_printusererror(Printer *pr, Value obj);

const char *mc_util_mathopstring(mcastmathoptype_t op);

mcobjdata_t *mc_gcmemory_allocobjectdata(State *state);

void mc_state_gcunmarkall(State *state);
void mc_state_gcmarkobjlist(Value *objects, size_t count);
void mc_state_gcmarkobject(Value obj);
void mc_state_gcsweep(State *state);
int mc_state_gcshouldsweep(State *state);
bool mc_state_gcdisablefor(Value obj);
void mc_state_gcenablefor(Value obj);
bool mc_state_gccandatabeputinpool(State *state, mcobjdata_t *data);
bool mc_vm_execvm(State* state, Value function, GenericList<Value>* constants, bool nested);

bool mc_traceback_vmpush(Traceback *traceback, State *state);
bool mc_vm_init(State *state);
void mc_vm_reset(State *state);
void mc_vmutil_getopinfo(mcopcode_t opc, const char **oname);
Value mc_scriptfn_typeof(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_arrayfirst(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_arraylast(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_arrayrest(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_reverse(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_makearray(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_externalfn(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_vec2add(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_vec2sub(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_testcheckargs(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_maketestdict(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_squarearray(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_print(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_println(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_tostring(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnjson_stringify(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnnumber_chr(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_length(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_indexof(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_charcodefirst(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_charcodeat(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_charat(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_getself(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_tonumber(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_left(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_right(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_replaceall(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_replacefirst(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_trim(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_matchhelper(State *state, void *data, Value thisval, size_t argc, Value *args, bool icase);
Value mc_objfnstring_matchglobcase(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_matchglobicase(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_tolower(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnstring_toupper(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnarray_length(State *state, void *data, Value thisval, size_t argc, Value *args);
void mc_vm_savestate(State *state, mcexecstate_t *est);
void mc_vm_restorestate(State *state, mcexecstate_t *est);
Value mc_objfnarray_map(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnarray_push(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnarray_pop(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnarray_join(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnmap_length(State *state, void *data, Value thisval, size_t argc, Value *args);

Value mc_objfnobject_iscallable(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnobject_isstring(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnobject_isarray(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnobject_ismap(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnobject_isnumber(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnobject_isbool(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnobject_isnull(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnobject_isfuncscript(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnobject_isexternal(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnobject_iserror(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_objfnobject_isfuncnative(State *state, void *data, Value thisval, size_t argc, Value *args);
void mc_state_makestdclasses(State *state);
Value mc_scriptfn_isnan(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_range(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_keys(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_values(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_copy(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_copydeep(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_remove(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_removeat(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_error(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_crash(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_assert(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_randseed(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_random(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptutil_slicearray(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptutil_slicestring(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_scriptfn_slice(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnmath_sqrt(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnmath_pow(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnmath_sin(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnmath_cos(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnmath_tan(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnmath_log(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnmath_ceil(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnmath_floor(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnmath_abs(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnfile_writefile(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnfile_readfile(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnfile_join(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnfile_isdirectory(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnfile_isfile(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnfile_stat(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfndir_readdir(State *state, void *data, Value thisval, size_t argc, Value *args);
Value mc_nsfnvm_hadrecovered(State *state, void *data, Value thisval, size_t argc, Value *args);
void mc_cli_installbuiltins(State *state);
void mc_cli_installjsondummy(State *state);
void mc_cli_installjsconsole(State *state);
void mc_cli_installmath(State *state);
void mc_cli_installfauxjavascript(State *state);
void mc_cli_installfileio(State *state);
void mc_cli_installdir(State *state);
void mc_cli_installvmvar(State *state);
void mc_cli_installotherstuff(State *state);
bool mc_cli_compileandrunsource(State *state, Value *vdest, const char *source, const char *filename);
bool mc_cli_compileandrunfile(State *state, const char *filename);
void mc_cli_installargv(State *state, int argc, char **argv, int beginat);
void mc_cli_printtypesize(const char *name, size_t sz);
void mc_cli_printtypesizes(void);
int main(int argc, char *argv[]);
void *mc_memory_malloc(size_t sz);
void *mc_memory_realloc(void *p, size_t nsz);
void *mc_memory_calloc(size_t count, size_t typsize);
void mc_memory_free(void *ptr);
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



struct mcconsolecolor_t
{
    int targetfds[5];
    size_t fdcount;
    bool ispiped;
};




class Printer
{
    public:
        struct Config
        {
            bool verbosefunc;
            bool quotstring;
            bool shouldflush;
        };

    public:
        static void destroy(Printer* pr)
        {
            releaseFromPtr(pr, true);
            if(!pr->m_prisstack)
            {
                mc_memory_free(pr);
            }
        }

        static bool initFromStack(Printer* pr, FILE* ofh, bool onstack)
        {
            return initFromStack(pr, ofh, onstack, false);
        }

        static bool initFromStack(Printer* pr, FILE* ofh, bool onstack, bool isnullctor)
        {
            pr->m_prfailed = false;
            pr->m_prdestrfile = ofh;
            pr->m_prisstack = onstack;
            pr->m_prdestbuf = nullptr;
            pr->m_prconfig.verbosefunc = true;
            pr->m_prconfig.quotstring = false;
            pr->m_prconfig.shouldflush = false;
            if(!isnullctor)
            {
                if(pr->m_prdestrfile == nullptr)
                {
                    pr->m_prdestbuf = Memory::make<StringBuffer>(0);
                }
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
        Config m_prconfig;
        bool m_prfailed;
        bool m_prisstack;
        FILE* m_prdestrfile;
        StringBuffer* m_prdestbuf;

    public:
        Printer()
        {
            MC_ASSERT(Printer::initFromStack(this, nullptr, false, true));
        }

        Printer(FILE* ofh)
        {
            MC_ASSERT(Printer::initFromStack(this, ofh, false));
        }

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
            InitialSize = 8,
        };

    private:
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

    public:
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
        inline PtrList(size_t tsz, bool isp)
        {
            bool ok;
            (void)ok;
            ok = initCapacity(this, InitialSize, tsz, isp);
            MC_ASSERT(ok);
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
            (void)ok;
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
            (void)ok;
            capacity = mc_util_upperpowoftwo(mincapacity * 2);
            ok = GenericDict::initDict(this, capacity);
            MC_ASSERT(ok);
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
            (void)ok;
            if(m_vdcount >= m_vditemcapacity)
            {
                ok = this->growAndRehash();
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
            (void)ok;
            ncap = MC_UTIL_INCCAPACITY(m_vdcellcapacity);    
            GenericDict newdict(ncap);
            newdict.m_funckeyequalsfn = m_funckeyequalsfn;
            newdict.m_funchashfn = m_funchashfn;
            for(i = 0; i < m_vdcount; i++)
            {
                key = (char*)this->getKeyAt(i);
                value = this->getValueAt(i);
                ok = newdict.setKV(key, value);
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
            (void)ok;
            ok = PtrDict::initValues(this, cap, copyfn, dfn);
            MC_ASSERT(ok);
        }

        PtrDict(mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
        {
            bool ok;
            (void)ok;
            ok = PtrDict::initValues(this, MC_CONF_GENERICDICTINITSIZE, copyfn, dfn);
            MC_ASSERT(ok);
        }

        bool growAndRehash()
        {
            bool ok;
            unsigned int i;
            char* key;
            void* value;
            size_t ncap;
            (void)ok;
            ncap = MC_UTIL_INCCAPACITY(this->gdcellcapacity);
            PtrDict newdict(ncap, this->funccopyfn, this->funcdestroyfn);
            for(i = 0; i < this->gdcount; i++)
            {
                key = this->gdkeys[i];
                value = this->gdvalues[i];
                ok = newdict.setActual(key, key, value);
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
            (void)ok;
            if(this->gdcount >= this->gditemcapacity)
            {
                ok = this->growAndRehash();
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
            (void)ok;
            if(!this->funccopyfn || !this->funcdestroyfn)
            {
                return nullptr;
            }
            dictcopy = Memory::make<PtrDict>(this->funccopyfn, this->funcdestroyfn);
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
            }
            return dictcopy;
        }
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


class Value
{
    public:
        enum Type
        {
            VALTYP_NONE,
            VALTYP_ERROR,
            VALTYP_NUMBER,
            VALTYP_BOOL,
            VALTYP_STRING,
            VALTYP_NULL,
            VALTYP_FUNCNATIVE,
            VALTYP_ARRAY,
            VALTYP_MAP,
            VALTYP_FUNCSCRIPT,
            VALTYP_EXTERNAL,
            VALTYP_FREED,
            /* for checking types with & */
            VALTYP_ANY
        };

        struct CompareResult
        {
            mcfloat_t result;
        };

    public:
        static inline bool isHashable(Value obj)
        {
            Value::Type type = obj.getType();
            switch(type)
            {
                case Value::VALTYP_STRING:
                case Value::VALTYP_NUMBER:
                case Value::VALTYP_BOOL:
                    return true;
                default:
                    break;
            }
            return false;
        }

        static const char* getTypename(Type type)
        {
            switch(type)
            {
                case Value::VALTYP_NONE:
                    return "none";
                case Value::VALTYP_ERROR:
                    return "error";
                case Value::VALTYP_NUMBER:
                    return "number";
                case Value::VALTYP_BOOL:
                    return "boolean";
                case Value::VALTYP_STRING:
                    return "string";
                case Value::VALTYP_NULL:
                    return "null";
                case Value::VALTYP_ARRAY:
                    return "array";
                case Value::VALTYP_MAP:
                    return "object";
                case Value::VALTYP_FUNCSCRIPT:
                case Value::VALTYP_FUNCNATIVE:
                    return "function";
                case Value::VALTYP_EXTERNAL:
                    return "external";
                case Value::VALTYP_FREED:
                    return "freed";
                case Value::VALTYP_ANY:
                    return "any";
                default:
                    break;
            }
            return "none";
        }

        static void unionNameCheckType(Printer* res, Type type, Type t, bool* inbetween)
        {
            if((type & t) == t)
            {
                if(*inbetween)
                {
                    res->put("|");
                }
                res->put(Value::getTypename(t));
                *inbetween = true;
            }
        }

        static char* getUnionName(Type type)
        {
            bool inbetween;
            Printer* res;
            if(type == Value::VALTYP_ANY || type == Value::VALTYP_NONE || type == Value::VALTYP_FREED)
            {
                return mc_util_strdup(Value::getTypename(type));
            }
            res = Memory::make<Printer>(nullptr);
            inbetween = false;
            unionNameCheckType(res, type, Value::VALTYP_NUMBER, &inbetween);
            unionNameCheckType(res, type, Value::VALTYP_BOOL, &inbetween);
            unionNameCheckType(res, type, Value::VALTYP_STRING, &inbetween);
            unionNameCheckType(res, type, Value::VALTYP_NULL, &inbetween);
            unionNameCheckType(res, type, Value::VALTYP_FUNCNATIVE, &inbetween);
            unionNameCheckType(res, type, Value::VALTYP_ARRAY, &inbetween);
            unionNameCheckType(res, type, Value::VALTYP_MAP, &inbetween);
            unionNameCheckType(res, type, Value::VALTYP_FUNCSCRIPT, &inbetween);
            unionNameCheckType(res, type, Value::VALTYP_EXTERNAL, &inbetween);
            unionNameCheckType(res, type, Value::VALTYP_ERROR, &inbetween);
            return res->getStringAndDestroy(nullptr);
        }

        static inline bool compare(Value a, Value b, CompareResult* cres)
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
            Value::Type atype;
            Value::Type btype;
            /*
            if(a.odata == b.odata)
            {
                return 0;
            }
            */
            cres->result = 1;
            atype = a.getType();
            btype = b.getType();
            if((atype == Value::VALTYP_NUMBER || atype == Value::VALTYP_BOOL || atype == Value::VALTYP_NULL) && (btype == Value::VALTYP_NUMBER || btype == Value::VALTYP_BOOL || btype == Value::VALTYP_NULL))
            {
                dnleft = Value::asNumber(a);
                dnright = Value::asNumber(b);
                cres->result = (dnleft - dnright);
                return true;
            }
            if(atype == btype && atype == Value::VALTYP_STRING)
            {
                alen = mc_value_stringgetlength(a);
                blen = mc_value_stringgetlength(b);
                #if 0
                fprintf(stderr, "compare: alen=%d, blen=%d\n", alen, blen);
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
            if((a.isAllocated() || a.isNull()) && (b.isAllocated() || b.isNull()))
            {
                adataval = (intptr_t)a.getAllocatedData();
                bdataval = (intptr_t)b.getAllocatedData();
                cres->result = (mcfloat_t)(adataval - bdataval);
                return true;
            }
            return false;
        }

        static inline bool equalsTo(Value a, Value b)
        {
            bool ok;
            CompareResult cres;
            Value::Type atype;
            Value::Type btype;
            (void)ok;
            atype = a.getType();
            btype = b.getType();
            if(atype != btype)
            {
                return false;
            }
            ok = Value::compare(a, b, &cres);
            return MC_UTIL_CMPFLOAT(cres.result, 0);
        }

        static bool callbackEqualsTo(Value* aptr, Value* bptr)
        {
            Value a;
            Value b;
            a = *aptr;
            b = *bptr;
            return equalsTo(a, b);
        }

        static size_t callbackHash(Value* objptr)
        {
            bool bval;
            mcfloat_t dval;
            Value obj;
            Value::Type type;
            obj = *objptr;
            type = obj.getType();
            switch(type)
            {
                case Value::VALTYP_NUMBER:
                    {
                        dval = Value::asNumber(obj);
                        return mc_util_hashdouble(dval);
                    }
                    break;
                case Value::VALTYP_BOOL:
                    {
                        bval = Value::asBool(obj);
                        return bval;
                    }
                    break;
                case Value::VALTYP_STRING:
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
        static Value copyDeepIntern(State* state, Value obj, GenericDict<TypeKeyT, TypeValueT>* targetdict)
        {
            Value::Type type;
            Value copy;
            Value* copyptr;
            copyptr = (Value*)targetdict->get(&obj);
            if(copyptr)
            {
                return *copyptr;
            }
            copy = Value::makeNull();
            type = obj.getType();
            switch(type)
            {
                case Value::VALTYP_FREED:
                case Value::VALTYP_ANY:
                case Value::VALTYP_NONE:
                    {
                        MC_ASSERT(false);
                        copy = Value::makeNull();
                    }
                    break;
                case Value::VALTYP_NUMBER:
                case Value::VALTYP_BOOL:
                case Value::VALTYP_NULL:
                case Value::VALTYP_FUNCNATIVE:
                    {
                        copy = obj;
                    }
                    break;
                case Value::VALTYP_STRING:
                    {
                        bool ok;
                        int len;
                        const char* str;
                        (void)ok;
                        str = mc_value_stringgetdata(obj);
                        len = mc_value_stringgetlength(obj);
                        copy = mc_value_makestringlen(state, str, len);
                        ok = targetdict->setKV(&obj, &copy);
                        if(!ok)
                        {
                            return Value::makeNull();
                        }
                        return copy;
                    }
                    break;
                case Value::VALTYP_FUNCSCRIPT:
                    {
                        return mc_value_copydeepfuncscript(state, obj, targetdict);
                    }
                    break;
                case Value::VALTYP_ARRAY:
                    {
                        return mc_value_copydeeparray(state, obj, targetdict);
                    }
                    break;
                case Value::VALTYP_MAP:
                    {
                        return mc_value_copydeepmap(state, obj, targetdict);
                    }
                    break;
                case Value::VALTYP_EXTERNAL:
                    {
                        copy = Value::copyFlat(state, obj);
                    }
                    break;
                case Value::VALTYP_ERROR:
                    {
                        copy = obj;
                    }
                    break;
            }
            return copy;
        }

        static Value copyDeep(State* state, Value obj)
        {
            Value res;
            auto targetdict = Memory::make<GenericDict<Value, Value>>();
            res = copyDeepIntern(state, obj, targetdict);
            Memory::destroy(targetdict);
            return res;
        }

        static Value copyFlat(State* state, Value obj);

        template<typename ValTypeT, typename ObjDataT>
        static inline Value makeDataFrom(ValTypeT type, ObjDataT* data)
        {
            Value object;
            memset(&object, 0, sizeof(Value));
            object.valtype = type;
            data->odtype = type;
            object.isallocated = true;
            object.uval.odata = data;
            return object;
        }

        static inline Value makeEmpty(Type t)
        {
            Value o = {};
            o.valtype = t;
            o.isallocated = false;
            return o;
        }

        static inline Value makeNumber(mcfloat_t val)
        {
            Value o;
            o = makeEmpty(Value::VALTYP_NUMBER);
            o.uval.valnumber = val;
            return o;
        }

        static inline Value makeBool(bool val)
        {
            Value o;
            o = makeEmpty(Value::VALTYP_BOOL);
            o.uval.valbool = val;
            return o;
        }

        static inline Value makeNull()
        {
            Value o;
            o = makeEmpty(Value::VALTYP_NULL);
            return o;
        }


        static inline bool asBool(Value obj)
        {
            if(obj.isNumber())
            {
                return obj.uval.valnumber;
            }
            return obj.uval.valbool;
        }

        static inline mcfloat_t asNumber(Value obj)
        {
            if(obj.isNumber())
            {
                if(obj.getType() == Value::VALTYP_BOOL)
                {
                    return obj.uval.valbool;
                }
                return obj.uval.valnumber;
            }
            return obj.uval.valnumber;
        }

        static inline mcobjfunction_t* asFunction(Value object);

    public:
        Type valtype;
        bool isallocated;        
        union
        {
            mcobjdata_t* odata;
            mcfloat_t valnumber;
            int valbool;
        } uval;

    public:
        inline Type getType()
        {
            return this->valtype;
        }

        mcobjdata_t* getAllocatedData()
        {
            return this->uval.odata;
        }

        inline bool isAllocated()
        {
            return this->isallocated;
        }

        inline bool isNumeric()
        {
            Type type;
            type = this->getType();
            return type == Value::VALTYP_NUMBER || type == Value::VALTYP_BOOL;
        }

        inline bool isNumber()
        {
            return (this->getType() == Value::VALTYP_NUMBER || this->getType() == Value::VALTYP_BOOL);
        }

        inline bool isNull()
        {
            return this->getType() == Value::VALTYP_NULL;
        }

        inline bool isFuncNative()
        {
            Type type;
            type = this->getType();
            return (type == Value::VALTYP_FUNCNATIVE);
        }

        inline bool isFuncScript()
        {
            Type type;
            type = this->getType();
            return (type == Value::VALTYP_FUNCSCRIPT);
        }

        inline bool isCallable()
        {
            return (this->isFuncNative() || this->isFuncScript());
        }

        inline bool isString()
        {
            Type type;
            type = this->getType();
            return (type == Value::VALTYP_STRING);
        }

        inline bool isMap()
        {
            Type type;
            type = this->getType();
            return (type == Value::VALTYP_MAP);
        }

        inline bool isArray()
        {
            Type type;
            type = this->getType();
            return (type == Value::VALTYP_ARRAY);
        }

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
    Traceback* traceback;
};

struct mcobjstring_t
{
    unsigned long hash;
    StringBuffer* strbuf;
};

struct mcobjmap_t
{
    GenericDict<Value, Value>* actualmap;
};

struct mcobjarray_t
{
    GenericList<Value>* actualarray;
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
                    Value* freevalsallocated;
                    Value freevalsstack[MC_CONF_FREEVALSCOUNT];
                } ufv;
                
                union
                {
                    char* fallocname;
                    const char* fconstname;
                } unamev;
                CompiledProgram* compiledprogcode;
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
            return this->funcdata.valscriptfunc.freevalscount >= MC_CONF_FREEVALSCOUNT;
        }

};

struct mcobjdata_t
{
    State* m_pstate;
    GCMemory* m_objmem;
    Value::Type odtype;
    bool gcmark;    
    union
    {
        mcobjstring_t valstring;
        mcobjerror_t valerror;
        mcobjarray_t* valarray;
        mcobjmap_t* valmap;
        mcobjfunction_t valfunc;
        mcobjuserdata_t valuserobject;
    } uvobj;
};


class GCMemory
{
    public:
        struct DataPool
        {
            public:
                /*
                * nasty lil hack: make use of metaprogramming instantiation to access a
                * type field that hasn't been declared yet. works, but it's still a hack.
                */
                template<typename StateT>
                static DataPool* getPoolForType(StateT* state, Value::Type type)
                {
                    switch(type)
                    {
                        case Value::VALTYP_ARRAY:
                            return &state->m_stategcmem->mempools[0];
                        case Value::VALTYP_MAP:
                            return &state->m_stategcmem->mempools[1];
                        case Value::VALTYP_STRING:
                            return &state->m_stategcmem->mempools[2];
                        default:
                            break;
                    }
                    return nullptr;
                }

            public:
                mcobjdata_t* data[MC_CONF_GCMEMPOOLSIZE];
                int count;
        };

    public:

        static void destroy(GCMemory* m)
        {
            size_t i;
            size_t j;
            mcobjdata_t* obj;
            mcobjdata_t* data;
            DataPool* pool;
            if(m != nullptr)
            {
                PtrList::destroy(m->gcobjlistremains, nullptr);
                PtrList::destroy(m->gcobjlistback, nullptr);
                for(i = 0; i < m->gcobjlist->count(); i++)
                {
                    obj = (mcobjdata_t*)m->gcobjlist->get(i);
                    mc_objectdata_deinit(obj);
                    memset(obj, 0, sizeof(mcobjdata_t));
                    mc_memory_free(obj);
                }
                PtrList::destroy(m->gcobjlist, nullptr);
                for(i = 0; i < MC_CONF_GCMEMPOOLCOUNT; i++)
                {
                    pool = &m->mempools[i];
                    for(j = 0; j < (size_t)pool->count; j++)
                    {
                        data = pool->data[j];
                        mc_objectdata_deinit(data);
                        memset(data, 0, sizeof(mcobjdata_t));
                        mc_memory_free(data);
                    }
                    memset(pool, 0, sizeof(DataPool));
                }
                for(i = 0; i < (size_t)m->onlydatapool.count; i++)
                {
                    mc_memory_free(m->onlydatapool.data[i]);
                }
                mc_memory_free(m);
            }
        }

    public:
        int allocssincesweep;
        PtrList* gcobjlist;
        PtrList* gcobjlistback;
        PtrList* gcobjlistremains;
        DataPool onlydatapool;
        DataPool mempools[MC_CONF_GCMEMPOOLCOUNT];

    public:
        GCMemory()
        {
            int i;
            this->gcobjlist = Memory::make<PtrList>(sizeof(void*), true);
            this->gcobjlistback = Memory::make<PtrList>(sizeof(void*), true);
            this->gcobjlistremains = Memory::make<PtrList>(sizeof(Value), false);
            this->allocssincesweep = 0;
            this->onlydatapool.count = 0;
            for(i = 0; i < MC_CONF_GCMEMPOOLCOUNT; i++)
            {
                DataPool* pool = &this->mempools[i];
                this->mempools[i].count = 0;
                memset(pool, 0, sizeof(DataPool));
            }
        }

};

class ObjClass
{
    public:
        struct Field
        {
            const char* name;
            bool ispseudo;
            mcnativefn_t fndest;
        };

    public:
        static void destroy(ObjClass* cl)
        {
            size_t i;
            Field* memb;
            for(i=0; i<cl->members->count(); i++)
            {
                memb = (Field*)cl->members->get(i);
                mc_memory_free(memb);
            }
            PtrList::destroy(cl->members, nullptr);
            mc_memory_free(cl);
        }

    public:
        const char* classname;
        ObjClass* parentclass;
        Value constructor;
        PtrList* members;

    public:
        ObjClass(const char* name, ObjClass* parclass)
        {
            this->parentclass = parclass;
            this->classname = name;
            this->constructor = Value::makeNull();
            this->members = Memory::make<PtrList>(sizeof(void*), true);
        }

        void addFunction(const char* name, bool ispseudo, mcnativefn_t fn)
        {
            Field* bt;
            bt = Memory::make<Field>();
            bt->name = name;
            bt->ispseudo = ispseudo;
            bt->fndest = fn;
            this->members->push(bt);
        }

        void addMember(const char* name, mcnativefn_t fn)
        {
            return this->addFunction(name, false, fn);
        }

        void addPseudo(const char* name, mcnativefn_t fn)
        {
            return this->addFunction(name, true, fn);
        }

};

struct mcopdefinition_t
{
    const char* name;
    int numoperands;
    int operandwidths[2];
};


class AstLocation
{
    public:
        static AstLocation Invalid()
        {        
            return AstLocation();
        }

    public:
        AstSourceFile* file;
        int line;
        int column;

    public:
        AstLocation(): AstLocation(nullptr, 0, 0)
        {
        }

        AstLocation(AstSourceFile* fi, int nlin, int coln)
        {
            this->file = fi;
            this->line = nlin;
            this->column = coln;
        }

};

class AstSymbol
{
    public:
        static void destroy(AstSymbol* symbol)
        {
            if(symbol != nullptr)
            {
                mc_memory_free(symbol->name);
                symbol->name = nullptr;
                mc_memory_free(symbol);
                symbol = nullptr;
            }
        }

        static AstSymbol* copy(AstSymbol* symbol)
        {
            return Memory::make<AstSymbol>(symbol->name, symbol->symtype, symbol->index, symbol->assignable);
        }

    public:
        mcastsymtype_t symtype;
        char* name;
        int index;
        bool assignable;

    public:
        AstSymbol(const char* syname, mcastsymtype_t sytype, int syindex, bool syassignable)
        {
            this->name = mc_util_strdup(syname);
            MC_ASSERT(this->name);
            this->symtype = sytype;
            this->index = syindex;
            this->assignable = syassignable;
        }
};

class mcastscopeblock_t
{
    public:
        static void destroy(mcastscopeblock_t* scope)
        {
            PtrDict::destroyItemsAndDict(scope->scopestore);
            mc_memory_free(scope);
            scope = nullptr;
        }

        static mcastscopeblock_t* copy(mcastscopeblock_t* scope)
        {
            mcastscopeblock_t* copy;
            copy = Memory::make<mcastscopeblock_t>(scope->offset, scope->scopestore->copy());
            copy->numdefinitions = scope->numdefinitions;
            return copy;
        }

    public:
        PtrDict* scopestore;
        int offset;
        int numdefinitions;

    public:
        mcastscopeblock_t(int ofs): mcastscopeblock_t(ofs, nullptr)
        {
        }

        mcastscopeblock_t(int ofs, PtrDict* ss)
        {
            if(ss != nullptr)
            {
                this->scopestore = ss;
            }
            else
            {
                this->scopestore = Memory::make<PtrDict>((mcitemcopyfn_t)AstSymbol::copy, (mcitemdestroyfn_t)AstSymbol::destroy);
            }
            this->numdefinitions = 0;
            this->offset = ofs;
        }
};


class AstScopeComp
{
    public:
        static void destroy(AstScopeComp* scope)
        {
            PtrList::destroy(scope->ipstackcontinue, nullptr);
            PtrList::destroy(scope->ipstackbreak, nullptr);
            PtrList::destroy(scope->compiledscopebytecode, nullptr);
            PtrList::destroy(scope->scopesrcposlist, nullptr);
            mc_memory_free(scope);
        }

    public:
        AstScopeComp* outer;
        PtrList* compiledscopebytecode;
        PtrList* scopesrcposlist;
        PtrList* ipstackbreak;
        PtrList* ipstackcontinue;
        mcinternopcode_t lastopcode = 0;

    public:
        AstScopeComp(AstScopeComp* ou)
        {
            this->outer = ou;
            this->compiledscopebytecode = Memory::make<PtrList>(sizeof(uint16_t), false);
            this->scopesrcposlist = Memory::make<PtrList>(sizeof(AstLocation), false);
            this->ipstackbreak = Memory::make<PtrList>(sizeof(int), false);
            this->ipstackcontinue = Memory::make<PtrList>(sizeof(int), false);
        }

        CompiledProgram* orphanResult()
        {
            uint16_t* bcdata;
            AstLocation* astlocdata;
            CompiledProgram* res;
            bcdata = (uint16_t*)this->compiledscopebytecode->data();
            astlocdata = (AstLocation*)this->scopesrcposlist->data();
            res = Memory::make<CompiledProgram>(bcdata, astlocdata, this->compiledscopebytecode->count());
            PtrList::orphanData(this->compiledscopebytecode);
            PtrList::orphanData(this->scopesrcposlist);
            return res;
        }
};

class AstScopeFile
{
    public:
        static void destroy(AstScopeFile* scope)
        {
            size_t i;
            void* name;
            for(i = 0; i < scope->loadedmodnames->count(); i++)
            {
                name = (void*)scope->loadedmodnames->get(i);
                mc_memory_free(name);
            }
            PtrList::destroy(scope->loadedmodnames, nullptr);
            Memory::destroy(scope->parser);
            mc_memory_free(scope);
        }

    public:
        AstParser* parser;
        AstSymTable* filesymtab;
        AstSourceFile* m_ccfile;
        PtrList* loadedmodnames;

    public:
        AstScopeFile(mcconfig_t* cfg, ErrList* errlist, AstSourceFile* file)
        {
            this->parser = Memory::make<AstParser>(cfg, errlist);
            this->filesymtab = nullptr;
            m_ccfile = file;
            this->loadedmodnames = Memory::make<PtrList>(sizeof(void*), true);
        }
};

class SymStore
{
    public:
        static void destroy(SymStore* store)
        {
            if(store != nullptr)
            {
                PtrDict::destroyItemsAndDict(store->storedsymbols);
                Memory::destroy(store->storedobjects);
                mc_memory_free(store);
                store = nullptr;
            }
        }

    public:
        PtrDict* storedsymbols;
        GenericList<Value>* storedobjects;

    public:
        SymStore()
        {
            this->storedsymbols = Memory::make<PtrDict>((mcitemcopyfn_t)AstSymbol::copy, (mcitemdestroyfn_t)AstSymbol::destroy);
            this->storedobjects = Memory::make<GenericList<Value>>(0, Value::makeNull());
        }

        AstSymbol* getSymbol(const char* name)
        {
            return (AstSymbol*)this->storedsymbols->get(name);
        }

        bool setNamed(const char* name, Value object)
        {
            bool ok;
            int ix;
            AstSymbol* symbol;
            AstSymbol* existingsymbol;
            (void)ok;
            existingsymbol = this->getSymbol(name);
            if(existingsymbol)
            {
                ok = this->storedobjects->set(existingsymbol->index, object);
                return ok;
            }
            ix = this->storedobjects->count();
            ok = this->storedobjects->push(object);
            symbol = Memory::make<AstSymbol>(name, MC_SYM_GLOBALBUILTIN, ix, false);
            ok = this->storedsymbols->set(name, symbol);
            return true;
        }

        Value getAtIndex(int ix, bool* outok)
        {
            Value* res;
            res = (Value*)this->storedobjects->getp(ix);
            if(!res)
            {
                *outok = false;
                return Value::makeNull();
            }
            *outok = true;
            return *res;
        }

        Value* getData()
        {
            return (Value*)this->storedobjects->data();
        }

        int getCount()
        {
            return this->storedobjects->count();
        }

};


class AstSymTable
{
    public:
        static void destroy(AstSymTable* table)
        {
            if(table != nullptr)
            {
                while(table->blockscopes->count() > 0)
                {
                    table->scopeBlockPop();
                }
                PtrList::destroy(table->blockscopes, nullptr);
                PtrList::destroy(table->modglobalsymbols, (mcitemdestroyfn_t)AstSymbol::destroy);
                PtrList::destroy(table->freesymbols, (mcitemdestroyfn_t)AstSymbol::destroy);
                mc_memory_free(table);
                table = nullptr;
            }
        }

    public:
        AstSymTable* outer;
        SymStore* symglobalstore;
        PtrList* blockscopes;
        PtrList* freesymbols;
        PtrList* modglobalsymbols;
        int maxnumdefinitions;
        int modglobaloffset;

    public:
        AstSymTable(AstSymTable* syouter, SymStore* sygstore, PtrList* syblockscopes, PtrList* syfreesyms, PtrList* symodglobalsymbols, int mgo)
        {
            this->maxnumdefinitions = 0;
            this->outer = syouter;
            this->symglobalstore = sygstore;
            this->modglobaloffset = mgo;
            if(syblockscopes)
            {
                this->blockscopes = syblockscopes;
            }
            else
            {
                this->blockscopes = Memory::make<PtrList>(sizeof(void*), true);
            }
            if(syfreesyms)
            {
                this->freesymbols = syfreesyms;
            }
            else
            {
                this->freesymbols = Memory::make<PtrList>(sizeof(void*), true);
            }
            if(symodglobalsymbols)
            {
                this->modglobalsymbols = symodglobalsymbols;
            }
            else
            {
                this->modglobalsymbols = Memory::make<PtrList>(sizeof(void*), true);
            }
            MC_ASSERT(this->scopeBlockPush());
        }

        AstSymTable* copy()
        {
            AstSymTable* copy;
            auto cblocks = this->blockscopes->copy((mcitemcopyfn_t)mcastscopeblock_t::copy, (mcitemdestroyfn_t)mcastscopeblock_t::destroy);
            auto cfrees = this->freesymbols->copy((mcitemcopyfn_t)AstSymbol::copy, (mcitemdestroyfn_t)AstSymbol::destroy);
            auto cmods = this->modglobalsymbols->copy((mcitemcopyfn_t)AstSymbol::copy, (mcitemdestroyfn_t)AstSymbol::destroy);
            copy = Memory::make<AstSymTable>(this->outer, this->symglobalstore, cblocks, cfrees, cmods, this->modglobaloffset);
            copy->maxnumdefinitions = this->maxnumdefinitions;
            copy->modglobaloffset = this->modglobaloffset;
            return copy;
        }

        bool setSymbol(AstSymbol* symbol)
        {
            mcastscopeblock_t* topscope;
            AstSymbol* existing;
            topscope = (mcastscopeblock_t*)this->blockscopes->top();
            existing = (AstSymbol*)topscope->scopestore->get(symbol->name);
            if(existing)
            {
                AstSymbol::destroy(existing);
            }
            return topscope->scopestore->set(symbol->name, symbol);
        }

        int nextSymbolIndex()
        {
            int ix;
            mcastscopeblock_t* topscope;
            topscope = (mcastscopeblock_t*)this->blockscopes->top();
            ix = topscope->offset + topscope->numdefinitions;
            return ix;
        }

        int getNumDefinitions()
        {
            int i;
            int count;
            mcastscopeblock_t* scope;
            count = 0;
            for(i = this->blockscopes->count() - 1; i >= 0; i--)
            {
                scope = (mcastscopeblock_t*)this->blockscopes->get(i);
                count += scope->numdefinitions;
            }
            return count;
        }


        bool addModuleSymbol(AstSymbol* symbol)
        {
            bool ok;
            AstSymbol* copy;
            (void)ok;
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
            copy = AstSymbol::copy(symbol);
            if(!copy)
            {
                return false;
            }
            ok = this->setSymbol(copy);
            return true;
        }

        AstSymbol* defineSymbol(const char* name, bool assignable)
        {
            bool ok;
            bool globalsymboladded;
            int ix;
            int definitionscount;
            mcastsymtype_t symboltype;
            AstSymbol* symbol;
            mcastscopeblock_t* topscope;
            AstSymbol* globalsymbol;
            AstSymbol* globalsymbolcopy;
            (void)ok;
            (void)globalsymboladded;
            globalsymbol = this->symglobalstore->getSymbol(name);
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
            ix = this->nextSymbolIndex();
            symbol = Memory::make<AstSymbol>(name, symboltype, ix, assignable);
            globalsymboladded = false;
            ok = false;
            if(symboltype == MC_SYM_MODULEGLOBAL && this->blockscopes->count() == 1)
            {
                globalsymbolcopy = AstSymbol::copy(symbol);
                if(!globalsymbolcopy)
                {
                    AstSymbol::destroy(symbol);
                    return nullptr;
                }
                ok = this->modglobalsymbols->push(globalsymbolcopy);
                globalsymboladded = true;
            }
            ok = this->setSymbol(symbol);
            topscope = (mcastscopeblock_t*)this->blockscopes->top();
            topscope->numdefinitions++;
            definitionscount = this->getNumDefinitions();
            if(definitionscount > this->maxnumdefinitions)
            {
                this->maxnumdefinitions = definitionscount;
            }
            return symbol;
        }

        AstSymbol* defineAndDestroyOld(AstSymbol* original)
        {
            bool ok;
            AstSymbol* copy;
            AstSymbol* symbol;
            (void)ok;
            copy = Memory::make<AstSymbol>(original->name, original->symtype, original->index, original->assignable);
            ok = this->freesymbols->push(copy);
            symbol = Memory::make<AstSymbol>(original->name, MC_SYM_FREE, this->freesymbols->count() - 1, original->assignable);
            ok = this->setSymbol(symbol);
            return symbol;
        }

        AstSymbol* defineFunctionName(const char* name, bool assignable)
        {
            bool ok;
            AstSymbol* symbol;
            (void)ok;
            /* module symbol */
            if(strchr(name, ':'))
            {
                return nullptr;
            }
            symbol = Memory::make<AstSymbol>(name, MC_SYM_FUNCTION, 0, assignable);
            ok = this->setSymbol(symbol);
            return symbol;
        }

        AstSymbol* defineThis()
        {
            bool ok;
            AstSymbol* symbol;
            (void)ok;
            symbol = Memory::make<AstSymbol>("this", MC_SYM_THIS, 0, false);
            ok = this->setSymbol(symbol);
            return symbol;
        }

        AstSymbol* resolve(const char* name)
        {
            int i;
            AstSymbol* symbol;
            mcastscopeblock_t* scope;
            symbol = nullptr;
            scope = nullptr;
            symbol = this->symglobalstore->getSymbol(name);
            if(symbol)
            {
                return symbol;
            }

            for(i = this->blockscopes->count() - 1; i >= 0; i--)
            {
                scope = (mcastscopeblock_t*)this->blockscopes->get(i);
                symbol = (AstSymbol*)scope->scopestore->get(name);
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
            AstSymbol* symbol;
            mcastscopeblock_t* topscope;
            symbol = this->symglobalstore->getSymbol(name);
            if(symbol)
            {
                return true;
            }
            topscope = (mcastscopeblock_t*)this->blockscopes->top();
            symbol = (AstSymbol*)topscope->scopestore->get(name);
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
            (void)ok;
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
            newscope = Memory::make<mcastscopeblock_t>(blockscopeoffset);
            ok = this->blockscopes->push(newscope);
            return true;
        }

        void scopeBlockPop()
        {
            mcastscopeblock_t* topscope;
            topscope = (mcastscopeblock_t*)this->blockscopes->top();
            this->blockscopes->pop(nullptr);
            Memory::destroy(topscope);
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

        AstSymbol* getModuleGlobalSymAt(int ix)
        {
            return (AstSymbol*)this->modglobalsymbols->get(ix);
        }
};

class Token
{
    public:
        enum Type
        {
            TOK_INVALID = 0,
            TOK_EOF,
            /* Operators */
            TOK_ASSIGN,
            TOK_ASSIGNPLUS,
            TOK_ASSIGNMINUS,
            TOK_ASSIGNASTERISK,
            TOK_ASSIGNSLASH,
            TOK_ASSIGNPERCENT,
            TOK_ASSIGNBINAND,
            TOK_ASSIGNBINOR,
            TOK_ASSIGNBINXOR,
            TOK_ASSIGNLSHIFT,
            TOK_ASSIGNRSHIFT,
            TOK_QUESTION,
            TOK_PLUS,
            TOK_PLUSPLUS,
            TOK_UNARYMINUS,
            TOK_MINUSMINUS,
            TOK_UNARYBINNOT,
            TOK_BANG,
            TOK_ASTERISK,
            TOK_SLASH,
            TOK_LT,
            TOK_LTE,
            TOK_GT,
            TOK_GTE,
            TOK_EQ,
            TOK_NOTEQ,
            TOK_AND,
            TOK_OR,
            TOK_BINAND,
            TOK_BINOR,
            TOK_BINXOR,
            TOK_LSHIFT,
            TOK_RSHIFT,
            /* Delimiters */
            TOK_COMMA,
            TOK_SEMICOLON,
            TOK_COLON,
            TOK_LPAREN,
            TOK_RPAREN,
            TOK_LBRACE,
            TOK_RBRACE,
            TOK_LBRACKET,
            TOK_RBRACKET,
            TOK_DOT,
            TOK_PERCENT,
            /* Keywords */
            TOK_FUNCTION,
            TOK_CONST,
            TOK_VAR,
            TOK_TRUE,
            TOK_FALSE,
            TOK_IF,
            TOK_ELSE,
            TOK_RETURN,
            TOK_WHILE,
            TOK_BREAK,
            TOK_FOR,
            TOK_IN,
            TOK_CONTINUE,
            TOK_NULL,
            TOK_IMPORT,
            TOK_RECOVER,
            /* Identifiers and literals */
            TOK_IDENT,
            TOK_NUMBER,
            TOK_STRING,
            TOK_TEMPLATESTRING,
            /* MUST be last. */
            TOK_TYPEMAX
        };

    public:
        static const char* tokenName(Type type)
        {
            switch(type)
            {
                case Token::TOK_EOF:
                    return "EOF";
                case Token::TOK_ASSIGN:
                    return "=";
                case Token::TOK_ASSIGNPLUS:
                    return "+=";
                case Token::TOK_ASSIGNMINUS:
                    return "-=";
                case Token::TOK_ASSIGNASTERISK:
                    return "*=";
                case Token::TOK_ASSIGNSLASH:
                    return "/=";
                case Token::TOK_ASSIGNPERCENT:
                    return "%=";
                case Token::TOK_ASSIGNBINAND:
                    return "&=";
                case Token::TOK_ASSIGNBINOR:
                    return "|=";
                case Token::TOK_ASSIGNBINXOR:
                    return "^=";
                case Token::TOK_ASSIGNLSHIFT:
                    return "<<=";
                case Token::TOK_ASSIGNRSHIFT:
                    return ">>=";
                case Token::TOK_QUESTION:
                    return "?";
                case Token::TOK_PLUS:
                    return "+";
                case Token::TOK_PLUSPLUS:
                    return "++";
                case Token::TOK_UNARYMINUS:
                    return "-";
                case Token::TOK_MINUSMINUS:
                    return "--";
                case Token::TOK_BANG:
                    return "!";
                case Token::TOK_ASTERISK:
                    return "*";
                case Token::TOK_SLASH:
                    return "/";
                case Token::TOK_LT:
                    return "<";
                case Token::TOK_LTE:
                    return "<=";
                case Token::TOK_GT:
                    return ">";
                case Token::TOK_GTE:
                    return ">=";
                case Token::TOK_EQ:
                    return "==";
                case Token::TOK_NOTEQ:
                    return "!=";
                case Token::TOK_AND:
                    return "&&";
                case Token::TOK_OR:
                    return "||";
                case Token::TOK_BINAND:
                    return "&";
                case Token::TOK_BINOR:
                    return "|";
                case Token::TOK_BINXOR:
                    return "^";
                case Token::TOK_LSHIFT:
                    return "<<";
                case Token::TOK_RSHIFT:
                    return ">>";
                case Token::TOK_COMMA:
                    return ",";
                case Token::TOK_SEMICOLON:
                    return ";";
                case Token::TOK_COLON:
                    return ":";
                case Token::TOK_LPAREN:
                    return "(";
                case Token::TOK_RPAREN:
                    return ")";
                case Token::TOK_LBRACE:
                    return "{";
                case Token::TOK_RBRACE:
                    return "}";
                case Token::TOK_LBRACKET:
                    return "[";
                case Token::TOK_RBRACKET:
                    return "]";
                case Token::TOK_DOT:
                    return ".";
                case Token::TOK_PERCENT:
                    return "%";
                case Token::TOK_FUNCTION:
                    return "FUNCTION";
                case Token::TOK_CONST:
                    return "CONST";
                case Token::TOK_VAR:
                    return "VAR";
                case Token::TOK_TRUE:
                    return "TRUE";
                case Token::TOK_FALSE:
                    return "FALSE";
                case Token::TOK_IF:
                    return "IF";
                case Token::TOK_ELSE:
                    return "ELSE";
                case Token::TOK_RETURN:
                    return "RETURN";
                case Token::TOK_WHILE:
                    return "WHILE";
                case Token::TOK_BREAK:
                    return "BREAK";
                case Token::TOK_FOR:
                    return "FOR";
                case Token::TOK_IN:
                    return "IN";
                case Token::TOK_CONTINUE:
                    return "CONTINUE";
                case Token::TOK_NULL:
                    return "nullptr";
                case Token::TOK_IMPORT:
                    return "IMPORT";
                case Token::TOK_RECOVER:
                    return "RECOVER";
                case Token::TOK_IDENT:
                    return "IDENT";
                case Token::TOK_NUMBER:
                    return "NUMBER";
                case Token::TOK_STRING:
                    return "STRING";
                case Token::TOK_TEMPLATESTRING:
                    return "TEMPLATE_STRING";
                default:
                    break;
            }
            return "ILLEGAL";
        }
    public:
        Type toktype;
        const char* tokstrdata;
        int tokstrlen;
        AstLocation m_tokpos;

    public:
        Token(): Token(Token::TOK_EOF, "", 0)
        {
        }

        Token(Type type, const char* literal, int len)
        {
            this->toktype = type;
            this->tokstrdata = literal;
            this->tokstrlen = len;
        }

        char* dupLiteralString()
        {
            return mc_util_strndup(this->tokstrdata, this->tokstrlen);
        }
};

class AstExpression
{
    public:
        enum mcastexprtype_t
        {
            EXPR_NONE,
            EXPR_IDENT,
            EXPR_NUMBERLITERAL,
            EXPR_BOOLLITERAL,
            EXPR_STRINGLITERAL,
            EXPR_NULLLITERAL,
            EXPR_ARRAYLITERAL,
            EXPR_MAPLITERAL,
            EXPR_PREFIX,
            EXPR_INFIX,
            EXPR_FUNCTIONLITERAL,
            EXPR_CALL,
            EXPR_INDEX,
            EXPR_ASSIGN,
            EXPR_LOGICAL,
            EXPR_TERNARY,
            EXPR_STMTDEFINE,
            EXPR_STMTIF,
            EXPR_STMTRETURN,
            EXPR_STMTEXPRESSION,
            EXPR_STMTLOOPWHILE,
            EXPR_STMTBREAK,
            EXPR_STMTCONTINUE,
            EXPR_STMTLOOPFOREACH,
            EXPR_STMTLOOPFORCLASSIC,
            EXPR_STMTBLOCK,
            EXPR_STMTIMPORT,
            EXPR_STMTRECOVER
        };

        class ExprCodeBlock
        {
            public:
                PtrList* statements;

            public:
                ExprCodeBlock(PtrList* stmts)
                {
                    this->statements = stmts;
                }

                static void destroy(ExprCodeBlock* block)
                {
                    if(block != nullptr)
                    {
                        PtrList::destroy(block->statements, (mcitemdestroyfn_t)destroyExpression);
                        mc_memory_free(block);
                    }
                }

                static ExprCodeBlock* copy(ExprCodeBlock* block)
                {
                    if(!block)
                    {
                        return nullptr;
                    }
                    auto statementscopy = block->statements->copy((mcitemcopyfn_t)copyExpression, (mcitemdestroyfn_t)destroyExpression);
                    if(!statementscopy)
                    {
                        return nullptr;
                    }
                    auto res = Memory::make<ExprCodeBlock>(statementscopy);
                    return res;
                }
        };

        class ExprIfCase
        {
            public:
                AstExpression* ifcond;
                ExprCodeBlock* consequence;

            public:
                ExprIfCase(AstExpression* test, ExprCodeBlock* consq)
                {
                    this->ifcond = test;
                    this->consequence = consq;
                }

                static ExprIfCase* copy(ExprIfCase* ifcase)
                {
                    AstExpression* testcopy;
                    ExprCodeBlock* consequencecopy;
                    ExprIfCase* ifcasecopy;
                    testcopy = nullptr;
                    consequencecopy = nullptr;
                    ifcasecopy = nullptr;
                    if(!ifcase)
                    {
                        return nullptr;
                    }
                    testcopy = copyExpression(ifcase->ifcond);
                    if(!testcopy)
                    {
                        goto err;
                    }
                    consequencecopy = ExprCodeBlock::copy(ifcase->consequence);
                    if(!testcopy || !consequencecopy)
                    {
                        goto err;
                    }
                    ifcasecopy = Memory::make<ExprIfCase>(testcopy, consequencecopy);
                    return ifcasecopy;
                err:
                    destroyExpression(testcopy);
                    Memory::destroy(consequencecopy);
                    ExprIfCase::destroy(ifcasecopy);
                    return nullptr;
                }

                static void destroy(ExprIfCase* cond)
                {
                    if(cond != nullptr)
                    {
                        destroyExpression(cond->ifcond);
                        Memory::destroy(cond->consequence);
                        mc_memory_free(cond);
                    }
                }
        };

        struct ExprIfStmt
        {
            PtrList* cases;
            ExprCodeBlock* alternative;
        };

        struct ExprLiteralMap
        {
            PtrList* litmapkeys;
            PtrList* litmapvalues;
        };

        struct ExprLiteralArray
        {
            PtrList* litarritems;
        };

        struct ExprLiteralString
        {
            size_t length;
            char* data;
        };

        struct ExprPrefix
        {
            mcastmathoptype_t op;
            AstExpression* right;
        };

        struct ExprInfix
        {
            mcastmathoptype_t op;
            AstExpression* left;
            AstExpression* right;
        };

        struct ExprLiteralFunction
        {
            char* name;
            PtrList* funcparamlist;
            ExprCodeBlock* body;
        };

        struct ExprCall
        {
            AstExpression* function;
            PtrList* args;
        };

        struct ExprIndex
        {
            bool isdot;
            AstExpression* left;
            AstExpression* index;
        };

        struct ExprAssign
        {
            AstExpression* dest;
            AstExpression* source;
            bool is_postfix;
        };

        struct ExprLogical
        {
            mcastmathoptype_t op;
            AstExpression* left;
            AstExpression* right;
        };

        struct ExprTernary
        {
            AstExpression* tercond;
            AstExpression* teriftrue;
            AstExpression* teriffalse;
        };

        class ExprIdent
        {
            public:
                char* value = nullptr;
                AstLocation pos = AstLocation::Invalid();

            public:
                ExprIdent()
                {
                }

                ExprIdent(Token tok)
                {
                    this->value = tok.dupLiteralString();
                    this->pos = tok.m_tokpos;
                }

                static ExprIdent* copy(ExprIdent* ident)
                {
                    ExprIdent* res = Memory::make<ExprIdent>();
                    res->value = mc_util_strdup(ident->value);
                    res->pos = ident->pos;
                    return res;
                }

                static void destroy(ExprIdent* ident)
                {
                    if(ident != nullptr)
                    {
                        mc_memory_free(ident->value);
                        ident->value = nullptr;
                        ident->pos = AstLocation::Invalid();
                        mc_memory_free(ident);
                    }
                }
        };

        class ExprFuncParam
        {
            public:
                ExprIdent* ident;

            public:
                ExprFuncParam(ExprIdent* idv)
                {
                    this->ident = idv;
                    MC_ASSERT(this->ident->value);
                }

                static ExprFuncParam* copy(ExprFuncParam* param)
                {
                    ExprFuncParam* res;
                    res = Memory::make<ExprFuncParam>(ExprIdent::copy(param->ident));
                    if(!res->ident->value)
                    {
                        mc_memory_free(res);
                        return nullptr;
                    }
                    return res;
                }

                static void destroy(ExprFuncParam* param)
                {
                    if(param != nullptr)
                    {
                        Memory::destroy(param->ident);
                        mc_memory_free(param);
                    }
                }
        };

        struct ExprDefine
        {
            ExprIdent* name;
            AstExpression* value;
            bool assignable;
        };

        struct ExprWhileStmt
        {
            AstExpression* loopcond;
            ExprCodeBlock* body;
        };

        struct ExprForeachStmt
        {
            ExprIdent* iterator;
            AstExpression* source;
            ExprCodeBlock* body;
        };

        struct ExprLoopStmt
        {
            AstExpression* init;
            AstExpression* loopcond;
            AstExpression* update;
            ExprCodeBlock* body;
        };

        struct ExprImportStmt
        {
            char* path;
        };

        struct ExprRecover
        {
            ExprIdent* errident;
            ExprCodeBlock* body;
        };


    public:
        static AstExpression* makeExpression(mcastexprtype_t type)
        {
            AstExpression* res = Memory::make<AstExpression>();
            res->exprtype = type;
            res->pos = AstLocation::Invalid();
            return res;
        }

        static AstExpression* makeRecover(ExprIdent* eid, ExprCodeBlock* body)
        {
            auto res = makeExpression(EXPR_STMTRECOVER);
            res->uexpr.exprrecoverstmt.errident = eid;
            res->uexpr.exprrecoverstmt.body = body;
            return res;
        }

        static AstExpression* makeliteralnumber(mcfloat_t val)
        {
            auto res = makeExpression(EXPR_NUMBERLITERAL);
            res->uexpr.exprlitnumber = val;
            return res;
        }

        static AstExpression* makeliteralbool(bool val)
        {
            auto res = makeExpression(EXPR_BOOLLITERAL);
            res->uexpr.exprlitbool = val;
            return res;
        }

        static AstExpression* makeliteralstring(char* value, size_t len)
        {
            auto res = makeExpression(EXPR_STRINGLITERAL);
            res->uexpr.exprlitstring.data = value;
            res->uexpr.exprlitstring.length = len;
            return res;
        }

        static AstExpression* makeliteralnull()
        {
            auto res = makeExpression(EXPR_NULLLITERAL);
            return res;
        }

        static AstExpression* makeident(ExprIdent* ident)
        {
            auto res = makeExpression(EXPR_IDENT);
            res->uexpr.exprident = ident;
            return res;
        }


        static AstExpression* makeliteralarray(PtrList* values)
        {
            auto res = makeExpression(EXPR_ARRAYLITERAL);
            res->uexpr.exprlitarray.litarritems = values;
            return res;
        }

        static AstExpression* makeliteralmap(PtrList* keys, PtrList* values)
        {
            auto res = makeExpression(EXPR_MAPLITERAL);
            res->uexpr.exprlitmap.litmapkeys = keys;
            res->uexpr.exprlitmap.litmapvalues = values;
            return res;
        }

        static AstExpression* makeprefix(mcastmathoptype_t op, AstExpression* right)
        {
            auto res = makeExpression(EXPR_PREFIX);
            res->uexpr.exprprefix.op = op;
            res->uexpr.exprprefix.right = right;
            return res;
        }

        static AstExpression* makeinfix(mcastmathoptype_t op, AstExpression* left, AstExpression* right)
        {
            auto res = makeExpression(EXPR_INFIX);
            res->uexpr.exprinfix.op = op;
            res->uexpr.exprinfix.left = left;
            res->uexpr.exprinfix.right = right;
            return res;
        }

        static AstExpression* makeliteralfunction(PtrList* params, ExprCodeBlock* body)
        {
            auto res = makeExpression(EXPR_FUNCTIONLITERAL);
            res->uexpr.exprlitfunction.name = nullptr;
            res->uexpr.exprlitfunction.funcparamlist = params;
            res->uexpr.exprlitfunction.body = body;
            return res;
        }

        static AstExpression* makecall(AstExpression* function, PtrList* args)
        {
            auto res = makeExpression(EXPR_CALL);
            res->uexpr.exprcall.function = function;
            res->uexpr.exprcall.args = args;
            return res;
        }

        static AstExpression* makeindex(AstExpression* left, AstExpression* index, bool isdot)
        {
            auto res = makeExpression(EXPR_INDEX);
            res->uexpr.exprindex.isdot = isdot;
            res->uexpr.exprindex.left = left;
            res->uexpr.exprindex.index = index;
            return res;
        }

        static AstExpression* makeassign(AstExpression* dest, AstExpression* source, bool is_postfix)
        {
            auto res = makeExpression(EXPR_ASSIGN);
            res->uexpr.exprassign.dest = dest;
            res->uexpr.exprassign.source = source;
            res->uexpr.exprassign.is_postfix = is_postfix;
            return res;
        }

        static AstExpression* makelogical(mcastmathoptype_t op, AstExpression* left, AstExpression* right)
        {
            auto res = makeExpression(EXPR_LOGICAL);
            res->uexpr.exprlogical.op = op;
            res->uexpr.exprlogical.left = left;
            res->uexpr.exprlogical.right = right;
            return res;
        }

        static AstExpression* maketernary(AstExpression* test, AstExpression* ift, AstExpression* iffalse)
        {
            auto res = makeExpression(EXPR_TERNARY);
            res->uexpr.exprternary.tercond = test;
            res->uexpr.exprternary.teriftrue = ift;
            res->uexpr.exprternary.teriffalse = iffalse;
            return res;
        }

        static AstExpression* makeinlinefunccall(AstExpression* expr, const char* fname)
        {
            auto fntoken = Token(Token::TOK_IDENT, fname, mc_util_strlen(fname));
            fntoken.m_tokpos = expr->pos;
            auto ident = Memory::make<ExprIdent>(fntoken);
            ident->pos = fntoken.m_tokpos;
            auto functionidentexpr = makeident(ident);
            functionidentexpr->pos = expr->pos;
            ident = nullptr;
            auto args = Memory::make<PtrList>(sizeof(void*), true);
            args->push(expr);
            auto ce = makecall(functionidentexpr, args);
            ce->pos = expr->pos;
            return ce;
        }

        static AstExpression* makedefine(ExprIdent* name, AstExpression* value, bool assignable)
        {
            auto res = makeExpression(EXPR_STMTDEFINE);
            res->uexpr.exprdefine.name = name;
            res->uexpr.exprdefine.value = value;
            res->uexpr.exprdefine.assignable = assignable;
            return res;
        }

        static AstExpression* makeif(PtrList* cases, ExprCodeBlock* alternative)
        {
            auto res = makeExpression(EXPR_STMTIF);
            res->uexpr.exprifstmt.cases = cases;
            res->uexpr.exprifstmt.alternative = alternative;
            return res;
        }

        static AstExpression* makereturn(AstExpression* value)
        {
            auto res = makeExpression(EXPR_STMTRETURN);
            res->uexpr.exprreturnvalue = value;
            return res;
        }

        static AstExpression* makeexprstmt(AstExpression* value)
        {
            auto res = makeExpression(EXPR_STMTEXPRESSION);
            res->uexpr.exprexpression = value;
            return res;
        }

        static AstExpression* makewhile(AstExpression* test, ExprCodeBlock* body)
        {
            auto res = makeExpression(EXPR_STMTLOOPWHILE);
            res->uexpr.exprwhileloopstmt.loopcond = test;
            res->uexpr.exprwhileloopstmt.body = body;
            return res;
        }

        static AstExpression* makebreak()
        {
            auto res = makeExpression(EXPR_STMTBREAK);
            return res;
        }

        static AstExpression* makeforeach(ExprIdent* iterator, AstExpression* source, ExprCodeBlock* body)
        {
            auto res = makeExpression(EXPR_STMTLOOPFOREACH);
            res->uexpr.exprforeachloopstmt.iterator = iterator;
            res->uexpr.exprforeachloopstmt.source = source;
            res->uexpr.exprforeachloopstmt.body = body;
            return res;
        }

        static AstExpression* makeforloop(AstExpression* init, AstExpression* test, AstExpression* update, ExprCodeBlock* body)
        {
            auto res = makeExpression(EXPR_STMTLOOPFORCLASSIC);
            res->uexpr.exprforloopstmt.init = init;
            res->uexpr.exprforloopstmt.loopcond = test;
            res->uexpr.exprforloopstmt.update = update;
            res->uexpr.exprforloopstmt.body = body;
            return res;
        }

        static AstExpression* makecontinue()
        {
            auto res = makeExpression(EXPR_STMTCONTINUE);
            return res;
        }

        static AstExpression* makeblock(ExprCodeBlock* block)
        {
            auto res = makeExpression(EXPR_STMTBLOCK);
            res->uexpr.exprblockstmt = block;
            return res;
        }

        static AstExpression* makeimport(char* strpath)
        {
            auto res = makeExpression(EXPR_STMTIMPORT);
            res->uexpr.exprimportstmt.path = strpath;
            return res;
        }


        static AstExpression* copyExpression(AstExpression* expr)
        {
            AstExpression* res;
            if(!expr)
            {
                return nullptr;
            }
            res = nullptr;
            switch(expr->exprtype)
            {
                case EXPR_NONE:
                    {
                        MC_ASSERT(false);
                    }
                    break;
                case EXPR_IDENT:
                    {
                        ExprIdent* ident;
                        ident = ExprIdent::copy(expr->uexpr.exprident);
                        if(!ident)
                        {
                            return nullptr;
                        }
                        res = makeident(ident);
                        if(!res)
                        {
                            Memory::destroy(ident);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_NUMBERLITERAL:
                    {
                        res = makeliteralnumber(expr->uexpr.exprlitnumber);
                    }
                    break;
                case EXPR_BOOLLITERAL:
                    {
                        res = makeliteralbool(expr->uexpr.exprlitbool);
                    }
                    break;
                case EXPR_STRINGLITERAL:
                    {
                        char* stringcopy;
                        stringcopy = mc_util_strndup(expr->uexpr.exprlitstring.data, expr->uexpr.exprlitstring.length);
                        if(!stringcopy)
                        {
                            return nullptr;
                        }
                        res = makeliteralstring(stringcopy, expr->uexpr.exprlitstring.length);
                        if(!res)
                        {
                            mc_memory_free(stringcopy);
                            return nullptr;
                        }
                    }
                    break;

                case EXPR_NULLLITERAL:
                    {
                        res = makeliteralnull();
                    }
                    break;
                case EXPR_ARRAYLITERAL:
                    {
                        PtrList* valuescopy;
                        valuescopy = expr->uexpr.exprlitarray.litarritems->copy((mcitemcopyfn_t)copyExpression, (mcitemdestroyfn_t)destroyExpression);
                        if(!valuescopy)
                        {
                            return nullptr;
                        }
                        res = makeliteralarray(valuescopy);
                        if(!res)
                        {
                            PtrList::destroy(valuescopy, (mcitemdestroyfn_t)destroyExpression);
                            return nullptr;
                        }
                    }
                    break;

                case EXPR_MAPLITERAL:
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
                        res = makeliteralmap(keyscopy, valuescopy);
                        if(!res)
                        {
                            PtrList::destroy(keyscopy, (mcitemdestroyfn_t)destroyExpression);
                            PtrList::destroy(valuescopy, (mcitemdestroyfn_t)destroyExpression);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_PREFIX:
                    {
                        AstExpression* rightcopy;
                        rightcopy = copyExpression(expr->uexpr.exprprefix.right);
                        if(!rightcopy)
                        {
                            return nullptr;
                        }
                        res = makeprefix(expr->uexpr.exprprefix.op, rightcopy);
                        if(!res)
                        {
                            destroyExpression(rightcopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_INFIX:
                    {
                        AstExpression* leftcopy;
                        AstExpression* rightcopy;
                        leftcopy = copyExpression(expr->uexpr.exprinfix.left);
                        rightcopy = copyExpression(expr->uexpr.exprinfix.right);
                        if(!leftcopy || !rightcopy)
                        {
                            destroyExpression(leftcopy);
                            destroyExpression(rightcopy);
                            return nullptr;
                        }
                        res = makeinfix(expr->uexpr.exprinfix.op, leftcopy, rightcopy);
                        if(!res)
                        {
                            destroyExpression(leftcopy);
                            destroyExpression(rightcopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_FUNCTIONLITERAL:
                    {
                        char* namecopy;
                        PtrList* pacopy;
                        ExprCodeBlock* bodycopy;
                        pacopy = expr->uexpr.exprlitfunction.funcparamlist->copy((mcitemcopyfn_t)ExprFuncParam::copy, (mcitemdestroyfn_t)ExprFuncParam::destroy);
                        bodycopy = ExprCodeBlock::copy(expr->uexpr.exprlitfunction.body);
                        namecopy = mc_util_strdup(expr->uexpr.exprlitfunction.name);
                        if(!pacopy || !bodycopy)
                        {
                            PtrList::destroy(pacopy, (mcitemdestroyfn_t)ExprFuncParam::destroy);
                            Memory::destroy(bodycopy);
                            mc_memory_free(namecopy);
                            return nullptr;
                        }
                        res = makeliteralfunction(pacopy, bodycopy);
                        if(!res)
                        {
                            PtrList::destroy(pacopy, (mcitemdestroyfn_t)ExprFuncParam::destroy);
                            Memory::destroy(bodycopy);
                            mc_memory_free(namecopy);
                            return nullptr;
                        }
                        res->uexpr.exprlitfunction.name = namecopy;
                    }
                    break;
                case EXPR_CALL:
                    {
                        AstExpression* fcopy;
                        PtrList* argscopy;
                        fcopy = copyExpression(expr->uexpr.exprcall.function);
                        argscopy = expr->uexpr.exprcall.args->copy((mcitemcopyfn_t)copyExpression, (mcitemdestroyfn_t)destroyExpression);
                        if(!fcopy || !argscopy)
                        {
                            destroyExpression(fcopy);
                            PtrList::destroy(expr->uexpr.exprcall.args, (mcitemdestroyfn_t)destroyExpression);
                            return nullptr;
                        }
                        res = makecall(fcopy, argscopy);
                        if(!res)
                        {
                            destroyExpression(fcopy);
                            PtrList::destroy(expr->uexpr.exprcall.args, (mcitemdestroyfn_t)destroyExpression);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_INDEX:
                    {
                        AstExpression* leftcopy;
                        AstExpression* indexcopy;
                        leftcopy = copyExpression(expr->uexpr.exprindex.left);
                        indexcopy = copyExpression(expr->uexpr.exprindex.index);
                        if(!leftcopy || !indexcopy)
                        {
                            destroyExpression(leftcopy);
                            destroyExpression(indexcopy);
                            return nullptr;
                        }
                        res = makeindex(leftcopy, indexcopy, false);
                        if(!res)
                        {
                            destroyExpression(leftcopy);
                            destroyExpression(indexcopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_ASSIGN:
                    {
                        AstExpression* destcopy;
                        AstExpression* sourcecopy;
                        destcopy = copyExpression(expr->uexpr.exprassign.dest);
                        sourcecopy = copyExpression(expr->uexpr.exprassign.source);
                        if(!destcopy || !sourcecopy)
                        {
                            destroyExpression(destcopy);
                            destroyExpression(sourcecopy);
                            return nullptr;
                        }
                        res = makeassign(destcopy, sourcecopy, expr->uexpr.exprassign.is_postfix);
                        if(!res)
                        {
                            destroyExpression(destcopy);
                            destroyExpression(sourcecopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_LOGICAL:
                    {
                        AstExpression* leftcopy;
                        AstExpression* rightcopy;
                        leftcopy = copyExpression(expr->uexpr.exprlogical.left);
                        rightcopy = copyExpression(expr->uexpr.exprlogical.right);
                        if(!leftcopy || !rightcopy)
                        {
                            destroyExpression(leftcopy);
                            destroyExpression(rightcopy);
                            return nullptr;
                        }
                        res = makelogical(expr->uexpr.exprlogical.op, leftcopy, rightcopy);
                        if(!res)
                        {
                            destroyExpression(leftcopy);
                            destroyExpression(rightcopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_TERNARY:
                    {
                        AstExpression* testcopy;
                        AstExpression* iftruecopy;
                        AstExpression* iffalsecopy;
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
                        res = maketernary(testcopy, iftruecopy, iffalsecopy);
                        if(!res)
                        {
                            destroyExpression(testcopy);
                            destroyExpression(iftruecopy);
                            destroyExpression(iffalsecopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_STMTDEFINE:
                    {
                        AstExpression* valuecopy;
                        valuecopy = copyExpression(expr->uexpr.exprdefine.value);
                        if(!valuecopy)
                        {
                            return nullptr;
                        }
                        res = makedefine(ExprIdent::copy(expr->uexpr.exprdefine.name), valuecopy, expr->uexpr.exprdefine.assignable);
                        if(!res)
                        {
                            destroyExpression(valuecopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_STMTIF:
                    {
                        PtrList* casescopy;
                        ExprCodeBlock* alternativecopy;
                        casescopy = expr->uexpr.exprifstmt.cases->copy((mcitemcopyfn_t)ExprIfCase::copy, (mcitemdestroyfn_t)ExprIfCase::destroy);
                        alternativecopy = ExprCodeBlock::copy(expr->uexpr.exprifstmt.alternative);
                        if(!casescopy || !alternativecopy)
                        {
                            PtrList::destroy(casescopy, (mcitemdestroyfn_t)ExprIfCase::destroy);
                            Memory::destroy(alternativecopy);
                            return nullptr;
                        }
                        res = makeif(casescopy, alternativecopy);
                        if(res)
                        {
                            PtrList::destroy(casescopy, (mcitemdestroyfn_t)ExprIfCase::destroy);
                            Memory::destroy(alternativecopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_STMTRETURN:
                    {
                        AstExpression* valuecopy;
                        valuecopy = copyExpression(expr->uexpr.exprreturnvalue);
                        if(!valuecopy)
                        {
                            return nullptr;
                        }
                        res = makereturn(valuecopy);
                        if(!res)
                        {
                            destroyExpression(valuecopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_STMTEXPRESSION:
                    {
                        AstExpression* valuecopy;
                        valuecopy = copyExpression(expr->uexpr.exprexpression);
                        if(!valuecopy)
                        {
                            return nullptr;
                        }
                        res = makeexprstmt(valuecopy);
                        if(!res)
                        {
                            destroyExpression(valuecopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_STMTLOOPWHILE:
                    {
                        AstExpression* testcopy;
                        ExprCodeBlock* bodycopy;
                        testcopy = copyExpression(expr->uexpr.exprwhileloopstmt.loopcond);
                        bodycopy = ExprCodeBlock::copy(expr->uexpr.exprwhileloopstmt.body);
                        if(!testcopy || !bodycopy)
                        {
                            destroyExpression(testcopy);
                            Memory::destroy(bodycopy);
                            return nullptr;
                        }
                        res = makewhile(testcopy, bodycopy);
                        if(!res)
                        {
                            destroyExpression(testcopy);
                            Memory::destroy(bodycopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_STMTBREAK:
                    {
                        res = makebreak();
                    }
                    break;
                case EXPR_STMTCONTINUE:
                    {
                        res = makecontinue();
                    }
                    break;
                case EXPR_STMTLOOPFOREACH:
                    {
                        AstExpression* sourcecopy;
                        ExprCodeBlock* bodycopy;
                        sourcecopy = copyExpression(expr->uexpr.exprforeachloopstmt.source);
                        bodycopy = ExprCodeBlock::copy(expr->uexpr.exprforeachloopstmt.body);
                        if(!sourcecopy || !bodycopy)
                        {
                            destroyExpression(sourcecopy);
                            Memory::destroy(bodycopy);
                            return nullptr;
                        }
                        res = makeforeach(ExprIdent::copy(expr->uexpr.exprforeachloopstmt.iterator), sourcecopy, bodycopy);
                        if(!res)
                        {
                            destroyExpression(sourcecopy);
                            Memory::destroy(bodycopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_STMTLOOPFORCLASSIC:
                    {
                        AstExpression* initcopy;
                        AstExpression* testcopy;
                        AstExpression* updatecopy;
                        ExprCodeBlock* bodycopy;
                        initcopy= copyExpression(expr->uexpr.exprforloopstmt.init);
                        testcopy = copyExpression(expr->uexpr.exprforloopstmt.loopcond);
                        updatecopy = copyExpression(expr->uexpr.exprforloopstmt.update);
                        bodycopy = ExprCodeBlock::copy(expr->uexpr.exprforloopstmt.body);
                        if(!initcopy || !testcopy || !updatecopy || !bodycopy)
                        {
                            destroyExpression(initcopy);
                            destroyExpression(testcopy);
                            destroyExpression(updatecopy);
                            Memory::destroy(bodycopy);
                            return nullptr;
                        }
                        res = makeforloop(initcopy, testcopy, updatecopy, bodycopy);
                        if(!res)
                        {
                            destroyExpression(initcopy);
                            destroyExpression(testcopy);
                            destroyExpression(updatecopy);
                            Memory::destroy(bodycopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_STMTBLOCK:
                    {
                        ExprCodeBlock* blockcopy;
                        blockcopy = ExprCodeBlock::copy(expr->uexpr.exprblockstmt);
                        if(!blockcopy)
                        {
                            return nullptr;
                        }
                        res = makeblock(blockcopy);
                        if(!res)
                        {
                            Memory::destroy(blockcopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_STMTIMPORT:
                    {
                        char* pathcopy;
                        pathcopy = mc_util_strdup(expr->uexpr.exprimportstmt.path);
                        if(!pathcopy)
                        {
                            return nullptr;
                        }
                        res = makeimport(pathcopy);
                        if(!res)
                        {
                            mc_memory_free(pathcopy);
                            return nullptr;
                        }
                    }
                    break;
                case EXPR_STMTRECOVER:
                    {
                        ExprCodeBlock* bodycopy;
                        ExprIdent* erroridentcopy;
                        bodycopy = ExprCodeBlock::copy(expr->uexpr.exprrecoverstmt.body);
                        erroridentcopy = ExprIdent::copy(expr->uexpr.exprrecoverstmt.errident);
                        if(!bodycopy || !erroridentcopy)
                        {
                            Memory::destroy(bodycopy);
                            Memory::destroy(erroridentcopy);
                            return nullptr;
                        }
                        res = makeRecover(erroridentcopy, bodycopy);
                        if(!res)
                        {
                            Memory::destroy(bodycopy);
                            Memory::destroy(erroridentcopy);
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


        static void destroyExpression(AstExpression* expr)
        {
            if(expr != nullptr)
            {
                switch(expr->exprtype)
                {
                    case EXPR_NONE:
                        {
                            MC_ASSERT(false);
                        }
                        break;
                    case EXPR_IDENT:
                        {
                            Memory::destroy(expr->uexpr.exprident);
                        }
                        break;
                    case EXPR_NUMBERLITERAL:
                    case EXPR_BOOLLITERAL:
                        {
                        }
                        break;
                    case EXPR_STRINGLITERAL:
                        {
                            mc_memory_free(expr->uexpr.exprlitstring.data);
                        }
                        break;
                    case EXPR_NULLLITERAL:
                        {
                        }
                        break;
                    case EXPR_ARRAYLITERAL:
                        {
                            PtrList::destroy(expr->uexpr.exprlitarray.litarritems, (mcitemdestroyfn_t)destroyExpression);
                        }
                        break;
                    case EXPR_MAPLITERAL:
                        {
                            PtrList::destroy(expr->uexpr.exprlitmap.litmapkeys, (mcitemdestroyfn_t)destroyExpression);
                            PtrList::destroy(expr->uexpr.exprlitmap.litmapvalues, (mcitemdestroyfn_t)destroyExpression);
                        }
                        break;
                    case EXPR_PREFIX:
                        {
                            destroyExpression(expr->uexpr.exprprefix.right);
                        }
                        break;
                    case EXPR_INFIX:
                        {
                            destroyExpression(expr->uexpr.exprinfix.left);
                            destroyExpression(expr->uexpr.exprinfix.right);
                        }
                        break;
                    case EXPR_FUNCTIONLITERAL:
                        {
                            ExprLiteralFunction* fn;
                            fn = &expr->uexpr.exprlitfunction;
                            mc_memory_free(fn->name);
                            PtrList::destroy(fn->funcparamlist, (mcitemdestroyfn_t)ExprFuncParam::destroy);
                            Memory::destroy(fn->body);
                        }
                        break;
                    case EXPR_CALL:
                        {
                            PtrList::destroy(expr->uexpr.exprcall.args, (mcitemdestroyfn_t)destroyExpression);
                            destroyExpression(expr->uexpr.exprcall.function);
                        }
                        break;
                    case EXPR_INDEX:
                        {
                            destroyExpression(expr->uexpr.exprindex.left);
                            destroyExpression(expr->uexpr.exprindex.index);
                        }
                        break;
                    case EXPR_ASSIGN:
                        {
                            destroyExpression(expr->uexpr.exprassign.dest);
                            destroyExpression(expr->uexpr.exprassign.source);
                        }
                        break;
                    case EXPR_LOGICAL:
                        {
                            destroyExpression(expr->uexpr.exprlogical.left);
                            destroyExpression(expr->uexpr.exprlogical.right);
                        }
                        break;
                    case EXPR_TERNARY:
                        {
                            destroyExpression(expr->uexpr.exprternary.tercond);
                            destroyExpression(expr->uexpr.exprternary.teriftrue);
                            destroyExpression(expr->uexpr.exprternary.teriffalse);
                        }
                        break;
                    case EXPR_STMTDEFINE:
                        {
                            Memory::destroy(expr->uexpr.exprdefine.name);
                            destroyExpression(expr->uexpr.exprdefine.value);
                        }
                        break;
                    case EXPR_STMTIF:
                        {
                            PtrList::destroy(expr->uexpr.exprifstmt.cases, (mcitemdestroyfn_t)ExprIfCase::destroy);
                            Memory::destroy(expr->uexpr.exprifstmt.alternative);
                        }
                        break;
                    case EXPR_STMTRETURN:
                        {
                            destroyExpression(expr->uexpr.exprreturnvalue);
                        }
                        break;
                    case EXPR_STMTEXPRESSION:
                        {
                            destroyExpression(expr->uexpr.exprexpression);
                        }
                        break;
                    case EXPR_STMTLOOPWHILE:
                        {
                            destroyExpression(expr->uexpr.exprwhileloopstmt.loopcond);
                            Memory::destroy(expr->uexpr.exprwhileloopstmt.body);
                        }
                        break;
                    case EXPR_STMTBREAK:
                        {
                        }
                        break;
                    case EXPR_STMTCONTINUE:
                        {
                        }
                        break;
                    case EXPR_STMTLOOPFOREACH:
                        {
                            Memory::destroy(expr->uexpr.exprforeachloopstmt.iterator);
                            destroyExpression(expr->uexpr.exprforeachloopstmt.source);
                            Memory::destroy(expr->uexpr.exprforeachloopstmt.body);
                        }
                        break;
                    case EXPR_STMTLOOPFORCLASSIC:
                        {
                            destroyExpression(expr->uexpr.exprforloopstmt.init);
                            destroyExpression(expr->uexpr.exprforloopstmt.loopcond);
                            destroyExpression(expr->uexpr.exprforloopstmt.update);
                            Memory::destroy(expr->uexpr.exprforloopstmt.body);
                        }
                        break;
                    case EXPR_STMTBLOCK:
                        {
                            Memory::destroy(expr->uexpr.exprblockstmt);
                        }
                        break;
                    case EXPR_STMTIMPORT:
                        {
                            mc_memory_free(expr->uexpr.exprimportstmt.path);
                        }
                        break;
                    case EXPR_STMTRECOVER:
                        {
                            Memory::destroy(expr->uexpr.exprrecoverstmt.body);
                            Memory::destroy(expr->uexpr.exprrecoverstmt.errident);
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
        mcastexprtype_t exprtype;
        AstLocation pos;
        union mcexprunion_t
        {
            ExprIdent* exprident;
            mcfloat_t exprlitnumber;
            bool exprlitbool;
            ExprLiteralArray exprlitarray;
            ExprImportStmt exprimportstmt;
            ExprLiteralString exprlitstring;
            ExprLiteralMap exprlitmap;
            ExprPrefix exprprefix;
            ExprCall exprcall;
            ExprIfStmt exprifstmt;
            ExprWhileStmt exprwhileloopstmt;
            ExprCodeBlock* exprblockstmt;
            ExprRecover exprrecoverstmt;
            ExprInfix exprinfix;
            ExprLiteralFunction exprlitfunction;
            ExprIndex exprindex;
            ExprAssign exprassign;
            ExprLogical exprlogical;
            ExprTernary exprternary;
            ExprDefine exprdefine;
            ExprForeachStmt exprforeachloopstmt;
            ExprLoopStmt exprforloopstmt;
            AstExpression* exprreturnvalue;
            AstExpression* exprexpression;
        } uexpr;

    public:
};


class CompiledProgram
{
    public:
        uint16_t* bytecode;
        AstLocation* progsrcposlist;
        int count;

    public:
        CompiledProgram(uint16_t* bc, AstLocation* spl, int cnt)
        {
            this->bytecode = bc;
            this->progsrcposlist = spl;
            this->count = cnt;
        }

        static void destroy(CompiledProgram* res)
        {
            if(res != nullptr)
            {
                mc_memory_free(res->bytecode);
                mc_memory_free(res->progsrcposlist);
                mc_memory_free(res);
            }
        }

};


class AstSourceFile
{
    public:
        static void destroy(AstSourceFile* file)
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
                mc_memory_free(file->m_dirpath);
                mc_memory_free(file->m_path);
                mc_memory_free(file);
            }
        }

    public:
        char* m_dirpath;
        char* m_path;
        PtrList* lines;

    public:
        AstSourceFile(const char* strpath)
        {
            size_t len;
            const char* lastslashpos;
            lastslashpos = strrchr(strpath, '/');
            if(lastslashpos)
            {
                len = lastslashpos - strpath + 1;
                m_dirpath = mc_util_strndup(strpath, len);
            }
            else
            {
                m_dirpath = mc_util_strdup("");
            }
            MC_ASSERT(m_dirpath);
            m_path = mc_util_strdup(strpath);
            MC_ASSERT(m_path);
            this->lines = Memory::make<PtrList>(sizeof(void*), true);
        }

        const char* getDirectory() const
        {
            return m_dirpath;
        }
        
        const char* path() const
        {
            return m_path;
        }
};

class Traceback
{
    public:
        class Item
        {
            public:
                char* trfuncname;
                AstLocation pos;

            public:
                const char* getSourceLine()
                {
                    const char* line;
                    PtrList* lines;
                    if(!this->pos.file)
                    {
                        return nullptr;
                    }
                    lines = this->pos.file->lines;
                    if((size_t)this->pos.line >= (size_t)lines->count())
                    {
                        return nullptr;
                    }
                    line = (const char*)lines->get(this->pos.line);
                    return line;
                }

                const char* getSourceFilename()
                {
                    if(!this->pos.file)
                    {
                        return nullptr;
                    }
                    return this->pos.file->path();
                }
        };

    public:
        PtrList* tbitems;

    public:
        Traceback()
        {
            this->tbitems = Memory::make<PtrList>(sizeof(Item), false);
        }

        static void destroy(Traceback* traceback)
        {
            size_t i;
            Item* item;
            if(traceback != nullptr)
            {
                for(i = 0; i < traceback->tbitems->count(); i++)
                {
                    item = (Item*)traceback->tbitems->get(i);
                    mc_memory_free(item->trfuncname);
                    item->trfuncname = nullptr;
                }
                PtrList::destroy(traceback->tbitems, nullptr);
                mc_memory_free(traceback);
                traceback = nullptr;
            }
        }

        bool push(const char* fname, AstLocation pos)
        {
            bool ok;
            Item item;
            (void)ok;
            item.trfuncname = mc_util_strdup(fname);
            if(!item.trfuncname)
            {
                return false;
            }
            item.pos = pos;
            ok = this->tbitems->push(&item);
            return true;
        }

        int mc_traceback_getdepth()
        {
            return this->tbitems->count();
        }

        const char* mc_traceback_getsourcefilepath(int depth)
        {
            Item* item;
            item = (Item*)this->tbitems->get(depth);
            if(!item)
            {
                return nullptr;
            }
            return item->getSourceFilename();
        }

        const char* mc_traceback_getsourcelinecode(int depth)
        {
            Item* item;
            item = (Item*)this->tbitems->get(depth);
            if(!item)
            {
                return nullptr;
            }
            return item->getSourceLine();
        }

        int mc_traceback_getsourcelinenumber(int depth)
        {
            Item* item;
            item = (Item*)this->tbitems->get(depth);
            if(!item)
            {
                return -1;
            }
            return item->pos.line;
        }

        int mc_traceback_getsourcecolumn(int depth)
        {
            Item* item;
            item = (Item*)this->tbitems->get(depth);
            if(!item)
            {
                return -1;
            }
            return item->pos.column;
        }

        const char* mc_traceback_getfunctionname(int depth)
        {
            Item* item;
            item = (Item*)this->tbitems->get(depth);
            if(!item)
            {
                return "";
            }
            return item->trfuncname;
        }

        bool printTo(Printer* pr, mcconsolecolor_t* mcc)
        {
            int i;
            int depth;
            const char* cblue;
            const char* cyell;
            const char* creset;
            const char* filename;
            Item* item;
            cblue = mc_consolecolor_get(mcc, 'b');
            cyell = mc_consolecolor_get(mcc, 'y');
            creset = mc_consolecolor_get(mcc, '0');
            depth = this->tbitems->count();
            for(i = 0; i < depth; i++)
            {
                item = (Item*)this->tbitems->get(i);
                filename = item->getSourceFilename();
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

};

class Error
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
        AstLocation m_pos = {};
        Traceback* m_traceback = nullptr;

    public:
        Error()
        {
        }

        const char* getMessage()
        {
            return m_message;
        }

        const char* getFile()
        {
            if(!m_pos.file)
            {
                return nullptr;
            }
            return m_pos.file->path();
        }

        const char* getSourceLineCode()
        {
            const char* line;
            PtrList* lines;
            if(!m_pos.file)
            {
                return nullptr;
            }
            lines = m_pos.file->lines;
            if(m_pos.line >= (int)lines->count())
            {
                return nullptr;
            }
            line = (const char*)lines->get(m_pos.line);
            return line;
        }

        int getSourceLineNumber()
        {
            if(m_pos.line < 0)
            {
                return -1;
            }
            return m_pos.line + 1;
        }

        int getSourceColumn()
        {
            if(m_pos.column < 0)
            {
                return -1;
            }
            return m_pos.column + 1;
        }

        mcerrtype_t getType()
        {
            switch(m_errtype)
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

        Traceback* getTraceback()
        {
            return m_traceback;
        }

        bool printErrorTo(Printer* pr)
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
            Traceback* traceback;
            mcconsolecolor_t mcc;
            mc_consolecolor_init(&mcc, fileno(stdout));
            cred = mc_consolecolor_get(&mcc, 'r');
            cblue = mc_consolecolor_get(&mcc, 'b');
            creset = mc_consolecolor_get(&mcc, '0');
            typestr = this->getTypeString();
            filename = this->getFile();
            line = this->getSourceLineCode();
            linenum = this->getSourceLineNumber();
            colnum = this->getSourceColumn();
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
            pr->format("%s%s ERROR%s in \"%s\" on %s%d:%d:%s %s\n", cred, typestr, creset, filename, cblue, linenum, colnum, creset, this->getMessage());
            traceback = this->getTraceback();
            if(traceback)
            {
                pr->format("traceback:\n");
                this->getTraceback()->printTo(pr, &mcc);
            }
            return true;
        }

};

class ErrList
{
    public:
        Error m_erroritems[MC_CONF_MAXERRORCOUNT];
        int m_count;

    public:
        ErrList()
        {
            m_count = 0;
        }

        void destroy()
        {
            this->clear();
        }

        size_t count() const
        {
            return m_count;
        }

        void clear()
        {
            int i;
            Error* error;
            for(i = 0; i < m_count; i++)
            {
                error = this->get(i);
                if(error->m_traceback)
                {
                    Memory::destroy(error->m_traceback);
                }
            }
            m_count = 0;
        }

        void pushMessage(mcerrtype_t type, AstLocation pos, const char* message)
        {
            int len;
            int tocopy;
            Error err;
            if(m_count >= MC_CONF_MAXERRORCOUNT)
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
                m_erroritems[m_count] = err;
                m_count++;
            }
        }

        template<typename... ArgsT>
        void pushFormat(mcerrtype_t type, AstLocation pos, const char* format, ArgsT&&... args)
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

        Error* get(int ix)
        {
            if(ix >= m_count)
            {
                return nullptr;
            }
            return &m_erroritems[ix];
        }

        Error* getLast()
        {
            if(m_count <= 0)
            {
                return nullptr;
            }
            return &m_erroritems[m_count - 1];
        }
};

class AstInfo
{
    public:
        ErrList* m_errors;
        const char* m_inputsource;
        int m_inputlength;
        int m_position;
        int m_nextposition;
        char m_ch;
        size_t m_line;
        size_t m_column;
        AstSourceFile* m_file;
        bool m_failed;
        bool m_continuetplstring;
        Token m_prevtoken;
        Token m_currtoken;
        Token m_peektoken;
};

class AstLexer: public AstInfo
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

        static bool init(AstLexer* lex, ErrList* errs, const char* input, AstSourceFile* file)
        {
            bool ok;
            (void)ok;
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
            lex->m_prevtoken = Token(Token::TOK_INVALID, nullptr, 0);
            lex->m_currtoken = Token(Token::TOK_INVALID, nullptr, 0);
            lex->m_peektoken = Token(Token::TOK_INVALID, nullptr, 0);
            return true;
        }

    public:
        AstInfo m_prevstate;

    public:
        bool failed()
        {
            return m_failed;
        }

        void conttplstring()
        {
            m_continuetplstring = true;
        }

        bool currentTokenIs(Token::Type type)
        {
            return m_currtoken.toktype == type;
        }

        bool peekTokenIs(Token::Type type)
        {
            return m_peektoken.toktype == type;
        }

        bool nextToken()
        {
            m_prevtoken = m_currtoken;
            m_currtoken = m_peektoken;
            m_peektoken = this->nextTokenInternal();
            return !m_failed;
        }

        bool previousToken()
        {
            if(m_prevtoken.toktype == Token::TOK_INVALID)
            {
                return false;
            }
            m_peektoken = m_currtoken;
            m_currtoken = m_prevtoken;
            m_prevtoken = Token(Token::TOK_INVALID, nullptr, 0);
            m_ch = m_prevstate.m_ch;
            m_column = m_prevstate.m_column;
            m_line = m_prevstate.m_line;
            m_position = m_prevstate.m_position;
            m_nextposition = m_prevstate.m_nextposition;
            return true;
        }

        Token nextTokenInternal()
        {
            char c;
            Token outtok;
            m_prevstate.m_ch = m_ch;
            m_prevstate.m_column = m_column;
            m_prevstate.m_line = m_line;
            m_prevstate.m_position = m_position;
            m_prevstate.m_nextposition = m_nextposition;
            while(true)
            {
                if(!m_continuetplstring)
                {
                    this->skipSpace();
                }
                outtok.toktype = Token::TOK_INVALID;
                outtok.tokstrdata = m_inputsource + m_position;
                outtok.tokstrlen = 1;
                outtok.m_tokpos = AstLocation(m_file, m_line, m_column);
                c = m_continuetplstring ? '`' : m_ch;
                switch(c)
                {
                    case '\0':
                        {
                            outtok = Token(Token::TOK_EOF, "EOF", 3);
                        }
                        break;
                    case '=':
                        {
                            if(this->peekChar() == '=')
                            {
                                outtok = Token(Token::TOK_EQ, "==", 2);
                                this->readChar();
                            }
                            else
                            {
                                outtok = Token(Token::TOK_ASSIGN, "=", 1);
                            }
                        }
                        break;
                    case '&':
                        {
                            if(this->peekChar() == '&')
                            {
                                outtok = Token(Token::TOK_AND, "&&", 2);
                                this->readChar();
                            }
                            else if(this->peekChar() == '=')
                            {
                                outtok = Token(Token::TOK_ASSIGNBINAND, "&=", 2);
                                this->readChar();
                            }
                            else
                            {
                                outtok = Token(Token::TOK_BINAND, "&", 1);
                            }
                        }
                        break;
                    case '|':
                        {
                            if(this->peekChar() == '|')
                            {
                                outtok = Token(Token::TOK_OR, "||", 2);
                                this->readChar();
                            }
                            else if(this->peekChar() == '=')
                            {
                                outtok = Token(Token::TOK_ASSIGNBINOR, "|=", 2);
                                this->readChar();
                            }
                            else
                            {
                                outtok = Token(Token::TOK_BINOR, "|", 1);
                            }
                        }
                        break;
                    case '^':
                        {
                            if(this->peekChar() == '=')
                            {
                                outtok = Token(Token::TOK_ASSIGNBINXOR, "^=", 2);
                                this->readChar();
                            }
                            else
                            {
                                outtok = Token(Token::TOK_BINXOR, "^", 1);
                                break;
                            }
                        }
                        break;
                    case '+':
                        {
                            if(this->peekChar() == '=')
                            {
                                outtok = Token(Token::TOK_ASSIGNPLUS, "+=", 2);
                                this->readChar();
                            }
                            else if(this->peekChar() == '+')
                            {
                                outtok = Token(Token::TOK_PLUSPLUS, "++", 2);
                                this->readChar();
                            }
                            else
                            {
                                outtok = Token(Token::TOK_PLUS, "+", 1);
                                break;
                            }
                        }
                        break;
                    case '-':
                        {
                            if(this->peekChar() == '=')
                            {
                                outtok = Token(Token::TOK_ASSIGNMINUS, "-=", 2);
                                this->readChar();
                            }
                            else if(this->peekChar() == '-')
                            {
                                outtok = Token(Token::TOK_MINUSMINUS, "--", 2);
                                this->readChar();
                            }
                            else
                            {
                                outtok = Token(Token::TOK_UNARYMINUS, "-", 1);
                                break;
                            }
                        }
                        break;
                    case '~':
                        {
                            outtok = Token(Token::TOK_UNARYBINNOT, "~", 1);
                        }
                        break;
                    case '!':
                        {
                            if(this->peekChar() == '=')
                            {
                                outtok = Token(Token::TOK_NOTEQ, "!=", 2);
                                this->readChar();
                            }
                            else
                            {
                                outtok = Token(Token::TOK_BANG, "!", 1);
                            }
                        }
                        break;
                    case '*':
                        {
                            if(this->peekChar() == '=')
                            {
                                outtok = Token(Token::TOK_ASSIGNASTERISK, "*=", 2);
                                this->readChar();
                            }
                            else
                            {
                                outtok = Token(Token::TOK_ASTERISK, "*", 1);
                                break;
                            }
                        }
                        break;
                    case '/':
                        {
                            if(this->peekChar() == '/')
                            {
                                this->readChar();
                                while(m_ch != '\n' && m_ch != '\0')
                                {
                                    this->readChar();
                                }
                                continue;
                            }
                            if(this->peekChar() == '=')
                            {
                                outtok = Token(Token::TOK_ASSIGNSLASH, "/=", 2);
                                this->readChar();
                            }
                            else
                            {
                                outtok = Token(Token::TOK_SLASH, "/", 1);
                                break;
                            }
                        }
                        break;
                    case '<':
                        {
                            if(this->peekChar() == '=')
                            {
                                outtok = Token(Token::TOK_LTE, "<=", 2);
                                this->readChar();
                            }
                            else if(this->peekChar() == '<')
                            {
                                this->readChar();
                                if(this->peekChar() == '=')
                                {
                                    outtok = Token(Token::TOK_ASSIGNLSHIFT, "<<=", 3);
                                    this->readChar();
                                }
                                else
                                {
                                    outtok = Token(Token::TOK_LSHIFT, "<<", 2);
                                }
                            }
                            else
                            {
                                outtok = Token(Token::TOK_LT, "<", 1);
                                break;
                            }
                        }
                        break;
                    case '>':
                        {
                            if(this->peekChar() == '=')
                            {
                                outtok = Token(Token::TOK_GTE, ">=", 2);
                                this->readChar();
                            }
                            else if(this->peekChar() == '>')
                            {
                                this->readChar();
                                if(this->peekChar() == '=')
                                {
                                    outtok = Token(Token::TOK_ASSIGNRSHIFT, ">>=", 3);
                                    this->readChar();
                                }
                                else
                                {
                                    outtok = Token(Token::TOK_RSHIFT, ">>", 2);
                                }
                            }
                            else
                            {
                                outtok = Token(Token::TOK_GT, ">", 1);
                            }
                        }
                        break;
                    case ',':
                        {
                            outtok = Token(Token::TOK_COMMA, ",", 1);
                        }
                        break;
                    case ';':
                        {
                            outtok = Token(Token::TOK_SEMICOLON, ";", 1);
                        }
                        break;
                    case ':':
                        {
                            outtok = Token(Token::TOK_COLON, ":", 1);
                        }
                        break;
                    case '(':
                        {
                            outtok = Token(Token::TOK_LPAREN, "(", 1);
                        }
                        break;
                    case ')':
                        {
                            outtok = Token(Token::TOK_RPAREN, ")", 1);
                        }
                        break;
                    case '{':
                        {
                            outtok = Token(Token::TOK_LBRACE, "{", 1);
                        }
                        break;
                    case '}':
                        {
                            outtok = Token(Token::TOK_RBRACE, "}", 1);
                        }
                        break;
                    case '[':
                        {
                            outtok = Token(Token::TOK_LBRACKET, "[", 1);
                        }
                        break;
                    case ']':
                        {
                            outtok = Token(Token::TOK_RBRACKET, "]", 1);
                        }
                        break;
                    case '.':
                        {
                            outtok = Token(Token::TOK_DOT, ".", 1);
                        }
                        break;
                    case '?':
                        {
                            outtok = Token(Token::TOK_QUESTION, "?", 1);
                        }
                        break;
                    case '%':
                        {
                            if(this->peekChar() == '=')
                            {
                                outtok = Token(Token::TOK_ASSIGNPERCENT, "%=", 2);
                                this->readChar();
                            }
                            else
                            {
                                outtok = Token(Token::TOK_PERCENT, "%", 1);
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
                                outtok = Token(Token::TOK_STRING, str, len);
                            }
                            else
                            {
                                outtok = Token(Token::TOK_INVALID, nullptr, 0);
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
                                outtok = Token(Token::TOK_STRING, str, len);
                            }
                            else
                            {
                                outtok = Token(Token::TOK_INVALID, nullptr, 0);
                            }
                        }
                        break;
                    case '`':
                        {
                            int len;
                            bool templatefound;
                            const char* str;
                            if(!m_continuetplstring)
                            {
                                this->readChar();
                            }
                            templatefound = false;
                            str = this->scanString('`', true, &templatefound, &len);
                            if(str)
                            {
                                if(templatefound)
                                {
                                    outtok = Token(Token::TOK_TEMPLATESTRING, str, len);
                                }
                                else
                                {
                                    outtok = Token(Token::TOK_STRING, str, len);
                                }
                            }
                            else
                            {
                                outtok = Token(Token::TOK_INVALID, nullptr, 0);
                            }
                        }
                        break;
                    default:
                        {
                            int identlen;
                            int numberlen;
                            const char* ident;
                            const char* number;
                            Token::Type type;
                            if(charIsLetter(m_ch) || (m_ch == '$'))
                            {
                                identlen = 0;
                                ident = this->scanIdent(&identlen);
                                type = this->lookupIdent(ident, identlen);
                                outtok = Token(type, ident, identlen);
                                outtok.m_tokpos = AstLocation(m_file, m_line, m_column);
                                return outtok;
                            }
                            if(charIsDigit(m_ch))
                            {
                                numberlen = 0;
                                number = this->scanNumber(&numberlen);
                                outtok = Token(Token::TOK_NUMBER, number, numberlen);
                                outtok.m_tokpos = AstLocation(m_file, m_line, m_column);
                                return outtok;
                            }
                        }
                        break;
                }
                this->readChar();
                if(this->failed())
                {
                    outtok = Token(Token::TOK_INVALID, nullptr, 0);
                }
                m_continuetplstring = false;
                outtok.m_tokpos = AstLocation(m_file, m_line, m_column);
                return outtok;
            }
            /* NB. never reached; but keep the compiler from complaining. */
            return outtok;
        }

        bool expectCurrent(Token::Type type)
        {
            const char* actualtypestr;
            const char* expectedtypestr;
            if(this->failed())
            {
                return false;
            }
            if(!this->currentTokenIs(type))
            {
                expectedtypestr = Token::tokenName(type);
                actualtypestr = Token::tokenName(m_currtoken.toktype);
                m_errors->pushFormat(MC_ERROR_PARSING, m_currtoken.m_tokpos, "expected token \"%s\", but got \"%s\"", expectedtypestr, actualtypestr);
                return false;
            }
            return true;
        }

        bool readChar()
        {
            bool ok;
            (void)ok;
            if(m_nextposition >= m_inputlength)
            {
                m_ch = '\0';
            }
            else
            {
                m_ch = m_inputsource[m_nextposition];
            }
            m_position = m_nextposition;
            m_nextposition++;
            if(m_ch == '\n')
            {
                m_line++;
                m_column = -1;
                ok = this->addLineAt(m_nextposition);
                if(!ok)
                {
                    m_failed = true;
                    return false;
                }
            }
            else
            {
                m_column++;
            }
            return true;
        }

        char peekChar()
        {
            if(m_nextposition >= m_inputlength)
            {
                return '\0';
            }
            return m_inputsource[m_nextposition];
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
            position = m_position;
            while(charIsDigit(m_ch) || charIsLetter(m_ch) ||(m_ch == '$') || (m_ch == ':'))
            {
                if(m_ch == ':')
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
            len = m_position - position;
            *outlen = len;
            return m_inputsource + position;
        }

        const char* scanNumber(int* outlen)
        {
            int len;
            int position;
            static const char allowed[] = ".xXaAbBcCdDeEfF";
            position = m_position;
            while(charIsDigit(m_ch) || charIsOneOf(m_ch, allowed, MC_UTIL_STATICARRAYSIZE(allowed) - 1))
            {
                this->readChar();
            }
            len = m_position - position;
            *outlen = len;
            return m_inputsource + position;
        }

        const char* scanString(char delimiter, bool istemplate, bool* outtemplatefound, int* outlen)
        {
            bool escaped;
            int len;
            int position;
            *outlen = 0;
            escaped = false;
            position = m_position;
            while(true)
            {
                if(m_ch == '\0')
                {
                    return nullptr;
                }
                if(m_ch == delimiter && !escaped)
                {
                    break;
                }
                if(istemplate && !escaped && m_ch == '$' && this->peekChar() == '{')
                {
                    *outtemplatefound = true;
                    break;
                }
                escaped = false;
                if(m_ch == '\\')
                {
                    escaped = true;
                }
                this->readChar();
            }
            len = m_position - position;
            *outlen = len;
            return m_inputsource + position;
        }

        Token::Type lookupIdent(const char* ident, int len)
        {
            int i;
            int klen;
            static struct
            {
                const char* value;
                Token::Type type;
            } keywords[] = {
                { "function", Token::TOK_FUNCTION },
                { "const", Token::TOK_CONST },
                { "var", Token::TOK_VAR },
                { "let", Token::TOK_VAR },
                { "true", Token::TOK_TRUE },
                { "false", Token::TOK_FALSE },
                { "if", Token::TOK_IF },
                { "else", Token::TOK_ELSE },
                { "return", Token::TOK_RETURN },
                { "while", Token::TOK_WHILE },
                { "break", Token::TOK_BREAK },
                { "for", Token::TOK_FOR },
                { "in", Token::TOK_IN },
                { "continue", Token::TOK_CONTINUE },
                { "null", Token::TOK_NULL },
                { "import", Token::TOK_IMPORT },
                { "recover", Token::TOK_RECOVER },
                { nullptr, (Token::Type)0}
            };
            for(i = 0; keywords[i].value != nullptr; i++)
            {
                klen = mc_util_strlen(keywords[i].value);
                if(klen == len && mc_util_strnequal(ident, keywords[i].value, len))
                {
                    return keywords[i].type;
                }
            }
            return Token::TOK_IDENT;
        }

        void skipSpace()
        {
            char ch;
            ch = m_ch;
            while(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
            {
                this->readChar();
                ch = m_ch;
            }
        }

        bool addLineAt(int offset)
        {
            bool ok;
            size_t linelen;
            char* line;
            const char* linestart;
            const char* newlineptr;
            (void)ok;
            if(!m_file)
            {
                return true;
            }
            if(m_line < m_file->lines->count())
            {
                return true;
            }
            linestart = m_inputsource + offset;
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
                m_failed = true;
                return false;
            }
            ok = m_file->lines->push(line);
            return true;
        }
};

class AstParser
{
    public:
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

    public:
        static mcastprecedence_t getPrecedence(Token::Type tk)
        {
            switch(tk)
            {
                case Token::TOK_EQ:
                case Token::TOK_NOTEQ:
                    return MC_ASTPREC_EQUALS;
                case Token::TOK_LT:
                case Token::TOK_LTE:
                case Token::TOK_GT:
                case Token::TOK_GTE:
                    return MC_ASTPREC_LESSGREATER;
                case Token::TOK_PLUS:
                case Token::TOK_UNARYMINUS:
                case Token::TOK_UNARYBINNOT:
                    return MC_ASTPREC_SUM;
                case Token::TOK_SLASH:
                case Token::TOK_ASTERISK:
                case Token::TOK_PERCENT:
                    return MC_ASTPREC_PRODUCT;
                case Token::TOK_LPAREN:
                case Token::TOK_LBRACKET:
                    return MC_ASTPREC_POSTFIX;
                case Token::TOK_ASSIGN:
                case Token::TOK_ASSIGNPLUS:
                case Token::TOK_ASSIGNMINUS:
                case Token::TOK_ASSIGNASTERISK:
                case Token::TOK_ASSIGNSLASH:
                case Token::TOK_ASSIGNPERCENT:
                case Token::TOK_ASSIGNBINAND:
                case Token::TOK_ASSIGNBINOR:
                case Token::TOK_ASSIGNBINXOR:
                case Token::TOK_ASSIGNLSHIFT:
                case Token::TOK_ASSIGNRSHIFT:
                    return MC_ASTPREC_ASSIGN;
                case Token::TOK_DOT:
                    return MC_ASTPREC_POSTFIX;
                case Token::TOK_AND:
                    return MC_ASTPREC_LOGICALAND;
                case Token::TOK_OR:
                    return MC_ASTPREC_LOGICALOR;
                case Token::TOK_BINOR:
                    return MC_ASTPREC_BINOR;
                case Token::TOK_BINXOR:
                    return MC_ASTPREC_BINXOR;
                case Token::TOK_BINAND:
                    return MC_ASTPREC_BINAND;
                case Token::TOK_LSHIFT:
                case Token::TOK_RSHIFT:
                    return MC_ASTPREC_SHIFT;
                case Token::TOK_QUESTION:
                    return MC_ASTPREC_TERNARY;
                case Token::TOK_PLUSPLUS:
                case Token::TOK_MINUSMINUS:
                    return MC_ASTPREC_INCDEC;
                default:
                    break;
            }
            return MC_ASTPREC_LOWEST;
        }

        static mcastmathoptype_t tokenToMathOP(Token::Type tk)
        {
            switch(tk)
            {
                case Token::TOK_ASSIGN:
                    return MC_MATHOP_ASSIGN;
                case Token::TOK_PLUS:
                    return MC_MATHOP_PLUS;
                case Token::TOK_UNARYMINUS:
                    return MC_MATHOP_MINUS;
                case Token::TOK_UNARYBINNOT:
                    return MC_MATHOP_BINNOT;
                case Token::TOK_BANG:
                    return MC_MATHOP_BANG;
                case Token::TOK_ASTERISK:
                    return MC_MATHOP_ASTERISK;
                case Token::TOK_SLASH:
                    return MC_MATHOP_SLASH;
                case Token::TOK_LT:
                    return MC_MATHOP_LT;
                case Token::TOK_LTE:
                    return MC_MATHOP_LTE;
                case Token::TOK_GT:
                    return MC_MATHOP_GT;
                case Token::TOK_GTE:
                    return MC_MATHOP_GTE;
                case Token::TOK_EQ:
                    return MC_MATHOP_EQ;
                case Token::TOK_NOTEQ:
                    return MC_MATHOP_NOTEQ;
                case Token::TOK_PERCENT:
                    return MC_MATHOP_MODULUS;
                case Token::TOK_AND:
                    return MC_MATHOP_LOGICALAND;
                case Token::TOK_OR:
                    return MC_MATHOP_LOGICALOR;
                case Token::TOK_ASSIGNPLUS:
                    return MC_MATHOP_PLUS;
                case Token::TOK_ASSIGNMINUS:
                    return MC_MATHOP_MINUS;
                case Token::TOK_ASSIGNASTERISK:
                    return MC_MATHOP_ASTERISK;
                case Token::TOK_ASSIGNSLASH:
                    return MC_MATHOP_SLASH;
                case Token::TOK_ASSIGNPERCENT:
                    return MC_MATHOP_MODULUS;
                case Token::TOK_ASSIGNBINAND:
                    return MC_MATHOP_BINAND;
                case Token::TOK_ASSIGNBINOR:
                    return MC_MATHOP_BINOR;
                case Token::TOK_ASSIGNBINXOR:
                    return MC_MATHOP_BINXOR;
                case Token::TOK_ASSIGNLSHIFT:
                    return MC_MATHOP_LSHIFT;
                case Token::TOK_ASSIGNRSHIFT:
                    return MC_MATHOP_RSHIFT;
                case Token::TOK_BINAND:
                    return MC_MATHOP_BINAND;
                case Token::TOK_BINOR:
                    return MC_MATHOP_BINOR;
                case Token::TOK_BINXOR:
                    return MC_MATHOP_BINXOR;
                case Token::TOK_LSHIFT:
                    return MC_MATHOP_LSHIFT;
                case Token::TOK_RSHIFT:
                    return MC_MATHOP_RSHIFT;
                case Token::TOK_PLUSPLUS:
                    return MC_MATHOP_PLUS;
                case Token::TOK_MINUSMINUS:
                    return MC_MATHOP_MINUS;
                default:
                    {
                        MC_ASSERT(false);
                    }
                    break;
            }
            return MC_MATHOP_NONE;
        }

        static char getEscapeChar(char c)
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

        static char* processAndCopyString(const char* input, size_t len)
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
                    output[outi] = getEscapeChar(input[ini]);
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

        static AstExpression* do_parsevarletstmt(AstParser* p)
        {
            bool assignable;
            AstExpression::ExprIdent* nameident;
            AstExpression* value;
            AstExpression* res;
            nameident = nullptr;
            value = nullptr;
            assignable = p->lexer.currentTokenIs(Token::TOK_VAR);
            p->lexer.nextToken();
            if(!p->lexer.expectCurrent(Token::TOK_IDENT))
            {
                goto err;
            }
            nameident = Memory::make<AstExpression::ExprIdent>(p->lexer.m_currtoken);
            p->lexer.nextToken();
            #if 0
                if(!p->lexer.expectCurrent(Token::TOK_ASSIGN))
            #else
                if(!p->lexer.currentTokenIs(Token::TOK_ASSIGN))
            #endif
            {
                value = AstExpression::makeliteralnull();
                goto finish;
            }
            p->lexer.nextToken();
            value = do_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!value)
            {
                goto err;
            }
            if(value->exprtype == AstExpression::EXPR_FUNCTIONLITERAL)
            {
                value->uexpr.exprlitfunction.name = mc_util_strdup(nameident->value);
                if(!value->uexpr.exprlitfunction.name)
                {
                    goto err;
                }
            }
            finish:
            res = AstExpression::makedefine(nameident, value, assignable);
            if(!res)
            {
                goto err;
            }
            return res;
        err:
            AstExpression::destroyExpression(value);
            Memory::destroy(nameident);
            return nullptr;
        }

        static AstExpression* do_parseifstmt(AstParser* p)
        {
            bool ok;
            PtrList* cases;
            AstExpression::ExprIfCase* cond;
            AstExpression::ExprIfCase* elif;
            AstExpression::ExprCodeBlock* alternative;
            (void)ok;
            AstExpression* res;
            cases = nullptr;
            alternative = nullptr;
            cases = Memory::make<PtrList>(sizeof(void*), true);
            p->lexer.nextToken();
            if(!p->lexer.expectCurrent(Token::TOK_LPAREN))
            {
                goto err;
            }
            p->lexer.nextToken();
            cond = Memory::make<AstExpression::ExprIfCase>(nullptr, nullptr);
            ok = cases->push(cond);
            cond->ifcond = do_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!cond->ifcond)
            {
                goto err;
            }
            if(!p->lexer.expectCurrent(Token::TOK_RPAREN))
            {
                goto err;
            }
            p->lexer.nextToken();
            cond->consequence = do_parsecodeblock(p);
            if(!cond->consequence)
            {
                goto err;
            }
            while(p->lexer.currentTokenIs(Token::TOK_ELSE))
            {
                p->lexer.nextToken();
                if(p->lexer.currentTokenIs(Token::TOK_IF))
                {
                    p->lexer.nextToken();
                    if(!p->lexer.expectCurrent(Token::TOK_LPAREN))
                    {
                        goto err;
                    }
                    p->lexer.nextToken();
                    elif = Memory::make<AstExpression::ExprIfCase>(nullptr, nullptr);
                    ok = cases->push(elif);
                    elif->ifcond = do_parseexpression(p, MC_ASTPREC_LOWEST);
                    if(!elif->ifcond)
                    {
                        goto err;
                    }
                    if(!p->lexer.expectCurrent(Token::TOK_RPAREN))
                    {
                        goto err;
                    }
                    p->lexer.nextToken();
                    elif->consequence = do_parsecodeblock(p);
                    if(!elif->consequence)
                    {
                        goto err;
                    }
                }
                else
                {
                    alternative = do_parsecodeblock(p);
                    if(!alternative)
                    {
                        goto err;
                    }
                }
            }
            res = AstExpression::makeif(cases, alternative);
            if(!res)
            {
                goto err;
            }
            return res;
        err:
            PtrList::destroy(cases, (mcitemdestroyfn_t)AstExpression::ExprIfCase::destroy);
            Memory::destroy(alternative);
            return nullptr;
        }

        static AstExpression* do_parsereturnstmt(AstParser* p)
        {
            AstExpression* res;
            AstExpression* expr;
            expr = nullptr;
            p->lexer.nextToken();
            if(!p->lexer.currentTokenIs(Token::TOK_SEMICOLON) && !p->lexer.currentTokenIs(Token::TOK_RBRACE) && !p->lexer.currentTokenIs(Token::TOK_EOF))
            {
                expr = do_parseexpression(p, MC_ASTPREC_LOWEST);
                if(!expr)
                {
                    return nullptr;
                }
            }
            res = AstExpression::makereturn(expr);
            if(!res)
            {
                AstExpression::destroyExpression(expr);
                return nullptr;
            }
            return res;
        }

        static AstExpression* do_parseexprstmt(AstParser* p)
        {
            AstExpression* res;
            AstExpression* expr;
            expr = do_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!expr)
            {
                return nullptr;
            }
            if(expr && (!p->m_config->replmode || p->depth > 0))
            {
                #if 0
                /* this is actually completely unnecessary */
                if(expr->exprtype != AstExpression::EXPR_ASSIGN && expr->exprtype != AstExpression::EXPR_CALL)
                {
                    p->m_prserrlist->pushFormat(MC_ERROR_PARSING, expr->pos, "only assignments and function calls can be expression statements");
                    AstExpression::destroyExpression(expr);
                    return nullptr;
                }
                #endif
            }
            res = AstExpression::makeexprstmt(expr);
            if(!res)
            {
                AstExpression::destroyExpression(expr);
                return nullptr;
            }
            return res;
        }

        static AstExpression* do_parseloopwhilestmt(AstParser* p)
        {
            AstExpression* res;
            AstExpression* test;
            AstExpression::ExprCodeBlock* body;
            test = nullptr;
            body = nullptr;
            p->lexer.nextToken();
            if(!p->lexer.expectCurrent(Token::TOK_LPAREN))
            {
                goto err;
            }
            p->lexer.nextToken();
            test = do_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!test)
            {
                goto err;
            }
            if(!p->lexer.expectCurrent(Token::TOK_RPAREN))
            {
                goto err;
            }
            p->lexer.nextToken();
            body = do_parsecodeblock(p);
            if(!body)
            {
                goto err;
            }
            res = AstExpression::makewhile(test, body);
            if(!res)
            {
                goto err;
            }
            return res;
        err:
            Memory::destroy(body);
            AstExpression::destroyExpression(test);
            return nullptr;
        }

        static AstExpression* do_parsebreakstmt(AstParser* p)
        {
            p->lexer.nextToken();
            return AstExpression::makebreak();
        }

        static AstExpression* do_parsecontinuestmt(AstParser* p)
        {
            p->lexer.nextToken();
            return AstExpression::makecontinue();
        }

        static AstExpression* do_parseblockstmt(AstParser* p)
        {
            AstExpression::ExprCodeBlock* block;
            AstExpression* res;
            block = do_parsecodeblock(p);
            if(!block)
            {
                return nullptr;
            }
            res = AstExpression::makeblock(block);
            if(!res)
            {
                Memory::destroy(block);
                return nullptr;
            }
            return res;
        }

        static AstExpression* do_parseimportstmt(AstParser* p)
        {
            char* processedname;
            AstExpression* res;
            p->lexer.nextToken();
            if(!p->lexer.expectCurrent(Token::TOK_STRING))
            {
                return nullptr;
            }
            processedname = processAndCopyString(p->lexer.m_currtoken.tokstrdata, p->lexer.m_currtoken.tokstrlen);
            if(!processedname)
            {
                p->m_prserrlist->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.m_tokpos, "error when parsing module name");
                return nullptr;
            }
            p->lexer.nextToken();
            res = AstExpression::makeimport(processedname);
            if(!res)
            {
                mc_memory_free(processedname);
                return nullptr;
            }
            return res;
        }

        static AstExpression* do_parserecoverstmt(AstParser* p)
        {
            AstExpression* res;
            AstExpression::ExprIdent* eid;
            AstExpression::ExprCodeBlock* body;
            eid = nullptr;
            body = nullptr;
            p->lexer.nextToken();
            if(!p->lexer.expectCurrent(Token::TOK_LPAREN))
            {
                return nullptr;
            }
            p->lexer.nextToken();
            if(!p->lexer.expectCurrent(Token::TOK_IDENT))
            {
                return nullptr;
            }
            eid = Memory::make<AstExpression::ExprIdent>(p->lexer.m_currtoken);
            p->lexer.nextToken();
            if(!p->lexer.expectCurrent(Token::TOK_RPAREN))
            {
                goto err;
            }
            p->lexer.nextToken();
            body = do_parsecodeblock(p);
            if(!body)
            {
                goto err;
            }
            res = AstExpression::makeRecover(eid, body);
            if(!res)
            {
                goto err;
            }
            return res;
        err:
            Memory::destroy(body);
            Memory::destroy(eid);
            return nullptr;
        }

        static AstExpression* do_parseloopforloopstmt(AstParser* p)
        {
            p->lexer.nextToken();
            if(!p->lexer.expectCurrent(Token::TOK_LPAREN))
            {
                return nullptr;
            }
            p->lexer.nextToken();
            if(p->lexer.currentTokenIs(Token::TOK_IDENT) && p->lexer.peekTokenIs(Token::TOK_IN))
            {
                return do_parseloopforeachstmt(p);
            }
            return do_parseloopforcstylestmt(p);
        }

        static AstExpression* do_parseloopforeachstmt(AstParser* p)
        {
            AstExpression* res;
            AstExpression* source;
            AstExpression::ExprCodeBlock* body;
            AstExpression::ExprIdent* iteratorident;
            source = nullptr;
            body = nullptr;
            iteratorident = nullptr;
            iteratorident = Memory::make<AstExpression::ExprIdent>(p->lexer.m_currtoken);
            p->lexer.nextToken();
            if(!p->lexer.expectCurrent(Token::TOK_IN))
            {
                goto err;
            }
            p->lexer.nextToken();
            source = do_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!source)
            {
                goto err;
            }
            if(!p->lexer.expectCurrent(Token::TOK_RPAREN))
            {
                goto err;
            }
            p->lexer.nextToken();
            body = do_parsecodeblock(p);
            if(!body)
            {
                goto err;
            }
            res = AstExpression::makeforeach(iteratorident, source, body);
            if(!res)
            {
                goto err;
            }
            return res;
        err:
            Memory::destroy(body);
            Memory::destroy(iteratorident);
            AstExpression::destroyExpression(source);
            return nullptr;
        }

        static AstExpression* do_parseloopforcstylestmt(AstParser* p)
        {
            AstExpression* res;
            AstExpression* init;
            AstExpression* test;
            AstExpression* update;
            AstExpression::ExprCodeBlock* body;
            init = nullptr;
            test = nullptr;
            update = nullptr;
            body = nullptr;
            if(!p->lexer.currentTokenIs(Token::TOK_SEMICOLON))
            {
                init = p->parseStatement();
                if(!init)
                {
                    goto err;
                }
                if(init->exprtype != AstExpression::EXPR_STMTDEFINE && init->exprtype != AstExpression::EXPR_STMTEXPRESSION)
                {
                    p->m_prserrlist->pushFormat(MC_ERROR_PARSING, init->pos, "expected a definition or expression as 'for' loop init clause");
                    goto err;
                }
                if(!p->lexer.expectCurrent(Token::TOK_SEMICOLON))
                {
                    goto err;
                }
            }
            p->lexer.nextToken();
            if(!p->lexer.currentTokenIs(Token::TOK_SEMICOLON))
            {
                test = do_parseexpression(p, MC_ASTPREC_LOWEST);
                if(!test)
                {
                    goto err;
                }
                if(!p->lexer.expectCurrent(Token::TOK_SEMICOLON))
                {
                    goto err;
                }
            }
            p->lexer.nextToken();
            if(!p->lexer.currentTokenIs(Token::TOK_RPAREN))
            {
                update = do_parseexpression(p, MC_ASTPREC_LOWEST);
                if(!update)
                {
                    goto err;
                }
                if(!p->lexer.expectCurrent(Token::TOK_RPAREN))
                {
                    goto err;
                }
            }
            p->lexer.nextToken();
            body = do_parsecodeblock(p);
            if(!body)
            {
                goto err;
            }
            res = AstExpression::makeforloop(init, test, update, body);
            if(!res)
            {
                goto err;
            }
            return res;
        err:
            AstExpression::destroyExpression(init);
            AstExpression::destroyExpression(test);
            AstExpression::destroyExpression(update);
            Memory::destroy(body);
            return nullptr;
        }

        static AstExpression::ExprCodeBlock* do_parsecodeblock(AstParser* p)
        {
            bool ok;
            AstExpression::ExprCodeBlock* res;
            AstExpression* expr;
            PtrList* statements;
            (void)ok;
            if(!p->lexer.expectCurrent(Token::TOK_LBRACE))
            {
                return nullptr;
            }
            p->lexer.nextToken();
            p->depth++;
            statements = Memory::make<PtrList>(sizeof(void*), true);
            while(!p->lexer.currentTokenIs(Token::TOK_RBRACE))
            {
                if(p->lexer.currentTokenIs(Token::TOK_EOF))
                {
                    p->m_prserrlist->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.m_tokpos, "unexpected EOF");
                    goto err;
                }
                if(p->lexer.currentTokenIs(Token::TOK_SEMICOLON))
                {
                    p->lexer.nextToken();
                    continue;
                }
                expr = p->parseStatement();
                if(!expr)
                {
                    goto err;
                }
                ok = statements->push(expr);
            }
            p->lexer.nextToken();
            p->depth--;
            res = Memory::make<AstExpression::ExprCodeBlock>(statements);
            return res;
        err:
            p->depth--;
            PtrList::destroy(statements, (mcitemdestroyfn_t)AstExpression::destroyExpression);
            return nullptr;
        }

        static AstExpression* do_parseexpression(AstParser* p, mcastprecedence_t prec)
        {
            char* literal;
            AstLocation pos;
            mcleftassocparsefn_t parseleftassoc;
            mcastrightassocparsefn_t parserightassoc;
            AstExpression* newleftexpr;
            AstExpression* leftexpr;
            pos = p->lexer.m_currtoken.m_tokpos;
            if(p->lexer.m_currtoken.toktype == Token::TOK_INVALID)
            {
                p->m_prserrlist->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.m_tokpos, "illegal token");
                return nullptr;
            }
            parserightassoc = p->rightassocfuncs[p->lexer.m_currtoken.toktype];
            if(!parserightassoc)
            {
                literal = p->lexer.m_currtoken.dupLiteralString();
                p->m_prserrlist->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.m_tokpos, "no prefix parse function for \"%s\" found", literal);
                mc_memory_free(literal);
                return nullptr;
            }
            leftexpr = parserightassoc(p);
            if(!leftexpr)
            {
                return nullptr;
            }
            leftexpr->pos = pos;
            while(!p->lexer.currentTokenIs(Token::TOK_SEMICOLON) && prec < getPrecedence(p->lexer.m_currtoken.toktype))
            {
                parseleftassoc = p->leftassocfuncs[p->lexer.m_currtoken.toktype];
                if(!parseleftassoc)
                {
                    return leftexpr;
                }
                pos = p->lexer.m_currtoken.m_tokpos;
                newleftexpr = parseleftassoc(p, leftexpr);
                if(!newleftexpr)
                {
                    AstExpression::destroyExpression(leftexpr);
                    return nullptr;
                }
                newleftexpr->pos = pos;
                leftexpr = newleftexpr;
            }
            return leftexpr;
        }

        static AstExpression* do_parseident(AstParser* p)
        {
            AstExpression::ExprIdent* ident;
            AstExpression* res;
            ident = Memory::make<AstExpression::ExprIdent>(p->lexer.m_currtoken);
            res = AstExpression::makeident(ident);
            if(!res)
            {
                Memory::destroy(ident);
                return nullptr;
            }
            p->lexer.nextToken();
            return res;
        }

        static AstExpression* do_parseliteralnumber(AstParser* p)
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
                literal = p->lexer.m_currtoken.dupLiteralString();
                p->m_prserrlist->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.m_tokpos, "failed to parse number literal \"%s\"", literal);
                mc_memory_free(literal);
                return nullptr;
            }    
            p->lexer.nextToken();
            return AstExpression::makeliteralnumber(number);
        }

        static AstExpression* do_parseliteralbool(AstParser* p)
        {
            AstExpression* res;
            res = AstExpression::makeliteralbool(p->lexer.m_currtoken.toktype == Token::TOK_TRUE);
            p->lexer.nextToken();
            return res;
        }

        static AstExpression* do_parseliteralstring(AstParser* p)
        {
            size_t len;
            char* processedliteral;
            AstExpression* res;
            processedliteral = processAndCopyString(p->lexer.m_currtoken.tokstrdata, p->lexer.m_currtoken.tokstrlen);
            if(!processedliteral)
            {
                p->m_prserrlist->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.m_tokpos, "error parsing string literal");
                return nullptr;
            }
            p->lexer.nextToken();
            len = mc_util_strlen(processedliteral);
            res = AstExpression::makeliteralstring(processedliteral, len);
            if(!res)
            {
                mc_memory_free(processedliteral);
                return nullptr;
            }
            return res;
        }

        static AstExpression* do_parseliteraltemplatestring(AstParser* p)
        {
            size_t len;
            char* processedliteral;
            AstLocation pos;
            AstExpression* leftstringexpr;
            AstExpression* templateexpr;
            AstExpression* tostrcallexpr;
            AstExpression* leftaddexpr;
            AstExpression* rightexpr;
            AstExpression* rightaddexpr;
            processedliteral = nullptr;
            leftstringexpr = nullptr;
            templateexpr = nullptr;
            tostrcallexpr = nullptr;
            leftaddexpr = nullptr;
            rightexpr = nullptr;
            rightaddexpr = nullptr;
            processedliteral = processAndCopyString(p->lexer.m_currtoken.tokstrdata, p->lexer.m_currtoken.tokstrlen);
            if(!processedliteral)
            {
                p->m_prserrlist->pushFormat(MC_ERROR_PARSING, p->lexer.m_currtoken.m_tokpos, "error parsing string literal");
                return nullptr;
            }
            p->lexer.nextToken();
            if(!p->lexer.expectCurrent(Token::TOK_LBRACE))
            {
                goto err;
            }
            p->lexer.nextToken();
            pos = p->lexer.m_currtoken.m_tokpos;
            len = mc_util_strlen(processedliteral);
            leftstringexpr = AstExpression::makeliteralstring(processedliteral, len);
            if(!leftstringexpr)
            {
                goto err;
            }
            leftstringexpr->pos = pos;
            processedliteral = nullptr;
            pos = p->lexer.m_currtoken.m_tokpos;
            templateexpr = do_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!templateexpr)
            {
                goto err;
            }
            tostrcallexpr = AstExpression::makeinlinefunccall(templateexpr, "tostring");
            if(!tostrcallexpr)
            {
                goto err;
            }
            tostrcallexpr->pos = pos;
            templateexpr = nullptr;
            leftaddexpr = AstExpression::makeinfix(MC_MATHOP_PLUS, leftstringexpr, tostrcallexpr);
            if(!leftaddexpr)
            {
                goto err;
            }
            leftaddexpr->pos = pos;
            leftstringexpr = nullptr;
            tostrcallexpr = nullptr;
            if(!p->lexer.expectCurrent(Token::TOK_RBRACE))
            {
                goto err;
            }
            p->lexer.previousToken();
            p->lexer.conttplstring();
            p->lexer.nextToken();
            p->lexer.nextToken();
            pos = p->lexer.m_currtoken.m_tokpos;
            rightexpr = do_parseexpression(p, MC_ASTPREC_HIGHEST);
            if(!rightexpr)
            {
                goto err;
            }
            rightaddexpr = AstExpression::makeinfix(MC_MATHOP_PLUS, leftaddexpr, rightexpr);
            if(!rightaddexpr)
            {
                goto err;
            }
            rightaddexpr->pos = pos;
            leftaddexpr = nullptr;
            rightexpr = nullptr;
            return rightaddexpr;
        err:
            AstExpression::destroyExpression(rightaddexpr);
            AstExpression::destroyExpression(rightexpr);
            AstExpression::destroyExpression(leftaddexpr);
            AstExpression::destroyExpression(tostrcallexpr);
            AstExpression::destroyExpression(templateexpr);
            AstExpression::destroyExpression(leftstringexpr);
            mc_memory_free(processedliteral);
            return nullptr;
        }

        static AstExpression* do_parseliteralnull(AstParser* p)
        {
            p->lexer.nextToken();
            return AstExpression::makeliteralnull();
        }

        static AstExpression* do_parseliteralarray(AstParser* p)
        {
            PtrList* array;
            AstExpression* res;
            array = do_parseexprlist(p, Token::TOK_LBRACKET, Token::TOK_RBRACKET, true);
            if(!array)
            {
                return nullptr;
            }
            res = AstExpression::makeliteralarray(array);
            if(!res)
            {
                PtrList::destroy(array, (mcitemdestroyfn_t)AstExpression::destroyExpression);
                return nullptr;
            }
            return res;
        }

        static AstExpression* do_parseliteralmap(AstParser* p)
        {
            bool ok;
            size_t len;
            char* str;
            PtrList* keys;
            PtrList* values;
            AstExpression* res;
            AstExpression* key;
            AstExpression* value;
            (void)ok;
            keys = Memory::make<PtrList>(sizeof(void*), true);
            values = Memory::make<PtrList>(sizeof(void*), true);
            if(!keys || !values)
            {
                goto err;
            }
            p->lexer.nextToken();
            while(!p->lexer.currentTokenIs(Token::TOK_RBRACE))
            {
                key = nullptr;
                if(p->lexer.currentTokenIs(Token::TOK_IDENT))
                {
                    str = p->lexer.m_currtoken.dupLiteralString();
                    len = mc_util_strlen(str);
                    key = AstExpression::makeliteralstring(str, len);
                    if(!key)
                    {
                        mc_memory_free(str);
                        goto err;
                    }
                    key->pos = p->lexer.m_currtoken.m_tokpos;
                    p->lexer.nextToken();
                }
                else
                {
                    key = do_parseexpression(p, MC_ASTPREC_LOWEST);
                    if(!key)
                    {
                        goto err;
                    }
                    switch(key->exprtype)
                    {
                        case AstExpression::EXPR_STRINGLITERAL:
                        case AstExpression::EXPR_NUMBERLITERAL:
                        case AstExpression::EXPR_BOOLLITERAL:
                            {
                            }
                            break;
                        default:
                            {
                                p->m_prserrlist->pushFormat(MC_ERROR_PARSING, key->pos, "can only use primitive types as literal 'map' object keys");
                                AstExpression::destroyExpression(key);
                                goto err;
                            }
                            break;
                    }
                }
                ok = keys->push(key);
                if(!p->lexer.expectCurrent(Token::TOK_COLON))
                {
                    goto err;
                }
                p->lexer.nextToken();
                value = do_parseexpression(p, MC_ASTPREC_LOWEST);
                if(!value)
                {
                    goto err;
                }
                ok = values->push(value);
                if(p->lexer.currentTokenIs(Token::TOK_RBRACE))
                {
                    break;
                }
                if(!p->lexer.expectCurrent(Token::TOK_COMMA))
                {
                    goto err;
                }
                p->lexer.nextToken();
            }
            p->lexer.nextToken();
            res = AstExpression::makeliteralmap(keys, values);
            if(!res)
            {
                goto err;
            }
            return res;
        err:
            PtrList::destroy(keys, (mcitemdestroyfn_t)AstExpression::destroyExpression);
            PtrList::destroy(values, (mcitemdestroyfn_t)AstExpression::destroyExpression);
            return nullptr;
        }

        static AstExpression* do_parseprefixexpr(AstParser* p)
        {
            mcastmathoptype_t op;
            AstExpression* res;
            AstExpression* right;
            op = tokenToMathOP(p->lexer.m_currtoken.toktype);
            p->lexer.nextToken();
            right = do_parseexpression(p, MC_ASTPREC_PREFIX);
            if(!right)
            {
                return nullptr;
            }
            res = AstExpression::makeprefix(op, right);
            if(!res)
            {
                AstExpression::destroyExpression(right);
                return nullptr;
            }
            return res;
        }

        static AstExpression* do_parseinfixexpr(AstParser* p, AstExpression* left)
        {
            mcastmathoptype_t op;
            mcastprecedence_t prec;
            AstExpression* res;
            AstExpression* right;
            op = tokenToMathOP(p->lexer.m_currtoken.toktype);
            prec = getPrecedence(p->lexer.m_currtoken.toktype);
            p->lexer.nextToken();
            right = do_parseexpression(p, prec);
            if(!right)
            {
                return nullptr;
            }
            res = AstExpression::makeinfix(op, left, right);
            if(!res)
            {
                AstExpression::destroyExpression(right);
                return nullptr;
            }
            return res;
        }

        static AstExpression* do_parsegroupedexpr(AstParser* p)
        {
            AstExpression* expr;
            p->lexer.nextToken();
            expr = do_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!expr || !p->lexer.expectCurrent(Token::TOK_RPAREN))
            {
                AstExpression::destroyExpression(expr);
                return nullptr;
            }
            p->lexer.nextToken();
            return expr;
        }


        static bool do_parsefuncparams(AstParser* p, PtrList* outparams)
        {
            bool ok;
            AstExpression::ExprIdent* ident;
            AstExpression::ExprFuncParam* param;
            (void)ok;
            if(!p->lexer.expectCurrent(Token::TOK_LPAREN))
            {
                return false;
            }
            p->lexer.nextToken();
            if(p->lexer.currentTokenIs(Token::TOK_RPAREN))
            {
                p->lexer.nextToken();
                return true;
            }
            if(!p->lexer.expectCurrent(Token::TOK_IDENT))
            {
                return false;
            }
            ident = Memory::make<AstExpression::ExprIdent>(p->lexer.m_currtoken);
            param = Memory::make<AstExpression::ExprFuncParam>(ident);
            ok = outparams->push(param);
            p->lexer.nextToken();
            while(p->lexer.currentTokenIs(Token::TOK_COMMA))
            {
                p->lexer.nextToken();
                if(!p->lexer.expectCurrent(Token::TOK_IDENT))
                {
                    return false;
                }
                ident = Memory::make<AstExpression::ExprIdent>(p->lexer.m_currtoken);
                param = Memory::make<AstExpression::ExprFuncParam>(ident);
                ok = outparams->push(param);
                p->lexer.nextToken();
            }
            if(!p->lexer.expectCurrent(Token::TOK_RPAREN))
            {
                return false;
            }
            p->lexer.nextToken();
            return true;
        }

        static AstExpression* do_parseliteralfunction(AstParser* p)
        {
            bool ok;
            PtrList* params;
            AstExpression::ExprCodeBlock* body;
            AstExpression* res;
            (void)ok;
            p->depth++;
            params = nullptr;
            body = nullptr;
            if(p->lexer.currentTokenIs(Token::TOK_FUNCTION))
            {
                p->lexer.nextToken();
            }
            params = Memory::make<PtrList>(sizeof(void*), true);
            ok = do_parsefuncparams(p, params);
            if(!ok)
            {
                goto err;
            }
            body = do_parsecodeblock(p);
            if(!body)
            {
                goto err;
            }
            res = AstExpression::makeliteralfunction(params, body);
            if(!res)
            {
                goto err;
            }
            p->depth -= 1;
            return res;
        err:
            Memory::destroy(body);
            PtrList::destroy(params, (mcitemdestroyfn_t)AstExpression::ExprFuncParam::destroy);
            p->depth -= 1;
            return nullptr;
        }

        static AstExpression* do_parsefunctionstmt(AstParser* p)
        {
            AstExpression::ExprIdent* nameident;
            AstExpression* res;
            AstExpression* value;
            AstLocation pos;
            nameident = nullptr;
            value = nullptr;
            pos = p->lexer.m_currtoken.m_tokpos;
            p->lexer.nextToken();
            if(!p->lexer.expectCurrent(Token::TOK_IDENT))
            {
                goto err;
            }
            nameident = Memory::make<AstExpression::ExprIdent>(p->lexer.m_currtoken);
            p->lexer.nextToken();
            value = do_parseliteralfunction(p);
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
            res = AstExpression::makedefine(nameident, value, false);
            if(!res)
            {
                goto err;
            }
            return res;
        err:
            AstExpression::destroyExpression(value);
            Memory::destroy(nameident);
            return nullptr;
        }


        static AstExpression* do_parsecallexpr(AstParser* p, AstExpression* left)
        {
            PtrList* args;
            AstExpression* res;
            AstExpression* function;
            function = left;
            args = do_parseexprlist(p, Token::TOK_LPAREN, Token::TOK_RPAREN, false);
            if(!args)
            {
                return nullptr;
            }
            res = AstExpression::makecall(function, args);
            if(!res)
            {
                PtrList::destroy(args, (mcitemdestroyfn_t)AstExpression::destroyExpression);
                return nullptr;
            }
            return res;
        }

        static PtrList* do_parseexprlist(AstParser* p, Token::Type starttoken, Token::Type endtoken, bool trailingcommaallowed)
        {
            bool ok;
            PtrList* res;
            AstExpression* argexpr;
            (void)ok;
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
            argexpr = do_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!argexpr)
            {
                goto err;
            }
            ok = res->push(argexpr);
            while(p->lexer.currentTokenIs(Token::TOK_COMMA))
            {
                p->lexer.nextToken();
                if(trailingcommaallowed && p->lexer.currentTokenIs(endtoken))
                {
                    break;
                }
                argexpr = do_parseexpression(p, MC_ASTPREC_LOWEST);
                if(!argexpr)
                {
                    goto err;
                }
                ok = res->push(argexpr);
            }
            if(!p->lexer.expectCurrent(endtoken))
            {
                goto err;
            }
            p->lexer.nextToken();
            return res;
        err:
            PtrList::destroy(res, (mcitemdestroyfn_t)AstExpression::destroyExpression);
            return nullptr;
        }

        static AstExpression* do_parseindexexpr(AstParser* p, AstExpression* left)
        {
            AstExpression* res;
            AstExpression* index;
            p->lexer.nextToken();
            index = do_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!index)
            {
                return nullptr;
            }
            if(!p->lexer.expectCurrent(Token::TOK_RBRACKET))
            {
                AstExpression::destroyExpression(index);
                return nullptr;
            }
            p->lexer.nextToken();
            res = AstExpression::makeindex(left, index, false);
            if(!res)
            {
                AstExpression::destroyExpression(index);
                return nullptr;
            }
            return res;
        }

        static AstExpression* do_parseassignexpr(AstParser* p, AstExpression* left)
        {
            AstLocation pos;
            mcastmathoptype_t op;
            Token::Type assigntype;
            AstExpression* res;
            AstExpression* source;
            AstExpression* leftcopy;
            AstExpression* newsource;
            source = nullptr;
            assigntype = p->lexer.m_currtoken.toktype;
            p->lexer.nextToken();
            source = do_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!source)
            {
                goto err;
            }
            switch(assigntype)
            {
                case Token::TOK_ASSIGNPLUS:
                case Token::TOK_ASSIGNMINUS:
                case Token::TOK_ASSIGNSLASH:
                case Token::TOK_ASSIGNASTERISK:
                case Token::TOK_ASSIGNPERCENT:
                case Token::TOK_ASSIGNBINAND:
                case Token::TOK_ASSIGNBINOR:
                case Token::TOK_ASSIGNBINXOR:
                case Token::TOK_ASSIGNLSHIFT:
                case Token::TOK_ASSIGNRSHIFT:
                    {
                        op = tokenToMathOP(assigntype);
                        leftcopy = AstExpression::copyExpression(left);
                        if(!leftcopy)
                        {
                            goto err;
                        }
                        pos = source->pos;
                        newsource = AstExpression::makeinfix(op, leftcopy, source);
                        if(!newsource)
                        {
                            AstExpression::destroyExpression(leftcopy);
                            goto err;
                        }
                        newsource->pos = pos;
                        source = newsource;
                    }
                    break;
                case Token::TOK_ASSIGN:
                    {
                    }
                    break;
                default:
                    {
                        MC_ASSERT(false);
                    }
                    break;
            }
            res = AstExpression::makeassign(left, source, false);
            if(!res)
            {
                goto err;
            }
            return res;
        err:
            AstExpression::destroyExpression(source);
            return nullptr;
        }

        static AstExpression* do_parselogicalexpr(AstParser* p, AstExpression* left)
        {
            mcastmathoptype_t op;
            mcastprecedence_t prec;
            AstExpression* res;
            AstExpression* right;
            op = tokenToMathOP(p->lexer.m_currtoken.toktype);
            prec = getPrecedence(p->lexer.m_currtoken.toktype);
            p->lexer.nextToken();
            right = do_parseexpression(p, prec);
            if(!right)
            {
                return nullptr;
            }
            res = AstExpression::makelogical(op, left, right);
            if(!res)
            {
                AstExpression::destroyExpression(right);
                return nullptr;
            }
            return res;
        }

        static AstExpression* do_parseternaryexpr(AstParser* p, AstExpression* left)
        {
            AstExpression* res;
            AstExpression* ift;
            AstExpression* iffalse;
            p->lexer.nextToken();
            ift = do_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!ift)
            {
                return nullptr;
            }
            if(!p->lexer.expectCurrent(Token::TOK_COLON))
            {
                AstExpression::destroyExpression(ift);
                return nullptr;
            }
            p->lexer.nextToken();
            iffalse = do_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!iffalse)
            {
                AstExpression::destroyExpression(ift);
                return nullptr;
            }
            res = AstExpression::maketernary(left, ift, iffalse);
            if(!res)
            {
                AstExpression::destroyExpression(ift);
                AstExpression::destroyExpression(iffalse);
                return nullptr;
            }
            return res;
        }

        static AstExpression* do_parseincdecprefixexpr(AstParser* p)
        {
            AstLocation pos;
            mcastmathoptype_t op;
            Token::Type operationtype;
            AstExpression* res;
            AstExpression* dest;
            AstExpression* source;
            AstExpression* destcopy;
            AstExpression* operation;
            AstExpression* oneliteral;
            source = nullptr;
            operationtype = p->lexer.m_currtoken.toktype;
            pos = p->lexer.m_currtoken.m_tokpos;
            p->lexer.nextToken();
            op = tokenToMathOP(operationtype);
            dest = do_parseexpression(p, MC_ASTPREC_PREFIX);
            if(!dest)
            {
                goto err;
            }
            oneliteral = AstExpression::makeliteralnumber(1);
            if(!oneliteral)
            {
                AstExpression::destroyExpression(dest);
                goto err;
            }
            oneliteral->pos = pos;
            destcopy = AstExpression::copyExpression(dest);
            if(!destcopy)
            {
                AstExpression::destroyExpression(oneliteral);
                AstExpression::destroyExpression(dest);
                goto err;
            }
            operation = AstExpression::makeinfix(op, destcopy, oneliteral);
            if(!operation)
            {
                AstExpression::destroyExpression(destcopy);
                AstExpression::destroyExpression(dest);
                AstExpression::destroyExpression(oneliteral);
                goto err;
            }
            operation->pos = pos;
            res = AstExpression::makeassign(dest, operation, false);
            if(!res)
            {
                AstExpression::destroyExpression(dest);
                AstExpression::destroyExpression(operation);
                goto err;
            }
            return res;
        err:
            AstExpression::destroyExpression(source);
            return nullptr;
        }

        static AstExpression* do_parseincdecpostfixexpr(AstParser* p, AstExpression* left)
        {
            AstLocation pos;
            mcastmathoptype_t op;
            Token::Type operationtype;
            AstExpression* res;
            AstExpression* source;
            AstExpression* leftcopy;
            AstExpression* operation;
            AstExpression* oneliteral;
            source = nullptr;
            operationtype = p->lexer.m_currtoken.toktype;
            pos = p->lexer.m_currtoken.m_tokpos;
            p->lexer.nextToken();
            op = tokenToMathOP(operationtype);
            leftcopy = AstExpression::copyExpression(left);
            if(!leftcopy)
            {
                goto err;
            }
            oneliteral = AstExpression::makeliteralnumber(1);
            if(!oneliteral)
            {
                AstExpression::destroyExpression(leftcopy);
                goto err;
            }
            oneliteral->pos = pos;
            operation = AstExpression::makeinfix(op, leftcopy, oneliteral);
            if(!operation)
            {
                AstExpression::destroyExpression(oneliteral);
                AstExpression::destroyExpression(leftcopy);
                goto err;
            }
            operation->pos = pos;
            res = AstExpression::makeassign(left, operation, true);
            if(!res)
            {
                AstExpression::destroyExpression(operation);
                goto err;
            }
            return res;
        err:
            AstExpression::destroyExpression(source);
            return nullptr;
        }

        static AstExpression* do_parsedotexpression(AstParser* p, AstExpression* left)
        {
            size_t len;
            char* str;
            AstExpression* res;
            AstExpression* index;
            p->lexer.nextToken();
            if(!p->lexer.expectCurrent(Token::TOK_IDENT))
            {
                return nullptr;
            }
            str = p->lexer.m_currtoken.dupLiteralString();
            len = mc_util_strlen(str);
            index = AstExpression::makeliteralstring(str, len);
            if(!index)
            {
                mc_memory_free(str);
                return nullptr;
            }
            index->pos = p->lexer.m_currtoken.m_tokpos;
            p->lexer.nextToken();
            res = AstExpression::makeindex(left, index, true);
            if(!res)
            {
                AstExpression::destroyExpression(index);
                return nullptr;
            }
            return res;
        }

        static void destroy(AstParser* parser)
        {
            if(parser != nullptr)
            {
                mc_memory_free(parser);
            }
        }

    public:
        mcconfig_t* m_config;
        AstLexer lexer;
        ErrList* m_prserrlist;
        mcastrightassocparsefn_t rightassocfuncs[Token::TOK_TYPEMAX];
        mcleftassocparsefn_t leftassocfuncs[Token::TOK_TYPEMAX];
        int depth;

    public:
        AstParser(mcconfig_t* config, ErrList* errors)
        {
            m_config = config;
            m_prserrlist = errors;
            {
                this->rightassocfuncs[Token::TOK_IDENT] = do_parseident;
                this->rightassocfuncs[Token::TOK_NUMBER] = do_parseliteralnumber;
                this->rightassocfuncs[Token::TOK_TRUE] = do_parseliteralbool;
                this->rightassocfuncs[Token::TOK_FALSE] = do_parseliteralbool;
                this->rightassocfuncs[Token::TOK_STRING] = do_parseliteralstring;
                this->rightassocfuncs[Token::TOK_TEMPLATESTRING] = do_parseliteraltemplatestring;
                this->rightassocfuncs[Token::TOK_NULL] = do_parseliteralnull;
                this->rightassocfuncs[Token::TOK_BANG] = do_parseprefixexpr;
                this->rightassocfuncs[Token::TOK_UNARYMINUS] = do_parseprefixexpr;
                this->rightassocfuncs[Token::TOK_UNARYBINNOT] = do_parseprefixexpr;
                this->rightassocfuncs[Token::TOK_LPAREN] = do_parsegroupedexpr;
                this->rightassocfuncs[Token::TOK_FUNCTION] = do_parseliteralfunction;
                this->rightassocfuncs[Token::TOK_LBRACKET] = do_parseliteralarray;
                this->rightassocfuncs[Token::TOK_LBRACE] = do_parseliteralmap;
                this->rightassocfuncs[Token::TOK_PLUSPLUS] = do_parseincdecprefixexpr;
                this->rightassocfuncs[Token::TOK_MINUSMINUS] = do_parseincdecprefixexpr;
                #if 0
                this->rightassocfuncs[Token::TOK_IF] = do_parseifstmt;
                #endif
                #if 1
                this->rightassocfuncs[Token::TOK_RECOVER] = do_parserecoverstmt;
                #endif

            }
            {
                this->leftassocfuncs[Token::TOK_PLUS] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_UNARYMINUS] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_SLASH] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_ASTERISK] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_PERCENT] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_EQ] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_NOTEQ] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_LT] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_LTE] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_GT] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_GTE] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_LPAREN] = do_parsecallexpr;
                this->leftassocfuncs[Token::TOK_LBRACKET] = do_parseindexexpr;
                this->leftassocfuncs[Token::TOK_ASSIGN] = do_parseassignexpr;
                this->leftassocfuncs[Token::TOK_ASSIGNPLUS] = do_parseassignexpr;
                this->leftassocfuncs[Token::TOK_ASSIGNMINUS] = do_parseassignexpr;
                this->leftassocfuncs[Token::TOK_ASSIGNSLASH] = do_parseassignexpr;
                this->leftassocfuncs[Token::TOK_ASSIGNASTERISK] = do_parseassignexpr;
                this->leftassocfuncs[Token::TOK_ASSIGNPERCENT] = do_parseassignexpr;
                this->leftassocfuncs[Token::TOK_ASSIGNBINAND] = do_parseassignexpr;
                this->leftassocfuncs[Token::TOK_ASSIGNBINOR] = do_parseassignexpr;
                this->leftassocfuncs[Token::TOK_ASSIGNBINXOR] = do_parseassignexpr;
                this->leftassocfuncs[Token::TOK_ASSIGNLSHIFT] = do_parseassignexpr;
                this->leftassocfuncs[Token::TOK_ASSIGNRSHIFT] = do_parseassignexpr;
                this->leftassocfuncs[Token::TOK_DOT] = do_parsedotexpression;
                this->leftassocfuncs[Token::TOK_AND] = do_parselogicalexpr;
                this->leftassocfuncs[Token::TOK_OR] = do_parselogicalexpr;
                this->leftassocfuncs[Token::TOK_BINAND] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_BINOR] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_BINXOR] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_LSHIFT] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_RSHIFT] = do_parseinfixexpr;
                this->leftassocfuncs[Token::TOK_QUESTION] = do_parseternaryexpr;
                this->leftassocfuncs[Token::TOK_PLUSPLUS] = do_parseincdecpostfixexpr;
                this->leftassocfuncs[Token::TOK_MINUSMINUS] = do_parseincdecpostfixexpr;
            }
            this->depth = 0;
        }

        AstExpression* parseStatement()
        {
            AstLocation pos;
            AstExpression* res;
            pos = this->lexer.m_currtoken.m_tokpos;
            res = nullptr;
            switch(this->lexer.m_currtoken.toktype)
            {
                case Token::TOK_VAR:
                case Token::TOK_CONST:
                    {
                        res = do_parsevarletstmt(this);
                    }
                    break;
                case Token::TOK_IF:
                    {
                        res = do_parseifstmt(this);
                    }
                    break;
                case Token::TOK_RETURN:
                    {
                        res = do_parsereturnstmt(this);
                    }
                    break;
                case Token::TOK_WHILE:
                    {
                        res = do_parseloopwhilestmt(this);
                    }
                    break;
                case Token::TOK_BREAK:
                    {
                        res = do_parsebreakstmt(this);
                    }
                    break;
                case Token::TOK_FOR:
                    {
                        res = do_parseloopforloopstmt(this);
                    }
                    break;
                case Token::TOK_FUNCTION:
                    {
                        if(this->lexer.peekTokenIs(Token::TOK_IDENT))
                        {
                            res = do_parsefunctionstmt(this);
                        }
                        else
                        {
                            res = do_parseexprstmt(this);
                        }
                    }
                    break;
                case Token::TOK_LBRACE:
                    {
                        if(m_config->replmode && this->depth == 0)
                        {
                            res = do_parseexprstmt(this);
                        }
                        else
                        {
                            res = do_parseblockstmt(this);
                        }
                    }
                    break;
                case Token::TOK_CONTINUE:
                    {
                        res = do_parsecontinuestmt(this);
                    }
                    break;
                case Token::TOK_IMPORT:
                    {
                        res = do_parseimportstmt(this);
                    }
                    break;
                case Token::TOK_RECOVER:
                    {
                        res = do_parserecoverstmt(this);
                    }
                    break;
                default:
                    {
                        res = do_parseexprstmt(this);
                    }
                    break;
            }
            if(res)
            {
                res->pos = pos;
            }
            return res;
        }

        PtrList* parseAll(const char* input, AstSourceFile* file)
        {
            bool ok;
            AstExpression* expr;
            PtrList* statements;
            (void)ok;
            this->depth = 0;
            ok = AstLexer::init(&this->lexer, m_prserrlist, input, file);
            if(!ok)
            {
                return nullptr;
            }
            this->lexer.nextToken();
            this->lexer.nextToken();
            statements = Memory::make<PtrList>(sizeof(void*), true);
            while(!this->lexer.currentTokenIs(Token::TOK_EOF))
            {
                if(this->lexer.currentTokenIs(Token::TOK_SEMICOLON))
                {
                    this->lexer.nextToken();
                    continue;
                }
                expr = parseStatement();
                if(!expr)
                {
                    goto err;
                }
                ok = statements->push(expr);
            }
            if(m_prserrlist->count() > 0)
            {
                goto err;
            }
            return statements;
        err:
            PtrList::destroy(statements, (mcitemdestroyfn_t)AstExpression::destroyExpression);
            return nullptr;
        }


};

class AstPrinter
{
    public:
        Printer* m_pdest;
        bool m_pseudolisp;

    public:
        AstPrinter(Printer* pr)
        {
            m_pseudolisp = false;
            m_pdest = pr;
        }

        void beginPrint(PtrList* statements)
        {
            m_pdest->m_prconfig.quotstring = true;
            fprintf(stderr, "---AST dump begin---\n");
            this->printStmtList(statements);
            fprintf(stderr, "\n---AST dump end---\n");
            m_pdest->m_prconfig.quotstring = false;
        }

        void printStmtList(PtrList* statements)
        {
            int i;
            int count;
            AstExpression* subex;
            count = statements->count();
            for(i = 0; i < count; i++)
            {
                subex = (AstExpression*)statements->get(i);
                this->printExpression(subex);
                if(i < (count - 1))
                {
                    m_pdest->put("\n");
                }
            }
        }

        void printFuncLiteral(AstExpression* astexpr)
        {
            size_t i;
            AstExpression::ExprFuncParam* param;
            AstExpression::ExprLiteralFunction* ex;
            ex = &astexpr->uexpr.exprlitfunction;
            if(m_pseudolisp)
            {
                m_pdest->format("(deffunction '(");
            }
            else
            {
                m_pdest->put("function(");
            }
            for(i = 0; i < ex->funcparamlist->count(); i++)
            {
                param = (AstExpression::ExprFuncParam*)ex->funcparamlist->get(i);
                m_pdest->put(param->ident->value);
                if(i < (ex->funcparamlist->count() - 1))
                {
                    m_pdest->put(", ");
                }
            }
            m_pdest->put(") ");
            printCodeblock(ex->body);
        }

        void printCall(AstExpression* astexpr)
        {
            size_t i;
            AstExpression::ExprCall* ex;
            AstExpression* arg;
            ex = &astexpr->uexpr.exprcall;
            this->printExpression(ex->function);
            m_pdest->put("(");
            for(i = 0; i < ex->args->count(); i++)
            {
                arg = (AstExpression*)ex->args->get(i);
                this->printExpression(arg);
                if(i < (ex->args->count() - 1))
                {
                    m_pdest->put(", ");
                }
            }
            m_pdest->put(")");
        }

        void printArrayLiteral(AstExpression* astexpr)
        {
            size_t i;
            size_t len;
            AstExpression::ExprLiteralArray* ex;
            AstExpression* itemex;
            PtrList* vl;
            ex = &astexpr->uexpr.exprlitarray;
            vl = ex->litarritems;
            len = vl->count();
            m_pdest->put("[");
            for(i = 0; i < len; i++)
            {
                itemex = (AstExpression*)vl->get(i);
                this->printExpression(itemex);
                if(i < (len - 1))
                {
                    m_pdest->put(", ");
                }
            }
            m_pdest->put("]");
        }

        void printStringLiteral(AstExpression* astexpr)
        {
            size_t slen;
            const char* sdata;
            AstExpression::ExprLiteralString* ex;
            ex = &astexpr->uexpr.exprlitstring;
            sdata = ex->data;
            slen = ex->length;
            if(m_pdest->m_prconfig.quotstring)
            {
                m_pdest->printEscapedString(sdata, slen);
            }
            else
            {
                m_pdest->put(sdata, slen);
            }
        }

        void printMapLiteral(AstExpression* astexpr)
        {
            size_t i;
            AstExpression* keyexpr;
            AstExpression* valexpr;
            AstExpression::ExprLiteralMap* ex;
            ex = &astexpr->uexpr.exprlitmap;
            m_pdest->put("{");
            for(i = 0; i < ex->litmapkeys->count(); i++)
            {
                keyexpr = (AstExpression*)ex->litmapkeys->get(i);
                valexpr = (AstExpression*)ex->litmapvalues->get(i);
                this->printExpression(keyexpr);
                m_pdest->put(" : ");
                this->printExpression(valexpr);
                if(i < (ex->litmapkeys->count() - 1))
                {
                    m_pdest->put(", ");
                }
            }
            m_pdest->put("}");
        }

        void printPrefix(AstExpression* astexpr)
        {
            AstExpression::ExprPrefix* ex;
            ex = &astexpr->uexpr.exprprefix;
            m_pdest->put("(");
            m_pdest->put(mc_util_mathopstring(ex->op));
            this->printExpression(ex->right);
            m_pdest->put(")");
        }

        void printInfix(AstExpression* astexpr)
        {
            AstExpression::ExprInfix* ex;
            ex = &astexpr->uexpr.exprinfix;
            m_pdest->put("(");
            this->printExpression(ex->left);
            m_pdest->put(" ");
            m_pdest->put(mc_util_mathopstring(ex->op));
            m_pdest->put(" ");
            this->printExpression(ex->right);
            m_pdest->put(")");
        }

        void printIndex(AstExpression* astexpr)
        {
            bool prevquot;
            AstExpression::ExprIndex* ex;
            ex = &astexpr->uexpr.exprindex;
            m_pdest->put("(");
            this->printExpression(ex->left);
            if(ex->isdot)
            {
                m_pdest->put(".");
                prevquot = m_pdest->m_prconfig.quotstring;
                m_pdest->m_prconfig.quotstring = false;
                this->printExpression(ex->index);
                m_pdest->m_prconfig.quotstring = prevquot;
            }
            else
            {
                m_pdest->put("[");
                this->printExpression(ex->index);
                m_pdest->put("]");
            }
            m_pdest->put(")");
        }

        void printAssign(AstExpression* astexpr)
        {
            AstExpression::ExprAssign* ex;
            ex = &astexpr->uexpr.exprassign;
            this->printExpression(ex->dest);
            m_pdest->put(" = ");
            this->printExpression(ex->source);
        }

        void printLogical(AstExpression* astexpr)
        {
            AstExpression::ExprLogical* ex;
            ex = &astexpr->uexpr.exprlogical;
            this->printExpression(ex->left);
            m_pdest->put(" ");
            m_pdest->put(mc_util_mathopstring(ex->op));
            m_pdest->put(" ");
            this->printExpression(ex->right);
        }

        void printTernary(AstExpression* astexpr)
        {
            AstExpression::ExprTernary* ex;
            ex = &astexpr->uexpr.exprternary;
            this->printExpression(ex->tercond);
            m_pdest->put(" ? ");
            this->printExpression(ex->teriftrue);
            m_pdest->put(" : ");
            this->printExpression(ex->teriffalse);
        }

        void printDefine(AstExpression* astexpr)
        {
            AstExpression::ExprDefine* ex;
            ex = &astexpr->uexpr.exprdefine;
            if(ex->assignable)
            {
                m_pdest->put("var ");
            }
            else
            {
                m_pdest->put("const ");
            }
            m_pdest->put(ex->name->value);
            m_pdest->put(" = ");
            if(ex->value)
            {
                this->printExpression(ex->value);
            }
        }

        void printIf(AstExpression* astexpr)
        {
            size_t i;
            AstExpression::ExprIfCase* ifcase;
            AstExpression::ExprIfStmt* ex;
            ex = &astexpr->uexpr.exprifstmt;
            ifcase = (AstExpression::ExprIfCase*)ex->cases->get(0);
            m_pdest->put("if (");
            this->printExpression(ifcase->ifcond);
            m_pdest->put(") ");
            printCodeblock(ifcase->consequence);
            for(i = 1; i < ex->cases->count(); i++)
            {
                AstExpression::ExprIfCase* elifcase = (AstExpression::ExprIfCase*)ex->cases->get(i);
                m_pdest->put(" elif (");
                this->printExpression(elifcase->ifcond);
                m_pdest->put(") ");
                printCodeblock(elifcase->consequence);
            }
            if(ex->alternative)
            {
                m_pdest->put(" else ");
                printCodeblock(ex->alternative);
            }
        }

        void printWhile(AstExpression* astexpr)
        {
            AstExpression::ExprWhileStmt* ex;
            ex = &astexpr->uexpr.exprwhileloopstmt;
            m_pdest->put("while (");
            this->printExpression(ex->loopcond);
            m_pdest->put(")");
            printCodeblock(ex->body);
        }

        void printForClassic(AstExpression* astexpr)
        {
            AstExpression::ExprLoopStmt* ex;
            ex = &astexpr->uexpr.exprforloopstmt;
            m_pdest->put("for (");
            if(ex->init)
            {
                this->printExpression(ex->init);
                m_pdest->put(" ");
            }
            else
            {
                m_pdest->put(";");
            }
            if(ex->loopcond)
            {
                this->printExpression(ex->loopcond);
                m_pdest->put("; ");
            }
            else
            {
                m_pdest->put(";");
            }
            if(ex->update)
            {
                this->printExpression(ex->update);
            }
            m_pdest->put(")");
            printCodeblock(ex->body);
        }

        void printForeach(AstExpression* astexpr)
        {
            AstExpression::ExprForeachStmt* ex;
            ex = &astexpr->uexpr.exprforeachloopstmt;
            m_pdest->put("for (");
            m_pdest->format("%s", ex->iterator->value);
            m_pdest->put(" in ");
            this->printExpression(ex->source);
            m_pdest->put(")");
            printCodeblock(ex->body);
        }

        void printImport(AstExpression* astexpr)
        {
            AstExpression::ExprImportStmt* ex;
            ex = &astexpr->uexpr.exprimportstmt;
            m_pdest->format("import \"%s\"", ex->path);
        }

        void printRecover(AstExpression* astexpr)
        {
            AstExpression::ExprRecover* ex;
            ex = &astexpr->uexpr.exprrecoverstmt;
            m_pdest->format("recover (%s)", ex->errident->value);
            printCodeblock(ex->body);
        }

        void printCodeblock(AstExpression::ExprCodeBlock* blockexpr)
        {
            size_t i;
            size_t cnt;
            AstExpression* istmt;
            cnt = blockexpr->statements->count();
            m_pdest->put("{ ");
            for(i = 0; i < cnt; i++)
            {
                istmt = (AstExpression*)blockexpr->statements->get(i);
                this->printExpression(istmt);
                m_pdest->put("\n");
            }
            m_pdest->put(" }");
        }

        void printExpression(AstExpression* astexpr)
        {
            switch(astexpr->exprtype)
            {
                case AstExpression::EXPR_IDENT:
                    {
                        AstExpression::ExprIdent* ex;
                        ex = astexpr->uexpr.exprident;
                        m_pdest->put(ex->value);
                    }
                    break;
                case AstExpression::EXPR_NUMBERLITERAL:
                    {
                        mcfloat_t fl;
                        fl = astexpr->uexpr.exprlitnumber;
                        m_pdest->format("%1.17g", fl);
                    }
                    break;
                case AstExpression::EXPR_BOOLLITERAL:
                    {
                        bool bl;
                        bl = astexpr->uexpr.exprlitbool;
                        m_pdest->format("%s", bl ? "true" : "false");
                    }
                    break;
                case AstExpression::EXPR_STRINGLITERAL:
                    {
                        this->printStringLiteral(astexpr);
                    }
                    break;
                case AstExpression::EXPR_NULLLITERAL:
                    {
                        m_pdest->put("null");
                    }
                    break;
                case AstExpression::EXPR_ARRAYLITERAL:
                    {
                        this->printArrayLiteral(astexpr);
                    }
                    break;
                case AstExpression::EXPR_MAPLITERAL:
                    {
                        this->printMapLiteral(astexpr);
                    }
                    break;
                case AstExpression::EXPR_PREFIX:
                    {
                        this->printPrefix(astexpr);
                    }
                    break;
                case AstExpression::EXPR_INFIX:
                    {
                        this->printInfix(astexpr);
                    }
                    break;
                case AstExpression::EXPR_FUNCTIONLITERAL:
                    {
                        this->printFuncLiteral(astexpr);
                    }
                    break;
                case AstExpression::EXPR_CALL:
                    {
                        this->printCall(astexpr);
                    }
                    break;
                case AstExpression::EXPR_INDEX:
                    {
                        this->printIndex(astexpr);
                    }
                    break;
                case AstExpression::EXPR_ASSIGN:
                    {
                        this->printAssign(astexpr);
                    }
                    break;
                case AstExpression::EXPR_LOGICAL:
                    {
                        printLogical(astexpr);
                    }
                    break;
                case AstExpression::EXPR_TERNARY:
                    {
                        printTernary(astexpr);
                    }
                    break;
                case AstExpression::EXPR_STMTDEFINE:
                    {
                        printDefine(astexpr);
                    }
                    break;
                case AstExpression::EXPR_STMTIF:
                    {
                        printIf(astexpr);
                    }
                    break;
                case AstExpression::EXPR_STMTRETURN:
                    {
                        AstExpression* ex;
                        ex = astexpr->uexpr.exprreturnvalue;
                        if(ex)
                        {
                            m_pdest->put("return ");
                            this->printExpression(ex);
                            m_pdest->put(";");
                        }
                        else
                        {
                            m_pdest->put("return;");
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTEXPRESSION:
                    {
                        AstExpression* ex;
                        ex = astexpr->uexpr.exprexpression;
                        if(ex)
                        {
                            this->printExpression(ex);
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTLOOPWHILE:
                    {
                        printWhile(astexpr);
                    }
                    break;
                case AstExpression::EXPR_STMTLOOPFORCLASSIC:
                    {
                        printForClassic(astexpr);
                    }
                    break;
                case AstExpression::EXPR_STMTLOOPFOREACH:
                    {
                        printForeach(astexpr);
                    }
                    break;
                case AstExpression::EXPR_STMTBLOCK:
                    {
                        AstExpression::ExprCodeBlock* ex;
                        ex = astexpr->uexpr.exprblockstmt;
                        printCodeblock(ex);
                    }
                    break;
                case AstExpression::EXPR_STMTBREAK:
                    {
                        m_pdest->put("break");
                    }
                    break;
                case AstExpression::EXPR_STMTCONTINUE:
                    {
                        m_pdest->put("continue");
                    }
                    break;
                case AstExpression::EXPR_STMTIMPORT:
                    {
                        printImport(astexpr);
                    }
                    break;
                case AstExpression::EXPR_NONE:
                    {
                        m_pdest->put("AstExpression::EXPR_NONE");
                    }
                    break;
                case AstExpression::EXPR_STMTRECOVER:
                    {
                        printRecover(astexpr);
                    }
                    break;
                default:
                    break;
            }
        }

};

class Module
{
    public:
        static const char* getModuleName(const char* strpath)
        {
            const char* lastslashpos;
            lastslashpos = strrchr(strpath, '/');
            if(lastslashpos)
            {
                return lastslashpos + 1;
            }
            return strpath;
        }

        static const char* findFile(const char* filename)
        {
            return filename;
        }

        static void destroy(Module* module)
        {
            if(module != nullptr)
            {
                mc_memory_free(module->name);
                PtrList::destroy(module->modsymbols, (mcitemdestroyfn_t)AstSymbol::destroy);
                mc_memory_free(module);
            }
        }

        static Module* copy(Module* src)
        {
            auto modsyms = src->modsymbols->copy((mcitemcopyfn_t)AstSymbol::copy, (mcitemdestroyfn_t)AstSymbol::destroy);
            return Memory::make<Module>(src->name, modsyms);
        }


    public:
        char* name;
        PtrList* modsymbols;

    public:
        Module(const char* nm): Module(nm, nullptr)
        {
        }

        Module(const char* nm, PtrList* ms)
        {
            this->name = mc_util_strdup(nm);
            MC_ASSERT(this->name);
            if(ms != nullptr)
            {
                this->modsymbols = ms;
            }
            else
            {
                this->modsymbols = Memory::make<PtrList>(sizeof(void*), true);
            }
            MC_ASSERT(this->modsymbols);
        }

        bool addSymbol(AstSymbol* symbol)
        {
            bool ok;
            AstSymbol* modulesymbol;
            Printer* namebuf;
            (void)ok;
            namebuf = Memory::make<Printer>(nullptr);
            ok = namebuf->format("%s::%s", this->name, symbol->name);
            modulesymbol = Memory::make<AstSymbol>(namebuf->getString(), MC_SYM_MODULEGLOBAL, symbol->index, false);
            Printer::destroy(namebuf);
            if(!modulesymbol)
            {
                return false;
            }
            ok = this->modsymbols->push(modulesymbol);
            return true;
        }
};


class AstOptimizer
{
    public:
        static AstExpression* optimizeExpr(AstExpression* expr)
        {
            switch(expr->exprtype)
            {
                case AstExpression::EXPR_INFIX:
                    return optimizeInfix(expr);
                case AstExpression::EXPR_PREFIX:
                    return optimizePrefix(expr);
                default:
                    break;
            }
            return nullptr;
        }

        static AstExpression* optimizeInfix(AstExpression* expr)
        {
            bool leftisnumeric;
            bool rightisnumeric;
            bool leftisstring;
            bool rightisstring;
            mcfloat_t dnleft;
            mcfloat_t dnright;
            size_t len;
            AstExpression* res;
            AstExpression* left;
            AstExpression* right;
            AstExpression* leftoptimized;
            AstExpression* rightoptimized;
            left = expr->uexpr.exprinfix.left;
            leftoptimized = optimizeExpr(left);
            if(leftoptimized)
            {
                left = leftoptimized;
            }
            right = expr->uexpr.exprinfix.right;
            rightoptimized = optimizeExpr(right);
            if(rightoptimized)
            {
                right = rightoptimized;
            }
            res = nullptr;
            leftisnumeric = left->exprtype == AstExpression::EXPR_NUMBERLITERAL || left->exprtype == AstExpression::EXPR_BOOLLITERAL;
            rightisnumeric = right->exprtype == AstExpression::EXPR_NUMBERLITERAL || right->exprtype == AstExpression::EXPR_BOOLLITERAL;
            leftisstring = left->exprtype == AstExpression::EXPR_STRINGLITERAL;
            rightisstring = right->exprtype == AstExpression::EXPR_STRINGLITERAL;
            if(leftisnumeric && rightisnumeric)
            {
                dnleft = left->exprtype == AstExpression::EXPR_NUMBERLITERAL ? left->uexpr.exprlitnumber : left->uexpr.exprlitbool;
                dnright = right->exprtype == AstExpression::EXPR_NUMBERLITERAL ? right->uexpr.exprlitnumber : right->uexpr.exprlitbool;
                switch(expr->uexpr.exprinfix.op)
                {
                    case MC_MATHOP_PLUS:
                        {
                            res = AstExpression::makeliteralnumber(mc_mathutil_add(dnleft, dnright));
                        }
                        break;
                    case MC_MATHOP_MINUS:
                        {
                            res = AstExpression::makeliteralnumber(mc_mathutil_sub(dnleft, dnright));
                        }
                        break;
                    case MC_MATHOP_ASTERISK:
                        {
                            res = AstExpression::makeliteralnumber(mc_mathutil_mult(dnleft, dnright));
                        }
                        break;
                    case MC_MATHOP_SLASH:
                        {
                            res = AstExpression::makeliteralnumber(mc_mathutil_div(dnleft, dnright));
                        }
                        break;
                    case MC_MATHOP_LT:
                        {
                            res = AstExpression::makeliteralbool(dnleft < dnright);
                        }
                        break;
                    case MC_MATHOP_LTE:
                        {
                            res = AstExpression::makeliteralbool(dnleft <= dnright);
                        }
                        break;
                    case MC_MATHOP_GT:
                        {
                            res = AstExpression::makeliteralbool(dnleft > dnright);
                        }
                        break;
                    case MC_MATHOP_GTE:
                        {
                            res = AstExpression::makeliteralbool(dnleft >= dnright);
                        }
                        break;
                    case MC_MATHOP_EQ:
                        {
                            res = AstExpression::makeliteralbool(MC_UTIL_CMPFLOAT(dnleft, dnright));
                        }
                        break;
                    case MC_MATHOP_NOTEQ:
                        {
                            res = AstExpression::makeliteralbool(!MC_UTIL_CMPFLOAT(dnleft, dnright));
                        }
                        break;
                    case MC_MATHOP_MODULUS:
                        {
                            res = AstExpression::makeliteralnumber(mc_mathutil_mod(dnleft, dnright));
                        }
                        break;
                    case MC_MATHOP_BINAND:
                        {
                            res = AstExpression::makeliteralnumber(mc_mathutil_binand(dnleft, dnright));
                        }
                        break;
                    case MC_MATHOP_BINOR:
                        {
                            res = AstExpression::makeliteralnumber(mc_mathutil_binor(dnleft, dnright));
                        }
                        break;
                    case MC_MATHOP_BINXOR:
                        {
                            res = AstExpression::makeliteralnumber(mc_mathutil_binxor(dnleft, dnright));
                        }
                        break;
                    case MC_MATHOP_LSHIFT:
                        {
                            res = AstExpression::makeliteralnumber(mc_mathutil_binshiftleft(dnleft, dnright));
                        }
                        break;
                    case MC_MATHOP_RSHIFT:
                        {
                            res = AstExpression::makeliteralnumber(mc_mathutil_binshiftright(dnleft, dnright));
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
                    res = AstExpression::makeliteralstring(resstr, len);
                    if(!res)
                    {
                        mc_memory_free(resstr);
                    }
                }
            }
            AstExpression::destroyExpression(leftoptimized);
            AstExpression::destroyExpression(rightoptimized);
            if(res)
            {
                res->pos = expr->pos;
            }
            return res;
        }

        static AstExpression* optimizePrefix(AstExpression* expr)
        {
            AstExpression* res;
            AstExpression* right;
            AstExpression* rightoptimized;
            right = expr->uexpr.exprprefix.right;
            rightoptimized = optimizeExpr(right);
            if(rightoptimized)
            {
                right = rightoptimized;
            }
            res = nullptr;
            if(expr->uexpr.exprprefix.op == MC_MATHOP_MINUS && right->exprtype == AstExpression::EXPR_NUMBERLITERAL)
            {
                res = AstExpression::makeliteralnumber(-right->uexpr.exprlitnumber);
            }
            else if(expr->uexpr.exprprefix.op == MC_MATHOP_BANG && right->exprtype == AstExpression::EXPR_BOOLLITERAL)
            {
                res = AstExpression::makeliteralbool(!right->uexpr.exprlitbool);
            }
            AstExpression::destroyExpression(rightoptimized);
            if(res)
            {
                res->pos = expr->pos;
            }
            return res;
        }
};

class AstCompiler
{
    public:
        static void destroy(AstCompiler* comp)
        {
            if(comp != nullptr)
            {
                comp->deinit();
                mc_memory_free(comp);
            }
        }

        static mcopdefinition_t* makeOpDef(mcopdefinition_t* dest, const char* name, int numop, int opa1, int opa2)
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
                return makeOpDef(def, name, opnum, opa1, opa2); \
            } \
            break;

        static mcopdefinition_t* opdefLookup(mcopdefinition_t* def, mcinternopcode_t op)
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

        static const char* opdefGetName(mcinternopcode_t op)
        {
            mcopdefinition_t def;
            return opdefLookup(&def, op)->name;
        }

        static bool initShallowCopy(AstCompiler* copy, AstCompiler* src)
        {
            bool ok;
            size_t i;
            int* val;
            int* valcopy;
            char* loadednamecopy;
            const char* key;
            const char* loadedname;
            PtrDict* modulescopy;
            GenericList<Value>* constantscopy;
            PtrList* srcloadedmodulenames;
            PtrList* copyloadedmodulenames;
            AstSymTable* srcst;
            AstSymTable* srcstocopy;
            AstSymTable* copyst;
            AstScopeFile* srcfilescope;
            AstScopeFile* copyfilescope;
            (void)ok;
            ok = copy->initBase(src->m_pstate, src->m_config, src->m_astmem, src->m_ccerrlist, src->m_files, src->m_compglobalstore, src->m_filestderr);
            if(!ok)
            {
                return false;
            }
            srcst = src->getsymtable();
            //MC_ASSERT(src->m_filescopelist->count() == 1);
            MC_ASSERT(srcst->outer == nullptr);
            srcstocopy = srcst->copy();
            if(!srcstocopy)
            {
                goto compilercopyfailed;
            }
            copyst = copy->getsymtable();
            AstSymTable::destroy(copyst);
            copyst = nullptr;
            copy->setsymtable(srcstocopy);
            modulescopy = src->m_modules->copy();
            if(!modulescopy)
            {
                goto compilercopyfailed;
            }
            PtrDict::destroyItemsAndDict(copy->m_modules);
            copy->m_modules = modulescopy;
            constantscopy = GenericList<Value>::copy(src->m_constants);
            if(!constantscopy)
            {
                goto compilercopyfailed;
            }
            Memory::destroy(copy->m_constants);
            copy->m_constants = constantscopy;
            for(i = 0; i < src->m_stringconstposdict->count(); i++)
            {
                key = src->m_stringconstposdict->getKeyAt(i);
                val = (int*)src->m_stringconstposdict->getValueAt(i);
                valcopy = (int*)mc_memory_malloc(sizeof(int));
                if(!valcopy)
                {
                    goto compilercopyfailed;
                }
                *valcopy = *val;
                ok = copy->m_stringconstposdict->set(key, valcopy);
            }
            srcfilescope = (AstScopeFile*)src->m_filescopelist->top();
            copyfilescope = (AstScopeFile*)copy->m_filescopelist->top();
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
            }
            return true;
        compilercopyfailed:
            copy->deinit();
            return false;
        }


    public:
        State* m_pstate = nullptr;
        mcconfig_t* m_config = nullptr;
        GCMemory* m_astmem = nullptr;
        ErrList* m_ccerrlist = nullptr;
        PtrList* m_files = nullptr;
        SymStore* m_compglobalstore = nullptr;
        GenericList<Value>* m_constants = nullptr;
        AstScopeComp* m_compilationscope = nullptr;
        PtrList* m_filescopelist = nullptr;
        PtrList* m_srcposstack = nullptr;
        PtrDict* m_modules = nullptr;
        PtrDict* m_stringconstposdict = nullptr;
        Printer* m_filestderr;

    public:
        AstCompiler()
        {
        }

        AstCompiler(State* state, mcconfig_t* config, GCMemory* gcmem, ErrList* errors, PtrList* files, SymStore* gstore, Printer* fstderr)
        {
            bool ok;
            (void)ok;
            ok = this->initBase(state, config, gcmem, errors, files, gstore, fstderr);
            MC_ASSERT(ok);
            m_pstate = state; 
        }

        bool initBase(State* state, mcconfig_t* cfg, GCMemory* gcmem, ErrList* errors, PtrList* files, SymStore* gstor, Printer* fstderr)
        {
            bool ok;
            const char* filename;
            (void)ok;
            m_pstate = state;
            m_config = cfg;
            m_astmem = gcmem;
            m_ccerrlist = errors;
            m_files = files;
            m_compglobalstore = gstor;
            m_filestderr = fstderr;
            m_filescopelist = Memory::make<PtrList>(sizeof(void*), true);
            m_constants = Memory::make<GenericList<Value>>(0, Value::makeNull());
            m_srcposstack = Memory::make<PtrList>(sizeof(AstLocation), false);
            m_modules = Memory::make<PtrDict>((mcitemcopyfn_t)Module::copy, (mcitemdestroyfn_t)Module::destroy);
            ok = this->pushCompilationScope();
            filename = "<none>";
            if(files->count() > 0)
            {
                filename = (const char*)files->top();
            }
            #if 1
            ok = this->filescopepush(filename);
            #endif
            m_stringconstposdict = Memory::make<PtrDict>(nullptr, nullptr);
            return true;
        }

        void deinit()
        {
            size_t i;
            int* val;
            for(i = 0; i < m_stringconstposdict->count(); i++)
            {
                val = (int*)m_stringconstposdict->getValueAt(i);
                mc_memory_free(val);
            }
            Memory::destroy(m_stringconstposdict);
            while(m_filescopelist->count() > 0)
            {
                this->filescopepop();
            }
            while(this->getCompilationScope())
            {
                this->popCompilationScope();
            }
            PtrDict::destroyItemsAndDict(m_modules);
            PtrList::destroy(m_srcposstack, nullptr);
            Memory::destroy(m_constants);
            PtrList::destroy(m_filescopelist, nullptr);
        }

        void appendByteAt(PtrList* res, const uint64_t* operands, int i, int n)
        {
            uint16_t val;
            val = (uint16_t)(operands[i] >> (n * 8));
            res->push(&val);
        }

        int genCode(mcinternopcode_t op, int operandscount, const uint64_t* operands, PtrList* res)
        {
            bool ok;
            int i;
            int width;
            int instrlen;
            uint16_t val;
            mcopdefinition_t vdef;
            mcopdefinition_t* def;
            (void)ok;
            def = opdefLookup(&vdef, op);
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
            for(i = 0; i < operandscount; i++)
            {
                width = def->operandwidths[i];
                switch(width)
                {
                    case 1:
                        {
                            appendByteAt(res, operands, i, 0);
                        }
                        break;
                    case 2:
                        {
                            appendByteAt(res, operands, i, 1);
                            appendByteAt(res, operands, i, 0);
                        }
                        break;
                    case 4:
                        {
                            appendByteAt(res, operands, i, 3);
                            appendByteAt(res, operands, i, 2);
                            appendByteAt(res, operands, i, 1);
                            appendByteAt(res, operands, i, 0);
                        }
                        break;
                    case 8:
                        {
                            appendByteAt(res, operands, i, 7);
                            appendByteAt(res, operands, i, 6);
                            appendByteAt(res, operands, i, 5);
                            appendByteAt(res, operands, i, 4);
                            appendByteAt(res, operands, i, 3);
                            appendByteAt(res, operands, i, 2);
                            appendByteAt(res, operands, i, 1);
                            appendByteAt(res, operands, i, 0);
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

        int emitOpCode(mcinternopcode_t op, int operandscount, uint64_t* operands)
        {
            bool ok;
            int i;
            int ip;
            int len;
            AstLocation* srcpos;
            AstScopeComp* compscope;
            (void)ok;
            ip = this->getip();
            len = genCode(op, operandscount, operands, this->getbytecode());
            if(len == 0)
            {
                return -1;
            }
            for(i = 0; i < len; i++)
            {
                srcpos = (AstLocation*)this->m_srcposstack->top();
                /*
                MC_ASSERT(srcpos->line >= 0);
                MC_ASSERT(srcpos->column >= 0);
                */
                ok = this->getsrcpositions()->push(srcpos);
            }
            compscope = this->getCompilationScope();
            compscope->lastopcode = op;
            return ip;
        }

        AstScopeComp* getCompilationScope()
        {
            return this->m_compilationscope;
        }

        bool pushCompilationScope()
        {
            AstScopeComp* nscope;
            AstScopeComp* currentscope;
            currentscope = this->getCompilationScope();
            nscope = Memory::make<AstScopeComp>(currentscope);
            this->setcompilationscope(nscope);
            return true;
        }

        void popCompilationScope()
        {
            AstScopeComp* currentscope;
            currentscope = this->getCompilationScope();
            MC_ASSERT(currentscope);
            this->setcompilationscope(currentscope->outer);
            Memory::destroy(currentscope);
        }

        bool pushSymtable(int globaloffset)
        {
            AstScopeFile* filescope;
            AstSymTable* currenttable;
            filescope = (AstScopeFile*)this->m_filescopelist->top();
            if(!filescope)
            {
                MC_ASSERT(false);
                return false;
            }
            currenttable = filescope->filesymtab;
            filescope->filesymtab = Memory::make<AstSymTable>(currenttable, this->m_compglobalstore, nullptr, nullptr, nullptr, globaloffset);
            return true;
        }

        void popSymtable()
        {
            AstScopeFile* filescope;
            AstSymTable* currenttable;
            filescope = (AstScopeFile*)this->m_filescopelist->top();
            if(filescope != nullptr)
            {
                currenttable = filescope->filesymtab;
                if(currenttable != nullptr)
                {
                    filescope->filesymtab = currenttable->outer;
                    AstSymTable::destroy(currenttable);
                }
            }
        }

        mcinternopcode_t getLastOpcode()
        {
            AstScopeComp* currentscope;
            currentscope = this->getCompilationScope();
            return currentscope->lastopcode;
        }

        bool doCompileSource(const char* code)
        {
            bool ok;
            PtrList* statements;
            AstScopeFile* filescope;
            (void)ok;
            filescope = (AstScopeFile*)this->m_filescopelist->top();
            MC_ASSERT(filescope);
            statements = filescope->parser->parseAll(code, filescope->m_ccfile);
            if(!statements)
            {
                /* errors are added by parser */
                return false;
            }
            if(this->m_config->dumpast)
            {
                AstPrinter apr(m_filestderr);
                apr.beginPrint(statements);
            }
            ok = this->compileStmtList(statements);
            PtrList::destroy(statements, (mcitemdestroyfn_t)AstExpression::destroyExpression);
            if(this->m_config->dumpbytecode)
            {
                mc_printer_printbytecode(m_filestderr,
                    (uint16_t*)this->m_compilationscope->compiledscopebytecode->data(),
                    (AstLocation*)this->m_compilationscope->scopesrcposlist->data(),
                    this->m_compilationscope->compiledscopebytecode->count(), false);
            }
            return ok;
        }

        bool compileStmtList(PtrList* statements)
        {
            bool ok;
            size_t i;
            AstExpression* expr;
            (void)ok;
            ok = true;
            for(i = 0; i < statements->count(); i++)
            {
                expr = (AstExpression*)statements->get(i);
                ok = this->compileExpression(expr);
                if(!ok)
                {
                    break;
                }
            }
            return ok;
        }

        bool compileImportStmt(AstExpression* importstmt)
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
            AstSymTable* symtab;
            AstScopeFile* fs;
            Module* module;
            AstSymTable* st;
            AstScopeFile* filescope;
            AstSymbol* symbol;
            (void)ok;
            /* todo: split into smaller functions */
            result = false;
            filepath = nullptr;
            code = nullptr;
            filescope = (AstScopeFile*)this->m_filescopelist->top();
            modpath = importstmt->uexpr.exprimportstmt.path;
            modname = Module::getModuleName(modpath);
            for(i = 0; i < filescope->loadedmodnames->count(); i++)
            {
                loadedname = (const char*)filescope->loadedmodnames->get(i);
                if(mc_util_strequal(loadedname, modname))
                {
                    if(this->m_config->fatalcomplaints)
                    {
                        this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, importstmt->pos, "module \"%s\" was already imported", modname);
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
            filepathbuf = Memory::make<Printer>(nullptr);
            if(mc_util_pathisabsolute(modpath))
            {
                filepathbuf->format("%s.mc", modpath);
            }
            else
            {
                filepathbuf->format("%s%s.mc", filescope->m_ccfile->getDirectory(), modpath);
            }
            filepathnoncanonicalised = filepathbuf->getString();
            filepath = mc_util_canonpath(filepathnoncanonicalised);
            Printer::destroy(filepathbuf);
            if(!filepath)
            {
                result = false;
                goto end;
            }
            symtab = this->getsymtable();
            if(symtab->outer != nullptr || symtab->blockscopes->count() > 1)
            {
                this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, importstmt->pos, "modules can only be imported in global scope");
                result = false;
                goto end;
            }
            for(i = 0; i < this->m_filescopelist->count(); i++)
            {
                fs = (AstScopeFile*)this->m_filescopelist->get(i);
                if(mc_util_strequal(fs->m_ccfile->path(), filepath))
                {
                    this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, importstmt->pos, "cyclic reference of file \"%s\"", filepath);
                    result = false;
                    goto end;
                }
            }
            module = (Module*)this->m_modules->get(filepath);
            if(!module)
            {
                /* todo: create new module function */
                searchedpath = Module::findFile(filepath);
                code = mc_fsutil_fileread(searchedpath, &flen);
                if(!code)
                {
                    this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, importstmt->pos, "reading module file \"%s\" failed", filepath);
                    result = false;
                    goto end;
                }
                module = Memory::make<Module>(modname);
                ok = this->filescopepush(searchedpath);
                ok = this->doCompileSource(code);
                st = this->getsymtable();
                for(i = 0; i < st->getModuleGlobalSymCount(); i++)
                {
                    symbol = st->getModuleGlobalSymAt(i);
                    module->addSymbol(symbol);
                }
                this->filescopepop();
                ok = this->m_modules->set(filepath, module);
            }
            for(i = 0; i < module->modsymbols->count(); i++)
            {
                symbol = (AstSymbol*)module->modsymbols->get(i);
                ok = symtab->addModuleSymbol(symbol);
            }
            namecopy = mc_util_strdup(modname);
            ok = filescope->loadedmodnames->push(namecopy);
            result = true;
        end:
            mc_memory_free(filepath);
            mc_memory_free(code);
            return result;
        }

        AstSymbol* doDefineSymbol(AstLocation pos, const char* name, bool assignable, bool canshadow)
        {
            AstSymbol* symbol;
            AstSymbol* currentsymbol;
            AstSymTable* symtab;
            symtab = this->getsymtable();
            if(!canshadow && !symtab->isTopGlobalScope())
            {
                currentsymbol = symtab->resolve(name);
                if(currentsymbol)
                {
                    this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, pos, "symbol \"%s\" is already defined", name);
                    return nullptr;
                }
            }
            symbol = symtab->defineSymbol(name, assignable);
            if(!symbol)
            {
                this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, pos, "cannot define symbol \"%s\"", name);
                return nullptr;
            }
            return symbol;
        }

        bool compileDefine(AstExpression* expr)
        {
            bool ok;
            AstSymbol* symbol;
            (void)ok;
            (void)ok;
            ok = this->compileExpression(expr->uexpr.exprdefine.value);
            symbol = this->doDefineSymbol(expr->uexpr.exprdefine.name->pos, expr->uexpr.exprdefine.name->value, expr->uexpr.exprdefine.assignable, false);
            if(!symbol)
            {
                return false;
            }
            ok = this->storesymbol(symbol, true);
            return true;
        }

        bool compileifstmt(uint64_t* opbuf, AstExpression* expr)
        {
            bool ok;
            size_t i;
            int afteraltip;
            int nextcasejumpip;
            int jumptoendip;
            int afterelifip;
            int* pos;
            AstExpression::ExprIfCase* ifcase;
            AstExpression::ExprIfStmt* ifstmt;
            PtrList* jumptoendips;
            (void)ok;
            ifstmt = &expr->uexpr.exprifstmt;
            jumptoendips = Memory::make<PtrList>(sizeof(int), false);
            for(i = 0; i < ifstmt->cases->count(); i++)
            {
                ifcase = (AstExpression::ExprIfCase*)ifstmt->cases->get(i);
                ok = this->compileExpression(ifcase->ifcond);
                opbuf[0] = 0xbeef;
                nextcasejumpip = this->emitOpCode(MC_OPCODE_JUMPIFFALSE, 1, opbuf);
                ok = this->compilecodeblock(ifcase->consequence);
                /* don't emit jump for the last statement */
                if(i < (ifstmt->cases->count() - 1) || ifstmt->alternative)
                {
                    opbuf[0] = 0xbeef;
                    jumptoendip = this->emitOpCode(MC_OPCODE_JUMP, 1, opbuf);
                    ok = jumptoendips->push(&jumptoendip);
                }
                afterelifip = this->getip();
                this->changeuint16operand(nextcasejumpip + 1, afterelifip);
            }
            if(ifstmt->alternative)
            {
                ok = this->compilecodeblock(ifstmt->alternative);
            }
            afteraltip = this->getip();
            for(i = 0; i < jumptoendips->count(); i++)
            {
                pos = (int*)jumptoendips->get(i);
                this->changeuint16operand(*pos + 1, afteraltip);
            }
            PtrList::destroy(jumptoendips, nullptr);
            return true;
        statementiferror:
            PtrList::destroy(jumptoendips, nullptr);
            return false;
        }

        bool compilereturnstmt(AstScopeComp* compscope, AstExpression* expr)
        {
            bool ok;
            int ip;
            (void)ok;
            if(compscope->outer == nullptr)
            {
                this->m_ccerrlist->pushFormat( MC_ERROR_COMPILING, expr->pos, "nothing to return from");
                return false;
            }
            ip = -1;
            if(expr->uexpr.exprreturnvalue)
            {
                ok = this->compileExpression(expr->uexpr.exprreturnvalue);
                ip = this->emitOpCode(MC_OPCODE_RETURNVALUE, 0, nullptr);
            }
            else
            {
                ip = this->emitOpCode(MC_OPCODE_RETURN, 0, nullptr);
            }
            if(ip < 0)
            {
                return false;
            }
            return true;
        }

        bool compilewhilestmt(uint64_t* opbuf, AstExpression* expr)
        {
            bool ok;
            int ip;
            int beforetestip;
            int aftertestip;
            int afterbodyip;
            int jumptoafterbodyip;
            AstExpression::ExprWhileStmt* loop;
            (void)ok;
            loop = &expr->uexpr.exprwhileloopstmt;
            beforetestip = this->getip();
            ok = this->compileExpression(loop->loopcond);
            aftertestip = this->getip();
            opbuf[0] = aftertestip + 6;
            ip = this->emitOpCode(MC_OPCODE_JUMPIFTRUE, 1, opbuf);
            if(ip < 0)
            {
                return false;
            }
            opbuf[0] = 0xdead;
            jumptoafterbodyip = this->emitOpCode(MC_OPCODE_JUMP, 1, opbuf);
            if(jumptoafterbodyip < 0)
            {
                return false;
            }
            ok = this->pushcontinueip(beforetestip);
            ok = this->pushbreakip(jumptoafterbodyip);
            ok = this->compilecodeblock(loop->body);
            this->popbreakip();
            this->popcontinueip();
            opbuf[0] = beforetestip;
            ip = this->emitOpCode(MC_OPCODE_JUMP, 1, opbuf);
            if(ip < 0)
            {
                return false;
            }
            afterbodyip = this->getip();
            this->changeuint16operand(jumptoafterbodyip + 1, afterbodyip);
            return true;
        }

        bool compilebreakstmt(uint64_t* opbuf, AstExpression* expr)
        {
            int ip;
            int breakip;
            breakip = this->getbreakip();
            if(breakip < 0)
            {
                this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, expr->pos, "nothing to break from.");
                return false;
            }
            opbuf[0] = breakip;
            ip = this->emitOpCode(MC_OPCODE_JUMP, 1, opbuf);
            if(ip < 0)
            {
                return false;
            }
            return true;
        }

        bool compilecontinuestmt(uint64_t* opbuf, AstExpression* expr)
        {
            int ip;
            int continueip;
            continueip = this->getcontinueip();
            if(continueip < 0)
            {
                this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, expr->pos, "nothing to continue from.");
                return false;
            }
            opbuf[0] = continueip;
            ip = this->emitOpCode(MC_OPCODE_JUMP, 1, opbuf);
            if(ip < 0)
            {
                return false;
            }
            return true;
        }


        bool compileExpression(AstExpression* expr)
        {
            bool ok;
            bool res;
            int ip;
            uint64_t opbuf[10];
            AstExpression* exproptimized;
            AstScopeComp* compscope;
            AstSymTable* symtab;
            (void)ok;
            ok = false;
            ip = -1;
            exproptimized = nullptr;
            #if 1
            exproptimized = AstOptimizer::optimizeExpr(expr);
            if(exproptimized != nullptr)
            {
                expr = exproptimized;
            }
            #endif
            ok = this->m_srcposstack->push(&expr->pos);
            compscope = this->getCompilationScope();
            symtab = this->getsymtable();
            res = false;
            switch(expr->exprtype)
            {
                case AstExpression::EXPR_INFIX:
                    {
                        bool rearrange;
                        mcinternopcode_t op;
                        AstExpression* left;
                        AstExpression* right;
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
                                    this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, expr->pos, "unknown infix operator");
                                    goto error;
                                }
                                break;
                        }
                        left = rearrange ? expr->uexpr.exprinfix.right : expr->uexpr.exprinfix.left;
                        right = rearrange ? expr->uexpr.exprinfix.left : expr->uexpr.exprinfix.right;
                        ok = this->compileExpression(left);
                        ok = this->compileExpression(right);
                        switch(expr->uexpr.exprinfix.op)
                        {
                            case MC_MATHOP_EQ:
                            case MC_MATHOP_NOTEQ:
                                {
                                    ip = this->emitOpCode(MC_OPCODE_COMPAREEQ, 0, nullptr);
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
                                    ip = this->emitOpCode(MC_OPCODE_COMPARE, 0, nullptr);
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
                        ip = this->emitOpCode(op, 0, nullptr);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;

                case AstExpression::EXPR_NUMBERLITERAL:
                    {
                        mcfloat_t number;
                        number = expr->uexpr.exprlitnumber;
                        opbuf[0] = mc_util_doubletouint64(number);
                        ip = this->emitOpCode(MC_OPCODE_NUMBER, 1, opbuf);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;

                case AstExpression::EXPR_STRINGLITERAL:
                    {
                        int pos = 0;
                        int* posval;
                        int* currentpos;
                        Value obj;
                        currentpos = (int*)this->m_stringconstposdict->get(expr->uexpr.exprlitstring.data);
                        if(currentpos)
                        {
                            pos = *currentpos;
                        }
                        else
                        {
                            obj = mc_value_makestringlen(this->m_pstate, expr->uexpr.exprlitstring.data, expr->uexpr.exprlitstring.length);
                            if(obj.isNull())
                            {
                                goto error;
                            }
                            pos = this->addconstant(obj);
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
                            ok = this->m_stringconstposdict->set(expr->uexpr.exprlitstring.data, posval);
                        }
                        opbuf[0] = pos;
                        ip = this->emitOpCode(MC_OPCODE_CONSTANT, 1, opbuf);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_NULLLITERAL:
                    {
                        ip = this->emitOpCode(MC_OPCODE_NULL, 0, nullptr);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_BOOLLITERAL:
                    {
                        ip = this->emitOpCode(expr->uexpr.exprlitbool ? MC_OPCODE_TRUE : MC_OPCODE_FALSE, 0, nullptr);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_ARRAYLITERAL:
                    {
                        size_t i;
                        for(i = 0; i < expr->uexpr.exprlitarray.litarritems->count(); i++)
                        {
                            ok = this->compileExpression((AstExpression*)expr->uexpr.exprlitarray.litarritems->get(i));
                            if(!ok)
                            {
                                goto error;
                            }
                        }
                        opbuf[0] = expr->uexpr.exprlitarray.litarritems->count();
                        ip = this->emitOpCode(MC_OPCODE_ARRAY, 1, opbuf);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_MAPLITERAL:
                    {
                        size_t i;
                        size_t len;
                        AstExpression* key;
                        AstExpression* val;
                        AstExpression::ExprLiteralMap* map;
                        map = &expr->uexpr.exprlitmap;
                        len = map->litmapkeys->count();
                        opbuf[0] = len;
                        ip = this->emitOpCode(MC_OPCODE_MAPSTART, 1, opbuf);
                        if(ip < 0)
                        {
                            goto error;
                        }
                        for(i = 0; i < len; i++)
                        {
                            key = (AstExpression*)map->litmapkeys->get(i);
                            val = (AstExpression*)map->litmapvalues->get(i);
                            ok = this->compileExpression(key);
                            if(!ok)
                            {
                                goto error;
                            }
                            ok = this->compileExpression(val);
                            if(!ok)
                            {
                                goto error;
                            }
                        }
                        opbuf[0] = len;
                        ip = this->emitOpCode(MC_OPCODE_MAPEND, 1, opbuf);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_PREFIX:
                    {
                        mcinternopcode_t op;
                        ok = this->compileExpression(expr->uexpr.exprprefix.right);
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
                                    this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, expr->pos, "unknown prefix operator.");
                                    goto error;
                                }
                                break;
                        }
                        ip = this->emitOpCode(op, 0, nullptr);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_IDENT:
                    {
                        AstSymbol* symbol;
                        AstExpression::ExprIdent* ident;
                        ident = expr->uexpr.exprident;
                        symbol = symtab->resolve(ident->value);
                        if(!symbol)
                        {
                            if(this->m_config->strictmode)
                            {
                                this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, ident->pos, "compilation: failed to resolve symbol \"%s\"", ident->value);
                                goto error;
                            }
                            else
                            {
                                symbol = this->doDefineSymbol(ident->pos, ident->value, true, false);
                            }
                        }
                        ok = this->readsymbol(symbol);
                        if(!ok)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_INDEX:
                    {
                        AstExpression::ExprIndex* index;
                        index = &expr->uexpr.exprindex;
                        ok = this->compileExpression(index->left);
                        if(!ok)
                        {
                            goto error;
                        }
                        ok = this->compileExpression(index->index);
                        if(!ok)
                        {
                            goto error;
                        }
                        #if 1
                        if(index->isdot)
                        {
                            ip = this->emitOpCode(MC_OPCODE_GETDOTINDEX, 0, nullptr);
                        }
                        else
                        #endif
                        {
                            ip = this->emitOpCode(MC_OPCODE_GETINDEX, 0, nullptr);
                        }
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_FUNCTIONLITERAL:
                    {
                        size_t i;
                        int pos;
                        int nlocals;
                        Value obj;
                        PtrList* freesyms;
                        CompiledProgram* comp_res;
                        AstExpression::ExprLiteralFunction* fn;
                        AstSymbol* symbol;
                        AstSymbol* fnsymbol;
                        AstSymbol* thissymbol;
                        AstSymbol* paramsymbol;
                        AstExpression::ExprFuncParam* param;
                        fn = &expr->uexpr.exprlitfunction;
                        ok = this->pushCompilationScope();
                        ok = this->pushSymtable(0);
                        compscope = this->getCompilationScope();
                        symtab = this->getsymtable();
                        if(fn->name)
                        {
                            fnsymbol = symtab->defineFunctionName(fn->name, false);
                            if(!fnsymbol)
                            {
                                this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, expr->pos, "cannot define function name as \"%s\"", fn->name);
                                goto error;
                            }
                        }
                        thissymbol = symtab->defineThis();
                        if(!thissymbol)
                        {
                            this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, expr->pos, "cannot define \"this\" symbol");
                            goto error;
                        }
                        for(i = 0; i < expr->uexpr.exprlitfunction.funcparamlist->count(); i++)
                        {
                            param = (AstExpression::ExprFuncParam*)expr->uexpr.exprlitfunction.funcparamlist->get(i);
                            paramsymbol = this->doDefineSymbol(param->ident->pos, param->ident->value, true, false);
                            if(!paramsymbol)
                            {
                                goto error;
                            }
                        }
                        ok = this->compileStmtList(fn->body->statements);
                        if(!ok)
                        {
                            goto error;
                        }
                        if(!this->lastopcodeis(MC_OPCODE_RETURNVALUE) && !this->lastopcodeis(MC_OPCODE_RETURN))
                        {
                            ip = this->emitOpCode(MC_OPCODE_RETURN, 0, nullptr);
                            if(ip < 0)
                            {
                                goto error;
                            }
                        }
                        freesyms = symtab->freesymbols;
                        /* because it gets destroyed with compiler_pop_compilation_scope() */
                        symtab->freesymbols = nullptr;
                        nlocals = symtab->maxnumdefinitions;
                        comp_res = compscope->orphanResult();
                        if(!comp_res)
                        {
                            PtrList::destroy(freesyms, (mcitemdestroyfn_t)AstSymbol::destroy);
                            goto error;
                        }
                        this->popSymtable();
                        this->popCompilationScope();
                        compscope = this->getCompilationScope();
                        symtab = this->getsymtable();
                        obj = mc_value_makefuncscript(this->m_pstate, fn->name, comp_res, true, nlocals, fn->funcparamlist->count(), 0);
                        if(obj.isNull())
                        {
                            PtrList::destroy(freesyms, (mcitemdestroyfn_t)AstSymbol::destroy);
                            CompiledProgram::destroy(comp_res);
                            goto error;
                        }
                        for(i = 0; i < freesyms->count(); i++)
                        {
                            symbol = (AstSymbol*)freesyms->get(i);
                            ok = this->readsymbol(symbol);
                            if(!ok)
                            {
                                PtrList::destroy(freesyms, (mcitemdestroyfn_t)AstSymbol::destroy);
                                goto error;
                            }
                        }
                        pos = this->addconstant(obj);
                        if(pos < 0)
                        {
                            PtrList::destroy(freesyms, (mcitemdestroyfn_t)AstSymbol::destroy);
                            goto error;
                        }
                        opbuf[0] = pos;
                        opbuf[1] = freesyms->count();
                        ip = this->emitOpCode(MC_OPCODE_FUNCTION, 2, opbuf);
                        if(ip < 0)
                        {
                            PtrList::destroy(freesyms, (mcitemdestroyfn_t)AstSymbol::destroy);
                            goto error;
                        }
                        PtrList::destroy(freesyms, (mcitemdestroyfn_t)AstSymbol::destroy);
                    }
                    break;

                case AstExpression::EXPR_CALL:
                    {
                        size_t i;
                        AstExpression* argexpr;
                        ok = this->compileExpression(expr->uexpr.exprcall.function);
                        if(!ok)
                        {
                            goto error;
                        }
                        for(i = 0; i < expr->uexpr.exprcall.args->count(); i++)
                        {
                            argexpr = (AstExpression*)expr->uexpr.exprcall.args->get(i);
                            ok = this->compileExpression(argexpr);
                            if(!ok)
                            {
                                goto error;
                            }
                        }
                        opbuf[0] = expr->uexpr.exprcall.args->count();
                        ip = this->emitOpCode(MC_OPCODE_CALL, 1, opbuf);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_ASSIGN:
                    {
                        AstExpression::ExprIndex* index;
                        AstExpression::ExprAssign* assign;
                        AstExpression::ExprIdent* ident;
                        AstSymbol* symbol;
                        assign = &expr->uexpr.exprassign;
                        if(assign->dest->exprtype != AstExpression::EXPR_IDENT && assign->dest->exprtype != AstExpression::EXPR_INDEX)
                        {
                            this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, assign->dest->pos, "expression is not assignable");
                            goto error;
                        }
                        if(assign->is_postfix)
                        {
                            ok = this->compileExpression(assign->dest);
                            if(!ok)
                            {
                                goto error;
                            }
                        }
                        ok = this->compileExpression(assign->source);
                        if(!ok)
                        {
                            goto error;
                        }
                        ip = this->emitOpCode(MC_OPCODE_DUP, 0, nullptr);
                        if(ip < 0)
                        {
                            goto error;
                        }
                        ok = this->m_srcposstack->push(&assign->dest->pos);
                        if(assign->dest->exprtype == AstExpression::EXPR_IDENT)
                        {
                            ident = assign->dest->uexpr.exprident;
                            symbol = symtab->resolve(ident->value);
                            if(!symbol)
                            {
                                if(this->m_config->strictmode)
                                {
                                    this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, assign->dest->pos, "cannot assign to undeclared symbol \"%s\"", ident->value);
                                    goto error;
                                }
                                else
                                {
                                    symbol = this->doDefineSymbol(ident->pos, ident->value, true, false);
                                    if(!symbol)
                                    {
                                        this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, assign->dest->pos, "failed to implicitly create symbol \"%s\"", ident->value);
                                        goto error;
                                    }
                                }
                            }
                            if(!symbol->assignable)
                            {
                                this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, assign->dest->pos, "compilation: cannot assign to readonly symbol \"%s\"", ident->value);
                                goto error;
                            }
                            ok = this->storesymbol(symbol, false);
                            if(!ok)
                            {
                                goto error;
                            }
                        }
                        else if(assign->dest->exprtype == AstExpression::EXPR_INDEX)
                        {
                            index = &assign->dest->uexpr.exprindex;
                            ok = this->compileExpression(index->left);
                            if(!ok)
                            {
                                goto error;
                            }
                            ok = this->compileExpression(index->index);
                            if(!ok)
                            {
                                goto error;
                            }
                            ip = this->emitOpCode(MC_OPCODE_SETINDEX, 0, nullptr);
                            if(ip < 0)
                            {
                                goto error;
                            }
                        }
                        if(assign->is_postfix)
                        {
                            ip = this->emitOpCode(MC_OPCODE_POP, 0, nullptr);
                            if(ip < 0)
                            {
                                goto error;
                            }
                        }
                        this->m_srcposstack->pop(nullptr);
                    }
                    break;

                case AstExpression::EXPR_LOGICAL:
                    {
                        int afterrightip;
                        int afterleftjumpip;
                        AstExpression::ExprLogical* logi;
                        logi = &expr->uexpr.exprlogical;
                        ok = this->compileExpression(logi->left);
                        if(!ok)
                        {
                            goto error;
                        }
                        ip = this->emitOpCode(MC_OPCODE_DUP, 0, nullptr);
                        if(ip < 0)
                        {
                            goto error;
                        }
                        afterleftjumpip = 0;
                        if(logi->op == MC_MATHOP_LOGICALAND)
                        {
                            opbuf[0] = 0xbeef;
                            afterleftjumpip = this->emitOpCode(MC_OPCODE_JUMPIFFALSE, 1, opbuf);
                        }
                        else
                        {
                            opbuf[0] = 0xbeef;
                            afterleftjumpip = this->emitOpCode(MC_OPCODE_JUMPIFTRUE, 1, opbuf);
                        }
                        if(afterleftjumpip < 0)
                        {
                            goto error;
                        }
                        ip = this->emitOpCode(MC_OPCODE_POP, 0, nullptr);
                        if(ip < 0)
                        {
                            goto error;
                        }
                        ok = this->compileExpression(logi->right);
                        if(!ok)
                        {
                            goto error;
                        }
                        afterrightip = this->getip();
                        this->changeuint16operand(afterleftjumpip + 1, afterrightip);
                    }
                    break;
                case AstExpression::EXPR_TERNARY:
                    {
                        int endip;
                        int elseip;
                        int endjumpip;
                        int elsejumpip;
                        AstExpression::ExprTernary* ternary;
                        ternary = &expr->uexpr.exprternary;
                        ok = this->compileExpression(ternary->tercond);
                        if(!ok)
                        {
                            goto error;
                        }
                        opbuf[0] = 0xbeef;
                        elsejumpip = this->emitOpCode(MC_OPCODE_JUMPIFFALSE, 1, opbuf);
                        ok = this->compileExpression(ternary->teriftrue);
                        if(!ok)
                        {
                            goto error;
                        }
                        opbuf[0] = 0xbeef;
                        endjumpip = this->emitOpCode(MC_OPCODE_JUMP, 1, opbuf);
                        elseip = this->getip();
                        this->changeuint16operand(elsejumpip + 1, elseip);
                        ok = this->compileExpression(ternary->teriffalse);
                        if(!ok)
                        {
                            goto error;
                        }
                        endip = this->getip();
                        this->changeuint16operand(endjumpip + 1, endip);
                    }
                    break;

                case AstExpression::EXPR_STMTEXPRESSION:
                    {
                        ok = this->compileExpression(expr->uexpr.exprexpression);
                        if(!ok)
                        {
                            return false;
                        }
                        ip = this->emitOpCode(MC_OPCODE_POP, 0, nullptr);
                        if(ip < 0)
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTDEFINE:
                    {
                        if(!this->compileDefine(expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTIF:
                    {
                        if(!this->compileifstmt(opbuf, expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTRETURN:
                    {
                        if(!this->compilereturnstmt(compscope, expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTLOOPWHILE:
                    {
                        if(!this->compilewhilestmt(opbuf, expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTBREAK:
                    {
                        if(!this->compilebreakstmt(opbuf, expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTCONTINUE:
                    {
                        if(!this->compilecontinuestmt(opbuf, expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTLOOPFOREACH:
                    {
                        int jumptoafterupdateip;
                        int updateip;
                        int afterupdateip;
                        int aftertestip;
                        int jumptoafterbodyip;
                        int afterbodyip;
                        AstSymbol* itersymbol;
                        AstSymbol* indexsymbol;
                        AstSymbol* sourcesymbol;
                        AstExpression::ExprForeachStmt* foreach;
                        foreach = &expr->uexpr.exprforeachloopstmt;
                        ok = symtab->scopeBlockPush();
                        /* Init */
                        indexsymbol = this->doDefineSymbol(expr->pos, "@i", false, true);
                        if(!indexsymbol)
                        {
                            return false;
                        }
                        opbuf[0] = 0;
                        ip = this->emitOpCode(MC_OPCODE_NUMBER, 1, opbuf);
                        if(ip < 0)
                        {
                            return false;
                        }
                        ok = this->storesymbol(indexsymbol, true);
                        if(!ok)
                        {
                            return false;
                        }
                        sourcesymbol = nullptr;
                        if(foreach->source->exprtype == AstExpression::EXPR_IDENT)
                        {
                            sourcesymbol = symtab->resolve(foreach->source->uexpr.exprident->value);
                            if(!sourcesymbol)
                            {
                                this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, foreach->source->pos, "symbol \"%s\" could not be resolved", foreach->source->uexpr.exprident->value);
                                return false;
                            }
                        }
                        else
                        {
                            ok = this->compileExpression(foreach->source);
                            if(!ok)
                            {
                                return false;
                            }
                            sourcesymbol = this->doDefineSymbol(foreach->source->pos, "@source", false, true);
                            if(!sourcesymbol)
                            {
                                return false;
                            }
                            ok = this->storesymbol(sourcesymbol, true);
                            if(!ok)
                            {
                                return false;
                            }
                        }
                        /* Update */
                        opbuf[0] = 0xbeef;
                        jumptoafterupdateip = this->emitOpCode(MC_OPCODE_JUMP, 1, opbuf);
                        if(jumptoafterupdateip < 0)
                        {
                            return false;
                        }
                        updateip = this->getip();
                        ok = this->readsymbol(indexsymbol);
                        if(!ok)
                        {
                            return false;
                        }
                        opbuf[0] = mc_util_doubletouint64(1);
                        ip = this->emitOpCode(MC_OPCODE_NUMBER, 1, opbuf);
                        if(ip < 0)
                        {
                            return false;
                        }
                        ip = this->emitOpCode(MC_OPCODE_ADD, 0, nullptr);
                        if(ip < 0)
                        {
                            return false;
                        }
                        ok = this->storesymbol(indexsymbol, false);
                        if(!ok)
                        {
                            return false;
                        }
                        afterupdateip = this->getip();
                        this->changeuint16operand(jumptoafterupdateip + 1, afterupdateip);
                        /* Test */
                        ok = this->m_srcposstack->push(&foreach->source->pos);
                        ok = this->readsymbol(sourcesymbol);
                        if(!ok)
                        {
                            return false;
                        }
                        ip = this->emitOpCode(MC_OPCODE_FOREACHLEN, 0, nullptr);
                        if(ip < 0)
                        {
                            return false;
                        }
                        this->m_srcposstack->pop(nullptr);
                        ok = this->readsymbol(indexsymbol);
                        if(!ok)
                        {
                            return false;
                        }
                        ip = this->emitOpCode(MC_OPCODE_COMPARE, 0, nullptr);
                        if(ip < 0)
                        {
                            return false;
                        }
                        ip = this->emitOpCode(MC_OPCODE_EQUAL, 0, nullptr);
                        if(ip < 0)
                        {
                            return false;
                        }
                        aftertestip = this->getip();
                        opbuf[0] = aftertestip + 6;
                        ip = this->emitOpCode(MC_OPCODE_JUMPIFFALSE, 1, opbuf);
                        if(ip < 0)
                        {
                            return false;
                        }
                        opbuf[0] = 0xdead;
                        jumptoafterbodyip = this->emitOpCode(MC_OPCODE_JUMP, 1, opbuf);
                        if(jumptoafterbodyip < 0)
                        {
                            return false;
                        }
                        ok = this->readsymbol(sourcesymbol);
                        if(!ok)
                        {
                            return false;
                        }
                        ok = this->readsymbol(indexsymbol);
                        if(!ok)
                        {
                            return false;
                        }
                        ip = this->emitOpCode(MC_OPCODE_GETVALUEAT, 0, nullptr);
                        if(ip < 0)
                        {
                            return false;
                        }
                        itersymbol = this->doDefineSymbol(foreach->iterator->pos, foreach->iterator->value, false, false);
                        if(!itersymbol)
                        {
                            return false;
                        }
                        ok = this->storesymbol(itersymbol, true);
                        if(!ok)
                        {
                            return false;
                        }
                        /* Body */
                        ok = this->pushcontinueip(updateip);
                        ok = this->pushbreakip(jumptoafterbodyip);
                        ok = this->compilecodeblock(foreach->body);
                        this->popbreakip();
                        this->popcontinueip();
                        opbuf[0] = updateip;
                        ip = this->emitOpCode(MC_OPCODE_JUMP, 1, opbuf);
                        if(ip < 0)
                        {
                            return false;
                        }
                        afterbodyip = this->getip();
                        this->changeuint16operand(jumptoafterbodyip + 1, afterbodyip);
                        symtab->scopeBlockPop();
                    }
                    break;
                case AstExpression::EXPR_STMTLOOPFORCLASSIC:
                    {
                        int afterbodyip;
                        int jumptoafterupdateip;
                        int updateip;
                        int afterupdateip;
                        int aftertestip;
                        int jumptoafterbodyip;
                        AstExpression::ExprLoopStmt* loop;
                        loop = &expr->uexpr.exprforloopstmt;
                        ok = symtab->scopeBlockPush();
                        /* Init */
                        jumptoafterupdateip = 0;
                        ok = false;
                        if(loop->init)
                        {
                            ok = this->compileExpression(loop->init);
                            if(!ok)
                            {
                                return false;
                            }
                            opbuf[0] = 0xbeef;
                            jumptoafterupdateip = this->emitOpCode(MC_OPCODE_JUMP, 1, opbuf);
                            if(jumptoafterupdateip < 0)
                            {
                                return false;
                            }
                        }
                        /* Update */
                        updateip = this->getip();
                        if(loop->update)
                        {
                            ok = this->compileExpression(loop->update);
                            if(!ok)
                            {
                                return false;
                            }
                            ip = this->emitOpCode(MC_OPCODE_POP, 0, nullptr);
                            if(ip < 0)
                            {
                                return false;
                            }
                        }
                        if(loop->init)
                        {
                            afterupdateip = this->getip();
                            this->changeuint16operand(jumptoafterupdateip + 1, afterupdateip);
                        }
                        /* Test */
                        if(loop->loopcond)
                        {
                            ok = this->compileExpression(loop->loopcond);
                            if(!ok)
                            {
                                return false;
                            }
                        }
                        else
                        {
                            ip = this->emitOpCode(MC_OPCODE_TRUE, 0, nullptr);
                            if(ip < 0)
                            {
                                return false;
                            }
                        }
                        aftertestip = this->getip();
                        opbuf[0] = aftertestip + 6;
                        ip = this->emitOpCode(MC_OPCODE_JUMPIFTRUE, 1, opbuf);
                        if(ip < 0)
                        {
                            return false;
                        }
                        opbuf[0] = 0xdead;
                        jumptoafterbodyip = this->emitOpCode(MC_OPCODE_JUMP, 1, opbuf);
                        if(jumptoafterbodyip < 0)
                        {
                            return false;
                        }
                        /* Body */
                        ok = this->pushcontinueip(updateip);
                        ok = this->pushbreakip(jumptoafterbodyip);
                        ok = this->compilecodeblock(loop->body);
                        if(!ok)
                        {
                            return false;
                        }
                        this->popbreakip();
                        this->popcontinueip();
                        opbuf[0] = updateip;
                        ip = this->emitOpCode(MC_OPCODE_JUMP, 1, opbuf);
                        if(ip < 0)
                        {
                            return false;
                        }
                        afterbodyip = this->getip();
                        this->changeuint16operand(jumptoafterbodyip + 1, afterbodyip);
                        symtab->scopeBlockPop();
                    }
                    break;
                case AstExpression::EXPR_STMTBLOCK:
                    {
                        ok = this->compilecodeblock(expr->uexpr.exprblockstmt);
                        if(!ok)
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTIMPORT:
                    {
                        ok = this->compileImportStmt(expr);
                        if(!ok)
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTRECOVER:
                    {
                        int recip;
                        int afterrecoverip;
                        int afterjumptorecoverip;
                        int jumptoafterrecoverip;
                        AstSymbol* errorsymbol;
                        AstExpression::ExprRecover* recover;
                        recover = &expr->uexpr.exprrecoverstmt;
                        if(this->m_config->strictmode)
                        {
                            if(symtab->isModuleGlobalScope())
                            {
                                this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, expr->pos, "recover statement cannot be defined in global scope");
                                return false;
                            }
                        }
                        #if 0
                        if(!symtab->isTopBlockScope())
                        {
                            this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, expr->pos, "recover statement cannot be defined within other statements");
                            return false;
                        }
                        #endif
                        opbuf[0] = 0xbeef;
                        recip = this->emitOpCode(MC_OPCODE_SETRECOVER, 1, opbuf);
                        if(recip < 0)
                        {
                            return false;
                        }
                        opbuf[0] = 0xbeef;
                        jumptoafterrecoverip = this->emitOpCode(MC_OPCODE_JUMP, 1, opbuf);
                        if(jumptoafterrecoverip < 0)
                        {
                            return false;
                        }
                        afterjumptorecoverip = this->getip();
                        this->changeuint16operand(recip + 1, afterjumptorecoverip);
                        ok = symtab->scopeBlockPush();
                        errorsymbol = this->doDefineSymbol(recover->errident->pos, recover->errident->value, false, false);
                        if(!errorsymbol)
                        {
                            return false;
                        }
                        ok = this->storesymbol(errorsymbol, true);
                        if(!ok)
                        {
                            return false;
                        }
                        ok = this->compilecodeblock(recover->body);
                        if(!ok)
                        {
                            return false;
                        }
                        if(!this->lastopcodeis(MC_OPCODE_RETURN) && !this->lastopcodeis(MC_OPCODE_RETURNVALUE))
                        {
                            #if 0
                                this->m_ccerrlist->pushFormat(MC_ERROR_COMPILING, expr->pos, "recover body must end with a return statement");
                                return false;
                            #else
                                mc_util_complain(expr->pos, "recover body should end with a return statement");
                            #endif
                        }
                        symtab->scopeBlockPop();
                        afterrecoverip = this->getip();
                        this->changeuint16operand(jumptoafterrecoverip + 1, afterrecoverip);
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
            this->m_srcposstack->pop(nullptr);
            AstExpression::destroyExpression(exproptimized);
            return res;
        }

        bool compilecodeblock(AstExpression::ExprCodeBlock* block)
        {
            bool ok;
            size_t i;
            int ip;
            AstSymTable* symtab;
            AstExpression* expr;
            (void)ok;
            symtab = this->getsymtable();
            if(!symtab)
            {
                return false;
            }
            ok = symtab->scopeBlockPush();
            if(block->statements->count() == 0)
            {
                ip = this->emitOpCode(MC_OPCODE_NULL, 0, nullptr);
                if(ip < 0)
                {
                    return false;
                }
                ip = this->emitOpCode(MC_OPCODE_POP, 0, nullptr);
                if(ip < 0)
                {
                    return false;
                }
            }
            for(i = 0; i < block->statements->count(); i++)
            {
                expr = (AstExpression*)block->statements->get(i);
                ok = this->compileExpression(expr);
                if(!ok)
                {
                    return false;
                }
            }
            symtab->scopeBlockPop();
            return true;
        }

        int addconstant(Value obj)
        {
            bool ok;
            int pos;
            (void)ok;
            ok = this->m_constants->push(obj);
            pos = this->m_constants->count() - 1;
            return pos;
        }

        void changeuint16operand(int ip, uint16_t operand)
        {
            uint16_t hi;
            uint16_t lo;
            PtrList* bytecode;
            bytecode = this->getbytecode();
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

        bool lastopcodeis(mcinternopcode_t op)
        {
            mcinternopcode_t lastopcode;
            lastopcode = this->getLastOpcode();
            return lastopcode == op;
        }

        bool readsymbol(AstSymbol* symbol)
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
                ip = this->emitOpCode(MC_OPCODE_GETMODULEGLOBAL, 1, opbuf);
            }
            else if(symbol->symtype == MC_SYM_GLOBALBUILTIN)
            {
                opbuf[0] = symbol->index;
                ip = this->emitOpCode(MC_OPCODE_GETGLOBALBUILTIN, 1, opbuf);
            }
            else if(symbol->symtype == MC_SYM_LOCAL)
            {
                opbuf[0] = symbol->index;
                ip = this->emitOpCode(MC_OPCODE_GETLOCAL, 1, opbuf);
            }
            else if(symbol->symtype == MC_SYM_FREE)
            {
                opbuf[0] = symbol->index;
                ip = this->emitOpCode(MC_OPCODE_GETFREE, 1, opbuf);
            }
            else if(symbol->symtype == MC_SYM_FUNCTION)
            {
                ip = this->emitOpCode(MC_OPCODE_CURRENTFUNCTION, 0, nullptr);
            }
            else if(symbol->symtype == MC_SYM_THIS)
            {
                ip = this->emitOpCode(MC_OPCODE_GETTHIS, 0, nullptr);
            }
            return ip >= 0;
        }

        bool storesymbol(AstSymbol* symbol, bool define)
        {
            int ip;
            uint64_t opbuf[10];
            ip = -1;
            if(symbol->symtype == MC_SYM_MODULEGLOBAL)
            {
                if(define)
                {
                    opbuf[0] = symbol->index;
                    ip = this->emitOpCode(MC_OPCODE_DEFINEMODULEGLOBAL, 1, opbuf);
                }
                else
                {
                    opbuf[0] = symbol->index;
                    ip = this->emitOpCode(MC_OPCODE_SETMODULEGLOBAL, 1, opbuf);
                }
            }
            else if(symbol->symtype == MC_SYM_LOCAL)
            {
                if(define)
                {
                    opbuf[0] = symbol->index;
                    ip = this->emitOpCode(MC_OPCODE_DEFINELOCAL, 1, opbuf);
                }
                else
                {
                    opbuf[0] = symbol->index;
                    ip = this->emitOpCode(MC_OPCODE_SETLOCAL, 1, opbuf);
                }
            }
            else if(symbol->symtype == MC_SYM_FREE)
            {
                opbuf[0] = symbol->index;
                ip = this->emitOpCode(MC_OPCODE_SETFREE, 1, opbuf);
            }
            return ip >= 0;
        }

        bool pushbreakip(int ip)
        {
            AstScopeComp* compscope;
            compscope = this->getCompilationScope();
            return compscope->ipstackbreak->push(&ip);
        }

        void popbreakip()
        {
            AstScopeComp* compscope;
            compscope = this->getCompilationScope();
            if(compscope->ipstackbreak->count() == 0)
            {
            }
            else
            {
                compscope->ipstackbreak->pop(nullptr);
            }
        }

        int getbreakip()
        {
            int* res;
            AstScopeComp* compscope;
            compscope = this->getCompilationScope();
            if(compscope->ipstackbreak->count() == 0)
            {
                return -1;
            }
            res = (int*)compscope->ipstackbreak->top();
            return *res;
        }

        bool pushcontinueip(int ip)
        {
            AstScopeComp* compscope;
            compscope = this->getCompilationScope();
            return compscope->ipstackcontinue->push(&ip);
        }

        void popcontinueip()
        {
            AstScopeComp* compscope;
            compscope = this->getCompilationScope();
            if(compscope->ipstackcontinue->count() == 0)
            {
                MC_ASSERT(false);
            }
            compscope->ipstackcontinue->pop(nullptr);
        }

        int getcontinueip()
        {
            int* res;
            AstScopeComp* compscope;
            compscope = this->getCompilationScope();
            if(compscope->ipstackcontinue->count() == 0)
            {
                MC_ASSERT(false);
                return -1;
            }
            res = (int*)compscope->ipstackcontinue->top();
            return *res;
        }

        int getip()
        {
            AstScopeComp* compscope;
            compscope = this->getCompilationScope();
            return compscope->compiledscopebytecode->count();
        }

        PtrList* getsrcpositions()
        {
            AstScopeComp* compscope;
            compscope = this->getCompilationScope();
            return compscope->scopesrcposlist;
        }

        PtrList* getbytecode()
        {
            AstScopeComp* compscope;
            compscope = this->getCompilationScope();
            return compscope->compiledscopebytecode;
        }


        bool filescopepush(const char* filepath)
        {
            bool ok;
            int globaloffset;
            mcastscopeblock_t* prevsttopscope;
            AstSymTable* prevst;
            AstSourceFile* file;
            AstScopeFile* filescope;
            (void)ok;
            prevst = nullptr;
            if(this->m_filescopelist->count() > 0)
            {
                prevst = this->getsymtable();
            }
            file = Memory::make<AstSourceFile>(filepath);
            ok = this->m_files->push(file);
            filescope = Memory::make<AstScopeFile>(this->m_config, this->m_ccerrlist, file);
            ok = this->m_filescopelist->push(filescope);
            globaloffset = 0;
            if(prevst)
            {
                prevsttopscope = prevst->scopeBlockGet();
                globaloffset = prevsttopscope->offset + prevsttopscope->numdefinitions;
            }
            ok = this->pushSymtable(globaloffset);
            return true;
        }

        void filescopepop()
        {
            int poppednumdefs;
            AstSymTable* poppedst;
            AstSymTable* currentst;
            AstScopeFile* scope;
            mcastscopeblock_t* currentsttopscope;
            mcastscopeblock_t* poppedsttopscope;
            poppedst = this->getsymtable();
            poppedsttopscope = poppedst->scopeBlockGet();
            poppednumdefs = poppedsttopscope->numdefinitions;
            while(this->getsymtable())
            {
                this->popSymtable();
            }
            scope = (AstScopeFile*)this->m_filescopelist->top();
            if(!scope)
            {
                MC_ASSERT(false);
            }
            Memory::destroy(scope);
            this->m_filescopelist->pop(nullptr);
            if(this->m_filescopelist->count() > 0)
            {
                currentst = this->getsymtable();
                currentsttopscope = currentst->scopeBlockGet();
                currentsttopscope->numdefinitions += poppednumdefs;
            }
        }

        void setcompilationscope(AstScopeComp* scope)
        {
            this->m_compilationscope = scope;
        }


        CompiledProgram* compilesource(const char* code, const char* filename)
        {
            bool ok;
            AstCompiler compshallowcopy;
            AstScopeComp* compscope;
            CompiledProgram* res;
            (void)ok;
            compscope = this->getCompilationScope();
            MC_ASSERT(this->m_srcposstack->count() == 0);
            MC_ASSERT(compscope->compiledscopebytecode->count() == 0);
            MC_ASSERT(compscope->ipstackbreak->count() == 0);
            MC_ASSERT(compscope->ipstackcontinue->count() == 0);
            this->m_srcposstack->clear();
            compscope->compiledscopebytecode->clear();
            compscope->scopesrcposlist->clear();
            compscope->ipstackbreak->clear();
            compscope->ipstackcontinue->clear();
            ok = this->filescopepush(filename);
            ok = initShallowCopy(&compshallowcopy, this);
            if(!ok)
            {
                return nullptr;
            }
            ok = this->doCompileSource(code);
            if(!ok)
            {
                goto compilefailed;
            }
            this->emitOpCode(MC_OPCODE_HALT, 0, 0);
            /* might've changed */
            compscope = this->getCompilationScope();
            MC_ASSERT(compscope->outer == nullptr);
            compscope = this->getCompilationScope();
            res = compscope->orphanResult();
            if(!res)
            {
                goto compilefailed;
            }
            compshallowcopy.deinit();
            return res;
        compilefailed:
            this->deinit();
            *this = compshallowcopy;
            return nullptr;
        }

        AstSymTable* getsymtable()
        {
            AstScopeFile* filescope = (AstScopeFile*)this->m_filescopelist->top();
            if(!filescope)
            {
                MC_ASSERT(false);
                return nullptr;
            }
            return filescope->filesymtab;
        }

        void setsymtable(AstSymTable* table)
        {
            AstScopeFile* filescope = (AstScopeFile*)this->m_filescopelist->top();
            if(!filescope)
            {
                MC_ASSERT(false);
            }
            filescope->filesymtab = table;
        }

        GenericList<Value>* getconstants()
        {
            return this->m_constants;
        }
};


class VMFrame
{
    public:
        Value function;
        int64_t bcposition = 0;
        int64_t basepointer = 0;
        AstLocation* framesrcposlist = nullptr;
        uint16_t* bytecode = nullptr;
        int64_t sourcebcpos = 0;
        int64_t bcsize = 0;
        int64_t recoverip = 0;
        bool isrecovering = false;

    public:
        static inline bool init(VMFrame* frame, Value functionobj, int64_t baseptr)
        {
            mcobjfunction_t* function;
            if(functionobj.getType() != Value::VALTYP_FUNCSCRIPT)
            {
                return false;
            }
            function = Value::asFunction(functionobj);
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

        inline uint16_t readUint8()
        {
            uint16_t data;
            data = this->bytecode[this->bcposition];
            this->bcposition++;
            return data;
        }

        inline uint16_t readUint16()
        {
            uint16_t* data;
            data = this->bytecode + this->bcposition;
            this->bcposition += 2;
            return (data[0] << 8) | data[1];
        }

        inline uint64_t readUint64()
        {
            uint64_t res;
            uint16_t* data;
            data = this->bytecode + this->bcposition;
            this->bcposition += 8;
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

        inline mcopcode_t readOpCode()
        {
            this->sourcebcpos = this->bcposition;
            return (mcopcode_t)this->readUint8();
        }

        inline AstLocation getPosition()
        {
            if(this->framesrcposlist)
            {
                return this->framesrcposlist[this->sourcebcpos];
            }
            return AstLocation::Invalid();
        }
};


struct mcexecstate_t
{
    size_t vsposition;
    VMFrame* currframe;
    GenericList<Value>* valuestack;
    GenericList<Value>* valthisstack;
    GenericList<Value>* nativethisstack;
    size_t thisstpos;
    GenericList<VMFrame>* framestack;
    Value lastpopped;

};

class State
{
    public:

        static void destroy(State* state)
        {
            if(state != nullptr)
            {
                state->deinit();
                mc_memory_free(state);
            }
        }


    public:
        mcconfig_t m_config;
        ErrList m_errorlist;
        GCMemory* m_stategcmem;
        SymStore* vmglobalstore;
        GenericList<Value>* globalvalstack;
        size_t globalvalcount;
        bool hadrecovered;
        bool running;
        Value operoverloadkeys[MC_CONF_MAXOPEROVERLOADS];
        PtrList* files;
        AstCompiler* compiler;
        mcexecstate_t execstate;
        Printer* stdoutprinter;
        Printer* stderrprinter;
        ObjClass* stdobjobject;
        ObjClass* stdobjnumber;
        ObjClass* stdobjstring;
        ObjClass* stdobjarray;
        ObjClass* stdobjmap;
        ObjClass* stdobjfunction;

    public:
        State()
        {
            this->setDefaultConfig();
            this->execstate.valuestack = Memory::make<GenericList<Value>>(MC_CONF_MINVMVALSTACKSIZE, Value::makeNull());
            this->execstate.valthisstack = Memory::make<GenericList<Value>>(MC_CONF_MINVMTHISSTACKSIZE, Value::makeNull());
            this->execstate.nativethisstack = Memory::make<GenericList<Value>>(MC_CONF_MINNATIVETHISSTACKSIZE, Value::makeNull());
            this->globalvalstack = Memory::make<GenericList<Value>>(MC_CONF_MINVMGLOBALS, Value::makeNull());
            this->execstate.framestack = Memory::make<GenericList<VMFrame>>(MC_CONF_MINVMFRAMES, VMFrame{});
            m_stategcmem = Memory::make<GCMemory>();
            if(!m_stategcmem)
            {
                goto err;
            }
            this->files = Memory::make<PtrList>(sizeof(void*), true);
            mc_vm_init(this);
            this->vmglobalstore = Memory::make<SymStore>();
            this->compiler = Memory::make<AstCompiler>(this, &m_config, m_stategcmem, &m_errorlist, this->files, this->vmglobalstore, this->stderrprinter);
            this->stdoutprinter = Memory::make<Printer>(stdout);
            this->stderrprinter = Memory::make<Printer>(stderr);
            mc_state_makestdclasses(this);
            return;
        err:
            this->deinit();
            MC_ASSERT(false);
        }


        void deinit()
        {
            Memory::destroy(this->compiler);
            Memory::destroy(this->vmglobalstore);
            Memory::destroy(m_stategcmem);
            PtrList::destroy(this->files, (mcitemdestroyfn_t)AstSourceFile::destroy);
            m_errorlist.destroy();
            Printer::destroy(this->stdoutprinter);
            Printer::destroy(this->stderrprinter);
            Memory::destroy(this->execstate.valuestack);
            Memory::destroy(this->globalvalstack);
            Memory::destroy(this->execstate.framestack);
            Memory::destroy(this->execstate.valthisstack);
            Memory::destroy(this->execstate.nativethisstack);
            Memory::destroy(this->stdobjobject);
            Memory::destroy(this->stdobjnumber);
            Memory::destroy(this->stdobjstring);
            Memory::destroy(this->stdobjarray);
            Memory::destroy(this->stdobjmap);
            Memory::destroy(this->stdobjfunction);
        }

        void reset()
        {
            this->errorsClear();
            mc_vm_reset(this);
        }

        void setDefaultConfig()
        {
            m_config.replmode = false;
            m_config.dumpast = false;
            m_config.dumpbytecode = false;
            m_config.fatalcomplaints = false;
            m_config.exitaftercompiling = false;
            m_config.printinstructions = false;
            m_config.strictmode = false;
        }

        template<typename... ArgsT>
        void pushError(mcerrtype_t type, AstLocation pos, const char* fmt, ArgsT&&... args)
        {
            m_errorlist.pushFormat(type, pos, fmt, args...);
        }

        template<typename... ArgsT>
        void pushRuntimeError(const char* fmt, ArgsT&&... args)
        {
            this->pushError(MC_ERROR_RUNTIME, AstLocation::Invalid(), fmt, args...);
        }

        bool setGlobalFunction(const char* name, mcnativefn_t fn, void* data)
        {
            Value obj;
            obj = mc_value_makefuncnative(this, name, fn, data);
            if(obj.isNull())
            {
                return false;
            }
            return this->setGlobalValue(name, obj);
        }

        bool setGlobalValue(const char* name, Value obj)
        {
            return this->vmglobalstore->setNamed(name, obj);
        }

        Value getGlobalByName(const char* name)
        {
            bool ok;
            Value res;
            AstSymbol* symbol;
            AstSymTable* st;
            (void)ok;
            st = this->compiler->getsymtable();
            symbol = st->resolve(name);
            if(!symbol)
            {
                this->pushError(MC_ERROR_USER, AstLocation::Invalid(), "symbol \"%s\" is not defined", name);
                return Value::makeNull();
            }
            res = Value::makeNull();
            if(symbol->symtype == MC_SYM_MODULEGLOBAL)
            {
                res = this->getGlobalByIndex(symbol->index);
            }
            else if(symbol->symtype == MC_SYM_GLOBALBUILTIN)
            {
                ok = false;
                res = this->vmglobalstore->getAtIndex(symbol->index, &ok);
                if(!ok)
                {
                    this->pushError(MC_ERROR_USER, AstLocation::Invalid(), "failed to get global object at %d", symbol->index);
                    return Value::makeNull();
                }
            }
            else
            {
                this->pushError(MC_ERROR_USER, AstLocation::Invalid(), "value associated with symbol \"%s\" could not be loaded", name);
                return Value::makeNull();
            }
            return res;
        }

        //xxxhere

        inline bool setGlobalByIndex(size_t ix, Value val)
        {
            this->globalvalstack->set(ix, val);
            if(ix >= this->globalvalcount)
            {
                this->globalvalcount = ix + 1;
            }
            return true;
        }

        inline Value getGlobalByIndex(size_t ix)
        {
            return this->globalvalstack->get(ix);
        }

        inline void setStackPos(size_t nsp)
        {
            #if 0
            size_t i;
            size_t count;
            size_t bytescount;
            if(nsp > this->execstate.vsposition)
            {
                /* to avoid gcing freed objects */
                count = nsp - this->execstate.vsposition;
                bytescount = (count - 0) * sizeof(Value);
                for(i=(this->execstate.vsposition - 0); (i != bytescount) && (i < this->execstate.valuestack->m_listcapacity); i++)
                {
                    //memset(&this->execstate.valuestack->m_listitems[i], 0, sizeof(Value));
                    this->execstate.valuestack->m_listitems[i].valtype = Value::VALTYP_NULL;
                }
            }
            #endif
            this->execstate.vsposition = nsp;
        }

        inline void vmStackPush(Value obj)
        {
            int numlocals;
            VMFrame* frame;
            mcobjfunction_t* currentfunction;
            (void)numlocals;
            (void)frame;
            (void)currentfunction;
        #if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
            if(this->execstate.currframe)
            {
                frame = this->execstate.currframe;
                currentfunction = Value::asFunction(frame->function);
                numlocals = currentfunction->funcdata.valscriptfunc.numlocals;
                MC_ASSERT((size_t)this->execstate.vsposition >= (size_t)(frame->basepointer + numlocals));
            }
        #endif
            #if 1
                this->execstate.valuestack->set(this->execstate.vsposition, obj);
            #else
                this->execstate.valuestack->push(obj);
            #endif
            this->execstate.vsposition++;
        }

        inline Value vmStackPop()
        {
            int numlocals;
            Value res;
            VMFrame* frame;
            mcobjfunction_t* currentfunction;
            (void)numlocals;
            (void)frame;
            (void)currentfunction;
        #if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
            if(this->execstate.vsposition == 0)
            {
                this->m_errorlist.pushFormat(MC_ERROR_RUNTIME, this->execstate.currframe->getPosition(), "stack underflow");
                MC_ASSERT(false);
                return Value::makeNull();
            }
            if(this->execstate.currframe)
            {
                frame = this->execstate.currframe;
                currentfunction = Value::asFunction(frame->function);
                numlocals = currentfunction->funcdata.valscriptfunc.numlocals;
                MC_ASSERT((this->execstate.vsposition - 1) >= (frame->basepointer + numlocals));
            }
        #endif
            this->execstate.vsposition--;
            res = this->execstate.valuestack->get(this->execstate.vsposition);
            this->execstate.lastpopped = res;
            return res;
        }

        inline Value vmStackGet(size_t nthitem)
        {
            size_t ix;
            ix = this->execstate.vsposition - 1 - nthitem;
            return this->execstate.valuestack->get(ix);
        }

        inline void vmStackThisPush(Value obj)
        {
            this->execstate.valthisstack->set(this->execstate.thisstpos, obj);
            this->execstate.thisstpos++;
        }

        inline Value vmStackThisPop()
        {
        #if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
            if(this->execstate.thisstpos == 0)
            {
                this->errorspushFormat(MC_ERROR_RUNTIME, this->execstate.currframe->getPosition(), "'this' stack underflow");
                MC_ASSERT(false);
                return Value::makeNull();
            }
        #endif
            this->execstate.thisstpos--;
            return this->execstate.valthisstack->get(this->execstate.thisstpos);
        }

        inline Value vmStackThisGet(size_t nthitem)
        {
            size_t ix;
            size_t cnt;
            Value val;
            (void)val;
            ix = this->execstate.thisstpos - 1 - nthitem;
            cnt = this->execstate.valthisstack->count();
            if((cnt == 0) || (ix > cnt))
            {
                //val = mc_value_makemap(this);
                //this->execstate.valthisstack->set(ix, val);
                //return mc_value_makemap(this);
                return Value::makeNull();
                //return val;
            }
            return this->execstate.valthisstack->get(ix);
        }


        bool vmPushFrame(VMFrame frame)
        {
            int add;
            mcobjfunction_t* framefunction;
            add = 0;
            this->execstate.framestack->set(this->execstate.framestack->m_listcount, frame);
            add = 1;
            this->execstate.currframe = this->execstate.framestack->getp(this->execstate.framestack->m_listcount);
            this->execstate.framestack->m_listcount += add;
            framefunction = Value::asFunction(frame.function);
            this->setStackPos(frame.basepointer + framefunction->funcdata.valscriptfunc.numlocals);
            return true;
        }


        inline bool vmPopFrame()
        {
            this->setStackPos(this->execstate.currframe->basepointer - 1);
            if(this->execstate.framestack->m_listcount <= 0)
            {
                MC_ASSERT(false);
                this->execstate.currframe = NULL;
                return false;
            }
            this->execstate.framestack->m_listcount--;
            if(this->execstate.framestack->m_listcount == 0)
            {
                this->execstate.currframe = NULL;
                return false;
            }
            this->execstate.currframe = this->execstate.framestack->getp(this->execstate.framestack->m_listcount - 1);
            return true;
        }

        inline Value vmCallNativeFunction(Value callee, AstLocation srcpos, Value selfval, size_t argc, Value* args)
        {
            Value::Type restype;
            Value res;
            Error* err; 
            Traceback* traceback;
            mcobjfunction_t* nativefun;
            nativefun = Value::asFunction(callee);
            res = nativefun->funcdata.valnativefunc.natptrfn(this, nativefun->funcdata.valnativefunc.userpointer, selfval, argc, args);
            if(mc_util_unlikely(this->m_errorlist.count() > 0))
            {
                err = this->m_errorlist.getLast();
                err->m_pos = srcpos;
                err->m_traceback = Memory::make<Traceback>();
                if(err->m_traceback)
                {
                    err->m_traceback->push(nativefun->funcdata.valnativefunc.natfnname, AstLocation::Invalid());
                }
                return Value::makeNull();
            }
            restype = res.getType();
            if(mc_util_unlikely(restype == Value::VALTYP_ERROR))
            {
                traceback = Memory::make<Traceback>();
                if(traceback)
                {
                    /* error builtin is treated in a special way */
                    if(!mc_util_strequal(nativefun->funcdata.valnativefunc.natfnname, "error"))
                    {
                        traceback->push(nativefun->funcdata.valnativefunc.natfnname, AstLocation::Invalid());
                    }
                    mc_traceback_vmpush(traceback, this);
                    mc_value_errorsettraceback(res, traceback);
                }
            }
            return res;
        }

        Value vmCallValue(GenericList<Value>* constants, Value callee, Value thisval, size_t argc, Value* args)
        {
            bool ok;
            size_t i;
            size_t oldsp;
            size_t oldthissp;
            size_t oldframescount;
            VMFrame tempframe;
            Value retv;
            mcexecstate_t est;
            Value::Type type;
            (void)oldsp;
            (void)oldframescount;
            if(constants == nullptr)
            {
                constants = this->compiler->getconstants();
            }
            type = callee.getType();
            if(type == Value::VALTYP_FUNCSCRIPT)
            {
                //VMFrame::init(&tempframe, callee, 0);
                VMFrame::init(&tempframe, callee, this->execstate.vsposition - argc);
                mc_vm_savestate(this, &est);
                oldsp = this->execstate.vsposition;
                oldthissp = this->execstate.thisstpos;
                this->vmPushFrame(tempframe);
                oldframescount = this->execstate.framestack->count();
                this->vmStackPush(callee);
                for(i = 0; i < argc; i++)
                {
                    this->vmStackPush(args[i]);
                }
                ok = mc_vm_execvm(this, callee, constants, true);
                if(!ok)
                {
                    mc_vm_restorestate(this, &est);
                    return Value::makeNull();
                }
                #if 1
                while(this->execstate.framestack->count() > oldframescount)
                {
                    this->vmPopFrame();
                }
                #endif
                this->execstate.thisstpos = oldthissp;
                retv = this->execstate.lastpopped;
                mc_vm_restorestate(this, &est);
                return retv;
            }
            if(type == Value::VALTYP_FUNCNATIVE)
            {
                return this->vmCallNativeFunction(callee, AstLocation::Invalid(), thisval, argc, args);
            }
            this->m_errorlist.pushFormat(MC_ERROR_USER, AstLocation::Invalid(), "object is not callable");
            return Value::makeNull();
        }

        Value callFunctionByName(const char* fname, Value thisval, size_t argc, Value* args)
        {
            Value res;
            Value callee;
            this->reset();
            callee = this->getGlobalByName(fname);
            if(callee.getType() == Value::VALTYP_NULL)
            {
                return Value::makeNull();
            }
            res = this->vmCallValue(this->compiler->getconstants(), callee, thisval, argc, (Value*)args);
            if(this->m_errorlist.count() > 0)
            {
                return Value::makeNull();
            }
            return res;
        }

        bool hasErrors()
        {
            return this->errorCount() > 0;
        }

        int errorCount()
        {
            return this->m_errorlist.count();
        }

        void errorsClear()
        {
            this->m_errorlist.clear();
        }

        Error* errorGet(int index)
        {
            return (Error*)this->m_errorlist.get(index);
        }

        void printErrors()
        {
            int i;
            int ecnt;
            Error* err;
            ecnt = this->errorCount();
            for(i = 0; i < ecnt; i++)
            {
                err = this->errorGet(i);
                err->printErrorTo(this->stderrprinter);
            }
        }

        CompiledProgram* compileSource(const char* code, const char* filename)
        {
            CompiledProgram* compres;
            this->errorsClear();
            compres = this->compiler->compilesource(code, filename);
            if(this->m_errorlist.count() > 0)
            {
                goto err;
            }
            return compres;
        err:
            CompiledProgram::destroy(compres);
            return nullptr;
        }

};

// bottom
mcobjfunction_t* Value::asFunction(Value object)
{
    mcobjdata_t* data;
    data = object.getAllocatedData();
    return &data->uvobj.valfunc;
}

Value Value::copyFlat(State* state, Value obj)
{
    bool ok;
    Value copy;
    Value::Type type;
    (void)ok;
    copy = Value::makeNull();
    type = obj.getType();
    switch(type)
    {
        case Value::VALTYP_ANY:
        case Value::VALTYP_FREED:
        case Value::VALTYP_NONE:
            {
                MC_ASSERT(false);
                copy = Value::makeNull();
            }
            break;
        case Value::VALTYP_NUMBER:
        case Value::VALTYP_BOOL:
        case Value::VALTYP_NULL:
        case Value::VALTYP_FUNCSCRIPT:
        case Value::VALTYP_FUNCNATIVE:
        case Value::VALTYP_ERROR:
            {
                copy = obj;
            }
            break;
        case Value::VALTYP_STRING:
            {
                size_t len;
                const char* str;
                str = mc_value_stringgetdata(obj);
                len = mc_value_stringgetlength(obj);
                copy = mc_value_makestringlen(state, str, len);
            }
            break;
        case Value::VALTYP_ARRAY:
            {
                int i;
                int len;
                Value item;
                len = mc_value_arraygetlength(obj);
                copy = mc_value_makearraycapacity(state, len);
                if(copy.isNull())
                {
                    return Value::makeNull();
                }
                for(i = 0; i < len; i++)
                {
                    item = mc_value_arraygetvalue(obj, i);
                    ok = mc_value_arraypush(copy, item);
                    if(!ok)
                    {
                        return Value::makeNull();
                    }
                }
            }
            break;
        case Value::VALTYP_MAP:
            {
                int i;
                Value key;
                Value val;
                copy = mc_value_makemap(state);
                for(i = 0; i < mc_value_mapgetlength(obj); i++)
                {
                    key = mc_value_mapgetkeyat(obj, i);
                    val = mc_value_mapgetvalueat(obj, i);
                    ok = mc_value_mapsetvalue(copy, key, val);
                    if(!ok)
                    {
                        return Value::makeNull();
                    }
                }
            }
            break;
        case Value::VALTYP_EXTERNAL:
            {
                void* datacopy;
                mcobjuserdata_t* objext;
                copy = mc_value_makeuserobject(state, nullptr);
                if(copy.isNull())
                {
                    return Value::makeNull();
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

char* mc_fsutil_fileread(const char* filename, size_t* flen)
{
    return mc_util_readfile(filename, flen);
}

size_t mc_fsutil_filewrite(const char* strpath, const char* string, size_t stringsize)
{
    size_t printedsz;
    FILE* fp;
    fp = fopen(strpath, "w");
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
    (void)ok;
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
        linestart = lineend + 1;
        lineend = strstr(linestart, delimiter);
    }
    rest = mc_util_strdup(linestart);
    if(!rest)
    {
        goto err;
    }
    ok = res->push(rest);
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

char* mc_util_joinstringarray(PtrList* items, const char* joinee, size_t jlen)
{
    size_t i;
    char* item;
    Printer* res;
    res = Memory::make<Printer>(nullptr);
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

char* mc_util_canonpath(const char* strpath)
{
    size_t i;
    char* joined;
    char* stritem;
    char* nextitem;
    void* item;
    const char* tmpstr;
    PtrList* split;
    if(!strchr(strpath, '/') || (!strstr(strpath, "/../") && !strstr(strpath, "./")))
    {
        return mc_util_strdup(strpath);
    }
    split = mc_util_splitstring(strpath, "/");
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
    joined = mc_util_joinstringarray(split, tmpstr, strlen(tmpstr));
    for(i = 0; i < split->count(); i++)
    {
        item = split->get(i);
        mc_memory_free(item);
    }
    PtrList::destroy(split, nullptr);
    return joined;
}

bool mc_util_pathisabsolute(const char* strpath)
{
    return strpath[0] == '/';
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



template<typename... ArgsT>
void mc_util_complain(AstLocation pos, const char *fmt, ArgsT&&... args)
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
        if(pos.file->path() != nullptr)
        {
            fname = pos.file->path();
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

MC_FORCEINLINE mcfloat_t mc_mathutil_add(mcfloat_t dnleft, mcfloat_t dnright)
{
    return ((dnleft) + dnright);
}

MC_FORCEINLINE mcfloat_t mc_mathutil_sub(mcfloat_t dnleft, mcfloat_t dnright)
{
    return ((dnleft) - (dnright));
}

MC_FORCEINLINE mcfloat_t mc_mathutil_mult(mcfloat_t dnleft, mcfloat_t dnright)
{
    return ((dnleft) * (dnright));
}

MC_FORCEINLINE mcfloat_t mc_mathutil_div(mcfloat_t dnleft, mcfloat_t dnright)
{
    return ((dnleft) / (dnright));
}

MC_FORCEINLINE mcfloat_t mc_mathutil_mod(mcfloat_t dnleft, mcfloat_t dnright)
{
    return (fmod((dnleft), (dnright)));
}


mcobjdata_t* mc_gcmemory_getdatafrompool(State* state, Value::Type type)
{
    bool ok;
    mcobjdata_t* data;
    GCMemory::DataPool* pool;
    (void)ok;
    pool = GCMemory::DataPool::getPoolForType(state, type);
    if(!pool || pool->count <= 0)
    {
        return nullptr;
    }
    data = pool->data[pool->count - 1];
    MC_ASSERT(state->m_stategcmem->gcobjlistback->count() >= state->m_stategcmem->gcobjlist->count());
    /*
    * we want to make sure that appending to gcobjlistback never fails in sweep
    * so this only reserves space there.
    */
    ok = state->m_stategcmem->gcobjlistback->push(data);
    ok = state->m_stategcmem->gcobjlist->push(data);
    pool->count--;
    return data;
}

Value mc_value_makestrcapacity(State* state, int capacity)
{
    mcobjdata_t* data;
    data = mc_gcmemory_getdatafrompool(state, Value::VALTYP_STRING);
    if(!data)
    {
        data = mc_gcmemory_allocobjectdata(state);
        if(!data)
        {
            return Value::makeNull();
        }
    }
    data->uvobj.valstring.hash = 0;
    data->uvobj.valstring.strbuf = Memory::make<StringBuffer>(capacity);
    return Value::makeDataFrom(Value::VALTYP_STRING, data);
}

template<typename... ArgsT>
Value mc_value_makestrformat(State* state, const char* fmt, ArgsT&&... args)
{
    Value res;
    mcobjdata_t* data;
    data = mc_gcmemory_getdatafrompool(state, Value::VALTYP_STRING);
    res = mc_value_makestrcapacity(state, 0);
    if(res.isNull())
    {
        return Value::makeNull();
    }
    data->uvobj.valstring.strbuf->appendFormat(fmt, args...);
    return res;
}

Value mc_value_makestringlen(State* state, const char* string, size_t len)
{
    bool ok;
    Value res;
    (void)ok;
    res = mc_value_makestrcapacity(state, len);
    if(res.isNull())
    {
        return res;
    }
    ok = mc_value_stringappendlen(res, string, len);
    if(!ok)
    {
        return Value::makeNull();
    }
    return res;
}

Value mc_value_makestring(State* state, const char* string)
{
    return mc_value_makestringlen(state, string, mc_util_strlen(string));
}

Value mc_value_makefuncnative(State* state, const char* name, mcnativefn_t fn, void* data)
{
    mcobjdata_t* obj;
    obj = mc_gcmemory_allocobjectdata(state);
    if(!obj)
    {
        return Value::makeNull();
    }
    obj->uvobj.valfunc.funcdata.valnativefunc.natfnname = mc_util_strdup(name);
    if(!obj->uvobj.valfunc.funcdata.valnativefunc.natfnname)
    {
        return Value::makeNull();
    }
    obj->uvobj.valfunc.funcdata.valnativefunc.natptrfn = fn;
    if(data)
    {
        obj->uvobj.valfunc.funcdata.valnativefunc.userpointer = data;
    }
    return Value::makeDataFrom(Value::VALTYP_FUNCNATIVE, obj);
}

Value mc_value_makearray(State* state)
{
    return mc_value_makearraycapacity(state, 8);
}

Value mc_value_makearraycapacity(State* state, size_t capacity)
{
    mcobjdata_t* data;
    data = mc_gcmemory_getdatafrompool(state, Value::VALTYP_ARRAY);
    if(data)
    {
        data->uvobj.valarray->actualarray->setEmpty();
        return Value::makeDataFrom(Value::VALTYP_ARRAY, data);
    }
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return Value::makeNull();
    }
    data->uvobj.valarray = Memory::make<mcobjarray_t>();
    data->uvobj.valarray->actualarray = Memory::make<GenericList<Value>>(capacity, Value::makeNull());
    if(!data->uvobj.valarray->actualarray)
    {
        return Value::makeNull();
    }
    return Value::makeDataFrom(Value::VALTYP_ARRAY, data);
}

Value mc_value_makemap(State* state)
{
    return mc_value_makemapcapacity(state, 32);
}

Value mc_value_makemapcapacity(State* state, size_t capacity)
{
    mcobjdata_t* data;
    data = mc_gcmemory_getdatafrompool(state, Value::VALTYP_MAP);
    if(data)
    {
        data->uvobj.valmap->actualmap->clear();
        return Value::makeDataFrom(Value::VALTYP_MAP, data);
    }
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return Value::makeNull();
    }
    data->uvobj.valmap = Memory::make<mcobjmap_t>();
    data->uvobj.valmap->actualmap = Memory::make<GenericDict<Value, Value>>(capacity);
    if(!data->uvobj.valmap->actualmap)
    {
        return Value::makeNull();
    }
    data->uvobj.valmap->actualmap->setHashFunction((mcitemhashfn_t)Value::callbackHash);
    data->uvobj.valmap->actualmap->setEqualsFunction((mcitemcomparefn_t)Value::callbackEqualsTo);
    return Value::makeDataFrom(Value::VALTYP_MAP, data);
}

Value mc_value_makeerror(State* state, const char* error)
{
    char* errorstr;
    Value res;
    errorstr = mc_util_strdup(error);
    if(!errorstr)
    {
        return Value::makeNull();
    }
    res = mc_value_makeerrornocopy(state, errorstr);
    if(res.isNull())
    {
        mc_memory_free(errorstr);
        return Value::makeNull();
    }
    return res;
}

Value mc_value_makeerrornocopy(State* state, char* error)
{
    mcobjdata_t* data;
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return Value::makeNull();
    }
    data->uvobj.valerror.message = error;
    data->uvobj.valerror.traceback = nullptr;
    return Value::makeDataFrom(Value::VALTYP_ERROR, data);
}

Value mc_value_makefuncscript(State* state, const char* name, CompiledProgram* cres, bool ownsdt, int nlocals, int nargs, int fvc)
{
    mcobjdata_t* data;
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return Value::makeNull();
    }
    if(ownsdt)
    {
        data->uvobj.valfunc.funcdata.valscriptfunc.unamev.fallocname = name ? mc_util_strdup(name) : mc_util_strdup("anonymous");
        if(!data->uvobj.valfunc.funcdata.valscriptfunc.unamev.fallocname)
        {
            return Value::makeNull();
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
    if(fvc >= MC_CONF_FREEVALSCOUNT)
    {
        data->uvobj.valfunc.funcdata.valscriptfunc.ufv.freevalsallocated = (Value*)mc_memory_malloc(sizeof(Value) * fvc);
        if(!data->uvobj.valfunc.funcdata.valscriptfunc.ufv.freevalsallocated)
        {
            return Value::makeNull();
        }
    }
    data->uvobj.valfunc.funcdata.valscriptfunc.freevalscount = fvc;
    return Value::makeDataFrom(Value::VALTYP_FUNCSCRIPT, data);
}

Value mc_value_makeuserobject(State* state, void* data)
{
    mcobjdata_t* obj;
    obj = mc_gcmemory_allocobjectdata(state);
    if(!obj)
    {
        return Value::makeNull();
    }
    obj->uvobj.valuserobject.data = data;
    obj->uvobj.valuserobject.datadestroyfn = nullptr;
    obj->uvobj.valuserobject.datacopyfn = nullptr;
    return Value::makeDataFrom(Value::VALTYP_EXTERNAL, obj);
}

GenericList<Value>* mc_value_arraygetactualarray(Value object)
{
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_ARRAY);
    data = object.getAllocatedData();
    return data->uvobj.valarray->actualarray;
}

MC_INLINE char* mc_value_stringgetdataintern(Value object)
{
    mcobjdata_t* data;
    data = object.getAllocatedData();
    MC_ASSERT(data->odtype == Value::VALTYP_STRING);
    return data->uvobj.valstring.strbuf->data();
}

mcobjuserdata_t* mc_value_userdatagetdata(Value object)
{
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_EXTERNAL);
    data = object.getAllocatedData();
    return &data->uvobj.valuserobject;
}

bool mc_value_userdatasetdata(Value object, void* extdata)
{
    mcobjuserdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_EXTERNAL);
    data = mc_value_userdatagetdata(object);
    if(!data)
    {
        return false;
    }
    data->data = extdata;
    return true;
}

bool mc_value_userdatasetdestroyfunction(Value object, mcitemdestroyfn_t dfn)
{
    mcobjuserdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_EXTERNAL);
    data = mc_value_userdatagetdata(object);
    if(!data)
    {
        return false;
    }
    data->datadestroyfn = dfn;
    return true;
}

bool mc_value_userdatasetcopyfunction(Value object, mcitemcopyfn_t copyfn)
{
    mcobjuserdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_EXTERNAL);
    data = mc_value_userdatagetdata(object);
    if(!data)
    {
        return false;
    }
    data->datacopyfn = copyfn;
    return true;
}


MC_INLINE const char* mc_value_stringgetdata(Value object)
{
    MC_ASSERT(object.getType() == Value::VALTYP_STRING);
    return mc_value_stringgetdataintern(object);
}

int mc_value_stringgetlength(Value object)
{
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_STRING);
    data = object.getAllocatedData();
    return data->uvobj.valstring.strbuf->length();
}

void mc_value_stringsetlength(Value object, int len)
{
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_STRING);
    data = object.getAllocatedData();
    data->uvobj.valstring.strbuf->setLength(len);
    mc_value_stringrehash(object);
}


MC_INLINE char* mc_value_stringgetmutabledata(Value object)
{
    MC_ASSERT(object.getType() == Value::VALTYP_STRING);
    return mc_value_stringgetdataintern(object);
}

bool mc_value_stringappendlen(Value obj, const char* src, size_t len)
{
    mcobjdata_t* data;
    mcobjstring_t* string;
    MC_ASSERT(obj.getType() == Value::VALTYP_STRING);
    data = obj.getAllocatedData();
    string = &data->uvobj.valstring;
    string->strbuf->append(src, len);
    mc_value_stringrehash(obj);
    return true;
}

bool mc_value_stringappend(Value obj, const char* src)
{
    return mc_value_stringappendlen(obj, src, strlen(src));
}

template<typename... ArgsT>
bool mc_value_stringappendformat(Value obj, const char* fmt, ArgsT&&... args)
{
    mcobjdata_t* data;
    mcobjstring_t* string;
    MC_ASSERT(obj.getType() == Value::VALTYP_STRING);
    data = obj.getAllocatedData();
    string = &data->uvobj.valstring;
    string->strbuf->appendFormat(fmt, args...);
    mc_value_stringrehash(obj);
    return true;
}


bool mc_value_stringappendvalue(Value destval, Value val)
{
    bool ok;
    int vlen;
    const char* vstr;
    (void)ok;
    if(val.getType() == Value::VALTYP_NUMBER)
    {
        mc_value_stringappendformat(destval, "%g", Value::asNumber(val));
        goto finished;
    }
    if(val.getType() == Value::VALTYP_STRING)
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

size_t mc_value_stringgethash(Value obj)
{
    size_t len;
    const char* str;
    mcobjdata_t* data;
    MC_ASSERT(obj.getType() == Value::VALTYP_STRING);
    data = obj.getAllocatedData();
    if(data->uvobj.valstring.hash == 0)
    {
        len = mc_value_stringgetlength(obj);
        str = mc_value_stringgetdata(obj);
        data->uvobj.valstring.hash = mc_util_hashdata(str, len);
    }
    return data->uvobj.valstring.hash;
}

bool mc_value_stringrehash(Value obj)
{
    size_t len;
    const char* str;
    mcobjdata_t* data;
    MC_ASSERT(obj.getType() == Value::VALTYP_STRING);
    data = obj.getAllocatedData();
    len = mc_value_stringgetlength(obj);
    str = mc_value_stringgetdata(obj);
    data->uvobj.valstring.hash = mc_util_hashdata(str, len);
    return true;
}



const char* mc_value_functiongetname(Value obj)
{
    mcobjdata_t* data;
    MC_ASSERT(obj.getType() == Value::VALTYP_FUNCSCRIPT);
    data = obj.getAllocatedData();
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

Value mc_value_functiongetfreevalat(Value obj, int ix)
{
    mcobjdata_t* data;
    mcobjfunction_t* fun;
    MC_ASSERT(obj.getType() == Value::VALTYP_FUNCSCRIPT);
    data = obj.getAllocatedData();
    MC_ASSERT(data);
    if(!data)
    {
        return Value::makeNull();
    }
    fun = &data->uvobj.valfunc;
    MC_ASSERT(ix >= 0 && ix < fun->funcdata.valscriptfunc.freevalscount);
    if(ix < 0 || ix >= fun->funcdata.valscriptfunc.freevalscount)
    {
        return Value::makeNull();
    }
    if(fun->freeValuesAreAllocated())
    {
        return fun->funcdata.valscriptfunc.ufv.freevalsallocated[ix];
    }
    return fun->funcdata.valscriptfunc.ufv.freevalsstack[ix];
}

void mc_value_functionsetfreevalat(Value obj, int ix, Value val)
{
    mcobjdata_t* data;
    mcobjfunction_t* fun;
    MC_ASSERT(obj.getType() == Value::VALTYP_FUNCSCRIPT);
    data = obj.getAllocatedData();
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

Value* mc_value_functiongetfreevals(Value obj)
{
    mcobjdata_t* data;
    mcobjfunction_t* fun;
    MC_ASSERT(obj.getType() == Value::VALTYP_FUNCSCRIPT);
    data = obj.getAllocatedData();
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

const char* mc_value_errorgetmessage(Value object)
{
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_ERROR);
    data = object.getAllocatedData();
    return data->uvobj.valerror.message;
}

void mc_value_errorsettraceback(Value object, Traceback* traceback)
{
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_ERROR);
    if(object.getType() == Value::VALTYP_ERROR)
    {
        data = object.getAllocatedData();
        MC_ASSERT(data->uvobj.valerror.traceback == nullptr);
        data->uvobj.valerror.traceback = traceback;
    }
}

Traceback* mc_value_errorgettraceback(Value object)
{
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_ERROR);
    data = object.getAllocatedData();
    return data->uvobj.valerror.traceback;
}

Value mc_value_arraygetvalue(Value object, size_t ix)
{
    Value* res;
    GenericList<Value>* array;
    MC_ASSERT(object.getType() == Value::VALTYP_ARRAY);
    array = mc_value_arraygetactualarray(object);
    if(ix >= array->count())
    {
        return Value::makeNull();
    }
    res = (Value*)array->getp(ix);
    if(!res)
    {
        return Value::makeNull();
    }
    return *res;
}

bool mc_value_arraysetvalue(Value object, size_t ix, Value val)
{
    size_t len;
    size_t toadd;
    GenericList<Value>* array;
    MC_ASSERT(object.getType() == Value::VALTYP_ARRAY);
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
            mc_value_arraypush(object, Value::makeNull());
            toadd++;
        }
    }
    return array->set(ix, val);
}

bool mc_value_arraypush(Value object, Value val)
{
    GenericList<Value>* array;
    MC_ASSERT(object.getType() == Value::VALTYP_ARRAY);
    array = mc_value_arraygetactualarray(object);
    return array->push(val);
}

int mc_value_arraygetlength(Value object)
{
    GenericList<Value>* array;
    MC_ASSERT(object.getType() == Value::VALTYP_ARRAY);
    array = mc_value_arraygetactualarray(object);
    return array->count();
}

Value mc_valarray_pop(Value object)
{
    Value dest;
    GenericList<Value>* array;
    MC_ASSERT(object.getType() == Value::VALTYP_ARRAY);
    array = mc_value_arraygetactualarray(object);
    if(array->pop(&dest))
    {
        return dest;
    }
    return Value::makeNull();
}

bool mc_value_arrayremoveat(Value object, int ix)
{
    GenericList<Value>* array;
    array = mc_value_arraygetactualarray(object);
    return array->removeAt(ix);
}

int mc_value_mapgetlength(Value object)
{
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    return data->uvobj.valmap->actualmap->count();
}

Value mc_value_mapgetkeyat(Value object, int ix)
{
    Value* res;
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    res = (Value*)data->uvobj.valmap->actualmap->getKeyAt(ix);
    if(!res)
    {
        return Value::makeNull();
    }
    return *res;
}

Value mc_value_mapgetvalueat(Value object, int ix)
{
    Value* res;
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    res = (Value*)data->uvobj.valmap->actualmap->getValueAt(ix);
    if(!res)
    {
        return Value::makeNull();
    }
    return *res;
}

bool mc_value_mapsetvalueat(Value object, int ix, Value val)
{
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    if(ix >= mc_value_mapgetlength(object))
    {
        return false;
    }
    data = object.getAllocatedData();
    return data->uvobj.valmap->actualmap->setValueAt(ix, &val);
}

Value mc_value_mapgetkvpairat(State* state, Value object, int ix)
{
    Value key;
    Value val;
    Value res;
    Value valobj;
    Value keyobj;
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    if(ix >= data->uvobj.valmap->actualmap->count())
    {
        return Value::makeNull();
    }
    key = mc_value_mapgetkeyat(object, ix);
    val = mc_value_mapgetvalueat(object, ix);
    res = mc_value_makemap(state);
    if(res.isNull())
    {
        return Value::makeNull();
    }
    keyobj = mc_value_makestring(state, "key");
    if(keyobj.isNull())
    {
        return Value::makeNull();
    }
    mc_value_mapsetvalue(res, keyobj, key);
    valobj = mc_value_makestring(state, "value");
    if(valobj.isNull())
    {
        return Value::makeNull();
    }
    mc_value_mapsetvalue(res, valobj, val);
    return res;
}

bool mc_value_mapsetvalue(Value object, Value key, Value val)
{
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    return data->uvobj.valmap->actualmap->setKV(&key, &val);
}

bool mc_value_mapsetvaluestring(Value object, const char* strkey, Value val)
{
    State* state;
    Value vkey;
    state = object.getAllocatedData()->m_pstate;
    vkey = mc_value_makestring(state, strkey);
    return mc_value_mapsetvalue(object, vkey, val);
}

void mc_value_mapsetstrfunc(Value map, const char* fnname, mcnativefn_t function)
{
    State* state;
    state = map.getAllocatedData()->m_pstate;
    mc_value_mapsetvaluestring(map, fnname, mc_value_makefuncnative(state, fnname, function, nullptr));
}

Value mc_value_mapgetvalue(Value object, Value key)
{
    Value* res;
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    res = (Value*)data->uvobj.valmap->actualmap->get(&key);
    if(!res)
    {
        return Value::makeNull();
    }
    return *res;
}

bool mc_value_mapgetvaluechecked(Value object, Value key, Value* dest)
{
    Value* res;
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    res = (Value*)data->uvobj.valmap->actualmap->get(&key);
    if(!res)
    {
        *dest = Value::makeNull();
        return false;
    }
    *dest = *res;
    return true;
}

bool mc_value_maphaskey(Value object, Value key)
{
    Value* res;
    mcobjdata_t* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    res = (Value*)data->uvobj.valmap->actualmap->get(&key);
    return res != nullptr;
}

void mc_objectdata_deinit(mcobjdata_t* data)
{
    switch(data->odtype)
    {
        case Value::VALTYP_FREED:
            {
                MC_ASSERT(false);
            }
            break;
        case Value::VALTYP_STRING:
            {
                StringBuffer::destroy(data->uvobj.valstring.strbuf);
            }
            break;
        case Value::VALTYP_FUNCSCRIPT:
            {
                if(data->uvobj.valfunc.funcdata.valscriptfunc.ownsdata)
                {
                    mc_memory_free(data->uvobj.valfunc.funcdata.valscriptfunc.unamev.fallocname);
                    CompiledProgram::destroy(data->uvobj.valfunc.funcdata.valscriptfunc.compiledprogcode);
                }
                if(data->uvobj.valfunc.freeValuesAreAllocated())
                {
                    mc_memory_free(data->uvobj.valfunc.funcdata.valscriptfunc.ufv.freevalsallocated);
                }
            }
            break;
        case Value::VALTYP_ARRAY:
            {
                Memory::destroy(data->uvobj.valarray->actualarray);
                Memory::destroy(data->uvobj.valarray);
            }
            break;
        case Value::VALTYP_MAP:
            {
                Memory::destroy(data->uvobj.valmap->actualmap);
                Memory::destroy(data->uvobj.valmap);
            }
            break;
        case Value::VALTYP_FUNCNATIVE:
            {
                mc_memory_free(data->uvobj.valfunc.funcdata.valnativefunc.natfnname);
            }
            break;
        case Value::VALTYP_EXTERNAL:
            {
                if(data->uvobj.valuserobject.datadestroyfn)
                {
                    data->uvobj.valuserobject.datadestroyfn(data->uvobj.valuserobject.data);
                }
            }
            break;
        case Value::VALTYP_ERROR:
            {
                mc_memory_free(data->uvobj.valerror.message);
                Memory::destroy(data->uvobj.valerror.traceback);
            }
            break;
        default:
            {
            }
            break;
    }
    data->odtype = Value::VALTYP_FREED;
}



template<typename TypeKeyT, typename TypeValueT>
Value mc_value_copydeepfuncscript(State* state, Value obj, GenericDict<TypeKeyT, TypeValueT>* targetdict)
{
    bool ok;
    int i;
    uint16_t* bytecodecopy;
    mcobjfunction_t* functioncopy;
    Value copy;
    Value freeval;
    Value freevalcopy;
    mcobjfunction_t* function;
    AstLocation* srcpositionscopy;
    CompiledProgram* comprescopy;
    (void)ok;
    function = Value::asFunction(obj);
    bytecodecopy = nullptr;
    srcpositionscopy = nullptr;
    comprescopy = nullptr;
    bytecodecopy = (uint16_t*)mc_memory_malloc(sizeof(uint16_t) * function->funcdata.valscriptfunc.compiledprogcode->count);
    if(!bytecodecopy)
    {
        return Value::makeNull();
    }
    memcpy(bytecodecopy, function->funcdata.valscriptfunc.compiledprogcode->bytecode, sizeof(uint16_t) * function->funcdata.valscriptfunc.compiledprogcode->count);
    srcpositionscopy = (AstLocation*)mc_memory_malloc(sizeof(AstLocation) * function->funcdata.valscriptfunc.compiledprogcode->count);
    if(!srcpositionscopy)
    {
        mc_memory_free(bytecodecopy);
        return Value::makeNull();
    }
    memcpy(srcpositionscopy, function->funcdata.valscriptfunc.compiledprogcode->progsrcposlist, sizeof(AstLocation) * function->funcdata.valscriptfunc.compiledprogcode->count);
    comprescopy = Memory::make<CompiledProgram>(bytecodecopy, srcpositionscopy, function->funcdata.valscriptfunc.compiledprogcode->count);
    /*
    * todo: add compilation result copy function
    */
    if(!comprescopy)
    {
        mc_memory_free(srcpositionscopy);
        mc_memory_free(bytecodecopy);
        return Value::makeNull();
    }
    copy = mc_value_makefuncscript(state, mc_value_functiongetname(obj), comprescopy, true, function->funcdata.valscriptfunc.numlocals, function->funcdata.valscriptfunc.numargs, 0);
    if(copy.isNull())
    {
        CompiledProgram::destroy(comprescopy);
        return Value::makeNull();
    }
    ok = targetdict->setKV(&obj, &copy);
    if(!ok)
    {
        return Value::makeNull();
    }
    functioncopy = Value::asFunction(copy);
    if(function->freeValuesAreAllocated())
    {
        functioncopy->funcdata.valscriptfunc.ufv.freevalsallocated = (Value*)mc_memory_malloc(sizeof(Value) * function->funcdata.valscriptfunc.freevalscount);
        if(!functioncopy->funcdata.valscriptfunc.ufv.freevalsallocated)
        {
            return Value::makeNull();
        }
    }
    functioncopy->funcdata.valscriptfunc.freevalscount = function->funcdata.valscriptfunc.freevalscount;
    for(i = 0; i < function->funcdata.valscriptfunc.freevalscount; i++)
    {
        freeval = mc_value_functiongetfreevalat(obj, i);
        freevalcopy = Value::copyDeepIntern(state, freeval, targetdict);
        if(!freeval.isNull() && freevalcopy.isNull())
        {
            return Value::makeNull();
        }
        mc_value_functionsetfreevalat(copy, i, freevalcopy);
    }
    return copy;
}

template<typename TypeKeyT, typename TypeValueT>
Value mc_value_copydeeparray(State* state, Value obj, GenericDict<TypeKeyT, TypeValueT>* targetdict)
{
    bool ok;
    int i;
    int len;
    Value copy;
    Value item;
    Value itemcopy;
    (void)ok;
    len = mc_value_arraygetlength(obj);
    copy = mc_value_makearraycapacity(state, len);
    if(copy.isNull())
    {
        return Value::makeNull();
    }
    ok = targetdict->setKV(&obj, &copy);
    if(!ok)
    {
        return Value::makeNull();
    }
    for(i = 0; i < len; i++)
    {
        item = mc_value_arraygetvalue(obj, i);
        itemcopy = Value::copyDeepIntern(state, item, targetdict);
        if(!item.isNull() && itemcopy.isNull())
        {
            return Value::makeNull();
        }
        ok = mc_value_arraypush(copy, itemcopy);
        if(!ok)
        {
            return Value::makeNull();
        }
    }
    return copy;
}

template<typename TypeKeyT, typename TypeValueT>
Value mc_value_copydeepmap(State* state, Value obj, GenericDict<TypeKeyT, TypeValueT>* targetdict)
{
    bool ok;
    int i;
    Value key;
    Value val;
    Value copy;
    Value keycopy;
    Value valcopy;
    (void)ok;
    copy = mc_value_makemap(state);
    if(copy.isNull())
    {
        return Value::makeNull();
    }
    ok = targetdict->setKV(&obj, &copy);
    if(!ok)
    {
        return Value::makeNull();
    }
    for(i = 0; i < mc_value_mapgetlength(obj); i++)
    {
        key = mc_value_mapgetkeyat(obj, i);
        val = mc_value_mapgetvalueat(obj, i);
        keycopy = Value::copyDeepIntern(state, key, targetdict);
        if(!key.isNull() && keycopy.isNull())
        {
            return Value::makeNull();
        }
        valcopy = Value::copyDeepIntern(state, val, targetdict);
        if(!val.isNull() && valcopy.isNull())
        {
            return Value::makeNull();
        }
        ok = mc_value_mapsetvalue(copy, keycopy, valcopy);
        if(!ok)
        {
            return Value::makeNull();
        }
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

void mc_printer_printoneinstruc(Printer* pr, uint16_t* code, uint16_t op, size_t* pos, AstLocation* sposlist, bool simple)
{
    bool ok;
    int i;
    mcfloat_t dval;
    uint64_t operands[2];
    mcopdefinition_t* def;
    mcopdefinition_t vdef;
    AstLocation srcpos;
    (void)ok;
    def = AstCompiler::opdefLookup(&vdef, op);
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

void mc_printer_printbytecode(Printer* pr, uint16_t* code, AstLocation* sposlist, size_t codesize, bool simple)
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

void mc_printer_printobjstring(Printer* pr, Value obj)
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

void mc_printer_printobjfuncscript(Printer* pr, Value obj)
{
    const char* fname;
    mcobjfunction_t* fn;
    fn = Value::asFunction(obj);
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

void mc_printer_printobjarray(Printer* pr, Value obj)
{
    bool recursion;
    size_t i;
    size_t alen;
    bool prevquot;
    Value iobj;
    GenericList<Value>* actualary;
    GenericList<Value>* otherary;
    actualary = mc_value_arraygetactualarray(obj);
    alen = mc_value_arraygetlength(obj);
    pr->put("[");
    for(i = 0; i < alen; i++)
    {
        recursion = false;
        iobj = mc_value_arraygetvalue(obj, i);
        if(iobj.getType() == Value::VALTYP_ARRAY)
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

void mc_printer_printobjmap(Printer* pr, Value obj)
{
    bool prevquot;
    size_t i;
    size_t alen;
    Value key;
    Value val;
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


void mc_printer_printobjerror(Printer* pr, Value obj)
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

void mc_printer_printvalue(Printer* pr, Value obj, bool accurate)
{
    Value::Type type;
    (void)accurate;
    type = obj.getType();
    switch(type)
    {
        case Value::VALTYP_FREED:
            {
                pr->put("FREED");
            }
            break;
        case Value::VALTYP_NONE:
            {
                pr->put("NONE");
            }
            break;
        case Value::VALTYP_NUMBER:
            {
                mcfloat_t number;
                number = Value::asNumber(obj);
                pr->printNumFloat(number);
            }
            break;
        case Value::VALTYP_BOOL:
            {
                pr->put(Value::asBool(obj) ? "true" : "false");
            }
            break;
        case Value::VALTYP_STRING:
            {
                mc_printer_printobjstring(pr, obj);
            }
            break;
        case Value::VALTYP_NULL:
            {
                pr->put("null");
            }
            break;
        case Value::VALTYP_FUNCSCRIPT:
            {
                mc_printer_printobjfuncscript(pr, obj);
            }
            break;
        case Value::VALTYP_ARRAY:
            {
                mc_printer_printobjarray(pr, obj);
            }
            break;
        case Value::VALTYP_MAP:
            {
                mc_printer_printobjmap(pr, obj);
            }
            break;
        case Value::VALTYP_FUNCNATIVE:
            {
                pr->put("FUNCNATIVE");
            }
            break;
        case Value::VALTYP_EXTERNAL:
            {
                pr->put("EXTERNAL");
            }
            break;
        case Value::VALTYP_ERROR:
            {
                mc_printer_printobjerror(pr, obj);
            }
            break;
        case Value::VALTYP_ANY:
            {
                MC_ASSERT(false);
            }
            break;
    }
}





bool mc_error_printusererror(Printer* pr, Value obj)
{
    const char* cred;
    const char* creset;
    mcconsolecolor_t mcc;
    Traceback* traceback;
    mc_consolecolor_init(&mcc, fileno(stdout));
    cred = mc_consolecolor_get(&mcc, 'r');
    creset = mc_consolecolor_get(&mcc, '0');
    pr->format("%sERROR:%s %s\n", cred, creset, mc_value_errorgetmessage(obj));
    traceback = mc_value_errorgettraceback(obj);
    MC_ASSERT(traceback != nullptr);
    if(traceback)
    {
        pr->format("%sTraceback:%s\n", cred, creset);
        traceback->printTo(pr, &mcc);
    }
    return true;
}



bool mc_vm_runexecfunc(State* state, CompiledProgram* comp_res, GenericList<Value>* constants);

Value mc_program_execute(State* state, CompiledProgram* program)
{
    bool ok;
    Value res;
    (void)ok;
    if(program == nullptr)
    {
        state->m_errorlist.pushFormat(MC_ERROR_USER, AstLocation::Invalid(), "program passed to execute was null.");
        return Value::makeNull();
    }
    state->reset();
    ok = mc_vm_runexecfunc(state, program, state->compiler->getconstants());
    if(!ok || state->m_errorlist.count() > 0)
    {
        return Value::makeNull();
    }
    //MC_ASSERT(state->execstate.vsposition == 0);
    res = state->execstate.lastpopped;
    if(res.getType() == Value::VALTYP_NONE)
    {
        return Value::makeNull();
    }
    return res;
}



Value mc_state_execcode(State* state, const char* code, const char* filename)
{
    bool ok;
    Value res;
    CompiledProgram* compres;
    (void)ok;
    state->reset();
    compres = state->compiler->compilesource(code, filename);
    if(!compres || state->m_errorlist.count() > 0)
    {
        goto err;
    }
    ok = mc_vm_runexecfunc(state, compres, state->compiler->getconstants());
    if(!ok || state->m_errorlist.count() > 0)
    {
        goto err;
    }
    MC_ASSERT(state->execstate.vsposition == 0);
    res = state->execstate.lastpopped;
    if(res.getType() == Value::VALTYP_NONE)
    {
        goto err;
    }
    CompiledProgram::destroy(compres);
    return res;
err:
    CompiledProgram::destroy(compres);
    return Value::makeNull();
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
    bool mc_argcheck_check(State* state, bool generateerror, size_t argc, Value* args, ...)
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
            mc_argcheck_checkactual((state), (generateerror), (argc), (args), sizeof((Value::Type[]){ __VA_ARGS__ }) / sizeof(Value::Type), (Value::Type[]){ __VA_ARGS__ })
    #endif
#endif

bool mc_argcheck_checkactual(State* state, bool generateerror, size_t argc, Value* args, size_t expectedargc, const Value::Type* expectedtypes)
{
    size_t i;
    char* expectedtypestr;
    const char* typestr;
    Value::Type type;
    Value::Type expectedtype;
    Value arg;
    if(argc != expectedargc)
    {
        if(generateerror)
        {
            state->pushError(MC_ERROR_RUNTIME, AstLocation::Invalid(), "Invalid number or arguments, got %d instead of %d", argc, expectedargc);
        }
        return false;
    }
    for(i = 0; i < argc; i++)
    {
        arg = args[i];
        type = arg.getType();
        expectedtype = expectedtypes[i];
        if(!(type & expectedtype))
        {
            if(generateerror)
            {
                typestr = Value::getTypename(type);
                expectedtypestr = Value::getUnionName(expectedtype);
                if(!expectedtypestr)
                {
                    return false;
                }
                state->pushError(MC_ERROR_RUNTIME, AstLocation::Invalid(), "Invalid argument %d type, got %s, expected %s", i, typestr, expectedtypestr);
                mc_memory_free(expectedtypestr);
            }
            return false;
        }
    }
    return true;
}





mcobjdata_t* mc_gcmemory_allocobjectdata(State* state)
{
    bool ok;
    mcobjdata_t* data;
    (void)ok;
    data = nullptr;
    state->m_stategcmem->allocssincesweep++;
    if(state->m_stategcmem->onlydatapool.count > 0)
    {
        data = state->m_stategcmem->onlydatapool.data[state->m_stategcmem->onlydatapool.count - 1];
        state->m_stategcmem->onlydatapool.count--;
    }
    else
    {
        data = Memory::make<mcobjdata_t>();
    }
    memset(data, 0, sizeof(mcobjdata_t));
    data->m_pstate = state;
    MC_ASSERT(state->m_stategcmem->gcobjlistback->count() >= state->m_stategcmem->gcobjlist->count());
    /*
    * we want to make sure that appending to gcobjlistback never fails in sweep
    * so this only reserves space there.
    */
    ok = state->m_stategcmem->gcobjlistback->push(data);
    if(!ok)
    {
        Memory::destroy(data);
        return nullptr;
    }
    ok = state->m_stategcmem->gcobjlist->push(data);
    if(!ok)
    {
        Memory::destroy(data);
        return nullptr;
    }
    data->m_objmem = state->m_stategcmem;
    return data;
}


void mc_state_gcunmarkall(State* state)
{
    size_t i;
    mcobjdata_t* data;
    for(i = 0; i < state->m_stategcmem->gcobjlist->count(); i++)
    {
        data = (mcobjdata_t*)state->m_stategcmem->gcobjlist->get(i);
        data->gcmark = false;
    }
}

void mc_state_gcmarkobjlist(Value* objects, size_t count)
{
    size_t i;
    Value obj;
    for(i = 0; i < count; i++)
    {
        obj = objects[i];
        mc_state_gcmarkobject(obj);
    }
}

void mc_state_gcmarkobject(Value obj)
{
    int i;
    int len;
    Value key;
    Value val;
    Value freeval;
    mcobjdata_t* data;
    mcobjdata_t* valdata;
    mcobjdata_t* keydata;
    mcobjdata_t* freevaldata;
    mcobjfunction_t* function;
    if(obj.isAllocated())
    {
        data = obj.getAllocatedData();
        if(!data->gcmark)
        {
            data->gcmark = true;
            switch(obj.getType())
            {
                case Value::VALTYP_MAP:
                    {
                        len = mc_value_mapgetlength(obj);
                        for(i = 0; i < len; i++)
                        {
                            key = mc_value_mapgetkeyat(obj, i);
                            if(key.isAllocated())
                            {
                                keydata = key.getAllocatedData();
                                if(!keydata->gcmark)
                                {
                                    mc_state_gcmarkobject(key);
                                }
                            }
                            val = mc_value_mapgetvalueat(obj, i);
                            if(val.isAllocated())
                            {
                                valdata = val.getAllocatedData();
                                if(!valdata->gcmark)
                                {
                                    mc_state_gcmarkobject(val);
                                }
                            }
                        }
                    }
                    break;
                case Value::VALTYP_ARRAY:
                    {
                        len = mc_value_arraygetlength(obj);
                        for(i = 0; i < len; i++)
                        {
                            val = mc_value_arraygetvalue(obj, i);
                            if(val.isAllocated())
                            {
                                valdata = val.getAllocatedData();
                                if(!valdata->gcmark)
                                {
                                    mc_state_gcmarkobject(val);
                                }
                            }
                        }
                    }
                    break;
                case Value::VALTYP_FUNCSCRIPT:
                    {
                        break;
                        function = Value::asFunction(obj);
                        for(i = 0; i < function->funcdata.valscriptfunc.freevalscount; i++)
                        {
                            freeval = mc_value_functiongetfreevalat(obj, i);
                            mc_state_gcmarkobject(freeval);
                            if(freeval.isAllocated())
                            {
                                freevaldata = freeval.getAllocatedData();
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

void mc_state_gcsweep(State* state)
{
    bool ok;
    size_t i;
    mcobjdata_t* data;
    PtrList* objstemp;
    GCMemory::DataPool* pool;
    (void)ok;
    mc_state_gcmarkobjlist((Value*)state->m_stategcmem->gcobjlistremains->data(), state->m_stategcmem->gcobjlistremains->count());
    MC_ASSERT(state->m_stategcmem->gcobjlistback->count() >= state->m_stategcmem->gcobjlist->count());
    state->m_stategcmem->gcobjlistback->clear();
    for(i = 0; i < state->m_stategcmem->gcobjlist->count(); i++)
    {
        data = (mcobjdata_t*)state->m_stategcmem->gcobjlist->get(i);
        if(data->gcmark)
        {
            /*
            * this should never fail because gcobjlistback's size should be equal to objects
            */
            ok = state->m_stategcmem->gcobjlistback->push(data);
            (void)ok;
            MC_ASSERT(ok);
        }
        else
        {
            if(mc_state_gccandatabeputinpool(state, data))
            {
                pool = GCMemory::DataPool::getPoolForType(state, data->odtype);
                pool->data[pool->count] = data;
                pool->count++;
            }
            else
            {
                mc_objectdata_deinit(data);
                if(state->m_stategcmem->onlydatapool.count < MC_CONF_GCMEMPOOLSIZE)
                {
                    state->m_stategcmem->onlydatapool.data[state->m_stategcmem->onlydatapool.count] = data;
                    state->m_stategcmem->onlydatapool.count++;
                }
                else
                {
                    Memory::destroy(data);
                    data = nullptr;
                }
            }
        }
    }
    objstemp = state->m_stategcmem->gcobjlist;
    state->m_stategcmem->gcobjlist = state->m_stategcmem->gcobjlistback;
    state->m_stategcmem->gcobjlistback = objstemp;
    state->m_stategcmem->allocssincesweep = 0;
}

int mc_state_gcshouldsweep(State* state)
{
    return state->m_stategcmem->allocssincesweep > MC_CONF_GCMEMSWEEPINTERVAL;
}


bool mc_state_gcdisablefor(Value obj)
{
    bool ok;
    mcobjdata_t* data;
    (void)ok;
    if(!obj.isAllocated())
    {
        return false;
    }
    data = obj.getAllocatedData();
    if(data->m_objmem->gcobjlistremains->contains(&obj))
    {
        return false;
    }
    ok = data->m_objmem->gcobjlistremains->push(&obj);
    return ok;
}

void mc_state_gcenablefor(Value obj)
{
    mcobjdata_t* data;
    if(obj.isAllocated())
    {
        data = obj.getAllocatedData();
        data->m_objmem->gcobjlistremains->removeItem(&obj);
    }
}


bool mc_state_gccandatabeputinpool(State* state, mcobjdata_t* data)
{
    Value obj;
    GCMemory::DataPool* pool;
    obj = Value::makeDataFrom(data->odtype, data);
    /*
    * this is to ensure that large objects won't be kept in pool indefinitely
    */
    switch(data->odtype)
    {
        case Value::VALTYP_ARRAY:
            {
                if(mc_value_arraygetlength(obj) > 1024)
                {
                    return false;
                }
            }
            break;
        case Value::VALTYP_MAP:
            {
                if(mc_value_mapgetlength(obj) > 1024)
                {
                    return false;
                }
            }
            break;
        case Value::VALTYP_STRING:
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
    pool= GCMemory::DataPool::getPoolForType(state, data->odtype);
    if(!pool || pool->count >= MC_CONF_GCMEMPOOLSIZE)
    {
        return false;
    }
    return true;
}



bool mc_traceback_vmpush(Traceback* traceback, State* state)
{
    bool ok;
    int i;
    VMFrame* frame;
    (void)ok;
    for(i = state->execstate.framestack->count() - 1; i >= 0; i--)
    {
        frame = state->execstate.framestack->getp(i);
        ok = traceback->push(mc_value_functiongetname(frame->function), frame->getPosition());
        if(!ok)
        {
            return false;
        }
    }
    return true;
}



bool mc_vm_init(State* state)
{
    int i;
    Value keyobj;
    state->hadrecovered = false;
    state->globalvalcount = 0;
    state->execstate.vsposition = 0;
    state->execstate.thisstpos = 0;
    state->execstate.lastpopped = Value::makeNull();
    state->running = false;
    for(i = 0; i < MC_CONF_MAXOPEROVERLOADS; i++)
    {
        state->operoverloadkeys[i] = Value::makeNull();
    }
#define SET_OPERATOR_OVERLOAD_KEY(op, key)\
    do\
    {\
        keyobj = mc_value_makestring(state, key);\
        if(keyobj.isNull())\
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

void mc_vm_reset(State* state)
{
    state->execstate.vsposition = 0;
    state->execstate.thisstpos = 0;
    while(state->execstate.framestack->m_listcount > 0)
    {
        state->vmPopFrame();
    }
}


bool mc_vm_runexecfunc(State* state, CompiledProgram* comp_res, GenericList<Value>* constants)
{
    bool res;
    size_t oldsp;
    size_t oldthissp;
    size_t oldframescount;
    Value mainfn;
    (void)oldsp;
    oldsp = state->execstate.vsposition;
    oldthissp = state->execstate.thisstpos;
    oldframescount = state->execstate.framestack->count();
    mainfn = mc_value_makefuncscript(state, "__main__", comp_res, false, 0, 0, 0);
    if(mainfn.isNull())
    {
        return false;
    }
    state->vmStackPush(mainfn);
    res = mc_vm_execvm(state, mainfn, constants, false);
    while(state->execstate.framestack->count() > oldframescount)
    {
        state->vmPopFrame();
    }
    //MC_ASSERT(state->execstate.vsposition == oldsp);
    state->execstate.thisstpos = oldthissp;
    return res;
}




MC_INLINE void mc_vm_rungc(State* state, GenericList<Value>* constants)
{
    size_t i;
    VMFrame* frame;
    mc_state_gcunmarkall(state);
    mc_state_gcmarkobjlist(state->vmglobalstore->getData(), state->vmglobalstore->getCount());
    mc_state_gcmarkobjlist((Value*)constants->data(), constants->count());
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


MC_FORCEINLINE bool mc_vmdo_callobject(State* state, Value callee, int nargs)
{
    bool ok;
    const char* calleetypename;
    Value::Type calleetype;
    VMFrame calleeframe;
    Value res;
    Value tmpval;
    Value selfval;
    Value* stackpos;
    mcobjfunction_t* calleefunction;
    (void)ok;
    calleetype = callee.getType();
    selfval = Value::makeNull();
    if(callee.isFuncNative())
    {
        if(!state->execstate.nativethisstack->pop(&tmpval))
        {
            #if 0
                state->stderrprinter->format("failed to pop native 'this' for = <");
                mc_printer_printvalue(state->stderrprinter, callee, true);
                state->stderrprinter->format(">\n");
                #if 0
                    state->m_errorlist.pushFormat(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "failed to pop native 'this'");
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
    if(calleetype == Value::VALTYP_FUNCSCRIPT)
    {
        calleefunction = Value::asFunction(callee);
        if(nargs != calleefunction->funcdata.valscriptfunc.numargs)
        {
            #if 0
            state->m_errorlist.pushFormat(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "invalid number of arguments to \"%s\": expected %d, got %d",
                              mc_value_functiongetname(callee), calleefunction->funcdata.valscriptfunc.numargs, nargs);
            return false;
            #endif
        }
        ok = VMFrame::init(&calleeframe, callee, state->execstate.vsposition - nargs);
        if(!ok)
        {
            state->m_errorlist.pushFormat(MC_ERROR_RUNTIME, AstLocation::Invalid(), "frame init failed in mc_vmdo_callobject");
            return false;
        }
        ok = state->vmPushFrame(calleeframe);
        if(!ok)
        {
            state->m_errorlist.pushFormat(MC_ERROR_RUNTIME, AstLocation::Invalid(), "pushing frame failed in mc_vmdo_callobject");
            return false;
        }
    }
    else if(calleetype == Value::VALTYP_FUNCNATIVE)
    {
        #if 0
        if(!selfval.isNull())
        {
            state->vmStackPop();
        }
        #endif
        stackpos = state->execstate.valuestack->data() + state->execstate.vsposition - nargs;
        res = state->vmCallNativeFunction(callee, state->execstate.currframe->getPosition(), selfval, nargs, stackpos);
        if(state->hasErrors())
        {
            return false;
        }
        state->setStackPos(state->execstate.vsposition - nargs - 1);
        state->vmStackPush(res);
    }
    else
    {
        calleetypename = Value::getTypename(calleetype);
        state->m_errorlist.pushFormat(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "%s object is not callable", calleetypename);
        return false;
    }
    return true;
}

MC_FORCEINLINE bool mc_vmdo_tryoverloadoperator(State* state, Value left, Value right, mcinternopcode_t op, bool* outoverloadfound)
{
    int numoper;
    Value key;
    Value callee;
    Value::Type lefttype;
    Value::Type righttype;
    *outoverloadfound = false;
    lefttype = left.getType();
    righttype = right.getType();
    if(lefttype != Value::VALTYP_MAP && righttype != Value::VALTYP_MAP)
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
    callee = Value::makeNull();
    if(lefttype == Value::VALTYP_MAP)
    {
        callee = mc_value_mapgetvalue(left, key);
    }
    if(!callee.isCallable())
    {
        if(righttype == Value::VALTYP_MAP)
        {
            callee = mc_value_mapgetvalue(right, key);
        }

        if(!callee.isCallable())
        {
            *outoverloadfound = false;
            return true;
        }
    }
    *outoverloadfound = true;
    state->vmStackPush(callee);
    state->vmStackPush(left);
    if(numoper == 2)
    {
        state->vmStackPush(right);
    }
    return mc_vmdo_callobject(state, callee, numoper);
}


MC_FORCEINLINE bool mc_vm_checkassign(State* state, Value oldvalue, Value nvalue)
{
    return true;
    Value::Type nvaluetype;
    Value::Type oldvaluetype;
    (void)state;
    oldvaluetype = oldvalue.getType();
    nvaluetype = nvalue.getType();
    if(oldvaluetype == Value::VALTYP_NULL || nvaluetype == Value::VALTYP_NULL)
    {
        return true;
    }
    #if 0
    if(oldvaluetype != nvaluetype)
    {
        state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "trying to assign variable of type %s to %s",
                          Value::getTypename(nvaluetype), Value::getTypename(oldvaluetype));
        return false;
    }
    #endif
    return true;
}


MC_FORCEINLINE bool mc_vmdo_opaddstring(State* state, Value valleft, Value valright, Value::Type righttype, mcopcode_t opcode)
{
    Value nstring;
    (void)opcode;
    (void)righttype;
    nstring = mc_value_makestrcapacity(state, 0);
    mc_value_stringappendvalue(nstring, valleft);
    mc_value_stringappendvalue(nstring, valright);
    state->vmStackPush(nstring);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_math(State* state, mcopcode_t opcode)
{
    bool ok;
    bool overloadfound;
    mcfloat_t res;
    mcfloat_t dnright;
    mcfloat_t dnleft;
    const char* opcodename;
    const char* lefttypename;
    const char* righttypename;
    Value valright;
    Value valleft;
    Value::Type lefttype;
    Value::Type righttype;
    (void)ok;
    valright = state->vmStackPop();
    valleft = state->vmStackPop();
    lefttype = valleft.getType();
    righttype = valright.getType();
    if(lefttype == Value::VALTYP_STRING && opcode == MC_OPCODE_ADD)
    {
        if(mc_vmdo_opaddstring(state, valleft, valright, righttype, opcode))
        {
            return true;
        }
    }
    else if((valleft.isNumeric() || valleft.isNull()) && (valright.isNumeric() || valright.isNull()))
    {
        dnright = Value::asNumber(valright);
        dnleft = Value::asNumber(valleft);
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
        state->vmStackPush(Value::makeNumber(res));
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
        opcodename = AstCompiler::opdefGetName(opcode);
        lefttypename = Value::getTypename(lefttype);
        righttypename = Value::getTypename(righttype);
        state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "invalid operand types for %s: got %s and %s",
                          opcodename, lefttypename, righttypename);
        return false;
    }
    return true;
}

MC_FORCEINLINE ObjClass* mc_vmdo_findclassforintern(State* state, Value::Type typ)
{
    (void)state;
    (void)typ;
    switch(typ)
    {
        case Value::VALTYP_NUMBER:
            {
                return state->stdobjnumber;
            }
            break;
        case Value::VALTYP_STRING:
            {
                return state->stdobjstring;
            }
            break;
        case Value::VALTYP_ARRAY:
            {
                return state->stdobjarray;
            }
            break;
        case Value::VALTYP_MAP:
            {
                return state->stdobjmap;
            }
            break;
        case Value::VALTYP_FUNCNATIVE:
        case Value::VALTYP_FUNCSCRIPT:
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

MC_FORCEINLINE ObjClass* mc_vmdo_findclassfor(State* state, Value::Type typ)
{
    ObjClass* cl;
    cl = mc_vmdo_findclassforintern(state, typ);
    if(cl != nullptr)
    {
        
    }
    return cl;
}

MC_INLINE ObjClass::Field* mc_vmdo_getclassmember(State* state, ObjClass* cl, const char* name)
{
    size_t i;
    ObjClass::Field* memb;
    (void)state;
    for(i=0; i<cl->members->count(); i++)
    {
        memb = (ObjClass::Field*)cl->members->get(i);
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

MC_FORCEINLINE bool mc_vmdo_findclassmembervalue(State* state, Value left, Value index, Value setval)
{
    Value fnval;
    Value retv;
    const char* idxname;
    ObjClass::Field* vdest;
    (void)state;
    (void)left;
    (void)index;
    (void)setval;
    ObjClass* cl;
    cl = mc_vmdo_findclassfor(state, left.getType());
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
                retv = state->vmCallNativeFunction(fnval, state->execstate.currframe->getPosition(), left, 0, nullptr);
                state->vmStackPush(retv);
                return true;
            }
            else
            {
                retv = fnval;
                state->vmStackPush(retv);
                return true;
            }
        }
    }
    return false;
}

MC_FORCEINLINE bool mc_vmdo_getindexpartial(State* state, Value left, Value::Type lefttype, Value index, Value::Type indextype, bool fromdot)
{
    int leftlen;
    int ix;
    char resstr[2];
    const char* str;
    const char* indextypename;
    const char* lefttypename;
    Value res;
    (void)fromdot;
    lefttypename = "unknown";
    if(lefttype == Value::VALTYP_MAP)
    {
        if(mc_value_mapgetvaluechecked(left, index, &res))
        {
            goto finished;
        }
    }
    if(index.isString())
    {
        if(mc_vmdo_findclassmembervalue(state, left, index, Value::makeNull()))
        {
            #if 0
            if(callee.isFuncNative())
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
            if(!left.isMap())
            {
                res = Value::makeNull();
                lefttypename = Value::getTypename(lefttype);
                state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "object type '%s' has no field '%s'", lefttypename, mc_value_stringgetdata(index));
                state->vmStackPush(res);
                return false;
            }
        }
    }
    if(lefttype != Value::VALTYP_ARRAY && lefttype != Value::VALTYP_MAP && lefttype != Value::VALTYP_STRING)
    {
        state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "getindexpartial: type %s is not indexable", lefttypename);

        return false;
    }
    res = Value::makeNull();
    if(lefttype == Value::VALTYP_ARRAY)
    {
        if(indextype != Value::VALTYP_NUMBER)
        {
            lefttypename = Value::getTypename(lefttype);
            indextypename = Value::getTypename(indextype);
            state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "cannot get partial index of %s with %s", lefttypename, indextypename);
            return false;
        }
        ix = (int)Value::asNumber(index);
        if(ix < 0)
        {
            ix = mc_value_arraygetlength(left) + ix;
        }
        if(ix >= 0 && ix < mc_value_arraygetlength(left))
        {
            res = mc_value_arraygetvalue(left, ix);
        }
    }
    else if(lefttype == Value::VALTYP_STRING)
    {
        if(indextype != Value::VALTYP_NUMBER)
        {
            lefttypename = Value::getTypename(lefttype);
            indextypename = Value::getTypename(indextype);
            state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "cannot index %s with %s", lefttypename, indextypename);
            return false;
        }
        str = mc_value_stringgetdata(left);
        leftlen = mc_value_stringgetlength(left);
        ix = (int)Value::asNumber(index);
        if(ix >= 0 && ix < leftlen)
        {
            resstr[0] = str[ix];
            res = mc_value_makestringlen(state, resstr, 1);
        }
    }
    finished:
    state->vmStackPush(res);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_getindexfull(State* state)
{
    Value::Type lefttype;
    Value::Type indextype;
    Value left;
    Value index;
    index = state->vmStackPop();
    left = state->vmStackPop();
    lefttype = left.getType();
    indextype = index.getType();
    return mc_vmdo_getindexpartial(state, left, lefttype, index, indextype, false);
}

MC_FORCEINLINE bool mc_vmdo_getdotindex(State* state)
{
    Value::Type lefttype;
    Value::Type indextype;
    Value left;
    Value index;
    index = state->vmStackPop();
    left = state->vmStackPop();
    lefttype = left.getType();
    indextype = index.getType();
    return mc_vmdo_getindexpartial(state, left, lefttype, index, indextype, true);
}

MC_FORCEINLINE bool mc_vmdo_setindexpartial(State* state, Value left, Value::Type lefttype, Value index, Value::Type indextype, Value nvalue)
{
    bool ok;
    int alen;
    int ix;
    const char* indextypename;
    const char* lefttypename;
    Value oldvalue;
    (void)ok;
    if(lefttype != Value::VALTYP_ARRAY && lefttype != Value::VALTYP_MAP)
    {
        lefttypename = Value::getTypename(lefttype);
        #if 0
        {
            int* p = nullptr;
            p[5] += 5;
        }
        #endif
        lefttypename = Value::getTypename(lefttype);
        state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "setindexpartial: type %s is not indexable", lefttypename);
        return false;
    }
    if(lefttype == Value::VALTYP_ARRAY)
    {
        if(indextype != Value::VALTYP_NUMBER)
        {
            lefttypename = Value::getTypename(lefttype);
            indextypename = Value::getTypename(indextype);
            state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "cannot set index of %s with %s", lefttypename, indextypename);
            return false;
        }
        ix = (int)Value::asNumber(index);                        
        ok = mc_value_arraysetvalue(left, ix, nvalue);
        alen = mc_value_arraygetlength(left);
        if(!ok)
        {
            state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "failed to set array index %d (of %d)", ix, alen);
            return false;
        }
    }
    else if(lefttype == Value::VALTYP_MAP)
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

MC_FORCEINLINE bool mc_vmdo_setindexfull(State* state)
{
    Value index;
    Value left;
    Value nvalue;
    Value::Type lefttype;
    Value::Type indextype;
    index = state->vmStackPop();
    left = state->vmStackPop();
    nvalue = state->vmStackPop();
    lefttype = left.getType();
    indextype = index.getType();
    return mc_vmdo_setindexpartial(state, left, lefttype, index, indextype, nvalue);
}

MC_FORCEINLINE bool mc_vmdo_getvalueatfull(State* state)
{
    int ix;
    int leftlen;
    char resstr[2];
    const char* lefttypename;
    const char* indextypename;
    const char* str;
    Value::Type lefttype;
    Value::Type indextype;
    Value index;
    Value left;
    Value res;
    index = state->vmStackPop();
    left = state->vmStackPop();
    lefttype = left.getType();
    indextype= index.getType();
    if(lefttype != Value::VALTYP_ARRAY && lefttype != Value::VALTYP_MAP && lefttype != Value::VALTYP_STRING)
    {
        lefttypename = Value::getTypename(lefttype);
        state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "getvalueatfull: type %s is not indexable", lefttypename);
        return false;
    }
    res = Value::makeNull();
    if(indextype != Value::VALTYP_NUMBER)
    {
        lefttypename = Value::getTypename(lefttype);
        indextypename = Value::getTypename(indextype);
        state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "cannot get full index %s with %s", lefttypename, indextypename);
        return false;
    }
    ix = (int)Value::asNumber(index);
    if(lefttype == Value::VALTYP_ARRAY)
    {
        res = mc_value_arraygetvalue(left, ix);
    }
    else if(lefttype == Value::VALTYP_MAP)
    {
        res = mc_value_mapgetkvpairat(state, left, ix);
    }
    else if(lefttype == Value::VALTYP_STRING)
    {
        str = mc_value_stringgetdata(left);
        leftlen = mc_value_stringgetlength(left);
        ix = (int)Value::asNumber(index);
        if(ix >= 0 && ix < leftlen)
        {
            resstr[0] = str[ix];
            res = mc_value_makestringlen(state, resstr, 1);
        }
    }
    state->vmStackPush(res);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_makefunction(State* state, GenericList<Value>* constants)
{
    int i;
    uint16_t numfree;
    uint16_t constantix;
    const char* fname;
    const char* tname;
    Value::Type constanttype;
    Value freeval;
    Value functionobj;
    Value* constant;
    mcobjfunction_t* constfun;
    constantix = state->execstate.currframe->readUint16();
    numfree = state->execstate.currframe->readUint8();
    constant = constants->getp(constantix);
    if(!constant)
    {
        state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "constant %d not found", constantix);
        return false;
    }
    constanttype = (*constant).getType();
    if(constanttype != Value::VALTYP_FUNCSCRIPT)
    {
        tname = Value::getTypename(constanttype);
        state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "%s is not a function", tname);
        return false;
    }
    constfun = Value::asFunction(*constant);
    fname = mc_value_functiongetname(*constant);
    functionobj = mc_value_makefuncscript(state, fname, constfun->funcdata.valscriptfunc.compiledprogcode, false, constfun->funcdata.valscriptfunc.numlocals, constfun->funcdata.valscriptfunc.numargs, numfree);
    if(functionobj.isNull())
    {
        return false;
    }
    for(i = 0; i < numfree; i++)
    {
        freeval = state->execstate.valuestack->get(state->execstate.vsposition - numfree + i);
        mc_value_functionsetfreevalat(functionobj, i, freeval);
    }
    state->setStackPos(state->execstate.vsposition - numfree);
    state->vmStackPush(functionobj);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_docmpvalue(State* state, mcopcode_t opcode)
{
    bool ok;
    bool isoverloaded;
    const char* lefttname;
    const char* righttname;
    Value::CompareResult cres;
    Value res;
    Value left;
    Value right;
    (void)ok;
    right = state->vmStackPop();
    left = state->vmStackPop();
    isoverloaded = false;
    ok = mc_vmdo_tryoverloadoperator(state, left, right, MC_OPCODE_COMPARE, &isoverloaded);
    if(!ok)
    {
        return false;
    }
    if(!isoverloaded)
    {
        ok = Value::compare(left, right, &cres);
        #if 0
        fprintf(stderr, "compare: ok=%d cres.result=%g\n", ok, cres.result);
        #endif
        if((ok == true) || (opcode == MC_OPCODE_COMPAREEQ))
        {
            res = Value::makeNumber(cres.result);
            state->vmStackPush(res);
        }
        else
        {
            righttname = Value::getTypename(right.getType());
            lefttname = Value::getTypename(left.getType());
            state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "cannot compare %s and %s", lefttname, righttname);
            return false;
        }
    }
    return true;
}

MC_FORCEINLINE bool mc_vmdo_docmpvalgreater(State* state, mcopcode_t opcode)
{
    bool resval;
    mcfloat_t comparisonres;
    Value res;
    Value value;
    value = state->vmStackPop();
    comparisonres = Value::asNumber(value);
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
    res = Value::makeBool(resval);
    state->vmStackPush(res);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_makearray(State* state)
{
    bool ok;
    int i;
    uint16_t count;
    Value item;
    Value arrayobj;
    Value* items;
    (void)ok;
    count = state->execstate.currframe->readUint16();
    arrayobj = mc_value_makearraycapacity(state, count);
    if(arrayobj.isNull())
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
    state->setStackPos(state->execstate.vsposition - count);
    state->vmStackPush(arrayobj);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_makemapstart(State* state)
{
    uint16_t count;
    Value mapobj;
    count = state->execstate.currframe->readUint16();
    mapobj = mc_value_makemapcapacity(state, count);
    if(mapobj.isNull())
    {
        return false;
    }
    state->vmStackThisPush(mapobj);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_makemapend(State* state)
{
    bool ok;
    int i;
    uint16_t kvpcount;
    uint16_t itemscount;
    const char* keytypename;
    Value::Type keytype;
    Value key;
    Value val;
    Value mapobj;
    Value* kvpairs;
    (void)ok;
    kvpcount = state->execstate.currframe->readUint16();
    itemscount = kvpcount * 2;
    mapobj = state->vmStackThisPop();
    kvpairs = state->execstate.valuestack->data() + state->execstate.vsposition - itemscount;
    for(i = 0; i < itemscount; i += 2)
    {
        key = kvpairs[i];
        if(!Value::isHashable(key))
        {
            keytype = key.getType();
            keytypename = Value::getTypename(keytype);
            state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "key of type %s is not hashable", keytypename);
            return false;
        }
        val = kvpairs[i + 1];
        ok = mc_value_mapsetvalue(mapobj, key, val);
    }
    state->setStackPos(state->execstate.vsposition - itemscount);
    state->vmStackPush(mapobj);
    return true;
}

#if defined(__GNUC__) || defined(__CLANG__) || defined(__TINYC__)
    #define MC_CONF_USECOMPUTEDGOTOS 1
#else
    #define MC_CONF_USECOMPUTEDGOTOS 0
#endif

#define MC_CONF_USECOMPUTEDGOTOS 0

#if defined(MC_CONF_USECOMPUTEDGOTOS) && (MC_CONF_USECOMPUTEDGOTOS == 1)
    #define MAKELABEL(opname) LABEL_##opname
    #define mcvm_case(opn) LABEL_##opn
#else
    #define mcvm_case(opn) case opn
#endif

#define mc_vmmac_break() \
    goto readnextop


void mc_vmutil_getopinfo(mcopcode_t opc, const char** oname)
{
    mcopdefinition_t vdef;
    mcopdefinition_t* def;
    *oname = "!invalid!";
    def = AstCompiler::opdefLookup(&vdef, opc);
    if(def != nullptr)
    {
        *oname = def->name;
    }
}

bool mc_vm_execvm(State* state, Value function, GenericList<Value>* constants, bool nested)
{
    bool ok;
    int fri;
    int prevcode;
    int opcode;
    size_t recoverframeix;
    VMFrame createdframe;
    Value errobj;
    VMFrame* frame;
    Error* err;
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
        state->error.pushFormat(MC_ERROR_USER, AstLocation::Invalid(), "state is already executing code");
        return false;
    }
    #endif
    /* naming is hard */
    targetfunction = Value::asFunction(function);
    ok = false;
    ok = VMFrame::init(&createdframe, function, state->execstate.vsposition - targetfunction->funcdata.valscriptfunc.numargs);
    if(!ok)
    {
        fprintf(stderr, "failed to init frames!\n");
        return false;
    }
    ok = state->vmPushFrame(createdframe);
    if(!ok)
    {
        state->m_errorlist.pushFormat(MC_ERROR_USER, AstLocation::Invalid(), "pushing frame failed");
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
    state->execstate.lastpopped = Value::makeNull();
    //while(state->execstate.currframe->bcposition < state->execstate.currframe->bcsize)
    while(true)
    {
        readnextop:
        prevcode = opcode;
        if(state->execstate.currframe == nullptr)
        {
            goto onexecfinish;
        }
        opcode = state->execstate.currframe->readOpCode();
        if(mc_util_unlikely(state->m_config.printinstructions))
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
                    state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "unknown opcode: %d (%s) (previous opcode was %d (%s))", opcode, thisname, prevcode, prevname);
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
                    Value res;
                    res = state->vmStackPop();
                    ok = state->vmPopFrame();
                    if(!ok)
                    {
                        goto onexecfinish;
                    }
                    state->vmStackPush(res);
                    if(nested)
                    {
                        goto onexecfinish;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_RETURN):
                {
                    ok = state->vmPopFrame();
                    state->vmStackPush(Value::makeNull());
                    if(!ok)
                    {
                        state->vmStackPop();
                        goto onexecfinish;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_CONSTANT):
                {
                    uint16_t constantix;
                    Value* constant;
                    constantix = state->execstate.currframe->readUint16();
                    constant = (Value*)constants->getp(constantix);
                    if(!constant)
                    {
                        state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "constant at %d not found", constantix);
                        goto onexecerror;
                    }
                    state->vmStackPush(*constant);
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
                    state->vmStackPop();
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_TRUE):
                {
                    state->vmStackPush(Value::makeBool(true));
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_FALSE):
                {
                    state->vmStackPush(Value::makeBool(false));
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
                    Value res;
                    Value::Type opertype;
                    Value operand;
                    operand = state->vmStackPop();
                    opertype = operand.getType();
                    if(mc_util_likely(opertype == Value::VALTYP_NUMBER))
                    {
                        val = Value::asNumber(operand);
                        res = Value::makeNumber(-val);
                        state->vmStackPush(res);
                    }
                    else
                    {
                        overloadfound = false;
                        ok = mc_vmdo_tryoverloadoperator(state, operand, Value::makeNull(), MC_OPCODE_MINUS, &overloadfound);
                        if(!ok)
                        {
                            goto onexecerror;
                        }
                        if(!overloadfound)
                        {
                            opertname = Value::getTypename(opertype);
                            state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "invalid operand type for MINUS, got %s", opertname);
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
                    Value res;
                    Value::Type opertype;
                    Value operand;
                    operand = state->vmStackPop();
                    opertype = operand.getType();
                    if(opertype == Value::VALTYP_NUMBER)
                    {
                        val = Value::asNumber(operand);
                        res = Value::makeNumber(~val);
                        state->vmStackPush(res);
                    }
                    else
                    {
                        overloadfound = false;
                        ok = mc_vmdo_tryoverloadoperator(state, operand, Value::makeNull(), MC_OPCODE_BINNOT, &overloadfound);
                        if(!ok)
                        {
                            goto onexecerror;
                        }
                        if(!overloadfound)
                        {
                            opertname = Value::getTypename(opertype);
                            state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "invalid operand type for BINNOT, got %s", opertname);
                            goto onexecerror;
                        }
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_BANG):
                {
                    bool overloadfound;
                    Value res;
                    Value operand;
                    Value::Type type;
                    operand = state->vmStackPop();
                    type = operand.getType();
                    if(type == Value::VALTYP_BOOL)
                    {
                        res = Value::makeBool(!Value::asBool(operand));
                        state->vmStackPush(res);
                    }
                    else if(type == Value::VALTYP_NULL)
                    {
                        res = Value::makeBool(true);
                        state->vmStackPush(res);
                    }
                    else
                    {
                        overloadfound = false;
                        ok = mc_vmdo_tryoverloadoperator(state, operand, Value::makeNull(), MC_OPCODE_BANG, &overloadfound);
                        if(!ok)
                        {
                            goto onexecerror;
                        }
                        if(!overloadfound)
                        {
                            res = Value::makeBool(false);
                            state->vmStackPush(res);
                        }
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_JUMP):
                {
                    uint16_t pos;
                    pos = state->execstate.currframe->readUint16();
                    state->execstate.currframe->bcposition = pos;
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_JUMPIFFALSE):
                {
                    uint16_t pos;
                    Value test;
                    pos = state->execstate.currframe->readUint16();
                    test = state->vmStackPop();
                    if(!Value::asBool(test))
                    {
                        state->execstate.currframe->bcposition = pos;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_JUMPIFTRUE):
                {
                    uint16_t pos;
                    Value test;
                    pos = state->execstate.currframe->readUint16();
                    test = state->vmStackPop();
                    if(Value::asBool(test))
                    {
                        state->execstate.currframe->bcposition = pos;
                    }
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_NULL):
                {
                    state->vmStackPush(Value::makeNull());
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_DEFINEMODULEGLOBAL):
                {
                    uint16_t ix;
                    Value value;
                    ix = state->execstate.currframe->readUint16();
                    value = state->vmStackPop();
                    state->setGlobalByIndex(ix, value);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_SETMODULEGLOBAL):
                {
                    uint16_t ix;
                    Value nvalue;
                    Value oldvalue;
                    ix = state->execstate.currframe->readUint16();
                    nvalue = state->vmStackPop();
                    oldvalue= state->getGlobalByIndex(ix);
                    if(!mc_vm_checkassign(state, oldvalue, nvalue))
                    {
                        goto onexecerror;
                    }
                    state->setGlobalByIndex(ix, nvalue);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_GETMODULEGLOBAL):
                {
                    uint16_t ix;
                    Value global;
                    ix = state->execstate.currframe->readUint16();
                    global = state->globalvalstack->get(ix);
                    state->vmStackPush(global);
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
                    Value callee;
                    nargs = state->execstate.currframe->readUint8();
                    callee = state->vmStackGet(nargs);
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
                    pos = state->execstate.currframe->readUint8();
                    state->execstate.valuestack->set(state->execstate.currframe->basepointer + pos, state->vmStackPop());
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_SETLOCAL):
                {
                    uint16_t pos;
                    Value nvalue;
                    Value oldvalue;
                    pos = state->execstate.currframe->readUint8();
                    nvalue = state->vmStackPop();
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
                    Value val;
                    pos = state->execstate.currframe->readUint8();
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
                    state->vmStackPush(val);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_GETGLOBALBUILTIN):
                {
                    uint16_t ix;
                    Value val;
                    ix = state->execstate.currframe->readUint16();
                    ok = false;
                    val = state->vmglobalstore->getAtIndex(ix, &ok);
                    if(!ok)
                    {
                        state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "global value %d not found", ix);
                        goto onexecerror;
                    }
                    state->vmStackPush(val);
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
                    Value val;
                    freeix = state->execstate.currframe->readUint8();
                    val = mc_value_functiongetfreevalat(state->execstate.currframe->function, freeix);
                    state->vmStackPush(val);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_SETFREE):
                {
                    uint16_t freeix;
                    Value val;
                    freeix = state->execstate.currframe->readUint8();
                    val = state->vmStackPop();
                    mc_value_functionsetfreevalat(state->execstate.currframe->function, freeix, val);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_CURRENTFUNCTION):
                {
                    Value currentfunction;
                    currentfunction = state->execstate.currframe->function;
                    state->vmStackPush(currentfunction);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_GETTHIS):
                {
                    Value obj;
                    obj = state->vmStackThisGet(0);
                    state->vmStackPush(obj);
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
                    Value val;
                    val = state->vmStackGet(0);
                    state->vmStackPush(val);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_FOREACHLEN):
                {
                    int len;
                    const char* tname;
                    Value val;
                    Value::Type type;
                    val = state->vmStackPop();
                    len = 0;
                    type = val.getType();
                    if(type == Value::VALTYP_ARRAY)
                    {
                        len = mc_value_arraygetlength(val);
                    }
                    else if(type == Value::VALTYP_MAP)
                    {
                        len = mc_value_mapgetlength(val);
                    }
                    else if(type == Value::VALTYP_STRING)
                    {
                        len = mc_value_stringgetlength(val);
                    }
                    else
                    {
                        tname = Value::getTypename(type);
                        state->pushError(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "cannot get length of %s", tname);
                        goto onexecerror;
                    }
                    state->vmStackPush(Value::makeNumber(len));
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_NUMBER):
                {
                    uint64_t val;
                    mcfloat_t dval;
                    Value obj;
                    val = state->execstate.currframe->readUint64();
                    dval = mc_util_uint64todouble(val);
                    obj = Value::makeNumber(dval);
                    state->vmStackPush(obj);
                }
                mc_vmmac_break();
            mcvm_case(MC_OPCODE_SETRECOVER):
                {
                    uint16_t recip;
                    recip = state->execstate.currframe->readUint16();
                    state->execstate.currframe->recoverip = recip;
                }
                mc_vmmac_break();
        }
    onexecerror:
        state->hadrecovered = false;
        if(state->m_errorlist.count() > 0)
        {
            err = state->m_errorlist.getLast();
            if(err->m_errtype == MC_ERROR_RUNTIME && state->m_errorlist.count() >= 1)
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
                    err->m_traceback = Memory::make<Traceback>();
                }
                if(err->m_traceback)
                {
                    mc_traceback_vmpush(err->m_traceback, state);
                }
                while(state->execstate.framestack->count() > (recoverframeix + 1))
                {
                    state->vmPopFrame();
                }
                errobj = mc_value_makeerror(state, err->m_message);
                if(!errobj.isNull())
                {
                    mc_value_errorsettraceback(errobj, err->m_traceback);
                    err->m_traceback = nullptr;
                }
                state->vmStackPush(errobj);
                state->execstate.currframe->bcposition = state->execstate.currframe->recoverip;
                state->execstate.currframe->isrecovering = true;
                state->m_errorlist.clear();
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
    if(state->m_errorlist.count() > 0)
    {
        err = state->m_errorlist.getLast();
        if(!err->m_traceback)
        {
            err->m_traceback = Memory::make<Traceback>();
        }
        if(err->m_traceback)
        {
            mc_traceback_vmpush(err->m_traceback, state);
        }
    }
    mc_vm_rungc(state, constants);
    state->running = false;
    return state->m_errorlist.count() == 0;
}





Value mc_scriptfn_typeof(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    Value arg;
    Value::Type type;
    const char* ts;
    (void)data;
    (void)state;
    (void)thisval;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_ANY))
    {
        return Value::makeNull();
    }
    arg = args[0];
    type = arg.getType();
    ts = Value::getTypename(type);
    return mc_value_makestring(state, ts);
}

Value mc_scriptfn_arrayfirst(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    Value arg;
    (void)state;
    (void)data;
    (void)thisval;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_ARRAY))
    {
        return Value::makeNull();
    }
    arg = args[0];
    return mc_value_arraygetvalue(arg, 0);
}

Value mc_scriptfn_arraylast(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int len;
    Value arg;
    (void)state;
    (void)argc;
    (void)thisval;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_ARRAY))
    {
        return Value::makeNull();
    }
    arg = args[0];
    len = mc_value_arraygetlength(arg);
    return mc_value_arraygetvalue(arg, len - 1);
}

Value mc_scriptfn_arrayrest(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    bool ok;
    int i;
    int len;
    Value arg;
    Value res;
    Value item;
    (void)state;
    (void)argc;
    (void)thisval;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_ARRAY))
    {
        return Value::makeNull();
    }
    arg = args[0];
    len = mc_value_arraygetlength(arg);
    if(len == 0)
    {
        return Value::makeNull();
    }
    res = mc_value_makearray(state);
    if(res.isNull())
    {
        return Value::makeNull();
    }
    for(i = 1; i < len; i++)
    {
        item = mc_value_arraygetvalue(arg, i);
        ok = mc_value_arraypush(res, item);
        if(!ok)
        {
            return Value::makeNull();
        }
    }
    return res;
}

Value mc_scriptfn_reverse(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    bool ok;
    int i;
    int inplen;
    char* resbuf;
    const char* inpstr;
    Value::Type type;
    Value arg;
    Value obj;
    Value res;
    (void)state;
    (void)argc;
    (void)thisval;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_ARRAY | Value::VALTYP_STRING))
    {
        return Value::makeNull();
    }
    arg = args[0];
    type = arg.getType();
    if(type == Value::VALTYP_ARRAY)
    {
        inplen = mc_value_arraygetlength(arg);
        res = mc_value_makearraycapacity(state, inplen);
        if(res.isNull())
        {
            return Value::makeNull();
        }
        for(i = 0; i < inplen; i++)
        {
            obj = mc_value_arraygetvalue(arg, i);
            ok = mc_value_arraysetvalue(res, inplen - i - 1, obj);
            if(!ok)
            {
                return Value::makeNull();
            }
        }
        return res;
    }
    if(type == Value::VALTYP_STRING)
    {
        inpstr = mc_value_stringgetdata(arg);
        inplen = mc_value_stringgetlength(arg);
        res = mc_value_makestrcapacity(state, inplen);
        if(res.isNull())
        {
            return Value::makeNull();
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
    return Value::makeNull();
}

Value mc_scriptfn_makearray(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    bool ok;
    int i;
    int capacity;
    Value res;
    Value objnull;
    (void)state;
    (void)argc;
    (void)thisval;
    (void)data;
    if(argc == 1)
    {
        if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER))
        {
            return Value::makeNull();
        }
        capacity = (int)Value::asNumber(args[0]);
        res = mc_value_makearraycapacity(state, capacity);
        if(res.isNull())
        {
            return Value::makeNull();
        }
        objnull = Value::makeNull();
        for(i = 0; i < capacity; i++)
        {
            ok = mc_value_arraypush(res, objnull);
            if(!ok)
            {
                return Value::makeNull();
            }
        }
        return res;
    }
    if(argc == 2)
    {
        if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER, Value::VALTYP_ANY))
        {
            return Value::makeNull();
        }
        capacity = (int)Value::asNumber(args[0]);
        res = mc_value_makearraycapacity(state, capacity);
        if(res.isNull())
        {
            return Value::makeNull();
        }
        for(i = 0; i < capacity; i++)
        {
            ok = mc_value_arraypush(res, args[1]);
            if(!ok)
            {
                return Value::makeNull();
            }
        }
        return res;
    }
    mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER);
    return Value::makeNull();
}

Value mc_scriptfn_externalfn(State* state, void *data, Value thisval, size_t argc, Value *args)
{
    int *test;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)args;
    test = (int*)data;
    *test = 42;
    return Value::makeNull();
}

Value mc_scriptfn_vec2add(State *state, void *data, Value thisval, size_t argc, Value *args)
{
    mcfloat_t a_x;
    mcfloat_t a_y;
    mcfloat_t b_x;
    mcfloat_t b_y;
    Value res;
    Value keyx;
    Value keyy;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    if (!mc_argcheck_check(state, true, argc, args, Value::VALTYP_MAP, Value::VALTYP_MAP))
    {
        return Value::makeNull();
    }
    keyx = mc_value_makestring(state, "x");
    keyy = mc_value_makestring(state, "y");
    a_x = Value::asNumber(mc_value_mapgetvalue(args[0], keyx));
    a_y = Value::asNumber(mc_value_mapgetvalue(args[0], keyy));
    b_x = Value::asNumber(mc_value_mapgetvalue(args[1], keyx));
    b_y = Value::asNumber(mc_value_mapgetvalue(args[1], keyy));
    res = mc_value_makemap(state);
    if (res.getType() == Value::VALTYP_NULL)
    {
        return res;
    }
    mc_value_mapsetvalue(res, keyx, Value::makeNumber(a_x + b_x));
    mc_value_mapsetvalue(res, keyy, Value::makeNumber(a_y + b_y));
    return res;
}

Value mc_scriptfn_vec2sub(State *state, void *data, Value thisval, size_t argc, Value *args)
{
    mcfloat_t a_x;
    mcfloat_t a_y;
    mcfloat_t b_y;
    mcfloat_t b_x;
    Value res;
    Value keyx;
    Value keyy;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    if (!mc_argcheck_check(state, true, argc, args, Value::VALTYP_MAP, Value::VALTYP_MAP))
    {
        return Value::makeNull();
    }
    keyx = mc_value_makestring(state, "x");
    keyy = mc_value_makestring(state, "y");
    a_x = Value::asNumber(mc_value_mapgetvalue(args[0], keyx));
    a_y = Value::asNumber(mc_value_mapgetvalue(args[0], keyy));
    b_x = Value::asNumber(mc_value_mapgetvalue(args[1], keyx));
    b_y = Value::asNumber(mc_value_mapgetvalue(args[1], keyy));
    res = mc_value_makemap(state);
    mc_value_mapsetvalue(res, keyx, Value::makeNumber(a_x - b_x));
    mc_value_mapsetvalue(res, keyy, Value::makeNumber(a_y - b_y));
    return res;
}

Value mc_scriptfn_testcheckargs(State* state, void *data, Value thisval, size_t argc, Value *args)
{
    (void)state;
    (void)args;
    (void)thisval;
    (void)argc;
    (void)data;
    if (!mc_argcheck_check(state, true, argc, args,
                  Value::VALTYP_NUMBER,
                  Value::VALTYP_ARRAY | Value::VALTYP_MAP,
                  Value::VALTYP_MAP,
                  Value::VALTYP_STRING,
                  Value::VALTYP_NUMBER | Value::VALTYP_BOOL,
                  Value::VALTYP_FUNCSCRIPT | Value::VALTYP_FUNCNATIVE,
                  Value::VALTYP_ANY))
    {
        return Value::makeNull();
    }
    return Value::makeNumber(42);
}


Value mc_scriptfn_maketestdict(State *state, void *data, Value thisval, size_t argc, Value *args)
{
    int i;
    int blen;
    int numitems;
    Value res;
    Value key;
    Value val;
    const char *tname;
    char keybuf[64];
    (void)data;
    (void)thisval;
    if (argc != 1)
    {
        state->pushRuntimeError("invalid type passed to maketestdict, got %d, expected 1", argc);
        return Value::makeNull();
    }    
    if (args[0].getType() != Value::VALTYP_NUMBER)
    {
        tname = Value::getTypename(args[0].getType());
        state->pushRuntimeError("invalid type passed to maketestdict, got %s", tname);
        return Value::makeNull();
    }
    numitems = Value::asNumber(args[0]);
    res = mc_value_makemap(state);
    if (res.getType() == Value::VALTYP_NULL)
    {
        return Value::makeNull();
    }
    for (i = 0; i < numitems; i++)
    {
        blen = sprintf(keybuf, "%d", i);
        key = mc_value_makestringlen(state, keybuf, blen);
        val = Value::makeNumber(i);
        mc_value_mapsetvalue(res, key, val);
    }
    return res;
}

Value mc_scriptfn_squarearray(State *state, void *data, Value thisval, size_t argc, Value *args)
{
    size_t i;
    mcfloat_t num;
    Value res;
    Value resitem;    
    (void)data;
    (void)thisval;
    res = mc_value_makearraycapacity(state, argc);
    for(i = 0; i < argc; i++)
    {
        if(args[i].getType() != Value::VALTYP_NUMBER)
        {
            state->pushRuntimeError("invalid type passed to squarearray");
            return Value::makeNull();
        }
        num = Value::asNumber(args[i]);
        resitem = Value::makeNumber(num * num);
        mc_value_arraypush(res, resitem);
    }
    return res;
}

Value mc_scriptfn_print(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t i;
    Value arg;
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
    return Value::makeNull();
}

Value mc_scriptfn_println(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    Value o;
    (void)thisval;
    o = mc_scriptfn_print(state, data, thisval, argc, args);
    state->stdoutprinter->putChar('\n');
    return o;
}

Value mc_scriptfn_tostring(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int reslen;
    const char* resstr;
    Value arg;
    Value res;
    Printer pr;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    arg = args[0];
    Printer::initFromStack(&pr, nullptr, true);
    mc_printer_printvalue(&pr, arg, false);
    if(pr.m_prfailed)
    {
        Printer::releaseFromPtr(&pr, true);
        return Value::makeNull();
    }
    resstr = pr.getString();
    reslen = pr.getLength();
    res = mc_value_makestringlen(state, resstr, reslen);
    Printer::releaseFromPtr(&pr, false);
    return res;
}

Value mc_nsfnjson_stringify(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int reslen;
    const char* resstr;
    Value arg;
    Value res;
    Printer pr;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    arg = args[0];
    Printer::initFromStack(&pr, nullptr, true);
    pr.m_prconfig.verbosefunc = false;
    pr.m_prconfig.quotstring = true;
    mc_printer_printvalue(&pr, arg, false);
    if(pr.m_prfailed)
    {
        Printer::releaseFromPtr(&pr, true);
        return Value::makeNull();
    }
    resstr = pr.getString();
    reslen = pr.getLength();
    res = mc_value_makestringlen(state, resstr, reslen);
    Printer::releaseFromPtr(&pr, true);
    return res;
}


Value mc_objfnnumber_chr(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    mcfloat_t val;
    char c;
    (void)state;
    (void)argc;
    (void)data;
    (void)args;
    val = Value::asNumber(thisval);
    c = (char)val;
    return mc_value_makestringlen(state, &c, 1);
}
 
Value mc_objfnstring_length(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t len;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    len = mc_value_stringgetlength(thisval);
    return Value::makeNumber(len);
}

/**
 * \brief Searches a string an instance of another string in it and returns the index of the first occurance.  If no occurance is found a -1 is returned.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return The index of the found string or -1 if it's not found.
 */
Value mc_objfnstring_indexof(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int inplen;
    int searchlen;
    int startindex;
    char tmpch;
    char* result;
    const char* inpstr;
    const char* searchstr;
    Value searchval;
    Value::Type searchtype;
    (void)state;
    (void)data;
    (void)inplen;
    (void)searchlen;
    (void)argc;
    startindex = 0;
    searchval = args[0];
    searchtype = searchval.getType();
    if(searchtype == Value::VALTYP_NULL)
    {
        return Value::makeNumber(-1);
    }
    searchstr = nullptr;
    searchlen = 0;
    inpstr = mc_value_stringgetdata(thisval);
    inplen = mc_value_stringgetlength(thisval);
    MC_ASSERT((searchtype == Value::VALTYP_NUMBER) || (searchtype == Value::VALTYP_STRING));
    if(searchtype == Value::VALTYP_NUMBER)
    {
        tmpch = Value::asNumber(searchval);
        inpstr = &tmpch;
        inplen = 1;
    }
    else if(searchtype == Value::VALTYP_STRING)
    {
        searchstr = mc_value_stringgetdata(searchval);
        searchlen = mc_value_stringgetlength(searchval);
    }

    result = (char*)strstr(inpstr + startindex, searchstr);
    if(result == nullptr)
    {
        return Value::makeNumber(-1);
    }
    return Value::makeNumber(result - inpstr);
}

Value mc_objfnstring_charcodefirst(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    char ch;
    size_t len;
    const char* str;
    Value sval;
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
    return Value::makeNumber(ch);
}


Value mc_objfnstring_charcodeat(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    char ch;
    long idx;
    size_t len;
    const char* str;
    Value sval;
    (void)state;
    (void)data;
    (void)thisval;
    (void)argc;
    (void)args;
    sval = thisval;
    str = mc_value_stringgetdata(sval);
    len = mc_value_stringgetlength(sval);
    idx = Value::asNumber(args[0]);
    if(idx >= (long)len)
    {
        return Value::makeNull();
    }
    ch = str[idx];
    return Value::makeNumber(ch);
}


Value mc_objfnstring_charat(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    char ch;
    long idx;
    size_t len;
    const char* str;
    Value sval;
    (void)state;
    (void)data;
    (void)thisval;
    (void)argc;
    (void)args;
    sval = thisval;
    str = mc_value_stringgetdata(sval);
    len = mc_value_stringgetlength(sval);
    idx = Value::asNumber(args[0]);
    if(idx >= (long)len)
    {
        return Value::makeNull();
    }
    ch = str[idx];
    return mc_value_makestringlen(state, &ch, 1);
}

Value mc_objfnstring_getself(State* state, void* data, Value thisval, size_t argc, Value* args)
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

Value mc_objfnstring_tonumber(State* state, void* data, Value thisval, size_t argc, Value* args)
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
    return Value::makeNumber(result);
err:
    state->pushError(MC_ERROR_RUNTIME, AstLocation::Invalid(), "cannot convert \"%s\" to number", string);
    return Value::makeNull();
}

/**
 * \brief Returns the specified number of characters from the left hand side of the string.  If more characters exist than the length of the string the entire string is returned.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return The section of the string from the left-hand side.
 */
Value mc_objfnstring_left(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int inplen;
    int startpos;
    char* result;
    const char* inpstr;
    Value obj;
    Value inpval;
    Value posval;
    (void)data;
    if(argc > 0 && args[0].getType() == Value::VALTYP_NUMBER)
    {
        inpval = thisval;
        posval = args[0];
        inpstr = mc_value_stringgetdata(inpval);
        inplen = mc_value_stringgetlength(inpval);
        startpos = Value::asNumber(posval);
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
            return Value::makeNull();
        }
        strncpy(result, inpstr, startpos);
        result[startpos] = '\0';
        obj = mc_value_makestringlen(state, result, startpos);
        mc_memory_free(result);
        return obj;
    }
    return Value::makeNull();
}

/**
 * \brief Returns the specified number of characters from the right hand side of the string.  If more characters exist than the length of the string the entire string is returned.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return The section of the string from the right-hand side.
 */
Value mc_objfnstring_right(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int inplen;
    int startpos;
    int strlength;
    char* result;
    const char* inpstr;
    Value obj;
    Value inpval;
    Value idxval;
    (void)data;
    (void)thisval;
    if(argc > 0 && args[1].getType() == Value::VALTYP_NUMBER)
    {
        inpval = thisval;
        idxval = args[0];
        inpstr = mc_value_stringgetdata(inpval);
        inplen = mc_value_stringgetlength(inpval);
        startpos = Value::asNumber(idxval);
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
            return Value::makeNull();
        }
        strlength = inplen;
        strncpy(result, inpstr + strlength - startpos, startpos);
        result[startpos] = '\0';
        obj = mc_value_makestringlen(state, result, startpos);
        mc_memory_free(result);
        return obj;
    }
    return Value::makeNull();
}

/**
 * \brief Replaces all occurances of one string in another.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return The string with all occurances replaced.
 */
Value mc_objfnstring_replaceall(State* state, void* data, Value thisval, size_t argc, Value* args)
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
    Value obj;
    Value inpval;
    Value searchval;
    Value repval;
    (void)data;
    (void)argc;
    if(args[0].getType() == Value::VALTYP_STRING && args[1].getType() == Value::VALTYP_STRING)
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
            return Value::makeNull();
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
        mc_memory_free(result);
        return obj;
    }
    return Value::makeNull();
}

/**
 * \brief Replaces the first occurance of one string in another.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return The string with the first occurance of the replacement replaced.
 */
Value mc_objfnstring_replacefirst(State* state, void* data, Value thisval, size_t argc, Value* args)
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
    Value obj;
    Value inpval;
    Value repval;
    Value searchval;
    (void)data;
    (void)argc;
    if(args[0].getType() == Value::VALTYP_STRING && args[1].getType() == Value::VALTYP_STRING)
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
            return Value::makeNull();
        }
        /* Replace the first instance of searchstr with replacestr */
        len = temp - inpstr;
        memcpy(result, inpstr, len);
        strcpy(result + len, replacestr);
        strcpy(result + len + replacelen, temp + searchlen);
        obj = mc_value_makestringlen(state, result, len);
        mc_memory_free(result);
        return obj;
    }
    return Value::makeNull();
}

/**
 * \brief Trims whitespace off the start and end of a string.
 * \param state Virtual Machine
 * \param data No clue what this is yet
 * \param argc The number of arguments
 * \param args The actual arguments
 * \return Returns a string that has whitespace trimmed from the start and finish.
 */
Value mc_objfnstring_trim(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int i;
    int j;
    int k;
    int inplen;
    char* result;
    const char* inpstr;
    Value obj;
    Value inpval;
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
    mc_memory_free(result);
    return obj;
}

Value mc_objfnstring_matchhelper(State* state, void* data, Value thisval, size_t argc, Value* args, bool icase)
{
    bool r;
    int flags;
    size_t inplen;
    size_t patlen;
    const char* inpstr;
    const char* patstr;
    Value patval;
    Value inpval;
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
    return Value::makeBool(r);
}

Value mc_objfnstring_matchglobcase(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnstring_matchhelper(state, data, thisval, argc, args, false);
}

Value mc_objfnstring_matchglobicase(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnstring_matchhelper(state, data, thisval, argc, args, false);
}

Value mc_objfnstring_tolower(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int inplen;
    const char* inpstr;
    Value resstr;
    Value inpval;
    (void)data;
    (void)argc;
    (void)args;
    inpval = thisval;
    inpstr = mc_value_stringgetdata(inpval);
    inplen = mc_value_stringgetlength(inpval);
    resstr = mc_value_makestringlen(state, inpstr, inplen);
    resstr.getAllocatedData()->uvobj.valstring.strbuf->toLowercase();
    return resstr;
}


Value mc_objfnstring_toupper(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int inplen;
    const char* inpstr;
    Value resstr;
    Value inpval;
    (void)data;
    (void)argc;
    (void)args;
    inpval = thisval;
    inpstr = mc_value_stringgetdata(inpval);
    inplen = mc_value_stringgetlength(inpval);
    resstr = mc_value_makestringlen(state, inpstr, inplen);
    resstr.getAllocatedData()->uvobj.valstring.strbuf->toUppercase();
    return resstr;    
}

Value mc_objfnstring_split(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    (void)state;
    (void)data;
    (void)thisval;
    (void)argc;
    (void)args;
    return Value::makeNull();
}

Value mc_objfnarray_length(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t len;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    len = mc_value_arraygetlength(thisval);
    return Value::makeNumber(len);
}

void mc_vm_savestate(State* state, mcexecstate_t* est)
{
    est->thisstpos = state->execstate.thisstpos;
    est->vsposition = state->execstate.vsposition;
    est->currframe = state->execstate.currframe;
}

void mc_vm_restorestate(State* state, mcexecstate_t* est)
{
    state->execstate.thisstpos = est->thisstpos;
    state->execstate.vsposition = est->vsposition;
    state->execstate.currframe = est->currframe;
}



Value mc_objfnarray_map(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t i;
    size_t len;
    Value val;
    Value res;
    Value callee;
    Value vargs[3];
    Value narr;
    GenericList<Value>* ary;
    GenericList<Value>* nary;
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
        vargs[1] = Value::makeNumber(i);
        res = state->vmCallValue(nullptr, callee, thisval, 1, vargs);
        nary->push(res);
    }
    return narr;
}

Value mc_objfnarray_push(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    bool ok;
    size_t i;
    size_t len;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_ARRAY, Value::VALTYP_ANY))
    {
        return Value::makeNull();
    }
    for(i=0; i<argc; i++)
    {
        ok = mc_value_arraypush(thisval, args[i]);
        if(!ok)
        {
            return Value::makeNull();
        }
    }
    len = mc_value_arraygetlength(thisval);
    return Value::makeNumber(len);
}

Value mc_objfnarray_pop(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    Value val;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_ARRAY, Value::VALTYP_ANY))
    {
        return Value::makeNull();
    }
    val = mc_valarray_pop(thisval);
    return val;
}

Value mc_objfnarray_join(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    bool havejoinee;
    int i;
    int slen;
    int alen;
    const char* str;
    Value rt;
    Value item;
    Value array;
    Value joinee;
    Printer pr;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    havejoinee = false;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_ARRAY, Value::VALTYP_ANY))
    {
        return Value::makeNull();
    }
    array= thisval;
    if(argc > 0)
    {
        havejoinee = true;
        joinee = args[0];
    }
    alen = mc_value_arraygetlength(array);
    Printer::initFromStack(&pr, nullptr, true);
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

Value mc_objfnmap_length(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t len;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    len = mc_value_mapgetlength(thisval);
    return Value::makeNumber(len);
}

Value mc_objfnmap_keys(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int i;
    int len;
    Value strval;
    Value arr;
    Value map;
    (void)data;
    (void)argc;
    (void)args;
    map = thisval;
    len = mc_value_mapgetlength(map);
    arr = mc_value_makearray(state);
    for(i=0; i<len; i++)
    {
        strval = mc_value_mapgetkeyat(map, i);
        mc_value_arraypush(arr, strval);
    }
    return arr;
}

Value mc_objfnutil_istype(State* state, void* data, Value thisval, size_t argc, Value* args, Value::Type vt)
{
    (void)data;
    (void)state;
    (void)argc;
    (void)args;
    return Value::makeBool(thisval.getType() == vt);
}

Value mc_objfnobject_iscallable(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    (void)data;
    (void)state;
    (void)argc;
    (void)args;
    return Value::makeBool(thisval.isCallable());
}

Value mc_objfnobject_isstring(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VALTYP_STRING);
}

Value mc_objfnobject_isarray(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VALTYP_ARRAY);
}

Value mc_objfnobject_ismap(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VALTYP_MAP);
}

Value mc_objfnobject_isnumber(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VALTYP_NUMBER);
}

Value mc_objfnobject_isbool(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VALTYP_BOOL);
}

Value mc_objfnobject_isnull(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VALTYP_NULL);
}

Value mc_objfnobject_isfuncscript(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VALTYP_FUNCSCRIPT);
}

Value mc_objfnobject_isexternal(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VALTYP_EXTERNAL);
}

Value mc_objfnobject_iserror(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VALTYP_ERROR);
}

Value mc_objfnobject_isfuncnative(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VALTYP_FUNCNATIVE);
}

void mc_state_makestdclasses(State* state)
{
    {
        state->stdobjobject = Memory::make<ObjClass>("Object", nullptr);
        state->stdobjobject->addMember("isString", mc_objfnobject_isstring);
        state->stdobjobject->addMember("isNumber", mc_objfnobject_isnumber);
        state->stdobjobject->addMember("isArray", mc_objfnobject_isarray);
        state->stdobjobject->addMember("isMap", mc_objfnobject_ismap);
        state->stdobjobject->addMember("isFuncNative", mc_objfnobject_isfuncnative);
        state->stdobjobject->addMember("isFuncScript", mc_objfnobject_isfuncscript);
        state->stdobjobject->addMember("isExternal", mc_objfnobject_isexternal);
        state->stdobjobject->addMember("isError", mc_objfnobject_iserror);
        state->stdobjobject->addMember("isNull", mc_objfnobject_isnull);
        state->stdobjobject->addMember("isBool", mc_objfnobject_isbool);
        state->stdobjobject->addMember("isCallable", mc_objfnobject_iscallable);

    }
    {
        state->stdobjnumber = Memory::make<ObjClass>("Number", state->stdobjobject);
        state->stdobjnumber->addMember("chr", mc_objfnnumber_chr);
        
    }
    {
        state->stdobjstring = Memory::make<ObjClass>("String", state->stdobjobject);
        state->stdobjstring->addPseudo("length", mc_objfnstring_length);
        state->stdobjstring->addMember("getself", mc_objfnstring_getself);
        state->stdobjstring->addMember("toNumber", mc_objfnstring_tonumber);
        state->stdobjstring->addMember("ord", mc_objfnstring_charcodefirst);
        state->stdobjstring->addMember("charCodeAt", mc_objfnstring_charcodeat);
        state->stdobjstring->addMember("charAt", mc_objfnstring_charat);
        state->stdobjstring->addMember("indexOf", mc_objfnstring_indexof);
        state->stdobjstring->addMember("left", mc_objfnstring_left);
        state->stdobjstring->addMember("right", mc_objfnstring_right);
        state->stdobjstring->addMember("replace", mc_objfnstring_replaceall);
        state->stdobjstring->addMember("replacefirst", mc_objfnstring_replacefirst);
        state->stdobjstring->addMember("match", mc_objfnstring_matchglobcase);
        state->stdobjstring->addMember("imatch", mc_objfnstring_matchglobicase);
        state->stdobjstring->addMember("trim", mc_objfnstring_trim);
        state->stdobjstring->addMember("toLower", mc_objfnstring_tolower);
        state->stdobjstring->addMember("toUpper", mc_objfnstring_toupper);
        state->stdobjstring->addMember("split", mc_objfnstring_split);

    }
    {
        state->stdobjarray = Memory::make<ObjClass>("Array", state->stdobjobject);
        state->stdobjarray->addPseudo("length", mc_objfnarray_length);
        state->stdobjarray->addMember("push", mc_objfnarray_push);
        state->stdobjarray->addMember("pop", mc_objfnarray_pop);
        state->stdobjarray->addMember("join", mc_objfnarray_join);
        state->stdobjarray->addMember("map", mc_objfnarray_map);
    }
    {
        state->stdobjmap = Memory::make<ObjClass>("Map", state->stdobjobject);
        state->stdobjmap->addPseudo("length", mc_objfnmap_length);
        state->stdobjmap->addMember("keys", mc_objfnmap_keys);
    }
    {
        state->stdobjfunction = Memory::make<ObjClass>("Function", state->stdobjobject);
    }
}

Value mc_scriptfn_isnan(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    mcfloat_t val;
    bool b;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER))
    {
        return Value::makeNull();
    }
    val = Value::asNumber(args[0]);
    b = false;
    if(val != val)
    {
        b = true;
    }
    return Value::makeBool(b);
}

Value mc_scriptfn_range(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    bool ok;
    size_t ai;
    int i;
    int start;
    int end;
    int step;
    const char* typestr;
    const char* expectedstr;
    Value::Type type;
    Value res;
    Value item;
    (void)data;
    (void)thisval;
    for(ai = 0; ai < argc; ai++)
    {
        type = args[ai].getType();
        if(type != Value::VALTYP_NUMBER)
        {
            typestr = Value::getTypename(type);
            expectedstr = Value::getTypename(Value::VALTYP_NUMBER);
            state->pushError(MC_ERROR_RUNTIME, AstLocation::Invalid(), "invalid argument %d passed to range, got %s instead of %s", ai, typestr, expectedstr);
            return Value::makeNull();
        }
    }
    start = 0;
    end = 0;
    step = 1;
    if(argc == 1)
    {
        end = (int)Value::asNumber(args[0]);
    }
    else if(argc == 2)
    {
        start = (int)Value::asNumber(args[0]);
        end = (int)Value::asNumber(args[1]);
    }
    else if(argc == 3)
    {
        start = (int)Value::asNumber(args[0]);
        end = (int)Value::asNumber(args[1]);
        step = (int)Value::asNumber(args[2]);
    }
    else
    {
        state->pushError(MC_ERROR_RUNTIME, AstLocation::Invalid(), "invalid number of arguments passed to range, got %d", argc);
        return Value::makeNull();
    }
    if(step == 0)
    {
        state->m_errorlist.pushFormat(MC_ERROR_RUNTIME, AstLocation::Invalid(), "range step cannot be 0");
        return Value::makeNull();
    }
    res = mc_value_makearray(state);
    if(res.isNull())
    {
        return Value::makeNull();
    }
    for(i = start; i < end; i += step)
    {
        item = Value::makeNumber(i);
        ok = mc_value_arraypush(res, item);
        if(!ok)
        {
            return Value::makeNull();
        }
    }
    return res;
}

Value mc_scriptfn_keys(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    bool ok;
    int i;
    int len;
    Value arg;
    Value res;
    Value key;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_MAP))
    {
        return Value::makeNull();
    }
    arg = args[0];
    res = mc_value_makearray(state);
    if(res.isNull())
    {
        return Value::makeNull();
    }
    len = mc_value_mapgetlength(arg);
    for(i = 0; i < len; i++)
    {
        key = mc_value_mapgetkeyat(arg, i);
        ok = mc_value_arraypush(res, key);
        if(!ok)
        {
            return Value::makeNull();
        }
    }
    return res;
}

Value mc_scriptfn_values(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    bool ok;
    int i;
    int len;
    Value key;
    Value arg;
    Value res;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_MAP))
    {
        return Value::makeNull();
    }
    arg = args[0];
    res = mc_value_makearray(state);
    if(res.isNull())
    {
        return Value::makeNull();
    }
    len = mc_value_mapgetlength(arg);
    for(i = 0; i < len; i++)
    {
        key = mc_value_mapgetvalueat(arg, i);
        ok = mc_value_arraypush(res, key);
        if(!ok)
        {
            return Value::makeNull();
        }
    }
    return res;
}

Value mc_scriptfn_copy(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_ANY))
    {
        return Value::makeNull();
    }
    return Value::copyFlat(state, args[0]);
}

Value mc_scriptfn_copydeep(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_ANY))
    {
        return Value::makeNull();
    }
    return Value::copyDeep(state, args[0]);
}

Value mc_scriptfn_remove(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    bool res;
    int i;
    int ix;
    Value obj;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_ARRAY, Value::VALTYP_ANY))
    {
        return Value::makeNull();
    }
    ix = -1;
    for(i = 0; i < mc_value_arraygetlength(args[0]); i++)
    {
        obj = mc_value_arraygetvalue(args[0], i);
        if(Value::equalsTo(obj, args[1]))
        {
            ix = i;
            break;
        }
    }
    if(ix == -1)
    {
        return Value::makeBool(false);
    }
    res = mc_value_arrayremoveat(args[0], ix);
    return Value::makeBool(res);
}

Value mc_scriptfn_removeat(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    bool res;
    int ix;
    Value::Type type;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_ARRAY, Value::VALTYP_NUMBER))
    {
        return Value::makeNull();
    }
    type= args[0].getType();
    ix = (int)Value::asNumber(args[1]);
    switch(type)
    {
        case Value::VALTYP_ARRAY:
            {
                res = mc_value_arrayremoveat(args[0], ix);
                return Value::makeBool(res);
            }
            break;
        default:
            {
            }
            break;
    }
    return Value::makeBool(true);
}

Value mc_scriptfn_error(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    (void)data;
    (void)thisval;
    if(argc == 1 && args[0].getType() == Value::VALTYP_STRING)
    {
        return mc_value_makeerror(state, mc_value_stringgetdata(args[0]));
    }
    return mc_value_makeerror(state, "");
}

Value mc_scriptfn_crash(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    (void)data;
    (void)thisval;
    if(argc == 1 && args[0].getType() == Value::VALTYP_STRING)
    {
        state->m_errorlist.pushMessage(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), mc_value_stringgetdata(args[0]));
    }
    else
    {
        state->m_errorlist.pushMessage(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), "");
    }
    return Value::makeNull();
}

Value mc_scriptfn_assert(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_BOOL))
    {
        return Value::makeNull();
    }
    if(!Value::asBool(args[0]))
    {
        state->m_errorlist.pushFormat(MC_ERROR_RUNTIME, AstLocation::Invalid(), "assertion failed");
        return Value::makeNull();
    }
    return Value::makeBool(true);
}

Value mc_scriptfn_randseed(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int seed;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER))
    {
        return Value::makeNull();
    }
    seed = (int)Value::asNumber(args[0]);
    srand(seed);
    return Value::makeBool(true);
}

Value mc_scriptfn_random(State* state, void* data, Value thisval, size_t argc, Value* args)
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
        return Value::makeNumber(res);
    }
    if(argc == 2)
    {
        if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER, Value::VALTYP_NUMBER))
        {
            return Value::makeNull();
        }
        min = Value::asNumber(args[0]);
        max = Value::asNumber(args[1]);
        if(min >= max)
        {
            state->m_errorlist.pushFormat(MC_ERROR_RUNTIME, AstLocation::Invalid(), "max is bigger than min");
            return Value::makeNull();
        }
        range = max - min;
        res = min + (res * range);
        return Value::makeNumber(res);
    }
    state->m_errorlist.pushFormat(MC_ERROR_RUNTIME, AstLocation::Invalid(), "invalid number or arguments");
    return Value::makeNull();
}

Value mc_scriptutil_slicearray(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int i;
    int len;
    int index;
    bool ok;
    Value res;
    Value item;
    (void)data;
    (void)argc;
    (void)thisval;
    index = (int)Value::asNumber(args[1]);
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
    if(res.isNull())
    {
        return Value::makeNull();
    }
    for(i = index; i < len; i++)
    {
        item = mc_value_arraygetvalue(args[0], i);
        ok = mc_value_arraypush(res, item);
        if(!ok)
        {
            return Value::makeNull();
        }
    }
    return res;
}

Value mc_scriptutil_slicestring(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int i;
    int len;
    int index;
    char c;
    Value res;
    const char* str;
    (void)data;
    (void)argc;
    (void)thisval;
    index = (int)Value::asNumber(args[1]);
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
    if(res.isNull())
    {
        return Value::makeNull();
    }
    for(i = index; i < len; i++)
    {
        c = str[i];
        mc_value_stringappendlen(res, &c, 1);
    }
    return res;
}

Value mc_scriptfn_slice(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    const char* typestr;
    Value::Type argtype;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_STRING | Value::VALTYP_ARRAY, Value::VALTYP_NUMBER))
    {
        return Value::makeNull();
    }
    argtype = args[0].getType();
    if(argtype == Value::VALTYP_ARRAY)
    {
        return mc_scriptutil_slicearray(state, data, thisval, argc, args);
    }
    if(argtype == Value::VALTYP_STRING)
    {
        return mc_scriptutil_slicestring(state, data, thisval, argc, args);
    }
    typestr = Value::getTypename(argtype);
    state->pushError(MC_ERROR_RUNTIME, AstLocation::Invalid(), "invalid argument 0 passed to slice, got %s instead", typestr);
    return Value::makeNull();
}

Value mc_nsfnmath_sqrt(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER))
    {
        return Value::makeNull();
    }
    arg = Value::asNumber(args[0]);
    res = sqrt(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_pow(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    mcfloat_t arg1;
    mcfloat_t arg2;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER, Value::VALTYP_NUMBER))
    {
        return Value::makeNull();
    }
    arg1 = Value::asNumber(args[0]);
    arg2 = Value::asNumber(args[1]);
    res = pow(arg1, arg2);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_sin(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER))
    {
        return Value::makeNull();
    }
    arg = Value::asNumber(args[0]);
    res = sin(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_cos(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER))
    {
        return Value::makeNull();
    }
    arg = Value::asNumber(args[0]);
    res = cos(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_tan(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER))
    {
        return Value::makeNull();
    }
    arg = Value::asNumber(args[0]);
    res = tan(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_log(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER))
    {
        return Value::makeNull();
    }
    arg = Value::asNumber(args[0]);
    res = log(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_ceil(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER))
    {
        return Value::makeNull();
    }
    arg = Value::asNumber(args[0]);
    res = ceil(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_floor(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER))
    {
        return Value::makeNull();
    }
    arg = Value::asNumber(args[0]);
    res = floor(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_abs(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    mcfloat_t arg;
    mcfloat_t res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_NUMBER))
    {
        return Value::makeNull();
    }
    arg = Value::asNumber(args[0]);
    res = MC_UTIL_FABS(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnfile_writefile(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int slen;
    int printedsz;
    const char* path;
    const char* string;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_STRING, Value::VALTYP_STRING))
    {
        return Value::makeNull();
    }
    path = mc_value_stringgetdata(args[0]);
    string = mc_value_stringgetdata(args[1]);
    slen = mc_value_stringgetlength(args[1]);
    printedsz = mc_fsutil_filewrite(path, string, slen);
    return Value::makeNumber(printedsz);
}

Value mc_nsfnfile_readfile(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t flen;
    char* contents;
    const char* path;
    Value res;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_STRING))
    {
        return Value::makeNull();
    }
    path = mc_value_stringgetdata(args[0]);
    contents = mc_fsutil_fileread(path, &flen);
    if(!contents)
    {
        return Value::makeNull();
    }
    res = mc_value_makestringlen(state, contents, flen);
    mc_memory_free(contents);
    contents = nullptr;
    return res;
}

Value mc_nsfnfile_join(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t i;
    Value res;
    Value arg;
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

Value mc_nsfnfile_isdirectory(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    const char* path;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_STRING))
    {
        return Value::makeNull();
    }
    path = mc_value_stringgetdata(args[0]);
    return Value::makeBool(osfn_pathisdirectory(path));
}

Value mc_nsfnfile_isfile(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    const char* path;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_STRING))
    {
        return Value::makeNull();
    }
    path = mc_value_stringgetdata(args[0]);
    return Value::makeBool(osfn_pathisfile(path));
}

Value mc_nsfnfile_stat(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    Value resmap;
    const char* path;
    const char* fullpath;
    struct stat st;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    char fpbuffer[1024];
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_STRING))
    {
        return Value::makeNull();
    }
    path = mc_value_stringgetdata(args[0]);
    if(stat(path, &st) == 0)
    {
        resmap = mc_value_makemap(state);
        fullpath = osfn_realpath(path, fpbuffer);
        mc_value_mapsetvaluestring(resmap, "dev", Value::makeNumber(st.st_dev));
        mc_value_mapsetvaluestring(resmap, "ino", Value::makeNumber(st.st_ino));
        mc_value_mapsetvaluestring(resmap, "mode", Value::makeNumber(st.st_mode));
        mc_value_mapsetvaluestring(resmap, "nlink", Value::makeNumber(st.st_nlink));
        mc_value_mapsetvaluestring(resmap, "uid", Value::makeNumber(st.st_uid));
        mc_value_mapsetvaluestring(resmap, "gid", Value::makeNumber(st.st_gid));
        mc_value_mapsetvaluestring(resmap, "size", Value::makeNumber(st.st_size));
        mc_value_mapsetvaluestring(resmap, "path", mc_value_makestring(state, fullpath));
        return resmap;
    }
    return Value::makeNull();
}

Value mc_nsfndir_readdir(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    bool isdot;
    bool joinpaths;
    const char* path;
    Value res;
    Value vjustname;
    Value vfullpath;
    Value vpath;
    Value vrespath;
    FSDirItem ent;
    FSDirReader reader;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    joinpaths = false;
    if(!mc_argcheck_check(state, true, argc, args, Value::VALTYP_STRING, Value::VALTYP_BOOL))
    {
        return Value::makeNull();
    }
    vpath = args[0];
    path = mc_value_stringgetdata(vpath);
    if(argc > 1)
    {
        joinpaths = Value::asBool(args[1]);
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
    state->m_errorlist.pushMessage(MC_ERROR_RUNTIME, state->execstate.currframe->getPosition(), strerror(errno));
    return Value::makeNull();
}

Value mc_nsfnvm_hadrecovered(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    (void)data;
    (void)thisval;
    (void)argc;
    (void)args;
    return Value::makeBool(state->hadrecovered);
}

void mc_cli_installbuiltins(State* state)
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
        state->setGlobalFunction(nativefunctions[i].name, nativefunctions[i].fn, nullptr);
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

void mc_cli_installjsondummy(State* state)
{
    Value jmap;
    jmap = mc_value_makemap(state);
    mc_value_mapsetstrfunc(jmap, "stringify", mc_nsfnjson_stringify);
    state->setGlobalValue("JSON", jmap);
}

void mc_cli_installjsconsole(State* state)
{
    Value jmap;
    jmap = mc_value_makemap(state);
    mc_value_mapsetstrfunc(jmap, "log", mc_scriptfn_println);
    state->setGlobalValue("console", jmap);
}

void mc_cli_installmath(State* state)
{
    Value jmap;
    jmap = mc_value_makemap(state);
    mc_value_mapsetstrfunc(jmap, "sqrt", mc_nsfnmath_sqrt);
    mc_value_mapsetstrfunc(jmap, "pow", mc_nsfnmath_pow);
    mc_value_mapsetstrfunc(jmap, "sin", mc_nsfnmath_sin);
    mc_value_mapsetstrfunc(jmap, "cos", mc_nsfnmath_cos);
    mc_value_mapsetstrfunc(jmap, "tan", mc_nsfnmath_tan);
    mc_value_mapsetstrfunc(jmap, "log", mc_nsfnmath_log);
    mc_value_mapsetstrfunc(jmap, "ceil", mc_nsfnmath_ceil);
    mc_value_mapsetstrfunc(jmap, "floor", mc_nsfnmath_floor);
    mc_value_mapsetstrfunc(jmap, "abs", mc_nsfnmath_abs);
    state->setGlobalValue("Math", jmap);
}

void mc_cli_installfauxjavascript(State* state)
{
    mc_cli_installjsondummy(state);
    mc_cli_installjsconsole(state);
    mc_cli_installmath(state);
}

void mc_cli_installfileio(State* state)
{
    Value map;
    map = mc_value_makemap(state);
    mc_value_mapsetstrfunc(map, "read", mc_nsfnfile_readfile);
    mc_value_mapsetstrfunc(map, "write", mc_nsfnfile_writefile);
    mc_value_mapsetstrfunc(map, "put", mc_nsfnfile_writefile);
    mc_value_mapsetstrfunc(map, "join", mc_nsfnfile_join);
    mc_value_mapsetstrfunc(map, "isDirectory", mc_nsfnfile_isdirectory);
    mc_value_mapsetstrfunc(map, "isFile", mc_nsfnfile_isfile);
    mc_value_mapsetstrfunc(map, "stat", mc_nsfnfile_stat);
    state->setGlobalValue("File", map);
}

void mc_cli_installdir(State* state)
{
    Value map;
    map = mc_value_makemap(state);
    mc_value_mapsetstrfunc(map, "read", mc_nsfndir_readdir);
    state->setGlobalValue("Dir", map);
}

void mc_cli_installvmvar(State* state)
{
    Value map;
    map = mc_value_makemap(state);
    mc_value_mapsetstrfunc(map, "hadRecovered", mc_nsfnvm_hadrecovered);
    state->setGlobalValue("VM", map);
}


static int g_extfnvar;

void mc_cli_installotherstuff(State* state)
{
    state->setGlobalValue("test", Value::makeNumber(42));
    state->setGlobalFunction("external_fn_test", mc_scriptfn_externalfn, &g_extfnvar);
    state->setGlobalFunction("test_check_args", mc_scriptfn_testcheckargs, nullptr);
    state->setGlobalFunction("vec2_add", mc_scriptfn_vec2add, nullptr);
    state->setGlobalFunction("vec2_sub", mc_scriptfn_vec2sub, nullptr);
    mc_cli_installfileio(state);
    mc_cli_installdir(state);
    mc_cli_installvmvar(state);
}


bool mc_cli_compileandrunsource(State* state, Value* vdest, const char* source, const char* filename)
{
    bool ok;
    Value tmp;
    CompiledProgram* program;
    ok = false;
    program = state->compileSource(source, filename);
    if(state->m_config.exitaftercompiling)
    {
        Memory::destroy(program);
        return true;
    }
    tmp = mc_program_execute(state, program);
    if(state->hasErrors())
    {
        state->printErrors();
    }
    else
    {
        ok = true;
    }
    if(vdest != nullptr)
    {
        *vdest =  tmp;
    }
    Memory::destroy(program);
    return ok;
}

bool mc_cli_compileandrunfile(State* state, const char* filename)
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
    mc_memory_free(code);
    return ok;
}

void mc_cli_installargv(State* state, int argc, char** argv, int beginat)
{
    int i;
    Value strval;
    Value argvobj;
    argvobj = mc_value_makearray(state);
    for(i=beginat; i<argc; i++)
    {
        strval = mc_value_makestring(state, argv[i]);
        mc_value_arraypush(argvobj, strval);
    }
    state->setGlobalValue("ARGV", argvobj);
}

#define printtypesize(...) \
    mc_cli_printtypesize(#__VA_ARGS__, sizeof(__VA_ARGS__))

void mc_cli_printtypesize(const char* name, size_t sz)
{
    printf("%ld\t%s\n", sz, name);
}

void mc_cli_printtypesizes()
{
    printtypesize(PtrDict);
    printtypesize(GenericDict<Value, Value>);
    printtypesize(PtrList);
    printtypesize(Printer::Config);
    printtypesize(Printer);
    printtypesize(Error);
    printtypesize(Traceback);
    printtypesize(AstSourceFile);
    printtypesize(AstExpression);
    printtypesize(CompiledProgram);
    printtypesize(State);
    printtypesize(GCMemory);
    printtypesize(SymStore);
    printtypesize(mcobjdata_t);
    printtypesize(ErrList);
    printtypesize(AstParser);
    printtypesize(mcconfig_t);
    printtypesize(AstSymTable);
    printtypesize(AstCompiler);
    printtypesize(AstSymbol);
    printtypesize(Value);
    printtypesize(Value::CompareResult);
    printtypesize(AstLocation);
    printtypesize(Token);
    printtypesize(AstExpression::ExprCodeBlock);
    printtypesize(AstExpression::ExprLiteralMap);
    printtypesize(AstExpression::ExprLiteralArray);
    printtypesize(AstExpression::ExprLiteralString);
    printtypesize(AstExpression::ExprPrefix);
    printtypesize(AstExpression::ExprInfix);
    printtypesize(AstExpression::ExprIfCase);
    printtypesize(AstExpression::ExprLiteralFunction);
    printtypesize(AstExpression::ExprCall);
    printtypesize(AstExpression::ExprIndex);
    printtypesize(AstExpression::ExprAssign);
    printtypesize(AstExpression::ExprLogical);
    printtypesize(AstExpression::ExprTernary);
    printtypesize(AstExpression::ExprIdent);
    printtypesize(AstExpression::ExprFuncParam);
    printtypesize(AstExpression::ExprDefine);
    printtypesize(AstExpression::ExprIfStmt);
    printtypesize(AstExpression::ExprWhileStmt);
    printtypesize(AstExpression::ExprForeachStmt);
    printtypesize(AstExpression::ExprLoopStmt);
    printtypesize(AstExpression::ExprImportStmt);
    printtypesize(AstExpression::ExprRecover);
    printtypesize(mcobjfunction_t);
    printtypesize(mcobjuserdata_t);
    printtypesize(mcobjerror_t);
    printtypesize(mcobjstring_t);
    printtypesize(mcopdefinition_t);
    printtypesize(mcastscopeblock_t);
    printtypesize(AstScopeFile);
    printtypesize(AstScopeComp);
    printtypesize(AstLexer);
    printtypesize(AstInfo);
    printtypesize(VMFrame);
    printtypesize(Traceback::Item);
    printtypesize(Module);
    printtypesize(mcstoddiyfpconv_t);
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
    Value tmp;
    State* state;
    optcontext_t options;
    ok = true;
    evalcode = nullptr;
    ok = true;
    nargc = 0;
    state = Memory::make<State>();
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
            state->m_config.dumpast = true;
        }
        else if(co == 'd')
        {
            state->m_config.dumpbytecode = true;
        }
        else if(co == 'x')
        {
            state->m_config.exitaftercompiling = true;
        }
        else if(co == 'p')
        {
            state->m_config.printinstructions = true;
        }
        else if(co == 's')
        {
            state->m_config.strictmode = true;
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
    Memory::destroy(state);
    fprintf(stderr, "ok=%d\n", ok);
    if(ok)
    {
        return 0;
    }
    return 1;
}

