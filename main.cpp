

#include <memory>
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

#if 0
    #define MC_UTIL_CMPFLOAT(a, b) (fabs((a) - (b)) < DBL_EPSILON)
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

typedef struct mcstoddiyfp_t mcstoddiyfp_t;

class Object;
class Printer;
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
class AstToken;
class SymStore;
class ObjClass;
class AstScopeBlock;

typedef Value (*mcnativefn_t)(State*, void*, Value, size_t, Value*);
typedef size_t (*mcitemhashfn_t)(void*);
typedef bool (*mcitemcomparefn_t)(void*, void*);
typedef void (*mcitemdestroyfn_t)(void*);
typedef void* (*mcitemcopyfn_t)(void*);
typedef void (*mcitemdeinitfn_t)(void*);
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
MC_INLINE const char* mc_value_stringgetdata(const Value& object);

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
int mc_state_mapgetlength(Value object);
Value mc_state_mapgetkeyat(Value object, int ix);
Value mc_state_mapgetvalueat(Value object, int ix);
bool mc_state_mapsetvalueat(Value object, int ix, Value val);
Value mc_state_mapgetkvpairat(State *state, Value object, int ix);
bool mc_state_mapsetvalue(Value object, Value key, Value val);
bool mc_state_mapsetvaluestring(State* state, Value object, const char *strkey, Value val);
Value mc_state_mapgetvalue(Value object, Value key);
bool mc_state_mapgetvaluechecked(Value object, Value key, Value *dest);
bool mc_state_maphaskey(Value object, Value key);
void mc_objectdata_deinit(Object *data);

void mc_printer_printoneinstruc(Printer *pr, uint16_t *code, uint16_t op, size_t *pos, AstLocation *sposlist, bool simple);
void mc_printer_printbytecode(Printer *pr, uint16_t *code, AstLocation *sposlist, size_t codesize, bool simple);
void mc_printer_printobjstring(Printer *pr, Value obj);
void mc_printer_printobjfuncscript(Printer *pr, Value obj);
void mc_printer_printobjarray(Printer *pr, Value obj);
void mc_printer_printobjmap(Printer *pr, Value obj);
void mc_printer_printobjerror(Printer *pr, Value obj);
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

Object *mc_gcmemory_allocobjectdata(State *state);

void mc_state_gcunmarkall(State *state);
void mc_state_gcmarkobjlist(Value *objects, size_t count);
void mc_state_gcmarkobject(Value obj);
void mc_state_gcsweep(State *state);
int mc_state_gcshouldsweep(State *state);
bool mc_state_gcdisablefor(Value obj);
void mc_state_gcenablefor(Value obj);
bool mc_state_gccandatabeputinpool(State *state, Object *data);

bool mc_vm_init(State *state);
void mc_vm_reset(State *state);

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

namespace Console
{
    class Color
    {
        private:
            int m_targetfds[5];
            size_t m_fdcount;
            bool m_ispiped;

        public:    
            Color(int fd)
            {
                size_t i;
                m_targetfds[0] = fd;
                m_fdcount = 1;
                m_ispiped = false;
                if(fd == -1)
                {
                    m_ispiped = true;
                }
                else
                {
                    for(i=0;i<m_fdcount; i++)
                    {
                        if(isatty(m_targetfds[i]) == 0)
                        {
                            m_ispiped = true;
                            break;
                        }
                    }
                }
            }

            const char* get(char code)
            {
                if(m_ispiped)
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

    };
}

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
            if(took)
            {
                return;
            }
            if(pr != nullptr)
            {
                if(pr->m_prdestbuf != nullptr)
                {
                    StringBuffer::destroy(pr->m_prdestbuf);
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

        inline void flush()
        {
            return;
            if(m_prdestrfile != nullptr)
            {
                //if(m_prconfig.shouldflush)
                {
                    fflush(m_prdestrfile);
                }
            }
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
            }
            flush();
            return true;
        }

        inline bool put(const char* str)
        {
            return put(str, mc_util_strlen(str));
        }

        inline bool putChar(int b)
        {
            char ch;
            ch = b;
            return put(&ch, 1);
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
            }
            flush();
            return true;
        }

        inline void printEscapedChar(int ch)
        {
            switch(ch)
            {
                case '\'':
                    {
                        put("\\\'");
                    }
                    break;
                case '\"':
                    {
                        put("\\\"");
                    }
                    break;
                case '\\':
                    {
                        put("\\\\");
                    }
                    break;
                case '\b':
                    {
                        put("\\b");
                    }
                    break;
                case '\f':
                    {
                        put("\\f");
                    }
                    break;
                case '\n':
                    {
                        put("\\n");
                    }
                    break;
                case '\r':
                    {
                        put("\\r");
                    }
                    break;
                case '\t':
                    {
                        put("\\t");
                    }
                    break;
                case 0:
                    {
                        put("\\0");
                    }
                    break;
                default:
                    {
                        format("\\x%02x", (unsigned char)ch);
                    }
                    break;
            }
        }

        inline void printEscapedString(const char* str, size_t len)
        {
            int ch;
            size_t i;
            put("\"");
            for(i=0; i<len; i++)
            {
                ch = str[i];
                if((ch < 32) || (ch > 127) || (ch == '\"') || (ch == '\\'))
                {
                    printEscapedChar(ch);
                }
                else
                {
                    putChar(ch);
                }
            }
            put("\"");
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
                #if defined(PRIiFAST64)
                    format("%" PRIiFAST64 "", inum);
                #else
                    format("%ld", inum);
                #endif
            }
            else
            {
                format("%g", flt);
            }
        }
};

template<typename ValType>
class GenericList
{
    public:
        template<typename OutputT, typename InputT>
        using DummyCopyFN = OutputT(*)(InputT);

        template<typename InputT>
        using DummyDestroyFN = void(*)(InputT);


    public:
        static void destroy(GenericList* list)
        {
            if(list != nullptr)
            {
                list->deInit();
                mc_memory_free(list);
                list = nullptr;
            }
        }

        template<typename InputT>
        static inline void destroy(GenericList* list, DummyDestroyFN<InputT> dfn)
        {
            if(list != nullptr)
            {    
                if(dfn != nullptr)
                {
                    clearAndDestroy(list, dfn);
                }
                list->deInit();
                mc_memory_free(list);
            }
        }

        template<typename InputT>
        static inline void clearAndDestroy(GenericList* list, DummyDestroyFN<InputT> dfn)
        {
            size_t i;
            for(i = 0; i < list->count(); i++)
            {
                auto item = list->get(i);
                if(dfn != nullptr)
                {
                    dfn((InputT)item);
                }
            }
            list->clear();
        }

        template<typename InputT, typename OutputT>
        inline GenericList* copyToHeap(DummyCopyFN<InputT, OutputT> copyfn, DummyDestroyFN<InputT> dfn)
        {
            bool ok;
            size_t i;
            GenericList* arrcopy;
            (void)ok;
            arrcopy = Memory::make<GenericList<ValType>>(m_listcapacity, m_nullvalue);
            for(i = 0; i < count(); i++)
            {
                auto item = (ValType)get(i);
                if(copyfn)
                {
                    auto itemcopy = (ValType)copyfn(item);
                    if(!arrcopy->push(itemcopy))
                    {
                        goto listcopyfailed;
                    }
                }
                else
                {
                    if(!arrcopy->push(item))
                    {
                        goto listcopyfailed;
                    }
                }
            }
            return arrcopy;
        listcopyfailed:
            Memory::destroy(arrcopy, dfn);
            return nullptr;
        }


        GenericList* copyToHeap()
        {
            DummyCopyFN<ValType, ValType> dummycopy = nullptr;
            DummyDestroyFN<ValType> dummydel = nullptr;
            return copyToHeap(dummycopy, dummydel);
        }


        template<typename InputT, typename OutputT>
        inline bool copyToStack(GenericList* dest, DummyCopyFN<InputT, OutputT> copyfn, DummyDestroyFN<InputT> dfn)
        {
            bool ok;
            size_t i;
            (void)ok;
            (void)dfn;
            for(i = 0; i < count(); i++)
            {
                auto item = (ValType)get(i);
                if(copyfn)
                {
                    auto itemcopy = (ValType)copyfn(item);
                    if(!dest->push(itemcopy))
                    {
                        goto listcopyfailed;
                    }
                }
                else
                {
                    if(!dest->push(item))
                    {
                        goto listcopyfailed;
                    }
                }
            }
            return true;
        listcopyfailed:
            return false;
        }

        bool copyToStack(GenericList* dest)
        {
            DummyCopyFN<ValType, ValType> dummycopy = nullptr;
            DummyDestroyFN<ValType> dummydel = nullptr;
            return copyToStack(dest, dummycopy, dummydel);
        }

        template<typename InputT, typename OutputT>
        inline GenericList copyToStack(DummyCopyFN<InputT, OutputT> copyfn, DummyDestroyFN<InputT> dfn)
        {
            GenericList dest;
            copyToStack(&dest, copyfn, dfn);
            return dest;
        }

        GenericList copyToStack()
        {
            GenericList dest;
            copyToStack(&dest);
            return dest;
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
        inline GenericList(): GenericList(0, /*was nullptr*/ ValType{})
        {
        }

        inline GenericList(size_t initialsize, ValType nullval)
        {
            m_listcount = 0;
            m_listcapacity = 0;
            m_listitems = nullptr;
            m_nullvalue = nullval;
            if(initialsize > 0)
            {
                ensureCapacity(initialsize, m_nullvalue, true);
            }
        }

        inline ~GenericList()
        {
            //deInit();
        }

        GenericList& operator=(const GenericList& other)
        {
            m_listcapacity = other.m_listcapacity;
            m_listcount = other.m_listcount;
            m_listitems = other.m_listitems;
            m_nullvalue = other.m_nullvalue;
            return *this;
        }

        inline void orphanData()
        {
            m_listcount = 0;
            m_listcapacity = 0;
            m_listitems = nullptr;
        }

        template<typename InputT>
        inline void deInit(DummyDestroyFN<InputT> dfn)
        {
            size_t i;
            for(i=0; i<m_listcount; i++)
            {
                auto item = get(i);
                dfn(item);
            }
            deInit();
        }

        inline void deInit()
        {
            mc_memory_free(m_listitems);
            m_listitems = nullptr;
            m_listcount = 0;
            m_listcapacity = 0;
        }

        inline void clear()
        {
            m_listcount = 0;
        }

        inline size_t count() const
        {
            return m_listcount;
        }

        inline ValType* data() const
        {
            return m_listitems;
        }

        inline ValType get(size_t idx) const
        {
            return m_listitems[idx];
        }

        inline ValType* getp(size_t idx) const
        {
            return &m_listitems[idx];
        }

        inline ValType top() const
        {
            if(m_listcount == 0)
            {
                return m_nullvalue;
            }
            return get(m_listcount - 1);
        }

        inline ValType* topp() const
        {
            if(m_listcount == 0)
            {
                return nullptr;
            }
            return getp(m_listcount - 1);
        }


        inline ValType* set(size_t idx, ValType val)
        {
            size_t need;
            need = idx + 1;
            if(((idx == 0) || (m_listcapacity == 0)) || (idx >= m_listcapacity))
            {
                ensureCapacity(need, m_nullvalue, false);
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
                if(dest != nullptr)
                {
                    *dest = m_listitems[m_listcount - 1];
                }
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
            return removeAtIntern(ix);
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

#include "strdict.h"
#include "valdict.h"

struct RuntimeConfig
{
    bool dumpast;
    bool exitafterastdump;
    bool dumpbytecode;
    bool printinstructions;
    bool fatalcomplaints;
    bool exitaftercompiling;
    bool strictmode;
    /* allows redefinition of symbols */
    bool replmode;
};

class ValData
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
        Type m_valtype;
        bool m_isallocated;        
        union
        {
            Object* odata;
            mcfloat_t valnumber;
            int valbool;
        } m_uval;
};

class Object
{
    public:
        enum
        {
            MaxFreeVal = (2),
        };

        struct ObjFunction
        {
            public:
                union
                {
                    struct
                    {
                        union
                        {
                            Value* freevalsallocated;
                            ValData freevalsstack[MaxFreeVal];
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
                } m_funcdata;

            public:
                inline bool freeValuesAreAllocated()
                {
                    return m_funcdata.valscriptfunc.freevalscount >= MaxFreeVal;
                }
        };

        struct ObjUserdata
        {
            void* data;
            mcitemdestroyfn_t datadestroyfn;
            mcitemcopyfn_t datacopyfn;
        };

        struct ObjArray
        {
            GenericList<Value>* actualarray;
        };

        struct ObjMap
        {
            ValDict<Value, Value>* actualmap;
        };

        struct ObjString
        {
            unsigned long hash;
            StringBuffer* strbuf;
        };

        struct ObjError
        {
            char* message;
            Traceback* traceback;
        };

        union ValUnion
        {
            ObjString valstring;
            ObjError valerror;
            ObjArray* valarray;
            ObjMap* valmap;
            ObjFunction valfunc;
            ObjUserdata valuserobject;
        };

    public:
        int8_t m_odtype;
        int8_t m_gcmark;
        GCMemory* m_objmem;
        State* m_pstate;
        ValUnion m_uvobj;

    public:
        Object(State* state)
        {
            m_pstate = state;
        }
};

class Value: public ValData
{
    public:
        static inline bool isHashable(Value obj)
        {
            Type type = obj.getType();
            switch(type)
            {
                case VALTYP_STRING:
                case VALTYP_NUMBER:
                case VALTYP_BOOL:
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
                case VALTYP_NONE:
                    return "none";
                case VALTYP_ERROR:
                    return "error";
                case VALTYP_NUMBER:
                    return "number";
                case VALTYP_BOOL:
                    return "boolean";
                case VALTYP_STRING:
                    return "string";
                case VALTYP_NULL:
                    return "null";
                case VALTYP_ARRAY:
                    return "array";
                case VALTYP_MAP:
                    return "object";
                case VALTYP_FUNCSCRIPT:
                case VALTYP_FUNCNATIVE:
                    return "function";
                case VALTYP_EXTERNAL:
                    return "external";
                case VALTYP_FREED:
                    return "freed";
                case VALTYP_ANY:
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
                res->put(getTypename(t));
                *inbetween = true;
            }
        }

        static char* getUnionName(Type type)
        {
            bool inbetween;
            Printer* res;
            if(type == VALTYP_ANY || type == VALTYP_NONE || type == VALTYP_FREED)
            {
                return mc_util_strdup(getTypename(type));
            }
            res = Memory::make<Printer>(nullptr);
            inbetween = false;
            unionNameCheckType(res, type, VALTYP_NUMBER, &inbetween);
            unionNameCheckType(res, type, VALTYP_BOOL, &inbetween);
            unionNameCheckType(res, type, VALTYP_STRING, &inbetween);
            unionNameCheckType(res, type, VALTYP_NULL, &inbetween);
            unionNameCheckType(res, type, VALTYP_FUNCNATIVE, &inbetween);
            unionNameCheckType(res, type, VALTYP_ARRAY, &inbetween);
            unionNameCheckType(res, type, VALTYP_MAP, &inbetween);
            unionNameCheckType(res, type, VALTYP_FUNCSCRIPT, &inbetween);
            unionNameCheckType(res, type, VALTYP_EXTERNAL, &inbetween);
            unionNameCheckType(res, type, VALTYP_ERROR, &inbetween);
            return res->getStringAndDestroy(nullptr);
        }

        static inline bool compare(Value a, Value b, CompareResult* cres)
        {
            bool isnumlike;
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
            Type atype;
            Type btype;
            /*
            if(a.odata == b.odata)
            {
                return 0;
            }
            */
            cres->result = 1;
            atype = a.getType();
            btype = b.getType();
            isnumlike = (
                (atype == VALTYP_NUMBER || atype == VALTYP_BOOL || atype == VALTYP_NULL) &&
                (btype == VALTYP_NUMBER || btype == VALTYP_BOOL || btype == VALTYP_NULL)
            );
            if(isnumlike)
            {
                dnleft = asNumber(a);
                dnright = asNumber(b);
                cres->result = (dnleft - dnright);
                return true;
            }
            if(atype == btype && atype == VALTYP_STRING)
            {
                alen = mc_value_stringgetlength(a);
                blen = mc_value_stringgetlength(b);
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
            Type atype;
            Type btype;
            (void)ok;
            atype = a.getType();
            btype = b.getType();
            if(atype != btype)
            {
                return false;
            }
            ok = compare(a, b, &cres);
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
            Type type;
            obj = *objptr;
            type = obj.getType();
            switch(type)
            {
                case VALTYP_NUMBER:
                    {
                        dval = asNumber(obj);
                        return mc_util_hashdouble(dval);
                    }
                    break;
                case VALTYP_BOOL:
                    {
                        bval = asBool(obj);
                        return static_cast<size_t>(bval);
                    }
                    break;
                case VALTYP_STRING:
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

        /*
        * copyDeep* and copyFlat have rather confusing names. but i can't think of better ones...
        */
        template<typename TypeKeyT, typename TypeValueT>
        static Value copyDeepIntern(State* state, Value obj, ValDict<TypeKeyT, TypeValueT>* targetdict)
        {
            Type type;
            Value copy;
            Value* copyptr;
            copyptr = targetdict->get(&obj);
            if(copyptr)
            {
                return *copyptr;
            }
            copy = makeNull();
            type = obj.getType();
            switch(type)
            {
                case VALTYP_FREED:
                case VALTYP_ANY:
                case VALTYP_NONE:
                    {
                        MC_ASSERT(false);
                        copy = makeNull();
                    }
                    break;
                case VALTYP_NUMBER:
                case VALTYP_BOOL:
                case VALTYP_NULL:
                case VALTYP_FUNCNATIVE:
                    {
                        copy = obj;
                    }
                    break;
                case VALTYP_STRING:
                    {
                        bool ok;
                        int len;
                        const char* str;
                        (void)ok;
                        str = mc_value_stringgetdata(obj);
                        len = mc_value_stringgetlength(obj);
                        copy = mc_value_makestringlen(state, str, len);
                        ok = targetdict->setKV(&obj, &copy);
                        return copy;
                    }
                    break;
                case VALTYP_FUNCSCRIPT:
                    {
                        return mc_value_copydeepfuncscript(state, obj, targetdict);
                    }
                    break;
                case VALTYP_ARRAY:
                    {
                        return mc_value_copydeeparray(state, obj, targetdict);
                    }
                    break;
                case VALTYP_MAP:
                    {
                        return mc_value_copydeepmap(state, obj, targetdict);
                    }
                    break;
                case VALTYP_EXTERNAL:
                    {
                        copy = copyFlat(state, obj);
                    }
                    break;
                case VALTYP_ERROR:
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
            auto targetdict = Memory::make<ValDict<Value, Value>>();
            res = copyDeepIntern(state, obj, targetdict);
            Memory::destroy(targetdict);
            return res;
        }

        static Value copyFlat(State* state, Value obj)
        {
            bool ok;
            Value copy;
            Type type;
            (void)ok;
            copy = makeNull();
            type = obj.getType();
            switch(type)
            {
                case VALTYP_ANY:
                case VALTYP_FREED:
                case VALTYP_NONE:
                    {
                        MC_ASSERT(false);
                        copy = makeNull();
                    }
                    break;
                case VALTYP_NUMBER:
                case VALTYP_BOOL:
                case VALTYP_NULL:
                case VALTYP_FUNCSCRIPT:
                case VALTYP_FUNCNATIVE:
                case VALTYP_ERROR:
                    {
                        copy = obj;
                    }
                    break;
                case VALTYP_STRING:
                    {
                        size_t len;
                        const char* str;
                        str = mc_value_stringgetdata(obj);
                        len = mc_value_stringgetlength(obj);
                        copy = mc_value_makestringlen(state, str, len);
                    }
                    break;
                case VALTYP_ARRAY:
                    {
                        int i;
                        int len;
                        Value item;
                        len = mc_value_arraygetlength(obj);
                        copy = mc_value_makearraycapacity(state, len);
                        if(copy.isNull())
                        {
                            return makeNull();
                        }
                        for(i = 0; i < len; i++)
                        {
                            item = mc_value_arraygetvalue(obj, i);
                            ok = mc_value_arraypush(copy, item);
                            if(!ok)
                            {
                                return makeNull();
                            }
                        }
                    }
                    break;
                case VALTYP_MAP:
                    {
                        int i;
                        Value key;
                        Value val;
                        copy = mc_value_makemap(state);
                        for(i = 0; i < mc_state_mapgetlength(obj); i++)
                        {
                            key = mc_state_mapgetkeyat(obj, i);
                            val = mc_state_mapgetvalueat(obj, i);
                            ok = mc_state_mapsetvalue(copy, key, val);
                            if(!ok)
                            {
                                return makeNull();
                            }
                        }
                    }
                    break;
                case VALTYP_EXTERNAL:
                    {
                        void* datacopy;
                        Object::ObjUserdata* objext;
                        copy = mc_value_makeuserobject(state, nullptr);
                        if(copy.isNull())
                        {
                            return makeNull();
                        }
                        objext = userdataGetData(obj);
                        datacopy = nullptr;
                        if(objext->datacopyfn != nullptr)
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

        static inline Value makeDataFrom(Type type, Object* data)
        {
            Value object;
            memset(&object, 0, sizeof(Value));
            object.m_valtype = type;
            data->m_odtype = type;
            object.m_isallocated = true;
            object.m_uval.odata = data;
            return object;
        }

        static inline Value makeEmpty(Type t)
        {
            Value o = {};
            o.m_valtype = t;
            o.m_isallocated = false;
            return o;
        }

        static inline Value makeNumber(mcfloat_t val)
        {
            Value o;
            o = makeEmpty(VALTYP_NUMBER);
            o.m_uval.valnumber = val;
            return o;
        }

        static inline Value makeBool(bool val)
        {
            Value o;
            o = makeEmpty(VALTYP_BOOL);
            o.m_uval.valbool = static_cast<int>(val);
            return o;
        }

        static inline Value makeNull()
        {
            Value o;
            o = makeEmpty(VALTYP_NULL);
            return o;
        }


        static inline bool asBool(Value obj)
        {
            if(obj.isNumber())
            {
                return obj.m_uval.valnumber != 0.0;
            }
            return obj.m_uval.valbool != 0;
        }

        static inline mcfloat_t asNumber(Value obj)
        {
            if(obj.isNumber())
            {
                if(obj.getType() == VALTYP_BOOL)
                {
                    return obj.m_uval.valbool;
                }
                return obj.m_uval.valnumber;
            }
            return obj.m_uval.valnumber;
        }

        static inline Object::ObjFunction* asFunction(Value object)
        {
            Object* data;
            data = object.getAllocatedData();
            return &data->m_uvobj.valfunc;
        }

        static Object::ObjUserdata* userdataGetData(Value object)
        {
            Object* data;
            MC_ASSERT(object.getType() == VALTYP_EXTERNAL);
            data = object.getAllocatedData();
            return &data->m_uvobj.valuserobject;
        }

    public:
        inline Type getType() const
        {
            return m_valtype;
        }

        Object* getAllocatedData() const
        {
            return m_uval.odata;
        }

        inline bool isAllocated() const
        {
            return m_isallocated;
        }

        inline bool isNumeric() const
        {
            Type type;
            type = getType();
            return type == VALTYP_NUMBER || type == VALTYP_BOOL;
        }

        inline bool isNumber() const
        {
            return (getType() == VALTYP_NUMBER || getType() == VALTYP_BOOL);
        }

        inline bool isNull() const
        {
            return getType() == VALTYP_NULL;
        }

        inline bool isFuncNative() const
        {
            Type type;
            type = getType();
            return (type == VALTYP_FUNCNATIVE);
        }

        inline bool isFuncScript() const
        {
            Type type;
            type = getType();
            return (type == VALTYP_FUNCSCRIPT);
        }

        inline bool isCallable() const
        {
            return (isFuncNative() || isFuncScript());
        }

        inline bool isString() const
        {
            Type type;
            type = getType();
            return (type == VALTYP_STRING);
        }

        inline bool isMap() const
        {
            Type type;
            type = getType();
            return (type == VALTYP_MAP);
        }

        inline bool isArray() const
        {
            Type type;
            type = getType();
            return (type == VALTYP_ARRAY);
        }
};

/*
* they absolutely MUST be identical in size.
* IF you need to add a field to Value, add it in ValData, since Value inherits ValData.
*/
static_assert(sizeof(ValData) == sizeof(Value));

class GCMemory
{
    public:
        enum
        {
            MemPoolSize = (16),
            SweepInterval = (128),

        };

        struct DataPool
        {
            public:
                GenericList<Object*> m_pooldata;
                int m_poolitemcount;
        };

    public:
        static void destroyPool(DataPool* pool)
        {
            size_t j;
            Object* data;
            for(j = 0; j < (size_t)pool->m_poolitemcount; j++)
            {
                data = pool->m_pooldata.get(j);
                mc_objectdata_deinit(data);
                mc_memory_free(data);
            }
            pool->m_pooldata.deInit();
        }

        static void destroy(GCMemory* m)
        {
            size_t i;
            Object* obj;
            if(m != nullptr)
            {
                Memory::destroy(m->m_gcobjlistremains);
                Memory::destroy(m->m_gcobjlistback);
                for(i = 0; i < m->m_gcobjliststored->count(); i++)
                {
                    obj = m->m_gcobjliststored->get(i);
                    mc_objectdata_deinit(obj);
                    mc_memory_free(obj);
                }
                Memory::destroy(m->m_gcobjliststored);
                destroyPool(&m->m_poolarray);
                destroyPool(&m->m_poolmap);
                destroyPool(&m->m_poolstring);
                destroyPool(&m->m_poolscriptfuncs);

                for(i = 0; i < (size_t)m->m_poolonlydata.m_poolitemcount; i++)
                {
                    auto p = m->m_poolonlydata.m_pooldata.get(i);
                    if(p != nullptr)
                    {
                        mc_memory_free(p);
                        m->m_poolonlydata.m_pooldata.set(i, nullptr);
                    }
                }
                m->m_poolonlydata.m_pooldata.deInit();
                mc_memory_free(m);
            }
        }

    public:
        int m_allocssincesweep;
        GenericList<Object*>* m_gcobjliststored;
        GenericList<Object*>* m_gcobjlistback;
        GenericList<Value>* m_gcobjlistremains;
        DataPool m_poolonlydata;
        DataPool m_poolarray;
        DataPool m_poolmap;
        DataPool m_poolstring;
        DataPool m_poolscriptfuncs;

    private:
        void initPool(DataPool* pool)
        {
            #if 0 
            pool->m_pooldata = Memory::make<GenericList<Object*>>(MemPoolSize, nullptr);
            #endif
            pool->m_poolitemcount = 0;
        }

    public:
        GCMemory()
        {
            m_gcobjliststored = Memory::make<GenericList<Object*>>(0, nullptr);
            m_gcobjlistback = Memory::make<GenericList<Object*>>(0, nullptr);
            m_gcobjlistremains = Memory::make<GenericList<Value>>(0, Value::makeNull());
            m_allocssincesweep = 0;
            initPool(&m_poolonlydata);
            initPool(&m_poolarray);
            initPool(&m_poolmap);
            initPool(&m_poolstring);
            initPool(&m_poolscriptfuncs);
        }

        template<typename ValTypT>
        inline DataPool* getPoolForType(ValTypT type)
        {
            switch(type)
            {
                case Value::VALTYP_FUNCSCRIPT:
                    return &m_poolscriptfuncs;
                case Value::VALTYP_ARRAY:
                    return &m_poolarray;
                case Value::VALTYP_MAP:
                    return &m_poolmap;
                case Value::VALTYP_STRING:
                    return &m_poolstring;
                default:
                    break;
            }
            return nullptr;
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
            cl->m_memberfields.deInit();
            mc_memory_free(cl);
        }

    public:
        const char* m_classname;
        ObjClass* m_parentclass;
        Value m_constructor;
        GenericList<Field> m_memberfields = GenericList<Field>(0, Field{});

    public:
        ObjClass(const char* name, ObjClass* parclass)
        {
            m_parentclass = parclass;
            m_classname = name;
            m_constructor = Value::makeNull();
        }

        void addFunction(const char* name, bool ispseudo, mcnativefn_t fn)
        {
            Field bt;
            bt.name = name;
            bt.ispseudo = ispseudo;
            bt.fndest = fn;
            m_memberfields.push(bt);
        }

        void addMember(const char* name, mcnativefn_t fn)
        {
            return addFunction(name, false, fn);
        }

        void addPseudo(const char* name, mcnativefn_t fn)
        {
            return addFunction(name, true, fn);
        }

};

class AstSymbol
{
    public:
        enum Type
        {
            SYMTYP_NONE = 0,
            SYMTYP_MODULEGLOBAL,
            SYMTYP_LOCAL,
            SYMTYP_GLOBALBUILTIN,
            SYMTYP_FREE,
            SYMTYP_FUNCTION,
            SYMTYP_THIS
        };

    public:
        static void destroy(AstSymbol* symbol)
        {
            symbol->deInit();
            mc_memory_free(symbol);
        }

        static void destroyHeap(AstSymbol* symbol)
        {
            destroy(symbol);
        }

        static AstSymbol* copyHeap(AstSymbol* symbol)
        {
            return Memory::make<AstSymbol>(symbol->m_symname, symbol->m_symtype, symbol->m_symindex, symbol->m_symisassignable);
        }

        static AstSymbol copyStack(AstSymbol* symbol)
        {
            return AstSymbol(symbol->m_symname, symbol->m_symtype, symbol->m_symindex, symbol->m_symisassignable);
        }


    public:
        Type m_symtype;
        char* m_symname;
        int m_symindex;
        bool m_symisassignable;

    public:
        AstSymbol(const char* syname, Type sytype, int syindex, bool syassignable)
        {
            m_symname = mc_util_strdup(syname);
            MC_ASSERT(m_symname);
            m_symtype = sytype;
            m_symindex = syindex;
            m_symisassignable = syassignable;
        }

        void deInit()
        {
            mc_memory_free(m_symname);
            m_symname = nullptr;
        }
};

class SymStore
{
    public:
        static void destroy(SymStore* store)
        {
            if(store != nullptr)
            {
                StrDict::destroyItemsAndDict(store->m_storedsymbols);
                store->m_storedobjects.deInit();
                mc_memory_free(store);
                store = nullptr;
            }
        }

    public:
        StrDict* m_storedsymbols;
        GenericList<Value> m_storedobjects = GenericList<Value>(0, Value::makeNull());

    public:
        SymStore()
        {
            m_storedsymbols = Memory::make<StrDict>((mcitemcopyfn_t)AstSymbol::copyHeap, (mcitemdestroyfn_t)AstSymbol::destroy);
        }

        AstSymbol* getSymbol(const char* name)
        {
            return (AstSymbol*)m_storedsymbols->get(name);
        }

        bool setNamed(const char* name, Value object)
        {
            bool ok;
            int ix;
            AstSymbol* symbol;
            AstSymbol* existingsymbol;
            (void)ok;
            existingsymbol = getSymbol(name);
            if(existingsymbol != nullptr)
            {
                ok = (m_storedobjects.set(existingsymbol->m_symindex, object) != nullptr);
                return ok;
            }
            ix = m_storedobjects.count();
            ok = m_storedobjects.push(object);
            symbol = Memory::make<AstSymbol>(name, AstSymbol::SYMTYP_GLOBALBUILTIN, ix, false);
            ok = m_storedsymbols->set(name, symbol);
            return true;
        }

        Value getAtIndex(int ix, bool* outok)
        {
            Value* res;
            res = m_storedobjects.getp(ix);
            if(res == nullptr)
            {
                *outok = false;
                return Value::makeNull();
            }
            *outok = true;
            return *res;
        }

        Value* getData()
        {
            return m_storedobjects.data();
        }

        int getCount()
        {
            return m_storedobjects.count();
        }
};

class AstScopeBlock
{
    public:
        static void destroy(AstScopeBlock* scope)
        {
            StrDict::destroyItemsAndDict(scope->m_blscopestore);
            mc_memory_free(scope);
            scope = nullptr;
        }

        static AstScopeBlock* copy(AstScopeBlock* scope)
        {
            AstScopeBlock* copy;
            copy = Memory::make<AstScopeBlock>(scope->m_blscopeoffset, scope->m_blscopestore->copy());
            copy->m_blscopenumdefs = scope->m_blscopenumdefs;
            return copy;
        }

   public:
        StrDict* m_blscopestore;
        int m_blscopeoffset;
        int m_blscopenumdefs;

    public:
        AstScopeBlock(int ofs): AstScopeBlock(ofs, nullptr)
        {
        }

        AstScopeBlock(int ofs, StrDict* ss)
        {
            if(ss != nullptr)
            {
                m_blscopestore = ss;
            }
            else
            {
                m_blscopestore = Memory::make<StrDict>((mcitemcopyfn_t)AstSymbol::copyHeap, (mcitemdestroyfn_t)AstSymbol::destroy);
            }
            m_blscopenumdefs = 0;
            m_blscopeoffset = ofs;
        }
};

class AstSymTable
{
    public:
        static void destroy(AstSymTable* table)
        {
            if(table != nullptr)
            {
                while(table->m_symtbblockscopes.count() > 0)
                {
                    table->scopeBlockPop();
                }
                table->m_symtbblockscopes.deInit();
                table->m_symtbmodglobalsymbols.deInit(AstSymbol::destroy);
                Memory::destroy(table->m_symtbfreesymbols, AstSymbol::destroy);
                mc_memory_free(table);
                table = nullptr;
            }
        }

    public:
        AstSymTable* m_symtbouter;
        SymStore* m_symtbglobalstore;
        GenericList<AstScopeBlock*> m_symtbblockscopes;
        GenericList<AstSymbol*>* m_symtbfreesymbols;
        GenericList<AstSymbol*> m_symtbmodglobalsymbols;
        int m_symtbmaxnumdefinitions;
        int m_symtbmodglobaloffset;

    public:
        AstSymTable(AstSymTable* syouter, SymStore* sygstore, GenericList<AstScopeBlock*>* syblockscopes, GenericList<AstSymbol*>* syfreesyms, GenericList<AstSymbol*>* symodglobalsymbols, int mgo)
        {
            m_symtbmaxnumdefinitions = 0;
            m_symtbouter = syouter;
            m_symtbglobalstore = sygstore;
            m_symtbmodglobaloffset = mgo;
            if(syblockscopes != nullptr)
            {
                m_symtbblockscopes = *syblockscopes;
            }
            if(syfreesyms != nullptr)
            {
                m_symtbfreesymbols = syfreesyms;
            }
            else
            {
                m_symtbfreesymbols = Memory::make<GenericList<AstSymbol*>>(0, nullptr);
            }
            if(symodglobalsymbols != nullptr)
            {
                m_symtbmodglobalsymbols = *symodglobalsymbols;
            }
            MC_ASSERT(scopeBlockPush());
        }

        AstSymTable* copy()
        {
            AstSymTable* copy;
            auto cblocks = m_symtbblockscopes.copyToStack(AstScopeBlock::copy, AstScopeBlock::destroy);
            auto cfrees = m_symtbfreesymbols->copyToHeap(AstSymbol::copyHeap, AstSymbol::destroyHeap);
            auto cmods = m_symtbmodglobalsymbols.copyToStack(AstSymbol::copyHeap, AstSymbol::destroyHeap);
            copy = Memory::make<AstSymTable>(m_symtbouter, m_symtbglobalstore, &cblocks, cfrees, &cmods, m_symtbmodglobaloffset);
            copy->m_symtbmaxnumdefinitions = m_symtbmaxnumdefinitions;
            copy->m_symtbmodglobaloffset = m_symtbmodglobaloffset;
            return copy;
        }

        bool setSymbol(AstSymbol* symbol)
        {
            AstScopeBlock* topscope;
            AstSymbol* existing;
            topscope = m_symtbblockscopes.top();
            existing = (AstSymbol*)topscope->m_blscopestore->get(symbol->m_symname);
            if(existing != nullptr)
            {
                AstSymbol::destroy(existing);
            }
            return topscope->m_blscopestore->set(symbol->m_symname, symbol);
        }

        int nextSymbolIndex()
        {
            int ix;
            AstScopeBlock* topscope;
            topscope = m_symtbblockscopes.top();
            ix = topscope->m_blscopeoffset + topscope->m_blscopenumdefs;
            return ix;
        }

        int getNumDefinitions()
        {
            int i;
            int count;
            AstScopeBlock* scope;
            count = 0;
            for(i = m_symtbblockscopes.count() - 1; i >= 0; i--)
            {
                scope = m_symtbblockscopes.get(i);
                count += scope->m_blscopenumdefs;
            }
            return count;
        }


        bool addModuleSymbol(AstSymbol* symbol)
        {
            bool ok;
            AstSymbol* copy;
            (void)ok;
            if(symbol->m_symtype != AstSymbol::SYMTYP_MODULEGLOBAL)
            {
                MC_ASSERT(false);
                return false;
            }
            if(isDefined(symbol->m_symname))
            {
                /* todo: make sure it should be true in this case */
                return true;
            }
            copy = AstSymbol::copyHeap(symbol);
            if(copy == nullptr)
            {
                return false;
            }
            ok = setSymbol(copy);
            return true;
        }

        AstSymbol* defineSymbol(const char* name, bool assignable)
        {
            bool ok;
            bool globalsymboladded;
            int ix;
            int definitionscount;
            AstSymbol::Type symboltype;
            AstSymbol* symbol;
            AstScopeBlock* topscope;
            AstSymbol* globalsymbol;
            AstSymbol* globalsymbolcopy;
            (void)ok;
            (void)globalsymboladded;
            globalsymbol = m_symtbglobalstore->getSymbol(name);
            if(globalsymbol != nullptr)
            {
                return nullptr;
            }
            /* module symbol */
            if(strchr(name, ':') != nullptr)
            {
                return nullptr;
            }
            /* "this" is reserved */
            if(mc_util_strequal(name, "this"))
            {
                return defineThis();
            }
            symboltype = m_symtbouter == nullptr ? AstSymbol::SYMTYP_MODULEGLOBAL : AstSymbol::SYMTYP_LOCAL;
            ix = nextSymbolIndex();
            symbol = Memory::make<AstSymbol>(name, symboltype, ix, assignable);
            globalsymboladded = false;
            ok = false;
            if(symboltype == AstSymbol::SYMTYP_MODULEGLOBAL && m_symtbblockscopes.count() == 1)
            {
                globalsymbolcopy = AstSymbol::copyHeap(symbol);
                if(globalsymbolcopy == nullptr)
                {
                    AstSymbol::destroy(symbol);
                    return nullptr;
                }
                ok = m_symtbmodglobalsymbols.push(globalsymbolcopy);
                globalsymboladded = true;
            }
            ok = setSymbol(symbol);
            topscope = m_symtbblockscopes.top();
            topscope->m_blscopenumdefs++;
            definitionscount = getNumDefinitions();
            if(definitionscount > m_symtbmaxnumdefinitions)
            {
                m_symtbmaxnumdefinitions = definitionscount;
            }
            return symbol;
        }

        AstSymbol* defineAndDestroyOld(AstSymbol* original)
        {
            bool ok;
            AstSymbol* copy;
            AstSymbol* symbol;
            (void)ok;
            copy = Memory::make<AstSymbol>(original->m_symname, original->m_symtype, original->m_symindex, original->m_symisassignable);
            ok = m_symtbfreesymbols->push(copy);
            symbol = Memory::make<AstSymbol>(original->m_symname, AstSymbol::SYMTYP_FREE, m_symtbfreesymbols->count() - 1, original->m_symisassignable);
            ok = setSymbol(symbol);
            return symbol;
        }

        AstSymbol* defineFunctionName(const char* name, bool assignable)
        {
            bool ok;
            AstSymbol* symbol;
            (void)ok;
            /* module symbol */
            if(strchr(name, ':') != nullptr)
            {
                return nullptr;
            }
            symbol = Memory::make<AstSymbol>(name, AstSymbol::SYMTYP_FUNCTION, 0, assignable);
            ok = setSymbol(symbol);
            return symbol;
        }

        AstSymbol* defineThis()
        {
            bool ok;
            AstSymbol* symbol;
            (void)ok;
            symbol = Memory::make<AstSymbol>("this", AstSymbol::SYMTYP_THIS, 0, false);
            ok = setSymbol(symbol);
            return symbol;
        }

        AstSymbol* resolve(const char* name)
        {
            int i;
            AstSymbol* symbol;
            AstScopeBlock* scope;
            symbol = nullptr;
            scope = nullptr;
            symbol = m_symtbglobalstore->getSymbol(name);
            if(symbol != nullptr)
            {
                return symbol;
            }

            for(i = m_symtbblockscopes.count() - 1; i >= 0; i--)
            {
                scope = m_symtbblockscopes.get(i);
                symbol = (AstSymbol*)scope->m_blscopestore->get(name);
                if(symbol != nullptr)
                {
                    break;
                }
            }
            if((symbol != nullptr) && symbol->m_symtype == AstSymbol::SYMTYP_THIS)
            {
                symbol = defineAndDestroyOld(symbol);
            }
            if((symbol == nullptr) && (m_symtbouter != nullptr))
            {
                symbol = m_symtbouter->resolve(name);
                if(symbol == nullptr)
                {
                    return nullptr;
                }
                if(symbol->m_symtype == AstSymbol::SYMTYP_MODULEGLOBAL || symbol->m_symtype == AstSymbol::SYMTYP_GLOBALBUILTIN)
                {
                    return symbol;
                }
                symbol = defineAndDestroyOld(symbol);
            }
            return symbol;
        }

        bool isDefined(const char* name)
        {
            /* todo: rename to something more obvious */
            AstSymbol* symbol;
            AstScopeBlock* topscope;
            symbol = m_symtbglobalstore->getSymbol(name);
            if(symbol != nullptr)
            {
                return true;
            }
            topscope = m_symtbblockscopes.top();
            symbol = (AstSymbol*)topscope->m_blscopestore->get(name);
            return symbol != nullptr;
        }

        bool scopeBlockPush()
        {
            bool ok;
            int blockscopeoffset;
            AstScopeBlock* newscope;
            AstScopeBlock* prevblockscope;
            (void)ok;
            blockscopeoffset = 0;
            prevblockscope = m_symtbblockscopes.top();
            if(prevblockscope != nullptr)
            {
                blockscopeoffset = m_symtbmodglobaloffset + prevblockscope->m_blscopeoffset + prevblockscope->m_blscopenumdefs;
            }
            else
            {
                blockscopeoffset = m_symtbmodglobaloffset;
            }
            newscope = Memory::make<AstScopeBlock>(blockscopeoffset);
            ok = m_symtbblockscopes.push(newscope);
            return true;
        }

        void scopeBlockPop()
        {
            AstScopeBlock* topscope;
            topscope = m_symtbblockscopes.top();
            m_symtbblockscopes.pop(nullptr);
            Memory::destroy(topscope);
        }

        AstScopeBlock* scopeBlockGet()
        {
            AstScopeBlock* topscope;
            topscope = m_symtbblockscopes.top();
            return topscope;
        }

        bool isModuleGlobalScope()
        {
            return m_symtbouter == nullptr;
        }

        bool isTopBlockScope()
        {
            return m_symtbblockscopes.count() == 1;
        }

        bool isTopGlobalScope()
        {
            return isModuleGlobalScope() && isTopBlockScope();
        }

        size_t getModuleGlobalSymCount()
        {
            return m_symtbmodglobalsymbols.count();
        }

        AstSymbol* getModuleGlobalSymAt(int ix)
        {
            return m_symtbmodglobalsymbols.get(ix);
        }
};

class AstLocation
{
    public:
        static AstLocation Invalid()
        {        
            return AstLocation();
        }

    public:
        AstSourceFile* m_locfile;
        int m_locline;
        int m_loccolumn;

    public:
        AstLocation(): AstLocation(nullptr, 0, 0)
        {
        }

        AstLocation(AstSourceFile* fi, int nlin, int coln)
        {
            m_locfile = fi;
            m_locline = nlin;
            m_loccolumn = coln;
        }

};

class AstScopeComp
{
    public:
        static void destroy(AstScopeComp* scope)
        {
            scope->m_scopeipstackcontinue.deInit();
            scope->m_scopeipstackbreak.deInit();
            scope->m_scopecompiledbc.deInit();
            scope->m_scopesrcposlist.deInit();
            mc_memory_free(scope);
        }

    public:
        AstScopeComp* m_outerscope;
        GenericList<uint16_t> m_scopecompiledbc = GenericList<uint16_t>(0, 0);
        GenericList<AstLocation> m_scopesrcposlist = GenericList<AstLocation>(0, AstLocation::Invalid());
        GenericList<int> m_scopeipstackbreak = GenericList<int>(0, 0);
        GenericList<int> m_scopeipstackcontinue = GenericList<int>(0, 0);
        mcinternopcode_t m_scopelastopcode = 0;

    public:
        AstScopeComp(AstScopeComp* ou)
        {
            m_outerscope = ou;
        }

        CompiledProgram* orphanResult()
        {
            uint16_t* bcdata;
            AstLocation* astlocdata;
            CompiledProgram* res;
            bcdata = m_scopecompiledbc.data();
            astlocdata = m_scopesrcposlist.data();
            res = Memory::make<CompiledProgram>(bcdata, astlocdata, m_scopecompiledbc.count());
            m_scopecompiledbc.orphanData();
            m_scopesrcposlist.orphanData();
            return res;
        }
};

class AstScopeFile
{
    public:
        static void destroy(AstScopeFile* scope)
        {
            scope->deInit();
            mc_memory_free(scope);
        }

    public:
        AstParser* m_filescopeparser = nullptr;
        AstSymTable* m_scopefilesymtab = nullptr;
        AstSourceFile* m_filescopesourcefile = nullptr;
        GenericList<char*> m_filescopeloadednames;

    public:
        AstScopeFile()
        {
        }

        AstScopeFile(RuntimeConfig* cfg, ErrList* errlist, AstSourceFile* file)
        {
            m_filescopeparser = Memory::make<AstParser>(cfg, errlist);
            m_scopefilesymtab = nullptr;
            m_filescopesourcefile = file;
        }

        void deInit()
        {
            size_t i;
            for(i = 0; i < m_filescopeloadednames.count(); i++)
            {
                auto name = m_filescopeloadednames.get(i);
                mc_memory_free(name);
            }
            m_filescopeloadednames.deInit();
            Memory::destroy(m_filescopeparser);
        }
};

class AstToken
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
                case AstToken::TOK_EOF:
                    return "EOF";
                case AstToken::TOK_ASSIGN:
                    return "=";
                case AstToken::TOK_ASSIGNPLUS:
                    return "+=";
                case AstToken::TOK_ASSIGNMINUS:
                    return "-=";
                case AstToken::TOK_ASSIGNASTERISK:
                    return "*=";
                case AstToken::TOK_ASSIGNSLASH:
                    return "/=";
                case AstToken::TOK_ASSIGNPERCENT:
                    return "%=";
                case AstToken::TOK_ASSIGNBINAND:
                    return "&=";
                case AstToken::TOK_ASSIGNBINOR:
                    return "|=";
                case AstToken::TOK_ASSIGNBINXOR:
                    return "^=";
                case AstToken::TOK_ASSIGNLSHIFT:
                    return "<<=";
                case AstToken::TOK_ASSIGNRSHIFT:
                    return ">>=";
                case AstToken::TOK_QUESTION:
                    return "?";
                case AstToken::TOK_PLUS:
                    return "+";
                case AstToken::TOK_PLUSPLUS:
                    return "++";
                case AstToken::TOK_UNARYMINUS:
                    return "-";
                case AstToken::TOK_MINUSMINUS:
                    return "--";
                case AstToken::TOK_BANG:
                    return "!";
                case AstToken::TOK_ASTERISK:
                    return "*";
                case AstToken::TOK_SLASH:
                    return "/";
                case AstToken::TOK_LT:
                    return "<";
                case AstToken::TOK_LTE:
                    return "<=";
                case AstToken::TOK_GT:
                    return ">";
                case AstToken::TOK_GTE:
                    return ">=";
                case AstToken::TOK_EQ:
                    return "==";
                case AstToken::TOK_NOTEQ:
                    return "!=";
                case AstToken::TOK_AND:
                    return "&&";
                case AstToken::TOK_OR:
                    return "||";
                case AstToken::TOK_BINAND:
                    return "&";
                case AstToken::TOK_BINOR:
                    return "|";
                case AstToken::TOK_BINXOR:
                    return "^";
                case AstToken::TOK_LSHIFT:
                    return "<<";
                case AstToken::TOK_RSHIFT:
                    return ">>";
                case AstToken::TOK_COMMA:
                    return ",";
                case AstToken::TOK_SEMICOLON:
                    return ";";
                case AstToken::TOK_COLON:
                    return ":";
                case AstToken::TOK_LPAREN:
                    return "(";
                case AstToken::TOK_RPAREN:
                    return ")";
                case AstToken::TOK_LBRACE:
                    return "{";
                case AstToken::TOK_RBRACE:
                    return "}";
                case AstToken::TOK_LBRACKET:
                    return "[";
                case AstToken::TOK_RBRACKET:
                    return "]";
                case AstToken::TOK_DOT:
                    return ".";
                case AstToken::TOK_PERCENT:
                    return "%";
                case AstToken::TOK_FUNCTION:
                    return "FUNCTION";
                case AstToken::TOK_CONST:
                    return "CONST";
                case AstToken::TOK_VAR:
                    return "VAR";
                case AstToken::TOK_TRUE:
                    return "TRUE";
                case AstToken::TOK_FALSE:
                    return "FALSE";
                case AstToken::TOK_IF:
                    return "IF";
                case AstToken::TOK_ELSE:
                    return "ELSE";
                case AstToken::TOK_RETURN:
                    return "RETURN";
                case AstToken::TOK_WHILE:
                    return "WHILE";
                case AstToken::TOK_BREAK:
                    return "BREAK";
                case AstToken::TOK_FOR:
                    return "FOR";
                case AstToken::TOK_IN:
                    return "IN";
                case AstToken::TOK_CONTINUE:
                    return "CONTINUE";
                case AstToken::TOK_NULL:
                    return "nullptr";
                case AstToken::TOK_IMPORT:
                    return "IMPORT";
                case AstToken::TOK_RECOVER:
                    return "RECOVER";
                case AstToken::TOK_IDENT:
                    return "IDENT";
                case AstToken::TOK_NUMBER:
                    return "NUMBER";
                case AstToken::TOK_STRING:
                    return "STRING";
                case AstToken::TOK_TEMPLATESTRING:
                    return "TEMPLATE_STRING";
                default:
                    break;
            }
            return "ILLEGAL";
        }
    public:
        Type m_toktype;
        const char* m_tokstrdata;
        int m_tokstrlength;
        AstLocation m_tokpos;

    public:
        AstToken(): AstToken(AstToken::TOK_EOF, "", 0)
        {
        }

        AstToken(Type type, const char* literal, int len)
        {
            m_toktype = type;
            m_tokstrdata = literal;
            m_tokstrlength = len;
        }

        char* dupLiteralString()
        {
            return mc_util_strndup(m_tokstrdata, m_tokstrlength);
        }

        inline Type type() const
        {
            return m_toktype;
        }
};

class AstExprData
{
    public:
        enum ExprType
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

        enum MathOpType
        {
            MATHOP_NONE,
            MATHOP_ASSIGN,
            MATHOP_PLUS,
            MATHOP_MINUS,
            MATHOP_BINNOT,
            MATHOP_BANG,
            MATHOP_ASTERISK,
            MATHOP_SLASH,
            MATHOP_LT,
            MATHOP_LTE,
            MATHOP_GT,
            MATHOP_GTE,
            MATHOP_EQ,
            MATHOP_NOTEQ,
            MATHOP_MODULUS,
            MATHOP_LOGICALAND,
            MATHOP_LOGICALOR,
            MATHOP_BINAND,
            MATHOP_BINOR,
            MATHOP_BINXOR,
            MATHOP_LSHIFT,
            MATHOP_RSHIFT
        };

        class ExprShim
        {
            public:
                ExprShim* operator->()
                {
                    return this;
                }
                
        };

        class ExprCodeBlock: public ExprShim
        {
            public:
                GenericList<AstExpression*> m_blockstatements;

            public:
                ExprCodeBlock()
                {
                }

                ExprCodeBlock(const GenericList<AstExpression*>& stmts)
                {
                    m_blockstatements = stmts;
                }

                ~ExprCodeBlock()
                {
                }

                static ExprCodeBlock copy(ExprCodeBlock* block)
                {
                    auto copyfn = AstExprData::copyExprData<AstExpression>;
                    auto delfn = AstExprData::destroyExprData<AstExpression>;
                    auto statementscopy = block->m_blockstatements.copyToStack(copyfn, delfn);
                    return ExprCodeBlock(statementscopy);
                }

                void deInit()
                {
                    m_blockstatements.deInit(destroyExprData<AstExpression>);
                }
        };

        class ExprIfCase: public ExprShim
        {
            public:
                AstExpression* m_ifcond;
                ExprCodeBlock m_ifthen;

            public:
                ExprIfCase(AstExpression* test, const ExprCodeBlock& consq)
                {
                    m_ifcond = test;
                    m_ifthen = consq;
                }

                static ExprIfCase* copy(ExprIfCase* ifcase)
                {
                    auto testcopy = copyExprData(ifcase->m_ifcond);
                    auto consequencecopy = ExprCodeBlock::copy(&ifcase->m_ifthen);
                    auto ifcasecopy = Memory::make<ExprIfCase>(testcopy, consequencecopy);
                    return ifcasecopy;
                }

                static void destroy(ExprIfCase* cond)
                {
                    if(cond != nullptr)
                    {
                        destroyExprData(cond->m_ifcond);
                        cond->m_ifthen.deInit();
                        mc_memory_free(cond);
                    }
                }
        };

        struct ExprIfStmt: public ExprShim
        {
            bool m_haveifstmtelsestmt;
            GenericList<ExprIfCase*> m_ifcases;
            ExprCodeBlock m_ifstmtelsestmt;
        };

        struct ExprLiteralMap: public ExprShim
        {
            GenericList<AstExpression*> m_litmapkeys;
            GenericList<AstExpression*> m_litmapvalues;
        };

        struct ExprLiteralArray: public ExprShim
        {
            GenericList<AstExpression*> m_litarritems;
        };

        struct ExprLiteralString: public ExprShim
        {
            size_t m_strexprlength;
            char* m_strexprdata;
        };

        struct ExprPrefix: public ExprShim
        {
            MathOpType op;
            AstExpression* right;
        };

        struct ExprInfix: public ExprShim
        {
            MathOpType op;
            AstExpression* left;
            AstExpression* right;
        };

        class ExprIdent: public ExprShim
        {
            public:
                char* m_identvalue = nullptr;
                AstLocation m_exprpos = AstLocation::Invalid();

            public:
                ExprIdent()
                {
                }

                ExprIdent(AstToken tok)
                {
                    m_identvalue = tok.dupLiteralString();
                    m_exprpos = tok.m_tokpos;
                }

                static ExprIdent copy(ExprIdent* ident)
                {
                    ExprIdent res = ExprIdent();
                    res.m_identvalue = mc_util_strdup(ident->m_identvalue);
                    res.m_exprpos = ident->m_exprpos;
                    return res;
                }

                void deInit()
                {
                    {
                        mc_memory_free(m_identvalue);
                        m_identvalue = nullptr;
                        m_exprpos = AstLocation::Invalid();
                    }
                }
        };

        class ExprFuncParam: public ExprShim
        {
            public:
                ExprIdent m_paramident;

            public:
                ExprFuncParam(const ExprIdent& idv)
                {
                    m_paramident = idv;
                    MC_ASSERT(m_paramident.m_identvalue);
                }

                static ExprFuncParam* copy(ExprFuncParam* param)
                {
                    ExprFuncParam* res;
                    res = Memory::make<ExprFuncParam>(ExprIdent::copy(&param->m_paramident));
                    if(res->m_paramident.m_identvalue == nullptr)
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
                        param->m_paramident.deInit();
                        mc_memory_free(param);
                    }
                }
        };

        struct ExprLiteralFunction: public ExprShim
        {
            char* name;
            GenericList<ExprFuncParam*> funcparamlist;
            ExprCodeBlock body;
        };

        struct ExprCall: public ExprShim
        {
            AstExpression* function;
            GenericList<AstExpression*> m_callargs;
        };

        struct ExprIndex: public ExprShim
        {
            bool isdot;
            AstExpression* left;
            AstExpression* index;
        };

        struct ExprAssign: public ExprShim
        {
            AstExpression* dest;
            AstExpression* source;
            bool is_postfix;
        };

        struct ExprLogical: public ExprShim
        {
            MathOpType op;
            AstExpression* left;
            AstExpression* right;
        };

        struct ExprTernary: public ExprShim
        {
            AstExpression* tercond;
            AstExpression* teriftrue;
            AstExpression* teriffalse;
        };

        struct ExprDefine: public ExprShim
        {
            ExprIdent name;
            AstExpression* value;
            bool assignable;
        };

        struct ExprWhileStmt: public ExprShim
        {
            AstExpression* loopcond;
            ExprCodeBlock body;
        };

        struct ExprForeachStmt: public ExprShim
        {
            ExprIdent iterator;
            AstExpression* source;
            ExprCodeBlock body;
        };

        struct ExprLoopStmt: public ExprShim
        {
            AstExpression* init;
            AstExpression* loopcond;
            AstExpression* update;
            ExprCodeBlock body;
        };

        struct ExprImportStmt: public ExprShim
        {
            char* path;
        };

        struct ExprRecover: public ExprShim
        {
            ExprIdent errident;
            ExprCodeBlock body;
        };

        virtual void destroyInstanceExpression() = 0;
        virtual AstExpression* copyInstanceExpression() = 0;

        template<typename InTypeT>
        static void destroyExprData(InTypeT* expr)
        {
            if(expr != nullptr)
            {
                ((AstExprData*)expr)->destroyInstanceExpression();
            }
        }

        template<typename InTypeT>
        static AstExpression* copyExprData(InTypeT* expr)
        {
            if(expr != nullptr)
            {
                return ((AstExprData*)expr)->copyInstanceExpression();
            }
            return nullptr;
        }

        union DataUnion
        {
            DataUnion()
            {
            }

            ~DataUnion()
            {
            }

            bool exprlitbool;
            mcfloat_t exprlitnumber;
            ExprLiteralArray exprlitarray;
            ExprImportStmt exprimportstmt;
            ExprIdent exprident;
            ExprCodeBlock exprblockstmt;
            AstExpression* exprreturnvalue;
            AstExpression* exprexpression;
            ExprLoopStmt exprforloopstmt;
            ExprTernary exprternary;
            ExprLogical exprlogical;
            ExprLiteralFunction exprlitfunction;
            ExprInfix exprinfix;
            ExprIndex exprindex;
            ExprForeachStmt exprforeachloopstmt;
            ExprDefine exprdefine;
            ExprAssign exprassign;
            ExprWhileStmt exprwhileloopstmt;
            ExprRecover exprrecoverstmt;
            ExprPrefix exprprefix;
            ExprLiteralString exprlitstring;
            ExprLiteralMap exprlitmap;
            ExprIfStmt exprifstmt;
            ExprCall exprcall;
        };

    public:
        ExprType m_exprtype;
        AstLocation m_exprpos;
        DataUnion m_uexpr;

};

class AstExpression: public AstExprData
{
    public:

        static AstExpression* makeAstItemBaseExpression(ExprType type)
        {
            auto res = Memory::make<AstExpression>();
            res->m_exprtype = type;
            res->m_exprpos = AstLocation::Invalid();
            return res;
        }

        static AstExpression* makeAstItemRecover(const ExprIdent& eid, const ExprCodeBlock& body)
        {
            auto res = makeAstItemBaseExpression(EXPR_STMTRECOVER);
            res->m_uexpr.exprrecoverstmt.errident = eid;
            res->m_uexpr.exprrecoverstmt.body = body;
            return res;
        }

        static AstExpression* makeAstItemLiteralNumber(mcfloat_t val)
        {
            auto res = makeAstItemBaseExpression(EXPR_NUMBERLITERAL);
            res->m_uexpr.exprlitnumber = val;
            return res;
        }

        static AstExpression* makeAstItemLiteralBool(bool val)
        {
            auto res = makeAstItemBaseExpression(EXPR_BOOLLITERAL);
            res->m_uexpr.exprlitbool = val;
            return res;
        }

        static AstExpression* makeAstItemLiteralString(char* value, size_t len)
        {
            auto res = makeAstItemBaseExpression(EXPR_STRINGLITERAL);
            res->m_uexpr.exprlitstring.m_strexprdata = value;
            res->m_uexpr.exprlitstring.m_strexprlength = len;
            return res;
        }

        static AstExpression* makeAstItemLiteralNull()
        {
            auto res = makeAstItemBaseExpression(EXPR_NULLLITERAL);
            return res;
        }

        static AstExpression* makeAstItemIdent(const ExprIdent& ident)
        {
            auto res = makeAstItemBaseExpression(EXPR_IDENT);
            res->m_uexpr.exprident = ident;
            return res;
        }

        static AstExpression* makeAstItemLiteralArray(const GenericList<AstExpression*>& values)
        {
            auto res = makeAstItemBaseExpression(EXPR_ARRAYLITERAL);
            res->m_uexpr.exprlitarray.m_litarritems = values;
            return res;
        }

        static AstExpression* makeAstItemLiteralMap(const GenericList<AstExpression*>& keys, const GenericList<AstExpression*>& values)
        {
            auto res = makeAstItemBaseExpression(EXPR_MAPLITERAL);
            res->m_uexpr.exprlitmap.m_litmapkeys = keys;
            res->m_uexpr.exprlitmap.m_litmapvalues = values;
            return res;
        }

        static AstExpression* makeAstItemPrefix(MathOpType op, AstExpression* right)
        {
            auto res = makeAstItemBaseExpression(EXPR_PREFIX);
            res->m_uexpr.exprprefix.op = op;
            res->m_uexpr.exprprefix.right = right;
            return res;
        }

        static AstExpression* makeAstItemInfix(MathOpType op, AstExpression* left, AstExpression* right)
        {
            auto res = makeAstItemBaseExpression(EXPR_INFIX);
            res->m_uexpr.exprinfix.op = op;
            res->m_uexpr.exprinfix.left = left;
            res->m_uexpr.exprinfix.right = right;
            return res;
        }

        static AstExpression* makeAstItemLiteralFunction(const GenericList<AstExpression::ExprFuncParam*>& params, const ExprCodeBlock& body)
        {
            auto res = makeAstItemBaseExpression(EXPR_FUNCTIONLITERAL);
            res->m_uexpr.exprlitfunction.name = nullptr;
            res->m_uexpr.exprlitfunction.funcparamlist = params;
            res->m_uexpr.exprlitfunction.body = body;
            return res;
        }

        static AstExpression* makeAstItemCallExpr(AstExpression* function, const GenericList<AstExpression*>& args)
        {
            auto res = makeAstItemBaseExpression(EXPR_CALL);
            res->m_uexpr.exprcall.function = function;
            res->m_uexpr.exprcall.m_callargs = args;
            return res;
        }

        static AstExpression* makeAstItemIndex(AstExpression* left, AstExpression* index, bool isdot)
        {
            auto res = makeAstItemBaseExpression(EXPR_INDEX);
            res->m_uexpr.exprindex.isdot = isdot;
            res->m_uexpr.exprindex.left = left;
            res->m_uexpr.exprindex.index = index;
            return res;
        }

        static AstExpression* makeAstItemAssign(AstExpression* dest, AstExpression* source, bool is_postfix)
        {
            auto res = makeAstItemBaseExpression(EXPR_ASSIGN);
            res->m_uexpr.exprassign.dest = dest;
            res->m_uexpr.exprassign.source = source;
            res->m_uexpr.exprassign.is_postfix = is_postfix;
            return res;
        }

        static AstExpression* makeAstItemLogical(MathOpType op, AstExpression* left, AstExpression* right)
        {
            auto res = makeAstItemBaseExpression(EXPR_LOGICAL);
            res->m_uexpr.exprlogical.op = op;
            res->m_uexpr.exprlogical.left = left;
            res->m_uexpr.exprlogical.right = right;
            return res;
        }

        static AstExpression* makeAstItemTernary(AstExpression* test, AstExpression* ift, AstExpression* iffalse)
        {
            auto res = makeAstItemBaseExpression(EXPR_TERNARY);
            res->m_uexpr.exprternary.tercond = test;
            res->m_uexpr.exprternary.teriftrue = ift;
            res->m_uexpr.exprternary.teriffalse = iffalse;
            return res;
        }

        static AstExpression* makeAstItemInlineCall(AstExpression* expr, const char* fname)
        {
            auto fntoken = AstToken(AstToken::TOK_IDENT, fname, mc_util_strlen(fname));
            fntoken.m_tokpos = expr->m_exprpos;
            auto ident = ExprIdent(fntoken);
            ident.m_exprpos = fntoken.m_tokpos;
            auto functionidentexpr = makeAstItemIdent(ident);
            functionidentexpr->m_exprpos = expr->m_exprpos;
            GenericList<AstExpression*> args(0, nullptr);
            args.push(expr);
            auto ce = makeAstItemCallExpr(functionidentexpr, args);
            ce->m_exprpos = expr->m_exprpos;
            return ce;
        }

        static AstExpression* makeAstItemDefine(const ExprIdent& name, AstExpression* value, bool assignable)
        {
            auto res = makeAstItemBaseExpression(EXPR_STMTDEFINE);
            res->m_uexpr.exprdefine.name = name;
            res->m_uexpr.exprdefine.value = value;
            res->m_uexpr.exprdefine.assignable = assignable;
            return res;
        }

        static AstExpression* makeAstItemIfStmt(const GenericList<ExprIfCase*>& cases, const ExprCodeBlock& alternative, bool havealt)
        {
            auto res = makeAstItemBaseExpression(EXPR_STMTIF);
            res->m_uexpr.exprifstmt.m_haveifstmtelsestmt = havealt;
            res->m_uexpr.exprifstmt.m_ifcases = cases;
            res->m_uexpr.exprifstmt.m_ifstmtelsestmt = alternative;
            return res;
        }

        static AstExpression* makeAstItemReturnStmt(AstExpression* value)
        {
            auto res = makeAstItemBaseExpression(EXPR_STMTRETURN);
            res->m_uexpr.exprreturnvalue = value;
            return res;
        }

        static AstExpression* makeAstItemExprStmt(AstExpression* value)
        {
            auto res = makeAstItemBaseExpression(EXPR_STMTEXPRESSION);
            res->m_uexpr.exprexpression = value;
            return res;
        }

        static AstExpression* makeAstItemWhileStmt(AstExpression* test, const ExprCodeBlock& body)
        {
            auto res = makeAstItemBaseExpression(EXPR_STMTLOOPWHILE);
            res->m_uexpr.exprwhileloopstmt.loopcond = test;
            res->m_uexpr.exprwhileloopstmt.body = body;
            return res;
        }

        static AstExpression* makeAstItemBreakStmt()
        {
            auto res = makeAstItemBaseExpression(EXPR_STMTBREAK);
            return res;
        }

        static AstExpression* makeAstItemForeachStmt(const ExprIdent& iterator, AstExpression* source, const ExprCodeBlock& body)
        {
            auto res = makeAstItemBaseExpression(EXPR_STMTLOOPFOREACH);
            res->m_uexpr.exprforeachloopstmt.iterator = iterator;
            res->m_uexpr.exprforeachloopstmt.source = source;
            res->m_uexpr.exprforeachloopstmt.body = body;
            return res;
        }

        static AstExpression* makeAstItemForLoopStmt(AstExpression* init, AstExpression* test, AstExpression* update, const ExprCodeBlock& body)
        {
            auto res = makeAstItemBaseExpression(EXPR_STMTLOOPFORCLASSIC);
            res->m_uexpr.exprforloopstmt.init = init;
            res->m_uexpr.exprforloopstmt.loopcond = test;
            res->m_uexpr.exprforloopstmt.update = update;
            res->m_uexpr.exprforloopstmt.body = body;
            return res;
        }

        static AstExpression* makeAstItemContinueStmt()
        {
            auto res = makeAstItemBaseExpression(EXPR_STMTCONTINUE);
            return res;
        }

        static AstExpression* makeAstItemBlockStmt(const ExprCodeBlock& block)
        {
            auto res = makeAstItemBaseExpression(EXPR_STMTBLOCK);
            res->m_uexpr.exprblockstmt = block;
            return res;
        }

        static AstExpression* makeAstItemImportExpr(char* strpath)
        {
            auto res = makeAstItemBaseExpression(EXPR_STMTIMPORT);
            res->m_uexpr.exprimportstmt.path = strpath;
            return res;
        }

        static AstExpression* copyExpression(AstExpression* expr)
        {
            AstExpression* res;
            if(expr == nullptr)
            {
                return nullptr;
            }
            res = nullptr;
            switch(expr->m_exprtype)
            {
                case EXPR_NONE:
                    {
                        MC_ASSERT(false);
                    }
                    break;
                case EXPR_IDENT:
                    {
                        auto ident = ExprIdent::copy(&expr->m_uexpr.exprident);
                        res = makeAstItemIdent(ident);
                    }
                    break;
                case EXPR_NUMBERLITERAL:
                    {
                        res = makeAstItemLiteralNumber(expr->m_uexpr.exprlitnumber);
                    }
                    break;
                case EXPR_BOOLLITERAL:
                    {
                        res = makeAstItemLiteralBool(expr->m_uexpr.exprlitbool);
                    }
                    break;
                case EXPR_STRINGLITERAL:
                    {
                        auto stringcopy = mc_util_strndup(expr->m_uexpr.exprlitstring.m_strexprdata, expr->m_uexpr.exprlitstring.m_strexprlength);
                        res = makeAstItemLiteralString(stringcopy, expr->m_uexpr.exprlitstring.m_strexprlength);
                    }
                    break;

                case EXPR_NULLLITERAL:
                    {
                        res = makeAstItemLiteralNull();
                    }
                    break;
                case EXPR_ARRAYLITERAL:
                    {
                        auto valuescopy = expr->m_uexpr.exprlitarray.m_litarritems.copyToStack(copyExpression, destroyExpression);
                        res = makeAstItemLiteralArray(valuescopy);
                    }
                    break;

                case EXPR_MAPLITERAL:
                    {
                        auto keyscopy = expr->m_uexpr.exprlitmap.m_litmapkeys.copyToStack(copyExpression, destroyExpression);
                        auto valuescopy = expr->m_uexpr.exprlitmap.m_litmapvalues.copyToStack(copyExpression, destroyExpression);
                        res = makeAstItemLiteralMap(keyscopy, valuescopy);
                    }
                    break;
                case EXPR_PREFIX:
                    {
                        auto rightcopy = copyExpression(expr->m_uexpr.exprprefix.right);
                        res = makeAstItemPrefix(expr->m_uexpr.exprprefix.op, rightcopy);
                    }
                    break;
                case EXPR_INFIX:
                    {
                        auto leftcopy = copyExpression(expr->m_uexpr.exprinfix.left);
                        auto rightcopy = copyExpression(expr->m_uexpr.exprinfix.right);
                        res = makeAstItemInfix(expr->m_uexpr.exprinfix.op, leftcopy, rightcopy);
                    }
                    break;
                case EXPR_FUNCTIONLITERAL:
                    {
                        auto pacopy = expr->m_uexpr.exprlitfunction.funcparamlist.copyToStack(ExprFuncParam::copy, ExprFuncParam::destroy);
                        auto bodycopy = ExprCodeBlock::copy(&expr->m_uexpr.exprlitfunction.body);
                        auto namecopy = mc_util_strdup(expr->m_uexpr.exprlitfunction.name);
                        res = makeAstItemLiteralFunction(pacopy, bodycopy);
                        res->m_uexpr.exprlitfunction.name = namecopy;
                    }
                    break;
                case EXPR_CALL:
                    {
                        auto fcopy = copyExpression(expr->m_uexpr.exprcall.function);
                        auto argscopy = expr->m_uexpr.exprcall.m_callargs.copyToStack(copyExpression, destroyExpression);
                        res = makeAstItemCallExpr(fcopy, argscopy);
                    }
                    break;
                case EXPR_INDEX:
                    {
                        auto leftcopy = copyExpression(expr->m_uexpr.exprindex.left);
                        auto indexcopy = copyExpression(expr->m_uexpr.exprindex.index);
                        res = makeAstItemIndex(leftcopy, indexcopy, false);
                    }
                    break;
                case EXPR_ASSIGN:
                    {
                        auto destcopy = copyExpression(expr->m_uexpr.exprassign.dest);
                        auto sourcecopy = copyExpression(expr->m_uexpr.exprassign.source);
                        res = makeAstItemAssign(destcopy, sourcecopy, expr->m_uexpr.exprassign.is_postfix);
                    }
                    break;
                case EXPR_LOGICAL:
                    {
                        auto leftcopy = copyExpression(expr->m_uexpr.exprlogical.left);
                        auto rightcopy = copyExpression(expr->m_uexpr.exprlogical.right);
                        res = makeAstItemLogical(expr->m_uexpr.exprlogical.op, leftcopy, rightcopy);
                    }
                    break;
                case EXPR_TERNARY:
                    {
                        auto testcopy = copyExpression(expr->m_uexpr.exprternary.tercond);
                        auto iftruecopy = copyExpression(expr->m_uexpr.exprternary.teriftrue);
                        auto iffalsecopy = copyExpression(expr->m_uexpr.exprternary.teriffalse);
                        res = makeAstItemTernary(testcopy, iftruecopy, iffalsecopy);
                    }
                    break;
                case EXPR_STMTDEFINE:
                    {
                        auto valuecopy = copyExpression(expr->m_uexpr.exprdefine.value);
                        res = makeAstItemDefine(ExprIdent::copy(&expr->m_uexpr.exprdefine.name), valuecopy, expr->m_uexpr.exprdefine.assignable);
                    }
                    break;
                case EXPR_STMTIF:
                    {
                        ExprCodeBlock alternativecopy;
                        auto casescopy = expr->m_uexpr.exprifstmt.m_ifcases.copyToStack(ExprIfCase::copy, ExprIfCase::destroy);
                        if(expr->m_uexpr.exprifstmt.m_haveifstmtelsestmt)
                        {
                            alternativecopy = ExprCodeBlock::copy(&expr->m_uexpr.exprifstmt.m_ifstmtelsestmt);
                        }
                        res = makeAstItemIfStmt(casescopy, alternativecopy, expr->m_uexpr.exprifstmt.m_haveifstmtelsestmt);
                    }
                    break;
                case EXPR_STMTRETURN:
                    {
                        auto valuecopy = copyExpression(expr->m_uexpr.exprreturnvalue);
                        res = makeAstItemReturnStmt(valuecopy);
                    }
                    break;
                case EXPR_STMTEXPRESSION:
                    {
                        auto valuecopy = copyExpression(expr->m_uexpr.exprexpression);
                        res = makeAstItemExprStmt(valuecopy);
                    }
                    break;
                case EXPR_STMTLOOPWHILE:
                    {
                        auto testcopy = copyExpression(expr->m_uexpr.exprwhileloopstmt.loopcond);
                        auto bodycopy = ExprCodeBlock::copy(&expr->m_uexpr.exprwhileloopstmt.body);
                        res = makeAstItemWhileStmt(testcopy, bodycopy);
                    }
                    break;
                case EXPR_STMTBREAK:
                    {
                        res = makeAstItemBreakStmt();
                    }
                    break;
                case EXPR_STMTCONTINUE:
                    {
                        res = makeAstItemContinueStmt();
                    }
                    break;
                case EXPR_STMTLOOPFOREACH:
                    {
                        auto sourcecopy = copyExpression(expr->m_uexpr.exprforeachloopstmt.source);
                        auto bodycopy = ExprCodeBlock::copy(&expr->m_uexpr.exprforeachloopstmt.body);
                        res = makeAstItemForeachStmt(ExprIdent::copy(&expr->m_uexpr.exprforeachloopstmt.iterator), sourcecopy, bodycopy);
                    }
                    break;
                case EXPR_STMTLOOPFORCLASSIC:
                    {
                        auto initcopy = copyExpression(expr->m_uexpr.exprforloopstmt.init);
                        auto testcopy = copyExpression(expr->m_uexpr.exprforloopstmt.loopcond);
                        auto updatecopy = copyExpression(expr->m_uexpr.exprforloopstmt.update);
                        auto bodycopy = ExprCodeBlock::copy(&expr->m_uexpr.exprforloopstmt.body);
                        res = makeAstItemForLoopStmt(initcopy, testcopy, updatecopy, bodycopy);
                    }
                    break;
                case EXPR_STMTBLOCK:
                    {
                        auto blockcopy = ExprCodeBlock::copy(&expr->m_uexpr.exprblockstmt);
                        res = makeAstItemBlockStmt(blockcopy);
                    }
                    break;
                case EXPR_STMTIMPORT:
                    {
                        auto pathcopy = mc_util_strdup(expr->m_uexpr.exprimportstmt.path);
                        res = makeAstItemImportExpr(pathcopy);
                    }
                    break;
                case EXPR_STMTRECOVER:
                    {
                        auto bodycopy = ExprCodeBlock::copy(&expr->m_uexpr.exprrecoverstmt.body);
                        auto erroridentcopy = ExprIdent::copy(&expr->m_uexpr.exprrecoverstmt.errident);
                        res = makeAstItemRecover(erroridentcopy, bodycopy);
                    }
                    break;
                default:
                    {
                    }
                    break;
            }
            if(res == nullptr)
            {
                return nullptr;
            }
            res->m_exprpos = expr->m_exprpos;
            return res;
        }

        void destroyInstanceExpression()
        {
            destroyExpression(this);
        }

        AstExpression* copyInstanceExpression()
        {
            return copyExpression(this);
        }

        static void destroyExpression(AstExpression* expr)
        {
            if(expr != nullptr)
            {
                switch(expr->m_exprtype)
                {
                    case EXPR_NONE:
                        {
                            MC_ASSERT(false);
                        }
                        break;
                    case EXPR_IDENT:
                        {
                            expr->m_uexpr.exprident.deInit();
                        }
                        break;
                    case EXPR_NUMBERLITERAL:
                    case EXPR_BOOLLITERAL:
                        {
                        }
                        break;
                    case EXPR_STRINGLITERAL:
                        {
                            mc_memory_free(expr->m_uexpr.exprlitstring.m_strexprdata);
                        }
                        break;
                    case EXPR_NULLLITERAL:
                        {
                        }
                        break;
                    case EXPR_ARRAYLITERAL:
                        {
                            expr->m_uexpr.exprlitarray.m_litarritems.deInit(destroyExpression);
                        }
                        break;
                    case EXPR_MAPLITERAL:
                        {
                            expr->m_uexpr.exprlitmap.m_litmapkeys.deInit(destroyExpression);
                            expr->m_uexpr.exprlitmap.m_litmapvalues.deInit(destroyExpression);
                        }
                        break;
                    case EXPR_PREFIX:
                        {
                            destroyExpression(expr->m_uexpr.exprprefix.right);
                        }
                        break;
                    case EXPR_INFIX:
                        {
                            destroyExpression(expr->m_uexpr.exprinfix.left);
                            destroyExpression(expr->m_uexpr.exprinfix.right);
                        }
                        break;
                    case EXPR_FUNCTIONLITERAL:
                        {
                            ExprLiteralFunction* fn;
                            fn = &expr->m_uexpr.exprlitfunction;
                            mc_memory_free(fn->name);
                            fn->funcparamlist.deInit(ExprFuncParam::destroy);
                            fn->body.deInit();
                        }
                        break;
                    case EXPR_CALL:
                        {
                            expr->m_uexpr.exprcall.m_callargs.deInit(destroyExpression);
                            destroyExpression(expr->m_uexpr.exprcall.function);
                        }
                        break;
                    case EXPR_INDEX:
                        {
                            destroyExpression(expr->m_uexpr.exprindex.left);
                            destroyExpression(expr->m_uexpr.exprindex.index);
                        }
                        break;
                    case EXPR_ASSIGN:
                        {
                            destroyExpression(expr->m_uexpr.exprassign.dest);
                            destroyExpression(expr->m_uexpr.exprassign.source);
                        }
                        break;
                    case EXPR_LOGICAL:
                        {
                            destroyExpression(expr->m_uexpr.exprlogical.left);
                            destroyExpression(expr->m_uexpr.exprlogical.right);
                        }
                        break;
                    case EXPR_TERNARY:
                        {
                            destroyExpression(expr->m_uexpr.exprternary.tercond);
                            destroyExpression(expr->m_uexpr.exprternary.teriftrue);
                            destroyExpression(expr->m_uexpr.exprternary.teriffalse);
                        }
                        break;
                    case EXPR_STMTDEFINE:
                        {
                            expr->m_uexpr.exprdefine.name.deInit();
                            destroyExpression(expr->m_uexpr.exprdefine.value);
                        }
                        break;
                    case EXPR_STMTIF:
                        {
                            expr->m_uexpr.exprifstmt.m_ifcases.deInit(ExprIfCase::destroy);
                            expr->m_uexpr.exprifstmt.m_ifstmtelsestmt.deInit();
                        }
                        break;
                    case EXPR_STMTRETURN:
                        {
                            destroyExpression(expr->m_uexpr.exprreturnvalue);
                        }
                        break;
                    case EXPR_STMTEXPRESSION:
                        {
                            destroyExpression(expr->m_uexpr.exprexpression);
                        }
                        break;
                    case EXPR_STMTLOOPWHILE:
                        {
                            destroyExpression(expr->m_uexpr.exprwhileloopstmt.loopcond);
                            expr->m_uexpr.exprwhileloopstmt.body.deInit();
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
                            expr->m_uexpr.exprforeachloopstmt.iterator.deInit();
                            destroyExpression(expr->m_uexpr.exprforeachloopstmt.source);
                            expr->m_uexpr.exprforeachloopstmt.body.deInit();
                        }
                        break;
                    case EXPR_STMTLOOPFORCLASSIC:
                        {
                            destroyExpression(expr->m_uexpr.exprforloopstmt.init);
                            destroyExpression(expr->m_uexpr.exprforloopstmt.loopcond);
                            destroyExpression(expr->m_uexpr.exprforloopstmt.update);
                            expr->m_uexpr.exprforloopstmt.body.deInit();
                        }
                        break;
                    case EXPR_STMTBLOCK:
                        {
                            expr->m_uexpr.exprblockstmt.deInit();
                        }
                        break;
                    case EXPR_STMTIMPORT:
                        {
                            mc_memory_free(expr->m_uexpr.exprimportstmt.path);
                        }
                        break;
                    case EXPR_STMTRECOVER:
                        {
                            expr->m_uexpr.exprrecoverstmt.body.deInit();
                            expr->m_uexpr.exprrecoverstmt.errident.deInit();
                        }
                        break;
                    default:
                        {
                        }
                        break;
                }
                Memory::destroy(expr);
                expr = nullptr;
            }
        }


    public:
};

class CompiledProgram
{
    public:
        uint16_t* m_compiledbytecode;
        AstLocation* m_progsrcposlist;
        int m_compiledcount;

    public:
        CompiledProgram(uint16_t* bc, AstLocation* spl, int cnt)
        {
            m_compiledbytecode = bc;
            m_progsrcposlist = spl;
            m_compiledcount = cnt;
        }

        static void destroy(CompiledProgram* res)
        {
            if(res != nullptr)
            {
                mc_memory_free(res->m_compiledbytecode);
                mc_memory_free(res->m_progsrcposlist);
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
                for(i = 0; i < file->m_srclines.count(); i++)
                {
                    item = (void*)file->m_srclines.get(i);
                    mc_memory_free(item);
                }
                file->m_srclines.deInit();
                mc_memory_free(file->m_dirpath);
                mc_memory_free(file->m_path);
                mc_memory_free(file);
            }
        }

    public:
        char* m_dirpath;
        char* m_path;
        GenericList<char*> m_srclines;

    public:
        AstSourceFile(const char* strpath)
        {
            size_t len;
            const char* lastslashpos;
            lastslashpos = strrchr(strpath, '/');
            if(lastslashpos != nullptr)
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
                char* m_tbtracefuncname;
                AstLocation m_tbpos;

            public:
                const char* getSourceLine()
                {
                    const char* line;
                    if(m_tbpos.m_locfile == nullptr)
                    {
                        return nullptr;
                    }
                    auto lines = &m_tbpos.m_locfile->m_srclines;
                    if((size_t)m_tbpos.m_locline >= (size_t)lines->count())
                    {
                        return nullptr;
                    }
                    line = (const char*)lines->get(m_tbpos.m_locline);
                    return line;
                }

                const char* getSourceFilename()
                {
                    if(m_tbpos.m_locfile == nullptr)
                    {
                        return nullptr;
                    }
                    return m_tbpos.m_locfile->path();
                }
        };

    public:
        GenericList<Item> m_tbitems = GenericList<Item>(0, Item{});

    public:
        Traceback()
        {
        }

        static void destroy(Traceback* traceback)
        {
            size_t i;
            if(traceback != nullptr)
            {
                for(i = 0; i < traceback->m_tbitems.count(); i++)
                {
                    auto item = traceback->m_tbitems.getp(i);
                    mc_memory_free(item->m_tbtracefuncname);
                    item->m_tbtracefuncname = nullptr;
                }
                traceback->m_tbitems.deInit();
                mc_memory_free(traceback);
                traceback = nullptr;
            }
        }

        bool push(const char* fname, AstLocation pos)
        {
            bool ok;
            Item item;
            (void)ok;
            item.m_tbtracefuncname = mc_util_strdup(fname);
            if(item.m_tbtracefuncname == nullptr)
            {
                return false;
            }
            item.m_tbpos = pos;
            ok = m_tbitems.push(item);
            return true;
        }

        int mc_traceback_getdepth()
        {
            return m_tbitems.count();
        }

        const char* mc_traceback_getsourcefilepath(int depth)
        {
            Item* item;
            item = m_tbitems.getp(depth);
            if(item == nullptr)
            {
                return nullptr;
            }
            return item->getSourceFilename();
        }

        const char* mc_traceback_getsourcelinecode(int depth)
        {
            Item* item;
            item = m_tbitems.getp(depth);
            if(item == nullptr)
            {
                return nullptr;
            }
            return item->getSourceLine();
        }

        int mc_traceback_getsourcelinenumber(int depth)
        {
            Item* item;
            item = m_tbitems.getp(depth);
            if(item == nullptr)
            {
                return -1;
            }
            return item->m_tbpos.m_locline;
        }

        int mc_traceback_getsourcecolumn(int depth)
        {
            Item* item;
            item = m_tbitems.getp(depth);
            if(item == nullptr)
            {
                return -1;
            }
            return item->m_tbpos.m_loccolumn;
        }

        const char* mc_traceback_getfunctionname(int depth)
        {
            Item* item;
            item = m_tbitems.getp(depth);
            if(item == nullptr)
            {
                return "";
            }
            return item->m_tbtracefuncname;
        }

        bool printTo(Printer* pr, Console::Color* mcc)
        {
            int i;
            int depth;
            const char* cblue;
            const char* cyell;
            const char* creset;
            const char* filename;
            Item* item;
            cblue = mcc->get('b');
            cyell = mcc->get('y');
            creset = mcc->get('0');
            depth = m_tbitems.count();
            for(i = 0; i < depth; i++)
            {
                item = m_tbitems.getp(i);
                filename = item->getSourceFilename();
                pr->format("  function %s%s%s", cblue, item->m_tbtracefuncname, creset);
                if(item->m_tbpos.m_locline >= 0 && item->m_tbpos.m_loccolumn >= 0)
                {
                    pr->format(" in %s%s:%d:%d%s", cyell, filename, item->m_tbpos.m_locline, item->m_tbpos.m_loccolumn, creset);
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
        enum
        {
            MaxErrorMsgLength = (64),
        };

        enum Type
        {
            ERRTYP_NONE = 0,
            ERRTYP_PARSING,
            ERRTYP_COMPILING,
            ERRTYP_RUNTIME,
            ERRTYP_TIMEOUT,
            ERRTYP_MEMORY,
            ERRTYP_USER
        };

    public:
        static const char* mc_util_errortypename(Type type)
        {
            switch(type)
            {
                case ERRTYP_PARSING:
                    return "PARSING";
                case ERRTYP_COMPILING:
                    return "COMPILATION";
                case ERRTYP_RUNTIME:
                    return "RUNTIME";
                case ERRTYP_TIMEOUT:
                    return "TIMEOUT";
                case ERRTYP_MEMORY:
                    return "ALLOCATION";
                case ERRTYP_USER:
                    return "USER";
                default:
                    break;
            }
            return "NONE";
        }

        static bool printUserError(Printer* pr, Value obj)
        {
            const char* cred;
            const char* creset;
            Traceback* traceback;
            static const char eprefix[] = "script error";
            Console::Color mcc(fileno(stdout));
            cred = mcc.get('r');
            creset = mcc.get('0');
            pr->format("%s%s:%s %s\n", cred, eprefix, creset, mc_value_errorgetmessage(obj));
            traceback = mc_value_errorgettraceback(obj);
            MC_ASSERT(traceback != nullptr);
            if(traceback != nullptr)
            {
                pr->format("%sTraceback:%s\n", cred, creset);
                traceback->printTo(pr, &mcc);
            }
            return true;
        }

        static bool printErrorTo(Error* err, Printer* pr)
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
            static const char eprefix[] = "runtime error";
            Console::Color mcc(fileno(stdout));
            cred = mcc.get('r');
            cblue = mcc.get('b');
            creset = mcc.get('0');
            typestr = err->getTypeString();
            filename = err->getFile();
            line = err->getSourceLineCode();
            linenum = err->getSourceLineNumber();
            colnum = err->getSourceColumn();
            if(line != nullptr)
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
            pr->format("%s%s %s%s in \"%s\" on %s%d:%d:%s %s\n", cred, eprefix, typestr, creset, filename, cblue, linenum, colnum, creset, err->getMessage());
            traceback = err->getTraceback();
            if(traceback != nullptr)
            {
                pr->format("traceback:\n");
                traceback->printTo(pr, &mcc);
            }
            return true;
        }

    public:
        Type m_errtype = ERRTYP_NONE;
        char m_message[MaxErrorMsgLength] = {};
        AstLocation m_pos;
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
            if(m_pos.m_locfile == nullptr)
            {
                return nullptr;
            }
            return m_pos.m_locfile->path();
        }

        const char* getSourceLineCode()
        {
            const char* line;
            if(m_pos.m_locfile == nullptr)
            {
                return nullptr;
            }
            auto lines = &m_pos.m_locfile->m_srclines;
            if(m_pos.m_locline >= (int)lines->count())
            {
                return nullptr;
            }
            line = (const char*)lines->get(m_pos.m_locline);
            return line;
        }

        int getSourceLineNumber()
        {
            if(m_pos.m_locline < 0)
            {
                return -1;
            }
            return m_pos.m_locline + 1;
        }

        int getSourceColumn()
        {
            if(m_pos.m_loccolumn < 0)
            {
                return -1;
            }
            return m_pos.m_loccolumn + 1;
        }

        Type getType()
        {
            switch(m_errtype)
            {
                case ERRTYP_NONE:
                    return ERRTYP_NONE;
                case ERRTYP_PARSING:
                    return ERRTYP_PARSING;
                case ERRTYP_COMPILING:
                    return ERRTYP_COMPILING;
                case ERRTYP_RUNTIME:
                    return ERRTYP_RUNTIME;
                case ERRTYP_TIMEOUT:
                    return ERRTYP_TIMEOUT;
                case ERRTYP_MEMORY:
                    return ERRTYP_MEMORY;
                case ERRTYP_USER:
                    return ERRTYP_USER;
                default:
                    break;
            }
            return ERRTYP_NONE;
        }

        const char* getTypeString()
        {
            return mc_util_errortypename(getType());
        }

        Traceback* getTraceback()
        {
            return m_traceback;
        }

        bool printErrorTo(Printer* pr)
        {
            return printErrorTo(this, pr);
        }
};

class ErrList
{
    public:
        enum
        {
            MaxErrItemCount = (10),
        };

    public:
        Error m_erroritems[MaxErrItemCount];
        int m_count;

    public:
        ErrList()
        {
            m_count = 0;
        }

        void destroy()
        {
            clear();
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
                error = get(i);
                if(error->m_traceback != nullptr)
                {
                    Memory::destroy(error->m_traceback);
                }
            }
            m_count = 0;
        }

        void pushMessage(Error::Type type, AstLocation pos, const char* message)
        {
            int len;
            int tocopy;
            Error err;
            if(m_count >= MaxErrItemCount)
            {
            }
            else
            {
                err.m_errtype = type;
                len = mc_util_strlen(message);
                tocopy = len;
                if(tocopy >= (Error::MaxErrorMsgLength - 1))
                {
                    tocopy = Error::MaxErrorMsgLength - 1;
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
        void pushFormat(Error::Type type, AstLocation pos, const char* format, ArgsT&&... args)
        {
            static auto tmpsnprintf = snprintf;
            int needsz;
            int printedsz;
            char res[Error::MaxErrorMsgLength];
            (void)needsz;
            (void)printedsz;
            needsz = tmpsnprintf(nullptr, 0, format, args...);
            printedsz = tmpsnprintf(res, Error::MaxErrorMsgLength, format, args...);
            MC_ASSERT(needsz == printedsz);
            pushMessage(type, pos, res);
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
        char m_currentchar;
        size_t m_line;
        size_t m_column;
        AstSourceFile* m_file;
        bool m_failed;
        bool m_continuetplstring;
        AstToken m_prevtoken;
        AstToken m_currtoken;
        AstToken m_peektoken;
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
            lex->m_currentchar = '\0';
            if(file != nullptr)
            {
                lex->m_line = file->m_srclines.count();
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
            lex->m_prevtoken = AstToken(AstToken::TOK_INVALID, nullptr, 0);
            lex->m_currtoken = AstToken(AstToken::TOK_INVALID, nullptr, 0);
            lex->m_peektoken = AstToken(AstToken::TOK_INVALID, nullptr, 0);
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

        bool currentTokenIs(AstToken::Type type)
        {
            return m_currtoken.type() == type;
        }

        bool peekTokenIs(AstToken::Type type)
        {
            return m_peektoken.type() == type;
        }

        bool nextToken()
        {
            m_prevtoken = m_currtoken;
            m_currtoken = m_peektoken;
            m_peektoken = nextTokenInternal();
            return !m_failed;
        }

        bool previousToken()
        {
            if(m_prevtoken.type() == AstToken::TOK_INVALID)
            {
                return false;
            }
            m_peektoken = m_currtoken;
            m_currtoken = m_prevtoken;
            m_prevtoken = AstToken(AstToken::TOK_INVALID, nullptr, 0);
            m_currentchar = m_prevstate.m_currentchar;
            m_column = m_prevstate.m_column;
            m_line = m_prevstate.m_line;
            m_position = m_prevstate.m_position;
            m_nextposition = m_prevstate.m_nextposition;
            return true;
        }

        AstToken nextTokenInternal()
        {
            char c;
            AstToken outtok;
            m_prevstate.m_currentchar = m_currentchar;
            m_prevstate.m_column = m_column;
            m_prevstate.m_line = m_line;
            m_prevstate.m_position = m_position;
            m_prevstate.m_nextposition = m_nextposition;
            while(true)
            {
                if(!m_continuetplstring)
                {
                    skipSpace();
                }
                outtok.m_toktype = AstToken::TOK_INVALID;
                outtok.m_tokstrdata = m_inputsource + m_position;
                outtok.m_tokstrlength = 1;
                outtok.m_tokpos = AstLocation(m_file, m_line, m_column);
                c = m_continuetplstring ? '`' : m_currentchar;
                switch(c)
                {
                    case '\0':
                        {
                            outtok = AstToken(AstToken::TOK_EOF, "EOF", 3);
                        }
                        break;
                    case '=':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::TOK_EQ, "==", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_ASSIGN, "=", 1);
                            }
                        }
                        break;
                    case '&':
                        {
                            if(peekChar() == '&')
                            {
                                outtok = AstToken(AstToken::TOK_AND, "&&", 2);
                                readChar();
                            }
                            else if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::TOK_ASSIGNBINAND, "&=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_BINAND, "&", 1);
                            }
                        }
                        break;
                    case '|':
                        {
                            if(peekChar() == '|')
                            {
                                outtok = AstToken(AstToken::TOK_OR, "||", 2);
                                readChar();
                            }
                            else if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::TOK_ASSIGNBINOR, "|=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_BINOR, "|", 1);
                            }
                        }
                        break;
                    case '^':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::TOK_ASSIGNBINXOR, "^=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_BINXOR, "^", 1);
                                break;
                            }
                        }
                        break;
                    case '+':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::TOK_ASSIGNPLUS, "+=", 2);
                                readChar();
                            }
                            else if(peekChar() == '+')
                            {
                                outtok = AstToken(AstToken::TOK_PLUSPLUS, "++", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_PLUS, "+", 1);
                                break;
                            }
                        }
                        break;
                    case '-':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::TOK_ASSIGNMINUS, "-=", 2);
                                readChar();
                            }
                            else if(peekChar() == '-')
                            {
                                outtok = AstToken(AstToken::TOK_MINUSMINUS, "--", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_UNARYMINUS, "-", 1);
                                break;
                            }
                        }
                        break;
                    case '~':
                        {
                            outtok = AstToken(AstToken::TOK_UNARYBINNOT, "~", 1);
                        }
                        break;
                    case '!':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::TOK_NOTEQ, "!=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_BANG, "!", 1);
                            }
                        }
                        break;
                    case '*':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::TOK_ASSIGNASTERISK, "*=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_ASTERISK, "*", 1);
                                break;
                            }
                        }
                        break;
                    case '/':
                        {
                            if(peekChar() == '/')
                            {
                                readChar();
                                while(m_currentchar != '\n' && m_currentchar != '\0')
                                {
                                    readChar();
                                }
                                continue;
                            }
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::TOK_ASSIGNSLASH, "/=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_SLASH, "/", 1);
                                break;
                            }
                        }
                        break;
                    case '<':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::TOK_LTE, "<=", 2);
                                readChar();
                            }
                            else if(peekChar() == '<')
                            {
                                readChar();
                                if(peekChar() == '=')
                                {
                                    outtok = AstToken(AstToken::TOK_ASSIGNLSHIFT, "<<=", 3);
                                    readChar();
                                }
                                else
                                {
                                    outtok = AstToken(AstToken::TOK_LSHIFT, "<<", 2);
                                }
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_LT, "<", 1);
                                break;
                            }
                        }
                        break;
                    case '>':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::TOK_GTE, ">=", 2);
                                readChar();
                            }
                            else if(peekChar() == '>')
                            {
                                readChar();
                                if(peekChar() == '=')
                                {
                                    outtok = AstToken(AstToken::TOK_ASSIGNRSHIFT, ">>=", 3);
                                    readChar();
                                }
                                else
                                {
                                    outtok = AstToken(AstToken::TOK_RSHIFT, ">>", 2);
                                }
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_GT, ">", 1);
                            }
                        }
                        break;
                    case ',':
                        {
                            outtok = AstToken(AstToken::TOK_COMMA, ",", 1);
                        }
                        break;
                    case ';':
                        {
                            outtok = AstToken(AstToken::TOK_SEMICOLON, ";", 1);
                        }
                        break;
                    case ':':
                        {
                            outtok = AstToken(AstToken::TOK_COLON, ":", 1);
                        }
                        break;
                    case '(':
                        {
                            outtok = AstToken(AstToken::TOK_LPAREN, "(", 1);
                        }
                        break;
                    case ')':
                        {
                            outtok = AstToken(AstToken::TOK_RPAREN, ")", 1);
                        }
                        break;
                    case '{':
                        {
                            outtok = AstToken(AstToken::TOK_LBRACE, "{", 1);
                        }
                        break;
                    case '}':
                        {
                            outtok = AstToken(AstToken::TOK_RBRACE, "}", 1);
                        }
                        break;
                    case '[':
                        {
                            outtok = AstToken(AstToken::TOK_LBRACKET, "[", 1);
                        }
                        break;
                    case ']':
                        {
                            outtok = AstToken(AstToken::TOK_RBRACKET, "]", 1);
                        }
                        break;
                    case '.':
                        {
                            outtok = AstToken(AstToken::TOK_DOT, ".", 1);
                        }
                        break;
                    case '?':
                        {
                            outtok = AstToken(AstToken::TOK_QUESTION, "?", 1);
                        }
                        break;
                    case '%':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::TOK_ASSIGNPERCENT, "%=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_PERCENT, "%", 1);
                                break;
                            }
                        }
                        break;
                    case '"':
                        {
                            int len;
                            const char* str;
                            readChar();
                            str = scanString('"', false, nullptr, &len);
                            if(str != nullptr)
                            {
                                outtok = AstToken(AstToken::TOK_STRING, str, len);
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_INVALID, nullptr, 0);
                            }
                        }
                        break;
                    case '\'':
                        {
                            int len;
                            const char* str;
                            readChar();
                            str = scanString('\'', false, nullptr, &len);
                            if(str != nullptr)
                            {
                                outtok = AstToken(AstToken::TOK_STRING, str, len);
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_INVALID, nullptr, 0);
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
                                readChar();
                            }
                            templatefound = false;
                            str = scanString('`', true, &templatefound, &len);
                            if(str != nullptr)
                            {
                                if(templatefound)
                                {
                                    outtok = AstToken(AstToken::TOK_TEMPLATESTRING, str, len);
                                }
                                else
                                {
                                    outtok = AstToken(AstToken::TOK_STRING, str, len);
                                }
                            }
                            else
                            {
                                outtok = AstToken(AstToken::TOK_INVALID, nullptr, 0);
                            }
                        }
                        break;
                    default:
                        {
                            int identlen;
                            int numberlen;
                            const char* ident;
                            const char* number;
                            AstToken::Type type;
                            if(charIsLetter(m_currentchar) || (m_currentchar == '$'))
                            {
                                identlen = 0;
                                ident = scanIdent(&identlen);
                                type = lookupIdent(ident, identlen);
                                outtok = AstToken(type, ident, identlen);
                                outtok.m_tokpos = AstLocation(m_file, m_line, m_column);
                                return outtok;
                            }
                            if(charIsDigit(m_currentchar))
                            {
                                numberlen = 0;
                                number = scanNumber(&numberlen);
                                outtok = AstToken(AstToken::TOK_NUMBER, number, numberlen);
                                outtok.m_tokpos = AstLocation(m_file, m_line, m_column);
                                return outtok;
                            }
                        }
                        break;
                }
                readChar();
                if(failed())
                {
                    outtok = AstToken(AstToken::TOK_INVALID, nullptr, 0);
                }
                m_continuetplstring = false;
                outtok.m_tokpos = AstLocation(m_file, m_line, m_column);
                return outtok;
            }
            /* NB. never reached; but keep the compiler from complaining. */
            return outtok;
        }

        bool expectCurrent(AstToken::Type type)
        {
            const char* actualtypestr;
            const char* expectedtypestr;
            if(failed())
            {
                return false;
            }
            if(!currentTokenIs(type))
            {
                expectedtypestr = AstToken::tokenName(type);
                actualtypestr = AstToken::tokenName(m_currtoken.type());
                m_errors->pushFormat(Error::ERRTYP_PARSING, m_currtoken.m_tokpos, "expected token \"%s\", but got \"%s\"", expectedtypestr, actualtypestr);
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
                m_currentchar = '\0';
            }
            else
            {
                m_currentchar = m_inputsource[m_nextposition];
            }
            m_position = m_nextposition;
            m_nextposition++;
            if(m_currentchar == '\n')
            {
                m_line++;
                m_column = -1;
                ok = addLineAt(m_nextposition);
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
            /*if(m_currentchar == '$')
            {
                readChar();
            }
            */
            position = m_position;
            while(charIsDigit(m_currentchar) || charIsLetter(m_currentchar) ||(m_currentchar == '$') || (m_currentchar == ':'))
            {
                if(m_currentchar == ':')
                {
                    if(peekChar() != ':')
                    {
                        goto end;
                    }
                    readChar();
                }
                readChar();
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
            static const char allowedchars[] = ".xXaAbBcCdDeEfF";
            static const int allowlen = ((int)(sizeof(allowedchars) / sizeof(allowedchars[0])));
            position = m_position;
            while(charIsDigit(m_currentchar) || charIsOneOf(m_currentchar, allowedchars, allowlen - 1))
            {
                readChar();
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
                if(m_currentchar == '\0')
                {
                    return nullptr;
                }
                if(m_currentchar == delimiter && !escaped)
                {
                    break;
                }
                if(istemplate && !escaped && m_currentchar == '$' && peekChar() == '{')
                {
                    *outtemplatefound = true;
                    break;
                }
                escaped = false;
                if(m_currentchar == '\\')
                {
                    escaped = true;
                }
                readChar();
            }
            len = m_position - position;
            *outlen = len;
            return m_inputsource + position;
        }

        AstToken::Type lookupIdent(const char* ident, int len)
        {
            int i;
            int klen;
            static struct
            {
                const char* value;
                AstToken::Type type;
            } keywords[] = {
                { "function", AstToken::TOK_FUNCTION },
                { "const", AstToken::TOK_CONST },
                { "var", AstToken::TOK_VAR },
                { "let", AstToken::TOK_VAR },
                { "true", AstToken::TOK_TRUE },
                { "false", AstToken::TOK_FALSE },
                { "if", AstToken::TOK_IF },
                { "else", AstToken::TOK_ELSE },
                { "return", AstToken::TOK_RETURN },
                { "while", AstToken::TOK_WHILE },
                { "break", AstToken::TOK_BREAK },
                { "for", AstToken::TOK_FOR },
                { "in", AstToken::TOK_IN },
                { "continue", AstToken::TOK_CONTINUE },
                { "null", AstToken::TOK_NULL },
                { "import", AstToken::TOK_IMPORT },
                { "recover", AstToken::TOK_RECOVER },
                { nullptr, (AstToken::Type)0}
            };
            for(i = 0; keywords[i].value != nullptr; i++)
            {
                klen = mc_util_strlen(keywords[i].value);
                if(klen == len && mc_util_strnequal(ident, keywords[i].value, len))
                {
                    return keywords[i].type;
                }
            }
            return AstToken::TOK_IDENT;
        }

        void skipSpace()
        {
            char ch;
            ch = m_currentchar;
            while(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
            {
                readChar();
                ch = m_currentchar;
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
            if(m_file == nullptr)
            {
                return true;
            }
            if(m_line < m_file->m_srclines.count())
            {
                return true;
            }
            linestart = m_inputsource + offset;
            newlineptr = strchr(linestart, '\n');
            line = nullptr;
            if(newlineptr == nullptr)
            {
                line = mc_util_strdup(linestart);
            }
            else
            {
                linelen = newlineptr - linestart;
                line = mc_util_strndup(linestart, linelen);
            }
            if(line == nullptr)
            {
                m_failed = true;
                return false;
            }
            ok = m_file->m_srclines.push(line);
            return true;
        }
};

class AstParser
{
    public:
        using mcastrightassocparsefn_t = bool(*)(AstParser*, AstExpression**);
        //using mcastleftassocparsefn_t = AstExpression* (*)(AstParser*, AstExpression*);
        using mcastleftassocparsefn_t = bool (*)(AstParser*, AstExpression**, AstExpression*);

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
        static mcastprecedence_t getPrecedence(AstToken::Type tk)
        {
            switch(tk)
            {
                case AstToken::TOK_EQ:
                case AstToken::TOK_NOTEQ:
                    return MC_ASTPREC_EQUALS;
                case AstToken::TOK_LT:
                case AstToken::TOK_LTE:
                case AstToken::TOK_GT:
                case AstToken::TOK_GTE:
                    return MC_ASTPREC_LESSGREATER;
                case AstToken::TOK_PLUS:
                case AstToken::TOK_UNARYMINUS:
                case AstToken::TOK_UNARYBINNOT:
                    return MC_ASTPREC_SUM;
                case AstToken::TOK_SLASH:
                case AstToken::TOK_ASTERISK:
                case AstToken::TOK_PERCENT:
                    return MC_ASTPREC_PRODUCT;
                case AstToken::TOK_LPAREN:
                case AstToken::TOK_LBRACKET:
                    return MC_ASTPREC_POSTFIX;
                case AstToken::TOK_ASSIGN:
                case AstToken::TOK_ASSIGNPLUS:
                case AstToken::TOK_ASSIGNMINUS:
                case AstToken::TOK_ASSIGNASTERISK:
                case AstToken::TOK_ASSIGNSLASH:
                case AstToken::TOK_ASSIGNPERCENT:
                case AstToken::TOK_ASSIGNBINAND:
                case AstToken::TOK_ASSIGNBINOR:
                case AstToken::TOK_ASSIGNBINXOR:
                case AstToken::TOK_ASSIGNLSHIFT:
                case AstToken::TOK_ASSIGNRSHIFT:
                    return MC_ASTPREC_ASSIGN;
                case AstToken::TOK_DOT:
                    return MC_ASTPREC_POSTFIX;
                case AstToken::TOK_AND:
                    return MC_ASTPREC_LOGICALAND;
                case AstToken::TOK_OR:
                    return MC_ASTPREC_LOGICALOR;
                case AstToken::TOK_BINOR:
                    return MC_ASTPREC_BINOR;
                case AstToken::TOK_BINXOR:
                    return MC_ASTPREC_BINXOR;
                case AstToken::TOK_BINAND:
                    return MC_ASTPREC_BINAND;
                case AstToken::TOK_LSHIFT:
                case AstToken::TOK_RSHIFT:
                    return MC_ASTPREC_SHIFT;
                case AstToken::TOK_QUESTION:
                    return MC_ASTPREC_TERNARY;
                case AstToken::TOK_PLUSPLUS:
                case AstToken::TOK_MINUSMINUS:
                    return MC_ASTPREC_INCDEC;
                default:
                    break;
            }
            return MC_ASTPREC_LOWEST;
        }

        static AstExpression::MathOpType tokenToMathOP(AstToken::Type tk)
        {
            switch(tk)
            {
                case AstToken::TOK_ASSIGN:
                    return AstExpression::MATHOP_ASSIGN;
                case AstToken::TOK_PLUS:
                    return AstExpression::MATHOP_PLUS;
                case AstToken::TOK_UNARYMINUS:
                    return AstExpression::MATHOP_MINUS;
                case AstToken::TOK_UNARYBINNOT:
                    return AstExpression::MATHOP_BINNOT;
                case AstToken::TOK_BANG:
                    return AstExpression::MATHOP_BANG;
                case AstToken::TOK_ASTERISK:
                    return AstExpression::MATHOP_ASTERISK;
                case AstToken::TOK_SLASH:
                    return AstExpression::MATHOP_SLASH;
                case AstToken::TOK_LT:
                    return AstExpression::MATHOP_LT;
                case AstToken::TOK_LTE:
                    return AstExpression::MATHOP_LTE;
                case AstToken::TOK_GT:
                    return AstExpression::MATHOP_GT;
                case AstToken::TOK_GTE:
                    return AstExpression::MATHOP_GTE;
                case AstToken::TOK_EQ:
                    return AstExpression::MATHOP_EQ;
                case AstToken::TOK_NOTEQ:
                    return AstExpression::MATHOP_NOTEQ;
                case AstToken::TOK_PERCENT:
                    return AstExpression::MATHOP_MODULUS;
                case AstToken::TOK_AND:
                    return AstExpression::MATHOP_LOGICALAND;
                case AstToken::TOK_OR:
                    return AstExpression::MATHOP_LOGICALOR;
                case AstToken::TOK_ASSIGNPLUS:
                    return AstExpression::MATHOP_PLUS;
                case AstToken::TOK_ASSIGNMINUS:
                    return AstExpression::MATHOP_MINUS;
                case AstToken::TOK_ASSIGNASTERISK:
                    return AstExpression::MATHOP_ASTERISK;
                case AstToken::TOK_ASSIGNSLASH:
                    return AstExpression::MATHOP_SLASH;
                case AstToken::TOK_ASSIGNPERCENT:
                    return AstExpression::MATHOP_MODULUS;
                case AstToken::TOK_ASSIGNBINAND:
                    return AstExpression::MATHOP_BINAND;
                case AstToken::TOK_ASSIGNBINOR:
                    return AstExpression::MATHOP_BINOR;
                case AstToken::TOK_ASSIGNBINXOR:
                    return AstExpression::MATHOP_BINXOR;
                case AstToken::TOK_ASSIGNLSHIFT:
                    return AstExpression::MATHOP_LSHIFT;
                case AstToken::TOK_ASSIGNRSHIFT:
                    return AstExpression::MATHOP_RSHIFT;
                case AstToken::TOK_BINAND:
                    return AstExpression::MATHOP_BINAND;
                case AstToken::TOK_BINOR:
                    return AstExpression::MATHOP_BINOR;
                case AstToken::TOK_BINXOR:
                    return AstExpression::MATHOP_BINXOR;
                case AstToken::TOK_LSHIFT:
                    return AstExpression::MATHOP_LSHIFT;
                case AstToken::TOK_RSHIFT:
                    return AstExpression::MATHOP_RSHIFT;
                case AstToken::TOK_PLUSPLUS:
                    return AstExpression::MATHOP_PLUS;
                case AstToken::TOK_MINUSMINUS:
                    return AstExpression::MATHOP_MINUS;
                default:
                    {
                        MC_ASSERT(false);
                    }
                    break;
            }
            return AstExpression::MATHOP_NONE;
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
            if(output == nullptr)
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

        static bool callback_parseident(AstParser* p, AstExpression** res)
        {
            return p->parseIdent(res);
        }

        static bool callback_parseliteralnumber(AstParser* p, AstExpression** res)
        {
            return p->parseLiteralNumber(res);
        }

        static bool callback_parseliteralbool(AstParser* p, AstExpression** res)
        {
            return p->parseLiteralBool(res);
        }

        static bool callback_parseliteralstring(AstParser* p, AstExpression** res)
        {
            return p->parseLiteralString(res);
        }

        static bool callback_parseliteraltemplatestring(AstParser* p, AstExpression** res)
        {
            return p->parseLiteralTemplateString(res);
        }

        static bool callback_parseliteralnull(AstParser* p, AstExpression** res)
        {
            return p->parseLiteralNull(res);
        }

        static bool callback_parseliteralarray(AstParser* p, AstExpression** res)
        {
            return p->parseLiteralArray(res);
        }

        static bool callback_parseliteralmap(AstParser* p, AstExpression** res)
        {
            return p->parseLiteralMap(res);
        }

        static bool callback_parseprefixexpr(AstParser* p, AstExpression** res)
        {
            return p->parsePrefixExpr(res);
        }

        static bool callback_parseinfixexpr(AstParser* p, AstExpression** res, AstExpression* left)
        {
            return p->parseInfixExpr(res, left);
        }

        static bool callback_parseliteralfunction(AstParser* p, AstExpression** res)
        {
            return p->parseLiteralFunction(res);
        }

        static bool callback_parseindexexpr(AstParser* p, AstExpression** res, AstExpression* left)
        {
            return p->parseIndexExpr(res, left);
        }

        static bool callback_parseassignexpr(AstParser* p, AstExpression** res, AstExpression* left)
        {
            return p->parseAssignExpr(res, left);
        }

        static bool callback_parselogicalexpr(AstParser* p, AstExpression** res, AstExpression* left)
        {
            return p->parseLogicalExpr(res, left);
        }

        static bool callback_parseternaryexpr(AstParser* p, AstExpression** res, AstExpression* left)
        {
            return p->parseTernaryExpr(res, left);
        }

        static bool callback_parseincdecprefixexpr(AstParser* p, AstExpression** res)
        {
            return p->parseIncDecPrefixExpr(res);
        }

        static bool callback_parseincdecpostfixexpr(AstParser* p, AstExpression** res, AstExpression* left)
        {
            return p->parseIncDecPostfixExpr(res, left);
        }

        static bool callback_parsedotexpression(AstParser* p, AstExpression** res, AstExpression* left)
        {
            return p->parseDotExpr(res, left);
        }

        static bool callback_parserecoverstmt(AstParser* p, AstExpression** res)
        {
            return p->parseRecoverStmt(res);
        }

        static bool callback_parsegroupedexpr(AstParser* p, AstExpression** res)
        {
            return p->parseGroupedExpr(res);
        }

        static bool callback_parsecallexpr(AstParser* p, AstExpression** res, AstExpression* left)
        {
            return p->parseCallExpr(res, left);
        }

        static void destroy(AstParser* parser)
        {
            if(parser != nullptr)
            {
                mc_memory_free(parser);
            }
        }

        /*
        * these two functions used to be a table; but that made it functionally
        * impossible if a callback for a token type existed or not
        */
        static mcastrightassocparsefn_t getRightAssocParseFunc(AstToken::Type t)
        {
            switch(t)
            {
                case AstToken::TOK_IDENT: return callback_parseident;
                case AstToken::TOK_NUMBER: return callback_parseliteralnumber;
                case AstToken::TOK_TRUE: return callback_parseliteralbool;
                case AstToken::TOK_FALSE: return callback_parseliteralbool;
                case AstToken::TOK_STRING: return callback_parseliteralstring;
                case AstToken::TOK_TEMPLATESTRING: return callback_parseliteraltemplatestring;
                case AstToken::TOK_NULL: return callback_parseliteralnull;
                case AstToken::TOK_BANG: return callback_parseprefixexpr;
                case AstToken::TOK_UNARYMINUS: return callback_parseprefixexpr;
                case AstToken::TOK_UNARYBINNOT: return callback_parseprefixexpr;
                case AstToken::TOK_LPAREN: return callback_parsegroupedexpr;
                case AstToken::TOK_FUNCTION: return callback_parseliteralfunction;
                case AstToken::TOK_LBRACKET: return callback_parseliteralarray;
                case AstToken::TOK_LBRACE: return callback_parseliteralmap;
                case AstToken::TOK_PLUSPLUS: return callback_parseincdecprefixexpr;
                case AstToken::TOK_MINUSMINUS: return callback_parseincdecprefixexpr;
                case AstToken::TOK_RECOVER: return callback_parserecoverstmt;
                default:
                    break;
            }
            return nullptr;
        }

        static mcastleftassocparsefn_t getLeftAssocParseFunc(AstToken::Type t)
        {
            switch(t)
            {
                case AstToken::TOK_PLUS: return callback_parseinfixexpr;
                case AstToken::TOK_UNARYMINUS: return callback_parseinfixexpr;
                case AstToken::TOK_SLASH: return callback_parseinfixexpr;
                case AstToken::TOK_ASTERISK: return callback_parseinfixexpr;
                case AstToken::TOK_PERCENT: return callback_parseinfixexpr;
                case AstToken::TOK_EQ: return callback_parseinfixexpr;
                case AstToken::TOK_NOTEQ: return callback_parseinfixexpr;
                case AstToken::TOK_LT: return callback_parseinfixexpr;
                case AstToken::TOK_LTE: return callback_parseinfixexpr;
                case AstToken::TOK_GT: return callback_parseinfixexpr;
                case AstToken::TOK_GTE: return callback_parseinfixexpr;
                case AstToken::TOK_LPAREN: return callback_parsecallexpr;
                case AstToken::TOK_LBRACKET: return callback_parseindexexpr;
                case AstToken::TOK_ASSIGN: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNPLUS: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNMINUS: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNSLASH: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNASTERISK: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNPERCENT: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNBINAND: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNBINOR: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNBINXOR: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNLSHIFT: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNRSHIFT: return callback_parseassignexpr;
                case AstToken::TOK_DOT: return callback_parsedotexpression;
                case AstToken::TOK_AND: return callback_parselogicalexpr;
                case AstToken::TOK_OR: return callback_parselogicalexpr;
                case AstToken::TOK_BINAND: return callback_parseinfixexpr;
                case AstToken::TOK_BINOR: return callback_parseinfixexpr;
                case AstToken::TOK_BINXOR: return callback_parseinfixexpr;
                case AstToken::TOK_LSHIFT: return callback_parseinfixexpr;
                case AstToken::TOK_RSHIFT: return callback_parseinfixexpr;
                case AstToken::TOK_QUESTION: return callback_parseternaryexpr;
                case AstToken::TOK_PLUSPLUS: return callback_parseincdecpostfixexpr;
                case AstToken::TOK_MINUSMINUS: return callback_parseincdecpostfixexpr;
                default:
                    break;
            }
            return nullptr;
        }

    public:
        RuntimeConfig* m_config;
        AstLexer m_lexer;
        ErrList* m_prserrlist;
        int m_parsedepth;

    public:
        AstParser(RuntimeConfig* config, ErrList* errors)
        {
            m_config = config;
            m_prserrlist = errors;
            m_parsedepth = 0;
        }

        bool parseVarLetStmt(AstExpression** res)
        {
            bool assignable;
            AstExpression* value;
            value = nullptr;
            assignable = m_lexer.currentTokenIs(AstToken::TOK_VAR);
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_IDENT))
            {
                return false;
            }
            auto nameident = AstExpression::ExprIdent(m_lexer.m_currtoken);
            m_lexer.nextToken();
            if(!m_lexer.currentTokenIs(AstToken::TOK_ASSIGN))
            {
                value = AstExpression::makeAstItemLiteralNull();
                goto finish;
            }
            m_lexer.nextToken();
            if(!parseExpression(&value, MC_ASTPREC_LOWEST))
            {
                goto err;
            }
            if(value->m_exprtype == AstExpression::EXPR_FUNCTIONLITERAL)
            {
                value->m_uexpr.exprlitfunction.name = mc_util_strdup(nameident.m_identvalue);
                if(value->m_uexpr.exprlitfunction.name == nullptr)
                {
                    goto err;
                }
            }
            finish:
            *res = AstExpression::makeAstItemDefine(nameident, value, assignable);
            return true;
        err:
            AstExpression::destroyExpression(value);
            nameident.deInit();
            return false;
        }

        bool parseIfStmt(AstExpression** res)
        {
            bool ok;
            bool havealt;
            AstExpression::ExprIfCase* cond;
            AstExpression::ExprIfCase* elif;
            AstExpression::ExprCodeBlock alternative;
            AstExpression::ExprCodeBlock emptyblocktop;
            (void)ok;
            havealt = false;
            GenericList<AstExpression::ExprIfCase*> cases(0, nullptr);
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_LPAREN))
            {
                goto err;
            }
            m_lexer.nextToken();
            cond = Memory::make<AstExpression::ExprIfCase>(nullptr, emptyblocktop);
            ok = cases.push(cond);
            if(!parseExpression(&cond->m_ifcond, MC_ASTPREC_LOWEST))
            {
                goto err;
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
            {
                goto err;
            }
            m_lexer.nextToken();
            if(!parseCodeBlock(&cond->m_ifthen))
            {
                goto err;
            }
            while(m_lexer.currentTokenIs(AstToken::TOK_ELSE))
            {
                AstExpression::ExprCodeBlock emptyblockinner;
                m_lexer.nextToken();
                if(m_lexer.currentTokenIs(AstToken::TOK_IF))
                {
                    m_lexer.nextToken();
                    if(!m_lexer.expectCurrent(AstToken::TOK_LPAREN))
                    {
                        goto err;
                    }
                    m_lexer.nextToken();
                    elif = Memory::make<AstExpression::ExprIfCase>(nullptr, emptyblockinner);
                    ok = cases.push(elif);
                    if(!parseExpression(&elif->m_ifcond, MC_ASTPREC_LOWEST))
                    {
                        goto err;
                    }
                    if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
                    {
                        goto err;
                    }
                    m_lexer.nextToken();
                    if(!parseCodeBlock(&elif->m_ifthen))
                    {
                        goto err;
                    }
                }
                else
                {
                    if(!parseCodeBlock(&alternative))
                    {
                        goto err;
                    }
                    havealt = true;
                }
            }
            *res = AstExpression::makeAstItemIfStmt(cases, alternative, havealt);
            return true;
        err:
            cases.deInit(AstExpression::ExprIfCase::destroy);
            alternative.deInit();
            return false;
        }

        bool parseReturnStmt(AstExpression** res)
        {
            AstExpression* expr;
            expr = nullptr;
            m_lexer.nextToken();
            if(!m_lexer.currentTokenIs(AstToken::TOK_SEMICOLON) && !m_lexer.currentTokenIs(AstToken::TOK_RBRACE) && !m_lexer.currentTokenIs(AstToken::TOK_EOF))
            {
                if(!parseExpression(&expr, MC_ASTPREC_LOWEST))
                {
                    return false;
                }
            }
            *res = AstExpression::makeAstItemReturnStmt(expr);
            return true;
        }

        bool parseExprStmt(AstExpression** res)
        {
            AstExpression* expr;
            if(!parseExpression(&expr, MC_ASTPREC_LOWEST))
            {
                return false;
            }
            if((expr != nullptr) && (!m_config->replmode || m_parsedepth > 0))
            {
            }
            *res = AstExpression::makeAstItemExprStmt(expr);
            return true;
        }

        bool parseLoopWhileStmt(AstExpression** res)
        {
            AstExpression* test;
            AstExpression::ExprCodeBlock body;
            test = nullptr;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_LPAREN))
            {
                goto err;
            }
            m_lexer.nextToken();
            if(!parseExpression(&test, MC_ASTPREC_LOWEST))
            {
                goto err;
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
            {
                goto err;
            }
            m_lexer.nextToken();
            if(!parseCodeBlock(&body))
            {
                goto err;
            }
            *res = AstExpression::makeAstItemWhileStmt(test, body);
            return true;
        err:
            body.deInit();
            AstExpression::destroyExpression(test);
            return false;
        }

        bool parseBreakStmt(AstExpression** res)
        {
            m_lexer.nextToken();
            auto expr = AstExpression::makeAstItemBreakStmt();
            if(expr == nullptr)
            {
                return false;
            }
            *res = expr;
            return true;
        }

        bool parseContinueStmt(AstExpression** res)
        {
            m_lexer.nextToken();
            auto expr = AstExpression::makeAstItemContinueStmt();
            if(expr == nullptr)
            {
                return false;
            }
            *res = expr;
            return true;
        }

        bool parseBlockStmt(AstExpression** res)
        {
            AstExpression::ExprCodeBlock block;
            if(!parseCodeBlock(&block))
            {
                return false;
            }
            *res = AstExpression::makeAstItemBlockStmt(block);
            return true;
        }

        bool parseImportStmt(AstExpression** res)
        {
            char* processedname;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_STRING))
            {
                return false;
            }
            processedname = processAndCopyString(m_lexer.m_currtoken.m_tokstrdata, m_lexer.m_currtoken.m_tokstrlength);
            if(processedname == nullptr)
            {
                m_prserrlist->pushFormat(Error::ERRTYP_PARSING, m_lexer.m_currtoken.m_tokpos, "error when parsing module name");
                return false;
            }
            m_lexer.nextToken();
            *res = AstExpression::makeAstItemImportExpr(processedname);
            return true;
        }

        bool parseRecoverStmt(AstExpression** res)
        {
            AstExpression::ExprCodeBlock body;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_LPAREN))
            {
                return false;
            }
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_IDENT))
            {
                return false;
            }
            auto eid = AstExpression::ExprIdent(m_lexer.m_currtoken);
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
            {
                goto err;
            }
            m_lexer.nextToken();
            if(!parseCodeBlock(&body))
            {
                goto err;
            }
            *res = AstExpression::makeAstItemRecover(eid, body);
            return true;
        err:
            body.deInit();
            eid.deInit();
            return false;
        }

        bool parseLoopForBaseStmt(AstExpression** res)
        {
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_LPAREN))
            {
                return false;
            }
            m_lexer.nextToken();
            if(m_lexer.currentTokenIs(AstToken::TOK_IDENT) && m_lexer.peekTokenIs(AstToken::TOK_IN))
            {
                return parseLoopForeachStmt(res);
            }
            return parseLoopForClassicStmt(res);
        }

        bool parseLoopForeachStmt(AstExpression** res)
        {
            AstExpression* source;
            AstExpression::ExprCodeBlock body;
            source = nullptr;
            auto iteratorident = AstExpression::ExprIdent(m_lexer.m_currtoken);
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_IN))
            {
                goto err;
            }
            m_lexer.nextToken();
            if(!parseExpression(&source, MC_ASTPREC_LOWEST))
            {
                goto err;
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
            {
                goto err;
            }
            m_lexer.nextToken();
            if(!parseCodeBlock(&body))
            {
                goto err;
            }
            *res = AstExpression::makeAstItemForeachStmt(iteratorident, source, body);
            return true;
        err:
            body.deInit();
            iteratorident.deInit();
            AstExpression::destroyExpression(source);
            return false;
        }

        bool parseLoopForClassicStmt(AstExpression** res)
        {
            AstExpression* init;
            AstExpression* test;
            AstExpression* update;
            AstExpression::ExprCodeBlock body;
            init = nullptr;
            test = nullptr;
            update = nullptr;
            if(!m_lexer.currentTokenIs(AstToken::TOK_SEMICOLON))
            {
                if(!parseStatement(&init))
                {
                    goto err;
                }
                if(init->m_exprtype != AstExpression::EXPR_STMTDEFINE && init->m_exprtype != AstExpression::EXPR_STMTEXPRESSION)
                {
                    m_prserrlist->pushFormat(Error::ERRTYP_PARSING, init->m_exprpos, "expected a definition or expression as 'for' loop init clause");
                    goto err;
                }
                if(!m_lexer.expectCurrent(AstToken::TOK_SEMICOLON))
                {
                    goto err;
                }
            }
            m_lexer.nextToken();
            if(!m_lexer.currentTokenIs(AstToken::TOK_SEMICOLON))
            {
                if(!parseExpression(&test, MC_ASTPREC_LOWEST))
                {
                    goto err;
                }
                if(!m_lexer.expectCurrent(AstToken::TOK_SEMICOLON))
                {
                    goto err;
                }
            }
            m_lexer.nextToken();
            if(!m_lexer.currentTokenIs(AstToken::TOK_RPAREN))
            {
                if(!parseExpression(&update, MC_ASTPREC_LOWEST))
                {
                    goto err;
                }
                if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
                {
                    goto err;
                }
            }
            m_lexer.nextToken();
            if(!parseCodeBlock(&body))
            {
                goto err;
            }
            *res = AstExpression::makeAstItemForLoopStmt(init, test, update, body);
            return true;
        err:
            AstExpression::destroyExpression(init);
            AstExpression::destroyExpression(test);
            AstExpression::destroyExpression(update);
            body.deInit();
            return false;
        }

        bool parseCodeBlock(AstExpression::ExprCodeBlock* res)
        {
            bool ok;
            AstExpression* expr;
            (void)ok;
            if(!m_lexer.expectCurrent(AstToken::TOK_LBRACE))
            {
                return false;
            }
            m_lexer.nextToken();
            m_parsedepth++;
            GenericList<AstExpression*> statements(0, nullptr);
            while(!m_lexer.currentTokenIs(AstToken::TOK_RBRACE))
            {
                if(m_lexer.currentTokenIs(AstToken::TOK_EOF))
                {
                    m_prserrlist->pushFormat(Error::ERRTYP_PARSING, m_lexer.m_currtoken.m_tokpos, "unexpected EOF");
                    goto err;
                }
                if(m_lexer.currentTokenIs(AstToken::TOK_SEMICOLON))
                {
                    m_lexer.nextToken();
                    continue;
                }
                if(!parseStatement(&expr))
                {
                    goto err;
                }
                ok = statements.push(expr);
            }
            m_lexer.nextToken();
            m_parsedepth--;
            *res = AstExpression::ExprCodeBlock(statements);
            return true;
        err:
            m_parsedepth--;
            statements.deInit(AstExpression::destroyExpression);
            return false;
        }

        bool parseExpression(AstExpression** res, mcastprecedence_t prec)
        {
            char* literal;
            AstLocation pos;
            mcastleftassocparsefn_t parseleftassoc;
            mcastrightassocparsefn_t parserightassoc;
            AstExpression* newleftexpr;
            AstExpression* leftexpr;
            pos = m_lexer.m_currtoken.m_tokpos;
            if(m_lexer.m_currtoken.type() == AstToken::TOK_INVALID)
            {
                m_prserrlist->pushFormat(Error::ERRTYP_PARSING, m_lexer.m_currtoken.m_tokpos, "illegal token");
                return false;
            }
            parserightassoc = getRightAssocParseFunc(m_lexer.m_currtoken.type());
            if(parserightassoc == nullptr)
            {
                literal = m_lexer.m_currtoken.dupLiteralString();
                m_prserrlist->pushFormat(Error::ERRTYP_PARSING, m_lexer.m_currtoken.m_tokpos, "no prefix parse function for \"%s\" found", literal);
                mc_memory_free(literal);
                return false;
            }
            if(!parserightassoc(this, &leftexpr))
            {
                return false;
            }
            leftexpr->m_exprpos = pos;
            while(!m_lexer.currentTokenIs(AstToken::TOK_SEMICOLON) && prec < getPrecedence(m_lexer.m_currtoken.type()))
            {
                parseleftassoc = getLeftAssocParseFunc(m_lexer.m_currtoken.m_toktype);
                if(parseleftassoc == nullptr)
                {
                    *res = leftexpr;
                    return true;
                }
                pos = m_lexer.m_currtoken.m_tokpos;
                if(!parseleftassoc(this, &newleftexpr, leftexpr))
                {
                    AstExpression::destroyExpression(leftexpr);
                    return false;
                }
                newleftexpr->m_exprpos = pos;
                leftexpr = newleftexpr;
            }
            *res = leftexpr;
            return true;
        }

        bool parseGroupedExpr(AstExpression** res)
        {
            AstExpression* expr;
            m_lexer.nextToken();
            if(!parseExpression(&expr, MC_ASTPREC_LOWEST) || !m_lexer.expectCurrent(AstToken::TOK_RPAREN))
            {
                AstExpression::destroyExpression(expr);
                return false;
            }
            m_lexer.nextToken();
            *res = expr;
            return true;
        }

        bool parseFuncParams(GenericList<AstExpression::ExprFuncParam*>* outparams)
        {
            bool ok;
            (void)ok;
            if(!m_lexer.expectCurrent(AstToken::TOK_LPAREN))
            {
                return false;
            }
            m_lexer.nextToken();
            if(m_lexer.currentTokenIs(AstToken::TOK_RPAREN))
            {
                m_lexer.nextToken();
                return true;
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_IDENT))
            {
                return false;
            }
            auto ident = AstExpression::ExprIdent(m_lexer.m_currtoken);
            auto param = Memory::make<AstExpression::ExprFuncParam>(ident);
            ok = outparams->push(param);
            m_lexer.nextToken();
            while(m_lexer.currentTokenIs(AstToken::TOK_COMMA))
            {
                m_lexer.nextToken();
                if(!m_lexer.expectCurrent(AstToken::TOK_IDENT))
                {
                    return false;
                }
                ident = AstExpression::ExprIdent(m_lexer.m_currtoken);
                param = Memory::make<AstExpression::ExprFuncParam>(ident);
                ok = outparams->push(param);
                m_lexer.nextToken();
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
            {
                return false;
            }
            m_lexer.nextToken();
            return true;
        }

        bool parseFunctionStmt(AstExpression** res)
        {
            AstExpression* value;
            AstLocation pos;
            value = nullptr;
            pos = m_lexer.m_currtoken.m_tokpos;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_IDENT))
            {
                return false;
            }
            auto nameident = AstExpression::ExprIdent(m_lexer.m_currtoken);
            m_lexer.nextToken();
            if(!callback_parseliteralfunction(this, &value))
            {
                goto err;
            }
            value->m_exprpos = pos;
            value->m_uexpr.exprlitfunction.name = mc_util_strdup(nameident.m_identvalue);
            if(value->m_uexpr.exprlitfunction.name == nullptr)
            {
                goto err;
            }
            *res = AstExpression::makeAstItemDefine(nameident, value, false);
            return true;
        err:
            AstExpression::destroyExpression(value);
            nameident.deInit();
            return false;
        }

        bool parseTernaryExpr(AstExpression** res, AstExpression* left)
        {
            AstExpression* ift;
            AstExpression* iffalse;
            m_lexer.nextToken();
            if(!parseExpression(&ift, MC_ASTPREC_LOWEST))
            {
                return false;
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_COLON))
            {
                AstExpression::destroyExpression(ift);
                return false;
            }
            m_lexer.nextToken();
            if(!parseExpression(&iffalse, MC_ASTPREC_LOWEST))
            {
                AstExpression::destroyExpression(ift);
                return false;
            }
            *res = AstExpression::makeAstItemTernary(left, ift, iffalse);
            return true;
        }

        bool parseLogicalExpr(AstExpression** res, AstExpression* left)
        {
            AstExpression::MathOpType op;
            mcastprecedence_t prec;
            AstExpression* right;
            op = tokenToMathOP(m_lexer.m_currtoken.m_toktype);
            prec = getPrecedence(m_lexer.m_currtoken.m_toktype);
            m_lexer.nextToken();
            if(!parseExpression(&right, prec))
            {
                return false;
            }
            *res = AstExpression::makeAstItemLogical(op, left, right);
            return true;
        }

        bool parseIndexExpr(AstExpression** res, AstExpression* left)
        {
            AstExpression* index;
            m_lexer.nextToken();
            if(!parseExpression(&index, MC_ASTPREC_LOWEST))
            {
                return false;
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_RBRACKET))
            {
                AstExpression::destroyExpression(index);
                return false;
            }
            m_lexer.nextToken();
            *res = AstExpression::makeAstItemIndex(left, index, false);
            return true;
        }

        bool parseAssignExpr(AstExpression** res, AstExpression* left)
        {
            AstLocation pos;
            AstExpression::MathOpType op;
            AstToken::Type assigntype;
            AstExpression* source;
            AstExpression* leftcopy;
            AstExpression* newsource;
            source = nullptr;
            assigntype = m_lexer.m_currtoken.m_toktype;
            m_lexer.nextToken();
            if(!parseExpression(&source, MC_ASTPREC_LOWEST))
            {
                goto err;
            }
            switch(assigntype)
            {
                case AstToken::TOK_ASSIGNPLUS:
                case AstToken::TOK_ASSIGNMINUS:
                case AstToken::TOK_ASSIGNSLASH:
                case AstToken::TOK_ASSIGNASTERISK:
                case AstToken::TOK_ASSIGNPERCENT:
                case AstToken::TOK_ASSIGNBINAND:
                case AstToken::TOK_ASSIGNBINOR:
                case AstToken::TOK_ASSIGNBINXOR:
                case AstToken::TOK_ASSIGNLSHIFT:
                case AstToken::TOK_ASSIGNRSHIFT:
                    {
                        op = tokenToMathOP(assigntype);
                        leftcopy = AstExpression::copyExpression(left);
                        if(leftcopy == nullptr)
                        {
                            goto err;
                        }
                        pos = source->m_exprpos;
                        newsource = AstExpression::makeAstItemInfix(op, leftcopy, source);
                        newsource->m_exprpos = pos;
                        source = newsource;
                    }
                    break;
                case AstToken::TOK_ASSIGN:
                    {
                    }
                    break;
                default:
                    {
                        MC_ASSERT(false);
                    }
                    break;
            }
            *res = AstExpression::makeAstItemAssign(left, source, false);
            return true;
        err:
            AstExpression::destroyExpression(source);
            return false;
        }

        bool parseIncDecPrefixExpr(AstExpression** res)
        {
            AstLocation pos;
            AstExpression::MathOpType op;
            AstToken::Type operationtype;
            AstExpression* dest;
            AstExpression* source;
            AstExpression* destcopy;
            AstExpression* operation;
            AstExpression* oneliteral;
            source = nullptr;
            operationtype = m_lexer.m_currtoken.m_toktype;
            pos = m_lexer.m_currtoken.m_tokpos;
            m_lexer.nextToken();
            op = tokenToMathOP(operationtype);
            if(!parseExpression(&dest, MC_ASTPREC_PREFIX))
            {
                goto err;
            }
            oneliteral = AstExpression::makeAstItemLiteralNumber(1);
            oneliteral->m_exprpos = pos;
            destcopy = AstExpression::copyExpression(dest);
            if(destcopy == nullptr)
            {
                AstExpression::destroyExpression(oneliteral);
                AstExpression::destroyExpression(dest);
                goto err;
            }
            operation = AstExpression::makeAstItemInfix(op, destcopy, oneliteral);
            operation->m_exprpos = pos;
            *res = AstExpression::makeAstItemAssign(dest, operation, false);
            return true;
        err:
            AstExpression::destroyExpression(source);
            return false;
        }

        bool parseIncDecPostfixExpr(AstExpression** res, AstExpression* left)
        {
            AstLocation pos;
            AstExpression::MathOpType op;
            AstToken::Type operationtype;
            AstExpression* source;
            AstExpression* leftcopy;
            AstExpression* operation;
            AstExpression* oneliteral;
            source = nullptr;
            operationtype = m_lexer.m_currtoken.m_toktype;
            pos = m_lexer.m_currtoken.m_tokpos;
            m_lexer.nextToken();
            op = tokenToMathOP(operationtype);
            leftcopy = AstExpression::copyExpression(left);
            if(leftcopy == nullptr)
            {
                goto err;
            }
            oneliteral = AstExpression::makeAstItemLiteralNumber(1);
            oneliteral->m_exprpos = pos;
            operation = AstExpression::makeAstItemInfix(op, leftcopy, oneliteral);
            operation->m_exprpos = pos;
            *res = AstExpression::makeAstItemAssign(left, operation, true);
            return true;
        err:
            AstExpression::destroyExpression(source);
            return false;
        }

        bool parsePrefixExpr(AstExpression** res)
        {
            AstExpression::MathOpType op;
            AstExpression* right;
            op = tokenToMathOP(m_lexer.m_currtoken.m_toktype);
            m_lexer.nextToken();
            if(!parseExpression(&right, MC_ASTPREC_PREFIX))
            {
                return false;
            }
            *res = AstExpression::makeAstItemPrefix(op, right);
            return true;
        }

        bool parseInfixExpr(AstExpression** res, AstExpression* left)
        {
            AstExpression::MathOpType op;
            mcastprecedence_t prec;
            AstExpression* right;
            op = tokenToMathOP(m_lexer.m_currtoken.m_toktype);
            prec = getPrecedence(m_lexer.m_currtoken.m_toktype);
            m_lexer.nextToken();
            if(!parseExpression(&right, prec))
            {
                return false;
            }
            *res = AstExpression::makeAstItemInfix(op, left, right);
            return true;
        }

        bool parseLiteralFunction(AstExpression** res)
        {
            bool ok;
            AstExpression::ExprCodeBlock body;
            (void)ok;
            m_parsedepth++;
            if(m_lexer.currentTokenIs(AstToken::TOK_FUNCTION))
            {
                m_lexer.nextToken();
            }
            GenericList<AstExpression::ExprFuncParam*> params(0, nullptr);
            ok = parseFuncParams(&params);
            if(!ok)
            {
                goto err;
            }
            if(!parseCodeBlock(&body))
            {
                goto err;
            }
            *res = AstExpression::makeAstItemLiteralFunction(params, body);
            m_parsedepth -= 1;
            return true;
        err:
            body.deInit();
            params.deInit(AstExpression::ExprFuncParam::destroy);
            m_parsedepth -= 1;
            return false;
        }

        bool parseLiteralArray(AstExpression** res)
        {
            GenericList<AstExpression*> array;
            if(!parseExprList(&array, AstToken::TOK_LBRACKET, AstToken::TOK_RBRACKET, true))
            {
                return false;
            }
            *res = AstExpression::makeAstItemLiteralArray(array);
            return true;
        }

        bool parseLiteralMap(AstExpression** res)
        {
            bool ok;
            size_t len;
            char* str;
            AstExpression* key;
            AstExpression* value;
            (void)ok;
            GenericList<AstExpression*> keys;
            GenericList<AstExpression*> values;
            m_lexer.nextToken();
            while(!m_lexer.currentTokenIs(AstToken::TOK_RBRACE))
            {
                key = nullptr;
                if(m_lexer.currentTokenIs(AstToken::TOK_IDENT))
                {
                    str = m_lexer.m_currtoken.dupLiteralString();
                    len = mc_util_strlen(str);
                    key = AstExpression::makeAstItemLiteralString(str, len);
                    key->m_exprpos = m_lexer.m_currtoken.m_tokpos;
                    m_lexer.nextToken();
                }
                else
                {
                    if(!parseExpression(&key, MC_ASTPREC_LOWEST))
                    {
                        goto err;
                    }
                    switch(key->m_exprtype)
                    {
                        case AstExpression::EXPR_STRINGLITERAL:
                        case AstExpression::EXPR_NUMBERLITERAL:
                        case AstExpression::EXPR_BOOLLITERAL:
                            {
                            }
                            break;
                        default:
                            {
                                m_prserrlist->pushFormat(Error::ERRTYP_PARSING, key->m_exprpos, "can only use primitive types as literal 'map' object keys");
                                AstExpression::destroyExpression(key);
                                goto err;
                            }
                            break;
                    }
                }
                ok = keys.push(key);
                if(!m_lexer.expectCurrent(AstToken::TOK_COLON))
                {
                    goto err;
                }
                m_lexer.nextToken();
                if(!parseExpression(&value, MC_ASTPREC_LOWEST))
                {
                    goto err;
                }
                ok = values.push(value);
                if(m_lexer.currentTokenIs(AstToken::TOK_RBRACE))
                {
                    break;
                }
                if(!m_lexer.expectCurrent(AstToken::TOK_COMMA))
                {
                    goto err;
                }
                m_lexer.nextToken();
            }
            m_lexer.nextToken();
            *res = AstExpression::makeAstItemLiteralMap(keys, values);
            return true;
        err:
            keys.deInit(AstExpression::destroyExpression);
            values.deInit(AstExpression::destroyExpression);
            return false;
        }

        bool parseLiteralTemplateString(AstExpression** res)
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
            processedliteral = processAndCopyString(m_lexer.m_currtoken.m_tokstrdata, m_lexer.m_currtoken.m_tokstrlength);
            if(processedliteral == nullptr)
            {
                m_prserrlist->pushFormat(Error::ERRTYP_PARSING, m_lexer.m_currtoken.m_tokpos, "error parsing string literal");
                return false;
            }
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_LBRACE))
            {
                goto err;
            }
            m_lexer.nextToken();
            pos = m_lexer.m_currtoken.m_tokpos;
            len = mc_util_strlen(processedliteral);
            leftstringexpr = AstExpression::makeAstItemLiteralString(processedliteral, len);
            leftstringexpr->m_exprpos = pos;
            processedliteral = nullptr;
            pos = m_lexer.m_currtoken.m_tokpos;
            if(!parseExpression(&templateexpr, MC_ASTPREC_LOWEST))
            {
                goto err;
            }
            tostrcallexpr = AstExpression::makeAstItemInlineCall(templateexpr, "tostring");
            tostrcallexpr->m_exprpos = pos;
            templateexpr = nullptr;
            leftaddexpr = AstExpression::makeAstItemInfix(AstExpression::MATHOP_PLUS, leftstringexpr, tostrcallexpr);
            leftaddexpr->m_exprpos = pos;
            leftstringexpr = nullptr;
            tostrcallexpr = nullptr;
            if(!m_lexer.expectCurrent(AstToken::TOK_RBRACE))
            {
                goto err;
            }
            m_lexer.previousToken();
            m_lexer.conttplstring();
            m_lexer.nextToken();
            m_lexer.nextToken();
            pos = m_lexer.m_currtoken.m_tokpos;
            if(!parseExpression(&rightexpr, MC_ASTPREC_HIGHEST))
            {
                goto err;
            }
            rightaddexpr = AstExpression::makeAstItemInfix(AstExpression::MATHOP_PLUS, leftaddexpr, rightexpr);
            rightaddexpr->m_exprpos = pos;
            leftaddexpr = nullptr;
            rightexpr = nullptr;
            *res = rightaddexpr;
            return true;
        err:
            AstExpression::destroyExpression(rightaddexpr);
            AstExpression::destroyExpression(rightexpr);
            AstExpression::destroyExpression(leftaddexpr);
            AstExpression::destroyExpression(tostrcallexpr);
            AstExpression::destroyExpression(templateexpr);
            AstExpression::destroyExpression(leftstringexpr);
            mc_memory_free(processedliteral);
            return false;
        }

        bool parseLiteralString(AstExpression** res)
        {
            size_t len;
            char* processedliteral;
            processedliteral = processAndCopyString(m_lexer.m_currtoken.m_tokstrdata, m_lexer.m_currtoken.m_tokstrlength);
            if(processedliteral == nullptr)
            {
                m_prserrlist->pushFormat(Error::ERRTYP_PARSING, m_lexer.m_currtoken.m_tokpos, "error parsing string literal");
                return false;
            }
            m_lexer.nextToken();
            len = mc_util_strlen(processedliteral);
            *res = AstExpression::makeAstItemLiteralString(processedliteral, len);
            return true;
        }

        bool parseLiteralNull(AstExpression** res)
        {
            m_lexer.nextToken();
            *res = AstExpression::makeAstItemLiteralNull();
            return true;
        }

        bool parseLiteralBool(AstExpression** res)
        {
            *res = AstExpression::makeAstItemLiteralBool(m_lexer.m_currtoken.m_toktype == AstToken::TOK_TRUE);
            m_lexer.nextToken();
            return true;
        }

        bool parseLiteralNumber(AstExpression** res)
        {
            mcfloat_t number;
            long parsedlen;
            char* end;
            char* literal;
            number = 0;
            errno = 0;
            number = mc_util_strtod(m_lexer.m_currtoken.m_tokstrdata, m_lexer.m_currtoken.m_tokstrlength, &end);
            parsedlen = end - m_lexer.m_currtoken.m_tokstrdata;
            if(errno || parsedlen != m_lexer.m_currtoken.m_tokstrlength)
            {
                literal = m_lexer.m_currtoken.dupLiteralString();
                m_prserrlist->pushFormat(Error::ERRTYP_PARSING, m_lexer.m_currtoken.m_tokpos, "failed to parse number literal \"%s\"", literal);
                mc_memory_free(literal);
                return false;
            }    
            m_lexer.nextToken();
            *res = AstExpression::makeAstItemLiteralNumber(number);
            return true;
        }

        bool parseDotExpr(AstExpression** res, AstExpression* left)
        {
            size_t len;
            char* str;
            AstExpression* index;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_IDENT))
            {
                return false;
            }
            str = m_lexer.m_currtoken.dupLiteralString();
            len = mc_util_strlen(str);
            index = AstExpression::makeAstItemLiteralString(str, len);
            index->m_exprpos = m_lexer.m_currtoken.m_tokpos;
            m_lexer.nextToken();
            *res = AstExpression::makeAstItemIndex(left, index, true);
            return true;
        }
        
        bool parseIdent(AstExpression** res)
        {
            auto ident = AstExpression::ExprIdent(m_lexer.m_currtoken);
            *res = AstExpression::makeAstItemIdent(ident);
            m_lexer.nextToken();
            return true;
        }

        bool parseCallExpr(AstExpression** res, AstExpression* left)
        {
            AstExpression* function;
            function = left;
            GenericList<AstExpression*> args(0, nullptr);
            if(!parseExprList(&args, AstToken::TOK_LPAREN, AstToken::TOK_RPAREN, false))
            {
                return false;
            }
            *res = AstExpression::makeAstItemCallExpr(function, args);
            return true;
        }

        bool parseExprList(GenericList<AstExpression*>* res, AstToken::Type starttoken, AstToken::Type endtoken, bool trailingcommaallowed)
        {
            bool ok;
            AstExpression* argexpr;
            (void)ok;
            if(!m_lexer.expectCurrent(starttoken))
            {
                return false;
            }
            m_lexer.nextToken();
            if(m_lexer.currentTokenIs(endtoken))
            {
                m_lexer.nextToken();
                return true;
            }
            if(!parseExpression(&argexpr, MC_ASTPREC_LOWEST))
            {
                goto err;
            }
            ok = res->push(argexpr);
            while(m_lexer.currentTokenIs(AstToken::TOK_COMMA))
            {
                m_lexer.nextToken();
                if(trailingcommaallowed && m_lexer.currentTokenIs(endtoken))
                {
                    break;
                }
                if(!parseExpression(&argexpr, MC_ASTPREC_LOWEST))
                {
                    goto err;
                }
                ok = res->push(argexpr);
            }
            if(!m_lexer.expectCurrent(endtoken))
            {
                goto err;
            }
            m_lexer.nextToken();
            return true;
        err:
            //Memory::destroy(res, AstExpression::destroyExpression);
            return false;
        }

        bool parseStatement(AstExpression** res)
        {
            AstLocation pos;
            AstExpression* expr;
            pos = m_lexer.m_currtoken.m_tokpos;
            expr = nullptr;
            switch(m_lexer.m_currtoken.m_toktype)
            {
                case AstToken::TOK_VAR:
                case AstToken::TOK_CONST:
                    {
                        if(!parseVarLetStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::TOK_IF:
                    {
                        if(!parseIfStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::TOK_RETURN:
                    {
                        if(!parseReturnStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::TOK_WHILE:
                    {
                        if(!parseLoopWhileStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::TOK_BREAK:
                    {
                        if(!parseBreakStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::TOK_FOR:
                    {
                        if(!parseLoopForBaseStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::TOK_FUNCTION:
                    {
                        if(m_lexer.peekTokenIs(AstToken::TOK_IDENT))
                        {
                            if(!parseFunctionStmt(&expr))
                            {
                                return false;
                            }
                        }
                        else
                        {
                            if(!parseExprStmt(&expr))
                            {
                                return false;
                            }
                        }
                    }
                    break;
                case AstToken::TOK_LBRACE:
                    {
                        if(m_config->replmode && m_parsedepth == 0)
                        {
                            if(!parseExprStmt(&expr))
                            {
                                return false;
                            }
                        }
                        else
                        {
                            if(!parseBlockStmt(&expr))
                            {
                                return false;
                            }
                        }
                    }
                    break;
                case AstToken::TOK_CONTINUE:
                    {
                        if(!parseContinueStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::TOK_IMPORT:
                    {
                        if(!parseImportStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::TOK_RECOVER:
                    {
                        if(!parseRecoverStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                default:
                    {
                        if(!parseExprStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
            }
            if(expr != nullptr)
            {
                expr->m_exprpos = pos;
            }
            *res = expr;
            return true;
        }

        bool parseAll(GenericList<AstExpression*>* statements, const char* input, AstSourceFile* file)
        {
            bool ok;
            AstExpression* expr;
            (void)ok;
            m_parsedepth = 0;
            ok = AstLexer::init(&m_lexer, m_prserrlist, input, file);
            if(!ok)
            {
                return false;
            }
            m_lexer.nextToken();
            m_lexer.nextToken();
            while(!m_lexer.currentTokenIs(AstToken::TOK_EOF))
            {
                if(m_lexer.currentTokenIs(AstToken::TOK_SEMICOLON))
                {
                    m_lexer.nextToken();
                    continue;
                }
                if(!parseStatement(&expr))
                {
                    goto err;
                }
                ok = statements->push(expr);
            }
            if(m_prserrlist->count() > 0)
            {
                goto err;
            }
            return true;
        err:
            statements->deInit(AstExpression::destroyExpression);
            return false;
        }
};

class AstPrinter
{
    public:
        static const char* getMathOpString(AstExpression::MathOpType op)
        {
            switch(op)
            {
                case AstExpression::MATHOP_NONE:
                    return "AstExpression::MATHOP_NONE";
                case AstExpression::MATHOP_ASSIGN:
                    return "=";
                case AstExpression::MATHOP_PLUS:
                    return "+";
                case AstExpression::MATHOP_MINUS:
                    return "-";
                case AstExpression::MATHOP_BANG:
                    return "!";
                case AstExpression::MATHOP_ASTERISK:
                    return "*";
                case AstExpression::MATHOP_SLASH:
                    return "/";
                case AstExpression::MATHOP_LT:
                    return "<";
                case AstExpression::MATHOP_GT:
                    return ">";
                case AstExpression::MATHOP_EQ:
                    return "==";
                case AstExpression::MATHOP_NOTEQ:
                    return "!=";
                case AstExpression::MATHOP_MODULUS:
                    return "%";
                case AstExpression::MATHOP_LOGICALAND:
                    return "&&";
                case AstExpression::MATHOP_LOGICALOR:
                    return "||";
                case AstExpression::MATHOP_BINAND:
                    return "&";
                case AstExpression::MATHOP_BINOR:
                    return "|";
                case AstExpression::MATHOP_BINXOR:
                    return "^";
                case AstExpression::MATHOP_LSHIFT:
                    return "<<";
                case AstExpression::MATHOP_RSHIFT:
                    return ">>";
                case AstExpression::MATHOP_LTE:
                    return "<=";
                case AstExpression::MATHOP_GTE:
                    return ">=";
                case AstExpression::MATHOP_BINNOT:
                    return "~";
            }
            return "AstExpression::MATHOP_UNKNOWN";
        }


    public:
        Printer* m_pdest;
        bool m_pseudolisp;

    public:
        AstPrinter(Printer* pr)
        {
            m_pseudolisp = false;
            m_pdest = pr;
        }

        void putl(const char* str, size_t len)
        {
            m_pdest->put(str, len);
        }

        template<size_t len>
        void put(const char (&str)[len])
        {
            m_pdest->put(str, len);
        }

        void put(const char* str)
        {
            m_pdest->put(str, strlen(str));
        }

        template<typename... ArgsT>
        void putfmt(const char* fmt, ArgsT&&... args)
        {
            m_pdest->format(fmt, args...);
        }

        void beginPrint(GenericList<AstExpression*>* statements)
        {
            m_pdest->m_prconfig.quotstring = true;
            fprintf(stderr, "---AST dump begin---\n");
            printStmtList(statements);
            fprintf(stderr, "\n---AST dump end---\n");
            m_pdest->m_prconfig.quotstring = false;
        }

        void printStmtList(GenericList<AstExpression*>* statements)
        {
            int i;
            int count;
            count = statements->count();
            for(i = 0; i < count; i++)
            {
                auto subex = statements->get(i);
                printExpression(subex);
                if(i < (count - 1))
                {
                    put("\n");
                }
            }
        }

        void printFuncLiteral(AstExpression* astexpr)
        {
            size_t i;
            auto ex = &astexpr->m_uexpr.exprlitfunction;
            if(m_pseudolisp)
            {
                putfmt("(deffunction '(");
            }
            else
            {
                put("function(");
            }
            for(i = 0; i < ex->funcparamlist.count(); i++)
            {
                auto param = ex->funcparamlist.get(i);
                put(param->m_paramident.m_identvalue);
                if(i < (ex->funcparamlist.count() - 1))
                {
                    put(", ");
                }
            }
            put(") ");
            printCodeblock(&ex->body);
        }

        void printCall(AstExpression* astexpr)
        {
            size_t i;
            auto ex = &astexpr->m_uexpr.exprcall;
            printExpression(ex->function);
            put("(");
            for(i = 0; i < ex->m_callargs.count(); i++)
            {
                auto arg = ex->m_callargs.get(i);
                printExpression(arg);
                if(i < (ex->m_callargs.count() - 1))
                {
                    put(", ");
                }
            }
            put(")");
        }

        void printArrayLiteral(AstExpression* astexpr)
        {
            size_t i;
            size_t len;
            auto ex = &astexpr->m_uexpr.exprlitarray;
            auto vl = &ex->m_litarritems;
            len = vl->count();
            put("[");
            for(i = 0; i < len; i++)
            {
                auto itemex = vl->get(i);
                printExpression(itemex);
                if(i < (len - 1))
                {
                    put(", ");
                }
            }
            put("]");
        }

        void printStringLiteral(AstExpression* astexpr)
        {
            size_t slen;
            const char* sdata;
            auto ex = &astexpr->m_uexpr.exprlitstring;
            sdata = ex->m_strexprdata;
            slen = ex->m_strexprlength;
            if(m_pdest->m_prconfig.quotstring)
            {
                m_pdest->printEscapedString(sdata, slen);
            }
            else
            {
                putl(sdata, slen);
            }
        }

        void printMapLiteral(AstExpression* astexpr)
        {
            size_t i;
            auto ex = &astexpr->m_uexpr.exprlitmap;
            put("{");
            for(i = 0; i < ex->m_litmapkeys.count(); i++)
            {
                auto keyexpr = ex->m_litmapkeys.get(i);
                auto valexpr = ex->m_litmapvalues.get(i);
                printExpression(keyexpr);
                put(" : ");
                printExpression(valexpr);
                if(i < (ex->m_litmapkeys.count() - 1))
                {
                    put(", ");
                }
            }
            put("}");
        }

        void printPrefix(AstExpression* astexpr)
        {
            auto ex = &astexpr->m_uexpr.exprprefix;
            put("(");
            put(getMathOpString(ex->op));
            printExpression(ex->right);
            put(")");
        }

        void printInfix(AstExpression* astexpr)
        {
            auto ex = &astexpr->m_uexpr.exprinfix;
            put("(");
            printExpression(ex->left);
            put(" ");
            put(getMathOpString(ex->op));
            put(" ");
            printExpression(ex->right);
            put(")");
        }

        void printIndex(AstExpression* astexpr)
        {
            bool prevquot;
            auto ex = &astexpr->m_uexpr.exprindex;
            put("(");
            printExpression(ex->left);
            if(ex->isdot)
            {
                put(".");
                prevquot = m_pdest->m_prconfig.quotstring;
                m_pdest->m_prconfig.quotstring = false;
                printExpression(ex->index);
                m_pdest->m_prconfig.quotstring = prevquot;
            }
            else
            {
                put("[");
                printExpression(ex->index);
                put("]");
            }
            put(")");
        }

        void printAssign(AstExpression* astexpr)
        {
            auto ex = &astexpr->m_uexpr.exprassign;
            printExpression(ex->dest);
            put(" = ");
            printExpression(ex->source);
        }

        void printLogical(AstExpression* astexpr)
        {
            auto ex = &astexpr->m_uexpr.exprlogical;
            printExpression(ex->left);
            put(" ");
            put(getMathOpString(ex->op));
            put(" ");
            printExpression(ex->right);
        }

        void printTernary(AstExpression* astexpr)
        {
            auto ex = &astexpr->m_uexpr.exprternary;
            printExpression(ex->tercond);
            put(" ? ");
            printExpression(ex->teriftrue);
            put(" : ");
            printExpression(ex->teriffalse);
        }

        void printDefine(AstExpression* astexpr)
        {
            auto ex = &astexpr->m_uexpr.exprdefine;
            if(ex->assignable)
            {
                put("var ");
            }
            else
            {
                put("const ");
            }
            put(ex->name.m_identvalue);
            put(" = ");
            if(ex->value != nullptr)
            {
                printExpression(ex->value);
            }
        }

        void printIf(AstExpression* astexpr)
        {
            size_t i;
            auto ex = &astexpr->m_uexpr.exprifstmt;
            auto ifcase = ex->m_ifcases.get(0);
            put("if (");
            printExpression(ifcase->m_ifcond);
            put(") ");
            printCodeblock(&ifcase->m_ifthen);
            for(i = 1; i < ex->m_ifcases.count(); i++)
            {
                auto elifcase = ex->m_ifcases.get(i);
                put(" elif (");
                printExpression(elifcase->m_ifcond);
                put(") ");
                printCodeblock(&elifcase->m_ifthen);
            }
            if(ex->m_haveifstmtelsestmt)
            {
                put(" else ");
                printCodeblock(&ex->m_ifstmtelsestmt);
            }
        }

        void printWhile(AstExpression* astexpr)
        {
            auto ex = &astexpr->m_uexpr.exprwhileloopstmt;
            put("while (");
            printExpression(ex->loopcond);
            put(")");
            printCodeblock(&ex->body);
        }

        void printForClassic(AstExpression* astexpr)
        {
            auto ex = &astexpr->m_uexpr.exprforloopstmt;
            put("for (");
            if(ex->init != nullptr)
            {
                printExpression(ex->init);
                put(" ");
            }
            else
            {
                put(";");
            }
            if(ex->loopcond != nullptr)
            {
                printExpression(ex->loopcond);
                put("; ");
            }
            else
            {
                put(";");
            }
            if(ex->update != nullptr)
            {
                printExpression(ex->update);
            }
            put(")");
            printCodeblock(&ex->body);
        }

        void printForeach(AstExpression* astexpr)
        {
            auto ex = &astexpr->m_uexpr.exprforeachloopstmt;
            put("for (");
            putfmt("%s", ex->iterator.m_identvalue);
            put(" in ");
            printExpression(ex->source);
            put(")");
            printCodeblock(&ex->body);
        }

        void printImport(AstExpression* astexpr)
        {
            auto ex = &astexpr->m_uexpr.exprimportstmt;
            putfmt("import \"%s\"", ex->path);
        }

        void printRecover(AstExpression* astexpr)
        {
            auto ex = &astexpr->m_uexpr.exprrecoverstmt;
            putfmt("recover (%s)", ex->errident.m_identvalue);
            printCodeblock(&ex->body);
        }

        void printCodeblock(AstExpression::ExprCodeBlock* blockexpr)
        {
            size_t i;
            size_t cnt;
            cnt = blockexpr->m_blockstatements.count();
            put("{ ");
            for(i = 0; i < cnt; i++)
            {
                auto istmt = blockexpr->m_blockstatements.get(i);
                printExpression(istmt);
                put("\n");
            }
            put(" }");
        }

        void printExpression(AstExpression* astexpr)
        {
            switch(astexpr->m_exprtype)
            {
                case AstExpression::EXPR_IDENT:
                    {
                        auto ex = &astexpr->m_uexpr.exprident;
                        put(ex->m_identvalue);
                    }
                    break;
                case AstExpression::EXPR_NUMBERLITERAL:
                    {
                        auto fl = astexpr->m_uexpr.exprlitnumber;
                        putfmt("%1.17g", fl);
                    }
                    break;
                case AstExpression::EXPR_BOOLLITERAL:
                    {
                        auto bl = astexpr->m_uexpr.exprlitbool;
                        putfmt("%s", bl ? "true" : "false");
                    }
                    break;
                case AstExpression::EXPR_STRINGLITERAL:
                    {
                        printStringLiteral(astexpr);
                    }
                    break;
                case AstExpression::EXPR_NULLLITERAL:
                    {
                        put("null");
                    }
                    break;
                case AstExpression::EXPR_ARRAYLITERAL:
                    {
                        printArrayLiteral(astexpr);
                    }
                    break;
                case AstExpression::EXPR_MAPLITERAL:
                    {
                        printMapLiteral(astexpr);
                    }
                    break;
                case AstExpression::EXPR_PREFIX:
                    {
                        printPrefix(astexpr);
                    }
                    break;
                case AstExpression::EXPR_INFIX:
                    {
                        printInfix(astexpr);
                    }
                    break;
                case AstExpression::EXPR_FUNCTIONLITERAL:
                    {
                        printFuncLiteral(astexpr);
                    }
                    break;
                case AstExpression::EXPR_CALL:
                    {
                        printCall(astexpr);
                    }
                    break;
                case AstExpression::EXPR_INDEX:
                    {
                        printIndex(astexpr);
                    }
                    break;
                case AstExpression::EXPR_ASSIGN:
                    {
                        printAssign(astexpr);
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
                        auto ex = astexpr->m_uexpr.exprreturnvalue;
                        if(ex != nullptr)
                        {
                            put("return ");
                            printExpression(ex);
                            put(";");
                        }
                        else
                        {
                            put("return;");
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTEXPRESSION:
                    {
                        auto ex = astexpr->m_uexpr.exprexpression;
                        if(ex != nullptr)
                        {
                            printExpression(ex);
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
                        ex = &astexpr->m_uexpr.exprblockstmt;
                        printCodeblock(ex);
                    }
                    break;
                case AstExpression::EXPR_STMTBREAK:
                    {
                        put("break");
                    }
                    break;
                case AstExpression::EXPR_STMTCONTINUE:
                    {
                        put("continue");
                    }
                    break;
                case AstExpression::EXPR_STMTIMPORT:
                    {
                        printImport(astexpr);
                    }
                    break;
                case AstExpression::EXPR_NONE:
                    {
                        put("AstExpression::EXPR_NONE");
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
            if(lastslashpos != nullptr)
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
                mc_memory_free(module->m_modname);
                module->m_modsymbols.deInit(AstSymbol::destroy);
                mc_memory_free(module);
            }
        }

        static Module* copy(Module* src)
        {
            auto modsyms = src->m_modsymbols.copyToStack(AstSymbol::copyHeap, AstSymbol::destroy);
            return Memory::make<Module>(src->m_modname, &modsyms);
        }


    public:
        char* m_modname;
        GenericList<AstSymbol*> m_modsymbols;

    public:
        Module(const char* nm): Module(nm, nullptr)
        {
        }

        Module(const char* nm, GenericList<AstSymbol*>* ms)
        {
            m_modname = mc_util_strdup(nm);
            MC_ASSERT(m_modname);
            if(ms != nullptr)
            {
                m_modsymbols = *ms;
            }
        }

        bool addSymbol(AstSymbol* symbol)
        {
            bool ok;
            AstSymbol* modulesymbol;
            (void)ok;
            Printer namebuf(nullptr);
            ok = namebuf.format("%s::%s", m_modname, symbol->m_symname);
            modulesymbol = Memory::make<AstSymbol>(namebuf.getString(), AstSymbol::SYMTYP_MODULEGLOBAL, symbol->m_symindex, false);
            Printer::releaseFromPtr(&namebuf, false);
            if(modulesymbol == nullptr)
            {
                return false;
            }
            ok = m_modsymbols.push(modulesymbol);
            return true;
        }
};

class AstCompiler
{
    public:
        enum OpCode
        {
            OPCODE_HALT = 0,
            OPCODE_CONSTANT,
            OPCODE_ADD,
            OPCODE_SUB,
            OPCODE_MUL,
            OPCODE_DIV,
            OPCODE_MOD,
            OPCODE_POP,
            OPCODE_BINOR,
            OPCODE_BINXOR,
            OPCODE_BINAND,
            OPCODE_LSHIFT,
            OPCODE_RSHIFT,
            OPCODE_BANG,
            OPCODE_COMPARE,
            OPCODE_TRUE,
            OPCODE_FALSE,
            OPCODE_COMPAREEQ,
            OPCODE_EQUAL,
            OPCODE_NOTEQUAL,
            OPCODE_GREATERTHAN,
            OPCODE_GREATERTHANEQUAL,
            OPCODE_MINUS,
            OPCODE_BINNOT,
            OPCODE_JUMP,
            OPCODE_JUMPIFFALSE,
            OPCODE_JUMPIFTRUE,
            OPCODE_NULL,
            OPCODE_GETMODULEGLOBAL,
            OPCODE_SETMODULEGLOBAL,
            OPCODE_DEFINEMODULEGLOBAL,
            OPCODE_ARRAY,
            OPCODE_MAPSTART,
            OPCODE_MAPEND,
            OPCODE_GETTHIS,
            OPCODE_GETINDEX,
            OPCODE_SETINDEX,
            OPCODE_GETDOTINDEX,
            OPCODE_GETVALUEAT,
            OPCODE_CALL,
            OPCODE_RETURNVALUE,
            OPCODE_RETURN,
            OPCODE_GETLOCAL,
            OPCODE_DEFINELOCAL,
            OPCODE_SETLOCAL,
            OPCODE_GETGLOBALBUILTIN,
            OPCODE_FUNCTION,
            OPCODE_GETFREE,
            OPCODE_SETFREE,
            OPCODE_CURRENTFUNCTION,
            OPCODE_DUP,
            OPCODE_NUMBER,
            OPCODE_FOREACHLEN,
            OPCODE_SETRECOVER,
            OPCODE_MAX
        };

        struct OpDefinition
        {
            const char* name;
            int numoperands;
            int operandwidths[2];
        };

    public:
        static void destroy(AstCompiler* comp)
        {
            if(comp != nullptr)
            {
                comp->deinit();
                mc_memory_free(comp);
            }
        }

        static OpDefinition* makeOpDef(OpDefinition* dest, const char* name, int numop, int opa1, int opa2)
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

        static OpDefinition* opdefLookup(OpDefinition* def, mcinternopcode_t op)
        {
            switch(op)
            {
                makecase(def, OPCODE_HALT, "OPCODE_HALT", 0, 0, 0);
                makecase(def, OPCODE_CONSTANT, "OPCODE_CONSTANT", 1, 2, 0);
                makecase(def, OPCODE_ADD, "OPCODE_ADD", 0, 0, 0);
                makecase(def, OPCODE_POP, "OPCODE_POP", 0, 0, 0);
                makecase(def, OPCODE_SUB, "OPCODE_SUB", 0, 0, 0);
                makecase(def, OPCODE_MUL, "OPCODE_MUL", 0, 0, 0);
                makecase(def, OPCODE_DIV, "OPCODE_DIV", 0, 0, 0);
                makecase(def, OPCODE_MOD, "OPCODE_MOD", 0, 0, 0);
                makecase(def, OPCODE_TRUE, "OPCODE_TRUE", 0, 0, 0);
                makecase(def, OPCODE_FALSE, "OPCODE_FALSE", 0, 0, 0);
                makecase(def, OPCODE_COMPARE, "OPCODE_COMPARE", 0, 0, 0);
                makecase(def, OPCODE_COMPAREEQ, "OPCODE_COMPAREEQ", 0, 0, 0);
                makecase(def, OPCODE_EQUAL, "OPCODE_EQUAL", 0, 0, 0);
                makecase(def, OPCODE_NOTEQUAL, "OPCODE_NOTEQUAL", 0, 0, 0);
                makecase(def, OPCODE_GREATERTHAN, "OPCODE_GREATERTHAN", 0, 0, 0);
                makecase(def, OPCODE_GREATERTHANEQUAL, "OPCODE_GREATERTHANEQUAL", 0, 0, 0);
                makecase(def, OPCODE_MINUS, "OPCODE_MINUS", 0, 0, 0);
                makecase(def, OPCODE_BINNOT, "OPCODE_BINNOT", 0, 0, 0);
                makecase(def, OPCODE_BANG, "OPCODE_BANG", 0, 0, 0);
                makecase(def, OPCODE_JUMP, "OPCODE_JUMP", 1, 2, 0);
                makecase(def, OPCODE_JUMPIFFALSE, "OPCODE_JUMPIFFALSE", 1, 2, 0);
                makecase(def, OPCODE_JUMPIFTRUE, "OPCODE_JUMPIFTRUE", 1, 2, 0);
                makecase(def, OPCODE_NULL, "OPCODE_NULL", 0, 0, 0);
                makecase(def, OPCODE_GETMODULEGLOBAL, "OPCODE_GETMODULEGLOBAL", 1, 2, 0);
                makecase(def, OPCODE_SETMODULEGLOBAL, "OPCODE_SETMODULEGLOBAL", 1, 2, 0);
                makecase(def, OPCODE_DEFINEMODULEGLOBAL, "OPCODE_DEFINEMODULEGLOBAL", 1, 2, 0);
                makecase(def, OPCODE_ARRAY, "OPCODE_ARRAY", 1, 2, 0);
                makecase(def, OPCODE_MAPSTART, "OPCODE_MAPSTART", 1, 2, 0);
                makecase(def, OPCODE_MAPEND, "OPCODE_MAPEND", 1, 2, 0);
                makecase(def, OPCODE_GETTHIS, "OPCODE_GETTHIS", 0, 0, 0);
                makecase(def, OPCODE_GETINDEX, "OPCODE_GETINDEX", 0, 0, 0);
                makecase(def, OPCODE_SETINDEX, "OPCODE_SETINDEX", 0, 0, 0);
                makecase(def, OPCODE_GETDOTINDEX, "OPCODE_GETDOTINDEX", 0, 0, 0);
                makecase(def, OPCODE_GETVALUEAT, "OPCODE_GETVALUEAT", 0, 0, 0);
                makecase(def, OPCODE_CALL, "OPCODE_CALL", 1, 1, 0);
                makecase(def, OPCODE_RETURNVALUE, "OPCODE_RETURNVALUE", 0, 0, 0);
                makecase(def, OPCODE_RETURN, "OPCODE_RETURN", 0, 0, 0);
                makecase(def, OPCODE_GETLOCAL, "OPCODE_GETLOCAL", 1, 1, 0);
                makecase(def, OPCODE_DEFINELOCAL, "OPCODE_DEFINELOCAL", 1, 1, 0);
                makecase(def, OPCODE_SETLOCAL, "OPCODE_SETLOCAL", 1, 1, 0);
                makecase(def, OPCODE_GETGLOBALBUILTIN, "OPCODE_GETGLOBALBUILTIN", 1, 2, 0);
                makecase(def, OPCODE_FUNCTION, "OPCODE_FUNCTION", 2, 2, 1);
                makecase(def, OPCODE_GETFREE, "OPCODE_GETFREE", 1, 1, 0);
                makecase(def, OPCODE_SETFREE, "OPCODE_SETFREE", 1, 1, 0);
                makecase(def, OPCODE_CURRENTFUNCTION, "OPCODE_CURRENTFUNCTION", 0, 0, 0);
                makecase(def, OPCODE_DUP, "OPCODE_DUP", 0, 0, 0);
                makecase(def, OPCODE_NUMBER, "OPCODE_NUMBER", 1, 8, 0);
                makecase(def, OPCODE_FOREACHLEN, "OPCODE_FOREACHLEN", 0, 0, 0);
                makecase(def, OPCODE_SETRECOVER, "OPCODE_SETRECOVER", 1, 2, 0);
                makecase(def, OPCODE_BINOR, "OPCODE_BINOR", 0, 0, 0);
                makecase(def, OPCODE_BINXOR, "OPCODE_BINXOR", 0, 0, 0);
                makecase(def, OPCODE_BINAND, "OPCODE_BINAND", 0, 0, 0);
                makecase(def, OPCODE_LSHIFT, "OPCODE_LSHIFT", 0, 0, 0);
                makecase(def, OPCODE_RSHIFT, "OPCODE_RSHIFT", 0, 0, 0);
                makecase(def, OPCODE_MAX, "OPCODE_MAX", 0, 0, 0);
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
            OpDefinition def;
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
            StrDict* modulescopy;
            GenericList<Value> constantscopy;
            GenericList<char*>* srcloadedmodulenames;
            GenericList<char*>* copyloadedmodulenames;
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
            //MC_ASSERT(src->m_filescopelist.count() == 1);
            MC_ASSERT(srcst->m_symtbouter == nullptr);
            srcstocopy = srcst->copy();
            if(srcstocopy == nullptr)
            {
                goto compilercopyfailed;
            }
            copyst = copy->getsymtable();
            AstSymTable::destroy(copyst);
            copyst = nullptr;
            copy->setsymtable(srcstocopy);
            modulescopy = src->m_modules->copy();
            if(modulescopy == nullptr)
            {
                goto compilercopyfailed;
            }
            StrDict::destroyItemsAndDict(copy->m_modules);
            copy->m_modules = modulescopy;
            if(!src->m_constants.copyToStack(&constantscopy))
            {
                goto compilercopyfailed;
            }
            copy->m_constants.deInit();
            copy->m_constants = constantscopy;
            for(i = 0; i < src->m_stringconstposdict->count(); i++)
            {
                key = src->m_stringconstposdict->getKeyAt(i);
                val = (int*)src->m_stringconstposdict->getValueAt(i);
                valcopy = (int*)mc_memory_malloc(sizeof(int));
                if(valcopy == nullptr)
                {
                    goto compilercopyfailed;
                }
                *valcopy = *val;
                ok = copy->m_stringconstposdict->set(key, valcopy);
            }
            srcfilescope = src->m_filescopelist.topp();
            copyfilescope = copy->m_filescopelist.topp();
            srcloadedmodulenames = &srcfilescope->m_filescopeloadednames;
            copyloadedmodulenames = &copyfilescope->m_filescopeloadednames;
            for(i = 0; i < srcloadedmodulenames->count(); i++)
            {
                loadedname = (const char*)srcloadedmodulenames->get(i);
                loadednamecopy = mc_util_strdup(loadedname);
                if(loadednamecopy == nullptr)
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
        RuntimeConfig* m_config = nullptr;
        GCMemory* m_astmem = nullptr;
        ErrList* m_ccerrlist = nullptr;
        GenericList<AstSourceFile*>* m_files = nullptr;
        SymStore* m_compglobalstore = nullptr;
        GenericList<Value> m_constants = GenericList<Value>(0, Value::makeNull());
        AstScopeComp* m_compilationscope = nullptr;
        GenericList<AstLocation> m_srcposstack = GenericList<AstLocation>(0, AstLocation::Invalid());
        StrDict* m_modules = nullptr;
        StrDict* m_stringconstposdict = nullptr;
        Printer* m_filestderr = nullptr;
        GenericList<AstScopeFile> m_filescopelist = GenericList<AstScopeFile>(0, AstScopeFile{});

    public:
        AstCompiler()
        {
        }

        AstCompiler(State* state, RuntimeConfig* config, GCMemory* gcmem, ErrList* errors, GenericList<AstSourceFile*>* files, SymStore* gstore, Printer* fstderr)
        {
            bool ok;
            (void)ok;
            ok = initBase(state, config, gcmem, errors, files, gstore, fstderr);
            MC_ASSERT(ok);
            m_pstate = state; 
        }

        bool initBase(State* state, RuntimeConfig* cfg, GCMemory* gcmem, ErrList* errors, GenericList<AstSourceFile*>* files, SymStore* gstor, Printer* fstderr)
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
            m_modules = Memory::make<StrDict>((mcitemcopyfn_t)Module::copy, (mcitemdestroyfn_t)Module::destroy);
            ok = pushCompilationScope();
            filename = "<none>";
            if(files->count() > 0)
            {
                filename = (const char*)files->top();
            }
            ok = filescopepush(filename);
            m_stringconstposdict = Memory::make<StrDict>(nullptr, nullptr);
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
            while(m_filescopelist.count() > 0)
            {
                filescopepop();
            }
            while(getCompilationScope() != nullptr)
            {
                popCompilationScope();
            }
            StrDict::destroyItemsAndDict(m_modules);
            m_srcposstack.deInit();
            m_constants.deInit();
            m_filescopelist.deInit();
        }

        void appendByteAt(GenericList<uint16_t>* res, const uint64_t* operands, int i, int n)
        {
            uint16_t val;
            val = (uint16_t)(operands[i] >> (n * 8));
            res->push(val);
        }

        int genCode(mcinternopcode_t op, int operandscount, const uint64_t* operands, GenericList<uint16_t>* res)
        {
            bool ok;
            int i;
            int width;
            int instrlen;
            uint16_t val;
            OpDefinition vdef;
            OpDefinition* def;
            (void)ok;
            def = opdefLookup(&vdef, op);
            if(def == nullptr)
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
            ok = res->push(val);
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
            AstLocation srcpos;
            AstScopeComp* compscope;
            (void)ok;
            ip = getip();
            len = genCode(op, operandscount, operands, getbytecode());
            if(len == 0)
            {
                return -1;
            }
            for(i = 0; i < len; i++)
            {
                srcpos = m_srcposstack.top();
                /*
                MC_ASSERT(srcpos->line >= 0);
                MC_ASSERT(srcpos->m_loccolumn >= 0);
                */
                ok = getsrcpositions()->push(srcpos);
            }
            compscope = getCompilationScope();
            compscope->m_scopelastopcode = op;
            return ip;
        }

        AstScopeComp* getCompilationScope()
        {
            return m_compilationscope;
        }

        bool pushCompilationScope()
        {
            AstScopeComp* nscope;
            AstScopeComp* currentscope;
            currentscope = getCompilationScope();
            nscope = Memory::make<AstScopeComp>(currentscope);
            setcompilationscope(nscope);
            return true;
        }

        void popCompilationScope()
        {
            AstScopeComp* currentscope;
            currentscope = getCompilationScope();
            MC_ASSERT(currentscope);
            setcompilationscope(currentscope->m_outerscope);
            Memory::destroy(currentscope);
        }

        bool pushSymtable(int globaloffset)
        {
            AstScopeFile* filescope;
            AstSymTable* currenttable;
            filescope = m_filescopelist.topp();
            if(filescope == nullptr)
            {
                MC_ASSERT(false);
                return false;
            }
            currenttable = filescope->m_scopefilesymtab;
            filescope->m_scopefilesymtab = Memory::make<AstSymTable>(currenttable, m_compglobalstore, nullptr, nullptr, nullptr, globaloffset);
            return true;
        }

        void popSymtable()
        {
            AstScopeFile* filescope;
            AstSymTable* currenttable;
            filescope = m_filescopelist.topp();
            if(filescope != nullptr)
            {
                currenttable = filescope->m_scopefilesymtab;
                if(currenttable != nullptr)
                {
                    filescope->m_scopefilesymtab = currenttable->m_symtbouter;
                    AstSymTable::destroy(currenttable);
                }
            }
        }

        mcinternopcode_t getLastOpcode()
        {
            AstScopeComp* currentscope;
            currentscope = getCompilationScope();
            return currentscope->m_scopelastopcode;
        }

        bool doCompileSource(const char* code)
        {
            bool ok;
            AstScopeFile* filescope;
            GenericList<AstExpression*> statements;
            (void)ok;
            filescope = m_filescopelist.topp();
            MC_ASSERT(filescope);
            if(!filescope->m_filescopeparser->parseAll(&statements, code, filescope->m_filescopesourcefile))
            {
                /* errors are added by parser */
                return false;
            }
            if(m_config->dumpast)
            {
                AstPrinter apr(m_filestderr);
                apr.beginPrint(&statements);
            }
            if(m_config->exitafterastdump)
            {
                statements.deInit(AstExpression::destroyExpression);
                return false;
            }
            ok = compileStmtList(&statements);
            statements.deInit(AstExpression::destroyExpression);
            if(m_config->dumpbytecode)
            {
                mc_printer_printbytecode(m_filestderr,
                    m_compilationscope->m_scopecompiledbc.data(),
                    m_compilationscope->m_scopesrcposlist.data(),
                    m_compilationscope->m_scopecompiledbc.count(), false);
            }
            return ok;
        }

        bool compileStmtList(GenericList<AstExpression*>* statements)
        {
            bool ok;
            size_t i;
            AstExpression* expr;
            (void)ok;
            ok = true;
            for(i = 0; i < statements->count(); i++)
            {
                expr = statements->get(i);
                ok = compileExpression(expr);
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
            Printer filepathbuf(nullptr);
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
            filescope = m_filescopelist.topp();
            modpath = importstmt->m_uexpr.exprimportstmt.path;
            modname = Module::getModuleName(modpath);
            for(i = 0; i < filescope->m_filescopeloadednames.count(); i++)
            {
                loadedname = filescope->m_filescopeloadednames.get(i);
                if(mc_util_strequal(loadedname, modname))
                {
                    if(m_config->fatalcomplaints)
                    {
                        m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, importstmt->m_exprpos, "module \"%s\" was already imported", modname);
                        result = false;
                    }
                    else
                    {
                        mc_util_complain(importstmt->m_exprpos, "module \"%s\" already imported; ignoring 'import' statement", modname);
                        result = true;
                    }
                    goto end;
                }
            }
            if(mc_util_pathisabsolute(modpath))
            {
                filepathbuf.format("%s.mc", modpath);
            }
            else
            {
                filepathbuf.format("%s%s.mc", filescope->m_filescopesourcefile->getDirectory(), modpath);
            }
            filepathnoncanonicalised = filepathbuf.getString();
            filepath = mc_util_canonpath(filepathnoncanonicalised);
            Printer::releaseFromPtr(&filepathbuf, false);
            if(filepath == nullptr)
            {
                result = false;
                goto end;
            }
            symtab = getsymtable();
            if(symtab->m_symtbouter != nullptr || symtab->m_symtbblockscopes.count() > 1)
            {
                m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, importstmt->m_exprpos, "modules can only be imported in global scope");
                result = false;
                goto end;
            }
            for(i = 0; i < m_filescopelist.count(); i++)
            {
                fs = m_filescopelist.getp(i);
                if(mc_util_strequal(fs->m_filescopesourcefile->path(), filepath))
                {
                    m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, importstmt->m_exprpos, "cyclic reference of file \"%s\"", filepath);
                    result = false;
                    goto end;
                }
            }
            module = (Module*)m_modules->get(filepath);
            if(module == nullptr)
            {
                /* todo: create new module function */
                searchedpath = Module::findFile(filepath);
                code = mc_fsutil_fileread(searchedpath, &flen);
                if(code == nullptr)
                {
                    m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, importstmt->m_exprpos, "reading module file \"%s\" failed", filepath);
                    result = false;
                    goto end;
                }
                module = Memory::make<Module>(modname);
                ok = filescopepush(searchedpath);
                ok = doCompileSource(code);
                st = getsymtable();
                for(i = 0; i < st->getModuleGlobalSymCount(); i++)
                {
                    symbol = st->getModuleGlobalSymAt(i);
                    module->addSymbol(symbol);
                }
                filescopepop();
                ok = m_modules->set(filepath, module);
            }
            for(i = 0; i < module->m_modsymbols.count(); i++)
            {
                symbol = module->m_modsymbols.get(i);
                ok = symtab->addModuleSymbol(symbol);
            }
            namecopy = mc_util_strdup(modname);
            ok = filescope->m_filescopeloadednames.push(namecopy);
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
            symtab = getsymtable();
            if(!canshadow && !symtab->isTopGlobalScope())
            {
                currentsymbol = symtab->resolve(name);
                if(currentsymbol != nullptr)
                {
                    m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, pos, "symbol \"%s\" is already defined", name);
                    return nullptr;
                }
            }
            symbol = symtab->defineSymbol(name, assignable);
            if(symbol == nullptr)
            {
                m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, pos, "cannot define symbol \"%s\"", name);
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
            ok = compileExpression(expr->m_uexpr.exprdefine.value);
            symbol = doDefineSymbol(expr->m_uexpr.exprdefine.name.m_exprpos, expr->m_uexpr.exprdefine.name.m_identvalue, expr->m_uexpr.exprdefine.assignable, false);
            if(symbol == nullptr)
            {
                return false;
            }
            ok = storesymbol(symbol, true);
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
            int pos;
            AstExpression::ExprIfCase* ifcase;
            AstExpression::ExprIfStmt* ifstmt;
            (void)ok;
            ifstmt = &expr->m_uexpr.exprifstmt;
            auto jumptoendips = Memory::make<GenericList<int>>(0, 0);
            for(i = 0; i < ifstmt->m_ifcases.count(); i++)
            {
                ifcase = ifstmt->m_ifcases.get(i);
                ok = compileExpression(ifcase->m_ifcond);
                opbuf[0] = 0xbeef;
                nextcasejumpip = emitOpCode(OPCODE_JUMPIFFALSE, 1, opbuf);
                ok = compilecodeblock(&ifcase->m_ifthen);
                /* don't emit jump for the last statement */
                if(i < (ifstmt->m_ifcases.count() - 1) || ifstmt->m_haveifstmtelsestmt)
                {
                    opbuf[0] = 0xbeef;
                    jumptoendip = emitOpCode(OPCODE_JUMP, 1, opbuf);
                    ok = jumptoendips->push(jumptoendip);
                }
                afterelifip = getip();
                changeuint16operand(nextcasejumpip + 1, afterelifip);
            }
            if(ifstmt->m_haveifstmtelsestmt)
            {
                ok = compilecodeblock(&ifstmt->m_ifstmtelsestmt);
            }
            afteraltip = getip();
            for(i = 0; i < jumptoendips->count(); i++)
            {
                pos = jumptoendips->get(i);
                changeuint16operand(pos + 1, afteraltip);
            }
            Memory::destroy(jumptoendips);
            return true;
        }

        bool compilereturnstmt(AstScopeComp* compscope, AstExpression* expr)
        {
            bool ok;
            int ip;
            (void)ok;
            if(compscope->m_outerscope == nullptr)
            {
                m_ccerrlist->pushFormat( Error::ERRTYP_COMPILING, expr->m_exprpos, "nothing to return from");
                return false;
            }
            ip = -1;
            if(expr->m_uexpr.exprreturnvalue != nullptr)
            {
                ok = compileExpression(expr->m_uexpr.exprreturnvalue);
                ip = emitOpCode(OPCODE_RETURNVALUE, 0, nullptr);
            }
            else
            {
                ip = emitOpCode(OPCODE_RETURN, 0, nullptr);
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
            loop = &expr->m_uexpr.exprwhileloopstmt;
            beforetestip = getip();
            ok = compileExpression(loop->loopcond);
            aftertestip = getip();
            opbuf[0] = aftertestip + 6;
            ip = emitOpCode(OPCODE_JUMPIFTRUE, 1, opbuf);
            if(ip < 0)
            {
                return false;
            }
            opbuf[0] = 0xdead;
            jumptoafterbodyip = emitOpCode(OPCODE_JUMP, 1, opbuf);
            if(jumptoafterbodyip < 0)
            {
                return false;
            }
            ok = pushcontinueip(beforetestip);
            ok = pushbreakip(jumptoafterbodyip);
            ok = compilecodeblock(&loop->body);
            popbreakip();
            popcontinueip();
            opbuf[0] = beforetestip;
            ip = emitOpCode(OPCODE_JUMP, 1, opbuf);
            if(ip < 0)
            {
                return false;
            }
            afterbodyip = getip();
            changeuint16operand(jumptoafterbodyip + 1, afterbodyip);
            return true;
        }

        bool compilebreakstmt(uint64_t* opbuf, AstExpression* expr)
        {
            int ip;
            int breakip;
            breakip = getbreakip();
            if(breakip < 0)
            {
                m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "nothing to break from.");
                return false;
            }
            opbuf[0] = breakip;
            ip = emitOpCode(OPCODE_JUMP, 1, opbuf);
            return ip >= 0;
        }

        bool compilecontinuestmt(uint64_t* opbuf, AstExpression* expr)
        {
            int ip;
            int continueip;
            continueip = getcontinueip();
            if(continueip < 0)
            {
                m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "nothing to continue from.");
                return false;
            }
            opbuf[0] = continueip;
            ip = emitOpCode(OPCODE_JUMP, 1, opbuf);
            return ip >= 0;
        }

        bool compileExpression(AstExpression* expr)
        {
            bool ok;
            bool res;
            int ip;
            uint64_t opbuf[10];
            AstScopeComp* compscope;
            AstSymTable* symtab;
            (void)ok;
            ok = false;
            ip = -1;
            ok = m_srcposstack.push(expr->m_exprpos);
            compscope = getCompilationScope();
            symtab = getsymtable();
            res = false;
            switch(expr->m_exprtype)
            {
                case AstExpression::EXPR_INFIX:
                    {
                        bool rearrange;
                        mcinternopcode_t op;
                        AstExpression* left;
                        AstExpression* right;
                        rearrange = false;
                        op = OPCODE_HALT;
                        switch(expr->m_uexpr.exprinfix.op)
                        {
                            case AstExpression::MATHOP_PLUS:
                                op = OPCODE_ADD;
                                break;
                            case AstExpression::MATHOP_MINUS:
                                op = OPCODE_SUB;
                                break;
                            case AstExpression::MATHOP_ASTERISK:
                                op = OPCODE_MUL;
                                break;
                            case AstExpression::MATHOP_SLASH:
                                op = OPCODE_DIV;
                                break;
                            case AstExpression::MATHOP_MODULUS:
                                op = OPCODE_MOD;
                                break;
                            case AstExpression::MATHOP_EQ:
                                op = OPCODE_EQUAL;
                                break;
                            case AstExpression::MATHOP_NOTEQ:
                                op = OPCODE_NOTEQUAL;
                                break;
                            case AstExpression::MATHOP_GT:
                                op = OPCODE_GREATERTHAN;
                                break;
                            case AstExpression::MATHOP_GTE:
                                op = OPCODE_GREATERTHANEQUAL;
                                break;
                            case AstExpression::MATHOP_LT:
                                op = OPCODE_GREATERTHAN;
                                rearrange = true;
                                break;
                            case AstExpression::MATHOP_LTE:
                                op = OPCODE_GREATERTHANEQUAL;
                                rearrange = true;
                                break;
                            case AstExpression::MATHOP_BINOR:
                                op = OPCODE_BINOR;
                                break;
                            case AstExpression::MATHOP_BINXOR:
                                op = OPCODE_BINXOR;
                                break;
                            case AstExpression::MATHOP_BINAND:
                                op = OPCODE_BINAND;
                                break;
                            case AstExpression::MATHOP_LSHIFT:
                                op = OPCODE_LSHIFT;
                                break;
                            case AstExpression::MATHOP_RSHIFT:
                                op = OPCODE_RSHIFT;
                                break;
                            default:
                                {
                                    m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "unknown infix operator");
                                    goto error;
                                }
                                break;
                        }
                        left = rearrange ? expr->m_uexpr.exprinfix.right : expr->m_uexpr.exprinfix.left;
                        right = rearrange ? expr->m_uexpr.exprinfix.left : expr->m_uexpr.exprinfix.right;
                        ok = compileExpression(left);
                        ok = compileExpression(right);
                        switch(expr->m_uexpr.exprinfix.op)
                        {
                            case AstExpression::MATHOP_EQ:
                            case AstExpression::MATHOP_NOTEQ:
                                {
                                    ip = emitOpCode(OPCODE_COMPAREEQ, 0, nullptr);
                                    if(ip < 0)
                                    {
                                        goto error;
                                    }
                                }
                                break;
                            case AstExpression::MATHOP_GT:
                            case AstExpression::MATHOP_GTE:
                            case AstExpression::MATHOP_LT:
                            case AstExpression::MATHOP_LTE:
                                {
                                    ip = emitOpCode(OPCODE_COMPARE, 0, nullptr);
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
                        ip = emitOpCode(op, 0, nullptr);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;

                case AstExpression::EXPR_NUMBERLITERAL:
                    {
                        mcfloat_t number;
                        number = expr->m_uexpr.exprlitnumber;
                        opbuf[0] = mc_util_doubletouint64(number);
                        ip = emitOpCode(OPCODE_NUMBER, 1, opbuf);
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
                        currentpos = (int*)m_stringconstposdict->get(expr->m_uexpr.exprlitstring.m_strexprdata);
                        if(currentpos != nullptr)
                        {
                            pos = *currentpos;
                        }
                        else
                        {
                            obj = mc_value_makestringlen(m_pstate, expr->m_uexpr.exprlitstring.m_strexprdata, expr->m_uexpr.exprlitstring.m_strexprlength);
                            if(obj.isNull())
                            {
                                goto error;
                            }
                            pos = addconstant(obj);
                            if(pos < 0)
                            {
                                goto error;
                            }
                            posval = (int*)mc_memory_malloc(sizeof(int));
                            if(posval == nullptr)
                            {
                                goto error;
                            }
                            *posval = pos;
                            ok = m_stringconstposdict->set(expr->m_uexpr.exprlitstring.m_strexprdata, posval);
                        }
                        opbuf[0] = pos;
                        ip = emitOpCode(OPCODE_CONSTANT, 1, opbuf);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_NULLLITERAL:
                    {
                        ip = emitOpCode(OPCODE_NULL, 0, nullptr);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_BOOLLITERAL:
                    {
                        ip = emitOpCode(expr->m_uexpr.exprlitbool ? OPCODE_TRUE : OPCODE_FALSE, 0, nullptr);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_ARRAYLITERAL:
                    {
                        size_t i;
                        for(i = 0; i < expr->m_uexpr.exprlitarray.m_litarritems.count(); i++)
                        {
                            ok = compileExpression(expr->m_uexpr.exprlitarray.m_litarritems.get(i));
                            if(!ok)
                            {
                                goto error;
                            }
                        }
                        opbuf[0] = expr->m_uexpr.exprlitarray.m_litarritems.count();
                        ip = emitOpCode(OPCODE_ARRAY, 1, opbuf);
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
                        map = &expr->m_uexpr.exprlitmap;
                        len = map->m_litmapkeys.count();
                        opbuf[0] = len;
                        ip = emitOpCode(OPCODE_MAPSTART, 1, opbuf);
                        if(ip < 0)
                        {
                            goto error;
                        }
                        for(i = 0; i < len; i++)
                        {
                            key = map->m_litmapkeys.get(i);
                            val = map->m_litmapvalues.get(i);
                            ok = compileExpression(key);
                            if(!ok)
                            {
                                goto error;
                            }
                            ok = compileExpression(val);
                            if(!ok)
                            {
                                goto error;
                            }
                        }
                        opbuf[0] = len;
                        ip = emitOpCode(OPCODE_MAPEND, 1, opbuf);
                        if(ip < 0)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_PREFIX:
                    {
                        mcinternopcode_t op;
                        ok = compileExpression(expr->m_uexpr.exprprefix.right);
                        if(!ok)
                        {
                            goto error;
                        }
                        op = OPCODE_HALT;
                        switch(expr->m_uexpr.exprprefix.op)
                        {
                            case AstExpression::MATHOP_MINUS:
                                op = OPCODE_MINUS;
                                break;
                            case AstExpression::MATHOP_BINNOT:
                                op = OPCODE_BINNOT;
                                break;
                            case AstExpression::MATHOP_BANG:
                                op = OPCODE_BANG;
                                break;
                            default:
                                {
                                    m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "unknown prefix operator.");
                                    goto error;
                                }
                                break;
                        }
                        ip = emitOpCode(op, 0, nullptr);
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
                        ident = &expr->m_uexpr.exprident;
                        symbol = symtab->resolve(ident->m_identvalue);
                        if(symbol == nullptr)
                        {
                            if(m_config->strictmode)
                            {
                                m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, ident->m_exprpos, "compilation: failed to resolve symbol \"%s\"", ident->m_identvalue);
                                goto error;
                            }
                            else
                            {
                                symbol = doDefineSymbol(ident->m_exprpos, ident->m_identvalue, true, false);
                            }
                        }
                        ok = readsymbol(symbol);
                        if(!ok)
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_INDEX:
                    {
                        AstExpression::ExprIndex* index;
                        index = &expr->m_uexpr.exprindex;
                        ok = compileExpression(index->left);
                        if(!ok)
                        {
                            goto error;
                        }
                        ok = compileExpression(index->index);
                        if(!ok)
                        {
                            goto error;
                        }
                        if(index->isdot)
                        {
                            ip = emitOpCode(OPCODE_GETDOTINDEX, 0, nullptr);
                        }
                        else
                        {
                            ip = emitOpCode(OPCODE_GETINDEX, 0, nullptr);
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
                        CompiledProgram* comp_res;
                        AstExpression::ExprLiteralFunction* fn;
                        AstSymbol* symbol;
                        AstSymbol* fnsymbol;
                        AstSymbol* thissymbol;
                        AstSymbol* paramsymbol;
                        AstExpression::ExprFuncParam* param;
                        fn = &expr->m_uexpr.exprlitfunction;
                        ok = pushCompilationScope();
                        ok = pushSymtable(0);
                        compscope = getCompilationScope();
                        symtab = getsymtable();
                        if(fn->name != nullptr)
                        {
                            fnsymbol = symtab->defineFunctionName(fn->name, false);
                            if(fnsymbol == nullptr)
                            {
                                m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "cannot define function name as \"%s\"", fn->name);
                                goto error;
                            }
                        }
                        thissymbol = symtab->defineThis();
                        if(thissymbol == nullptr)
                        {
                            m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "cannot define \"this\" symbol");
                            goto error;
                        }
                        for(i = 0; i < expr->m_uexpr.exprlitfunction.funcparamlist.count(); i++)
                        {
                            param = expr->m_uexpr.exprlitfunction.funcparamlist.get(i);
                            paramsymbol = doDefineSymbol(param->m_paramident.m_exprpos, param->m_paramident.m_identvalue, true, false);
                            if(paramsymbol == nullptr)
                            {
                                goto error;
                            }
                        }
                        ok = compileStmtList(&fn->body.m_blockstatements);
                        if(!ok)
                        {
                            goto error;
                        }
                        if(!lastopcodeis(OPCODE_RETURNVALUE) && !lastopcodeis(OPCODE_RETURN))
                        {
                            ip = emitOpCode(OPCODE_RETURN, 0, nullptr);
                            if(ip < 0)
                            {
                                goto error;
                            }
                        }
                        auto freesyms = symtab->m_symtbfreesymbols;
                        /* because it gets destroyed with compiler_pop_compilation_scope() */
                        symtab->m_symtbfreesymbols = nullptr;
                        nlocals = symtab->m_symtbmaxnumdefinitions;
                        comp_res = compscope->orphanResult();
                        if(comp_res == nullptr)
                        {
                            Memory::destroy(freesyms, AstSymbol::destroy);
                            goto error;
                        }
                        popSymtable();
                        popCompilationScope();
                        compscope = getCompilationScope();
                        symtab = getsymtable();
                        obj = mc_value_makefuncscript(m_pstate, fn->name, comp_res, true, nlocals, fn->funcparamlist.count(), 0);
                        if(obj.isNull())
                        {
                            Memory::destroy(freesyms, AstSymbol::destroy);
                            CompiledProgram::destroy(comp_res);
                            goto error;
                        }
                        for(i = 0; i < freesyms->count(); i++)
                        {
                            symbol = freesyms->get(i);
                            ok = readsymbol(symbol);
                            if(!ok)
                            {
                                Memory::destroy(freesyms, AstSymbol::destroy);
                                goto error;
                            }
                        }
                        pos = addconstant(obj);
                        if(pos < 0)
                        {
                            Memory::destroy(freesyms, AstSymbol::destroy);
                            goto error;
                        }
                        opbuf[0] = pos;
                        opbuf[1] = freesyms->count();
                        ip = emitOpCode(OPCODE_FUNCTION, 2, opbuf);
                        if(ip < 0)
                        {
                            Memory::destroy(freesyms, AstSymbol::destroy);
                            goto error;
                        }
                        Memory::destroy(freesyms, AstSymbol::destroy);
                    }
                    break;

                case AstExpression::EXPR_CALL:
                    {
                        size_t i;
                        AstExpression* argexpr;
                        ok = compileExpression(expr->m_uexpr.exprcall.function);
                        if(!ok)
                        {
                            goto error;
                        }
                        for(i = 0; i < expr->m_uexpr.exprcall.m_callargs.count(); i++)
                        {
                            argexpr = expr->m_uexpr.exprcall.m_callargs.get(i);
                            ok = compileExpression(argexpr);
                            if(!ok)
                            {
                                goto error;
                            }
                        }
                        opbuf[0] = expr->m_uexpr.exprcall.m_callargs.count();
                        ip = emitOpCode(OPCODE_CALL, 1, opbuf);
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
                        assign = &expr->m_uexpr.exprassign;
                        if(assign->dest->m_exprtype != AstExpression::EXPR_IDENT && assign->dest->m_exprtype != AstExpression::EXPR_INDEX)
                        {
                            m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, assign->dest->m_exprpos, "expression is not assignable");
                            goto error;
                        }
                        if(assign->is_postfix)
                        {
                            ok = compileExpression(assign->dest);
                            if(!ok)
                            {
                                goto error;
                            }
                        }
                        ok = compileExpression(assign->source);
                        if(!ok)
                        {
                            goto error;
                        }
                        ip = emitOpCode(OPCODE_DUP, 0, nullptr);
                        if(ip < 0)
                        {
                            goto error;
                        }
                        ok = m_srcposstack.push(assign->dest->m_exprpos);
                        if(assign->dest->m_exprtype == AstExpression::EXPR_IDENT)
                        {
                            ident = &assign->dest->m_uexpr.exprident;
                            symbol = symtab->resolve(ident->m_identvalue);
                            if(symbol == nullptr)
                            {
                                if(m_config->strictmode)
                                {
                                    m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, assign->dest->m_exprpos, "cannot assign to undeclared symbol \"%s\"", ident->m_identvalue);
                                    goto error;
                                }
                                else
                                {
                                    symbol = doDefineSymbol(ident->m_exprpos, ident->m_identvalue, true, false);
                                    if(symbol == nullptr)
                                    {
                                        m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, assign->dest->m_exprpos, "failed to implicitly create symbol \"%s\"", ident->m_identvalue);
                                        goto error;
                                    }
                                }
                            }
                            if(!symbol->m_symisassignable)
                            {
                                m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, assign->dest->m_exprpos, "compilation: cannot assign to readonly symbol \"%s\"", ident->m_identvalue);
                                goto error;
                            }
                            ok = storesymbol(symbol, false);
                            if(!ok)
                            {
                                goto error;
                            }
                        }
                        else if(assign->dest->m_exprtype == AstExpression::EXPR_INDEX)
                        {
                            index = &assign->dest->m_uexpr.exprindex;
                            ok = compileExpression(index->left);
                            if(!ok)
                            {
                                goto error;
                            }
                            ok = compileExpression(index->index);
                            if(!ok)
                            {
                                goto error;
                            }
                            ip = emitOpCode(OPCODE_SETINDEX, 0, nullptr);
                            if(ip < 0)
                            {
                                goto error;
                            }
                        }
                        if(assign->is_postfix)
                        {
                            ip = emitOpCode(OPCODE_POP, 0, nullptr);
                            if(ip < 0)
                            {
                                goto error;
                            }
                        }
                        m_srcposstack.pop(nullptr);
                    }
                    break;

                case AstExpression::EXPR_LOGICAL:
                    {
                        int afterrightip;
                        int afterleftjumpip;
                        AstExpression::ExprLogical* logi;
                        logi = &expr->m_uexpr.exprlogical;
                        ok = compileExpression(logi->left);
                        if(!ok)
                        {
                            goto error;
                        }
                        ip = emitOpCode(OPCODE_DUP, 0, nullptr);
                        if(ip < 0)
                        {
                            goto error;
                        }
                        afterleftjumpip = 0;
                        if(logi->op == AstExpression::MATHOP_LOGICALAND)
                        {
                            opbuf[0] = 0xbeef;
                            afterleftjumpip = emitOpCode(OPCODE_JUMPIFFALSE, 1, opbuf);
                        }
                        else
                        {
                            opbuf[0] = 0xbeef;
                            afterleftjumpip = emitOpCode(OPCODE_JUMPIFTRUE, 1, opbuf);
                        }
                        if(afterleftjumpip < 0)
                        {
                            goto error;
                        }
                        ip = emitOpCode(OPCODE_POP, 0, nullptr);
                        if(ip < 0)
                        {
                            goto error;
                        }
                        ok = compileExpression(logi->right);
                        if(!ok)
                        {
                            goto error;
                        }
                        afterrightip = getip();
                        changeuint16operand(afterleftjumpip + 1, afterrightip);
                    }
                    break;
                case AstExpression::EXPR_TERNARY:
                    {
                        int endip;
                        int elseip;
                        int endjumpip;
                        int elsejumpip;
                        AstExpression::ExprTernary* ternary;
                        ternary = &expr->m_uexpr.exprternary;
                        ok = compileExpression(ternary->tercond);
                        if(!ok)
                        {
                            goto error;
                        }
                        opbuf[0] = 0xbeef;
                        elsejumpip = emitOpCode(OPCODE_JUMPIFFALSE, 1, opbuf);
                        ok = compileExpression(ternary->teriftrue);
                        if(!ok)
                        {
                            goto error;
                        }
                        opbuf[0] = 0xbeef;
                        endjumpip = emitOpCode(OPCODE_JUMP, 1, opbuf);
                        elseip = getip();
                        changeuint16operand(elsejumpip + 1, elseip);
                        ok = compileExpression(ternary->teriffalse);
                        if(!ok)
                        {
                            goto error;
                        }
                        endip = getip();
                        changeuint16operand(endjumpip + 1, endip);
                    }
                    break;

                case AstExpression::EXPR_STMTEXPRESSION:
                    {
                        ok = compileExpression(expr->m_uexpr.exprexpression);
                        if(!ok)
                        {
                            return false;
                        }
                        ip = emitOpCode(OPCODE_POP, 0, nullptr);
                        if(ip < 0)
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTDEFINE:
                    {
                        if(!compileDefine(expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTIF:
                    {
                        if(!compileifstmt(opbuf, expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTRETURN:
                    {
                        if(!compilereturnstmt(compscope, expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTLOOPWHILE:
                    {
                        if(!compilewhilestmt(opbuf, expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTBREAK:
                    {
                        if(!compilebreakstmt(opbuf, expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTCONTINUE:
                    {
                        if(!compilecontinuestmt(opbuf, expr))
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
                        foreach = &expr->m_uexpr.exprforeachloopstmt;
                        ok = symtab->scopeBlockPush();
                        /* Init */
                        indexsymbol = doDefineSymbol(expr->m_exprpos, "@i", false, true);
                        if(indexsymbol == nullptr)
                        {
                            return false;
                        }
                        opbuf[0] = 0;
                        ip = emitOpCode(OPCODE_NUMBER, 1, opbuf);
                        if(ip < 0)
                        {
                            return false;
                        }
                        ok = storesymbol(indexsymbol, true);
                        if(!ok)
                        {
                            return false;
                        }
                        sourcesymbol = nullptr;
                        if(foreach->source->m_exprtype == AstExpression::EXPR_IDENT)
                        {
                            sourcesymbol = symtab->resolve(foreach->source->m_uexpr.exprident.m_identvalue);
                            if(sourcesymbol == nullptr)
                            {
                                m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, foreach->source->m_exprpos, "symbol \"%s\" could not be resolved", foreach->source->m_uexpr.exprident.m_identvalue);
                                return false;
                            }
                        }
                        else
                        {
                            ok = compileExpression(foreach->source);
                            if(!ok)
                            {
                                return false;
                            }
                            sourcesymbol = doDefineSymbol(foreach->source->m_exprpos, "@source", false, true);
                            if(sourcesymbol == nullptr)
                            {
                                return false;
                            }
                            ok = storesymbol(sourcesymbol, true);
                            if(!ok)
                            {
                                return false;
                            }
                        }
                        /* Update */
                        opbuf[0] = 0xbeef;
                        jumptoafterupdateip = emitOpCode(OPCODE_JUMP, 1, opbuf);
                        if(jumptoafterupdateip < 0)
                        {
                            return false;
                        }
                        updateip = getip();
                        ok = readsymbol(indexsymbol);
                        if(!ok)
                        {
                            return false;
                        }
                        opbuf[0] = mc_util_doubletouint64(1);
                        ip = emitOpCode(OPCODE_NUMBER, 1, opbuf);
                        if(ip < 0)
                        {
                            return false;
                        }
                        ip = emitOpCode(OPCODE_ADD, 0, nullptr);
                        if(ip < 0)
                        {
                            return false;
                        }
                        ok = storesymbol(indexsymbol, false);
                        if(!ok)
                        {
                            return false;
                        }
                        afterupdateip = getip();
                        changeuint16operand(jumptoafterupdateip + 1, afterupdateip);
                        /* Test */
                        ok = m_srcposstack.push(foreach->source->m_exprpos);
                        ok = readsymbol(sourcesymbol);
                        if(!ok)
                        {
                            return false;
                        }
                        ip = emitOpCode(OPCODE_FOREACHLEN, 0, nullptr);
                        if(ip < 0)
                        {
                            return false;
                        }
                        m_srcposstack.pop(nullptr);
                        ok = readsymbol(indexsymbol);
                        if(!ok)
                        {
                            return false;
                        }
                        ip = emitOpCode(OPCODE_COMPARE, 0, nullptr);
                        if(ip < 0)
                        {
                            return false;
                        }
                        ip = emitOpCode(OPCODE_EQUAL, 0, nullptr);
                        if(ip < 0)
                        {
                            return false;
                        }
                        aftertestip = getip();
                        opbuf[0] = aftertestip + 6;
                        ip = emitOpCode(OPCODE_JUMPIFFALSE, 1, opbuf);
                        if(ip < 0)
                        {
                            return false;
                        }
                        opbuf[0] = 0xdead;
                        jumptoafterbodyip = emitOpCode(OPCODE_JUMP, 1, opbuf);
                        if(jumptoafterbodyip < 0)
                        {
                            return false;
                        }
                        ok = readsymbol(sourcesymbol);
                        if(!ok)
                        {
                            return false;
                        }
                        ok = readsymbol(indexsymbol);
                        if(!ok)
                        {
                            return false;
                        }
                        ip = emitOpCode(OPCODE_GETVALUEAT, 0, nullptr);
                        if(ip < 0)
                        {
                            return false;
                        }
                        itersymbol = doDefineSymbol(foreach->iterator.m_exprpos, foreach->iterator.m_identvalue, false, false);
                        if(itersymbol == nullptr)
                        {
                            return false;
                        }
                        ok = storesymbol(itersymbol, true);
                        if(!ok)
                        {
                            return false;
                        }
                        /* Body */
                        ok = pushcontinueip(updateip);
                        ok = pushbreakip(jumptoafterbodyip);
                        ok = compilecodeblock(&foreach->body);
                        popbreakip();
                        popcontinueip();
                        opbuf[0] = updateip;
                        ip = emitOpCode(OPCODE_JUMP, 1, opbuf);
                        if(ip < 0)
                        {
                            return false;
                        }
                        afterbodyip = getip();
                        changeuint16operand(jumptoafterbodyip + 1, afterbodyip);
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
                        loop = &expr->m_uexpr.exprforloopstmt;
                        ok = symtab->scopeBlockPush();
                        /* Init */
                        jumptoafterupdateip = 0;
                        ok = false;
                        if(loop->init != nullptr)
                        {
                            ok = compileExpression(loop->init);
                            if(!ok)
                            {
                                return false;
                            }
                            opbuf[0] = 0xbeef;
                            jumptoafterupdateip = emitOpCode(OPCODE_JUMP, 1, opbuf);
                            if(jumptoafterupdateip < 0)
                            {
                                return false;
                            }
                        }
                        /* Update */
                        updateip = getip();
                        if(loop->update != nullptr)
                        {
                            ok = compileExpression(loop->update);
                            if(!ok)
                            {
                                return false;
                            }
                            ip = emitOpCode(OPCODE_POP, 0, nullptr);
                            if(ip < 0)
                            {
                                return false;
                            }
                        }
                        if(loop->init != nullptr)
                        {
                            afterupdateip = getip();
                            changeuint16operand(jumptoafterupdateip + 1, afterupdateip);
                        }
                        /* Test */
                        if(loop->loopcond != nullptr)
                        {
                            ok = compileExpression(loop->loopcond);
                            if(!ok)
                            {
                                return false;
                            }
                        }
                        else
                        {
                            ip = emitOpCode(OPCODE_TRUE, 0, nullptr);
                            if(ip < 0)
                            {
                                return false;
                            }
                        }
                        aftertestip = getip();
                        opbuf[0] = aftertestip + 6;
                        ip = emitOpCode(OPCODE_JUMPIFTRUE, 1, opbuf);
                        if(ip < 0)
                        {
                            return false;
                        }
                        opbuf[0] = 0xdead;
                        jumptoafterbodyip = emitOpCode(OPCODE_JUMP, 1, opbuf);
                        if(jumptoafterbodyip < 0)
                        {
                            return false;
                        }
                        /* Body */
                        ok = pushcontinueip(updateip);
                        ok = pushbreakip(jumptoafterbodyip);
                        ok = compilecodeblock(&loop->body);
                        if(!ok)
                        {
                            return false;
                        }
                        popbreakip();
                        popcontinueip();
                        opbuf[0] = updateip;
                        ip = emitOpCode(OPCODE_JUMP, 1, opbuf);
                        if(ip < 0)
                        {
                            return false;
                        }
                        afterbodyip = getip();
                        changeuint16operand(jumptoafterbodyip + 1, afterbodyip);
                        symtab->scopeBlockPop();
                    }
                    break;
                case AstExpression::EXPR_STMTBLOCK:
                    {
                        ok = compilecodeblock(&expr->m_uexpr.exprblockstmt);
                        if(!ok)
                        {
                            return false;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTIMPORT:
                    {
                        ok = compileImportStmt(expr);
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
                        recover = &expr->m_uexpr.exprrecoverstmt;
                        if(m_config->strictmode)
                        {
                            if(symtab->isModuleGlobalScope())
                            {
                                m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "recover statement cannot be defined in global scope");
                                return false;
                            }
                        }
                        opbuf[0] = 0xbeef;
                        recip = emitOpCode(OPCODE_SETRECOVER, 1, opbuf);
                        if(recip < 0)
                        {
                            return false;
                        }
                        opbuf[0] = 0xbeef;
                        jumptoafterrecoverip = emitOpCode(OPCODE_JUMP, 1, opbuf);
                        if(jumptoafterrecoverip < 0)
                        {
                            return false;
                        }
                        afterjumptorecoverip = getip();
                        changeuint16operand(recip + 1, afterjumptorecoverip);
                        ok = symtab->scopeBlockPush();
                        errorsymbol = doDefineSymbol(recover->errident.m_exprpos, recover->errident.m_identvalue, false, false);
                        if(errorsymbol == nullptr)
                        {
                            return false;
                        }
                        ok = storesymbol(errorsymbol, true);
                        if(!ok)
                        {
                            return false;
                        }
                        ok = compilecodeblock(&recover->body);
                        if(!ok)
                        {
                            return false;
                        }
                        if(!lastopcodeis(OPCODE_RETURN) && !lastopcodeis(OPCODE_RETURNVALUE))
                        {
                            mc_util_complain(expr->m_exprpos, "recover body should end with a return statement");
                        }
                        symtab->scopeBlockPop();
                        afterrecoverip = getip();
                        changeuint16operand(jumptoafterrecoverip + 1, afterrecoverip);
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
            m_srcposstack.pop(nullptr);
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
            symtab = getsymtable();
            if(symtab == nullptr)
            {
                return false;
            }
            ok = symtab->scopeBlockPush();
            if(block->m_blockstatements.count() == 0)
            {
                ip = emitOpCode(OPCODE_NULL, 0, nullptr);
                if(ip < 0)
                {
                    return false;
                }
                ip = emitOpCode(OPCODE_POP, 0, nullptr);
                if(ip < 0)
                {
                    return false;
                }
            }
            for(i = 0; i < block->m_blockstatements.count(); i++)
            {
                expr = block->m_blockstatements.get(i);
                ok = compileExpression(expr);
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
            ok = m_constants.push(obj);
            pos = m_constants.count() - 1;
            return pos;
        }

        void changeuint16operand(int ip, uint16_t operand)
        {
            uint16_t hi;
            uint16_t lo;
            auto bytecode = getbytecode();
            if((ip + 1) >= (int)bytecode->count())
            {
            }
            else
            {
                hi = operand >> 8;
                bytecode->set(ip, hi);
                lo = operand;
                bytecode->set(ip + 1, lo);
            }
        }

        bool lastopcodeis(mcinternopcode_t op)
        {
            mcinternopcode_t lastopcode;
            lastopcode = getLastOpcode();
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
            if(symbol->m_symtype == AstSymbol::SYMTYP_MODULEGLOBAL)
            {
                opbuf[0] = symbol->m_symindex;
                ip = emitOpCode(OPCODE_GETMODULEGLOBAL, 1, opbuf);
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_GLOBALBUILTIN)
            {
                opbuf[0] = symbol->m_symindex;
                ip = emitOpCode(OPCODE_GETGLOBALBUILTIN, 1, opbuf);
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_LOCAL)
            {
                opbuf[0] = symbol->m_symindex;
                ip = emitOpCode(OPCODE_GETLOCAL, 1, opbuf);
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_FREE)
            {
                opbuf[0] = symbol->m_symindex;
                ip = emitOpCode(OPCODE_GETFREE, 1, opbuf);
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_FUNCTION)
            {
                ip = emitOpCode(OPCODE_CURRENTFUNCTION, 0, nullptr);
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_THIS)
            {
                ip = emitOpCode(OPCODE_GETTHIS, 0, nullptr);
            }
            return ip >= 0;
        }

        bool storesymbol(AstSymbol* symbol, bool define)
        {
            int ip;
            uint64_t opbuf[10];
            ip = -1;
            if(symbol->m_symtype == AstSymbol::SYMTYP_MODULEGLOBAL)
            {
                if(define)
                {
                    opbuf[0] = symbol->m_symindex;
                    ip = emitOpCode(OPCODE_DEFINEMODULEGLOBAL, 1, opbuf);
                }
                else
                {
                    opbuf[0] = symbol->m_symindex;
                    ip = emitOpCode(OPCODE_SETMODULEGLOBAL, 1, opbuf);
                }
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_LOCAL)
            {
                if(define)
                {
                    opbuf[0] = symbol->m_symindex;
                    ip = emitOpCode(OPCODE_DEFINELOCAL, 1, opbuf);
                }
                else
                {
                    opbuf[0] = symbol->m_symindex;
                    ip = emitOpCode(OPCODE_SETLOCAL, 1, opbuf);
                }
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_FREE)
            {
                opbuf[0] = symbol->m_symindex;
                ip = emitOpCode(OPCODE_SETFREE, 1, opbuf);
            }
            return ip >= 0;
        }

        bool pushbreakip(int ip)
        {
            AstScopeComp* compscope;
            compscope = getCompilationScope();
            return compscope->m_scopeipstackbreak.push(ip);
        }

        void popbreakip()
        {
            AstScopeComp* compscope;
            compscope = getCompilationScope();
            if(compscope->m_scopeipstackbreak.count() > 0)
            {
                compscope->m_scopeipstackbreak.pop(nullptr);
            }
        }

        int getbreakip()
        {
            int res;
            AstScopeComp* compscope;
            compscope = getCompilationScope();
            if(compscope->m_scopeipstackbreak.count() == 0)
            {
                return -1;
            }
            res = compscope->m_scopeipstackbreak.top();
            return res;
        }

        bool pushcontinueip(int ip)
        {
            AstScopeComp* compscope;
            compscope = getCompilationScope();
            return compscope->m_scopeipstackcontinue.push(ip);
        }

        void popcontinueip()
        {
            AstScopeComp* compscope;
            compscope = getCompilationScope();
            if(compscope->m_scopeipstackcontinue.count() == 0)
            {
                MC_ASSERT(false);
            }
            compscope->m_scopeipstackcontinue.pop(nullptr);
        }

        int getcontinueip()
        {
            int res;
            AstScopeComp* compscope;
            compscope = getCompilationScope();
            if(compscope->m_scopeipstackcontinue.count() == 0)
            {
                MC_ASSERT(false);
                return -1;
            }
            res = compscope->m_scopeipstackcontinue.top();
            return res;
        }

        int getip()
        {
            AstScopeComp* compscope;
            compscope = getCompilationScope();
            return compscope->m_scopecompiledbc.count();
        }

        GenericList<AstLocation>* getsrcpositions()
        {
            AstScopeComp* compscope;
            compscope = getCompilationScope();
            return &compscope->m_scopesrcposlist;
        }

        GenericList<uint16_t>* getbytecode()
        {
            AstScopeComp* compscope;
            compscope = getCompilationScope();
            return &compscope->m_scopecompiledbc;
        }

        bool filescopepush(const char* filepath)
        {
            bool ok;
            int globaloffset;
            AstScopeBlock* prevsttopscope;
            AstSymTable* prevst;
            AstSourceFile* file;
            (void)ok;
            prevst = nullptr;
            if(m_filescopelist.count() > 0)
            {
                prevst = getsymtable();
            }
            file = Memory::make<AstSourceFile>(filepath);
            ok = m_files->push(file);
            AstScopeFile filescope(m_config, m_ccerrlist, file);
            ok = m_filescopelist.push(filescope);
            globaloffset = 0;
            if(prevst != nullptr)
            {
                prevsttopscope = prevst->scopeBlockGet();
                globaloffset = prevsttopscope->m_blscopeoffset + prevsttopscope->m_blscopenumdefs;
            }
            ok = pushSymtable(globaloffset);
            return true;
        }

        void filescopepop()
        {
            int poppednumdefs;
            AstSymTable* poppedst;
            AstSymTable* currentst;
            AstScopeBlock* currentsttopscope;
            AstScopeBlock* poppedsttopscope;
            poppedst = getsymtable();
            poppedsttopscope = poppedst->scopeBlockGet();
            poppednumdefs = poppedsttopscope->m_blscopenumdefs;
            while(getsymtable() != nullptr)
            {
                popSymtable();
            }
            auto scope = m_filescopelist.topp();
            if(scope == nullptr)
            {
                MC_ASSERT(false);
            }
            scope->deInit();
            m_filescopelist.pop(nullptr);
            if(m_filescopelist.count() > 0)
            {
                currentst = getsymtable();
                currentsttopscope = currentst->scopeBlockGet();
                currentsttopscope->m_blscopenumdefs += poppednumdefs;
            }
        }

        void setcompilationscope(AstScopeComp* scope)
        {
            m_compilationscope = scope;
        }

        CompiledProgram* compilesource(const char* code, const char* filename)
        {
            bool ok;
            AstCompiler compshallowcopy;
            AstScopeComp* compscope;
            CompiledProgram* res;
            (void)ok;
            compscope = getCompilationScope();
            MC_ASSERT(m_srcposstack.count() == 0);
            MC_ASSERT(compscope->m_scopecompiledbc.count() == 0);
            MC_ASSERT(compscope->m_scopeipstackbreak.count() == 0);
            MC_ASSERT(compscope->m_scopeipstackcontinue.count() == 0);
            m_srcposstack.clear();
            compscope->m_scopecompiledbc.clear();
            compscope->m_scopesrcposlist.clear();
            compscope->m_scopeipstackbreak.clear();
            compscope->m_scopeipstackcontinue.clear();
            ok = filescopepush(filename);
            ok = initShallowCopy(&compshallowcopy, this);
            if(!ok)
            {
                return nullptr;
            }
            ok = doCompileSource(code);
            if(!ok)
            {
                goto compilefailed;
            }
            emitOpCode(OPCODE_HALT, 0, 0);
            /* might've changed */
            compscope = getCompilationScope();
            MC_ASSERT(compscope->m_outerscope == nullptr);
            compscope = getCompilationScope();
            res = compscope->orphanResult();
            if(res == nullptr)
            {
                goto compilefailed;
            }
            compshallowcopy.deinit();
            return res;
        compilefailed:
            deinit();
            *this = compshallowcopy;
            return nullptr;
        }

        AstSymTable* getsymtable()
        {
            auto filescope = m_filescopelist.topp();
            if(filescope == nullptr)
            {
                MC_ASSERT(false);
                return nullptr;
            }
            return filescope->m_scopefilesymtab;
        }

        void setsymtable(AstSymTable* table)
        {
            auto filescope = m_filescopelist.topp();
            if(filescope == nullptr)
            {
                MC_ASSERT(false);
            }
            filescope->m_scopefilesymtab = table;
        }

        GenericList<Value>* getconstants()
        {
            return &m_constants;
        }
};

class VMFrame
{
    public:
        Value m_function;
        int64_t m_bcposition = 0;
        int64_t m_basepointer = 0;
        AstLocation* m_framesrcposlist = nullptr;
        uint16_t* m_framebytecode = nullptr;
        int64_t m_sourcebcpos = 0;
        int64_t m_bcsize = 0;
        int64_t m_recoverip = 0;
        bool m_isrecovering = false;

    public:
        static inline bool init(VMFrame* frame, Value functionobj, int64_t baseptr)
        {
            Object::ObjFunction* function;
            if(functionobj.getType() != Value::VALTYP_FUNCSCRIPT)
            {
                return false;
            }
            function = Value::asFunction(functionobj);
            frame->m_function = functionobj;
            frame->m_bcposition = 0;
            frame->m_basepointer = baseptr;
            frame->m_sourcebcpos = 0;
            frame->m_framebytecode = function->m_funcdata.valscriptfunc.compiledprogcode->m_compiledbytecode;
            frame->m_framesrcposlist = function->m_funcdata.valscriptfunc.compiledprogcode->m_progsrcposlist;
            frame->m_bcsize = function->m_funcdata.valscriptfunc.compiledprogcode->m_compiledcount;
            frame->m_recoverip = -1;
            frame->m_isrecovering = false;
            return true;
        }

        inline uint16_t readUint8()
        {
            uint16_t data;
            data = m_framebytecode[m_bcposition];
            m_bcposition++;
            return data;
        }

        inline uint16_t readUint16()
        {
            uint16_t* data;
            data = m_framebytecode + m_bcposition;
            m_bcposition += 2;
            return (data[0] << 8) | data[1];
        }

        inline uint64_t readUint64()
        {
            uint64_t res;
            uint16_t* data;
            data = m_framebytecode + m_bcposition;
            m_bcposition += 8;
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

        inline AstCompiler::OpCode readOpCode()
        {
            m_sourcebcpos = m_bcposition;
            return (AstCompiler::OpCode)readUint8();
        }

        inline AstLocation getPosition()
        {
            if(m_framesrcposlist != nullptr)
            {
                return m_framesrcposlist[m_sourcebcpos];
            }
            return AstLocation::Invalid();
        }
};

class State
{
    public:
        enum
        {
            MaxOperOverloads = (25),
            MinValstackSize = (4),
            MinVMThisstackSize = (MinValstackSize),
            MinNativeThisstackSize = (32),
            MinVMGlobals = (4),
            MinVMFrames = (4),
        };

        struct ExecInfo
        {
            public:
                size_t thisstpos;
                size_t vsposition;
                Value lastpopped;
                VMFrame* currframe;
                GenericList<Value> valuestack = GenericList<Value>(MinValstackSize, Value::makeNull());
                GenericList<Value> valthisstack = GenericList<Value>(MinVMThisstackSize, Value::makeNull());
                GenericList<Value> nativethisstack = GenericList<Value>(MinNativeThisstackSize, Value::makeNull());
                GenericList<VMFrame> framestack = GenericList<VMFrame>(MinVMFrames, VMFrame{});

            public:
                void deInit()
                {
                    this->valuestack.deInit();
                    this->framestack.deInit();
                    this->valthisstack.deInit();
                    this->nativethisstack.deInit();
                }
        };

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
        RuntimeConfig m_config;
        ErrList m_errorlist;
        GCMemory* m_stategcmem;
        SymStore* m_vmglobalstore;
        GenericList<Value> m_globalvalstack = GenericList<Value>(MinVMGlobals, Value::makeNull());;
        size_t m_globalvalcount;
        bool m_hadrecovered;
        bool m_running;
        Value m_operoverloadkeys[MaxOperOverloads];
        GenericList<AstSourceFile*> m_sharedfilelist;
        AstCompiler* m_sharedcompiler;
        ExecInfo m_execstate;
        Printer* m_stdoutprinter;
        Printer* m_stderrprinter;
        ObjClass* m_stdobjectobject;
        ObjClass* m_stdobjectnumber;
        ObjClass* m_stdobjectstring;
        ObjClass* m_stdobjectarray;
        ObjClass* m_stdobjectmap;
        ObjClass* m_stdobjectfunction;

    public:
        State()
        {
            setDefaultConfig();
            m_stategcmem = Memory::make<GCMemory>();
            if(m_stategcmem == nullptr)
            {
                goto err;
            }
            mc_vm_init(this);
            m_stdoutprinter = Memory::make<Printer>(stdout);
            m_stderrprinter = Memory::make<Printer>(stderr);
            m_vmglobalstore = Memory::make<SymStore>();
            m_sharedcompiler = Memory::make<AstCompiler>(this, &m_config, m_stategcmem, &m_errorlist, &m_sharedfilelist, m_vmglobalstore, m_stderrprinter);
            mc_state_makestdclasses(this);
            return;
        err:
            deinit();
            MC_ASSERT(false);
        }

        void deinit()
        {
            Memory::destroy(m_sharedcompiler);
            Memory::destroy(m_vmglobalstore);
            Memory::destroy(m_stategcmem);
            m_sharedfilelist.deInit(AstSourceFile::destroy);
            m_errorlist.destroy();
            Printer::destroy(m_stdoutprinter);
            Printer::destroy(m_stderrprinter);
            m_globalvalstack.deInit();
            m_execstate.deInit();
            Memory::destroy(m_stdobjectobject);
            Memory::destroy(m_stdobjectnumber);
            Memory::destroy(m_stdobjectstring);
            Memory::destroy(m_stdobjectarray);
            Memory::destroy(m_stdobjectmap);
            Memory::destroy(m_stdobjectfunction);
        }

        void reset()
        {
            errorsClear();
            mc_vm_reset(this);
        }

        void setDefaultConfig()
        {
            m_config.replmode = false;
            m_config.dumpast = false;
            m_config.exitafterastdump = false;
            m_config.dumpbytecode = false;
            m_config.fatalcomplaints = false;
            m_config.exitaftercompiling = false;
            m_config.printinstructions = false;
            m_config.strictmode = false;
        }

        template<typename... ArgsT>
        void pushError(Error::Type type, AstLocation pos, const char* fmt, ArgsT&&... args)
        {
            m_errorlist.pushFormat(type, pos, fmt, args...);
        }

        template<typename... ArgsT>
        void pushRuntimeError(const char* fmt, ArgsT&&... args)
        {
            pushError(Error::ERRTYP_RUNTIME, AstLocation::Invalid(), fmt, args...);
        }

        bool runExecFunc(CompiledProgram* comp_res, GenericList<Value>* constants)
        {
            bool res;
            size_t oldsp;
            size_t oldthissp;
            size_t oldframescount;
            Value mainfn;
            (void)oldsp;
            oldsp = m_execstate.vsposition;
            oldthissp = m_execstate.thisstpos;
            oldframescount = m_execstate.framestack.count();
            mainfn = mc_value_makefuncscript(this, "__main__", comp_res, false, 0, 0, 0);
            if(mainfn.isNull())
            {
                return false;
            }
            vmStackPush(mainfn);
            res = execVM(mainfn, constants, false);
            while(m_execstate.framestack.count() > oldframescount)
            {
                vmPopFrame();
            }
            //MC_ASSERT(m_execstate.vsposition == oldsp);
            m_execstate.thisstpos = oldthissp;
            return res;
        }

        Value execProgram(CompiledProgram* program)
        {
            bool ok;
            Value res;
            (void)ok;
            if(program == nullptr)
            {
                m_errorlist.pushFormat(Error::ERRTYP_USER, AstLocation::Invalid(), "program passed to execute was null.");
                return Value::makeNull();
            }
            reset();
            ok = runExecFunc(program, m_sharedcompiler->getconstants());
            if(!ok || m_errorlist.count() > 0)
            {
                return Value::makeNull();
            }
            //MC_ASSERT(m_execstate.vsposition == 0);
            res = m_execstate.lastpopped;
            if(res.getType() == Value::VALTYP_NONE)
            {
                return Value::makeNull();
            }
            return res;
        }


        bool setGlobalFunction(const char* name, mcnativefn_t fn, void* data)
        {
            Value obj;
            obj = mc_value_makefuncnative(this, name, fn, data);
            if(obj.isNull())
            {
                return false;
            }
            return setGlobalValue(name, obj);
        }

        inline bool setGlobalValue(const char* name, Value obj)
        {
            return m_vmglobalstore->setNamed(name, obj);
        }

        inline Value getGlobalByName(const char* name)
        {
            bool ok;
            Value res;
            AstSymbol* symbol;
            AstSymTable* st;
            (void)ok;
            st = m_sharedcompiler->getsymtable();
            symbol = st->resolve(name);
            if(symbol == nullptr)
            {
                pushError(Error::ERRTYP_USER, AstLocation::Invalid(), "symbol \"%s\" is not defined", name);
                return Value::makeNull();
            }
            res = Value::makeNull();
            if(symbol->m_symtype == AstSymbol::SYMTYP_MODULEGLOBAL)
            {
                res = getGlobalByIndex(symbol->m_symindex);
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_GLOBALBUILTIN)
            {
                ok = false;
                res = m_vmglobalstore->getAtIndex(symbol->m_symindex, &ok);
                if(!ok)
                {
                    pushError(Error::ERRTYP_USER, AstLocation::Invalid(), "failed to get global object at %d", symbol->m_symindex);
                    return Value::makeNull();
                }
            }
            else
            {
                pushError(Error::ERRTYP_USER, AstLocation::Invalid(), "value associated with symbol \"%s\" could not be loaded", name);
                return Value::makeNull();
            }
            return res;
        }

        //inserthere
        inline void saveExecInfo(ExecInfo* est)
        {
            est->thisstpos = m_execstate.thisstpos;
            est->vsposition = m_execstate.vsposition;
            est->currframe = m_execstate.currframe;
        }


        inline void restoreExecInfo(ExecInfo* est)
        {
            m_execstate.thisstpos = est->thisstpos;
            m_execstate.vsposition = est->vsposition;
            m_execstate.currframe = est->currframe;
            est->deInit();
        }

        inline bool setGlobalByIndex(size_t ix, Value val)
        {
            m_globalvalstack.set(ix, val);
            if(ix >= m_globalvalcount)
            {
                m_globalvalcount = ix + 1;
            }
            return true;
        }

        inline Value getGlobalByIndex(size_t ix)
        {
            return m_globalvalstack.get(ix);
        }

        inline void setStackPos(size_t nsp)
        {
            #if 1
            size_t i;
            size_t count;
            size_t bytescount;
            if(nsp > m_execstate.vsposition)
            {
                /* to avoid gcing freed objects */
                count = nsp - m_execstate.vsposition;
                bytescount = (count - 0) * sizeof(Value);
                for(i=(m_execstate.vsposition - 0); (i != bytescount) && (i < m_execstate.valuestack.m_listcapacity); i++)
                {
                    //memset(&m_execstate.valuestack.m_listitems[i], 0, sizeof(Value));
                    m_execstate.valuestack.m_listitems[i].m_valtype = Value::VALTYP_NULL;
                }
            }
            #endif
            m_execstate.vsposition = nsp;
        }

        inline void vmStackPush(Value obj)
        {
            int numlocals;
            VMFrame* frame;
            Object::ObjFunction* currentfunction;
            (void)numlocals;
            (void)frame;
            (void)currentfunction;
        #if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
            if(m_execstate.currframe)
            {
                frame = m_execstate.currframe;
                currentfunction = Value::asFunction(frame->m_function);
                numlocals = currentfunction->m_funcdata.valscriptfunc.numlocals;
                MC_ASSERT((size_t)m_execstate.vsposition >= (size_t)(frame->m_basepointer + numlocals));
            }
        #endif
            m_execstate.valuestack.set(m_execstate.vsposition, obj);
            m_execstate.vsposition++;
        }

        inline Value vmStackPop()
        {
            int numlocals;
            Value res;
            VMFrame* frame;
            Object::ObjFunction* currentfunction;
            (void)numlocals;
            (void)frame;
            (void)currentfunction;
        #if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
            if(m_execstate.vsposition == 0)
            {
                m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "stack underflow");
                MC_ASSERT(false);
                return Value::makeNull();
            }
            if(m_execstate.currframe)
            {
                frame = m_execstate.currframe;
                currentfunction = Value::asFunction(frame->m_function);
                numlocals = currentfunction->m_funcdata.valscriptfunc.numlocals;
                MC_ASSERT((m_execstate.vsposition - 1) >= (frame->m_basepointer + numlocals));
            }
        #endif
            m_execstate.vsposition--;
            res = m_execstate.valuestack.get(m_execstate.vsposition);
            m_execstate.lastpopped = res;
            return res;
        }

        inline Value vmStackGet(size_t nthitem)
        {
            size_t ix;
            ix = m_execstate.vsposition - 1 - nthitem;
            return m_execstate.valuestack.get(ix);
        }

        inline void vmStackThisPush(Value obj)
        {
            m_execstate.valthisstack.set(m_execstate.thisstpos, obj);
            m_execstate.thisstpos++;
        }

        inline Value vmStackThisPop()
        {
        #if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
            if(m_execstate.thisstpos == 0)
            {
                errorspushFormat(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "'this' stack underflow");
                MC_ASSERT(false);
                return Value::makeNull();
            }
        #endif
            m_execstate.thisstpos--;
            return m_execstate.valthisstack.get(m_execstate.thisstpos);
        }

        inline Value vmStackThisGet(size_t nthitem)
        {
            size_t ix;
            size_t cnt;
            Value val;
            (void)val;
            ix = m_execstate.thisstpos - 1 - nthitem;
            cnt = m_execstate.valthisstack.count();
            if((cnt == 0) || (ix > cnt))
            {
                //val = mc_value_makemap(this);
                //m_execstate.valthisstack.set(ix, val);
                //return mc_value_makemap(this);
                return Value::makeNull();
                //return val;
            }
            return m_execstate.valthisstack.get(ix);
        }

        inline bool vmPushFrame(VMFrame frame)
        {
            int add;
            Object::ObjFunction* framefunction;
            add = 0;
            m_execstate.framestack.set(m_execstate.framestack.m_listcount, frame);
            add = 1;
            m_execstate.currframe = m_execstate.framestack.getp(m_execstate.framestack.m_listcount);
            m_execstate.framestack.m_listcount += add;
            framefunction = Value::asFunction(frame.m_function);
            setStackPos(frame.m_basepointer + framefunction->m_funcdata.valscriptfunc.numlocals);
            return true;
        }

        inline bool vmPopFrame()
        {
            setStackPos(m_execstate.currframe->m_basepointer - 1);
            if(m_execstate.framestack.m_listcount <= 0)
            {
                MC_ASSERT(false);
                m_execstate.currframe = NULL;
                return false;
            }
            m_execstate.framestack.m_listcount--;
            if(m_execstate.framestack.m_listcount == 0)
            {
                m_execstate.currframe = NULL;
                return false;
            }
            m_execstate.currframe = m_execstate.framestack.getp(m_execstate.framestack.m_listcount - 1);
            return true;
        }

        bool vmTracebackPush(Traceback* traceback)
        {
            bool ok;
            int i;
            VMFrame* frame;
            (void)ok;
            for(i = m_execstate.framestack.count() - 1; i >= 0; i--)
            {
                frame = m_execstate.framestack.getp(i);
                ok = traceback->push(mc_value_functiongetname(frame->m_function), frame->getPosition());
                if(!ok)
                {
                    return false;
                }
            }
            return true;
        }

        inline Value vmCallNativeFunction(Value callee, AstLocation srcpos, Value selfval, size_t argc, Value* args)
        {
            Value::Type restype;
            Value res;
            Error* err; 
            Traceback* traceback;
            Object::ObjFunction* nativefun;
            nativefun = Value::asFunction(callee);
            res = nativefun->m_funcdata.valnativefunc.natptrfn(this, nativefun->m_funcdata.valnativefunc.userpointer, selfval, argc, args);
            if(mc_util_unlikely(m_errorlist.count() > 0))
            {
                err = m_errorlist.getLast();
                err->m_pos = srcpos;
                err->m_traceback = Memory::make<Traceback>();
                if(err->m_traceback != nullptr)
                {
                    err->m_traceback->push(nativefun->m_funcdata.valnativefunc.natfnname, AstLocation::Invalid());
                }
                return Value::makeNull();
            }
            restype = res.getType();
            if(mc_util_unlikely(restype == Value::VALTYP_ERROR))
            {
                traceback = Memory::make<Traceback>();
                if(traceback != nullptr)
                {
                    /* error builtin is treated in a special way */
                    if(!mc_util_strequal(nativefun->m_funcdata.valnativefunc.natfnname, "error"))
                    {
                        traceback->push(nativefun->m_funcdata.valnativefunc.natfnname, AstLocation::Invalid());
                    }
                    vmTracebackPush(traceback);
                    mc_value_errorsettraceback(res, traceback);
                }
            }
            return res;
        }

        inline Value vmCallValue(GenericList<Value>* constants, Value callee, Value thisval, size_t argc, Value* args)
        {
            bool ok;
            size_t i;
            size_t oldsp;
            size_t oldthissp;
            size_t oldframescount;
            VMFrame tempframe;
            Value retv;
            ExecInfo est;
            Value::Type type;
            (void)oldsp;
            (void)oldframescount;
            if(constants == nullptr)
            {
                constants = m_sharedcompiler->getconstants();
            }
            type = callee.getType();
            if(type == Value::VALTYP_FUNCSCRIPT)
            {
                //VMFrame::init(&tempframe, callee, 0);
                VMFrame::init(&tempframe, callee, m_execstate.vsposition - argc);
                saveExecInfo(&est);
                oldsp = m_execstate.vsposition;
                oldthissp = m_execstate.thisstpos;
                vmPushFrame(tempframe);
                oldframescount = m_execstate.framestack.count();
                vmStackPush(callee);
                for(i = 0; i < argc; i++)
                {
                    vmStackPush(args[i]);
                }
                ok = execVM(callee, constants, true);
                if(!ok)
                {
                    restoreExecInfo(&est);
                    return Value::makeNull();
                }
                #if 1
                while(m_execstate.framestack.count() > oldframescount)
                {
                    vmPopFrame();
                }
                #endif
                m_execstate.thisstpos = oldthissp;
                retv = m_execstate.lastpopped;
                restoreExecInfo(&est);
                return retv;
            }
            if(type == Value::VALTYP_FUNCNATIVE)
            {
                return vmCallNativeFunction(callee, AstLocation::Invalid(), thisval, argc, args);
            }
            m_errorlist.pushFormat(Error::ERRTYP_USER, AstLocation::Invalid(), "object is not callable");
            return Value::makeNull();
        }

        inline bool vmCallObject(Value callee, int nargs)
        {
            bool ok;
            const char* calleetypename;
            Value::Type calleetype;
            VMFrame calleeframe;
            Value res;
            Value tmpval;
            Value selfval;
            Value* stackpos;
            Object::ObjFunction* calleefunction;
            (void)ok;
            calleetype = callee.getType();
            selfval = Value::makeNull();
            if(callee.isFuncNative())
            {
                if(!m_execstate.nativethisstack.pop(&tmpval))
                {
                    #if 0
                        m_stderrprinter->format("failed to pop native 'this' for = <");
                        mc_printer_printvalue(m_stderrprinter, callee, true);
                        m_stderrprinter->format(">\n");
                        #if 0
                            m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "failed to pop native 'this'");
                        #endif
                    #endif
                }
                selfval = tmpval;
            }
            #if 0
            {
                m_stderrprinter->format("selfval = <<<");
                mc_printer_printvalue(m_stderrprinter, selfval, true);
                m_stderrprinter->format(">>>\n");
            }
            #endif
            if(calleetype == Value::VALTYP_FUNCSCRIPT)
            {
                calleefunction = Value::asFunction(callee);
                if(nargs != calleefunction->m_funcdata.valscriptfunc.numargs)
                {
                    #if 0
                    m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "invalid number of arguments to \"%s\": expected %d, got %d",
                                      mc_value_functiongetname(callee), calleefunction->m_funcdata.valscriptfunc.numargs, nargs);
                    return false;
                    #endif
                }
                ok = VMFrame::init(&calleeframe, callee, m_execstate.vsposition - nargs);
                if(!ok)
                {
                    m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, AstLocation::Invalid(), "frame init failed in vmCallObject");
                    return false;
                }
                ok = vmPushFrame(calleeframe);
                if(!ok)
                {
                    m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, AstLocation::Invalid(), "pushing frame failed in vmCallObject");
                    return false;
                }
            }
            else if(calleetype == Value::VALTYP_FUNCNATIVE)
            {
                #if 0
                if(!selfval.isNull())
                {
                    vmStackPop();
                }
                #endif
                stackpos = m_execstate.valuestack.data() + m_execstate.vsposition - nargs;
                res = vmCallNativeFunction(callee, m_execstate.currframe->getPosition(), selfval, nargs, stackpos);
                if(hasErrors())
                {
                    return false;
                }
                setStackPos(m_execstate.vsposition - nargs - 1);
                vmStackPush(res);
            }
            else
            {
                calleetypename = Value::getTypename(calleetype);
                m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "%s object is not callable", calleetypename);
                return false;
            }
            return true;
        }

        bool vmTryOverloadOperator(Value left, Value right, mcinternopcode_t op, bool* outoverloadfound)
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
            if(op == AstCompiler::OPCODE_MINUS || op == AstCompiler::OPCODE_BINNOT || op == AstCompiler::OPCODE_BANG)
            {
                numoper = 1;
            }
            key = m_operoverloadkeys[op];
            callee = Value::makeNull();
            if(lefttype == Value::VALTYP_MAP)
            {
                callee = mc_state_mapgetvalue(left, key);
            }
            if(!callee.isCallable())
            {
                if(righttype == Value::VALTYP_MAP)
                {
                    callee = mc_state_mapgetvalue(right, key);
                }

                if(!callee.isCallable())
                {
                    *outoverloadfound = false;
                    return true;
                }
            }
            *outoverloadfound = true;
            vmStackPush(callee);
            vmStackPush(left);
            if(numoper == 2)
            {
                vmStackPush(right);
            }
            return vmCallObject(callee, numoper);
        }


        inline Value callFunctionByName(const char* fname, Value thisval, size_t argc, Value* args)
        {
            Value res;
            Value callee;
            reset();
            callee = getGlobalByName(fname);
            if(callee.getType() == Value::VALTYP_NULL)
            {
                return Value::makeNull();
            }
            res = vmCallValue(m_sharedcompiler->getconstants(), callee, thisval, argc, args);
            if(m_errorlist.count() > 0)
            {
                return Value::makeNull();
            }
            return res;
        }

        inline bool vmCheckAssign(Value oldvalue, Value nvalue)
        {
            return true;
            Value::Type nvaluetype;
            Value::Type oldvaluetype;
            oldvaluetype = oldvalue.getType();
            nvaluetype = nvalue.getType();
            if(oldvaluetype == Value::VALTYP_NULL || nvaluetype == Value::VALTYP_NULL)
            {
                return true;
            }
            #if 0
            if(oldvaluetype != nvaluetype)
            {
                pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "trying to assign variable of type %s to %s",
                                  Value::getTypename(nvaluetype), Value::getTypename(oldvaluetype));
                return false;
            }
            #endif
            return true;
        }

        inline bool vmOpAddString(Value valleft, Value valright, Value::Type righttype, AstCompiler::OpCode opcode)
        {
            Value nstring;
            (void)opcode;
            (void)righttype;
            nstring = mc_value_makestrcapacity(this, 0);
            mc_value_stringappendvalue(nstring, valleft);
            mc_value_stringappendvalue(nstring, valright);
            vmStackPush(nstring);
            return true;
        }

        inline bool vmOpMath(AstCompiler::OpCode opcode)
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
            valright = vmStackPop();
            valleft = vmStackPop();
            lefttype = valleft.getType();
            righttype = valright.getType();
            if(lefttype == Value::VALTYP_STRING && opcode == AstCompiler::OPCODE_ADD)
            {
                if(vmOpAddString(valleft, valright, righttype, opcode))
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
                    case AstCompiler::OPCODE_ADD:
                        {
                            res = mc_mathutil_add(dnleft, dnright);
                        }
                        break;
                    case AstCompiler::OPCODE_SUB:
                        {
                            res = mc_mathutil_sub(dnleft, dnright);
                        }
                        break;
                    case AstCompiler::OPCODE_MUL:
                        {
                            res = mc_mathutil_mult(dnleft, dnright);
                        }
                        break;
                    case AstCompiler::OPCODE_DIV:
                        {
                            res = mc_mathutil_div(dnleft, dnright);
                        }
                        break;
                    case AstCompiler::OPCODE_MOD:
                        {
                            res = mc_mathutil_mod(dnleft, dnright);
                        }
                        break;
                    case AstCompiler::OPCODE_BINOR:
                        {
                            res = mc_mathutil_binor(dnleft, dnright);
                        }
                        break;
                    case AstCompiler::OPCODE_BINXOR:
                        {
                            res = mc_mathutil_binxor(dnleft, dnright);
                        }
                        break;
                    case AstCompiler::OPCODE_BINAND:
                        {
                            res = mc_mathutil_binand(dnleft, dnright);
                        }
                        break;
                    /*
                    // TODO: shifting, signedness: how does nodejs do it?
                    // enabling checks for <0 breaks sha1.mc!
                    */
                    case AstCompiler::OPCODE_LSHIFT:
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
                    case AstCompiler::OPCODE_RSHIFT:
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
                vmStackPush(Value::makeNumber(res));
                return true;
            }
            overloadfound = false;
            ok = vmTryOverloadOperator(valleft, valright, opcode, &overloadfound);
            if(!ok)
            {
                return false;
            }
            if(!overloadfound)
            {
                opcodename = AstCompiler::opdefGetName(opcode);
                lefttypename = Value::getTypename(lefttype);
                righttypename = Value::getTypename(righttype);
                pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "invalid operand types for %s: got %s and %s",
                                  opcodename, lefttypename, righttypename);
                return false;
            }
            return true;
        }

        inline ObjClass* findClassForIntern(Value::Type typ)
        {
            (void)typ;
            switch(typ)
            {
                case Value::VALTYP_NUMBER:
                    {
                        return m_stdobjectnumber;
                    }
                    break;
                case Value::VALTYP_STRING:
                    {
                        return m_stdobjectstring;
                    }
                    break;
                case Value::VALTYP_ARRAY:
                    {
                        return m_stdobjectarray;
                    }
                    break;
                case Value::VALTYP_MAP:
                    {
                        return m_stdobjectmap;
                    }
                    break;
                case Value::VALTYP_FUNCNATIVE:
                case Value::VALTYP_FUNCSCRIPT:
                    {
                        return m_stdobjectfunction;
                    }
                    break;
                default:
                    {
                    }
                    break;
            }
            return m_stdobjectobject;
        }

        inline ObjClass* vmFindClassFor(Value::Type typ)
        {
            ObjClass* cl;
            cl = findClassForIntern(typ);
            if(cl != nullptr)
            {
                return cl;
            }
            return nullptr;
        }

        inline ObjClass::Field* vmGetClassMember(ObjClass* cl, const char* name)
        {
            size_t i;
            for(i=0; i<cl->m_memberfields.count(); i++)
            {
                auto memb = cl->m_memberfields.getp(i);
                if(strcmp(memb->name, name) == 0)
                {
                    return memb;
                }
            }
            if(cl->m_parentclass != nullptr)
            {
                return vmGetClassMember(cl->m_parentclass, name);
            }
            return nullptr;
        }

        inline bool vmFindClassmemberValue(Value left, Value index, Value setval)
        {
            Value fnval;
            Value retv;
            const char* idxname;
            ObjClass::Field* vdest;
            (void)left;
            (void)index;
            (void)setval;
            ObjClass* cl;
            cl = vmFindClassFor(left.getType());
            if(cl != nullptr)
            {
                idxname = mc_value_stringgetdata(index);
                vdest = vmGetClassMember(cl, idxname);
                if(vdest == nullptr)
                {
                    return false;
                }
                else
                {
                    fnval = mc_value_makefuncnative(this, vdest->name, vdest->fndest, nullptr);
                    if(vdest->ispseudo)
                    {
                        retv = vmCallNativeFunction(fnval, m_execstate.currframe->getPosition(), left, 0, nullptr);
                        vmStackPush(retv);
                        return true;
                    }
                    else
                    {
                        retv = fnval;
                        vmStackPush(retv);
                        return true;
                    }
                }
            }
            return false;
        }

        inline bool vmGetIndexPartial(Value left, Value::Type lefttype, Value index, Value::Type indextype, bool fromdot)
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
                if(mc_state_mapgetvaluechecked(left, index, &res))
                {
                    goto finished;
                }
            }
            if(index.isString())
            {
                if(vmFindClassmemberValue(left, index, Value::makeNull()))
                {
                    #if 0
                    if(callee.isFuncNative())
                    #endif
                    {
                        m_execstate.nativethisstack.push(left);
                    }
                    #if 0
                        m_stderrprinter->format("getindexpartial:left=<");
                        mc_printer_printvalue(m_stderrprinter, left, true);
                        m_stderrprinter->format(">\n");
                    #endif
                    return true;
                }
                else
                {
                    if(!left.isMap())
                    {
                        res = Value::makeNull();
                        lefttypename = Value::getTypename(lefttype);
                        pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "object type '%s' has no field '%s'", lefttypename, mc_value_stringgetdata(index));
                        vmStackPush(res);
                        return false;
                    }
                }
            }
            if(lefttype != Value::VALTYP_ARRAY && lefttype != Value::VALTYP_MAP && lefttype != Value::VALTYP_STRING)
            {
                pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "getindexpartial: type %s is not indexable", lefttypename);

                return false;
            }
            res = Value::makeNull();
            if(lefttype == Value::VALTYP_ARRAY)
            {
                if(indextype != Value::VALTYP_NUMBER)
                {
                    lefttypename = Value::getTypename(lefttype);
                    indextypename = Value::getTypename(indextype);
                    pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "cannot get partial index of %s with %s", lefttypename, indextypename);
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
                    pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "cannot index %s with %s", lefttypename, indextypename);
                    return false;
                }
                str = mc_value_stringgetdata(left);
                leftlen = mc_value_stringgetlength(left);
                ix = (int)Value::asNumber(index);
                if(ix >= 0 && ix < leftlen)
                {
                    resstr[0] = str[ix];
                    res = mc_value_makestringlen(this, resstr, 1);
                }
            }
            finished:
            vmStackPush(res);
            return true;
        }

        inline bool vmGetIndexFull()
        {
            Value::Type lefttype;
            Value::Type indextype;
            Value left;
            Value index;
            index = vmStackPop();
            left = vmStackPop();
            lefttype = left.getType();
            indextype = index.getType();
            return vmGetIndexPartial(left, lefttype, index, indextype, false);
        }

        inline bool vmGetDotIndex()
        {
            Value::Type lefttype;
            Value::Type indextype;
            Value left;
            Value index;
            index = vmStackPop();
            left = vmStackPop();
            lefttype = left.getType();
            indextype = index.getType();
            return vmGetIndexPartial(left, lefttype, index, indextype, true);
        }

        inline bool setIndexPartial(Value left, Value::Type lefttype, Value index, Value::Type indextype, Value nvalue)
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
                pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "setindexpartial: type %s is not indexable", lefttypename);
                return false;
            }
            if(lefttype == Value::VALTYP_ARRAY)
            {
                if(indextype != Value::VALTYP_NUMBER)
                {
                    lefttypename = Value::getTypename(lefttype);
                    indextypename = Value::getTypename(indextype);
                    pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "cannot set index of %s with %s", lefttypename, indextypename);
                    return false;
                }
                ix = (int)Value::asNumber(index);                        
                ok = mc_value_arraysetvalue(left, ix, nvalue);
                alen = mc_value_arraygetlength(left);
                if(!ok)
                {
                    pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "failed to set array index %d (of %d)", ix, alen);
                    return false;
                }
            }
            else if(lefttype == Value::VALTYP_MAP)
            {
                oldvalue = mc_state_mapgetvalue(left, index);
                if(!vmCheckAssign(oldvalue, nvalue))
                {
                    return false;
                }
                ok = mc_state_mapsetvalue(left, index, nvalue);
                if(!ok)
                {
                    return false;
                }
            }
            return true;
        }

        inline bool vmSetIndexFull()
        {
            Value index;
            Value left;
            Value nvalue;
            Value::Type lefttype;
            Value::Type indextype;
            index = vmStackPop();
            left = vmStackPop();
            nvalue = vmStackPop();
            lefttype = left.getType();
            indextype = index.getType();
            return setIndexPartial(left, lefttype, index, indextype, nvalue);
        }

        inline bool vmGetValueAtFull()
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
            index = vmStackPop();
            left = vmStackPop();
            lefttype = left.getType();
            indextype= index.getType();
            if(lefttype != Value::VALTYP_ARRAY && lefttype != Value::VALTYP_MAP && lefttype != Value::VALTYP_STRING)
            {
                lefttypename = Value::getTypename(lefttype);
                pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "getvalueatfull: type %s is not indexable", lefttypename);
                return false;
            }
            res = Value::makeNull();
            if(indextype != Value::VALTYP_NUMBER)
            {
                lefttypename = Value::getTypename(lefttype);
                indextypename = Value::getTypename(indextype);
                pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "cannot get full index %s with %s", lefttypename, indextypename);
                return false;
            }
            ix = (int)Value::asNumber(index);
            if(lefttype == Value::VALTYP_ARRAY)
            {
                res = mc_value_arraygetvalue(left, ix);
            }
            else if(lefttype == Value::VALTYP_MAP)
            {
                res = mc_state_mapgetkvpairat(this, left, ix);
            }
            else if(lefttype == Value::VALTYP_STRING)
            {
                str = mc_value_stringgetdata(left);
                leftlen = mc_value_stringgetlength(left);
                ix = (int)Value::asNumber(index);
                if(ix >= 0 && ix < leftlen)
                {
                    resstr[0] = str[ix];
                    res = mc_value_makestringlen(this, resstr, 1);
                }
            }
            vmStackPush(res);
            return true;
        }

        // xxhere


        inline bool hasErrors()
        {
            return errorCount() > 0;
        }

        inline int errorCount()
        {
            return m_errorlist.count();
        }

        inline void errorsClear()
        {
            m_errorlist.clear();
        }

        inline Error* errorGet(int index)
        {
            return m_errorlist.get(index);
        }

        void printErrors()
        {
            int i;
            int ecnt;
            Error* err;
            ecnt = errorCount();
            for(i = 0; i < ecnt; i++)
            {
                err = errorGet(i);
                err->printErrorTo(m_stderrprinter);
            }
        }

        CompiledProgram* compileSource(const char* code, const char* filename)
        {
            CompiledProgram* compres;
            errorsClear();
            compres = m_sharedcompiler->compilesource(code, filename);
            if(m_errorlist.count() > 0)
            {
                goto err;
            }
            return compres;
        err:
            CompiledProgram::destroy(compres);
            return nullptr;
        }

        bool execVM(Value function, GenericList<Value>* constants, bool nested);


};

// bottom

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
    if(fp == nullptr)
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
    if(outputstring == nullptr)
    {
        return nullptr;
    }
    outputstring[n] = '\0';
    memcpy(outputstring, string, n);
    return outputstring;
}

char* mc_util_strdup(const char* string)
{
    if(string == nullptr)
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

GenericList<void*>* mc_util_splitstring(const char* str, const char* delimiter)
{
    bool ok;
    size_t i;
    long len;
    char* rest;
    char* line;
    const char* lineend;
    const char* linestart;
    (void)ok;
    auto res = Memory::make<GenericList<void*>>(0, nullptr);
    rest = nullptr;
    if(str == nullptr)
    {
        return res;
    }
    linestart = str;
    lineend = strstr(linestart, delimiter);
    while(lineend != nullptr)
    {
        len = lineend - linestart;
        line = mc_util_strndup(linestart, len);
        if(line == nullptr)
        {
            goto err;
        }
        ok = res->push(line);
        linestart = lineend + 1;
        lineend = strstr(linestart, delimiter);
    }
    rest = mc_util_strdup(linestart);
    if(rest == nullptr)
    {
        goto err;
    }
    ok = res->push(rest);
    return res;
err:
    mc_memory_free(rest);
    if(res != nullptr)
    {
        for(i = 0; i < res->count(); i++)
        {
            line = (char*)res->get(i);
            mc_memory_free(line);
        }
    }
    Memory::destroy(res);
    return nullptr;
}

char* mc_util_joinstringarray(GenericList<void*>* items, const char* joinee, size_t jlen)
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
    if((strchr(strpath, '/') == nullptr) || ((strstr(strpath, "/../") == nullptr) && (strstr(strpath, "./") == nullptr)))
    {
        return mc_util_strdup(strpath);
    }
    auto split = mc_util_splitstring(strpath, "/");
    if(split == nullptr)
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
    Memory::destroy(split);
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
    return stod_strtod(&p, end, 1);
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
    if(pos.m_locfile != nullptr)
    {
        if(pos.m_locfile->path() != nullptr)
        {
            fname = pos.m_locfile->path();
        }
        nline = pos.m_locline;
        ncol = pos.m_loccolumn;
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

Object* mc_gcmemory_getdatafrompool(State* state, Value::Type type)
{
    bool ok;
    Object* data;
    GCMemory::DataPool* pool;
    (void)ok;
    pool = state->m_stategcmem->getPoolForType(type);
    if((pool == nullptr) || pool->m_poolitemcount <= 0)
    {
        return nullptr;
    }
    data = pool->m_pooldata.get(pool->m_poolitemcount - 1);
    MC_ASSERT(state->m_stategcmem->m_gcobjlistback->count() >= state->m_stategcmem->m_gcobjliststored->count());
    /*
    * we want to make sure that appending to m_gcobjlistback never fails in sweep
    * so this only reserves space there.
    */
    ok = state->m_stategcmem->m_gcobjlistback->push(data);
    ok = state->m_stategcmem->m_gcobjliststored->push(data);
    pool->m_poolitemcount--;
    return data;
}

Value mc_value_makestrcapacity(State* state, int capacity)
{
    Object* data;
    data = mc_gcmemory_getdatafrompool(state, Value::VALTYP_STRING);
    if(data == nullptr)
    {
        data = mc_gcmemory_allocobjectdata(state);
        if(data == nullptr)
        {
            return Value::makeNull();
        }
    }
    data->m_uvobj.valstring.hash = 0;
    data->m_uvobj.valstring.strbuf = Memory::make<StringBuffer>(capacity);
    return Value::makeDataFrom(Value::VALTYP_STRING, data);
}

template<typename... ArgsT>
Value mc_value_makestrformat(State* state, const char* fmt, ArgsT&&... args)
{
    Value res;
    Object* data;
    data = mc_gcmemory_getdatafrompool(state, Value::VALTYP_STRING);
    res = mc_value_makestrcapacity(state, 0);
    if(res.isNull())
    {
        return Value::makeNull();
    }
    data->m_uvobj.valstring.strbuf->appendFormat(fmt, args...);
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
    Object* obj;
    obj = mc_gcmemory_allocobjectdata(state);
    if(obj == nullptr)
    {
        return Value::makeNull();
    }
    obj->m_uvobj.valfunc.m_funcdata.valnativefunc.natfnname = mc_util_strdup(name);
    if(obj->m_uvobj.valfunc.m_funcdata.valnativefunc.natfnname == nullptr)
    {
        return Value::makeNull();
    }
    obj->m_uvobj.valfunc.m_funcdata.valnativefunc.natptrfn = fn;
    if(data != nullptr)
    {
        obj->m_uvobj.valfunc.m_funcdata.valnativefunc.userpointer = data;
    }
    return Value::makeDataFrom(Value::VALTYP_FUNCNATIVE, obj);
}

Value mc_value_makearray(State* state)
{
    return mc_value_makearraycapacity(state, 8);
}

Value mc_value_makearraycapacity(State* state, size_t capacity)
{
    Object* data;
    data = mc_gcmemory_getdatafrompool(state, Value::VALTYP_ARRAY);
    if(data != nullptr)
    {
        data->m_uvobj.valarray->actualarray->setEmpty();
        return Value::makeDataFrom(Value::VALTYP_ARRAY, data);
    }
    data = mc_gcmemory_allocobjectdata(state);
    if(data == nullptr)
    {
        return Value::makeNull();
    }
    data->m_uvobj.valarray = Memory::make<Object::ObjArray>();
    data->m_uvobj.valarray->actualarray = Memory::make<GenericList<Value>>(capacity, Value::makeNull());
    if(data->m_uvobj.valarray->actualarray == nullptr)
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
    Object* data;
    data = mc_gcmemory_getdatafrompool(state, Value::VALTYP_MAP);
    if(data != nullptr)
    {
        data->m_uvobj.valmap->actualmap->clear();
        return Value::makeDataFrom(Value::VALTYP_MAP, data);
    }
    data = mc_gcmemory_allocobjectdata(state);
    if(data == nullptr)
    {
        return Value::makeNull();
    }
    data->m_uvobj.valmap = Memory::make<Object::ObjMap>();
    data->m_uvobj.valmap->actualmap = Memory::make<ValDict<Value, Value>>(capacity);
    if(data->m_uvobj.valmap->actualmap == nullptr)
    {
        return Value::makeNull();
    }
    data->m_uvobj.valmap->actualmap->setHashFunction((mcitemhashfn_t)Value::callbackHash);
    data->m_uvobj.valmap->actualmap->setEqualsFunction((mcitemcomparefn_t)Value::callbackEqualsTo);
    return Value::makeDataFrom(Value::VALTYP_MAP, data);
}

Value mc_value_makeerror(State* state, const char* error)
{
    char* errorstr;
    Value res;
    errorstr = mc_util_strdup(error);
    if(errorstr == nullptr)
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
    Object* data;
    data = mc_gcmemory_allocobjectdata(state);
    if(data == nullptr)
    {
        return Value::makeNull();
    }
    data->m_uvobj.valerror.message = error;
    data->m_uvobj.valerror.traceback = nullptr;
    return Value::makeDataFrom(Value::VALTYP_ERROR, data);
}

Value mc_value_makefuncscript(State* state, const char* name, CompiledProgram* cres, bool ownsdt, int nlocals, int nargs, int fvc)
{
    Object* data;
    data = mc_gcmemory_allocobjectdata(state);
    if(data == nullptr)
    {
        return Value::makeNull();
    }
    if(ownsdt)
    {
        data->m_uvobj.valfunc.m_funcdata.valscriptfunc.unamev.fallocname = (name != nullptr) ? mc_util_strdup(name) : mc_util_strdup("anonymous");
        if(data->m_uvobj.valfunc.m_funcdata.valscriptfunc.unamev.fallocname == nullptr)
        {
            return Value::makeNull();
        }
    }
    else
    {
        data->m_uvobj.valfunc.m_funcdata.valscriptfunc.unamev.fconstname = (name != nullptr) ? name : "anonymous";
    }
    data->m_uvobj.valfunc.m_funcdata.valscriptfunc.compiledprogcode = cres;
    data->m_uvobj.valfunc.m_funcdata.valscriptfunc.ownsdata = ownsdt;
    data->m_uvobj.valfunc.m_funcdata.valscriptfunc.numlocals = nlocals;
    data->m_uvobj.valfunc.m_funcdata.valscriptfunc.numargs = nargs;
    if(fvc >= Object::MaxFreeVal)
    {
        data->m_uvobj.valfunc.m_funcdata.valscriptfunc.ufv.freevalsallocated = (Value*)mc_memory_malloc(sizeof(Value) * fvc);
        if(data->m_uvobj.valfunc.m_funcdata.valscriptfunc.ufv.freevalsallocated == nullptr)
        {
            return Value::makeNull();
        }
    }
    data->m_uvobj.valfunc.m_funcdata.valscriptfunc.freevalscount = fvc;
    return Value::makeDataFrom(Value::VALTYP_FUNCSCRIPT, data);
}

Value mc_value_makeuserobject(State* state, void* data)
{
    Object* obj;
    obj = mc_gcmemory_allocobjectdata(state);
    if(obj == nullptr)
    {
        return Value::makeNull();
    }
    obj->m_uvobj.valuserobject.data = data;
    obj->m_uvobj.valuserobject.datadestroyfn = nullptr;
    obj->m_uvobj.valuserobject.datacopyfn = nullptr;
    return Value::makeDataFrom(Value::VALTYP_EXTERNAL, obj);
}

GenericList<Value>* mc_value_arraygetactualarray(Value object)
{
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_ARRAY);
    data = object.getAllocatedData();
    return data->m_uvobj.valarray->actualarray;
}

MC_INLINE char* mc_value_stringgetdataintern(const Value& object)
{
    Object* data;
    data = object.getAllocatedData();
    MC_ASSERT(data->m_odtype == Value::VALTYP_STRING);
    return data->m_uvobj.valstring.strbuf->data();
}


bool mc_value_userdatasetdata(Value object, void* extdata)
{
    Object::ObjUserdata* data;
    MC_ASSERT(object.getType() == Value::VALTYP_EXTERNAL);
    data = Value::userdataGetData(object);
    if(data == nullptr)
    {
        return false;
    }
    data->data = extdata;
    return true;
}

bool mc_value_userdatasetdestroyfunction(Value object, mcitemdestroyfn_t dfn)
{
    Object::ObjUserdata* data;
    MC_ASSERT(object.getType() == Value::VALTYP_EXTERNAL);
    data = Value::userdataGetData(object);
    if(data == nullptr)
    {
        return false;
    }
    data->datadestroyfn = dfn;
    return true;
}

bool mc_value_userdatasetcopyfunction(Value object, mcitemcopyfn_t copyfn)
{
    Object::ObjUserdata* data;
    MC_ASSERT(object.getType() == Value::VALTYP_EXTERNAL);
    data = Value::userdataGetData(object);
    if(data == nullptr)
    {
        return false;
    }
    data->datacopyfn = copyfn;
    return true;
}


MC_INLINE const char* mc_value_stringgetdata(const Value& object)
{
    MC_ASSERT(object.getType() == Value::VALTYP_STRING);
    return mc_value_stringgetdataintern(object);
}

int mc_value_stringgetlength(Value object)
{
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_STRING);
    data = object.getAllocatedData();
    return data->m_uvobj.valstring.strbuf->length();
}

void mc_value_stringsetlength(Value object, int len)
{
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_STRING);
    data = object.getAllocatedData();
    data->m_uvobj.valstring.strbuf->setLength(len);
    mc_value_stringrehash(object);
}


MC_INLINE char* mc_value_stringgetmutabledata(Value object)
{
    MC_ASSERT(object.getType() == Value::VALTYP_STRING);
    return mc_value_stringgetdataintern(object);
}

bool mc_value_stringappendlen(Value obj, const char* src, size_t len)
{
    Object* data;
    Object::ObjString* string;
    MC_ASSERT(obj.getType() == Value::VALTYP_STRING);
    data = obj.getAllocatedData();
    string = &data->m_uvobj.valstring;
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
    Object* data;
    Object::ObjString* string;
    MC_ASSERT(obj.getType() == Value::VALTYP_STRING);
    data = obj.getAllocatedData();
    string = &data->m_uvobj.valstring;
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
    Object* data;
    MC_ASSERT(obj.getType() == Value::VALTYP_STRING);
    data = obj.getAllocatedData();
    if(data->m_uvobj.valstring.hash == 0)
    {
        len = mc_value_stringgetlength(obj);
        str = mc_value_stringgetdata(obj);
        data->m_uvobj.valstring.hash = mc_util_hashdata(str, len);
    }
    return data->m_uvobj.valstring.hash;
}

bool mc_value_stringrehash(Value obj)
{
    size_t len;
    const char* str;
    Object* data;
    MC_ASSERT(obj.getType() == Value::VALTYP_STRING);
    data = obj.getAllocatedData();
    len = mc_value_stringgetlength(obj);
    str = mc_value_stringgetdata(obj);
    data->m_uvobj.valstring.hash = mc_util_hashdata(str, len);
    return true;
}



const char* mc_value_functiongetname(Value obj)
{
    Object* data;
    MC_ASSERT(obj.getType() == Value::VALTYP_FUNCSCRIPT);
    data = obj.getAllocatedData();
    MC_ASSERT(data);
    if(data == nullptr)
    {
        return nullptr;
    }
    if(data->m_uvobj.valfunc.m_funcdata.valscriptfunc.ownsdata)
    {
        return data->m_uvobj.valfunc.m_funcdata.valscriptfunc.unamev.fallocname;
    }
    return data->m_uvobj.valfunc.m_funcdata.valscriptfunc.unamev.fconstname;
}

Value mc_value_functiongetfreevalat(Value obj, int ix)
{
    Object* data;
    Object::ObjFunction* fun;
    MC_ASSERT(obj.getType() == Value::VALTYP_FUNCSCRIPT);
    data = obj.getAllocatedData();
    MC_ASSERT(data);
    if(data == nullptr)
    {
        return Value::makeNull();
    }
    fun = &data->m_uvobj.valfunc;
    MC_ASSERT(ix >= 0 && ix < fun->m_funcdata.valscriptfunc.freevalscount);
    if(ix < 0 || ix >= fun->m_funcdata.valscriptfunc.freevalscount)
    {
        return Value::makeNull();
    }
    if(fun->freeValuesAreAllocated())
    {
        return (Value)fun->m_funcdata.valscriptfunc.ufv.freevalsallocated[ix];
    }
    return (Value)fun->m_funcdata.valscriptfunc.ufv.freevalsstack[ix];
}

void mc_value_functionsetfreevalat(Value obj, int ix, Value val)
{
    Object* data;
    Object::ObjFunction* fun;
    MC_ASSERT(obj.getType() == Value::VALTYP_FUNCSCRIPT);
    data = obj.getAllocatedData();
    MC_ASSERT(data);
    if(data != nullptr)
    {
        fun = &data->m_uvobj.valfunc;
        MC_ASSERT(ix >= 0 && ix < fun->m_funcdata.valscriptfunc.freevalscount);
        if(ix < 0 || ix >= fun->m_funcdata.valscriptfunc.freevalscount)
        {
        }
        else
        {
            if(fun->freeValuesAreAllocated())
            {
                fun->m_funcdata.valscriptfunc.ufv.freevalsallocated[ix] = val;
            }
            else
            {
                fun->m_funcdata.valscriptfunc.ufv.freevalsstack[ix] = val;
            }
        }
    }
}

Value* mc_value_functiongetfreevals(Value obj)
{
    Object* data;
    Object::ObjFunction* fun;
    MC_ASSERT(obj.getType() == Value::VALTYP_FUNCSCRIPT);
    data = obj.getAllocatedData();
    MC_ASSERT(data);
    if(data == nullptr)
    {
        return nullptr;
    }
    fun = &data->m_uvobj.valfunc;
    if(fun->freeValuesAreAllocated())
    {
        return fun->m_funcdata.valscriptfunc.ufv.freevalsallocated;
    }
    return (Value*)fun->m_funcdata.valscriptfunc.ufv.freevalsstack;
}

const char* mc_value_errorgetmessage(Value object)
{
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_ERROR);
    data = object.getAllocatedData();
    return data->m_uvobj.valerror.message;
}

void mc_value_errorsettraceback(Value object, Traceback* traceback)
{
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_ERROR);
    if(object.getType() == Value::VALTYP_ERROR)
    {
        data = object.getAllocatedData();
        MC_ASSERT(data->m_uvobj.valerror.traceback == nullptr);
        data->m_uvobj.valerror.traceback = traceback;
    }
}

Traceback* mc_value_errorgettraceback(Value object)
{
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_ERROR);
    data = object.getAllocatedData();
    return data->m_uvobj.valerror.traceback;
}

Value mc_value_arraygetvalue(Value object, size_t ix)
{
    Value* res;
    MC_ASSERT(object.getType() == Value::VALTYP_ARRAY);
    auto array = mc_value_arraygetactualarray(object);
    if(ix >= array->count())
    {
        return Value::makeNull();
    }
    res = array->getp(ix);
    if(res == nullptr)
    {
        return Value::makeNull();
    }
    return *res;
}

bool mc_value_arraysetvalue(Value object, size_t ix, Value val)
{
    size_t len;
    size_t toadd;
    MC_ASSERT(object.getType() == Value::VALTYP_ARRAY);
    auto array = mc_value_arraygetactualarray(object);
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
    return array->set(ix, val) != nullptr;
}

bool mc_value_arraypush(Value object, Value val)
{
    MC_ASSERT(object.getType() == Value::VALTYP_ARRAY);
    auto array = mc_value_arraygetactualarray(object);
    return array->push(val);
}

int mc_value_arraygetlength(Value object)
{
    MC_ASSERT(object.getType() == Value::VALTYP_ARRAY);
    auto array = mc_value_arraygetactualarray(object);
    return array->count();
}

Value mc_valarray_pop(Value object)
{
    Value dest;
    MC_ASSERT(object.getType() == Value::VALTYP_ARRAY);
    auto array = mc_value_arraygetactualarray(object);
    if(array->pop(&dest))
    {
        return dest;
    }
    return Value::makeNull();
}

bool mc_value_arrayremoveat(Value object, int ix)
{
    auto array = mc_value_arraygetactualarray(object);
    return array->removeAt(ix);
}

int mc_state_mapgetlength(Value object)
{
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    return data->m_uvobj.valmap->actualmap->count();
}

Value mc_state_mapgetkeyat(Value object, int ix)
{
    Value* res;
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    res = data->m_uvobj.valmap->actualmap->getKeyAt(ix);
    if(res == nullptr)
    {
        return Value::makeNull();
    }
    return *res;
}

Value mc_state_mapgetvalueat(Value object, int ix)
{
    Value* res;
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    res = data->m_uvobj.valmap->actualmap->getValueAt(ix);
    if(res == nullptr)
    {
        return Value::makeNull();
    }
    return *res;
}

bool mc_state_mapsetvalueat(Value object, int ix, Value val)
{
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    if(ix >= mc_state_mapgetlength(object))
    {
        return false;
    }
    data = object.getAllocatedData();
    return data->m_uvobj.valmap->actualmap->setValueAt(ix, &val);
}

Value mc_state_mapgetkvpairat(State* state, Value object, int ix)
{
    Value key;
    Value val;
    Value res;
    Value valobj;
    Value keyobj;
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    if(ix >= data->m_uvobj.valmap->actualmap->count())
    {
        return Value::makeNull();
    }
    key = mc_state_mapgetkeyat(object, ix);
    val = mc_state_mapgetvalueat(object, ix);
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
    mc_state_mapsetvalue(res, keyobj, key);
    valobj = mc_value_makestring(state, "value");
    if(valobj.isNull())
    {
        return Value::makeNull();
    }
    mc_state_mapsetvalue(res, valobj, val);
    return res;
}

bool mc_state_mapsetvalue(Value object, Value key, Value val)
{
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    return data->m_uvobj.valmap->actualmap->setKV(&key, &val);
}

bool mc_state_mapsetvaluestring(State* state, Value object, const char* strkey, Value val)
{
    Value vkey;
    vkey = mc_value_makestring(state, strkey);
    return mc_state_mapsetvalue(object, vkey, val);
}

void mc_state_mapsetstrfunc(State* state, Value map, const char* fnname, mcnativefn_t function)
{
    mc_state_mapsetvaluestring(state, map, fnname, mc_value_makefuncnative(state, fnname, function, nullptr));
}

Value mc_state_mapgetvalue(Value object, Value key)
{
    Value* res;
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    res = data->m_uvobj.valmap->actualmap->get(&key);
    if(res == nullptr)
    {
        return Value::makeNull();
    }
    return *res;
}

bool mc_state_mapgetvaluechecked(Value object, Value key, Value* dest)
{
    Value* res;
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    res = data->m_uvobj.valmap->actualmap->get(&key);
    if(res == nullptr)
    {
        *dest = Value::makeNull();
        return false;
    }
    *dest = *res;
    return true;
}

bool mc_state_maphaskey(Value object, Value key)
{
    Value* res;
    Object* data;
    MC_ASSERT(object.getType() == Value::VALTYP_MAP);
    data = object.getAllocatedData();
    res = data->m_uvobj.valmap->actualmap->get(&key);
    return res != nullptr;
}

void mc_objectdata_deinit(Object* data)
{
    switch(data->m_odtype)
    {
        case Value::VALTYP_FREED:
            {
                MC_ASSERT(false);
            }
            break;
        case Value::VALTYP_STRING:
            {
                StringBuffer::destroy(data->m_uvobj.valstring.strbuf);
            }
            break;
        case Value::VALTYP_FUNCSCRIPT:
            {
                if(data->m_uvobj.valfunc.m_funcdata.valscriptfunc.ownsdata)
                {
                    mc_memory_free(data->m_uvobj.valfunc.m_funcdata.valscriptfunc.unamev.fallocname);
                    CompiledProgram::destroy(data->m_uvobj.valfunc.m_funcdata.valscriptfunc.compiledprogcode);
                }
                if(data->m_uvobj.valfunc.freeValuesAreAllocated())
                {
                    mc_memory_free(data->m_uvobj.valfunc.m_funcdata.valscriptfunc.ufv.freevalsallocated);
                }
            }
            break;
        case Value::VALTYP_ARRAY:
            {
                Memory::destroy(data->m_uvobj.valarray->actualarray);
                Memory::destroy(data->m_uvobj.valarray);
            }
            break;
        case Value::VALTYP_MAP:
            {
                Memory::destroy(data->m_uvobj.valmap->actualmap);
                Memory::destroy(data->m_uvobj.valmap);
            }
            break;
        case Value::VALTYP_FUNCNATIVE:
            {
                mc_memory_free(data->m_uvobj.valfunc.m_funcdata.valnativefunc.natfnname);
            }
            break;
        case Value::VALTYP_EXTERNAL:
            {
                if(data->m_uvobj.valuserobject.datadestroyfn != nullptr)
                {
                    data->m_uvobj.valuserobject.datadestroyfn(data->m_uvobj.valuserobject.data);
                }
            }
            break;
        case Value::VALTYP_ERROR:
            {
                mc_memory_free(data->m_uvobj.valerror.message);
                Memory::destroy(data->m_uvobj.valerror.traceback);
            }
            break;
        default:
            {
            }
            break;
    }
    data->m_odtype = Value::VALTYP_FREED;
}



template<typename TypeKeyT, typename TypeValueT>
Value mc_value_copydeepfuncscript(State* state, Value obj, ValDict<TypeKeyT, TypeValueT>* targetdict)
{
    bool ok;
    int i;
    uint16_t* bytecodecopy;
    Object::ObjFunction* functioncopy;
    Value copy;
    Value freeval;
    Value freevalcopy;
    Object::ObjFunction* function;
    AstLocation* srcpositionscopy;
    CompiledProgram* comprescopy;
    (void)ok;
    function = Value::asFunction(obj);
    bytecodecopy = nullptr;
    srcpositionscopy = nullptr;
    comprescopy = nullptr;
    bytecodecopy = (uint16_t*)mc_memory_malloc(sizeof(uint16_t) * function->m_funcdata.valscriptfunc.compiledprogcode->m_compiledcount);
    if(!bytecodecopy)
    {
        return Value::makeNull();
    }
    memcpy(bytecodecopy, function->m_funcdata.valscriptfunc.compiledprogcode->m_compiledbytecode, sizeof(uint16_t) * function->m_funcdata.valscriptfunc.compiledprogcode->m_compiledcount);
    srcpositionscopy = (AstLocation*)mc_memory_malloc(sizeof(AstLocation) * function->m_funcdata.valscriptfunc.compiledprogcode->m_compiledcount);
    if(!srcpositionscopy)
    {
        mc_memory_free(bytecodecopy);
        return Value::makeNull();
    }
    memcpy(srcpositionscopy, function->m_funcdata.valscriptfunc.compiledprogcode->m_progsrcposlist, sizeof(AstLocation) * function->m_funcdata.valscriptfunc.compiledprogcode->m_compiledcount);
    comprescopy = Memory::make<CompiledProgram>(bytecodecopy, srcpositionscopy, function->m_funcdata.valscriptfunc.compiledprogcode->m_compiledcount);
    /*
    * todo: add compilation result copy function
    */
    if(!comprescopy)
    {
        mc_memory_free(srcpositionscopy);
        mc_memory_free(bytecodecopy);
        return Value::makeNull();
    }
    copy = mc_value_makefuncscript(state, mc_value_functiongetname(obj), comprescopy, true, function->m_funcdata.valscriptfunc.numlocals, function->m_funcdata.valscriptfunc.numargs, 0);
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
        functioncopy->m_funcdata.valscriptfunc.ufv.freevalsallocated = (Value*)mc_memory_malloc(sizeof(Value) * function->m_funcdata.valscriptfunc.freevalscount);
        if(!functioncopy->m_funcdata.valscriptfunc.ufv.freevalsallocated)
        {
            return Value::makeNull();
        }
    }
    functioncopy->m_funcdata.valscriptfunc.freevalscount = function->m_funcdata.valscriptfunc.freevalscount;
    for(i = 0; i < function->m_funcdata.valscriptfunc.freevalscount; i++)
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
Value mc_value_copydeeparray(State* state, Value obj, ValDict<TypeKeyT, TypeValueT>* targetdict)
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
Value mc_value_copydeepmap(State* state, Value obj, ValDict<TypeKeyT, TypeValueT>* targetdict)
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
    for(i = 0; i < mc_state_mapgetlength(obj); i++)
    {
        key = mc_state_mapgetkeyat(obj, i);
        val = mc_state_mapgetvalueat(obj, i);
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
        ok = mc_state_mapsetvalue(copy, keycopy, valcopy);
        if(!ok)
        {
            return Value::makeNull();
        }
    }
    return copy;
}




bool mc_printutil_bcreadoperands(AstCompiler::OpDefinition* def, const uint16_t* instr, uint64_t outoperands[2])
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
    AstCompiler::OpDefinition* def;
    AstCompiler::OpDefinition vdef;
    AstLocation srcpos;
    (void)ok;
    def = AstCompiler::opdefLookup(&vdef, op);
    MC_ASSERT(def != nullptr);
    if(sposlist != nullptr)
    {
        srcpos = sposlist[*pos];
        if(simple)
        {
            pr->put("<");
        }
        pr->format("@%d:%d %04d %s", srcpos.m_locline, srcpos.m_loccolumn, *pos, def->name);
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
            if(op == AstCompiler::OPCODE_NUMBER)
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
    Object::ObjFunction* fn;
    fn = Value::asFunction(obj);
    fname = mc_value_functiongetname(obj);
    pr->format("<scriptfunction '%s' locals=%d argc=%d fvc=%d", fname, fn->m_funcdata.valscriptfunc.numlocals, fn->m_funcdata.valscriptfunc.numargs, fn->m_funcdata.valscriptfunc.freevalscount);
    #if 0
    if(pr->m_prconfig.verbosefunc)
    {
        pr->put(" [");
        mc_printer_printbytecode(pr, fn->m_funcdata.valscriptfunc.compiledprogcode->m_compiledbytecode, fn->m_funcdata.valscriptfunc.compiledprogcode->m_progsrcposlist, fn->m_funcdata.valscriptfunc.compiledprogcode->m_compiledcount, true);
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
    auto actualary = mc_value_arraygetactualarray(obj);
    alen = mc_value_arraygetlength(obj);
    pr->put("[");
    for(i = 0; i < alen; i++)
    {
        recursion = false;
        iobj = mc_value_arraygetvalue(obj, i);
        if(iobj.getType() == Value::VALTYP_ARRAY)
        {
            auto otherary = mc_value_arraygetactualarray(iobj);
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
    alen = mc_state_mapgetlength(obj);
    pr->put("{");
    for(i = 0; i < alen; i++)
    {
        key = mc_state_mapgetkeyat(obj, i);
        val = mc_state_mapgetvalueat(obj, i);
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
                Error::printUserError(pr, obj);
            }
            break;
        case Value::VALTYP_ANY:
            {
                MC_ASSERT(false);
            }
            break;
    }
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
            state->pushError(Error::ERRTYP_RUNTIME, AstLocation::Invalid(), "Invalid number or arguments, got %d instead of %d", argc, expectedargc);
        }
        return false;
    }
    for(i = 0; i < argc; i++)
    {
        arg = args[i];
        type = arg.getType();
        expectedtype = expectedtypes[i];
        if((type & expectedtype) == 0)
        {
            if(generateerror)
            {
                typestr = Value::getTypename(type);
                expectedtypestr = Value::getUnionName(expectedtype);
                if(expectedtypestr == nullptr)
                {
                    return false;
                }
                state->pushError(Error::ERRTYP_RUNTIME, AstLocation::Invalid(), "Invalid argument %d type, got %s, expected %s", i, typestr, expectedtypestr);
                mc_memory_free(expectedtypestr);
            }
            return false;
        }
    }
    return true;
}

Object* mc_gcmemory_allocobjectdata(State* state)
{
    bool ok;
    Object* data;
    (void)ok;
    data = nullptr;
    state->m_stategcmem->m_allocssincesweep++;
    if(state->m_stategcmem->m_poolonlydata.m_poolitemcount > 0)
    {
        data = state->m_stategcmem->m_poolonlydata.m_pooldata.get(state->m_stategcmem->m_poolonlydata.m_poolitemcount - 1);
        state->m_stategcmem->m_poolonlydata.m_poolitemcount--;
    }
    else
    {
        data = Memory::make<Object>(state);
    }
    MC_ASSERT(state->m_stategcmem->m_gcobjlistback->count() >= state->m_stategcmem->m_gcobjliststored->count());
    /*
    * we want to make sure that appending to m_gcobjlistback never fails in sweep
    * so this only reserves space there.
    */
    ok = state->m_stategcmem->m_gcobjlistback->push(data);
    if(!ok)
    {
        Memory::destroy(data);
        return nullptr;
    }
    ok = state->m_stategcmem->m_gcobjliststored->push(data);
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
    Object* data;
    for(i = 0; i < state->m_stategcmem->m_gcobjliststored->count(); i++)
    {
        data = state->m_stategcmem->m_gcobjliststored->get(i);
        data->m_gcmark = 0;
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
    Object* data;
    Object* valdata;
    Object* keydata;
    Object* freevaldata;
    Object::ObjFunction* function;
    if(obj.isAllocated())
    {
        data = obj.getAllocatedData();
        if(data->m_gcmark == 0)
        {
            data->m_gcmark = 1;
            switch(obj.getType())
            {
                case Value::VALTYP_MAP:
                    {
                        len = mc_state_mapgetlength(obj);
                        for(i = 0; i < len; i++)
                        {
                            key = mc_state_mapgetkeyat(obj, i);
                            if(key.isAllocated())
                            {
                                keydata = key.getAllocatedData();
                                if(keydata->m_gcmark == 0)
                                {
                                    mc_state_gcmarkobject(key);
                                }
                            }
                            val = mc_state_mapgetvalueat(obj, i);
                            if(val.isAllocated())
                            {
                                valdata = val.getAllocatedData();
                                if(valdata->m_gcmark == 0)
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
                                if(valdata->m_gcmark == 0)
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
                        for(i = 0; i < function->m_funcdata.valscriptfunc.freevalscount; i++)
                        {
                            freeval = mc_value_functiongetfreevalat(obj, i);
                            mc_state_gcmarkobject(freeval);
                            if(freeval.isAllocated())
                            {
                                freevaldata = freeval.getAllocatedData();
                                if(freevaldata->m_gcmark == 0)
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
    Object* data;
    GCMemory::DataPool* pool;
    (void)ok;
    mc_state_gcmarkobjlist(state->m_stategcmem->m_gcobjlistremains->data(), state->m_stategcmem->m_gcobjlistremains->count());
    MC_ASSERT(state->m_stategcmem->m_gcobjlistback->count() >= state->m_stategcmem->m_gcobjliststored->count());
    state->m_stategcmem->m_gcobjlistback->clear();
    for(i = 0; i < state->m_stategcmem->m_gcobjliststored->count(); i++)
    {
        data = state->m_stategcmem->m_gcobjliststored->get(i);
        if(data->m_gcmark != 0)
        {
            /*
            * this should never fail because m_gcobjlistback's size should be equal to objects
            */
            ok = state->m_stategcmem->m_gcobjlistback->push(data);
            (void)ok;
            MC_ASSERT(ok);
        }
        else
        {
            if(mc_state_gccandatabeputinpool(state, data))
            {
                pool = state->m_stategcmem->getPoolForType(data->m_odtype);
                pool->m_pooldata.set(pool->m_poolitemcount, data);
                pool->m_poolitemcount++;
            }
            else
            {
                mc_objectdata_deinit(data);
                /*
                if(state->m_stategcmem->m_poolonlydata.m_poolitemcount < GCMemory::MemPoolSize)
                {
                */
                    state->m_stategcmem->m_poolonlydata.m_pooldata.set(state->m_stategcmem->m_poolonlydata.m_poolitemcount, data);
                    state->m_stategcmem->m_poolonlydata.m_poolitemcount++;
                /*
                }
                else
                {
                    Memory::destroy(data);
                    data = nullptr;
                }
                */
            }
        }
    }
    auto objstemp = state->m_stategcmem->m_gcobjliststored;
    state->m_stategcmem->m_gcobjliststored = state->m_stategcmem->m_gcobjlistback;
    state->m_stategcmem->m_gcobjlistback = objstemp;
    state->m_stategcmem->m_allocssincesweep = 0;
}

int mc_state_gcshouldsweep(State* state)
{
    return static_cast<int>(state->m_stategcmem->m_allocssincesweep > GCMemory::SweepInterval);
}

bool mc_state_gcdisablefor(Value obj)
{
    MC_ASSERT(!"mc_state_gcdisablefor() is broken!");
    #if 1
        (void)obj;
    #else
    bool ok;
    Object* data;
    (void)ok;
    if(!obj.isAllocated())
    {
        return false;
    }
    data = obj.getAllocatedData();
    if(data->m_objmem->m_gcobjlistremains->contains(&obj))
    {
        return false;
    }
    ok = data->m_objmem->m_gcobjlistremains->push(obj);
    return ok;
    #endif
    return false;
}

void mc_state_gcenablefor(Value obj)
{
    MC_ASSERT(!"mc_state_gcenablefor() is broken!");
    #if 1
        (void)obj;
    #else
    Object* data;
    if(obj.isAllocated())
    {
        data = obj.getAllocatedData();
        data->m_objmem->m_gcobjlistremains->removeItem(&obj);
    }
    #endif
}

bool mc_state_gccandatabeputinpool(State* state, Object* data)
{
    Value obj;
    GCMemory::DataPool* pool;
    obj = Value::makeDataFrom((Value::Type)data->m_odtype, data);
    /*
    * this is to ensure that large objects won't be kept in pool indefinitely
    */
    #if 0
    switch(data->m_odtype)
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
                if(mc_state_mapgetlength(obj) > 1024)
                {
                    return false;
                }
            }
            break;
        case Value::VALTYP_STRING:
            {
                #if 0
                if(!data->m_uvobj.valstring.isAllocated() || data->m_uvobj.valstring.capacity > 4096)
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
    #endif
    pool= state->m_stategcmem->getPoolForType(data->m_odtype);
    if(pool == nullptr /*|| pool->m_poolitemcount >= GCMemory::MemPoolSize*/)
    {
        return false;
    }
    return true;
}

void mc_vm_setoverloadkey(State* state, AstCompiler::OpCode opc, const char* rawstr)
{
    Value keyobj;
    keyobj = mc_value_makestring(state, rawstr);
    state->m_operoverloadkeys[opc] = keyobj;
}

bool mc_vm_init(State* state)
{
    int i;
    state->m_hadrecovered = false;
    state->m_globalvalcount = 0;
    state->m_execstate.vsposition = 0;
    state->m_execstate.thisstpos = 0;
    state->m_execstate.lastpopped = Value::makeNull();
    state->m_running = false;
    for(i = 0; i < State::MaxOperOverloads; i++)
    {
        state->m_operoverloadkeys[i] = Value::makeNull();
    }
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_ADD, "__operator_add__");
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_SUB, "__operator_sub__");
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_MUL, "__operator_mul__");
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_DIV, "__operator_div__");
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_MOD, "__operator_mod__");
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_BINOR, "__operator_or__");
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_BINXOR, "__operator_xor__");
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_BINAND, "__operator_and__");
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_LSHIFT, "__operator_lshift__");
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_RSHIFT, "__operator_rshift__");
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_MINUS, "__operator_minus__");
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_BINNOT, "__operator_binnot__");
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_BANG, "__operator_bang__");
    mc_vm_setoverloadkey(state, AstCompiler::OPCODE_COMPARE, "__cmp__");
    return true;
}

void mc_vm_reset(State* state)
{
    state->m_execstate.vsposition = 0;
    state->m_execstate.thisstpos = 0;
    while(state->m_execstate.framestack.m_listcount > 0)
    {
        state->vmPopFrame();
    }
}


MC_INLINE void mc_vm_rungc(State* state, GenericList<Value>* constants)
{
    size_t i;
    VMFrame* frame;
    mc_state_gcunmarkall(state);
    mc_state_gcmarkobjlist(state->m_vmglobalstore->getData(), state->m_vmglobalstore->getCount());
    mc_state_gcmarkobjlist(constants->data(), constants->count());
    mc_state_gcmarkobjlist(state->m_globalvalstack.data(), state->m_globalvalcount);
    for(i = 0; i < state->m_execstate.framestack.count(); i++)
    {
        frame = state->m_execstate.framestack.getp(i);
        mc_state_gcmarkobject(frame->m_function);
    }
    mc_state_gcmarkobjlist(state->m_execstate.valuestack.data(), state->m_execstate.vsposition);
    mc_state_gcmarkobjlist(state->m_execstate.valthisstack.data(), state->m_execstate.thisstpos);
    mc_state_gcmarkobject(state->m_execstate.lastpopped);
    mc_state_gcmarkobjlist(state->m_operoverloadkeys, State::MaxOperOverloads);
    mc_state_gcsweep(state);
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
    Object::ObjFunction* constfun;
    constantix = state->m_execstate.currframe->readUint16();
    numfree = state->m_execstate.currframe->readUint8();
    constant = constants->getp(constantix);
    if(constant == nullptr)
    {
        state->pushError(Error::ERRTYP_RUNTIME, state->m_execstate.currframe->getPosition(), "constant %d not found", constantix);
        return false;
    }
    constanttype = (*constant).getType();
    if(constanttype != Value::VALTYP_FUNCSCRIPT)
    {
        tname = Value::getTypename(constanttype);
        state->pushError(Error::ERRTYP_RUNTIME, state->m_execstate.currframe->getPosition(), "%s is not a function", tname);
        return false;
    }
    constfun = Value::asFunction(*constant);
    fname = mc_value_functiongetname(*constant);
    functionobj = mc_value_makefuncscript(state, fname, constfun->m_funcdata.valscriptfunc.compiledprogcode, false, constfun->m_funcdata.valscriptfunc.numlocals, constfun->m_funcdata.valscriptfunc.numargs, numfree);
    if(functionobj.isNull())
    {
        return false;
    }
    for(i = 0; i < numfree; i++)
    {
        freeval = state->m_execstate.valuestack.get(state->m_execstate.vsposition - numfree + i);
        mc_value_functionsetfreevalat(functionobj, i, freeval);
    }
    state->setStackPos(state->m_execstate.vsposition - numfree);
    state->vmStackPush(functionobj);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_docmpvalue(State* state, AstCompiler::OpCode opcode)
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
    ok = state->vmTryOverloadOperator(left, right, AstCompiler::OPCODE_COMPARE, &isoverloaded);
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
        if((ok) || (opcode == AstCompiler::OPCODE_COMPAREEQ))
        {
            res = Value::makeNumber(cres.result);
            state->vmStackPush(res);
        }
        else
        {
            righttname = Value::getTypename(right.getType());
            lefttname = Value::getTypename(left.getType());
            state->pushError(Error::ERRTYP_RUNTIME, state->m_execstate.currframe->getPosition(), "cannot compare %s and %s", lefttname, righttname);
            return false;
        }
    }
    return true;
}

MC_FORCEINLINE bool mc_vmdo_docmpvalgreater(State* state, AstCompiler::OpCode opcode)
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
        case AstCompiler::OPCODE_EQUAL:
            {
                resval = MC_UTIL_CMPFLOAT(comparisonres, 0);
            }
            break;
        case AstCompiler::OPCODE_NOTEQUAL:
            {
                resval = !MC_UTIL_CMPFLOAT(comparisonres, 0);
            }
            break;
        case AstCompiler::OPCODE_GREATERTHAN:
            {
                resval = comparisonres > 0;
            }
            break;
        case AstCompiler::OPCODE_GREATERTHANEQUAL:
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
    count = state->m_execstate.currframe->readUint16();
    arrayobj = mc_value_makearraycapacity(state, count);
    if(arrayobj.isNull())
    {
        return false;
    }
    items = state->m_execstate.valuestack.data() + state->m_execstate.vsposition - count;
    for(i = 0; i < count; i++)
    {
        item = items[i];
        ok = mc_value_arraypush(arrayobj, item);
        if(!ok)
        {
            return false;
        }
    }
    state->setStackPos(state->m_execstate.vsposition - count);
    state->vmStackPush(arrayobj);
    return true;
}

MC_FORCEINLINE bool mc_vmdo_makemapstart(State* state)
{
    uint16_t count;
    Value mapobj;
    count = state->m_execstate.currframe->readUint16();
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
    kvpcount = state->m_execstate.currframe->readUint16();
    itemscount = kvpcount * 2;
    mapobj = state->vmStackThisPop();
    kvpairs = state->m_execstate.valuestack.data() + state->m_execstate.vsposition - itemscount;
    for(i = 0; i < itemscount; i += 2)
    {
        key = kvpairs[i];
        if(!Value::isHashable(key))
        {
            keytype = key.getType();
            keytypename = Value::getTypename(keytype);
            state->pushError(Error::ERRTYP_RUNTIME, state->m_execstate.currframe->getPosition(), "key of type %s is not hashable", keytypename);
            return false;
        }
        val = kvpairs[i + 1];
        ok = mc_state_mapsetvalue(mapobj, key, val);
    }
    state->setStackPos(state->m_execstate.vsposition - itemscount);
    state->vmStackPush(mapobj);
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
#else
    #define mcvm_case(opn) case AstCompiler::opn
#endif

#define mc_vmmac_break() \
    goto readnextop


void mc_vmutil_getopinfo(AstCompiler::OpCode opc, const char** oname)
{
    AstCompiler::OpDefinition vdef;
    AstCompiler::OpDefinition* def;
    *oname = "!invalid!";
    def = AstCompiler::opdefLookup(&vdef, opc);
    if(def != nullptr)
    {
        *oname = def->name;
    }
}

bool State::execVM(Value function, GenericList<Value>* constants, bool nested)
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
    Object::ObjFunction* targetfunction;
    opcode = 0;
    (void)oname;
    (void)prevcode;
    #if defined(MC_CONF_USECOMPUTEDGOTOS) && (MC_CONF_USECOMPUTEDGOTOS == 1)
        static void* dispatchtable[] = {
            &&MAKELABEL(OPCODE_HALT),
            &&MAKELABEL(OPCODE_CONSTANT),
            &&MAKELABEL(OPCODE_ADD),
            &&MAKELABEL(OPCODE_SUB),
            &&MAKELABEL(OPCODE_MUL),
            &&MAKELABEL(OPCODE_DIV),
            &&MAKELABEL(OPCODE_MOD),
            &&MAKELABEL(OPCODE_POP),
            &&MAKELABEL(OPCODE_BINOR),
            &&MAKELABEL(OPCODE_BINXOR),
            &&MAKELABEL(OPCODE_BINAND),
            &&MAKELABEL(OPCODE_LSHIFT),
            &&MAKELABEL(OPCODE_RSHIFT),
            &&MAKELABEL(OPCODE_BANG),
            &&MAKELABEL(OPCODE_COMPARE),
            &&MAKELABEL(OPCODE_TRUE),
            &&MAKELABEL(OPCODE_FALSE),
            &&MAKELABEL(OPCODE_COMPAREEQ),
            &&MAKELABEL(OPCODE_EQUAL),
            &&MAKELABEL(OPCODE_NOTEQUAL),
            &&MAKELABEL(OPCODE_GREATERTHAN),
            &&MAKELABEL(OPCODE_GREATERTHANEQUAL),
            &&MAKELABEL(OPCODE_MINUS),
            &&MAKELABEL(OPCODE_BINNOT),
            &&MAKELABEL(OPCODE_JUMP),
            &&MAKELABEL(OPCODE_JUMPIFFALSE),
            &&MAKELABEL(OPCODE_JUMPIFTRUE),
            &&MAKELABEL(OPCODE_NULL),
            &&MAKELABEL(OPCODE_GETMODULEGLOBAL),
            &&MAKELABEL(OPCODE_SETMODULEGLOBAL),
            &&MAKELABEL(OPCODE_DEFINEMODULEGLOBAL),
            &&MAKELABEL(OPCODE_ARRAY),
            &&MAKELABEL(OPCODE_MAPSTART),
            &&MAKELABEL(OPCODE_MAPEND),
            &&MAKELABEL(OPCODE_GETTHIS),
            &&MAKELABEL(OPCODE_GETINDEX),
            &&MAKELABEL(OPCODE_SETINDEX),
            &&MAKELABEL(OPCODE_GETDOTINDEX),
            &&MAKELABEL(OPCODE_GETVALUEAT),
            &&MAKELABEL(OPCODE_CALL),
            &&MAKELABEL(OPCODE_RETURNVALUE),
            &&MAKELABEL(OPCODE_RETURN),
            &&MAKELABEL(OPCODE_GETLOCAL),
            &&MAKELABEL(OPCODE_DEFINELOCAL),
            &&MAKELABEL(OPCODE_SETLOCAL),
            &&MAKELABEL(OPCODE_GETGLOBALBUILTIN),
            &&MAKELABEL(OPCODE_FUNCTION),
            &&MAKELABEL(OPCODE_GETFREE),
            &&MAKELABEL(OPCODE_SETFREE),
            &&MAKELABEL(OPCODE_CURRENTFUNCTION),
            &&MAKELABEL(OPCODE_DUP),
            &&MAKELABEL(OPCODE_NUMBER),
            &&MAKELABEL(OPCODE_FOREACHLEN),
            &&MAKELABEL(OPCODE_SETRECOVER),
        };
    #endif
    #if 0
    if(mc_util_unlikely(m_running))
    {
        pushError(Error::ERRTYP_USER, AstLocation::Invalid(), "state is already executing code");
        return false;
    }
    #endif
    /* naming is hard */
    targetfunction = Value::asFunction(function);
    ok = false;
    ok = VMFrame::init(&createdframe, function, m_execstate.vsposition - targetfunction->m_funcdata.valscriptfunc.numargs);
    if(!ok)
    {
        fprintf(stderr, "failed to init frames!\n");
        return false;
    }
    ok = vmPushFrame(createdframe);
    if(!ok)
    {
        m_errorlist.pushFormat(Error::ERRTYP_USER, AstLocation::Invalid(), "pushing frame failed");
        return false;
    }
    m_running = true;
    m_execstate.lastpopped = Value::makeNull();
    //while(m_execstate.currframe->m_bcposition < m_execstate.currframe->m_bcsize)
    while(true)
    {
        readnextop:
        #if !defined(MC_CONF_USECOMPUTEDGOTOS) || (MC_CONF_USECOMPUTEDGOTOS == 0)
        prevcode = opcode;
        #endif
        frame = m_execstate.currframe;
        if(frame == nullptr)
        {
            goto onexecfinish;
        }
        opcode = frame->readOpCode();
        if(mc_util_unlikely(m_config.printinstructions))
        {
            mc_vmutil_getopinfo((AstCompiler::OpCode)opcode, &oname);
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
                    mc_vmutil_getopinfo((AstCompiler::OpCode)opcode, &thisname);
                    mc_vmutil_getopinfo((AstCompiler::OpCode)prevcode, &prevname);
                    pushError(Error::ERRTYP_RUNTIME, frame->getPosition(), "unknown opcode: %d (%s) (previous opcode was %d (%s))", opcode, thisname, prevcode, prevname);
                    MC_ASSERT(false);
                    goto onexecerror;
                }
                break;
            #endif
            mcvm_case(OPCODE_HALT):
                {
                    goto onexecfinish;
                }
            mcvm_case(OPCODE_RETURNVALUE):
                {
                    Value res;
                    res = vmStackPop();
                    ok = vmPopFrame();
                    if(!ok)
                    {
                        goto onexecfinish;
                    }
                    vmStackPush(res);
                    if(nested)
                    {
                        goto onexecfinish;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_RETURN):
                {
                    ok = vmPopFrame();
                    vmStackPush(Value::makeNull());
                    if(!ok)
                    {
                        vmStackPop();
                        goto onexecfinish;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_CONSTANT):
                {
                    uint16_t constantix;
                    Value* constant;
                    constantix = frame->readUint16();
                    constant = constants->getp(constantix);
                    if(constant == nullptr)
                    {
                        pushError(Error::ERRTYP_RUNTIME, frame->getPosition(), "constant at %d not found", constantix);
                        goto onexecerror;
                    }
                    vmStackPush(*constant);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_ADD):
            mcvm_case(OPCODE_SUB):
            mcvm_case(OPCODE_MUL):
            mcvm_case(OPCODE_DIV):
            mcvm_case(OPCODE_MOD):
            mcvm_case(OPCODE_BINOR):
            mcvm_case(OPCODE_BINXOR):
            mcvm_case(OPCODE_BINAND):
            mcvm_case(OPCODE_LSHIFT):
            mcvm_case(OPCODE_RSHIFT):
                {
                    if(!vmOpMath((AstCompiler::OpCode)opcode))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_POP):
                {
                    vmStackPop();
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_TRUE):
                {
                    vmStackPush(Value::makeBool(true));
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_FALSE):
                {
                    vmStackPush(Value::makeBool(false));
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_COMPARE):
            mcvm_case(OPCODE_COMPAREEQ):
                {
                    if(!mc_vmdo_docmpvalue(this, (AstCompiler::OpCode)opcode))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_EQUAL):
            mcvm_case(OPCODE_NOTEQUAL):
            mcvm_case(OPCODE_GREATERTHAN):
            mcvm_case(OPCODE_GREATERTHANEQUAL):
                {
                    if(!mc_vmdo_docmpvalgreater(this, (AstCompiler::OpCode)opcode))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_MINUS):
                {
                    bool overloadfound;
                    mcfloat_t val;
                    const char* opertname;
                    Value res;
                    Value::Type opertype;
                    Value operand;
                    operand = vmStackPop();
                    opertype = operand.getType();
                    if(mc_util_likely(opertype == Value::VALTYP_NUMBER))
                    {
                        val = Value::asNumber(operand);
                        res = Value::makeNumber(-val);
                        vmStackPush(res);
                    }
                    else
                    {
                        overloadfound = false;
                        ok = vmTryOverloadOperator(operand, Value::makeNull(), AstCompiler::OPCODE_MINUS, &overloadfound);
                        if(!ok)
                        {
                            goto onexecerror;
                        }
                        if(!overloadfound)
                        {
                            opertname = Value::getTypename(opertype);
                            pushError(Error::ERRTYP_RUNTIME, frame->getPosition(), "invalid operand type for MINUS, got %s", opertname);
                            goto onexecerror;
                        }
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_BINNOT):
                {
                    bool overloadfound;
                    int64_t val;
                    const char* opertname;
                    Value res;
                    Value::Type opertype;
                    Value operand;
                    operand = vmStackPop();
                    opertype = operand.getType();
                    if(opertype == Value::VALTYP_NUMBER)
                    {
                        val = Value::asNumber(operand);
                        res = Value::makeNumber(~val);
                        vmStackPush(res);
                    }
                    else
                    {
                        overloadfound = false;
                        ok = vmTryOverloadOperator(operand, Value::makeNull(), AstCompiler::OPCODE_BINNOT, &overloadfound);
                        if(!ok)
                        {
                            goto onexecerror;
                        }
                        if(!overloadfound)
                        {
                            opertname = Value::getTypename(opertype);
                            pushError(Error::ERRTYP_RUNTIME, frame->getPosition(), "invalid operand type for BINNOT, got %s", opertname);
                            goto onexecerror;
                        }
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_BANG):
                {
                    bool overloadfound;
                    Value res;
                    Value operand;
                    Value::Type type;
                    operand = vmStackPop();
                    type = operand.getType();
                    if(type == Value::VALTYP_BOOL)
                    {
                        res = Value::makeBool(!Value::asBool(operand));
                        vmStackPush(res);
                    }
                    else if(type == Value::VALTYP_NULL)
                    {
                        res = Value::makeBool(true);
                        vmStackPush(res);
                    }
                    else
                    {
                        overloadfound = false;
                        ok = vmTryOverloadOperator(operand, Value::makeNull(), AstCompiler::OPCODE_BANG, &overloadfound);
                        if(!ok)
                        {
                            goto onexecerror;
                        }
                        if(!overloadfound)
                        {
                            res = Value::makeBool(false);
                            vmStackPush(res);
                        }
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_JUMP):
                {
                    uint16_t pos;
                    pos = frame->readUint16();
                    frame->m_bcposition = pos;
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_JUMPIFFALSE):
                {
                    uint16_t pos;
                    Value test;
                    pos = frame->readUint16();
                    test = vmStackPop();
                    if(!Value::asBool(test))
                    {
                        frame->m_bcposition = pos;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_JUMPIFTRUE):
                {
                    uint16_t pos;
                    Value test;
                    pos = frame->readUint16();
                    test = vmStackPop();
                    if(Value::asBool(test))
                    {
                        frame->m_bcposition = pos;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_NULL):
                {
                    vmStackPush(Value::makeNull());
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_DEFINEMODULEGLOBAL):
                {
                    uint16_t ix;
                    Value value;
                    ix = frame->readUint16();
                    value = vmStackPop();
                    setGlobalByIndex(ix, value);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_SETMODULEGLOBAL):
                {
                    uint16_t ix;
                    Value nvalue;
                    Value oldvalue;
                    ix = frame->readUint16();
                    nvalue = vmStackPop();
                    oldvalue= getGlobalByIndex(ix);
                    if(!vmCheckAssign(oldvalue, nvalue))
                    {
                        goto onexecerror;
                    }
                    setGlobalByIndex(ix, nvalue);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_GETMODULEGLOBAL):
                {
                    uint16_t ix;
                    Value global;
                    ix = frame->readUint16();
                    global = m_globalvalstack.get(ix);
                    vmStackPush(global);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_ARRAY):
                {
                    if(!mc_vmdo_makearray(this))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_MAPSTART):
                {
                    if(!mc_vmdo_makemapstart(this))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_MAPEND):
                {
                    if(!mc_vmdo_makemapend(this))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_GETVALUEAT):
                {
                    if(!vmGetValueAtFull())
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_CALL):
                {
                    uint16_t nargs;
                    Value callee;
                    nargs = frame->readUint8();
                    callee = vmStackGet(nargs);
                    ok = vmCallObject(callee, nargs);
                    if(!ok)
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_DEFINELOCAL):
                {
                    uint16_t pos;
                    pos = frame->readUint8();
                    m_execstate.valuestack.set(frame->m_basepointer + pos, vmStackPop());
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_SETLOCAL):
                {
                    uint16_t pos;
                    Value nvalue;
                    Value oldvalue;
                    pos = frame->readUint8();
                    nvalue = vmStackPop();
                    oldvalue = m_execstate.valuestack.get(frame->m_basepointer + pos);
                    if(!vmCheckAssign(oldvalue, nvalue))
                    {
                        goto onexecerror;
                    }
                    m_execstate.valuestack.set(frame->m_basepointer + pos, nvalue);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_GETLOCAL):
                {
                    size_t finalpos;
                    size_t pos;
                    Value val;
                    pos = frame->readUint8();
                    finalpos = frame->m_basepointer + pos;
                    val = m_execstate.valuestack.get(finalpos);
                    #if 0
                    {
                        Printer* pr = m_stderrprinter;
                        pr->format("GETLOCAL: finalpos=%ld val=<<<", finalpos);
                        mc_printer_printvalue(pr, val, true);
                        pr->format(">>>\n");
                    }
                    #endif
                    vmStackPush(val);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_GETGLOBALBUILTIN):
                {
                    uint16_t ix;
                    Value val;
                    ix = frame->readUint16();
                    ok = false;
                    val = m_vmglobalstore->getAtIndex(ix, &ok);
                    if(!ok)
                    {
                        pushError(Error::ERRTYP_RUNTIME, frame->getPosition(), "global value %d not found", ix);
                        goto onexecerror;
                    }
                    vmStackPush(val);
                }
                mc_vmmac_break();

            mcvm_case(OPCODE_FUNCTION):
                {
                    if(!mc_vmdo_makefunction(this, constants))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_GETFREE):
                {
                    uint16_t freeix;
                    Value val;
                    freeix = frame->readUint8();
                    val = mc_value_functiongetfreevalat(frame->m_function, freeix);
                    vmStackPush(val);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_SETFREE):
                {
                    uint16_t freeix;
                    Value val;
                    freeix = frame->readUint8();
                    val = vmStackPop();
                    mc_value_functionsetfreevalat(frame->m_function, freeix, val);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_CURRENTFUNCTION):
                {
                    Value currentfunction;
                    currentfunction = frame->m_function;
                    vmStackPush(currentfunction);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_GETTHIS):
                {
                    Value obj;
                    obj = vmStackThisGet(0);
                    vmStackPush(obj);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_GETDOTINDEX):
                {
                    if(!vmGetDotIndex())
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_GETINDEX):
                {
                    if(!vmGetIndexFull())
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_SETINDEX):
                {
                    if(!vmSetIndexFull())
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_DUP):
                {
                    Value val;
                    val = vmStackGet(0);
                    vmStackPush(val);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_FOREACHLEN):
                {
                    int len;
                    const char* tname;
                    Value val;
                    Value::Type type;
                    val = vmStackPop();
                    len = 0;
                    type = val.getType();
                    if(type == Value::VALTYP_ARRAY)
                    {
                        len = mc_value_arraygetlength(val);
                    }
                    else if(type == Value::VALTYP_MAP)
                    {
                        len = mc_state_mapgetlength(val);
                    }
                    else if(type == Value::VALTYP_STRING)
                    {
                        len = mc_value_stringgetlength(val);
                    }
                    else
                    {
                        tname = Value::getTypename(type);
                        pushError(Error::ERRTYP_RUNTIME, frame->getPosition(), "cannot get length of %s", tname);
                        goto onexecerror;
                    }
                    vmStackPush(Value::makeNumber(len));
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_NUMBER):
                {
                    uint64_t val;
                    mcfloat_t dval;
                    Value obj;
                    val = frame->readUint64();
                    dval = mc_util_uint64todouble(val);
                    obj = Value::makeNumber(dval);
                    vmStackPush(obj);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_SETRECOVER):
                {
                    uint16_t recip;
                    recip = frame->readUint16();
                    frame->m_recoverip = recip;
                }
                mc_vmmac_break();
        }
    onexecerror:
        m_hadrecovered = false;
        if(m_errorlist.count() > 0)
        {
            err = m_errorlist.getLast();
            if(err->m_errtype == Error::ERRTYP_RUNTIME && m_errorlist.count() >= 1)
            {
                recoverframeix = -1;
                for(fri = m_execstate.framestack.count() - 1; fri >= 0; fri--)
                {
                    frame = m_execstate.framestack.getp(fri);
                    if(frame->m_recoverip >= 0 && !frame->m_isrecovering)
                    {
                        recoverframeix = fri;
                        break;
                    }
                }
                if((int)recoverframeix < 0)
                {
                    goto onexecfinish;
                }
                if(err->m_traceback == nullptr)
                {
                    err->m_traceback = Memory::make<Traceback>();
                }
                if(err->m_traceback != nullptr)
                {
                    vmTracebackPush(err->m_traceback);
                }
                while(m_execstate.framestack.count() > (recoverframeix + 1))
                {
                    vmPopFrame();
                }
                errobj = mc_value_makeerror(this, err->m_message);
                if(!errobj.isNull())
                {
                    mc_value_errorsettraceback(errobj, err->m_traceback);
                    err->m_traceback = nullptr;
                }
                vmStackPush(errobj);
                m_execstate.currframe->m_bcposition = m_execstate.currframe->m_recoverip;
                m_execstate.currframe->m_isrecovering = true;
                m_errorlist.clear();
                m_hadrecovered = true;
            }
            else
            {
                goto onexecfinish;
            }
        }
        if(mc_state_gcshouldsweep(this) != 0)
        {
            mc_vm_rungc(this, constants);
        }
    }

onexecfinish:
    if(m_errorlist.count() > 0)
    {
        err = m_errorlist.getLast();
        if(err->m_traceback == nullptr)
        {
            err->m_traceback = Memory::make<Traceback>();
        }
        if(err->m_traceback != nullptr)
        {
            vmTracebackPush(err->m_traceback);
        }
    }
    mc_vm_rungc(this, constants);
    m_running = false;
    return m_errorlist.count() == 0;
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
    a_x = Value::asNumber(mc_state_mapgetvalue(args[0], keyx));
    a_y = Value::asNumber(mc_state_mapgetvalue(args[0], keyy));
    b_x = Value::asNumber(mc_state_mapgetvalue(args[1], keyx));
    b_y = Value::asNumber(mc_state_mapgetvalue(args[1], keyy));
    res = mc_value_makemap(state);
    if (res.getType() == Value::VALTYP_NULL)
    {
        return res;
    }
    mc_state_mapsetvalue(res, keyx, Value::makeNumber(a_x + b_x));
    mc_state_mapsetvalue(res, keyy, Value::makeNumber(a_y + b_y));
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
    a_x = Value::asNumber(mc_state_mapgetvalue(args[0], keyx));
    a_y = Value::asNumber(mc_state_mapgetvalue(args[0], keyy));
    b_x = Value::asNumber(mc_state_mapgetvalue(args[1], keyx));
    b_y = Value::asNumber(mc_state_mapgetvalue(args[1], keyy));
    res = mc_value_makemap(state);
    mc_state_mapsetvalue(res, keyx, Value::makeNumber(a_x - b_x));
    mc_state_mapsetvalue(res, keyy, Value::makeNumber(a_y - b_y));
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
        mc_state_mapsetvalue(res, key, val);
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
    pr = state->m_stdoutprinter;
    for(i = 0; i < argc; i++)
    {
        arg = args[i];
        mc_printer_printvalue(pr, arg, false);
        state->m_stdoutprinter->flush();
    }
    return Value::makeNull();
}

Value mc_scriptfn_println(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    Value o;
    (void)thisval;
    o = mc_scriptfn_print(state, data, thisval, argc, args);
    state->m_stdoutprinter->putChar('\n');
    state->m_stdoutprinter->flush();
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
        Printer::releaseFromPtr(&pr, false);
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
        Printer::releaseFromPtr(&pr, false);
        return Value::makeNull();
    }
    resstr = pr.getString();
    reslen = pr.getLength();
    res = mc_value_makestringlen(state, resstr, reslen);
    Printer::releaseFromPtr(&pr, false);
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
    state->pushError(Error::ERRTYP_RUNTIME, AstLocation::Invalid(), "cannot convert \"%s\" to number", string);
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
        while((temp = strstr(temp, searchstr)) != nullptr)
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
        while((temp = strstr(inpstr, searchstr)) != nullptr)
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
    while(((isspace(result[i]) != 0) || result[i] == '\t') && result[i] != '\0')
    {
        i++;
    }
    /* Trim whitespace from the end of the string */
    while(((isspace(result[j]) != 0) || result[j] == '\t') && j >= i)
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
    resstr.getAllocatedData()->m_uvobj.valstring.strbuf->toLowercase();
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
    resstr.getAllocatedData()->m_uvobj.valstring.strbuf->toUppercase();
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



Value mc_objfnarray_map(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t i;
    size_t len;
    Value val;
    Value res;
    Value callee;
    Value narr;
    (void)state;
    (void)data;
    (void)argc;
    callee = args[0];
    narr = mc_value_makearray(state);
    auto nary = mc_value_arraygetactualarray(narr);
    auto ary = mc_value_arraygetactualarray(thisval);
    len = ary->count();
    for(i=0; i<len; i++)
    {
        val = ary->get(i);
        #if 1
            Value vargs[] = {val};
        #else
            Value vargs[3];
            #if 0
            vargs[0] = val;
            vargs[1] = Value::makeNumber(i);
            #else
            vargs[0] = val;        
            #endif
        #endif
        //State::ExecInfo est;
        //state->saveExecInfo(&est);
        res = state->vmCallValue(state->m_sharedcompiler->getconstants(), callee, thisval, 1, vargs);
        //state->restoreExecInfo(&est);
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
    Printer::releaseFromPtr(&pr, false);
    return rt;
}

Value mc_objfnmap_length(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t len;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    len = mc_state_mapgetlength(thisval);
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
    len = mc_state_mapgetlength(map);
    arr = mc_value_makearray(state);
    for(i=0; i<len; i++)
    {
        strval = mc_state_mapgetkeyat(map, i);
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
        state->m_stdobjectobject = Memory::make<ObjClass>("Object", nullptr);
        state->m_stdobjectobject->addMember("isString", mc_objfnobject_isstring);
        state->m_stdobjectobject->addMember("isNumber", mc_objfnobject_isnumber);
        state->m_stdobjectobject->addMember("isArray", mc_objfnobject_isarray);
        state->m_stdobjectobject->addMember("isMap", mc_objfnobject_ismap);
        state->m_stdobjectobject->addMember("isFuncNative", mc_objfnobject_isfuncnative);
        state->m_stdobjectobject->addMember("isFuncScript", mc_objfnobject_isfuncscript);
        state->m_stdobjectobject->addMember("isExternal", mc_objfnobject_isexternal);
        state->m_stdobjectobject->addMember("isError", mc_objfnobject_iserror);
        state->m_stdobjectobject->addMember("isNull", mc_objfnobject_isnull);
        state->m_stdobjectobject->addMember("isBool", mc_objfnobject_isbool);
        state->m_stdobjectobject->addMember("isCallable", mc_objfnobject_iscallable);

    }
    {
        state->m_stdobjectnumber = Memory::make<ObjClass>("Number", state->m_stdobjectobject);
        state->m_stdobjectnumber->addMember("chr", mc_objfnnumber_chr);
        
    }
    {
        state->m_stdobjectstring = Memory::make<ObjClass>("String", state->m_stdobjectobject);
        state->m_stdobjectstring->addPseudo("length", mc_objfnstring_length);
        state->m_stdobjectstring->addMember("getself", mc_objfnstring_getself);
        state->m_stdobjectstring->addMember("toNumber", mc_objfnstring_tonumber);
        state->m_stdobjectstring->addMember("ord", mc_objfnstring_charcodefirst);
        state->m_stdobjectstring->addMember("charCodeAt", mc_objfnstring_charcodeat);
        state->m_stdobjectstring->addMember("charAt", mc_objfnstring_charat);
        state->m_stdobjectstring->addMember("indexOf", mc_objfnstring_indexof);
        state->m_stdobjectstring->addMember("left", mc_objfnstring_left);
        state->m_stdobjectstring->addMember("right", mc_objfnstring_right);
        state->m_stdobjectstring->addMember("replace", mc_objfnstring_replaceall);
        state->m_stdobjectstring->addMember("replacefirst", mc_objfnstring_replacefirst);
        state->m_stdobjectstring->addMember("match", mc_objfnstring_matchglobcase);
        state->m_stdobjectstring->addMember("imatch", mc_objfnstring_matchglobicase);
        state->m_stdobjectstring->addMember("trim", mc_objfnstring_trim);
        state->m_stdobjectstring->addMember("toLower", mc_objfnstring_tolower);
        state->m_stdobjectstring->addMember("toUpper", mc_objfnstring_toupper);
        state->m_stdobjectstring->addMember("split", mc_objfnstring_split);

    }
    {
        state->m_stdobjectarray = Memory::make<ObjClass>("Array", state->m_stdobjectobject);
        state->m_stdobjectarray->addPseudo("length", mc_objfnarray_length);
        state->m_stdobjectarray->addMember("push", mc_objfnarray_push);
        state->m_stdobjectarray->addMember("pop", mc_objfnarray_pop);
        state->m_stdobjectarray->addMember("join", mc_objfnarray_join);
        state->m_stdobjectarray->addMember("map", mc_objfnarray_map);
    }
    {
        state->m_stdobjectmap = Memory::make<ObjClass>("Map", state->m_stdobjectobject);
        state->m_stdobjectmap->addPseudo("length", mc_objfnmap_length);
        state->m_stdobjectmap->addMember("keys", mc_objfnmap_keys);
    }
    {
        state->m_stdobjectfunction = Memory::make<ObjClass>("Function", state->m_stdobjectobject);
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
            state->pushError(Error::ERRTYP_RUNTIME, AstLocation::Invalid(), "invalid argument %d passed to range, got %s instead of %s", ai, typestr, expectedstr);
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
        state->pushError(Error::ERRTYP_RUNTIME, AstLocation::Invalid(), "invalid number of arguments passed to range, got %d", argc);
        return Value::makeNull();
    }
    if(step == 0)
    {
        state->m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, AstLocation::Invalid(), "range step cannot be 0");
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
    len = mc_state_mapgetlength(arg);
    for(i = 0; i < len; i++)
    {
        key = mc_state_mapgetkeyat(arg, i);
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
    len = mc_state_mapgetlength(arg);
    for(i = 0; i < len; i++)
    {
        key = mc_state_mapgetvalueat(arg, i);
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
        state->m_errorlist.pushMessage(Error::ERRTYP_RUNTIME, state->m_execstate.currframe->getPosition(), mc_value_stringgetdata(args[0]));
    }
    else
    {
        state->m_errorlist.pushMessage(Error::ERRTYP_RUNTIME, state->m_execstate.currframe->getPosition(), "");
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
        state->m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, AstLocation::Invalid(), "assertion failed");
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
            state->m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, AstLocation::Invalid(), "max is bigger than min");
            return Value::makeNull();
        }
        range = max - min;
        res = min + (res * range);
        return Value::makeNumber(res);
    }
    state->m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, AstLocation::Invalid(), "invalid number or arguments");
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
    state->pushError(Error::ERRTYP_RUNTIME, AstLocation::Invalid(), "invalid argument 0 passed to slice, got %s instead", typestr);
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
    res = fabs(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_hypot(State* state, void* data, Value thisval, size_t argc, Value* args)
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
    res = hypot(arg1, arg2);
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
    if(contents == nullptr)
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
        mc_state_mapsetvaluestring(state, resmap, "dev", Value::makeNumber(st.st_dev));
        mc_state_mapsetvaluestring(state, resmap, "ino", Value::makeNumber(st.st_ino));
        mc_state_mapsetvaluestring(state, resmap, "mode", Value::makeNumber(st.st_mode));
        mc_state_mapsetvaluestring(state, resmap, "nlink", Value::makeNumber(st.st_nlink));
        mc_state_mapsetvaluestring(state, resmap, "uid", Value::makeNumber(st.st_uid));
        mc_state_mapsetvaluestring(state, resmap, "gid", Value::makeNumber(st.st_gid));
        mc_state_mapsetvaluestring(state, resmap, "size", Value::makeNumber(st.st_size));
        mc_state_mapsetvaluestring(state, resmap, "path", mc_value_makestring(state, fullpath));
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
    FSDirReader::Item ent;
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
    if(reader.openDir(path))
    {
        res = mc_value_makearray(state);
        while(reader.readItem(&ent))
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
        reader.closeDir();
        return res;
    }
    state->m_errorlist.pushMessage(Error::ERRTYP_RUNTIME, state->m_execstate.currframe->getPosition(), strerror(errno));
    return Value::makeNull();
}

Value mc_nsfnvm_hadrecovered(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    (void)data;
    (void)thisval;
    (void)argc;
    (void)args;
    return Value::makeBool(state->m_hadrecovered);
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
    mc_state_mapsetstrfunc(state, jmap, "stringify", mc_nsfnjson_stringify);
    state->setGlobalValue("JSON", jmap);
}

void mc_cli_installjsconsole(State* state)
{
    Value jmap;
    jmap = mc_value_makemap(state);
    mc_state_mapsetstrfunc(state, jmap, "log", mc_scriptfn_println);
    state->setGlobalValue("console", jmap);
}

void mc_cli_installmath(State* state)
{
    Value jmap;
    jmap = mc_value_makemap(state);
    mc_state_mapsetstrfunc(state, jmap, "sqrt", mc_nsfnmath_sqrt);
    mc_state_mapsetstrfunc(state, jmap, "pow", mc_nsfnmath_pow);
    mc_state_mapsetstrfunc(state, jmap, "sin", mc_nsfnmath_sin);
    mc_state_mapsetstrfunc(state, jmap, "cos", mc_nsfnmath_cos);
    mc_state_mapsetstrfunc(state, jmap, "tan", mc_nsfnmath_tan);
    mc_state_mapsetstrfunc(state, jmap, "log", mc_nsfnmath_log);
    mc_state_mapsetstrfunc(state, jmap, "ceil", mc_nsfnmath_ceil);
    mc_state_mapsetstrfunc(state, jmap, "floor", mc_nsfnmath_floor);
    mc_state_mapsetstrfunc(state, jmap, "abs", mc_nsfnmath_abs);
    mc_state_mapsetstrfunc(state, jmap, "hypot", mc_nsfnmath_hypot);
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
    mc_state_mapsetstrfunc(state, map, "read", mc_nsfnfile_readfile);
    mc_state_mapsetstrfunc(state, map, "write", mc_nsfnfile_writefile);
    mc_state_mapsetstrfunc(state, map, "put", mc_nsfnfile_writefile);
    mc_state_mapsetstrfunc(state, map, "join", mc_nsfnfile_join);
    mc_state_mapsetstrfunc(state, map, "isDirectory", mc_nsfnfile_isdirectory);
    mc_state_mapsetstrfunc(state, map, "isFile", mc_nsfnfile_isfile);
    mc_state_mapsetstrfunc(state, map, "stat", mc_nsfnfile_stat);
    state->setGlobalValue("File", map);
}

void mc_cli_installdir(State* state)
{
    Value map;
    map = mc_value_makemap(state);
    mc_state_mapsetstrfunc(state, map, "read", mc_nsfndir_readdir);
    state->setGlobalValue("Dir", map);
}

void mc_cli_installvmvar(State* state)
{
    Value map;
    map = mc_value_makemap(state);
    mc_state_mapsetstrfunc(state, map, "hadRecovered", mc_nsfnvm_hadrecovered);
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
    tmp = state->execProgram(program);
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
    printtypesize(StrDict);
    printtypesize(ValDict<Value, Value>);
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
    printtypesize(Object);
    printtypesize(ErrList);
    printtypesize(AstParser);
    printtypesize(RuntimeConfig);
    printtypesize(AstSymTable);
    printtypesize(AstCompiler);
    printtypesize(AstSymbol);
    printtypesize(Value);
    printtypesize(Value::CompareResult);
    printtypesize(AstLocation);
    printtypesize(AstToken);
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
    printtypesize(Object::ObjFunction);
    printtypesize(Object::ObjUserdata);
    printtypesize(Object::ObjError);
    printtypesize(Object::ObjString);
    printtypesize(AstCompiler::OpDefinition);
    printtypesize(AstScopeBlock);
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
    printtypesize(int16_t);
    printtypesize(uint8_t);
    printtypesize(int8_t);


}

static OptParser::LongFlags longopts[] =
{
    {"help", 'h', OptParser::OPTPARSE_NONE, "this help"},
    {"printsizes", 't', OptParser::OPTPARSE_NONE, "print type sizes"},
    {"eval", 'e', OptParser::OPTPARSE_REQUIRED, "evaluate a single line of code"},
    {"dumpast", 'a', OptParser::OPTPARSE_NONE, "dump AST after parsing"},
    {"quitafterdump", 'q', OptParser::OPTPARSE_NONE, "quit after dumping AST"},
    {"dumpbc", 'd', OptParser::OPTPARSE_NONE, "dump bytecode after compiling"},
    {"exitcompile", 'x', OptParser::OPTPARSE_NONE, "exit after compiling (for debugging)"},
    {"printins", 'p', OptParser::OPTPARSE_NONE, "print each instruction as it is being executed"},
    {"strict", 's', OptParser::OPTPARSE_NONE, "enable strict mode"},
    {0, 0, (OptParser::ArgType)0, nullptr}
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
    ok = true;
    evalcode = nullptr;
    ok = true;
    nargc = 0;
    mc_memory_init();
    state = Memory::make<State>();
    nargc = 0;
    OptParser options(argc, argv);
    options.m_willpermute = 0;
    while((opt = options.nextLongFlag(longopts, &longindex)) != -1)
    {
        co = longopts[longindex].m_shortname;
        if(opt == '?')
        {
            printf("%s: %s\n", argv[0], options.m_errmsg);
        }
        else if(co == 'h')
        {
            optprs_printusage(argv, longopts, false);
        }
        else if(co == 'e')
        {
            evalcode = options.m_optarg;
        }
        else if(co == 'a')
        {
            state->m_config.dumpast = true;
        }
        else if(co == 'q')
        {
            state->m_config.exitafterastdump = true;
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
        arg = options.nextPositional();
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
    mc_memory_finish();
    fprintf(stderr, "ok=%d\n", static_cast<int>(ok));
    if(ok)
    {
        return 0;
    }
    return 1;
}

