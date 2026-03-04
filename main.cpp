

#include <memory>
#include <functional>
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
    #define MC_INLINE inline __attribute__((always_inline))
#else
    #define MC_INLINE inline
#endif

#if 0
    #define MC_UTIL_CMPFLOAT(a, b) (fabs((a) - (b)) < DBL_EPSILON)
#else
    #define MC_UTIL_CMPFLOAT(a, b) ((a) == (b))
#endif

#if 0
    #define MC_ASSERT(x)
#else
    #define MC_ASSERT(x) Util::onAssertCall((x), #x, __FILE__, __LINE__, nullptr)
#endif

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

// does nothing; just for grepping
#define MC_PROTO

using NumFloat = double;
using OPValCode = uint16_t;
using NumShiftInt = uint32_t;

typedef struct mcstoddiyfp_t mcstoddiyfp_t;

class Object;
class Value;
class GCMemory;
class State;
class ErrList;
class Traceback;
class AstExpression;
class AstParser;

using CallbackNativeFN = Value (*)(State*, void*, Value, size_t, Value*);
using CallbackHashItemFN = size_t (*)(void*);
using CallbackCompareFN = bool (*)(void*, void*);
using CallbackDestroyFN = void (*)(void*);
using CallbackCopyFN = void* (*)(void*);

namespace Util
{
    /* must come before any other function/class body */
    template<typename... ArgsT>
    MC_INLINE void onAssertCall(bool x, const char* exprstr, const char* file, int line, const char* fmt, ArgsT&&... args)
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
}

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

class IOHandle
{
    public:
        struct Config
        {
            bool verbosefunc;
            bool quotstring;
            bool shouldflush;
        };

    public:
        static void destroy(IOHandle* pr)
        {
            pr->release(true);
            if(!pr->m_prisstack)
            {
                mc_memory_free(pr);
            }
        }

    private:
        MC_INLINE bool initFromStack(FILE* ofh, bool onstack)
        {
            return initFromStack(ofh, onstack, false);
        }

        MC_INLINE bool initFromStack(FILE* ofh, bool onstack, bool isnullctor)
        {
            m_prfailed = false;
            m_prdestrfile = ofh;
            m_prisstack = onstack;
            m_prdestbuf = nullptr;
            m_prconfig.verbosefunc = true;
            m_prconfig.quotstring = false;
            m_prconfig.shouldflush = false;
            if(!isnullctor)
            {
                if(m_prdestrfile == nullptr)
                {
                    m_prdestbuf = Memory::make<StringBuffer>(0);
                }
            }
            return true;
        }

    public:
        Config m_prconfig;
        bool m_prfailed;
        bool m_prisstack;
        FILE* m_prdestrfile;
        StringBuffer* m_prdestbuf;

    public:
        MC_INLINE IOHandle()
        {
            initFromStack(nullptr, true);
        }

        MC_INLINE IOHandle(FILE* ofh)
        {
            initFromStack(ofh, false);
        }

        MC_INLINE void release(bool took)
        {
            if(took)
            {
                return;
            }
            if(m_prdestbuf != nullptr)
            {
                StringBuffer::destroy(m_prdestbuf);
            }
        }

        MC_INLINE void flush()
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

        MC_INLINE bool put(const char* str, size_t len)
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

        MC_INLINE bool put(const char* str)
        {
            return put(str, strlen(str));
        }

        MC_INLINE bool putChar(int b)
        {
            char ch;
            ch = b;
            return put(&ch, 1);
        }

        template<typename... ArgsT>
        MC_INLINE bool format(const char* fmt, ArgsT&&... args)
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

        MC_INLINE void printEscapedChar(int ch)
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

        MC_INLINE void printEscapedString(const char* str, size_t len)
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

        MC_INLINE const char* getString()
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

        MC_INLINE size_t getLength()
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

        MC_INLINE char* takeStringAndDestroy(size_t* lendest)
        {
            char* res;
            if(m_prfailed)
            {
                IOHandle::destroy(this);
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
            IOHandle::destroy(this);
            return res;
        }

        MC_INLINE void printNumFloat(NumFloat flt)
        {
            int64_t inum;
            inum = (int64_t)flt;
            if(flt == inum)
            {
                #if defined(PRIiFAST64) && !defined(__CPPCHECK__)
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

namespace Util
{
    size_t hashData(const void* ptr, size_t len)
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

    size_t upperPowerOfTwo(size_t v)
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

    char* strDuplicate(const char* string, size_t n)
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

    char* strDuplicate(const char* string)
    {
        if(string == nullptr)
        {
            return nullptr;
        }
        return strDuplicate(string, strlen(string));
    }
}


template<typename StoredTyp>
class GenericList
{
    public:
        using OnCopyFN = std::function<StoredTyp(StoredTyp)>;
        using OnDestroyFN = std::function<void(StoredTyp)>;



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

        static void destroy(GenericList* list, OnDestroyFN dfn)
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

        static void clearAndDestroy(GenericList* list, OnDestroyFN dfn)
        {
            size_t i;
            for(i = 0; i < list->count(); i++)
            {
                auto item = list->get(i);
                if(dfn != nullptr)
                {
                    dfn(item);
                }
            }
            list->clear();
        }

        GenericList* copyToHeap(OnCopyFN copyfn, OnDestroyFN dfn)
        {
            bool ok;
            size_t i;
            GenericList* arrcopy;
            (void)ok;
            arrcopy = Memory::make<GenericList<StoredTyp>>(m_listcapacity);
            for(i = 0; i < count(); i++)
            {
                auto item = (StoredTyp)get(i);
                if(copyfn)
                {
                    auto itemcopy = (StoredTyp)copyfn(item);
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
            OnCopyFN dummycopy = nullptr;
            OnDestroyFN dummydel = nullptr;
            return copyToHeap(dummycopy, dummydel);
        }

        bool copyToStack(GenericList* dest, OnCopyFN copyfn, OnDestroyFN dfn)
        {
            bool ok;
            size_t i;
            (void)ok;
            (void)dfn;
            for(i = 0; i < count(); i++)
            {
                auto item = (StoredTyp)get(i);
                if(copyfn)
                {
                    auto itemcopy = (StoredTyp)copyfn(item);
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
            OnCopyFN dummycopy = nullptr;
            OnDestroyFN dummydel = nullptr;
            return copyToStack(dest, dummycopy, dummydel);
        }

        GenericList copyToStack(OnCopyFN copyfn, OnDestroyFN dfn)
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

    private:
        size_t m_listcapacity;
        size_t m_listcount;
        StoredTyp* m_listitems;

    private:
        bool removeAtIntern(unsigned int ix)
        {
            size_t tomovebytes;
            void* src;
            void* dest;
            if(ix == (m_listcount - 1))
            {
                m_listcount--;
                return true;
            }
            tomovebytes = (m_listcount - 1 - ix) * sizeof(StoredTyp);
            dest = m_listitems + (ix * sizeof(StoredTyp));
            src = m_listitems + ((ix + 1) * sizeof(StoredTyp));
            memmove(dest, src, tomovebytes);
            m_listcount--;
            return true;
        }

        void ensureCapacity(size_t needsize, const StoredTyp& fillval, bool first)
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
                    m_listitems = (StoredTyp*)mc_memory_malloc(sizeof(StoredTyp) * ncap);
                }
                else
                {
                    m_listitems = (StoredTyp*)mc_memory_realloc(m_listitems, sizeof(StoredTyp) * ncap);
                }
                for(i = oldcap; i < ncap; i++)
                {
                    m_listitems[i] = fillval;
                }
            }
        }

        template<typename OtherT>
        void moveFrom(OtherT* other)
        {
            m_listcount = other->m_listcount;
            m_listcapacity = other->m_listcapacity;
            m_listitems = other->m_listitems;
        }

    public:
        GenericList(): GenericList(0)
        {
        }

        GenericList(GenericList&& other)
        {
            moveFrom(&other);
        }

        GenericList(const GenericList& other)
        {
            moveFrom(&other);
        }

        GenericList(size_t initialsize)
        {
            m_listcount = 0;
            m_listcapacity = 0;
            m_listitems = nullptr;
            if(initialsize > 0)
            {
                if constexpr(std::is_pointer<StoredTyp>::value)
                {
                    ensureCapacity(initialsize, nullptr, true);
                }
                else
                {
                    ensureCapacity(initialsize, {}, true);                    
                }
            }
        }

        ~GenericList()
        {
            //deInit();
        }

        GenericList& operator=(const GenericList& other)
        {
            moveFrom(&other);
            return *this;
        }

        void orphanData()
        {
            #if 1
            m_listcount = 0;
            m_listcapacity = 0;
            m_listitems = nullptr;
            #endif
        }

        void deInit(OnDestroyFN dfn)
        {
            size_t i;
            for(i=0; i<m_listcount; i++)
            {
                auto item = get(i);
                dfn(item);
            }
            deInit();
        }

        void deInit()
        {
            if(m_listitems != nullptr)
            {
                mc_memory_free(m_listitems);
            }
            m_listitems = nullptr;
            m_listcount = 0;
            m_listcapacity = 0;
        }

        void clear()
        {
            m_listcount = 0;
        }

        size_t count() const
        {
            return m_listcount;
        }

        size_t capacity() const
        {
            return m_listcapacity;
        }

        StoredTyp* data() const
        {
            return m_listitems;
        }

        StoredTyp get(size_t idx) const
        {
            return m_listitems[idx];
        }

        StoredTyp* getp(size_t idx) const
        {
            return &m_listitems[idx];
        }

        StoredTyp top() const
        {
            if(m_listcount == 0)
            {
                if constexpr(std::is_pointer<StoredTyp>::value)
                {
                    return nullptr;
                }
                else
                {
                    return {};
                }
            }
            return get(m_listcount - 1);
        }

        StoredTyp* topp() const
        {
            int ofs = 0;
            if(m_listcount == 0)
            {
                return nullptr;
            }
            if(m_listcount > 0)
            {
                ofs = m_listcount - 1;
            }
            return getp(ofs);
        }


        StoredTyp* set(size_t idx, const StoredTyp& val)
        {
            size_t need;
            need = idx + 1;
            if(((idx == 0) || (m_listcapacity == 0)) || (idx >= m_listcapacity))
            {
                if constexpr(std::is_pointer<StoredTyp>::value)
                {
                    ensureCapacity(need, nullptr, false);
                }
                else
                {
                    ensureCapacity(need, {}, false);
                }
            }
            if(idx > m_listcount)
            {
                m_listcount = idx;
            }
            m_listitems[idx] = val;
            return &m_listitems[idx];
        }

        bool push(const StoredTyp& value)
        {
            size_t oldcap;
            if(m_listcapacity < m_listcount + 1)
            {
                oldcap = m_listcapacity;
                m_listcapacity = MC_UTIL_INCCAPACITY(oldcap);
                if(m_listitems == nullptr)
                {
                    m_listitems = (StoredTyp*)mc_memory_malloc(sizeof(StoredTyp) * m_listcapacity);
                }
                else
                {
                    m_listitems = (StoredTyp*)mc_memory_realloc(m_listitems, sizeof(StoredTyp) * m_listcapacity);
                }
            }
            m_listitems[m_listcount] = value;
            m_listcount++;
            return true;
        }

        bool pop(StoredTyp* dest)
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

        bool removeAt(unsigned int ix)
        {
            if(ix >= m_listcount)
            {
                return false;
            }
            if(ix == 0)
            {
                m_listitems += sizeof(StoredTyp);
                m_listcapacity--;
                m_listcount--;
                return true;
            }
            return removeAtIntern(ix);
        }

        void setEmpty()
        {
            if((m_listcapacity > 0) && (m_listitems != nullptr))
            {
                memset(m_listitems, 0, sizeof(StoredTyp) * m_listcapacity);
            }
            m_listcount = 0;
            m_listcapacity = 0;
        }
};



#include "strdict.h"
#include "valdict.h"

class StrView
{
    public:
        char* m_strdata = nullptr;
        size_t m_strlength = 0;

    private:
        template<typename InputT>
        MC_INLINE void copyFrom(InputT* other)
        {
            m_strdata = other->m_strdata;
            m_strlength = other->m_strlength;
        }

    public:
        MC_INLINE StrView()
        {
        }

        MC_INLINE ~StrView()
        {
        }

        template<typename InputT>
        MC_INLINE StrView(InputT* str, size_t length)
        {
            m_strdata = static_cast<char*>(str);
            m_strlength = length;
        }

        MC_INLINE StrView(const StrView& other)
        {
            copyFrom(&other);
        }

        MC_INLINE StrView(StrView&& other)
        {
            copyFrom(&other);
        }

        template<size_t len>
        MC_INLINE StrView(const char (&str)[len])
        {
            m_strdata = str;
            m_strlength = len;
        }

        MC_INLINE StrView& operator=(const StrView& other)
        {
            copyFrom(&other);
            return *this;
        }

        MC_INLINE void deAlloc()
        {
            if(m_strdata != nullptr)
            {
                mc_memory_free(m_strdata);
            }
            m_strdata = nullptr;
        }

        MC_INLINE size_t length() const
        {
            return m_strlength;
        }

        MC_INLINE char* data()
        {
            return m_strdata;
        }

        MC_INLINE char* data() const
        {
            return m_strdata;
        }
};

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

bool mc_util_strequal(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
}

bool mc_util_strnequal(const char* a, const char* b, size_t len)
{
    return strncmp(a, b, len) == 0;
}

bool mc_util_splitstring(GenericList<StrView>* dest, const char* str, const char* delimiter)
{
    size_t i;
    long len;
    long restlen;
    char* reststr;
    char* line;
    const char* lineend;
    const char* linestart;
    reststr = nullptr;
    if(str == nullptr)
    {
        return false;
    }
    linestart = str;
    lineend = strstr(linestart, delimiter);
    while(lineend != nullptr)
    {
        len = lineend - linestart;
        line = Util::strDuplicate(linestart, len);
        if(line == nullptr)
        {
            goto err;
        }
        dest->push(StrView(line, len));
        linestart = lineend + 1;
        lineend = strstr(linestart, delimiter);
    }
    reststr = Util::strDuplicate(linestart);
    if(reststr == nullptr)
    {
        goto err;
    }
    restlen = strlen(reststr);
    dest->push(StrView(reststr, restlen));
    return true;
err:
    mc_memory_free(reststr);
    if(dest->count() > 0)
    {
        for(i = 0; i < dest->count(); i++)
        {
            auto spitem = dest->getp(i);
            spitem->deAlloc();
        }
    }
    return false;
}

char* mc_util_joinstringarray(GenericList<StrView>* items, const char* joinee, size_t jlen)
{
    size_t i;
    IOHandle* res;
    res = Memory::make<IOHandle>(nullptr);
    for(i = 0; i < items->count(); i++)
    {
        auto item = items->get(i);
        res->put(item.data(), item.length());
        if(i < (items->count() - 1))
        {
            res->put(joinee, jlen);
        }
    }
    return res->takeStringAndDestroy(nullptr);
}

char* mc_util_canonpath(const char* strpath)
{
    size_t i;
    char* joined;
    const char* tmpstr;
    GenericList<StrView> spvals;
    if((strchr(strpath, '/') == nullptr) || ((strstr(strpath, "/../") == nullptr) && (strstr(strpath, "./") == nullptr)))
    {
        return Util::strDuplicate(strpath);
    }
    if(!mc_util_splitstring(&spvals, strpath, "/"))
    {
        return nullptr;
    }
    for(i = 0; i < spvals.count() - 1; i++)
    {
        auto stritem = spvals.get(i);
        auto nextitem = spvals.get(i + 1);
        if(mc_util_strequal(stritem.data(), "."))
        {
            stritem.deAlloc();
            spvals.removeAt(i);
            i = -1;
            continue;
        }
        if(mc_util_strequal(nextitem.data(), ".."))
        {
            stritem.deAlloc();
            nextitem.deAlloc();
            spvals.removeAt(i);
            spvals.removeAt(i);
            i = -1;
        }
    }
    tmpstr = "/";
    joined = mc_util_joinstringarray(&spvals, tmpstr, strlen(tmpstr));
    /*
    for(i = 0; i < spvals.count(); i++)
    {
        auto item = spvals.get(i);
        item.deAlloc();
    }
    */
    return joined;
}

bool mc_util_pathisabsolute(const char* strpath)
{
    return strpath[0] == '/';
}

size_t mc_util_hashdouble(NumFloat val)
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

NumFloat mc_util_strtod(const char* str, size_t slen, char** endptr)
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

uint64_t mc_util_doubletouint64(NumFloat val)
{
    union
    {
        uint64_t val_uint64;
        double val_double;
    } temp;
    temp.val_double = val;
    return temp.val_uint64;
}

NumFloat mc_util_uint64todouble(uint64_t val)
{
    union
    {
        uint64_t val_uint64;
        double val_double;
    } temp;
    temp.val_uint64 = val;
    return temp.val_double;
}

static MC_INLINE NumShiftInt mc_mathutil_binshiftleft(NumFloat dnleft, NumFloat dnright)
{
    int64_t sileft;
    int64_t siright;
    NumShiftInt ivleft;
    NumShiftInt ivright;
    if((dnleft < 0) || (dnright < 0))
    {
        sileft = (int64_t)dnleft;
        siright = (int64_t)dnright;
        return (NumFloat)(sileft << siright);
    }
    ivleft = (NumShiftInt)dnleft;
    ivright = (NumShiftInt)dnright;
    ivright &= 0x1f;
    return (ivleft << ivright);
}

static MC_INLINE NumShiftInt mc_mathutil_binshiftright(NumFloat dnleft, NumFloat dnright)
{
    int64_t sileft;
    int64_t siright;
    NumShiftInt ivleft;
    NumShiftInt ivright;
    if((dnleft < 0) || (dnright < 0))
    {
        sileft = (int64_t)dnleft;
        siright = (int64_t)dnright;
        return (NumFloat)(sileft >> siright);
    }
    ivleft = (NumShiftInt)dnleft;
    ivright = (NumShiftInt)dnright;
    ivright &= 0x1f;
    return (ivleft >> ivright);
}

#define mc_util_numtoint64(n) \
    ((int64_t)(n))

#define mc_mathutil_binor(dnleft, dnright) \
    ( \
        mc_util_numtoint64(dnleft) | mc_util_numtoint64(dnright) \
    )

#define mc_mathutil_binand(dnleft, dnright) \
    ( \
        mc_util_numtoint64(dnleft) & mc_util_numtoint64(dnright) \
    )

#define mc_mathutil_binxor(dnleft, dnright) \
    ( \
        mc_util_numtoint64(dnleft) ^ mc_util_numtoint64(dnright) \
    )

#define mc_mathutil_add(dnleft, dnright) \
    ( \
        (dnleft) + (dnright) \
    )

#define mc_mathutil_sub(dnleft, dnright) \
    ( \
        (dnleft) - (dnright) \
    )

#define mc_mathutil_mult(dnleft, dnright) \
    ( \
        (dnleft) * (dnright) \
    )

#define mc_mathutil_div(dnleft, dnright) \
    ( \
        (dnleft) / (dnright) \
    )

#define mc_mathutil_mod(dnleft, dnright) \
    ( \
        fmod((dnleft), (dnright)) \
    )

class SourceFile
{
    public:
        static void destroy(SourceFile* file)
        {
            size_t i;
            if(file != nullptr)
            {
                for(i = 0; i < file->m_srclines.count(); i++)
                {
                    auto item = file->m_srclines.get(i);
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
        SourceFile(const char* strpath)
        {
            size_t len;
            const char* lastslashpos;
            lastslashpos = strrchr(strpath, '/');
            if(lastslashpos != nullptr)
            {
                len = lastslashpos - strpath + 1;
                m_dirpath = Util::strDuplicate(strpath, len);
            }
            else
            {
                m_dirpath = Util::strDuplicate("");
            }
            MC_ASSERT(m_dirpath);
            m_path = Util::strDuplicate(strpath);
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

class SourceLocation
{
    public:
        static SourceLocation Invalid()
        {        
            return SourceLocation();
        }

    public:
        SourceFile* m_locfile;
        int m_locline;
        int m_loccolumn;

    public:
        SourceLocation(): SourceLocation(nullptr, 0, 0)
        {
        }

        SourceLocation(SourceFile* fi, int nlin, int coln)
        {
            m_locfile = fi;
            m_locline = nlin;
            m_loccolumn = coln;
        }
};

class CompiledProgram
{
    public:
        uint16_t* m_compiledbytecode;
        SourceLocation* m_progsrcposlist;
        int m_compiledcount;

    public:
        CompiledProgram(uint16_t* bc, SourceLocation* spl, int cnt)
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

class Instruction
{
    public:
        enum Code
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

        struct Definition
        {
            const char* name;
            int numoperands;
            int operandwidths[2];
        };

    public:
        static MC_INLINE Definition* makeOpDef(Definition* dest, const char* name, int numop, int opa1, int opa2)
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

        static MC_INLINE Definition* opdefLookup(Definition* def, Code op)
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

        static const char* opdefGetName(Code op)
        {
            Definition def;
            return opdefLookup(&def, op)->name;
        }

        static bool bcReadOperands(Definition* def, const OPValCode* instr, uint64_t outoperands[2])
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

        static void bcPrintOneInstruction(IOHandle* pr, OPValCode* code, OPValCode op, size_t* pos, SourceLocation* sposlist, bool simple)
        {
            int i;
            NumFloat dval;
            uint64_t operands[2];
            Definition* def;
            Definition vdef;
            SourceLocation srcpos;
            def = opdefLookup(&vdef, (Code)op);
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
            if(bcReadOperands(def, code + (*pos), operands))
            {
                for(i = 0; i < def->numoperands; i++)
                {
                    if(op == (OPValCode)OPCODE_NUMBER)
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

        static void bcPrintByteCodeTo(IOHandle* pr, OPValCode* code, SourceLocation* sposlist, size_t codesize, bool simple)
        {
            OPValCode op;
            size_t pos;
            pos = 0;
            while(pos < codesize)
            {
                op = code[pos];
                bcPrintOneInstruction(pr, code, op, &pos, sposlist, simple);
            }
        }
};

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
        /* can NOT be an enum class, because values must be combinable */
        enum Type
        {
            VT_NONE,
            VT_ERROR,
            VT_NUMBER,
            VT_BOOL,
            VT_STRING,
            VT_NULL,
            VT_FUNCNATIVE,
            VT_ARRAY,
            VT_MAP,
            VT_FUNCSCRIPT,
            VT_EXTERNAL,
            VT_FREED,
            /* for checking types with & */
            VT_ANY
        };


        struct CompareResult
        {
            NumFloat result;
        };

    public:
        Type m_valtype;
        bool m_isallocated;        
        union
        {
            Object* valobject;
            NumFloat valnumber;
            int valbool;
        } m_uval;
};

class GCMemory
{
    public:
        enum
        {
            MinPoolSize = (4),
            SweepInterval = (128),
        };

        class DataPool
        {
            public:
                GenericList<Object*> m_pooldata = GenericList<Object*>(MinPoolSize);
                int m_poolitemcount;
        };

    public:
        MC_PROTO static void wrapDestroyObjData(Object* data);

        static void destroyPool(DataPool* pool)
        {
            size_t j;
            Object* data;
            for(j = 0; j < (size_t)pool->m_poolitemcount; j++)
            {
                data = pool->m_pooldata.get(j);
                wrapDestroyObjData(data);
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
                m->m_gcobjlistremains.deInit();
                Memory::destroy(m->m_gcobjlistback);
                for(i = 0; i < m->m_gcobjliststored->count(); i++)
                {
                    obj = m->m_gcobjliststored->get(i);
                    wrapDestroyObjData(obj);
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
                    }
                }
                m->m_poolonlydata.m_pooldata.deInit();
                mc_memory_free(m);
            }
        }

        static void create()
        {
            GCMemory::m_myself = Memory::make<GCMemory>();
        }

        static void destroy()
        {
            destroy(GCMemory::m_myself);
        }

        static GCMemory* get()
        {
            return GCMemory::m_myself;
        }

    public:
        int m_allocssincesweep;
        GenericList<Object*>* m_gcobjliststored;
        GenericList<Object*>* m_gcobjlistback;
        GenericList<Value> m_gcobjlistremains;
        DataPool m_poolonlydata;
        DataPool m_poolarray;
        DataPool m_poolmap;
        DataPool m_poolstring;
        DataPool m_poolscriptfuncs;
        static GCMemory* m_myself;

    private:
        void initPool(DataPool* pool)
        {
            pool->m_poolitemcount = 0;
        }

    public:
        GCMemory()
        {
            m_gcobjliststored = Memory::make<GenericList<Object*>>();
            m_gcobjlistback = Memory::make<GenericList<Object*>>();
            m_allocssincesweep = 0;
            initPool(&m_poolonlydata);
            initPool(&m_poolarray);
            initPool(&m_poolmap);
            initPool(&m_poolstring);
            initPool(&m_poolscriptfuncs);
        }

        template<typename VType>
        MC_INLINE DataPool* getPoolForType(VType type)
        {
            switch(type)
            {
                case ValData::VT_FUNCSCRIPT:
                    return &m_poolscriptfuncs;
                case ValData::VT_ARRAY:
                    return &m_poolarray;
                case ValData::VT_MAP:
                    return &m_poolmap;
                case ValData::VT_STRING:
                    return &m_poolstring;
                default:
                    break;
            }
            return nullptr;
        }
};

class Object
{
    public:
        static Object* getDataFromPool(ValData::Type type)
        {
            Object* data;
            GCMemory::DataPool* pool;
            pool = GCMemory::get()->getPoolForType(type);
            if((pool == nullptr) || pool->m_poolitemcount <= 0)
            {
                return nullptr;
            }
            data = pool->m_pooldata.get(pool->m_poolitemcount - 1);
            /*
            * we want to make sure that appending to m_gcobjlistback never fails in sweep
            * so this only reserves space there.
            */
            GCMemory::get()->m_gcobjlistback->push(data);
            GCMemory::get()->m_gcobjliststored->push(data);
            pool->m_poolitemcount--;
            return data;
        }

    public:
        int8_t m_odtype;
        int8_t m_gcmark;
        GCMemory* m_objmem;

    public:
        MC_INLINE Object()
        {
        }
};

class Value: public ValData
{
    public:
        class ObjError: public Object
        {
            public:
                char* message;
                Traceback* traceback;

            public:
        };

        class ObjString: public Object
        {
            public:
                unsigned long m_hashval;
                StringBuffer* m_strbuf;

            public:

        };

        class ObjFunction: public Object
        {
            public:
                enum
                {
                    MaxFreeVal = (2),
                };

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
                        CallbackNativeFN natptrfn;
                        void* userpointer;
                    } valnativefunc;
                } m_funcdata;

            public:
                MC_INLINE bool freeValuesAreAllocated()
                {
                    return m_funcdata.valscriptfunc.freevalscount >= MaxFreeVal;
                }

                MC_INLINE const char* getName()
                {
                    if(m_funcdata.valscriptfunc.ownsdata)
                    {
                        return m_funcdata.valscriptfunc.unamev.fallocname;
                    }
                    return m_funcdata.valscriptfunc.unamev.fconstname;
                }

                MC_INLINE Value getFreeValAt(int ix)
                {
                    if(ix < 0 || ix >= m_funcdata.valscriptfunc.freevalscount)
                    {
                        return makeNull();
                    }
                    if(freeValuesAreAllocated())
                    {
                        return (Value)m_funcdata.valscriptfunc.ufv.freevalsallocated[ix];
                    }
                    return (Value)m_funcdata.valscriptfunc.ufv.freevalsstack[ix];
                }

                MC_INLINE void setFreeValAt(int ix, Value val)
                {
                    if(ix < 0 || ix >= m_funcdata.valscriptfunc.freevalscount)
                    {
                    }
                    else
                    {
                        if(freeValuesAreAllocated())
                        {
                            m_funcdata.valscriptfunc.ufv.freevalsallocated[ix] = val;
                        }
                        else
                        {
                            m_funcdata.valscriptfunc.ufv.freevalsstack[ix] = val;
                        }
                    }
                }

                MC_INLINE Value* getFreeVals()
                {
                    if(freeValuesAreAllocated())
                    {
                        return m_funcdata.valscriptfunc.ufv.freevalsallocated;
                    }
                    return (Value*)m_funcdata.valscriptfunc.ufv.freevalsstack;
                }
        };

        class ObjUserdata: public Object
        {
            public:
                void* data;
                CallbackDestroyFN datadestroyfn;
                CallbackCopyFN datacopyfn;
        };

        class ObjArray: public Object
        {
            public:
                GenericList<Value>* m_actualarray;

            public:
                MC_INLINE size_t count()
                {
                    return m_actualarray->count();
                }

                MC_INLINE size_t size()
                {
                    return m_actualarray->count();
                }

                MC_INLINE size_t length()
                {
                    return m_actualarray->count();
                }

                MC_INLINE Value getDirect(size_t i)
                {
                    return m_actualarray->get(i);
                }

                Value get(size_t ix)
                {
                    Value* res;
                    if(ix >= count())
                    {
                        return makeNull();
                    }
                    res = getp(ix);
                    if(res == nullptr)
                    {
                        return makeNull();
                    }
                    return *res;
                }

                MC_INLINE Value* getp(size_t i)
                {
                    return m_actualarray->getp(i);
                }

                MC_INLINE Value* setDirect(size_t i, const Value& val)
                {
                    return m_actualarray->set(i, val);
                }

                bool set(size_t ix, Value val)
                {
                    size_t len;
                    size_t toadd;
                    len = count();
                    if((ix >= len) || (len == 0))
                    {
                        toadd = len+1;
                        #if 0
                            fprintf(stderr, "ix=%d toadd=%d len=%d\n", ix, toadd, len);
                        #endif
                        while(toadd != (ix+2))
                        {
                            push(makeNull());
                            toadd++;
                        }
                    }
                    return setDirect(ix, val) != nullptr;
                }

                MC_INLINE bool push(const Value& val)
                {
                    return m_actualarray->push(val);
                }

                MC_INLINE bool pop(Value* dest)
                {
                    return m_actualarray->pop(dest);
                }

                MC_INLINE bool removeAt(size_t ix)
                {
                    return m_actualarray->removeAt(ix);
                }
        };

        class ObjMap: public Object
        {
            public:
                ValDict<Value, Value>* m_actualmap;

            public:
                MC_INLINE size_t count()
                {
                    return m_actualmap->count();
                }

                MC_INLINE bool mapHasKey(Value key)
                {
                    return (getValPtr(&key) != nullptr);
                }

                MC_INLINE Value getKeyAt(unsigned int ix)
                {
                    auto v = m_actualmap->getKeyAt(ix);
                    if(v == nullptr)
                    {
                        return makeNull();
                    }
                    return *v;
                }

                MC_INLINE Value getValueAt(unsigned int ix)
                {
                    auto v = m_actualmap->getValueAt(ix);
                    if(v == nullptr)
                    {
                        return makeNull();
                    }
                    return *v;
                }

                bool getValueChecked(Value key, Value* dest)
                {
                    Value* res;
                    res = getValPtr(&key);
                    if(res == nullptr)
                    {
                        *dest = makeNull();
                        return false;
                    }
                    *dest = *res;
                    return true;
                }

                MC_INLINE Value* getValPtr(Value* key)
                {
                    return m_actualmap->get(key);
                }

                Value getValue(Value key)
                {
                    Value* res;
                    res = getValPtr(&key);
                    if(res == nullptr)
                    {
                        return makeNull();
                    }
                    return *res;
                }

                MC_INLINE bool setKV(Value key, Value value)
                {
                    return m_actualmap->setKV(&key, &value);
                }

                MC_INLINE bool setValue(Value key, Value value)
                {
                    return m_actualmap->setKV(&key, &value);
                }

                MC_INLINE bool setValueAt(unsigned int ix, Value value)
                {
                    return m_actualmap->setValueAt(ix, &value);
                }

                bool setValString(const char* strkey, Value val)
                {
                    Value vkey;
                    vkey = makeString(strkey);
                    return setValue(vkey, val);
                }

                void setStrFunc(const char* fnname, CallbackNativeFN function)
                {
                    setValString(fnname, makeFuncNative(fnname, function, nullptr));
                }

                Value getKVPairAt(size_t ix)
                {
                    Value mapkey;
                    Value val;
                    Value res;
                    Value valobj;
                    Value keyobj;
                    if(ix >= count())
                    {
                        return makeNull();
                    }
                    mapkey = getKeyAt(ix);
                    val = getValueAt(ix);
                    res = makeMap();
                    auto m = res.asMap();
                    keyobj = makeString("key");
                    if(keyobj.isNull())
                    {
                        return makeNull();
                    }
                    m->setKV(keyobj, mapkey);
                    valobj = makeString("value");
                    if(valobj.isNull())
                    {
                        return makeNull();
                    }
                    m->setKV(valobj, val);
                    return res;
                }
        };

        class ValPrinter
        {
            public:
                static void valPrintObjFuncScript(IOHandle* pr, const Value& val)
                {
                    const char* fname;
                    auto fn = val.asFunction();
                    fname = fn->getName();
                    auto numlocals = fn->m_funcdata.valscriptfunc.numlocals;
                    auto numargs = fn->m_funcdata.valscriptfunc.numargs;
                    auto freevc = fn->m_funcdata.valscriptfunc.freevalscount;
                    pr->format("<scriptfunction '%s' locals=%d argc=%d fvc=%d", fname, numlocals, numargs, freevc);
                    #if 1
                    if(pr->m_prconfig.verbosefunc)
                    {
                        auto code = fn->m_funcdata.valscriptfunc.compiledprogcode->m_compiledbytecode;
                        auto poslist = fn->m_funcdata.valscriptfunc.compiledprogcode->m_progsrcposlist;
                        auto count = fn->m_funcdata.valscriptfunc.compiledprogcode->m_compiledcount;
                        pr->put(" [");
                        Instruction::bcPrintByteCodeTo(pr, code, poslist, count, true);
                        pr->put(" ]");
                    }
                    else
                    #endif
                    {
                    }
                    pr->put(">");
                }

                static void valPrintObjArray(IOHandle* pr, const Value& val)
                {
                    bool recursion;
                    size_t i;
                    size_t alen;
                    bool prevquot;
                    Value iobj;
                    auto actualary = val.asArray();
                    alen = actualary->size();
                    pr->put("[");
                    for(i = 0; i < alen; i++)
                    {
                        recursion = false;
                        iobj = actualary->get(i);
                        if(iobj.getType() == VT_ARRAY)
                        {
                            auto otherary = iobj.asArray();
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
                            doPrintValue(pr, iobj, false);
                        }
                        pr->m_prconfig.quotstring = prevquot;
                        if(i < (alen - 1))
                        {
                            pr->put(", ");
                        }
                    }
                    pr->put("]");
                }

                static void valPrintObjMap(IOHandle* pr, const Value& val)
                {
                    bool prevquot;
                    size_t i;
                    size_t alen;
                    Value mapkey;
                    Value mapval;
                    auto m = val.asMap();
                    alen = m->count();
                    pr->put("{");
                    for(i = 0; i < alen; i++)
                    {
                        mapkey = m->getKeyAt(i);
                        mapval = m->getValueAt(i);
                        prevquot = pr->m_prconfig.quotstring;
                        pr->m_prconfig.quotstring = true;
                        doPrintValue(pr, mapkey, false);
                        pr->put(": ");
                        doPrintValue(pr, mapval, false);
                        pr->m_prconfig.quotstring = prevquot;
                        if(i < (alen - 1))
                        {
                            pr->put(", ");
                        }
                    }
                    pr->put("}");
                }

                MC_PROTO static void valPrintObjError(IOHandle* pre, const Value& val);

                static void doPrintValue(IOHandle* pr, const Value& val, bool accurate)
                {
                    Type type;
                    (void)accurate;
                    type = val.getType();
                    switch(type)
                    {
                        case VT_FREED:
                            {
                                pr->put("FREED");
                            }
                            break;
                        case VT_NONE:
                            {
                                pr->put("NONE");
                            }
                            break;
                        case VT_NUMBER:
                            {
                                NumFloat number;
                                number = val.asNumber();
                                pr->printNumFloat(number);
                            }
                            break;
                        case VT_BOOL:
                            {
                                pr->put(val.asBool() ? "true" : "false");
                            }
                            break;
                        case VT_STRING:
                            {
                                valPrintObjString(pr, val);
                            }
                            break;
                        case VT_NULL:
                            {
                                pr->put("null");
                            }
                            break;
                        case VT_FUNCSCRIPT:
                            {
                                valPrintObjFuncScript(pr, val);
                            }
                            break;
                        case VT_ARRAY:
                            {
                                valPrintObjArray(pr, val);
                            }
                            break;
                        case VT_MAP:
                            {
                                valPrintObjMap(pr, val);
                            }
                            break;
                        case VT_FUNCNATIVE:
                            {
                                pr->put("FUNCNATIVE");
                            }
                            break;
                        case VT_EXTERNAL:
                            {
                                pr->put("EXTERNAL");
                            }
                            break;
                        case VT_ERROR:
                            {
                                valPrintObjError(pr, val);
                            }
                            break;
                        case VT_ANY:
                            {
                                MC_ASSERT(false);
                            }
                            break;
                    }
                }
        };

    public:
        static void markObject(Value obj)
        {
            int i;
            int len;
            Value key;
            Value mapval;
            Value freeval;
            Object* data;
            Object* valdata;
            Object* keydata;
            Object* freevaldata;
            if(obj.isAllocated())
            {
                data = obj.getAllocatedData<Object>();
                if(data->m_gcmark == 0)
                {
                    data->m_gcmark = 1;
                    switch(obj.getType())
                    {
                        case VT_MAP:
                            {
                                auto m = obj.asMap();
                                len = m->count();
                                for(i = 0; i < len; i++)
                                {
                                    key = m->getKeyAt(i);
                                    if(key.isAllocated())
                                    {
                                        keydata = key.getAllocatedData<Object>();
                                        if(keydata->m_gcmark == 0)
                                        {
                                            markObject(key);
                                        }
                                    }
                                    mapval = m->getValueAt(i);
                                    if(mapval.isAllocated())
                                    {
                                        valdata = mapval.getAllocatedData<Object>();
                                        if(valdata->m_gcmark == 0)
                                        {
                                            markObject(mapval);
                                        }
                                    }
                                }
                            }
                            break;
                        case VT_ARRAY:
                            {
                                auto arr = obj.asArray();
                                len = arr->size();
                                for(i = 0; i < len; i++)
                                {
                                    auto itm = arr->get(i);
                                    if(itm.isAllocated())
                                    {
                                        valdata = itm.getAllocatedData<Object>();
                                        if(valdata->m_gcmark == 0)
                                        {
                                            markObject(itm);
                                        }
                                    }
                                }
                            }
                            break;
                        case VT_FUNCSCRIPT:
                            {
                                break;
                                auto function = obj.asFunction();
                                for(i = 0; i < function->m_funcdata.valscriptfunc.freevalscount; i++)
                                {
                                    freeval = function->getFreeValAt(i);
                                    markObject(freeval);
                                    if(freeval.isAllocated())
                                    {
                                        freevaldata = freeval.getAllocatedData<Object>();
                                        if(freevaldata->m_gcmark == 0)
                                        {
                                            markObject(freeval);
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

        static void markListOfValues(Value* objects, size_t count)
        {
            size_t i;
            Value obj;
            for(i = 0; i < count; i++)
            {
                obj = objects[i];
                markObject(obj);
            }
        }

        template<typename DerivTyp>
        static DerivTyp* makeObjData(ValData::Type typ)
        {
            Object* data;
            data = Object::getDataFromPool(typ);
            if(data != nullptr)
            {
                data->m_odtype = typ;
                return (DerivTyp*)data;
            }
            GCMemory::get()->m_allocssincesweep++;
            if(GCMemory::get()->m_poolonlydata.m_poolitemcount > 0)
            {
                data = (DerivTyp*)GCMemory::get()->m_poolonlydata.m_pooldata.get(GCMemory::get()->m_poolonlydata.m_poolitemcount - 1);
                GCMemory::get()->m_poolonlydata.m_poolitemcount--;
            }
            else
            {
                data = Memory::make<DerivTyp>();
            }
            /*
            * we want to make sure that appending to m_gcobjlistback never fails in sweep
            * so this only reserves space there.
            */
            GCMemory::get()->m_gcobjlistback->push(data);
            GCMemory::get()->m_gcobjliststored->push(data);
            data->m_objmem = GCMemory::get();
            data->m_odtype = typ;
            return (DerivTyp*)data;
        }

        static void destroyObjData(Object* data)
        {
            switch(data->m_odtype)
            {
                case ValData::VT_FREED:
                    {
                        MC_ASSERT(false);
                    }
                    break;
                case ValData::VT_STRING:
                    {
                        StringBuffer::destroy(((ObjString*)data)->m_strbuf);
                    }
                    break;
                case ValData::VT_FUNCSCRIPT:
                    {
                        auto fn = (ObjFunction*)data;
                        if(fn->m_funcdata.valscriptfunc.ownsdata)
                        {
                            mc_memory_free(fn->m_funcdata.valscriptfunc.unamev.fallocname);
                            CompiledProgram::destroy(fn->m_funcdata.valscriptfunc.compiledprogcode);
                        }
                        if(fn->freeValuesAreAllocated())
                        {
                            mc_memory_free(fn->m_funcdata.valscriptfunc.ufv.freevalsallocated);
                        }
                    }
                    break;
                case ValData::VT_ARRAY:
                    {
                        auto arr = (ObjArray*)data;
                        Memory::destroy(arr->m_actualarray);
                        //Memory::destroy(arr);
                    }
                    break;
                case ValData::VT_MAP:
                    {
                        auto m = (ObjMap*)data;
                        Memory::destroy(m->m_actualmap);
                        //Memory::destroy(m);
                    }
                    break;
                case ValData::VT_FUNCNATIVE:
                    {
                        auto fn = (ObjFunction*)data;
                        mc_memory_free(fn->m_funcdata.valnativefunc.natfnname);
                    }
                    break;
                case ValData::VT_EXTERNAL:
                    {
                        auto ud = (ObjUserdata*)data;
                        if(ud->datadestroyfn != nullptr)
                        {
                            ud->datadestroyfn(ud->data);
                        }
                    }
                    break;
                case ValData::VT_ERROR:
                    {
                        auto e = (ObjError*)data;
                        mc_memory_free(e->message);
                        Memory::destroy(e->traceback);
                    }
                    break;
                default:
                    {
                    }
                    break;
            }
            data->m_odtype = ValData::VT_FREED;
        }

        static bool canStoreInPool(Object* data)
        {
            #if 0
            return false;
            #endif
            Value obj;
            GCMemory::DataPool* pool;
            obj = makeValueFromObject(data);
            /*
            * this is to ensure that large objects won't be kept in pool indefinitely
            */
            #if 1
            switch(data->m_odtype)
            {
                case VT_ARRAY:
                    {
                        if(obj.asArray()->size() > 1024)
                        {
                            return false;
                        }
                    }
                    break;
                case VT_MAP:
                    {
                        if(obj.asMap()->count() > 1024)
                        {
                            return false;
                        }
                    }
                    break;
                case VT_STRING:
                    {
                        #if 0
                        auto os = (ObjString*)data;
                        if(!os->isAllocated() || os->capacity() > 4096)
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
            pool= GCMemory::get()->getPoolForType(data->m_odtype);
            if(pool == nullptr)
            {
                return false;
            }
            return true;
        }

        static MC_INLINE bool isHashable(Value obj)
        {
            Type type = obj.getType();
            switch(type)
            {
                case VT_STRING:
                case VT_NUMBER:
                case VT_BOOL:
                    return true;
                default:
                    break;
            }
            return false;
        }

        static MC_INLINE const char* getTypename(Type type)
        {
            switch(type)
            {
                case VT_NONE:
                    return "none";
                case VT_ERROR:
                    return "error";
                case VT_NUMBER:
                    return "number";
                case VT_BOOL:
                    return "boolean";
                case VT_STRING:
                    return "string";
                case VT_NULL:
                    return "null";
                case VT_ARRAY:
                    return "array";
                case VT_MAP:
                    return "object";
                case VT_FUNCSCRIPT:
                case VT_FUNCNATIVE:
                    return "function";
                case VT_EXTERNAL:
                    return "external";
                case VT_FREED:
                    return "freed";
                case VT_ANY:
                    return "any";
                default:
                    break;
            }
            return "none";
        }

        static MC_INLINE void checkInbetween(IOHandle* res, Type type, Type t, bool* inbetween)
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

        static MC_INLINE char* getUnionName(Type type)
        {
            bool inbetween;
            IOHandle* res;
            if(type == VT_ANY || type == VT_NONE || type == VT_FREED)
            {
                return Util::strDuplicate(getTypename(type));
            }
            res = Memory::make<IOHandle>(nullptr);
            inbetween = false;
            checkInbetween(res, type, VT_NUMBER, &inbetween);
            checkInbetween(res, type, VT_BOOL, &inbetween);
            checkInbetween(res, type, VT_STRING, &inbetween);
            checkInbetween(res, type, VT_NULL, &inbetween);
            checkInbetween(res, type, VT_FUNCNATIVE, &inbetween);
            checkInbetween(res, type, VT_ARRAY, &inbetween);
            checkInbetween(res, type, VT_MAP, &inbetween);
            checkInbetween(res, type, VT_FUNCSCRIPT, &inbetween);
            checkInbetween(res, type, VT_EXTERNAL, &inbetween);
            checkInbetween(res, type, VT_ERROR, &inbetween);
            return res->takeStringAndDestroy(nullptr);
        }

        static MC_INLINE bool compare(Value a, Value b, CompareResult* cres)
        {
            bool isnumlike;
            const char* astring;
            const char* bstring;
            int alen;
            int blen;
            intptr_t adataval;
            intptr_t bdataval;
            NumFloat dnleft;
            NumFloat dnright;
            size_t ahash;
            size_t bhash;
            Type atype;
            Type btype;
            /*
            if(a.valobject == b.valobject)
            {
                return 0;
            }
            */
            cres->result = 1;
            atype = a.getType();
            btype = b.getType();
            isnumlike = (
                (atype == VT_NUMBER || atype == VT_BOOL || atype == VT_NULL) &&
                (btype == VT_NUMBER || btype == VT_BOOL || btype == VT_NULL)
            );
            if(isnumlike)
            {
                dnleft = a.asNumber();
                dnright = b.asNumber();
                cres->result = (dnleft - dnright);
                return true;
            }
            if(atype == btype && atype == VT_STRING)
            {
                alen = a.stringGetLength();
                blen = b.stringGetLength();
                if(alen != blen)
                {
                    cres->result = alen - blen;
                    return false;
                }
                ahash = a.stringGetHash();
                bhash = b.stringGetHash();
                if(ahash != bhash)
                {
                    cres->result = ahash - bhash;
                    return false;
                }
                astring = a.stringGetData();
                bstring = b.stringGetData();
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
                adataval = (intptr_t)a.getAllocatedData<Object>();
                bdataval = (intptr_t)b.getAllocatedData<Object>();
                cres->result = (NumFloat)(adataval - bdataval);
                return true;
            }
            return false;
        }

        static MC_INLINE bool equalsTo(Value a, Value b)
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
            NumFloat dval;
            Value obj;
            Type type;
            obj = *objptr;
            type = obj.getType();
            switch(type)
            {
                case VT_NUMBER:
                    {
                        dval = obj.asNumber();
                        return mc_util_hashdouble(dval);
                    }
                    break;
                case VT_BOOL:
                    {
                        bval = obj.asBool();
                        return static_cast<size_t>(bval);
                    }
                    break;
                case VT_STRING:
                    {
                        return obj.stringGetHash();
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
        static Value copyDeepFuncScript(Value obj, ValDict<TypeKeyT, TypeValueT>* targetdict)
        {
            int i;
            auto function = obj.asFunction();
            auto fvcnt = function->m_funcdata.valscriptfunc.freevalscount;
            auto bccnt = function->m_funcdata.valscriptfunc.compiledprogcode->m_compiledcount;
            auto bytecodecopy = (OPValCode*)mc_memory_malloc(sizeof(OPValCode) * bccnt);
            if(!bytecodecopy)
            {
                return makeNull();
            }
            memcpy(bytecodecopy, function->m_funcdata.valscriptfunc.compiledprogcode->m_compiledbytecode, sizeof(OPValCode) * bccnt);
            auto srcpositionscopy = (SourceLocation*)mc_memory_malloc(sizeof(SourceLocation) * bccnt);
            if(!srcpositionscopy)
            {
                mc_memory_free(bytecodecopy);
                return makeNull();
            }
            auto poslist = function->m_funcdata.valscriptfunc.compiledprogcode->m_progsrcposlist;
            memcpy(srcpositionscopy, poslist, sizeof(SourceLocation) * bccnt);
            auto comprescopy = Memory::make<CompiledProgram>(bytecodecopy, srcpositionscopy, bccnt);
            /*
            * todo: add compilation result copy function
            */
            if(!comprescopy)
            {
                mc_memory_free(srcpositionscopy);
                mc_memory_free(bytecodecopy);
                return makeNull();
            }
            auto nlocals = function->m_funcdata.valscriptfunc.numlocals;
            auto nargs = function->m_funcdata.valscriptfunc.numargs;
            auto name = function->getName();
            auto copy = makeFuncScript(name, comprescopy, true, nlocals, nargs, 0);
            if(copy.isNull())
            {
                CompiledProgram::destroy(comprescopy);
                return makeNull();
            }
            if(!targetdict->setKV(&obj, &copy))
            {
                return makeNull();
            }
            auto functioncopy = copy.asFunction();
            if(function->freeValuesAreAllocated())
            {
                functioncopy->m_funcdata.valscriptfunc.ufv.freevalsallocated = (Value*)mc_memory_malloc(sizeof(Value) * fvcnt);
                if(!functioncopy->m_funcdata.valscriptfunc.ufv.freevalsallocated)
                {
                    return makeNull();
                }
            }
            functioncopy->m_funcdata.valscriptfunc.freevalscount = fvcnt;
            for(i = 0; i < fvcnt; i++)
            {
                auto freeval = function->getFreeValAt(i);
                auto freevalcopy = copyDeepIntern(freeval, targetdict);
                if(!freeval.isNull() && freevalcopy.isNull())
                {
                    return makeNull();
                }
                functioncopy->setFreeValAt(i, freevalcopy);
            }
            return copy;
        }

        template<typename TypeKeyT, typename TypeValueT>
        static MC_INLINE Value copyDeepArray(Value obj, ValDict<TypeKeyT, TypeValueT>* targetdict)
        {
            int i;
            int len;
            Value copy;
            Value item;
            Value itemcopy;
            len = obj.asArray()->size();
            copy = makeArray(len);
            auto carr = copy.asArray();
            if(!targetdict->setKV(&obj, &copy))
            {
                return makeNull();
            }
            auto arr = obj.asArray();
            for(i = 0; i < len; i++)
            {
                item = arr->get(i);
                itemcopy = copyDeepIntern(item, targetdict);
                if(!item.isNull() && itemcopy.isNull())
                {
                    return makeNull();
                }
                carr->push(itemcopy);
            }
            return copy;
        }

        template<typename TypeKeyT, typename TypeValueT>
        static MC_INLINE Value copyDeepMap(Value obj, ValDict<TypeKeyT, TypeValueT>* targetdict)
        {
            size_t i;
            Value mapkey;
            Value mapval;
            Value copy;
            Value keycopy;
            Value valcopy;
            copy = makeMap();
            auto mcopy = copy.asMap();
            if(copy.isNull())
            {
                return makeNull();
            }
            if(!targetdict->setKV(&obj, &copy))
            {
                return makeNull();
            }
            auto m = obj.asMap();
            for(i = 0; i < m->count(); i++)
            {
                mapkey = m->getKeyAt(i);
                mapval = m->getValueAt(i);
                keycopy = copyDeepIntern(mapkey, targetdict);
                if(!mapkey.isNull() && keycopy.isNull())
                {
                    return makeNull();
                }
                valcopy = copyDeepIntern(mapval, targetdict);
                if(!mapval.isNull() && valcopy.isNull())
                {
                    return makeNull();
                }
                if(!mcopy->setValue(keycopy, valcopy))
                {
                    return makeNull();
                }
            }
            return copy;
        }

        /*
        * copyDeep* and copyFlat have rather confusing names. but i can't think of better ones...
        */
        template<typename TypeKeyT, typename TypeValueT>
        static Value copyDeepIntern(Value obj, ValDict<TypeKeyT, TypeValueT>* targetdict)
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
                case VT_FREED:
                case VT_ANY:
                case VT_NONE:
                    {
                        MC_ASSERT(false);
                        copy = makeNull();
                    }
                    break;
                case VT_NUMBER:
                case VT_BOOL:
                case VT_NULL:
                case VT_FUNCNATIVE:
                    {
                        copy = obj;
                    }
                    break;
                case VT_STRING:
                    {
                        bool ok;
                        int len;
                        const char* str;
                        (void)ok;
                        str = obj.stringGetData();
                        len = obj.stringGetLength();
                        copy = makeString(str, len);
                        ok = targetdict->setKV(&obj, &copy);
                        return copy;
                    }
                    break;
                case VT_FUNCSCRIPT:
                    {
                        return copyDeepFuncScript(obj, targetdict);
                    }
                    break;
                case VT_ARRAY:
                    {
                        return copyDeepArray(obj, targetdict);
                    }
                    break;
                case VT_MAP:
                    {
                        return copyDeepMap(obj, targetdict);
                    }
                    break;
                case VT_EXTERNAL:
                    {
                        copy = copyFlat(obj);
                    }
                    break;
                case VT_ERROR:
                    {
                        copy = obj;
                    }
                    break;
            }
            return copy;
        }

        static MC_INLINE Value copyDeep(Value obj)
        {
            Value res;
            auto targetdict = Memory::make<ValDict<Value, Value>>();
            res = copyDeepIntern(obj, targetdict);
            Memory::destroy(targetdict);
            return res;
        }

        static MC_INLINE Value copyFlat(Value obj)
        {
            bool ok;
            Value copy;
            Type type;
            (void)ok;
            copy = makeNull();
            type = obj.getType();
            switch(type)
            {
                case VT_ANY:
                case VT_FREED:
                case VT_NONE:
                    {
                        MC_ASSERT(false);
                        copy = makeNull();
                    }
                    break;
                case VT_NUMBER:
                case VT_BOOL:
                case VT_NULL:
                case VT_FUNCSCRIPT:
                case VT_FUNCNATIVE:
                case VT_ERROR:
                    {
                        copy = obj;
                    }
                    break;
                case VT_STRING:
                    {
                        size_t len;
                        const char* str;
                        str = obj.stringGetData();
                        len = obj.stringGetLength();
                        copy = makeString(str, len);
                    }
                    break;
                case VT_ARRAY:
                    {
                        int i;
                        int len;
                        Value item;
                        len = obj.asArray()->size();
                        copy = makeArray(len);
                        auto arr = copy.asArray();
                        auto carr = copy.asArray();
                        for(i = 0; i < len; i++)
                        {
                            item = arr->get(i);
                            ok = carr->push(item);
                            if(!ok)
                            {
                                return makeNull();
                            }
                        }
                    }
                    break;
                case VT_MAP:
                    {
                        size_t i;
                        Value mapkey;
                        Value mapval;
                        copy = makeMap();
                        auto mself = obj.asMap();
                        auto mcopy = copy.asMap();
                        for(i = 0; i < mself->count(); i++)
                        {
                            mapkey = mself->getKeyAt(i);
                            mapval = mself->getValueAt(i);
                            ok = mcopy->setValue(mapkey, mapval);
                            if(!ok)
                            {
                                return makeNull();
                            }
                        }
                    }
                    break;
                case VT_EXTERNAL:
                    {
                        void* datacopy;
                        ObjUserdata* objext;
                        copy = makeUserObj(nullptr);
                        if(copy.isNull())
                        {
                            return makeNull();
                        }
                        objext = obj.asUserdata();
                        datacopy = nullptr;
                        if(objext->datacopyfn != nullptr)
                        {
                            datacopy = objext->datacopyfn(objext->data);
                        }
                        else
                        {
                            datacopy = objext->data;
                        }
                        userdataSetData(copy, datacopy);
                        userdataSetDestroyFunc(copy, objext->datadestroyfn);
                        userdataSetCopyFunc(copy, objext->datacopyfn);
                    }
                    break;
            }
            return copy;
        }

        static MC_INLINE Value makeValueFromObject(Object* data)
        {
            Value vrt;
            memset(&vrt, 0, sizeof(Value));
            vrt.m_valtype = (Type)data->m_odtype;
            vrt.m_isallocated = true;
            vrt.m_uval.valobject = data;
            return vrt;
        }

        static MC_INLINE Value makeEmpty(Type t)
        {
            Value o = {};
            o.m_valtype = t;
            o.m_isallocated = false;
            return o;
        }

        static MC_INLINE Value makeNumber(NumFloat val)
        {
            Value o;
            o = makeEmpty(VT_NUMBER);
            o.m_uval.valnumber = val;
            return o;
        }

        static MC_INLINE Value makeBool(bool val)
        {
            Value o;
            o = makeEmpty(VT_BOOL);
            o.m_uval.valbool = static_cast<int>(val);
            return o;
        }

        static MC_INLINE Value makeNull()
        {
            Value o;
            o = makeEmpty(VT_NULL);
            return o;
        }

        static Value makeStrEmptyCapacity(int capacity)
        {
            auto data = makeObjData<ObjString>(VT_STRING);
            if(data == nullptr)
            {
                return makeNull();
            }
            data->m_hashval = 0;
            data->m_strbuf = Memory::make<StringBuffer>(capacity);
            return makeValueFromObject(data);
        }

        template<typename... ArgsT>
        static Value makeStrFormat(const char* fmt, ArgsT&&... args)
        {
            Value res;
            auto data = (ObjString*)Object::getDataFromPool(VT_STRING);
            res = makeStrEmptyCapacity(0);
            if(res.isNull())
            {
                return makeNull();
            }
            data->m_strbuf->appendFormat(fmt, args...);
            return res;
        }

        static Value makeString(const char* string, size_t len)
        {
            Value res;
            res = makeStrEmptyCapacity(len);
            if(res.isNull())
            {
                return res;
            }
            if(!res.stringAppend(string, len))
            {
                return makeNull();
            }
            return res;
        }

        static Value makeString(const char* string)
        {
            return makeString(string, strlen(string));
        }

        static Value makeFuncScript(const char* name, CompiledProgram* cres, bool ownsdt, int nlocals, int nargs, int fvc)
        {
            if(name == nullptr)
            {
                fprintf(stderr, "makeFuncScript: name is NULL!!\n");
            }
            auto data = makeObjData<ObjFunction>(VT_FUNCSCRIPT);
            if(data == nullptr)
            {
                return makeNull();
            }
            if(ownsdt)
            {
                
                data->m_funcdata.valscriptfunc.unamev.fallocname = (name != nullptr) ? Util::strDuplicate(name) : Util::strDuplicate("anonymous");
                if(data->m_funcdata.valscriptfunc.unamev.fallocname == nullptr)
                {
                    return makeNull();
                }
            }
            else
            {
                data->m_funcdata.valscriptfunc.unamev.fconstname = (name != nullptr) ? name : "anonymous";
            }
            data->m_funcdata.valscriptfunc.compiledprogcode = cres;
            data->m_funcdata.valscriptfunc.ownsdata = ownsdt;
            data->m_funcdata.valscriptfunc.numlocals = nlocals;
            data->m_funcdata.valscriptfunc.numargs = nargs;
            if(fvc >= ObjFunction::MaxFreeVal)
            {
                data->m_funcdata.valscriptfunc.ufv.freevalsallocated = (Value*)mc_memory_malloc(sizeof(Value) * fvc);
                if(data->m_funcdata.valscriptfunc.ufv.freevalsallocated == nullptr)
                {
                    return makeNull();
                }
            }
            data->m_funcdata.valscriptfunc.freevalscount = fvc;
            return makeValueFromObject(data);
        }

        static Value makeFuncNative(const char* name, CallbackNativeFN fn, void* data)
        {
            auto obj = makeObjData<ObjFunction>(VT_FUNCNATIVE);
            if(obj == nullptr)
            {
                return makeNull();
            }
            obj->m_funcdata.valnativefunc.natfnname = Util::strDuplicate(name);
            if(obj->m_funcdata.valnativefunc.natfnname == nullptr)
            {
                return makeNull();
            }
            obj->m_funcdata.valnativefunc.natptrfn = fn;
            if(data != nullptr)
            {
                obj->m_funcdata.valnativefunc.userpointer = data;
            }
            return makeValueFromObject(obj);
        }

        static Value makeArrayFromList(GenericList<Value>* list)
        {
            auto data = makeObjData<ObjArray>(VT_ARRAY);
            if(data == nullptr)
            {
                return makeNull();
            }
            data->m_actualarray = list;
            if(data->m_actualarray == nullptr)
            {
                return makeNull();
            }
            return makeValueFromObject(data);
        }

        static Value makeArray()
        {
            return makeArray(8);
        }

        static Value makeArray(size_t capacity)
        {
            auto arr = Memory::make<GenericList<Value>>(capacity);
            return makeArrayFromList(arr);
        }

        static Value makeMap()
        {
            return makeMap(32);
        }

        static Value makeMap(size_t capacity)
        {
            auto data = makeObjData<ObjMap>(VT_MAP);
            if(data == nullptr)
            {
                return makeNull();
            }
            data->m_actualmap = Memory::make<ValDict<Value, Value>>(capacity);
            if(data->m_actualmap == nullptr)
            {
                return makeNull();
            }
            data->m_actualmap->setHashFunction((CallbackHashItemFN)callbackHash);
            data->m_actualmap->setEqualsFunction((CallbackCompareFN)callbackEqualsTo);
            return makeValueFromObject(data);
        }

        static Value makeErrorNoCopy(char* error)
        {
            auto data = makeObjData<ObjError>(VT_ERROR);
            if(data == nullptr)
            {
                return makeNull();
            }
            data->message = error;
            data->traceback = nullptr;
            return makeValueFromObject(data);
        }

        static Value makeError(const char* error)
        {
            char* errorstr;
            Value res;
            errorstr = Util::strDuplicate(error);
            if(errorstr == nullptr)
            {
                return makeNull();
            }
            res = makeErrorNoCopy(errorstr);
            if(res.isNull())
            {
                mc_memory_free(errorstr);
                return makeNull();
            }
            return res;
        }

        static Value makeUserObj(void* data)
        {
            auto obj = makeObjData<ObjUserdata>(VT_EXTERNAL);
            if(obj == nullptr)
            {
                return makeNull();
            }
            obj->data = data;
            obj->datadestroyfn = nullptr;
            obj->datacopyfn = nullptr;
            return makeValueFromObject(obj);
        }

        static void valPrintObjString(IOHandle* pr, const Value& val)
        {
            size_t len;
            const char* str;
            str = val.stringGetData();
            len = val.stringGetLength();
            if(pr->m_prconfig.quotstring)
            {
                pr->printEscapedString(str, len);
            }
            else
            {
                pr->put(str, len);
            }
        }

        static bool userdataSetData(Value selfval, void* extdata)
        {
            ObjUserdata* data;
            MC_ASSERT(selfval.getType() == VT_EXTERNAL);
            data = selfval.asUserdata();
            if(data == nullptr)
            {
                return false;
            }
            data->data = extdata;
            return true;
        }

        static bool userdataSetDestroyFunc(Value selfval, CallbackDestroyFN dfn)
        {
            ObjUserdata* data;
            MC_ASSERT(selfval.getType() == VT_EXTERNAL);
            data = selfval.asUserdata();
            if(data == nullptr)
            {
                return false;
            }
            data->datadestroyfn = dfn;
            return true;
        }

        static bool userdataSetCopyFunc(Value selfval, CallbackCopyFN copyfn)
        {
            ObjUserdata* data;
            MC_ASSERT(selfval.getType() == VT_EXTERNAL);
            data = selfval.asUserdata();
            if(data == nullptr)
            {
                return false;
            }
            data->datacopyfn = copyfn;
            return true;
        }


        static const char* errorGetMessage(Value selfval)
        {
            auto data = selfval.getAllocatedData<ObjError>();
            return data->message;
        }

        static void errorSetTraceback(Value selfval, Traceback* traceback)
        {
            auto data = selfval.getAllocatedData<ObjError>();
            MC_ASSERT(data->traceback == nullptr);
            data->traceback = traceback;
        }

        static Traceback* errorGetTraceback(Value selfval)
        {
            auto data = selfval.getAllocatedData<ObjError>();
            return data->traceback;
        }

        static Value arrayPop(Value selfval)
        {
            Value dest;
            auto array = selfval.asArray();
            if(array->pop(&dest))
            {
                return dest;
            }
            return makeNull();
        }

        static bool arrayRemoveAt(Value selfval, size_t ix)
        {
            auto array = selfval.asArray();
            return array->removeAt(ix);
        }

        static void printValue(IOHandle* pr, const Value& val, bool accurate)
        {
            ValPrinter::doPrintValue(pr, val, accurate);
        }

    public:
        MC_INLINE Type getType() const
        {
            return m_valtype;
        }

        template<typename ActualT>
        MC_INLINE ActualT* getAllocatedData()
        {
            return static_cast<ActualT*>(m_uval.valobject);
        }

        template<typename ActualT>
        MC_INLINE ActualT* getAllocatedData() const
        {
            return static_cast<ActualT*>(m_uval.valobject);
        }

        MC_INLINE bool isAllocated() const
        {
            return m_isallocated;
        }

        MC_INLINE bool isNumeric() const
        {
            Type type;
            type = getType();
            return type == VT_NUMBER || type == VT_BOOL;
        }

        MC_INLINE bool isNumber() const
        {
            return (getType() == VT_NUMBER || getType() == VT_BOOL);
        }

        MC_INLINE bool isNull() const
        {
            return getType() == VT_NULL;
        }

        MC_INLINE bool isFuncNative() const
        {
            Type type;
            type = getType();
            return (type == VT_FUNCNATIVE);
        }

        MC_INLINE bool isFuncScript() const
        {
            Type type;
            type = getType();
            return (type == VT_FUNCSCRIPT);
        }

        MC_INLINE bool isCallable() const
        {
            return (isFuncNative() || isFuncScript());
        }

        MC_INLINE bool isString() const
        {
            Type type;
            type = getType();
            return (type == VT_STRING);
        }

        MC_INLINE bool isMap() const
        {
            Type type;
            type = getType();
            return (type == VT_MAP);
        }

        MC_INLINE bool isArray() const
        {
            Type type;
            type = getType();
            return (type == VT_ARRAY);
        }

        MC_INLINE bool asBool() const
        {
            if(isNumber())
            {
                return m_uval.valnumber != 0.0;
            }
            return m_uval.valbool != 0;
        }

        MC_INLINE NumFloat asNumber() const
        {
            if(isNumber())
            {
                if(getType() == VT_BOOL)
                {
                    return m_uval.valbool;
                }
                return m_uval.valnumber;
            }
            return m_uval.valnumber;
        }

        MC_INLINE ObjFunction* asFunction() const
        {
            return getAllocatedData<ObjFunction>();
        }

        MC_INLINE ObjUserdata* asUserdata() const
        {
            return getAllocatedData<ObjUserdata>();
        }

        MC_INLINE ObjArray* asArray()
        {
            return getAllocatedData<ObjArray>();
        }

        MC_INLINE ObjArray* asArray() const
        {
            auto data = getAllocatedData<ObjArray>();
            return data;
        }

        MC_INLINE ObjMap* asMap()
        {
            return getAllocatedData<ObjMap>();
        }

        MC_INLINE ObjMap* asMap() const
        {
            return getAllocatedData<ObjMap>();
        }

        MC_INLINE char* getStringDataIntern()
        {
            auto data = getAllocatedData<ObjString>();
            return data->m_strbuf->data();
        }

        MC_INLINE char* getStringDataIntern() const
        {
            auto data = getAllocatedData<ObjString>();
            return data->m_strbuf->data();
        }

        MC_INLINE const char* stringGetData() const
        {
            return getStringDataIntern();
        }

        MC_INLINE int stringGetLength() const
        {
            auto data = getAllocatedData<ObjString>();
            return data->m_strbuf->length();
        }

        MC_INLINE void stringSetLength(int len)
        {
            auto data = getAllocatedData<ObjString>();
            data->m_strbuf->setLength(len);
            this->stringRehash();
        }

        MC_INLINE char* stringGetMutableData()
        {
            return getStringDataIntern();
        }

        MC_INLINE bool stringAppend(const char* src, size_t len)
        {
            auto string = getAllocatedData<ObjString>();
            string->m_strbuf->append(src, len);
            this->stringRehash();
            return true;
        }

        MC_INLINE bool stringAppend(const char* src)
        {
            return this->stringAppend(src, strlen(src));
        }

        template<typename... ArgsT>
        MC_INLINE bool stringAppendFmt(const char* fmt, ArgsT&&... args)
        {
            auto string = getAllocatedData<ObjString>();
            string->m_strbuf->appendFormat(fmt, args...);
            this->stringRehash();
            return true;
        }

        MC_INLINE bool stringAppendValue(Value val)
        {
            bool ok;
            int vlen;
            const char* vstr;
            (void)ok;
            if(val.getType() == VT_NUMBER)
            {
                this->stringAppendFmt("%g", val.asNumber());
                goto finished;
            }
            if(val.getType() == VT_STRING)
            {
                vlen = val.stringGetLength();
                vstr = val.stringGetData();
                ok = this->stringAppend(vstr, vlen);
                if(!ok)
                {
                    return false;
                }
                goto finished;
            }
            return false;
            finished:
            this->stringRehash();
            return true;
        }

        MC_INLINE size_t stringGetHash() const
        {
            size_t len;
            const char* str;
            auto data = getAllocatedData<ObjString>();
            if(data->m_hashval == 0)
            {
                len = this->stringGetLength();
                str = this->stringGetData();
                data->m_hashval = Util::hashData(str, len);
            }
            return data->m_hashval;
        }

        MC_INLINE bool stringRehash()
        {
            size_t len;
            const char* str;
            auto data = getAllocatedData<ObjString>();
            len = this->stringGetLength();
            str = this->stringGetData();
            data->m_hashval = Util::hashData(str, len);
            return true;
        }
};

/*
* they absolutely MUST be identical in size.
* IF you need to add a field to Value, add it in ValData, since Value inherits ValData.
*/
static_assert(sizeof(ValData) == sizeof(Value));

class ObjClass
{
    public:
        class Field
        {
            public:
                const char* m_fieldname;
                bool m_ispseudo;
                CallbackNativeFN m_fndest;
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
        GenericList<Field> m_memberfields = GenericList<Field>(0);

    public:
        ObjClass(const char* name, ObjClass* parclass)
        {
            m_parentclass = parclass;
            m_classname = name;
            m_constructor = Value::makeNull();
        }

        void addFunction(const char* name, bool ispseudo, CallbackNativeFN fn)
        {
            Field bt;
            bt.m_fieldname = name;
            bt.m_ispseudo = ispseudo;
            bt.m_fndest = fn;
            m_memberfields.push(bt);
        }

        void addMember(const char* name, CallbackNativeFN fn)
        {
            return addFunction(name, false, fn);
        }

        void addPseudo(const char* name, CallbackNativeFN fn)
        {
            return addFunction(name, true, fn);
        }
};

class Traceback
{
    public:
        class Item
        {
            public:
                friend class Traceback;

            protected:
                char* m_tbtracefuncname;
                SourceLocation m_tbpos;

            public:
                const char* getTraceFunctionName()
                {
                    return m_tbtracefuncname;
                }

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
        friend class Item;

    private:
        GenericList<Item> m_tbitems;

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
            }
        }

        bool push(const char* fname, SourceLocation pos)
        {
            bool ok;
            Item item;
            (void)ok;
            item.m_tbtracefuncname = Util::strDuplicate(fname);
            if(item.m_tbtracefuncname == nullptr)
            {
                return false;
            }
            item.m_tbpos = pos;
            ok = m_tbitems.push(item);
            return true;
        }

        int getDepth()
        {
            return m_tbitems.count();
        }

        const char* getSourceFilename(int depth)
        {
            Item* item;
            item = m_tbitems.getp(depth);
            if(item == nullptr)
            {
                return nullptr;
            }
            return item->getSourceFilename();
        }

        const char* getSourceLineCode(int depth)
        {
            Item* item;
            item = m_tbitems.getp(depth);
            if(item == nullptr)
            {
                return nullptr;
            }
            return item->getSourceLine();
        }

        int getSourceLineNumber(int depth)
        {
            Item* item;
            item = m_tbitems.getp(depth);
            if(item == nullptr)
            {
                return -1;
            }
            return item->m_tbpos.m_locline;
        }

        int getSourceColumn(int depth)
        {
            Item* item;
            item = m_tbitems.getp(depth);
            if(item == nullptr)
            {
                return -1;
            }
            return item->m_tbpos.m_loccolumn;
        }

        const char* getFunctionName(int depth)
        {
            Item* item;
            item = m_tbitems.getp(depth);
            if(item == nullptr)
            {
                return "";
            }
            return item->getTraceFunctionName();
        }

        bool printTo(IOHandle* pr, Console::Color* mcc)
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

        static bool printUserError(IOHandle* pr, const Value& val)
        {
            const char* cred;
            const char* creset;
            Traceback* traceback;
            static const char eprefix[] = "script error";
            Console::Color mcc(fileno(stdout));
            cred = mcc.get('r');
            creset = mcc.get('0');
            pr->format("%s%s:%s %s\n", cred, eprefix, creset, Value::errorGetMessage(val));
            traceback = Value::errorGetTraceback(val);
            MC_ASSERT(traceback != nullptr);
            if(traceback != nullptr)
            {
                pr->format("%sTraceback:%s\n", cred, creset);
                traceback->printTo(pr, &mcc);
            }
            return true;
        }

        static bool printErrorTo(Error* err, IOHandle* pr)
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
        SourceLocation m_pos;
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

        bool printErrorTo(IOHandle* pr)
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

        void pushMessage(Error::Type type, SourceLocation pos, const char* message)
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
                len = strlen(message);
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
        void pushFormat(Error::Type type, SourceLocation pos, const char* format, ArgsT&&... args)
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
            m_symname = Util::strDuplicate(syname);
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
                store->m_storedsymbols->destroyItemsAndDict();
                store->m_storedobjects.deInit();
                mc_memory_free(store);
            }
        }

    public:
        StrDict<char*, AstSymbol*>* m_storedsymbols;
        GenericList<Value> m_storedobjects;

    public:
        SymStore()
        {
            m_storedsymbols = Memory::make<StrDict<char*, AstSymbol*>>((CallbackCopyFN)AstSymbol::copyHeap, (CallbackDestroyFN)AstSymbol::destroy);
        }

        AstSymbol* getSymbol(const char* name)
        {
            return m_storedsymbols->get(name);
        }

        bool setNamed(const char* name, Value val)
        {
            bool ok;
            int ix;
            AstSymbol* symbol;
            AstSymbol* existingsymbol;
            (void)ok;
            existingsymbol = getSymbol(name);
            if(existingsymbol != nullptr)
            {
                ok = (m_storedobjects.set(existingsymbol->m_symindex, val) != nullptr);
                return ok;
            }
            ix = m_storedobjects.count();
            ok = m_storedobjects.push(val);
            symbol = Memory::make<AstSymbol>(name, AstSymbol::SYMTYP_GLOBALBUILTIN, ix, false);
            ok = m_storedsymbols->set((char*)name, symbol);
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
            scope->m_blscopestore->destroyItemsAndDict();
            mc_memory_free(scope);
        }

        static AstScopeBlock* copy(AstScopeBlock* scope)
        {
            AstScopeBlock* copy;
            copy = Memory::make<AstScopeBlock>(scope->m_blscopeoffset, scope->m_blscopestore->copy());
            copy->m_blscopenumdefs = scope->m_blscopenumdefs;
            return copy;
        }

   public:
        StrDict<char*, AstSymbol*>* m_blscopestore;
        int m_blscopeoffset;
        int m_blscopenumdefs;

    public:
        AstScopeBlock(int ofs): AstScopeBlock(ofs, nullptr)
        {
        }

        AstScopeBlock(int ofs, StrDict<char*, AstSymbol*>* ss)
        {
            if(ss != nullptr)
            {
                m_blscopestore = ss;
            }
            else
            {
                m_blscopestore = Memory::make<StrDict<char*, AstSymbol*>>((CallbackCopyFN)AstSymbol::copyHeap, (CallbackDestroyFN)AstSymbol::destroy);
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
            bool ok;
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
                m_symtbfreesymbols = Memory::make<GenericList<AstSymbol*>>(0);
            }
            if(symodglobalsymbols != nullptr)
            {
                m_symtbmodglobalsymbols = *symodglobalsymbols;
            }
            ok = scopeBlockPush();
            MC_ASSERT(ok);
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
            existing = topscope->m_blscopestore->get(symbol->m_symname);
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
            AstSymbol* copy;
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
            if(!setSymbol(copy))
            {
                return false;
            }
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
            if(!setSymbol(symbol))
            {
                return nullptr;
            }
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
            /* module symbol */
            if(strchr(name, ':') != nullptr)
            {
                return nullptr;
            }
            auto symbol = Memory::make<AstSymbol>(name, AstSymbol::SYMTYP_FUNCTION, 0, assignable);
            if(!setSymbol(symbol))
            {
                return nullptr;
            }
            return symbol;
        }

        AstSymbol* defineThis()
        {
            auto symbol = Memory::make<AstSymbol>("this", AstSymbol::SYMTYP_THIS, 0, false);
            if(!setSymbol(symbol))
            {
                return nullptr;
            }
            return symbol;
        }

        AstSymbol* resolve(const char* name)
        {
            int i;
            AstScopeBlock* scope;
            scope = nullptr;
            auto symbol = m_symtbglobalstore->getSymbol(name);
            if(symbol != nullptr)
            {
                return symbol;
            }

            for(i = m_symtbblockscopes.count() - 1; i >= 0; i--)
            {
                scope = m_symtbblockscopes.get(i);
                symbol = scope->m_blscopestore->get(name);
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
            auto symbol = m_symtbglobalstore->getSymbol(name);
            if(symbol != nullptr)
            {
                return true;
            }
            auto topscope = m_symtbblockscopes.top();
            symbol = topscope->m_blscopestore->get(name);
            return symbol != nullptr;
        }

        bool scopeBlockPush()
        {
            int blockscopeoffset;
            blockscopeoffset = 0;
            auto prevblockscope = m_symtbblockscopes.top();
            if(prevblockscope != nullptr)
            {
                blockscopeoffset = m_symtbmodglobaloffset + prevblockscope->m_blscopeoffset + prevblockscope->m_blscopenumdefs;
            }
            else
            {
                blockscopeoffset = m_symtbmodglobaloffset;
            }
            auto newscope = Memory::make<AstScopeBlock>(blockscopeoffset);
            m_symtbblockscopes.push(newscope);
            return true;
        }

        void scopeBlockPop()
        {
            auto topscope = m_symtbblockscopes.top();
            m_symtbblockscopes.pop(nullptr);
            Memory::destroy(topscope);
        }

        AstScopeBlock* scopeBlockGet()
        {
            auto topscope = m_symtbblockscopes.top();
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
        GenericList<uint16_t> m_scopecompiledbc;
        GenericList<SourceLocation> m_scopesrcposlist;
        GenericList<int> m_scopeipstackbreak;
        GenericList<int> m_scopeipstackcontinue;
        OPValCode m_scopelastopcode = 0;

    public:
        AstScopeComp(AstScopeComp* ou)
        {
            m_outerscope = ou;
        }

        CompiledProgram* orphanResult()
        {
            auto bcdata = m_scopecompiledbc.data();
            auto astlocdata = m_scopesrcposlist.data();
            auto res = Memory::make<CompiledProgram>(bcdata, astlocdata, m_scopecompiledbc.count());
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
        SourceFile* m_filescopesourcefile = nullptr;
        GenericList<char*> m_filescopeloadednames;

    public:
        AstScopeFile()
        {
        }

        AstScopeFile(RuntimeConfig* cfg, ErrList* errlist, SourceFile* file)
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
        enum class Type
        {
            T_INVALID = 0,
            T_EOF,
            /* Operators */
            T_ASSIGN,
            T_ASSIGNPLUS,
            T_ASSIGNMINUS,
            T_ASSIGNASTERISK,
            T_ASSIGNSLASH,
            T_ASSIGNPERCENT,
            T_ASSIGNBINAND,
            T_ASSIGNBINOR,
            T_ASSIGNBINXOR,
            T_ASSIGNLSHIFT,
            T_ASSIGNRSHIFT,
            T_QUESTION,
            T_PLUS,
            T_PLUSPLUS,
            T_UNARYMINUS,
            T_MINUSMINUS,
            T_UNARYBINNOT,
            T_BANG,
            T_ASTERISK,
            T_SLASH,
            T_LESSTHAN,
            T_LESSEQUAL,
            T_GREATERTHAN,
            T_GREATEREQUAL,
            T_EQUAL,
            T_NOTEQ,
            T_LOGICALAND,
            T_LOGICALOR,
            T_BINAND,
            T_BINOR,
            T_BINXOR,
            T_LSHIFT,
            T_RSHIFT,
            /* Delimiters */
            T_COMMA,
            T_SEMICOLON,
            T_COLON,
            T_PARENOPEN,
            T_PARENCLOSE,
            T_BRACEOPEN,
            T_BRACECLOSE,
            T_BRACKETOPEN,
            T_BRACKETCLOSE,
            T_DOT,
            T_MATHMODULO,
            /* Keywords */
            T_KWFUNCTION,
            T_KWCONST,
            T_KWVAR,
            T_KWTRUE,
            T_KWFALSE,
            T_KWIF,
            T_KWELSE,
            T_KWRETURN,
            T_KWWHILE,
            T_KWBREAK,
            T_KWFOR,
            T_KWIN,
            T_KWCONTINUE,
            T_KWNULL,
            T_KWIMPORT,
            T_KWRECOVER,
            /* Identifiers and literals */
            T_IDENT,
            T_NUMBER,
            T_STRING,
            T_TEMPLATESTRING,
            /* MUST be last. */
            T_TYPEMAX
        };

    public:
        static const char* tokenName(Type type)
        {
            switch(type)
            {
                case Type::T_EOF:
                    return "EOF";
                case Type::T_ASSIGN:
                    return "=";
                case Type::T_ASSIGNPLUS:
                    return "+=";
                case Type::T_ASSIGNMINUS:
                    return "-=";
                case Type::T_ASSIGNASTERISK:
                    return "*=";
                case Type::T_ASSIGNSLASH:
                    return "/=";
                case Type::T_ASSIGNPERCENT:
                    return "%=";
                case Type::T_ASSIGNBINAND:
                    return "&=";
                case Type::T_ASSIGNBINOR:
                    return "|=";
                case Type::T_ASSIGNBINXOR:
                    return "^=";
                case Type::T_ASSIGNLSHIFT:
                    return "<<=";
                case Type::T_ASSIGNRSHIFT:
                    return ">>=";
                case Type::T_QUESTION:
                    return "?";
                case Type::T_PLUS:
                    return "+";
                case Type::T_PLUSPLUS:
                    return "++";
                case Type::T_UNARYMINUS:
                    return "-";
                case Type::T_MINUSMINUS:
                    return "--";
                case Type::T_BANG:
                    return "!";
                case Type::T_ASTERISK:
                    return "*";
                case Type::T_SLASH:
                    return "/";
                case Type::T_LESSTHAN:
                    return "<";
                case Type::T_LESSEQUAL:
                    return "<=";
                case Type::T_GREATERTHAN:
                    return ">";
                case Type::T_GREATEREQUAL:
                    return ">=";
                case Type::T_EQUAL:
                    return "==";
                case Type::T_NOTEQ:
                    return "!=";
                case Type::T_LOGICALAND:
                    return "&&";
                case Type::T_LOGICALOR:
                    return "||";
                case Type::T_BINAND:
                    return "&";
                case Type::T_BINOR:
                    return "|";
                case Type::T_BINXOR:
                    return "^";
                case Type::T_LSHIFT:
                    return "<<";
                case Type::T_RSHIFT:
                    return ">>";
                case Type::T_COMMA:
                    return ",";
                case Type::T_SEMICOLON:
                    return ";";
                case Type::T_COLON:
                    return ":";
                case Type::T_PARENOPEN:
                    return "(";
                case Type::T_PARENCLOSE:
                    return ")";
                case Type::T_BRACEOPEN:
                    return "{";
                case Type::T_BRACECLOSE:
                    return "}";
                case Type::T_BRACKETOPEN:
                    return "[";
                case Type::T_BRACKETCLOSE:
                    return "]";
                case Type::T_DOT:
                    return ".";
                case Type::T_MATHMODULO:
                    return "%";
                case Type::T_KWFUNCTION:
                    return "FUNCTION";
                case Type::T_KWCONST:
                    return "CONST";
                case Type::T_KWVAR:
                    return "VAR";
                case Type::T_KWTRUE:
                    return "TRUE";
                case Type::T_KWFALSE:
                    return "FALSE";
                case Type::T_KWIF:
                    return "IF";
                case Type::T_KWELSE:
                    return "ELSE";
                case Type::T_KWRETURN:
                    return "RETURN";
                case Type::T_KWWHILE:
                    return "WHILE";
                case Type::T_KWBREAK:
                    return "BREAK";
                case Type::T_KWFOR:
                    return "FOR";
                case Type::T_KWIN:
                    return "IN";
                case Type::T_KWCONTINUE:
                    return "CONTINUE";
                case Type::T_KWNULL:
                    return "nullptr";
                case Type::T_KWIMPORT:
                    return "IMPORT";
                case Type::T_KWRECOVER:
                    return "RECOVER";
                case Type::T_IDENT:
                    return "IDENT";
                case Type::T_NUMBER:
                    return "NUMBER";
                case Type::T_STRING:
                    return "STRING";
                case Type::T_TEMPLATESTRING:
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
        SourceLocation m_tokpos;

    public:
        AstToken(): AstToken(Type::T_EOF, "", 0)
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
            return Util::strDuplicate(m_tokstrdata, m_tokstrlength);
        }

        MC_INLINE Type type() const
        {
            return m_toktype;
        }
};

class AstLexData
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
        SourceFile* m_file;
        bool m_failed;
        bool m_continuetplstring;
        AstToken m_prevtoken;
        AstToken m_currtoken;
        AstToken m_peektoken;
};

class AstLexer: public AstLexData
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

        static bool init(AstLexer* lex, ErrList* errs, const char* input, SourceFile* file)
        {
            bool ok;
            (void)ok;
            lex->m_errors = errs;
            lex->m_inputsource = input;
            lex->m_inputlength = (int)strlen(input);
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
            lex->m_prevtoken = AstToken(AstToken::Type::T_INVALID, nullptr, 0);
            lex->m_currtoken = AstToken(AstToken::Type::T_INVALID, nullptr, 0);
            lex->m_peektoken = AstToken(AstToken::Type::T_INVALID, nullptr, 0);
            return true;
        }

    public:
        AstLexData m_prevstate;

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
            if(m_prevtoken.type() == AstToken::Type::T_INVALID)
            {
                return false;
            }
            m_peektoken = m_currtoken;
            m_currtoken = m_prevtoken;
            m_prevtoken = AstToken(AstToken::Type::T_INVALID, nullptr, 0);
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
                outtok.m_toktype = AstToken::Type::T_INVALID;
                outtok.m_tokstrdata = m_inputsource + m_position;
                outtok.m_tokstrlength = 1;
                outtok.m_tokpos = SourceLocation(m_file, m_line, m_column);
                c = m_continuetplstring ? '`' : m_currentchar;
                switch(c)
                {
                    case '\0':
                        {
                            outtok = AstToken(AstToken::Type::T_EOF, "EOF", 3);
                        }
                        break;
                    case '=':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::Type::T_EQUAL, "==", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_ASSIGN, "=", 1);
                            }
                        }
                        break;
                    case '&':
                        {
                            if(peekChar() == '&')
                            {
                                outtok = AstToken(AstToken::Type::T_LOGICALAND, "&&", 2);
                                readChar();
                            }
                            else if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::Type::T_ASSIGNBINAND, "&=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_BINAND, "&", 1);
                            }
                        }
                        break;
                    case '|':
                        {
                            if(peekChar() == '|')
                            {
                                outtok = AstToken(AstToken::Type::T_LOGICALOR, "||", 2);
                                readChar();
                            }
                            else if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::Type::T_ASSIGNBINOR, "|=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_BINOR, "|", 1);
                            }
                        }
                        break;
                    case '^':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::Type::T_ASSIGNBINXOR, "^=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_BINXOR, "^", 1);
                                break;
                            }
                        }
                        break;
                    case '+':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::Type::T_ASSIGNPLUS, "+=", 2);
                                readChar();
                            }
                            else if(peekChar() == '+')
                            {
                                outtok = AstToken(AstToken::Type::T_PLUSPLUS, "++", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_PLUS, "+", 1);
                                break;
                            }
                        }
                        break;
                    case '-':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::Type::T_ASSIGNMINUS, "-=", 2);
                                readChar();
                            }
                            else if(peekChar() == '-')
                            {
                                outtok = AstToken(AstToken::Type::T_MINUSMINUS, "--", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_UNARYMINUS, "-", 1);
                                break;
                            }
                        }
                        break;
                    case '~':
                        {
                            outtok = AstToken(AstToken::Type::T_UNARYBINNOT, "~", 1);
                        }
                        break;
                    case '!':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::Type::T_NOTEQ, "!=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_BANG, "!", 1);
                            }
                        }
                        break;
                    case '*':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::Type::T_ASSIGNASTERISK, "*=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_ASTERISK, "*", 1);
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
                                outtok = AstToken(AstToken::Type::T_ASSIGNSLASH, "/=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_SLASH, "/", 1);
                                break;
                            }
                        }
                        break;
                    case '<':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::Type::T_LESSEQUAL, "<=", 2);
                                readChar();
                            }
                            else if(peekChar() == '<')
                            {
                                readChar();
                                if(peekChar() == '=')
                                {
                                    outtok = AstToken(AstToken::Type::T_ASSIGNLSHIFT, "<<=", 3);
                                    readChar();
                                }
                                else
                                {
                                    outtok = AstToken(AstToken::Type::T_LSHIFT, "<<", 2);
                                }
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_LESSTHAN, "<", 1);
                                break;
                            }
                        }
                        break;
                    case '>':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::Type::T_GREATEREQUAL, ">=", 2);
                                readChar();
                            }
                            else if(peekChar() == '>')
                            {
                                readChar();
                                if(peekChar() == '=')
                                {
                                    outtok = AstToken(AstToken::Type::T_ASSIGNRSHIFT, ">>=", 3);
                                    readChar();
                                }
                                else
                                {
                                    outtok = AstToken(AstToken::Type::T_RSHIFT, ">>", 2);
                                }
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_GREATERTHAN, ">", 1);
                            }
                        }
                        break;
                    case ',':
                        {
                            outtok = AstToken(AstToken::Type::T_COMMA, ",", 1);
                        }
                        break;
                    case ';':
                        {
                            outtok = AstToken(AstToken::Type::T_SEMICOLON, ";", 1);
                        }
                        break;
                    case ':':
                        {
                            outtok = AstToken(AstToken::Type::T_COLON, ":", 1);
                        }
                        break;
                    case '(':
                        {
                            outtok = AstToken(AstToken::Type::T_PARENOPEN, "(", 1);
                        }
                        break;
                    case ')':
                        {
                            outtok = AstToken(AstToken::Type::T_PARENCLOSE, ")", 1);
                        }
                        break;
                    case '{':
                        {
                            outtok = AstToken(AstToken::Type::T_BRACEOPEN, "{", 1);
                        }
                        break;
                    case '}':
                        {
                            outtok = AstToken(AstToken::Type::T_BRACECLOSE, "}", 1);
                        }
                        break;
                    case '[':
                        {
                            outtok = AstToken(AstToken::Type::T_BRACKETOPEN, "[", 1);
                        }
                        break;
                    case ']':
                        {
                            outtok = AstToken(AstToken::Type::T_BRACKETCLOSE, "]", 1);
                        }
                        break;
                    case '.':
                        {
                            outtok = AstToken(AstToken::Type::T_DOT, ".", 1);
                        }
                        break;
                    case '?':
                        {
                            outtok = AstToken(AstToken::Type::T_QUESTION, "?", 1);
                        }
                        break;
                    case '%':
                        {
                            if(peekChar() == '=')
                            {
                                outtok = AstToken(AstToken::Type::T_ASSIGNPERCENT, "%=", 2);
                                readChar();
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_MATHMODULO, "%", 1);
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
                                outtok = AstToken(AstToken::Type::T_STRING, str, len);
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_INVALID, nullptr, 0);
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
                                outtok = AstToken(AstToken::Type::T_STRING, str, len);
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_INVALID, nullptr, 0);
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
                                    outtok = AstToken(AstToken::Type::T_TEMPLATESTRING, str, len);
                                }
                                else
                                {
                                    outtok = AstToken(AstToken::Type::T_STRING, str, len);
                                }
                            }
                            else
                            {
                                outtok = AstToken(AstToken::Type::T_INVALID, nullptr, 0);
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
                                outtok.m_tokpos = SourceLocation(m_file, m_line, m_column);
                                return outtok;
                            }
                            if(charIsDigit(m_currentchar))
                            {
                                numberlen = 0;
                                number = scanNumber(&numberlen);
                                outtok = AstToken(AstToken::Type::T_NUMBER, number, numberlen);
                                outtok.m_tokpos = SourceLocation(m_file, m_line, m_column);
                                return outtok;
                            }
                        }
                        break;
                }
                readChar();
                if(failed())
                {
                    outtok = AstToken(AstToken::Type::T_INVALID, nullptr, 0);
                }
                m_continuetplstring = false;
                outtok.m_tokpos = SourceLocation(m_file, m_line, m_column);
                return outtok;
            }
            /* NB. never reached; but keep the compiler from complaining. */
            return outtok;
        }

        bool expectCurrent(AstToken::Type type)
        {
            if(failed())
            {
                return false;
            }
            if(!currentTokenIs(type))
            {
                auto expectedtypestr = AstToken::tokenName(type);
                auto actualtypestr = AstToken::tokenName(m_currtoken.type());
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
                { "function", AstToken::Type::T_KWFUNCTION },
                { "const", AstToken::Type::T_KWCONST },
                { "var", AstToken::Type::T_KWVAR },
                { "let", AstToken::Type::T_KWVAR },
                { "true", AstToken::Type::T_KWTRUE },
                { "false", AstToken::Type::T_KWFALSE },
                { "if", AstToken::Type::T_KWIF },
                { "else", AstToken::Type::T_KWELSE },
                { "return", AstToken::Type::T_KWRETURN },
                { "while", AstToken::Type::T_KWWHILE },
                { "break", AstToken::Type::T_KWBREAK },
                { "for", AstToken::Type::T_KWFOR },
                { "in", AstToken::Type::T_KWIN },
                { "continue", AstToken::Type::T_KWCONTINUE },
                { "null", AstToken::Type::T_KWNULL },
                { "import", AstToken::Type::T_KWIMPORT },
                { "recover", AstToken::Type::T_KWRECOVER },
                { nullptr, (AstToken::Type)0}
            };
            for(i = 0; keywords[i].value != nullptr; i++)
            {
                klen = strlen(keywords[i].value);
                if(klen == len && mc_util_strnequal(ident, keywords[i].value, len))
                {
                    return keywords[i].type;
                }
            }
            return AstToken::Type::T_IDENT;
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
                line = Util::strDuplicate(linestart);
            }
            else
            {
                linelen = newlineptr - linestart;
                line = Util::strDuplicate(linestart, linelen);
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

class AstIdentifier
{
    public:
        char* m_identvalue = nullptr;
        SourceLocation m_exprpos = SourceLocation::Invalid();

    public:
        AstIdentifier()
        {
        }

        AstIdentifier(AstToken tok)
        {
            m_identvalue = tok.dupLiteralString();
            m_exprpos = tok.m_tokpos;
        }

        static AstIdentifier* copy(AstIdentifier* ident)
        {
            AstIdentifier* res = Memory::make<AstIdentifier>();
            res->m_identvalue = Util::strDuplicate(ident->m_identvalue);
            res->m_exprpos = ident->m_exprpos;
            return res;
        }

        static void destroy(AstIdentifier* ident)
        {
            if(ident != nullptr)
            {
                mc_memory_free(ident->m_identvalue);
                ident->m_identvalue = nullptr;
                ident->m_exprpos = SourceLocation::Invalid();
                mc_memory_free(ident);
            }
        }
};



class AstExpression
{
    public:
        enum ExprType
        {
            EXPR_NONE,
            EXPR_IDENT,
            EXPR_LITERALNUMBER,
            EXPR_LITERALBOOL,
            EXPR_LITERALSTRING,
            EXPR_LITERALNULL,
            EXPR_LITERALARRAY,
            EXPR_LITERALMAP,
            EXPR_PREFIX,
            EXPR_INFIX,
            EXPR_LITERALFUNCTION,
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
            MATHOP_LESSTHAN,
            MATHOP_LESSEQUAL,
            MATHOP_GREATERTHAN,
            MATHOP_GREATEREQAL,
            MATHOP_EQUAL,
            MATHOP_NOTEQUAL,
            MATHOP_MODULUS,
            MATHOP_LOGICALAND,
            MATHOP_LOGICALOR,
            MATHOP_BINAND,
            MATHOP_BINOR,
            MATHOP_BINXOR,
            MATHOP_LEFTSHIFT,
            MATHOP_RIGHTSHIFT
        };

    public:
        static AstExpression* copyExpression(AstExpression* expr)
        {
            if(!expr)
            {
                return nullptr;
            }
            return expr->copy();
        }


        static void destroyExpression(AstExpression* expr)
        {
            if(expr != nullptr)
            {
                expr->destroy();
                Memory::destroy(expr);
                expr = nullptr;
            }
        }

    public:
        ExprType m_exprtype;
        SourceLocation m_exprpos;

    public:
        virtual AstExpression* copy()
        {
            return nullptr;
        }

        virtual void destroy()
        {
        }

        template<typename TargetTyp, typename... ArgsT>
        static TargetTyp* make(ArgsT&&... args)
        {
            static_assert((std::is_base_of<AstExpression, TargetTyp>::value));
            auto res = Memory::make<TargetTyp>(args...);
            res->m_exprpos = SourceLocation::Invalid();
            return res;
        }
};

class AstCodeBlock
{
    public:
        GenericList<AstExpression*> m_blockstatements;

    public:
        AstCodeBlock(const GenericList<AstExpression*>& stmts)
        {
            m_blockstatements = stmts;
        }

        static void destroy(AstCodeBlock* block)
        {
            if(block != nullptr)
            {
                block->m_blockstatements.deInit(AstExpression::destroyExpression);
                mc_memory_free(block);
            }
        }

        static AstCodeBlock* copy(AstCodeBlock* block)
        {
            if(!block)
            {
                return nullptr;
            }
            auto copyfn = AstExpression::copyExpression;
            auto delfn = AstExpression::destroyExpression;
            auto statementscopy = block->m_blockstatements.copyToStack(copyfn, delfn);
            auto res = Memory::make<AstCodeBlock>(statementscopy);
            return res;
        }
};

class ExprLiteralNull: public AstExpression
{
    public:
        ExprLiteralNull()
        {
            m_exprtype = AstExpression::EXPR_LITERALNULL;
        }

        AstExpression* copy()
        {
            return AstExpression::make<ExprLiteralNull>();
        }
};

class ExprLiteralNumber: public AstExpression
{
    public:
        NumFloat m_exprnumber;

    public:
        ExprLiteralNumber(NumFloat val)
        {
            m_exprtype = AstExpression::EXPR_LITERALNUMBER;
            m_exprnumber = val;
        }

        AstExpression* copy()
        {
            return AstExpression::make<ExprLiteralNumber>(m_exprnumber);
        }
};

class ExprLiteralBool: public AstExpression
{
    public:
        bool m_exprbool;

    public:
        ExprLiteralBool(bool val)
        {
            m_exprtype = AstExpression::EXPR_LITERALBOOL;
            m_exprbool = val;
        }

        AstExpression* copy()
        {
            return AstExpression::make<ExprLiteralBool>(m_exprbool);
        }
};

class ExprReturnStmt: public AstExpression
{
    public:
        AstExpression* m_exprretvalue;

    public:
        ExprReturnStmt(AstExpression* value)
        {
            m_exprtype = AstExpression::EXPR_STMTRETURN;
            m_exprretvalue = value;
        }

        AstExpression* copy()
        {
            AstExpression* valuecopy;
            valuecopy = AstExpression::copyExpression(m_exprretvalue);
            return AstExpression::make<ExprReturnStmt>(valuecopy);
        }

        void destroy()
        {
            AstExpression::destroyExpression(m_exprretvalue);
        }
};

class ExprExpression: public AstExpression
{
    public:
        AstExpression* m_exprexprvalue;

    public:
        ExprExpression(AstExpression* value)
        {
            m_exprtype = AstExpression::EXPR_STMTEXPRESSION;
            m_exprexprvalue = value;
        }

        AstExpression* copy()
        {
            AstExpression* valuecopy;
            valuecopy = AstExpression::copyExpression(m_exprexprvalue);
            return AstExpression::make<ExprExpression>(valuecopy);
        }

        void destroy()
        {
            AstExpression::destroyExpression(m_exprexprvalue);
        }
};

class ExprBreakStmt: public AstExpression
{
    public:
        ExprBreakStmt()
        {
            m_exprtype = AstExpression::EXPR_STMTBREAK;
        }

        AstExpression* copy()
        {
            return AstExpression::make<ExprBreakStmt>();
        }

};

class ExprContinueStmt: public AstExpression
{
    public:
        ExprContinueStmt()
        {
            m_exprtype = AstExpression::EXPR_STMTCONTINUE;
        }

        AstExpression* copy()
        {
            return AstExpression::make<ExprContinueStmt>();
        }

};

class ExprIdent: public AstExpression
{
    public:
        AstIdentifier* m_expridvalue;

    public:
        ExprIdent(AstIdentifier* ident)
        {
            m_exprtype = AstExpression::EXPR_IDENT;
            m_expridvalue = ident;
        }

        AstExpression* copy()
        {
            AstIdentifier* ident;
            ident = AstIdentifier::copy(m_expridvalue);
            return AstExpression::make<ExprIdent>(ident);
        }

        void destroy()
        {
            Memory::destroy(m_expridvalue);
        }

};

class ExprBlock: public AstExpression
{
    public:
        AstCodeBlock* m_exprblockvalue;

    public:
        ExprBlock(AstCodeBlock* block)
        {
            m_exprtype = AstExpression::EXPR_STMTBLOCK;
            m_exprblockvalue = block;
        }

        AstExpression* copy()
        {
            AstCodeBlock* blockcopy;
            blockcopy = AstCodeBlock::copy(m_exprblockvalue);
            return AstExpression::make<ExprBlock>(blockcopy);
        }

        void destroy()
        {
            Memory::destroy(m_exprblockvalue);
        }
};

class ExprIfCase
{
    public:
        AstExpression* m_exprifcond;
        AstCodeBlock* m_exprifthen;

    public:
        ExprIfCase(AstExpression* test, AstCodeBlock* consq)
        {
            m_exprifcond = test;
            m_exprifthen = consq;
        }

        static ExprIfCase* copy(ExprIfCase* ifcase)
        {
            AstExpression* testcopy;
            AstCodeBlock* consequencecopy;
            ExprIfCase* ifcasecopy;
            testcopy = nullptr;
            consequencecopy = nullptr;
            ifcasecopy = nullptr;
            if(!ifcase)
            {
                return nullptr;
            }
            testcopy = AstExpression::copyExpression(ifcase->m_exprifcond);
            if(!testcopy)
            {
                goto err;
            }
            consequencecopy = AstCodeBlock::copy(ifcase->m_exprifthen);
            if(!testcopy || !consequencecopy)
            {
                goto err;
            }
            ifcasecopy = Memory::make<ExprIfCase>(testcopy, consequencecopy);
            return ifcasecopy;
        err:
            AstExpression::destroyExpression(testcopy);
            Memory::destroy(consequencecopy);
            ExprIfCase::destroy(ifcasecopy);
            return nullptr;
        }

        static void destroy(ExprIfCase* cond)
        {
            if(cond != nullptr)
            {
                AstExpression::destroyExpression(cond->m_exprifcond);
                Memory::destroy(cond->m_exprifthen);
                mc_memory_free(cond);
            }
        }
};

class ExprIfStmt: public AstExpression
{
    public:
        GenericList<ExprIfCase*> m_exprifcases;
        AstCodeBlock* m_exprifelse;

    public:
        ExprIfStmt(const GenericList<ExprIfCase*>& cases, AstCodeBlock* exprifelse)
        {
            m_exprtype = AstExpression::EXPR_STMTIF;
            m_exprifcases = cases;
            m_exprifelse = exprifelse;
        }

        AstExpression* copy()
        {
            AstCodeBlock* alternativecopy;
            auto casescopy = m_exprifcases.copyToStack(ExprIfCase::copy, ExprIfCase::destroy);
            alternativecopy = AstCodeBlock::copy(m_exprifelse);
            return AstExpression::make<ExprIfStmt>(casescopy, alternativecopy);
        }

        void destroy()
        {
            m_exprifcases.deInit(ExprIfCase::destroy);
            Memory::destroy(m_exprifelse);
        }

};

class ExprLiteralMap: public AstExpression
{
    public:
        GenericList<AstExpression*> m_exprlitmapkeys;
        GenericList<AstExpression*> m_exprlitmapvalues;

    public:
        ExprLiteralMap(const GenericList<AstExpression*>& keys, const GenericList<AstExpression*>& values)
        {
            m_exprtype = AstExpression::EXPR_LITERALMAP;
            m_exprlitmapkeys = keys;
            m_exprlitmapvalues = values;
        }

        AstExpression* copy()
        {
            auto keyscopy = m_exprlitmapkeys.copyToStack(AstExpression::copyExpression, AstExpression::destroyExpression);
            auto valuescopy = m_exprlitmapvalues.copyToStack(AstExpression::copyExpression, AstExpression::destroyExpression);
            return AstExpression::make<ExprLiteralMap>(keyscopy, valuescopy);
        }

        void destroy()
        {
            m_exprlitmapkeys.deInit(AstExpression::destroyExpression);
            m_exprlitmapvalues.deInit(AstExpression::destroyExpression);
        }

};

class ExprLiteralArray: public AstExpression
{
    public:
        GenericList<AstExpression*> m_exprlitarritems;

    public:
        ExprLiteralArray(const GenericList<AstExpression*>& values)
        {
            m_exprtype = AstExpression::EXPR_LITERALARRAY;
            m_exprlitarritems = values;
        }

        AstExpression* copy()
        {
            auto valuescopy = m_exprlitarritems.copyToStack(AstExpression::copyExpression, AstExpression::destroyExpression);
            return AstExpression::make<ExprLiteralArray>(valuescopy);
        }

        void destroy()
        {
            m_exprlitarritems.deInit(AstExpression::destroyExpression);
        }
};

class ExprLiteralString: public AstExpression
{
    public:
        size_t m_exprstrlength;
        char* m_exprstrdata;

    public:
        ExprLiteralString(char* value, size_t len)
        {
            m_exprtype = AstExpression::EXPR_LITERALSTRING;
            m_exprstrdata = value;
            m_exprstrlength = len;

        }

        AstExpression* copy()
        {
            char* stringcopy;
            stringcopy = Util::strDuplicate(m_exprstrdata, m_exprstrlength);
            return AstExpression::make<ExprLiteralString>(stringcopy, m_exprstrlength);
        }

        void destroy()
        {
            mc_memory_free(m_exprstrdata);
        }

};

class ExprPrefix: public AstExpression
{
    public:
        MathOpType m_exprprefixoper;
        AstExpression* m_exprprefixright;

    public:
        ExprPrefix(AstExpression::MathOpType op, AstExpression* right)
        {
            m_exprtype = AstExpression::EXPR_PREFIX;
            m_exprprefixoper = op;
            m_exprprefixright = right;
        }

        AstExpression* copy()
        {
            AstExpression* rightcopy;
            rightcopy = AstExpression::copyExpression(m_exprprefixright);
            return AstExpression::make<ExprPrefix>(m_exprprefixoper, rightcopy);
        }

        void destroy()
        {
            AstExpression::destroyExpression(m_exprprefixright);
        }
};

class ExprInfix: public AstExpression
{
    public:
        MathOpType m_exprinfixoper;
        AstExpression* m_exprinfixleft;
        AstExpression* m_exprinfixright;

    public:
        ExprInfix(AstExpression::MathOpType op, AstExpression* left, AstExpression* right)
        {
            m_exprtype = AstExpression::EXPR_INFIX;
            m_exprinfixoper = op;
            m_exprinfixleft = left;
            m_exprinfixright = right;

        }

        AstExpression* copy()
        {
            AstExpression* leftcopy;
            AstExpression* rightcopy;
            leftcopy = AstExpression::copyExpression(m_exprinfixleft);
            rightcopy = AstExpression::copyExpression(m_exprinfixright);
            return AstExpression::make<ExprInfix>(m_exprinfixoper, leftcopy, rightcopy);
        }

        void destroy()
        {
            AstExpression::destroyExpression(m_exprinfixleft);
            AstExpression::destroyExpression(m_exprinfixright);
        }
};

class ExprFuncParam
{
    public:
        AstIdentifier* m_exprfnparamident;

    public:
        ExprFuncParam(AstIdentifier* idv)
        {
            m_exprfnparamident = idv;
            MC_ASSERT(m_exprfnparamident->m_identvalue);
        }

        static ExprFuncParam* copy(ExprFuncParam* param)
        {
            ExprFuncParam* res;
            res = Memory::make<ExprFuncParam>(AstIdentifier::copy(param->m_exprfnparamident));
            if(!res->m_exprfnparamident->m_identvalue)
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
                Memory::destroy(param->m_exprfnparamident);
                mc_memory_free(param);
            }
        }

};

class ExprLiteralFunction: public AstExpression
{
    public:
        char* m_exprfnname;
        GenericList<ExprFuncParam*> m_exprfnparamlist;
        AstCodeBlock* m_exprfnbody;

    public:
        ExprLiteralFunction(char* name, const GenericList<ExprFuncParam*>& params, AstCodeBlock* body)
        {
            m_exprtype = AstExpression::EXPR_LITERALFUNCTION;
            m_exprfnname = name;
            m_exprfnparamlist = params;
            m_exprfnbody = body;
        }

        AstExpression* copy()
        {
            char* namecopy;
            AstCodeBlock* bodycopy;
            auto pacopy = m_exprfnparamlist.copyToStack(ExprFuncParam::copy, ExprFuncParam::destroy);
            bodycopy = AstCodeBlock::copy(m_exprfnbody);
            namecopy = Util::strDuplicate(m_exprfnname);
            return AstExpression::make<ExprLiteralFunction>(namecopy, pacopy, bodycopy);
        }

        void destroy()
        {
            mc_memory_free(m_exprfnname);
            m_exprfnparamlist.deInit(ExprFuncParam::destroy);
            Memory::destroy(m_exprfnbody);
        }
};

class ExprCall: public AstExpression
{
    public:
        AstExpression* m_exprcallfunction;
        GenericList<AstExpression*> m_exprcallargs;

    public:
        ExprCall(AstExpression* function, const GenericList<AstExpression*>& args)
        {
            m_exprtype = AstExpression::EXPR_CALL;
            m_exprcallfunction = function;
            m_exprcallargs = args;
        }

        AstExpression* copy()
        {
            AstExpression* fcopy;
            fcopy = AstExpression::copyExpression(m_exprcallfunction);
            auto argscopy = m_exprcallargs.copyToStack(AstExpression::copyExpression, AstExpression::destroyExpression);
            return AstExpression::make<ExprCall>(fcopy, argscopy);
        }

        void destroy()
        {
            m_exprcallargs.deInit(AstExpression::destroyExpression);
            AstExpression::destroyExpression(m_exprcallfunction);
        }
};

class ExprIndex: public AstExpression
{
    public:
        bool m_exprindexisdot;
        AstExpression* m_exprindexleft;
        AstExpression* m_exprindexindex;

    public:
        ExprIndex(AstExpression* left, AstExpression* index, bool isdot)
        {
            m_exprtype = AstExpression::EXPR_INDEX;
            m_exprindexisdot = isdot;
            m_exprindexleft = left;
            m_exprindexindex = index;
        }

        AstExpression* copy()
        {
            AstExpression* leftcopy;
            AstExpression* indexcopy;
            leftcopy = AstExpression::copyExpression(m_exprindexleft);
            indexcopy = AstExpression::copyExpression(m_exprindexindex);
            return AstExpression::make<ExprIndex>(leftcopy, indexcopy, false);
        }

        void destroy()
        {
            AstExpression::destroyExpression(m_exprindexleft);
            AstExpression::destroyExpression(m_exprindexindex);
        }
};

class ExprAssign: public AstExpression
{
    public:
        AstExpression* m_exprassigndest;
        AstExpression* m_exprassignsource;
        bool m_exprassignispostfix;

    public:
        ExprAssign(AstExpression* dest, AstExpression* source, bool ispostfix)
        {
            m_exprtype = AstExpression::EXPR_ASSIGN;
            m_exprassigndest = dest;
            m_exprassignsource = source;
            m_exprassignispostfix = ispostfix;
        }

        AstExpression* copy()
        {
            AstExpression* destcopy;
            AstExpression* sourcecopy;
            destcopy = AstExpression::copyExpression(m_exprassigndest);
            sourcecopy = AstExpression::copyExpression(m_exprassignsource);
            return AstExpression::make<ExprAssign>(destcopy, sourcecopy, m_exprassignispostfix);
        }

        void destroy()
        {
            AstExpression::destroyExpression(m_exprassigndest);
            AstExpression::destroyExpression(m_exprassignsource);
        }
};

class ExprLogical: public AstExpression
{
    public:
        MathOpType m_exprlogoper;
        AstExpression* m_exprlogleft;
        AstExpression* m_exprlogright;

    public:
        ExprLogical(AstExpression::MathOpType op, AstExpression* left, AstExpression* right)
        {
            m_exprtype = AstExpression::EXPR_LOGICAL;
            m_exprlogoper = op;
            m_exprlogleft = left;
            m_exprlogright = right;

        }

        AstExpression* copy()
        {
            AstExpression* leftcopy;
            AstExpression* rightcopy;
            leftcopy = AstExpression::copyExpression(m_exprlogleft);
            rightcopy = AstExpression::copyExpression(m_exprlogright);
            return AstExpression::make<ExprLogical>(m_exprlogoper, leftcopy, rightcopy);
        }

        void destroy()
        {
            AstExpression::destroyExpression(m_exprlogleft);
            AstExpression::destroyExpression(m_exprlogright);
        }
};

class ExprTernary: public AstExpression
{
    public:
        AstExpression* m_exprterncond;
        AstExpression* m_exprterniftrue;
        AstExpression* m_exprterniffalse;

    public:
        ExprTernary(AstExpression* test, AstExpression* ift, AstExpression* iffalse)
        {
            m_exprtype = AstExpression::EXPR_TERNARY;
            m_exprterncond = test;
            m_exprterniftrue = ift;
            m_exprterniffalse = iffalse;
        }

        AstExpression* copy()
        {
            AstExpression* testcopy;
            AstExpression* iftruecopy;
            AstExpression* iffalsecopy;
            testcopy = AstExpression::copyExpression(m_exprterncond);
            iftruecopy = AstExpression::copyExpression(m_exprterniftrue);
            iffalsecopy = AstExpression::copyExpression(m_exprterniffalse);
            return AstExpression::make<ExprTernary>(testcopy, iftruecopy, iffalsecopy);
        }

        void destroy()
        {
            AstExpression::destroyExpression(m_exprterncond);
            AstExpression::destroyExpression(m_exprterniftrue);
            AstExpression::destroyExpression(m_exprterniffalse);
        }
};

class ExprDefine: public AstExpression
{
    public:
        AstIdentifier* m_exprdefname;
        AstExpression* m_exprdefvalue;
        bool m_exprdefassignable;

    public:
        ExprDefine(AstIdentifier* name, AstExpression* value, bool assignable)
        {
            m_exprtype = AstExpression::EXPR_STMTDEFINE;
            m_exprdefname = name;
            m_exprdefvalue = value;
            m_exprdefassignable = assignable;
        }


        AstExpression* copy()
        {
            AstExpression* valuecopy;
            valuecopy = AstExpression::copyExpression(m_exprdefvalue);
            return AstExpression::make<ExprDefine>(AstIdentifier::copy(m_exprdefname), valuecopy, m_exprdefassignable);
        }

        void destroy()
        {
            Memory::destroy(m_exprdefname);
            AstExpression::destroyExpression(m_exprdefvalue);
        }
};

class ExprWhileStmt: public AstExpression
{
    public:
        AstExpression* m_exprwhilecond;
        AstCodeBlock* m_exprwhilebody;

    public:
        ExprWhileStmt(AstExpression* test, AstCodeBlock* body)
        {
            m_exprtype = AstExpression::EXPR_STMTLOOPWHILE;
            m_exprwhilecond = test;
            m_exprwhilebody = body;
        }

        AstExpression* copy()
        {
            AstExpression* testcopy;
            AstCodeBlock* bodycopy;
            testcopy = AstExpression::copyExpression(m_exprwhilecond);
            bodycopy = AstCodeBlock::copy(m_exprwhilebody);
            return AstExpression::make<ExprWhileStmt>(testcopy, bodycopy);
        }

        void destroy()
        {
            AstExpression::destroyExpression(m_exprwhilecond);
            Memory::destroy(m_exprwhilebody);
        }

};

class ExprForeachStmt: public AstExpression
{
    public:
        AstIdentifier* m_exprforeachiterator;
        AstExpression* m_exprforeachsource;
        AstCodeBlock* m_exprforeachbody;

    public:
        ExprForeachStmt(AstIdentifier* iterator, AstExpression* source, AstCodeBlock* body)
        {
            m_exprtype = AstExpression::EXPR_STMTLOOPFOREACH;
            m_exprforeachiterator = iterator;
            m_exprforeachsource = source;
            m_exprforeachbody = body;

        }

        AstExpression* copy()
        {
            AstExpression* sourcecopy;
            AstCodeBlock* bodycopy;
            sourcecopy = AstExpression::copyExpression(m_exprforeachsource);
            bodycopy = AstCodeBlock::copy(m_exprforeachbody);
            return AstExpression::make<ExprForeachStmt>(AstIdentifier::copy(m_exprforeachiterator), sourcecopy, bodycopy);
        }

        void destroy()
        {
            Memory::destroy(m_exprforeachiterator);
            AstExpression::destroyExpression(m_exprforeachsource);
            Memory::destroy(m_exprforeachbody);
        }
};

class ExprLoopStmt: public AstExpression
{
    public:
        AstExpression* m_exprforloopinit;
        AstExpression* m_exprforloopcond;
        AstExpression* m_exprforloopupdate;
        AstCodeBlock* m_exprforloopbody;

    public:
        ExprLoopStmt(AstExpression* init, AstExpression* test, AstExpression* update, AstCodeBlock* body)
        {
            m_exprtype = AstExpression::EXPR_STMTLOOPFORCLASSIC;
            m_exprforloopinit = init;
            m_exprforloopcond = test;
            m_exprforloopupdate = update;
            m_exprforloopbody = body;

        }

        AstExpression* copy()
        {
            AstExpression* initcopy;
            AstExpression* testcopy;
            AstExpression* updatecopy;
            AstCodeBlock* bodycopy;
            initcopy= AstExpression::copyExpression(m_exprforloopinit);
            testcopy = AstExpression::copyExpression(m_exprforloopcond);
            updatecopy = AstExpression::copyExpression(m_exprforloopupdate);
            bodycopy = AstCodeBlock::copy(m_exprforloopbody);
            return AstExpression::make<ExprLoopStmt>(initcopy, testcopy, updatecopy, bodycopy);
        }

        void destroy()
        {
            AstExpression::destroyExpression(m_exprforloopinit);
            AstExpression::destroyExpression(m_exprforloopcond);
            AstExpression::destroyExpression(m_exprforloopupdate);
            Memory::destroy(m_exprforloopbody);
        }

};

class ExprImportStmt: public AstExpression
{
    public:
        char* m_exprimportpath;

    public:
        ExprImportStmt(char* path)
        {
            m_exprtype = AstExpression::EXPR_STMTIMPORT;
            m_exprimportpath = path;
        }

        AstExpression* copy()
        {
            char* pathcopy;
            pathcopy = Util::strDuplicate(m_exprimportpath);
            return AstExpression::make<ExprImportStmt>(pathcopy);
        }

        void destroy()
        {
            mc_memory_free(m_exprimportpath);
        }
};

class ExprRecover: public AstExpression
{
    public:
        AstIdentifier* m_exprrecovererrident;
        AstCodeBlock* m_exprrecoverbody;

    public:
        ExprRecover(AstIdentifier* eid, AstCodeBlock* body)
        {
            m_exprtype = AstExpression::EXPR_STMTRECOVER;
            m_exprrecovererrident = eid;
            m_exprrecoverbody = body;
        }

        AstExpression* copy()
        {
            AstCodeBlock* bodycopy;
            AstIdentifier* erroridentcopy;
            bodycopy = AstCodeBlock::copy(m_exprrecoverbody);
            erroridentcopy = AstIdentifier::copy(m_exprrecovererrident);
            return AstExpression::make<ExprRecover>(erroridentcopy, bodycopy);
        }

        void destroy()
        {
            Memory::destroy(m_exprrecoverbody);
            Memory::destroy(m_exprrecovererrident);
        }
};

class AstParser
{
    public:
        using AssocParseRightFN = bool(*)(AstParser*, AstExpression**);
        using AssocParseLeftFN = bool (*)(AstParser*, AstExpression**, AstExpression*);

        enum class Precedence
        {
            Lowest = 0,
            Assign,
            /* a = b */
            Ternary,
            /* a ? b : c */
            LogicalOr,
            /* || */
            LogicalAnd,
            /* && */
            BinaryOr,
            /* | */
            BinaryXor,
            /* ^ */
            BinaryAnd,
            /* & */
            Equals,
            /* == != */
            LessGreater,
            /* >, >=, <, <= */
            Shift,
            /* << >> */
            Sum,
            /* + - */
            Product,
            /* * / % */
            Prefix,
            /* -x !x ++x --x */
            IncrementDecrement,
            /* x++ x-- */
            Postfix,
            /* myFunction(x) x["foo"] x.foo */
            Highest
        };

    public:
        static AstExpression* makeAstItemInlineCall(AstExpression* expr, const char* fname)
        {
            GenericList<AstExpression*> args(0);
            auto fntoken = AstToken(AstToken::Type::T_IDENT, fname, strlen(fname));
            fntoken.m_tokpos = expr->m_exprpos;
            auto ident = Memory::make<AstIdentifier>(fntoken);
            ident->m_exprpos = fntoken.m_tokpos;
            auto functionidentexpr = AstExpression::make<ExprIdent>(ident);
            functionidentexpr->m_exprpos = expr->m_exprpos;
            ident = nullptr;
            args.push(expr);
            auto ce = AstExpression::make<ExprCall>(functionidentexpr, args);
            ce->m_exprpos = expr->m_exprpos;
            return ce;
        }

        static Precedence getPrecedence(AstToken::Type tk)
        {
            switch(tk)
            {
                case AstToken::Type::T_EQUAL:
                case AstToken::Type::T_NOTEQ:
                    return Precedence::Equals;
                case AstToken::Type::T_LESSTHAN:
                case AstToken::Type::T_LESSEQUAL:
                case AstToken::Type::T_GREATERTHAN:
                case AstToken::Type::T_GREATEREQUAL:
                    return Precedence::LessGreater;
                case AstToken::Type::T_PLUS:
                case AstToken::Type::T_UNARYMINUS:
                case AstToken::Type::T_UNARYBINNOT:
                    return Precedence::Sum;
                case AstToken::Type::T_SLASH:
                case AstToken::Type::T_ASTERISK:
                case AstToken::Type::T_MATHMODULO:
                    return Precedence::Product;
                case AstToken::Type::T_PARENOPEN:
                case AstToken::Type::T_BRACKETOPEN:
                    return Precedence::Postfix;
                case AstToken::Type::T_ASSIGN:
                case AstToken::Type::T_ASSIGNPLUS:
                case AstToken::Type::T_ASSIGNMINUS:
                case AstToken::Type::T_ASSIGNASTERISK:
                case AstToken::Type::T_ASSIGNSLASH:
                case AstToken::Type::T_ASSIGNPERCENT:
                case AstToken::Type::T_ASSIGNBINAND:
                case AstToken::Type::T_ASSIGNBINOR:
                case AstToken::Type::T_ASSIGNBINXOR:
                case AstToken::Type::T_ASSIGNLSHIFT:
                case AstToken::Type::T_ASSIGNRSHIFT:
                    return Precedence::Assign;
                case AstToken::Type::T_DOT:
                    return Precedence::Postfix;
                case AstToken::Type::T_LOGICALAND:
                    return Precedence::LogicalAnd;
                case AstToken::Type::T_LOGICALOR:
                    return Precedence::LogicalOr;
                case AstToken::Type::T_BINOR:
                    return Precedence::BinaryOr;
                case AstToken::Type::T_BINXOR:
                    return Precedence::BinaryXor;
                case AstToken::Type::T_BINAND:
                    return Precedence::BinaryAnd;
                case AstToken::Type::T_LSHIFT:
                case AstToken::Type::T_RSHIFT:
                    return Precedence::Shift;
                case AstToken::Type::T_QUESTION:
                    return Precedence::Ternary;
                case AstToken::Type::T_PLUSPLUS:
                case AstToken::Type::T_MINUSMINUS:
                    return Precedence::IncrementDecrement;
                default:
                    break;
            }
            return Precedence::Lowest;
        }

        static AstExpression::MathOpType tokenToMathOP(AstToken::Type tk)
        {
            switch(tk)
            {
                case AstToken::Type::T_ASSIGN:
                    return AstExpression::MATHOP_ASSIGN;
                case AstToken::Type::T_PLUS:
                    return AstExpression::MATHOP_PLUS;
                case AstToken::Type::T_UNARYMINUS:
                    return AstExpression::MATHOP_MINUS;
                case AstToken::Type::T_UNARYBINNOT:
                    return AstExpression::MATHOP_BINNOT;
                case AstToken::Type::T_BANG:
                    return AstExpression::MATHOP_BANG;
                case AstToken::Type::T_ASTERISK:
                    return AstExpression::MATHOP_ASTERISK;
                case AstToken::Type::T_SLASH:
                    return AstExpression::MATHOP_SLASH;
                case AstToken::Type::T_LESSTHAN:
                    return AstExpression::MATHOP_LESSTHAN;
                case AstToken::Type::T_LESSEQUAL:
                    return AstExpression::MATHOP_LESSEQUAL;
                case AstToken::Type::T_GREATERTHAN:
                    return AstExpression::MATHOP_GREATERTHAN;
                case AstToken::Type::T_GREATEREQUAL:
                    return AstExpression::MATHOP_GREATEREQAL;
                case AstToken::Type::T_EQUAL:
                    return AstExpression::MATHOP_EQUAL;
                case AstToken::Type::T_NOTEQ:
                    return AstExpression::MATHOP_NOTEQUAL;
                case AstToken::Type::T_MATHMODULO:
                    return AstExpression::MATHOP_MODULUS;
                case AstToken::Type::T_LOGICALAND:
                    return AstExpression::MATHOP_LOGICALAND;
                case AstToken::Type::T_LOGICALOR:
                    return AstExpression::MATHOP_LOGICALOR;
                case AstToken::Type::T_ASSIGNPLUS:
                    return AstExpression::MATHOP_PLUS;
                case AstToken::Type::T_ASSIGNMINUS:
                    return AstExpression::MATHOP_MINUS;
                case AstToken::Type::T_ASSIGNASTERISK:
                    return AstExpression::MATHOP_ASTERISK;
                case AstToken::Type::T_ASSIGNSLASH:
                    return AstExpression::MATHOP_SLASH;
                case AstToken::Type::T_ASSIGNPERCENT:
                    return AstExpression::MATHOP_MODULUS;
                case AstToken::Type::T_ASSIGNBINAND:
                    return AstExpression::MATHOP_BINAND;
                case AstToken::Type::T_ASSIGNBINOR:
                    return AstExpression::MATHOP_BINOR;
                case AstToken::Type::T_ASSIGNBINXOR:
                    return AstExpression::MATHOP_BINXOR;
                case AstToken::Type::T_ASSIGNLSHIFT:
                    return AstExpression::MATHOP_LEFTSHIFT;
                case AstToken::Type::T_ASSIGNRSHIFT:
                    return AstExpression::MATHOP_RIGHTSHIFT;
                case AstToken::Type::T_BINAND:
                    return AstExpression::MATHOP_BINAND;
                case AstToken::Type::T_BINOR:
                    return AstExpression::MATHOP_BINOR;
                case AstToken::Type::T_BINXOR:
                    return AstExpression::MATHOP_BINXOR;
                case AstToken::Type::T_LSHIFT:
                    return AstExpression::MATHOP_LEFTSHIFT;
                case AstToken::Type::T_RSHIFT:
                    return AstExpression::MATHOP_RIGHTSHIFT;
                case AstToken::Type::T_PLUSPLUS:
                    return AstExpression::MATHOP_PLUS;
                case AstToken::Type::T_MINUSMINUS:
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
        static AssocParseRightFN getRightAssocParseFunc(AstToken::Type t)
        {
            switch(t)
            {
                case AstToken::Type::T_IDENT: return callback_parseident;
                case AstToken::Type::T_NUMBER: return callback_parseliteralnumber;
                case AstToken::Type::T_KWTRUE: return callback_parseliteralbool;
                case AstToken::Type::T_KWFALSE: return callback_parseliteralbool;
                case AstToken::Type::T_STRING: return callback_parseliteralstring;
                case AstToken::Type::T_TEMPLATESTRING: return callback_parseliteraltemplatestring;
                case AstToken::Type::T_KWNULL: return callback_parseliteralnull;
                case AstToken::Type::T_BANG: return callback_parseprefixexpr;
                case AstToken::Type::T_UNARYMINUS: return callback_parseprefixexpr;
                case AstToken::Type::T_UNARYBINNOT: return callback_parseprefixexpr;
                case AstToken::Type::T_PARENOPEN: return callback_parsegroupedexpr;
                case AstToken::Type::T_KWFUNCTION: return callback_parseliteralfunction;
                case AstToken::Type::T_BRACKETOPEN: return callback_parseliteralarray;
                case AstToken::Type::T_BRACEOPEN: return callback_parseliteralmap;
                case AstToken::Type::T_PLUSPLUS: return callback_parseincdecprefixexpr;
                case AstToken::Type::T_MINUSMINUS: return callback_parseincdecprefixexpr;
                case AstToken::Type::T_KWRECOVER: return callback_parserecoverstmt;
                default:
                    break;
            }
            return nullptr;
        }

        static AssocParseLeftFN getLeftAssocParseFunc(AstToken::Type t)
        {
            switch(t)
            {
                case AstToken::Type::T_PLUS: return callback_parseinfixexpr;
                case AstToken::Type::T_UNARYMINUS: return callback_parseinfixexpr;
                case AstToken::Type::T_SLASH: return callback_parseinfixexpr;
                case AstToken::Type::T_ASTERISK: return callback_parseinfixexpr;
                case AstToken::Type::T_MATHMODULO: return callback_parseinfixexpr;
                case AstToken::Type::T_EQUAL: return callback_parseinfixexpr;
                case AstToken::Type::T_NOTEQ: return callback_parseinfixexpr;
                case AstToken::Type::T_LESSTHAN: return callback_parseinfixexpr;
                case AstToken::Type::T_LESSEQUAL: return callback_parseinfixexpr;
                case AstToken::Type::T_GREATERTHAN: return callback_parseinfixexpr;
                case AstToken::Type::T_GREATEREQUAL: return callback_parseinfixexpr;
                case AstToken::Type::T_PARENOPEN: return callback_parsecallexpr;
                case AstToken::Type::T_BRACKETOPEN: return callback_parseindexexpr;
                case AstToken::Type::T_ASSIGN: return callback_parseassignexpr;
                case AstToken::Type::T_ASSIGNPLUS: return callback_parseassignexpr;
                case AstToken::Type::T_ASSIGNMINUS: return callback_parseassignexpr;
                case AstToken::Type::T_ASSIGNSLASH: return callback_parseassignexpr;
                case AstToken::Type::T_ASSIGNASTERISK: return callback_parseassignexpr;
                case AstToken::Type::T_ASSIGNPERCENT: return callback_parseassignexpr;
                case AstToken::Type::T_ASSIGNBINAND: return callback_parseassignexpr;
                case AstToken::Type::T_ASSIGNBINOR: return callback_parseassignexpr;
                case AstToken::Type::T_ASSIGNBINXOR: return callback_parseassignexpr;
                case AstToken::Type::T_ASSIGNLSHIFT: return callback_parseassignexpr;
                case AstToken::Type::T_ASSIGNRSHIFT: return callback_parseassignexpr;
                case AstToken::Type::T_DOT: return callback_parsedotexpression;
                case AstToken::Type::T_LOGICALAND: return callback_parselogicalexpr;
                case AstToken::Type::T_LOGICALOR: return callback_parselogicalexpr;
                case AstToken::Type::T_BINAND: return callback_parseinfixexpr;
                case AstToken::Type::T_BINOR: return callback_parseinfixexpr;
                case AstToken::Type::T_BINXOR: return callback_parseinfixexpr;
                case AstToken::Type::T_LSHIFT: return callback_parseinfixexpr;
                case AstToken::Type::T_RSHIFT: return callback_parseinfixexpr;
                case AstToken::Type::T_QUESTION: return callback_parseternaryexpr;
                case AstToken::Type::T_PLUSPLUS: return callback_parseincdecpostfixexpr;
                case AstToken::Type::T_MINUSMINUS: return callback_parseincdecpostfixexpr;
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
            assignable = m_lexer.currentTokenIs(AstToken::Type::T_KWVAR);
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::Type::T_IDENT))
            {
                return false;
            }
            auto nameident = Memory::make<AstIdentifier>(m_lexer.m_currtoken);
            m_lexer.nextToken();
            if(!m_lexer.currentTokenIs(AstToken::Type::T_ASSIGN))
            {
                value = AstExpression::make<ExprLiteralNull>();
                goto finish;
            }
            m_lexer.nextToken();
            if(!parseExpression(&value, Precedence::Lowest))
            {
                goto err;
            }
            if(value->m_exprtype == AstExpression::EXPR_LITERALFUNCTION)
            {
                auto ofn = static_cast<ExprLiteralFunction*>(value);
                ofn->m_exprfnname = Util::strDuplicate(nameident->m_identvalue);
                if(ofn->m_exprfnname == nullptr)
                {
                    goto err;
                }
            }
            finish:
            *res = AstExpression::make<ExprDefine>(nameident, value, assignable);
            return true;
        err:
            AstExpression::destroyExpression(value);
            Memory::destroy(nameident);
            return false;
        }

        bool parseIfStmt(AstExpression** res)
        {
            bool ok;
            ExprIfCase* cond;
            ExprIfCase* elif;
            AstCodeBlock* alternative;
            AstCodeBlock* emptyblocktop = nullptr;
            GenericList<ExprIfCase*> cases;
            (void)ok;
            alternative = nullptr;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::Type::T_PARENOPEN))
            {
                goto err;
            }
            m_lexer.nextToken();
            cond = Memory::make<ExprIfCase>(nullptr, emptyblocktop);
            ok = cases.push(cond);
            if(!parseExpression(&cond->m_exprifcond, Precedence::Lowest))
            {
                goto err;
            }
            if(!m_lexer.expectCurrent(AstToken::Type::T_PARENCLOSE))
            {
                goto err;
            }
            m_lexer.nextToken();
            if(!parseCodeBlock(&cond->m_exprifthen))
            {
                goto err;
            }
            while(m_lexer.currentTokenIs(AstToken::Type::T_KWELSE))
            {
                AstCodeBlock* emptyblockinner = nullptr;
                m_lexer.nextToken();
                if(m_lexer.currentTokenIs(AstToken::Type::T_KWIF))
                {
                    m_lexer.nextToken();
                    if(!m_lexer.expectCurrent(AstToken::Type::T_PARENOPEN))
                    {
                        goto err;
                    }
                    m_lexer.nextToken();
                    elif = Memory::make<ExprIfCase>(nullptr, emptyblockinner);
                    ok = cases.push(elif);
                    if(!parseExpression(&elif->m_exprifcond, Precedence::Lowest))
                    {
                        goto err;
                    }
                    if(!m_lexer.expectCurrent(AstToken::Type::T_PARENCLOSE))
                    {
                        goto err;
                    }
                    m_lexer.nextToken();
                    if(!parseCodeBlock(&elif->m_exprifthen))
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
                }
            }
            *res = AstExpression::make<ExprIfStmt>(cases, alternative);
            return true;
        err:
            cases.deInit(ExprIfCase::destroy);
            Memory::destroy(alternative);
            return false;
        }

        bool parseReturnStmt(AstExpression** res)
        {
            AstExpression* expr;
            expr = nullptr;
            m_lexer.nextToken();
            if(!m_lexer.currentTokenIs(AstToken::Type::T_SEMICOLON) && !m_lexer.currentTokenIs(AstToken::Type::T_BRACECLOSE) && !m_lexer.currentTokenIs(AstToken::Type::T_EOF))
            {
                if(!parseExpression(&expr, Precedence::Lowest))
                {
                    return false;
                }
            }
            *res = AstExpression::make<ExprReturnStmt>(expr);
            return true;
        }

        bool parseExprStmt(AstExpression** res)
        {
            AstExpression* expr;
            if(!parseExpression(&expr, Precedence::Lowest))
            {
                return false;
            }
            if((expr != nullptr) && (!m_config->replmode || m_parsedepth > 0))
            {
            }
            *res = AstExpression::make<ExprExpression>(expr);
            return true;
        }

        bool parseLoopWhileStmt(AstExpression** res)
        {
            AstExpression* test;
            AstCodeBlock* body;
            test = nullptr;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::Type::T_PARENOPEN))
            {
                goto err;
            }
            m_lexer.nextToken();
            if(!parseExpression(&test, Precedence::Lowest))
            {
                goto err;
            }
            if(!m_lexer.expectCurrent(AstToken::Type::T_PARENCLOSE))
            {
                goto err;
            }
            m_lexer.nextToken();
            if(!parseCodeBlock(&body))
            {
                goto err;
            }
            *res = AstExpression::make<ExprWhileStmt>(test, body);
            return true;
        err:
            Memory::destroy(body);
            AstExpression::destroyExpression(test);
            return false;
        }

        bool parseBreakStmt(AstExpression** res)
        {
            m_lexer.nextToken();
            auto expr = AstExpression::make<ExprBreakStmt>();
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
            auto expr = AstExpression::make<ExprContinueStmt>();
            if(expr == nullptr)
            {
                return false;
            }
            *res = expr;
            return true;
        }

        bool parseBlockStmt(AstExpression** res)
        {
            AstCodeBlock* block;
            if(!parseCodeBlock(&block))
            {
                return false;
            }
            *res = AstExpression::make<ExprBlock>(block);
            return true;
        }

        bool parseImportStmt(AstExpression** res)
        {
            char* processedname;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::Type::T_STRING))
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
            *res = AstExpression::make<ExprImportStmt>(processedname);
            return true;
        }

        bool parseRecoverStmt(AstExpression** res)
        {
            AstCodeBlock* body;
            body = nullptr;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::Type::T_PARENOPEN))
            {
                return false;
            }
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::Type::T_IDENT))
            {
                return false;
            }
            auto eid = Memory::make<AstIdentifier>(m_lexer.m_currtoken);
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::Type::T_PARENCLOSE))
            {
                goto err;
            }
            m_lexer.nextToken();
            if(!parseCodeBlock(&body))
            {
                goto err;
            }
            *res = AstExpression::make<ExprRecover>(eid, body);
            return true;
        err:
            Memory::destroy(body);
            Memory::destroy(eid);
            return false;
        }

        bool parseLoopForBaseStmt(AstExpression** res)
        {
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::Type::T_PARENOPEN))
            {
                return false;
            }
            m_lexer.nextToken();
            if(m_lexer.currentTokenIs(AstToken::Type::T_IDENT) && m_lexer.peekTokenIs(AstToken::Type::T_KWIN))
            {
                return parseLoopForeachStmt(res);
            }
            return parseLoopForClassicStmt(res);
        }

        bool parseLoopForeachStmt(AstExpression** res)
        {
            AstExpression* source;
            AstCodeBlock* body;
            body = nullptr;
            source = nullptr;
            auto iteratorident = Memory::make<AstIdentifier>(m_lexer.m_currtoken);
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::Type::T_KWIN))
            {
                goto err;
            }
            m_lexer.nextToken();
            if(!parseExpression(&source, Precedence::Lowest))
            {
                goto err;
            }
            if(!m_lexer.expectCurrent(AstToken::Type::T_PARENCLOSE))
            {
                goto err;
            }
            m_lexer.nextToken();
            if(!parseCodeBlock(&body))
            {
                goto err;
            }
            *res = AstExpression::make<ExprForeachStmt>(iteratorident, source, body);
            return true;
        err:
            Memory::destroy(body);
            Memory::destroy(iteratorident);
            AstExpression::destroyExpression(source);
            return false;
        }

        bool parseLoopForClassicStmt(AstExpression** res)
        {
            AstExpression* init;
            AstExpression* test;
            AstExpression* update;
            AstCodeBlock* body;
            init = nullptr;
            test = nullptr;
            update = nullptr;
            if(!m_lexer.currentTokenIs(AstToken::Type::T_SEMICOLON))
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
                if(!m_lexer.expectCurrent(AstToken::Type::T_SEMICOLON))
                {
                    goto err;
                }
            }
            m_lexer.nextToken();
            if(!m_lexer.currentTokenIs(AstToken::Type::T_SEMICOLON))
            {
                if(!parseExpression(&test, Precedence::Lowest))
                {
                    goto err;
                }
                if(!m_lexer.expectCurrent(AstToken::Type::T_SEMICOLON))
                {
                    goto err;
                }
            }
            m_lexer.nextToken();
            if(!m_lexer.currentTokenIs(AstToken::Type::T_PARENCLOSE))
            {
                if(!parseExpression(&update, Precedence::Lowest))
                {
                    goto err;
                }
                if(!m_lexer.expectCurrent(AstToken::Type::T_PARENCLOSE))
                {
                    goto err;
                }
            }
            m_lexer.nextToken();
            if(!parseCodeBlock(&body))
            {
                goto err;
            }
            *res = AstExpression::make<ExprLoopStmt>(init, test, update, body);
            return true;
        err:
            AstExpression::destroyExpression(init);
            AstExpression::destroyExpression(test);
            AstExpression::destroyExpression(update);
            Memory::destroy(body);
            return false;
        }

        bool parseCodeBlock(AstCodeBlock** res)
        {
            bool ok;
            AstExpression* expr;
            GenericList<AstExpression*> statements;
            (void)ok;
            expr = nullptr;
            if(!m_lexer.expectCurrent(AstToken::Type::T_BRACEOPEN))
            {
                return false;
            }
            m_lexer.nextToken();
            m_parsedepth++;
            while(!m_lexer.currentTokenIs(AstToken::Type::T_BRACECLOSE))
            {
                if(m_lexer.currentTokenIs(AstToken::Type::T_EOF))
                {
                    m_prserrlist->pushFormat(Error::ERRTYP_PARSING, m_lexer.m_currtoken.m_tokpos, "unexpected EOF");
                    goto err;
                }
                if(m_lexer.currentTokenIs(AstToken::Type::T_SEMICOLON))
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
            *res = Memory::make<AstCodeBlock>(statements);
            return true;
        err:
            m_parsedepth--;
            statements.deInit(AstExpression::destroyExpression);
            return false;
        }

        bool parseExpression(AstExpression** res, Precedence prec)
        {
            char* literal;
            SourceLocation pos;
            AssocParseLeftFN parseleftassoc;
            AssocParseRightFN parserightassoc;
            AstExpression* newleftexpr;
            AstExpression* leftexpr;
            pos = m_lexer.m_currtoken.m_tokpos;
            if(m_lexer.m_currtoken.type() == AstToken::Type::T_INVALID)
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
            while(!m_lexer.currentTokenIs(AstToken::Type::T_SEMICOLON) && prec < getPrecedence(m_lexer.m_currtoken.type()))
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
            expr = nullptr;
            m_lexer.nextToken();
            if(!parseExpression(&expr, Precedence::Lowest) || !m_lexer.expectCurrent(AstToken::Type::T_PARENCLOSE))
            {
                AstExpression::destroyExpression(expr);
                return false;
            }
            m_lexer.nextToken();
            *res = expr;
            return true;
        }

        bool parseFuncParams(GenericList<ExprFuncParam*>* outparams)
        {
            bool ok;
            (void)ok;
            if(!m_lexer.expectCurrent(AstToken::Type::T_PARENOPEN))
            {
                return false;
            }
            m_lexer.nextToken();
            if(m_lexer.currentTokenIs(AstToken::Type::T_PARENCLOSE))
            {
                m_lexer.nextToken();
                return true;
            }
            if(!m_lexer.expectCurrent(AstToken::Type::T_IDENT))
            {
                return false;
            }
            auto ident = Memory::make<AstIdentifier>(m_lexer.m_currtoken);
            auto param = Memory::make<ExprFuncParam>(ident);
            ok = outparams->push(param);
            m_lexer.nextToken();
            while(m_lexer.currentTokenIs(AstToken::Type::T_COMMA))
            {
                m_lexer.nextToken();
                if(!m_lexer.expectCurrent(AstToken::Type::T_IDENT))
                {
                    return false;
                }
                ident = Memory::make<AstIdentifier>(m_lexer.m_currtoken);
                param = Memory::make<ExprFuncParam>(ident);
                ok = outparams->push(param);
                m_lexer.nextToken();
            }
            if(!m_lexer.expectCurrent(AstToken::Type::T_PARENCLOSE))
            {
                return false;
            }
            m_lexer.nextToken();
            return true;
        }

        bool parseFunctionStmt(AstExpression** res)
        {
            AstExpression* value;
            SourceLocation pos;
            ExprLiteralFunction* ofn;
            value = nullptr;
            pos = m_lexer.m_currtoken.m_tokpos;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::Type::T_IDENT))
            {
                return false;
            }
            auto nameident = Memory::make<AstIdentifier>(m_lexer.m_currtoken);
            m_lexer.nextToken();
            if(!callback_parseliteralfunction(this, &value))
            {
                goto err;
            }
            value->m_exprpos = pos;
            ofn = static_cast<ExprLiteralFunction*>(value);
            ofn->m_exprfnname = Util::strDuplicate(nameident->m_identvalue);
            if(ofn->m_exprfnname == nullptr)
            {
                goto err;
            }
            *res = AstExpression::make<ExprDefine>(nameident, value, false);
            return true;
        err:
            AstExpression::destroyExpression(value);
            Memory::destroy(nameident);
            return false;
        }

        bool parseTernaryExpr(AstExpression** res, AstExpression* left)
        {
            AstExpression* ift;
            AstExpression* iffalse;
            m_lexer.nextToken();
            if(!parseExpression(&ift, Precedence::Lowest))
            {
                return false;
            }
            if(!m_lexer.expectCurrent(AstToken::Type::T_COLON))
            {
                AstExpression::destroyExpression(ift);
                return false;
            }
            m_lexer.nextToken();
            if(!parseExpression(&iffalse, Precedence::Lowest))
            {
                AstExpression::destroyExpression(ift);
                return false;
            }
            *res = AstExpression::make<ExprTernary>(left, ift, iffalse);
            return true;
        }

        bool parseLogicalExpr(AstExpression** res, AstExpression* left)
        {
            AstExpression::MathOpType op;
            Precedence prec;
            AstExpression* right;
            op = tokenToMathOP(m_lexer.m_currtoken.m_toktype);
            prec = getPrecedence(m_lexer.m_currtoken.m_toktype);
            m_lexer.nextToken();
            if(!parseExpression(&right, prec))
            {
                return false;
            }
            *res = AstExpression::make<ExprLogical>(op, left, right);
            return true;
        }

        bool parseIndexExpr(AstExpression** res, AstExpression* left)
        {
            AstExpression* index;
            m_lexer.nextToken();
            if(!parseExpression(&index, Precedence::Lowest))
            {
                return false;
            }
            if(!m_lexer.expectCurrent(AstToken::Type::T_BRACKETCLOSE))
            {
                AstExpression::destroyExpression(index);
                return false;
            }
            m_lexer.nextToken();
            *res = AstExpression::make<ExprIndex>(left, index, false);
            return true;
        }

        bool parseAssignExpr(AstExpression** res, AstExpression* left)
        {
            SourceLocation pos;
            AstExpression::MathOpType op;
            AstToken::Type assigntype;
            AstExpression* source;
            AstExpression* leftcopy;
            AstExpression* newsource;
            source = nullptr;
            assigntype = m_lexer.m_currtoken.m_toktype;
            m_lexer.nextToken();
            if(!parseExpression(&source, Precedence::Lowest))
            {
                goto err;
            }
            switch(assigntype)
            {
                case AstToken::Type::T_ASSIGNPLUS:
                case AstToken::Type::T_ASSIGNMINUS:
                case AstToken::Type::T_ASSIGNSLASH:
                case AstToken::Type::T_ASSIGNASTERISK:
                case AstToken::Type::T_ASSIGNPERCENT:
                case AstToken::Type::T_ASSIGNBINAND:
                case AstToken::Type::T_ASSIGNBINOR:
                case AstToken::Type::T_ASSIGNBINXOR:
                case AstToken::Type::T_ASSIGNLSHIFT:
                case AstToken::Type::T_ASSIGNRSHIFT:
                    {
                        op = tokenToMathOP(assigntype);
                        leftcopy = AstExpression::copyExpression(left);
                        if(leftcopy == nullptr)
                        {
                            goto err;
                        }
                        pos = source->m_exprpos;
                        newsource = AstExpression::make<ExprInfix>(op, leftcopy, source);
                        newsource->m_exprpos = pos;
                        source = newsource;
                    }
                    break;
                case AstToken::Type::T_ASSIGN:
                    {
                    }
                    break;
                default:
                    {
                        MC_ASSERT(false);
                    }
                    break;
            }
            *res = AstExpression::make<ExprAssign>(left, source, false);
            return true;
        err:
            AstExpression::destroyExpression(source);
            return false;
        }

        bool parseIncDecPrefixExpr(AstExpression** res)
        {
            SourceLocation pos;
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
            if(!parseExpression(&dest, Precedence::Prefix))
            {
                goto err;
            }
            oneliteral = AstExpression::make<ExprLiteralNumber>(1);
            oneliteral->m_exprpos = pos;
            destcopy = AstExpression::copyExpression(dest);
            if(destcopy == nullptr)
            {
                AstExpression::destroyExpression(oneliteral);
                AstExpression::destroyExpression(dest);
                goto err;
            }
            operation = AstExpression::make<ExprInfix>(op, destcopy, oneliteral);
            operation->m_exprpos = pos;
            *res = AstExpression::make<ExprAssign>(dest, operation, false);
            return true;
        err:
            AstExpression::destroyExpression(source);
            return false;
        }

        bool parseIncDecPostfixExpr(AstExpression** res, AstExpression* left)
        {
            SourceLocation pos;
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
            oneliteral = AstExpression::make<ExprLiteralNumber>(1);
            oneliteral->m_exprpos = pos;
            operation = AstExpression::make<ExprInfix>(op, leftcopy, oneliteral);
            operation->m_exprpos = pos;
            *res = AstExpression::make<ExprAssign>(left, operation, true);
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
            if(!parseExpression(&right, Precedence::Prefix))
            {
                return false;
            }
            *res = AstExpression::make<ExprPrefix>(op, right);
            return true;
        }

        bool parseInfixExpr(AstExpression** res, AstExpression* left)
        {
            AstExpression::MathOpType op;
            Precedence prec;
            AstExpression* right;
            op = tokenToMathOP(m_lexer.m_currtoken.m_toktype);
            prec = getPrecedence(m_lexer.m_currtoken.m_toktype);
            m_lexer.nextToken();
            if(!parseExpression(&right, prec))
            {
                return false;
            }
            *res = AstExpression::make<ExprInfix>(op, left, right);
            return true;
        }

        bool parseLiteralFunction(AstExpression** res)
        {
            bool ok;
            AstCodeBlock* body;
            GenericList<ExprFuncParam*> params;
            (void)ok;
            m_parsedepth++;
            body = nullptr;
            if(m_lexer.currentTokenIs(AstToken::Type::T_KWFUNCTION))
            {
                m_lexer.nextToken();
            }
            ok = parseFuncParams(&params);
            if(!ok)
            {
                goto err;
            }
            if(!parseCodeBlock(&body))
            {
                goto err;
            }
            *res = AstExpression::make<ExprLiteralFunction>(nullptr, params, body);
            m_parsedepth -= 1;
            return true;
        err:
            Memory::destroy(body);
            params.deInit(ExprFuncParam::destroy);
            m_parsedepth -= 1;
            return false;
        }

        bool parseLiteralArray(AstExpression** res)
        {
            GenericList<AstExpression*> array;
            if(!parseExprList(&array, AstToken::Type::T_BRACKETOPEN, AstToken::Type::T_BRACKETCLOSE, true))
            {
                return false;
            }
            *res = AstExpression::make<ExprLiteralArray>(array);
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
            while(!m_lexer.currentTokenIs(AstToken::Type::T_BRACECLOSE))
            {
                key = nullptr;
                if(m_lexer.currentTokenIs(AstToken::Type::T_IDENT))
                {
                    str = m_lexer.m_currtoken.dupLiteralString();
                    len = strlen(str);
                    key = AstExpression::make<ExprLiteralString>(str, len);
                    key->m_exprpos = m_lexer.m_currtoken.m_tokpos;
                    m_lexer.nextToken();
                }
                else
                {
                    if(!parseExpression(&key, Precedence::Lowest))
                    {
                        goto err;
                    }
                    switch(key->m_exprtype)
                    {
                        case AstExpression::EXPR_LITERALSTRING:
                        case AstExpression::EXPR_LITERALNUMBER:
                        case AstExpression::EXPR_LITERALBOOL:
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
                if(!m_lexer.expectCurrent(AstToken::Type::T_COLON))
                {
                    goto err;
                }
                m_lexer.nextToken();
                if(!parseExpression(&value, Precedence::Lowest))
                {
                    goto err;
                }
                ok = values.push(value);
                if(m_lexer.currentTokenIs(AstToken::Type::T_BRACECLOSE))
                {
                    break;
                }
                if(!m_lexer.expectCurrent(AstToken::Type::T_COMMA))
                {
                    goto err;
                }
                m_lexer.nextToken();
            }
            m_lexer.nextToken();
            *res = AstExpression::make<ExprLiteralMap>(keys, values);
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
            SourceLocation pos;
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
            if(!m_lexer.expectCurrent(AstToken::Type::T_BRACEOPEN))
            {
                goto err;
            }
            m_lexer.nextToken();
            pos = m_lexer.m_currtoken.m_tokpos;
            len = strlen(processedliteral);
            leftstringexpr = AstExpression::make<ExprLiteralString>(processedliteral, len);
            leftstringexpr->m_exprpos = pos;
            processedliteral = nullptr;
            pos = m_lexer.m_currtoken.m_tokpos;
            if(!parseExpression(&templateexpr, Precedence::Lowest))
            {
                goto err;
            }
            tostrcallexpr = makeAstItemInlineCall(templateexpr, "tostring");
            tostrcallexpr->m_exprpos = pos;
            templateexpr = nullptr;
            leftaddexpr = AstExpression::make<ExprInfix>(AstExpression::MATHOP_PLUS, leftstringexpr, tostrcallexpr);
            leftaddexpr->m_exprpos = pos;
            leftstringexpr = nullptr;
            tostrcallexpr = nullptr;
            if(!m_lexer.expectCurrent(AstToken::Type::T_BRACECLOSE))
            {
                goto err;
            }
            m_lexer.previousToken();
            m_lexer.conttplstring();
            m_lexer.nextToken();
            m_lexer.nextToken();
            pos = m_lexer.m_currtoken.m_tokpos;
            if(!parseExpression(&rightexpr, Precedence::Highest))
            {
                goto err;
            }
            rightaddexpr = AstExpression::make<ExprInfix>(AstExpression::MATHOP_PLUS, leftaddexpr, rightexpr);
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
            len = strlen(processedliteral);
            *res = AstExpression::make<ExprLiteralString>(processedliteral, len);
            return true;
        }

        bool parseLiteralNull(AstExpression** res)
        {
            m_lexer.nextToken();
            *res = AstExpression::make<ExprLiteralNull>();
            return true;
        }

        bool parseLiteralBool(AstExpression** res)
        {
            *res = AstExpression::make<ExprLiteralBool>(m_lexer.m_currtoken.m_toktype == AstToken::Type::T_KWTRUE);
            m_lexer.nextToken();
            return true;
        }

        bool parseLiteralNumber(AstExpression** res)
        {
            NumFloat number;
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
            *res = AstExpression::make<ExprLiteralNumber>(number);
            return true;
        }

        bool parseDotExpr(AstExpression** res, AstExpression* left)
        {
            size_t len;
            char* str;
            AstExpression* index;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::Type::T_IDENT))
            {
                return false;
            }
            str = m_lexer.m_currtoken.dupLiteralString();
            len = strlen(str);
            index = AstExpression::make<ExprLiteralString>(str, len);
            index->m_exprpos = m_lexer.m_currtoken.m_tokpos;
            m_lexer.nextToken();
            *res = AstExpression::make<ExprIndex>(left, index, true);
            return true;
        }
        
        bool parseIdent(AstExpression** res)
        {
            auto ident = Memory::make<AstIdentifier>(m_lexer.m_currtoken);
            *res = AstExpression::make<ExprIdent>(ident);
            m_lexer.nextToken();
            return true;
        }

        bool parseCallExpr(AstExpression** res, AstExpression* left)
        {
            AstExpression* function;
            GenericList<AstExpression*> args;
            function = left;
            if(!parseExprList(&args, AstToken::Type::T_PARENOPEN, AstToken::Type::T_PARENCLOSE, false))
            {
                return false;
            }
            *res = AstExpression::make<ExprCall>(function, args);
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
            if(!parseExpression(&argexpr, Precedence::Lowest))
            {
                goto err;
            }
            ok = res->push(argexpr);
            while(m_lexer.currentTokenIs(AstToken::Type::T_COMMA))
            {
                m_lexer.nextToken();
                if(trailingcommaallowed && m_lexer.currentTokenIs(endtoken))
                {
                    break;
                }
                if(!parseExpression(&argexpr, Precedence::Lowest))
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
            SourceLocation pos;
            AstExpression* expr;
            pos = m_lexer.m_currtoken.m_tokpos;
            expr = nullptr;
            switch(m_lexer.m_currtoken.m_toktype)
            {
                case AstToken::Type::T_KWVAR:
                case AstToken::Type::T_KWCONST:
                    {
                        if(!parseVarLetStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::Type::T_KWIF:
                    {
                        if(!parseIfStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::Type::T_KWRETURN:
                    {
                        if(!parseReturnStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::Type::T_KWWHILE:
                    {
                        if(!parseLoopWhileStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::Type::T_KWBREAK:
                    {
                        if(!parseBreakStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::Type::T_KWFOR:
                    {
                        if(!parseLoopForBaseStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::Type::T_KWFUNCTION:
                    {
                        if(m_lexer.peekTokenIs(AstToken::Type::T_IDENT))
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
                case AstToken::Type::T_BRACEOPEN:
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
                case AstToken::Type::T_KWCONTINUE:
                    {
                        if(!parseContinueStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::Type::T_KWIMPORT:
                    {
                        if(!parseImportStmt(&expr))
                        {
                            return false;
                        }
                    }
                    break;
                case AstToken::Type::T_KWRECOVER:
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

        bool parseAll(GenericList<AstExpression*>* statements, const char* input, SourceFile* file)
        {
            bool ok;
            AstExpression* expr;
            (void)ok;
            m_parsedepth = 0;
            expr = nullptr;
            ok = AstLexer::init(&m_lexer, m_prserrlist, input, file);
            if(!ok)
            {
                return false;
            }
            m_lexer.nextToken();
            m_lexer.nextToken();
            while(!m_lexer.currentTokenIs(AstToken::Type::T_EOF))
            {
                if(m_lexer.currentTokenIs(AstToken::Type::T_SEMICOLON))
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
                case AstExpression::MATHOP_LESSTHAN:
                    return "<";
                case AstExpression::MATHOP_GREATERTHAN:
                    return ">";
                case AstExpression::MATHOP_EQUAL:
                    return "==";
                case AstExpression::MATHOP_NOTEQUAL:
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
                case AstExpression::MATHOP_LEFTSHIFT:
                    return "<<";
                case AstExpression::MATHOP_RIGHTSHIFT:
                    return ">>";
                case AstExpression::MATHOP_LESSEQUAL:
                    return "<=";
                case AstExpression::MATHOP_GREATEREQAL:
                    return ">=";
                case AstExpression::MATHOP_BINNOT:
                    return "~";
            }
            return "AstExpression::MATHOP_UNKNOWN";
        }

    public:
        IOHandle* m_pdest;
        bool m_pseudolisp;

    public:
        AstPrinter(IOHandle* pr)
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
            auto ex = static_cast<ExprLiteralFunction*>(astexpr);
            if(m_pseudolisp)
            {
                putfmt("(deffunction '(");
            }
            else
            {
                put("function(");
            }
            for(i = 0; i < ex->m_exprfnparamlist.count(); i++)
            {
                auto param = ex->m_exprfnparamlist.get(i);
                put(param->m_exprfnparamident->m_identvalue);
                if(i < (ex->m_exprfnparamlist.count() - 1))
                {
                    put(", ");
                }
            }
            put(") ");
            printCodeblock(ex->m_exprfnbody);
        }

        void printCall(AstExpression* astexpr)
        {
            size_t i;
            auto ex = static_cast<ExprCall*>(astexpr);
            printExpression(ex->m_exprcallfunction);
            put("(");
            for(i = 0; i < ex->m_exprcallargs.count(); i++)
            {
                auto arg = ex->m_exprcallargs.get(i);
                printExpression(arg);
                if(i < (ex->m_exprcallargs.count() - 1))
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
            auto ex = static_cast<ExprLiteralArray*>(astexpr);
            auto vl = &ex->m_exprlitarritems;
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
            auto ex = static_cast<ExprLiteralString*>(astexpr);
            sdata = ex->m_exprstrdata;
            slen = ex->m_exprstrlength;
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
            auto ex = static_cast<ExprLiteralMap*>(astexpr);
            put("{");
            for(i = 0; i < ex->m_exprlitmapkeys.count(); i++)
            {
                auto keyexpr = ex->m_exprlitmapkeys.get(i);
                auto valexpr = ex->m_exprlitmapvalues.get(i);
                printExpression(keyexpr);
                put(" : ");
                printExpression(valexpr);
                if(i < (ex->m_exprlitmapkeys.count() - 1))
                {
                    put(", ");
                }
            }
            put("}");
        }

        void printPrefix(AstExpression* astexpr)
        {
            auto ex = static_cast<ExprPrefix*>(astexpr);
            put("(");
            put(getMathOpString(ex->m_exprprefixoper));
            printExpression(ex->m_exprprefixright);
            put(")");
        }

        void printInfix(AstExpression* astexpr)
        {
            auto ex = static_cast<ExprInfix*>(astexpr);
            put("(");
            printExpression(ex->m_exprinfixleft);
            put(" ");
            put(getMathOpString(ex->m_exprinfixoper));
            put(" ");
            printExpression(ex->m_exprinfixright);
            put(")");
        }

        void printIndex(AstExpression* astexpr)
        {
            bool prevquot;
            auto ex = static_cast<ExprIndex*>(astexpr);
            put("(");
            printExpression(ex->m_exprindexleft);
            if(ex->m_exprindexisdot)
            {
                put(".");
                prevquot = m_pdest->m_prconfig.quotstring;
                m_pdest->m_prconfig.quotstring = false;
                printExpression(ex->m_exprindexindex);
                m_pdest->m_prconfig.quotstring = prevquot;
            }
            else
            {
                put("[");
                printExpression(ex->m_exprindexindex);
                put("]");
            }
            put(")");
        }

        void printAssign(AstExpression* astexpr)
        {
            auto ex = static_cast<ExprAssign*>(astexpr);
            printExpression(ex->m_exprassigndest);
            put(" = ");
            printExpression(ex->m_exprassignsource);
        }

        void printLogical(AstExpression* astexpr)
        {
            auto ex = static_cast<ExprLogical*>(astexpr);
            printExpression(ex->m_exprlogleft);
            put(" ");
            put(getMathOpString(ex->m_exprlogoper));
            put(" ");
            printExpression(ex->m_exprlogright);
        }

        void printTernary(AstExpression* astexpr)
        {
            auto ex = static_cast<ExprTernary*>(astexpr);
            printExpression(ex->m_exprterncond);
            put(" ? ");
            printExpression(ex->m_exprterniftrue);
            put(" : ");
            printExpression(ex->m_exprterniffalse);
        }

        void printDefine(AstExpression* astexpr)
        {
            auto ex = static_cast<ExprDefine*>(astexpr);
            if(ex->m_exprdefassignable)
            {
                put("var ");
            }
            else
            {
                put("const ");
            }
            put(ex->m_exprdefname->m_identvalue);
            put(" = ");
            if(ex->m_exprdefvalue != nullptr)
            {
                printExpression(ex->m_exprdefvalue);
            }
        }

        void printIf(AstExpression* astexpr)
        {
            size_t i;
            auto ex = static_cast<ExprIfStmt*>(astexpr);
            auto ifcase = ex->m_exprifcases.get(0);
            put("if (");
            printExpression(ifcase->m_exprifcond);
            put(") ");
            printCodeblock(ifcase->m_exprifthen);
            for(i = 1; i < ex->m_exprifcases.count(); i++)
            {
                auto elifcase = ex->m_exprifcases.get(i);
                put(" elif (");
                printExpression(elifcase->m_exprifcond);
                put(") ");
                printCodeblock(elifcase->m_exprifthen);
            }
            if(ex->m_exprifelse != nullptr)
            {
                put(" else ");
                printCodeblock(ex->m_exprifelse);
            }
        }

        void printWhile(AstExpression* astexpr)
        {
            auto ex = static_cast<ExprWhileStmt*>(astexpr);
            put("while (");
            printExpression(ex->m_exprwhilecond);
            put(")");
            printCodeblock(ex->m_exprwhilebody);
        }

        void printForClassic(AstExpression* astexpr)
        {
            auto ex = static_cast<ExprLoopStmt*>(astexpr);
            put("for (");
            if(ex->m_exprforloopinit != nullptr)
            {
                printExpression(ex->m_exprforloopinit);
                put(" ");
            }
            else
            {
                put(";");
            }
            if(ex->m_exprforloopcond != nullptr)
            {
                printExpression(ex->m_exprforloopcond);
                put("; ");
            }
            else
            {
                put(";");
            }
            if(ex->m_exprforloopupdate != nullptr)
            {
                printExpression(ex->m_exprforloopupdate);
            }
            put(")");
            printCodeblock(ex->m_exprforloopbody);
        }

        void printForeach(AstExpression* astexpr)
        {
            auto ex = static_cast<ExprForeachStmt*>(astexpr);
            put("for (");
            putfmt("%s", ex->m_exprforeachiterator->m_identvalue);
            put(" in ");
            printExpression(ex->m_exprforeachsource);
            put(")");
            printCodeblock(ex->m_exprforeachbody);
        }

        void printImport(AstExpression* astexpr)
        {
            auto ex = static_cast<ExprImportStmt*>(astexpr);
            putfmt("import \"%s\"", ex->m_exprimportpath);
        }

        void printRecover(AstExpression* astexpr)
        {
            auto ex = static_cast<ExprRecover*>(astexpr);
            putfmt("recover (%s)", ex->m_exprrecovererrident->m_identvalue);
            printCodeblock(ex->m_exprrecoverbody);
        }

        void printCodeblock(AstCodeBlock* blockexpr)
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
                        auto ex = static_cast<ExprIdent*>(astexpr)->m_expridvalue;
                        put(ex->m_identvalue);
                    }
                    break;
                case AstExpression::EXPR_LITERALNUMBER:
                    {
                        auto fl = static_cast<ExprLiteralNumber*>(astexpr)->m_exprnumber;
                        putfmt("%1.17g", fl);
                    }
                    break;
                case AstExpression::EXPR_LITERALBOOL:
                    {
                        auto bl = static_cast<ExprLiteralBool*>(astexpr)->m_exprbool;
                        putfmt("%s", bl ? "true" : "false");
                    }
                    break;
                case AstExpression::EXPR_LITERALSTRING:
                    {
                        printStringLiteral(astexpr);
                    }
                    break;
                case AstExpression::EXPR_LITERALNULL:
                    {
                        put("null");
                    }
                    break;
                case AstExpression::EXPR_LITERALARRAY:
                    {
                        printArrayLiteral(astexpr);
                    }
                    break;
                case AstExpression::EXPR_LITERALMAP:
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
                case AstExpression::EXPR_LITERALFUNCTION:
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
                        auto ex = static_cast<ExprReturnStmt*>(astexpr)->m_exprretvalue;
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
                        auto ex = static_cast<ExprExpression*>(astexpr)->m_exprexprvalue;
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
                        AstCodeBlock* ex;
                        ex = static_cast<ExprBlock*>(astexpr)->m_exprblockvalue;
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
            m_modname = Util::strDuplicate(nm);
            MC_ASSERT(m_modname);
            if(ms != nullptr)
            {
                m_modsymbols = *ms;
            }
        }

        bool addSymbol(AstSymbol* symbol)
        {
            bool ok;
            (void)ok;
            IOHandle namebuf(nullptr);
            ok = namebuf.format("%s::%s", m_modname, symbol->m_symname);
            auto modulesymbol = Memory::make<AstSymbol>(namebuf.getString(), AstSymbol::SYMTYP_MODULEGLOBAL, symbol->m_symindex, false);
            namebuf.release(false);
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
        template<typename... ArgsT>
        void complain(SourceLocation pos, const char *fmt, ArgsT&&... args)
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

        static void destroy(AstCompiler* comp)
        {
            if(comp != nullptr)
            {
                comp->deinit();
                mc_memory_free(comp);
            }
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
            StrDict<char*, Module*>* modulescopy;
            GenericList<Value> constantscopy;
            GenericList<char*>* srcloadedmodulenames;
            GenericList<char*>* copyloadedmodulenames;
            AstSymTable* srcst;
            AstSymTable* srcstocopy;
            AstSymTable* copyst;
            AstScopeFile* srcfilescope;
            AstScopeFile* copyfilescope;
            (void)ok;
            ok = copy->initBase(src->m_pstate, src->m_config, src->m_astmem, src->m_ccerrlist, src->m_sourcefiles, src->m_compglobalstore, src->m_filestderr);
            if(!ok)
            {
                return false;
            }
            srcst = src->getsymtable();
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
            copy->m_modules->destroyItemsAndDict();
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
                val = src->m_stringconstposdict->getValueAt(i);
                valcopy = (int*)mc_memory_malloc(sizeof(int));
                if(valcopy == nullptr)
                {
                    goto compilercopyfailed;
                }
                *valcopy = *val;
                copy->m_stringconstposdict->set((char*)key, valcopy);
            }
            srcfilescope = src->m_filescopelist.topp();
            copyfilescope = copy->m_filescopelist.topp();
            srcloadedmodulenames = &srcfilescope->m_filescopeloadednames;
            copyloadedmodulenames = &copyfilescope->m_filescopeloadednames;
            for(i = 0; i < srcloadedmodulenames->count(); i++)
            {
                loadedname = (const char*)srcloadedmodulenames->get(i);
                loadednamecopy = Util::strDuplicate(loadedname);
                if(loadednamecopy == nullptr)
                {
                    goto compilercopyfailed;
                }
                copyloadedmodulenames->push(loadednamecopy);
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
        GenericList<SourceFile*>* m_sourcefiles = nullptr;
        SymStore* m_compglobalstore = nullptr;
        GenericList<Value> m_constants = GenericList<Value>(0);
        AstScopeComp* m_compilationscope = nullptr;
        GenericList<SourceLocation> m_srcposstack = GenericList<SourceLocation>(0);
        StrDict<char*, Module*>* m_modules = nullptr;
        StrDict<char*, int*>* m_stringconstposdict = nullptr;
        IOHandle* m_filestderr = nullptr;
        GenericList<AstScopeFile> m_filescopelist = GenericList<AstScopeFile>(0);

    public:
        AstCompiler()
        {
        }

        AstCompiler(State* state, RuntimeConfig* config, GCMemory* gcmem, ErrList* errors, GenericList<SourceFile*>* files, SymStore* gstore, IOHandle* fstderr)
        {
            initBase(state, config, gcmem, errors, files, gstore, fstderr);
            m_pstate = state; 
        }

        bool initBase(State* state, RuntimeConfig* cfg, GCMemory* gcmem, ErrList* errors, GenericList<SourceFile*>* files, SymStore* gstor, IOHandle* fstderr)
        {
            const char* filename;
            m_pstate = state;
            m_config = cfg;
            m_astmem = gcmem;
            m_ccerrlist = errors;
            m_sourcefiles = files;
            m_compglobalstore = gstor;
            m_filestderr = fstderr;
            m_modules = Memory::make<StrDict<char*, Module*>>((CallbackCopyFN)Module::copy, (CallbackDestroyFN)Module::destroy);
            pushCompilationScope();
            filename = "<none>";
            if(files->count() > 0)
            {
                filename = (const char*)files->top();
            }
            filescopepush(filename);
            m_stringconstposdict = Memory::make<StrDict<char*, int*>>(nullptr, nullptr);
            return true;
        }

        void deinit()
        {
            size_t i;
            for(i = 0; i < m_stringconstposdict->count(); i++)
            {
                auto val = m_stringconstposdict->getValueAt(i);
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
            m_modules->destroyItemsAndDict();
            m_srcposstack.deInit();
            m_constants.deInit();
            m_filescopelist.deInit();
        }

        void appendByteAt(GenericList<OPValCode>* res, const uint64_t* operands, int i, int n)
        {
            auto val = (OPValCode)(operands[i] >> (n * 8));
            res->push(val);
        }

        int genCode(OPValCode op, int operandscount, const uint64_t* operands, GenericList<OPValCode>* res)
        {
            int i;
            int width;
            int instrlen;
            OPValCode val;
            Instruction::Definition vdef;
            Instruction::Definition* def;
            def = Instruction::opdefLookup(&vdef, (Instruction::Code)op);
            if(def == nullptr)
            {
                return 0;
            }
            MC_ASSERT(operandscount == def->numoperands);
            instrlen = 1;
            for(i = 0; i < def->numoperands; i++)
            {
                instrlen += def->operandwidths[i];
            }
            val = op;
            res->push(val);
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

        template<typename OpTypeT>
        int emitOpCode(OpTypeT op, int operandscount, uint64_t* operands)
        {
            int i;
            int ip;
            int len;
            SourceLocation srcpos;
            AstScopeComp* compscope;
            ip = getip();
            len = genCode((OPValCode)op, operandscount, operands, getbytecode());
            if(len == 0)
            {
                return -1;
            }
            for(i = 0; i < len; i++)
            {
                srcpos = m_srcposstack.top();
                getsrcpositions()->push(srcpos);
            }
            compscope = getCompilationScope();
            compscope->m_scopelastopcode = (OPValCode)op;
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

        OPValCode getLastOpcode()
        {
            AstScopeComp* currentscope;
            currentscope = getCompilationScope();
            return currentscope->m_scopelastopcode;
        }

        bool doCompileSource(const char* code)
        {
            AstScopeFile* filescope;
            GenericList<AstExpression*> statements;
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
            compileStmtList(&statements);
            statements.deInit(AstExpression::destroyExpression);
            if(m_config->dumpbytecode)
            {
                Instruction::bcPrintByteCodeTo(m_filestderr,
                    m_compilationscope->m_scopecompiledbc.data(),
                    m_compilationscope->m_scopesrcposlist.data(),
                    m_compilationscope->m_scopecompiledbc.count(), false);
            }
            return true;
        }

        bool compileStmtList(GenericList<AstExpression*>* statements)
        {
            size_t i;
            AstExpression* expr;
            for(i = 0; i < statements->count(); i++)
            {
                expr = statements->get(i);
                if(!compileExpression(expr))
                {
                    return false;
                }
            }
            return true;
        }

        bool compileImportStmt(AstExpression* expr)
        {
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
            IOHandle filepathbuf(nullptr);
            AstSymTable* symtab;
            AstScopeFile* fs;
            Module* module;
            AstSymTable* st;
            AstScopeFile* filescope;
            AstSymbol* symbol;
            result = false;
            filepath = nullptr;
            code = nullptr;
            filescope = m_filescopelist.topp();
            auto importstmt = static_cast<ExprImportStmt*>(expr);
            modpath = importstmt->m_exprimportpath;
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
                        complain(importstmt->m_exprpos, "module \"%s\" already imported; ignoring 'import' statement", modname);
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
            filepathbuf.release(false);
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
            module = m_modules->get(filepath);
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
                filescopepush(searchedpath);
                doCompileSource(code);
                st = getsymtable();
                for(i = 0; i < st->getModuleGlobalSymCount(); i++)
                {
                    symbol = st->getModuleGlobalSymAt(i);
                    module->addSymbol(symbol);
                }
                filescopepop();
                m_modules->set(filepath, module);
            }
            for(i = 0; i < module->m_modsymbols.count(); i++)
            {
                symbol = module->m_modsymbols.get(i);
                symtab->addModuleSymbol(symbol);
            }
            namecopy = Util::strDuplicate(modname);
            filescope->m_filescopeloadednames.push(namecopy);
            result = true;
        end:
            mc_memory_free(filepath);
            mc_memory_free(code);
            return result;
        }

        AstSymbol* doDefineSymbol(SourceLocation pos, const char* name, bool assignable, bool canshadow)
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
            AstSymbol* symbol;
            auto ud = static_cast<ExprDefine*>(expr);
            if(!compileExpression(ud->m_exprdefvalue))
            {
                return false;
            }
            symbol = doDefineSymbol(ud->m_exprdefname->m_exprpos, ud->m_exprdefname->m_identvalue, ud->m_exprdefassignable, false);
            if(symbol == nullptr)
            {
                return false;
            }
            if(!storesymbol(symbol, true))
            {
                return false;
            }
            return true;
        }

        bool compileIfStmt(AstExpression* expr)
        {
            size_t i;
            int afteraltip;
            int nextcasejumpip;
            int jumptoendip;
            int afterelifip;
            int pos;
            uint64_t opbuf[10];
            GenericList<int> jumptoendips;
            auto ifstmt = static_cast<ExprIfStmt*>(expr);
            for(i = 0; i < ifstmt->m_exprifcases.count(); i++)
            {
                auto ifcase = ifstmt->m_exprifcases.get(i);
                if(!compileExpression(ifcase->m_exprifcond))
                {
                    return false;
                }
                opbuf[0] = 0xbeef;
                nextcasejumpip = emitOpCode(Instruction::OPCODE_JUMPIFFALSE, 1, opbuf);
                if(!compilecodeblock(ifcase->m_exprifthen))
                {
                    return false;
                }
                /* don't emit jump for the last statement */
                if(i < (ifstmt->m_exprifcases.count() - 1) || (ifstmt->m_exprifelse != nullptr))
                {
                    opbuf[0] = 0xbeef;
                    jumptoendip = emitOpCode(Instruction::OPCODE_JUMP, 1, opbuf);
                    jumptoendips.push(jumptoendip);
                }
                afterelifip = getip();
                changeOperand(nextcasejumpip + 1, afterelifip);
            }
            if(ifstmt->m_exprifelse != nullptr)
            {
                if(!compilecodeblock(ifstmt->m_exprifelse))
                {
                    return false;
                }
            }
            afteraltip = getip();
            for(i = 0; i < jumptoendips.count(); i++)
            {
                pos = jumptoendips.get(i);
                changeOperand(pos + 1, afteraltip);
            }
            jumptoendips.deInit();
            return true;
        }

        bool compilereturnstmt(AstExpression* expr)
        {
            auto compscope = getCompilationScope();
            if(compscope->m_outerscope == nullptr)
            {
                m_ccerrlist->pushFormat( Error::ERRTYP_COMPILING, expr->m_exprpos, "nothing to return from");
                return false;
            }
            auto rts = static_cast<ExprReturnStmt*>(expr);
            if(rts->m_exprretvalue != nullptr)
            {
                if(!compileExpression(rts->m_exprretvalue))
                {
                    return false;
                }
                emitOpCode(Instruction::OPCODE_RETURNVALUE, 0, nullptr);
            }
            else
            {
                emitOpCode(Instruction::OPCODE_RETURN, 0, nullptr);
            }
            return true;
        }

        bool compilewhilestmt(AstExpression* expr)
        {
            int beforetestip;
            int aftertestip;
            int afterbodyip;
            int jumptoafterbodyip;
            uint64_t opbuf[10];
            auto loop = static_cast<ExprWhileStmt*>(expr);
            beforetestip = getip();
            if(!compileExpression(loop->m_exprwhilecond))
            {
                return false;
            }
            aftertestip = getip();
            opbuf[0] = aftertestip + 6;
            emitOpCode(Instruction::OPCODE_JUMPIFTRUE, 1, opbuf);
            opbuf[0] = 0xdead;
            jumptoafterbodyip = emitOpCode(Instruction::OPCODE_JUMP, 1, opbuf);
            if(jumptoafterbodyip < 0)
            {
                return false;
            }
            {
                pushcontinueip(beforetestip);
                {
                    pushbreakip(jumptoafterbodyip);
                    {
                        if(!compilecodeblock(loop->m_exprwhilebody))
                        {
                            return false;
                        }
                    }
                    popbreakip();
                }
                popcontinueip();
            }
            opbuf[0] = beforetestip;
            emitOpCode(Instruction::OPCODE_JUMP, 1, opbuf);
            afterbodyip = getip();
            changeOperand(jumptoafterbodyip + 1, afterbodyip);
            return true;
        }

        bool compilebreakstmt(AstExpression* expr)
        {
            int breakip;
            uint64_t opbuf[10];
            breakip = getbreakip();
            if(breakip < 0)
            {
                m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "nothing to break from.");
                return false;
            }
            opbuf[0] = breakip;
            emitOpCode(Instruction::OPCODE_JUMP, 1, opbuf);
            return true;
        }

        bool compilecontinuestmt(AstExpression* expr)
        {
            int continueip;
            uint64_t opbuf[10];
            continueip = getcontinueip();
            if(continueip < 0)
            {
                m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "nothing to continue from.");
                return false;
            }
            opbuf[0] = continueip;
            emitOpCode(Instruction::OPCODE_JUMP, 1, opbuf);
            return true;
        }

        bool compileFuncLiteral(AstExpression* expr)
        {
            size_t i;
            int pos;
            int nlocals;
            Value val;
            CompiledProgram* comp_res;
            ExprLiteralFunction* fn;
            AstSymbol* symbol;
            AstSymbol* fnsymbol;
            AstSymbol* thissymbol;
            AstSymbol* paramsymbol;
            ExprFuncParam* param;
            uint64_t opbuf[10];
            fn = static_cast<ExprLiteralFunction*>(expr);
            pushCompilationScope();
            pushSymtable(0);
            auto compscope = getCompilationScope();
            auto symtab = getsymtable();
            if(fn->m_exprfnname != nullptr)
            {
                fnsymbol = symtab->defineFunctionName(fn->m_exprfnname, false);
                if(fnsymbol == nullptr)
                {
                    m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "cannot define function name as \"%s\"", fn->m_exprfnname);
                    return false;
                }
            }
            thissymbol = symtab->defineThis();
            if(thissymbol == nullptr)
            {
                m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "cannot define \"this\" symbol");
                return false;
            }
            for(i = 0; i < fn->m_exprfnparamlist.count(); i++)
            {
                param = fn->m_exprfnparamlist.get(i);
                paramsymbol = doDefineSymbol(param->m_exprfnparamident->m_exprpos, param->m_exprfnparamident->m_identvalue, true, false);
                if(paramsymbol == nullptr)
                {
                    return false;
                }
            }
            if(!compileStmtList(&fn->m_exprfnbody->m_blockstatements))
            {
                return false;
            }
            if(!lastopcodeis(Instruction::OPCODE_RETURNVALUE) && !lastopcodeis(Instruction::OPCODE_RETURN))
            {
                emitOpCode(Instruction::OPCODE_RETURN, 0, nullptr);
            }
            auto freesyms = symtab->m_symtbfreesymbols;
            /* because it gets destroyed with compiler_pop_compilation_scope() */
            symtab->m_symtbfreesymbols = nullptr;
            nlocals = symtab->m_symtbmaxnumdefinitions;
            comp_res = compscope->orphanResult();
            if(comp_res == nullptr)
            {
                Memory::destroy(freesyms, AstSymbol::destroy);
                return false;
            }
            popSymtable();
            popCompilationScope();
            compscope = getCompilationScope();
            symtab = getsymtable();
            val = Value::makeFuncScript(fn->m_exprfnname, comp_res, true, nlocals, fn->m_exprfnparamlist.count(), 0);
            if(val.isNull())
            {
                Memory::destroy(freesyms, AstSymbol::destroy);
                CompiledProgram::destroy(comp_res);
                return false;
            }
            for(i = 0; i < freesyms->count(); i++)
            {
                symbol = freesyms->get(i);
                if(!readsymbol(symbol))
                {
                    Memory::destroy(freesyms, AstSymbol::destroy);
                    return false;
                }
            }
            pos = addconstant(val);
            if(pos < 0)
            {
                Memory::destroy(freesyms, AstSymbol::destroy);
                return false;
            }
            opbuf[0] = pos;
            opbuf[1] = freesyms->count();
            emitOpCode(Instruction::OPCODE_FUNCTION, 2, opbuf);
            Memory::destroy(freesyms, AstSymbol::destroy);
            return true;
        }

        bool compilePrefixExpr(AstExpression* expr)
        {
            Instruction::Code op;
            auto pre = static_cast<ExprPrefix*>(expr);
            if(!compileExpression(pre->m_exprprefixright))
            {
                return false;
            }
            op = Instruction::OPCODE_HALT;
            switch(pre->m_exprprefixoper)
            {
                case AstExpression::MATHOP_MINUS:
                    op = Instruction::OPCODE_MINUS;
                    break;
                case AstExpression::MATHOP_BINNOT:
                    op = Instruction::OPCODE_BINNOT;
                    break;
                case AstExpression::MATHOP_BANG:
                    op = Instruction::OPCODE_BANG;
                    break;
                default:
                    {
                        m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "unknown prefix operator.");
                        return false;
                    }
                    break;
            }
            emitOpCode(op, 0, nullptr);
            return true;
        }

        bool compileInfixExpr(AstExpression* expr)
        {
            bool rearrange;
            Instruction::Code op;
            AstExpression* left;
            AstExpression* right;
            rearrange = false;
            op = Instruction::OPCODE_HALT;
            auto infe = static_cast<ExprInfix*>(expr);
            switch(infe->m_exprinfixoper)
            {
                case AstExpression::MATHOP_PLUS:
                    {
                        op = Instruction::OPCODE_ADD;
                    }
                    break;
                case AstExpression::MATHOP_MINUS:
                    {
                        op = Instruction::OPCODE_SUB;
                    }
                    break;
                case AstExpression::MATHOP_ASTERISK:
                    {
                        op = Instruction::OPCODE_MUL;
                    }
                    break;
                case AstExpression::MATHOP_SLASH:
                    {
                        op = Instruction::OPCODE_DIV;
                    }
                    break;
                case AstExpression::MATHOP_MODULUS:
                    {
                        op = Instruction::OPCODE_MOD;
                    }
                    break;
                case AstExpression::MATHOP_EQUAL:
                    {
                        op = Instruction::OPCODE_EQUAL;
                    }
                    break;
                case AstExpression::MATHOP_NOTEQUAL:
                    {
                        op = Instruction::OPCODE_NOTEQUAL;
                    }
                    break;
                case AstExpression::MATHOP_GREATERTHAN:
                    {
                        op = Instruction::OPCODE_GREATERTHAN;
                    }
                    break;
                case AstExpression::MATHOP_GREATEREQAL:
                    {
                        op = Instruction::OPCODE_GREATERTHANEQUAL;
                    }
                    break;
                case AstExpression::MATHOP_LESSTHAN:
                    {
                        op = Instruction::OPCODE_GREATERTHAN;
                        rearrange = true;
                    }
                    break;
                case AstExpression::MATHOP_LESSEQUAL:
                    {
                        op = Instruction::OPCODE_GREATERTHANEQUAL;
                        rearrange = true;
                    }
                    break;
                case AstExpression::MATHOP_BINOR:
                    {
                        op = Instruction::OPCODE_BINOR;
                    }
                    break;
                case AstExpression::MATHOP_BINXOR:
                    {
                        op = Instruction::OPCODE_BINXOR;
                    }
                    break;
                case AstExpression::MATHOP_BINAND:
                    {
                        op = Instruction::OPCODE_BINAND;
                    }
                    break;
                case AstExpression::MATHOP_LEFTSHIFT:
                    {
                        op = Instruction::OPCODE_LSHIFT;
                    }
                    break;
                case AstExpression::MATHOP_RIGHTSHIFT:
                    {
                        op = Instruction::OPCODE_RSHIFT;
                    }
                    break;
                default:
                    {
                        m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "unknown infix operator");
                        return false;
                    }
                    break;
            }
            left = rearrange ? infe->m_exprinfixright : infe->m_exprinfixleft;
            right = rearrange ? infe->m_exprinfixleft : infe->m_exprinfixright;
            if(!compileExpression(left))
            {
                return false;
            }
            if(!compileExpression(right))
            {
                return false;
            }
            switch(infe->m_exprinfixoper)
            {
                case AstExpression::MATHOP_EQUAL:
                case AstExpression::MATHOP_NOTEQUAL:
                    {
                        emitOpCode(Instruction::OPCODE_COMPAREEQ, 0, nullptr);
                    }
                    break;
                case AstExpression::MATHOP_GREATERTHAN:
                case AstExpression::MATHOP_GREATEREQAL:
                case AstExpression::MATHOP_LESSTHAN:
                case AstExpression::MATHOP_LESSEQUAL:
                    {
                        emitOpCode(Instruction::OPCODE_COMPARE, 0, nullptr);
                    }
                    break;
                default:
                    {
                    }
                    break;
            }
            emitOpCode(op, 0, nullptr);
            return true;
        }

        bool compileStringLiteral(AstExpression* expr)
        {
            int pos;
            int* posval;
            int* currentpos;
            Value val;
            uint64_t opbuf[10];
            pos = 0;
            auto lits = static_cast<ExprLiteralString*>(expr);
            currentpos = m_stringconstposdict->get(lits->m_exprstrdata);
            if(currentpos != nullptr)
            {
                pos = *currentpos;
            }
            else
            {
                val = Value::makeString(lits->m_exprstrdata, lits->m_exprstrlength);
                if(val.isNull())
                {
                    return false;
                }
                pos = addconstant(val);
                if(pos < 0)
                {
                    return false;
                }
                posval = (int*)mc_memory_malloc(sizeof(int));
                if(posval == nullptr)
                {
                    return false;
                }
                *posval = pos;
                m_stringconstposdict->set(lits->m_exprstrdata, posval);
            }
            opbuf[0] = pos;
            emitOpCode(Instruction::OPCODE_CONSTANT, 1, opbuf);
            return true;
        }

        bool compileRecoverStmt(AstExpression* expr)
        {
            int recip;
            int afterrecoverip;
            int afterjumptorecoverip;
            int jumptoafterrecoverip;
            AstSymTable* symtab;
            AstSymbol* errorsymbol;
            ExprRecover* recover;
            uint64_t opbuf[10];
            symtab = getsymtable();
            recover = static_cast<ExprRecover*>(expr);
            if(m_config->strictmode)
            {
                if(symtab->isModuleGlobalScope())
                {
                    m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, expr->m_exprpos, "recover statement cannot be defined in global scope");
                    return false;
                }
            }
            opbuf[0] = 0xbeef;
            recip = emitOpCode(Instruction::OPCODE_SETRECOVER, 1, opbuf);
            if(recip < 0)
            {
                return false;
            }
            opbuf[0] = 0xbeef;
            jumptoafterrecoverip = emitOpCode(Instruction::OPCODE_JUMP, 1, opbuf);
            if(jumptoafterrecoverip < 0)
            {
                return false;
            }
            afterjumptorecoverip = getip();
            changeOperand(recip + 1, afterjumptorecoverip);
            symtab->scopeBlockPush();
            errorsymbol = doDefineSymbol(recover->m_exprrecovererrident->m_exprpos, recover->m_exprrecovererrident->m_identvalue, false, false);
            if(errorsymbol == nullptr)
            {
                return false;
            }
            if(!storesymbol(errorsymbol, true))
            {
                return false;
            }
            if(!compilecodeblock(recover->m_exprrecoverbody))
            {
                return false;
            }
            if(!lastopcodeis(Instruction::OPCODE_RETURN) && !lastopcodeis(Instruction::OPCODE_RETURNVALUE))
            {
                complain(expr->m_exprpos, "recover body should end with a return statement");
            }
            symtab->scopeBlockPop();
            afterrecoverip = getip();
            changeOperand(jumptoafterrecoverip + 1, afterrecoverip);
            return true;
        }

        bool compileNumberLiteral(AstExpression* expr)
        {
            NumFloat number;
            uint64_t opbuf[10];
            number = static_cast<ExprLiteralNumber*>(expr)->m_exprnumber;
            opbuf[0] = mc_util_doubletouint64(number);
            emitOpCode(Instruction::OPCODE_NUMBER, 1, opbuf);
            return true;
        }

        bool compileNullLiteral(AstExpression* expr)
        {
            (void)expr;
            emitOpCode(Instruction::OPCODE_NULL, 0, nullptr);
            return true;
        }

        bool compileArrayLiteral(AstExpression* expr)
        {
            size_t i;
            uint64_t opbuf[10];
            auto arr = static_cast<ExprLiteralArray*>(expr);
            for(i = 0; i < arr->m_exprlitarritems.count(); i++)
            {
                if(!compileExpression(arr->m_exprlitarritems.get(i)))
                {
                    return false;
                }
            }
            opbuf[0] = arr->m_exprlitarritems.count();
            emitOpCode(Instruction::OPCODE_ARRAY, 1, opbuf);
            return true;
        }

        bool compileMapLiteral(AstExpression* expr)
        {
            size_t i;
            size_t len;
            AstExpression* key;
            AstExpression* val;
            ExprLiteralMap* map;
            uint64_t opbuf[10];
            map = static_cast<ExprLiteralMap*>(expr);
            len = map->m_exprlitmapkeys.count();
            opbuf[0] = len;
            emitOpCode(Instruction::OPCODE_MAPSTART, 1, opbuf);
            for(i = 0; i < len; i++)
            {
                key = map->m_exprlitmapkeys.get(i);
                val = map->m_exprlitmapvalues.get(i);
                if(!compileExpression(key))
                {
                    return false;
                }
                if(!compileExpression(val))
                {
                    return false;
                }
            }
            opbuf[0] = len;
            emitOpCode(Instruction::OPCODE_MAPEND, 1, opbuf);
            return true;
        }

        bool compileAssignExpr(AstExpression* expr)
        {
            AstSymTable* symtab;
            ExprIndex* index;
            ExprAssign* assign;
            AstIdentifier* ident;
            AstSymbol* symbol;
            symtab = getsymtable();
            assign = static_cast<ExprAssign*>(expr);
            if(assign->m_exprassigndest->m_exprtype != AstExpression::EXPR_IDENT && assign->m_exprassigndest->m_exprtype != AstExpression::EXPR_INDEX)
            {
                m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, assign->m_exprassigndest->m_exprpos, "expression is not assignable");
                return false;
            }
            if(assign->m_exprassignispostfix)
            {
                if(!compileExpression(assign->m_exprassigndest))
                {
                    return false;
                }
            }
            if(!compileExpression(assign->m_exprassignsource))
            {
                return false;
            }
            emitOpCode(Instruction::OPCODE_DUP, 0, nullptr);
            m_srcposstack.push(assign->m_exprassigndest->m_exprpos);
            if(assign->m_exprassigndest->m_exprtype == AstExpression::EXPR_IDENT)
            {
                auto ide = static_cast<ExprIdent*>(assign->m_exprassigndest);
                ident = ide->m_expridvalue;
                symbol = symtab->resolve(ident->m_identvalue);
                if(symbol == nullptr)
                {
                    if(m_config->strictmode)
                    {
                        m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, assign->m_exprassigndest->m_exprpos, "cannot assign to undeclared symbol \"%s\"", ident->m_identvalue);
                        return false;
                    }
                    else
                    {
                        symbol = doDefineSymbol(ident->m_exprpos, ident->m_identvalue, true, false);
                        if(symbol == nullptr)
                        {
                            m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, assign->m_exprassigndest->m_exprpos, "failed to implicitly create symbol \"%s\"", ident->m_identvalue);
                            return false;
                        }
                    }
                }
                if(!symbol->m_symisassignable)
                {
                    m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, assign->m_exprassigndest->m_exprpos, "compilation: cannot assign to readonly symbol \"%s\"", ident->m_identvalue);
                    return false;
                }
                if(!storesymbol(symbol, false))
                {
                    return false;
                }
            }
            else if(assign->m_exprassigndest->m_exprtype == AstExpression::EXPR_INDEX)
            {
                index = static_cast<ExprIndex*>(assign->m_exprassigndest);
                if(!compileExpression(index->m_exprindexleft))
                {
                    return false;
                }
                if(!compileExpression(index->m_exprindexindex))
                {
                    return false;
                }
                emitOpCode(Instruction::OPCODE_SETINDEX, 0, nullptr);
            }
            if(assign->m_exprassignispostfix)
            {
                emitOpCode(Instruction::OPCODE_POP, 0, nullptr);
            }
            m_srcposstack.pop(nullptr);
            return true;
        }

        bool compileIndexExpr(AstExpression* expr)
        {
            auto index = static_cast<ExprIndex*>(expr);
            if(!compileExpression(index->m_exprindexleft))
            {
                return false;
            }
            if(!compileExpression(index->m_exprindexindex))
            {
                return false;
            }
            if(index->m_exprindexisdot)
            {
                emitOpCode(Instruction::OPCODE_GETDOTINDEX, 0, nullptr);
            }
            else
            {
                emitOpCode(Instruction::OPCODE_GETINDEX, 0, nullptr);
            }
            return true;
        }

        bool compileForeachStmt(AstExpression* expr)
        {
            int jumptoafterupdateip;
            int updateip;
            int afterupdateip;
            int aftertestip;
            int jumptoafterbodyip;
            int afterbodyip;
            AstSymbol* sourcesymbol;
            uint64_t opbuf[10];
            auto symtab = getsymtable();
            auto foreach = static_cast<ExprForeachStmt*>(expr);
            symtab->scopeBlockPush();
            /* Init */
            auto indexsymbol = doDefineSymbol(expr->m_exprpos, "@i", false, true);
            if(indexsymbol == nullptr)
            {
                return false;
            }
            opbuf[0] = 0;
            emitOpCode(Instruction::OPCODE_NUMBER, 1, opbuf);
            if(!storesymbol(indexsymbol, true))
            {
                return false;
            }
            sourcesymbol = nullptr;
            if(foreach->m_exprforeachsource->m_exprtype == AstExpression::EXPR_IDENT)
            {
                auto exsrc = static_cast<ExprIdent*>(foreach->m_exprforeachsource);
                sourcesymbol = symtab->resolve(exsrc->m_expridvalue->m_identvalue);
                if(sourcesymbol == nullptr)
                {
                    m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, foreach->m_exprforeachsource->m_exprpos, "symbol \"%s\" could not be resolved", exsrc->m_expridvalue->m_identvalue);
                    return false;
                }
            }
            else
            {
                if(!compileExpression(foreach->m_exprforeachsource))
                {
                    return false;
                }
                sourcesymbol = doDefineSymbol(foreach->m_exprforeachsource->m_exprpos, "@source", false, true);
                if(sourcesymbol == nullptr)
                {
                    return false;
                }
                if(!storesymbol(sourcesymbol, true))
                {
                    return false;
                }
            }
            /* Update */
            opbuf[0] = 0xbeef;
            jumptoafterupdateip = emitOpCode(Instruction::OPCODE_JUMP, 1, opbuf);
            if(jumptoafterupdateip < 0)
            {
                return false;
            }
            updateip = getip();
            if(!readsymbol(indexsymbol))
            {
                return false;
            }
            opbuf[0] = mc_util_doubletouint64(1);
            emitOpCode(Instruction::OPCODE_NUMBER, 1, opbuf);
            emitOpCode(Instruction::OPCODE_ADD, 0, nullptr);
            if(!storesymbol(indexsymbol, false))
            {
                return false;
            }
            afterupdateip = getip();
            changeOperand(jumptoafterupdateip + 1, afterupdateip);
            /* Test */
            m_srcposstack.push(foreach->m_exprforeachsource->m_exprpos);
            if(!readsymbol(sourcesymbol))
            {
                return false;
            }
            emitOpCode(Instruction::OPCODE_FOREACHLEN, 0, nullptr);
            m_srcposstack.pop(nullptr);
            if(!readsymbol(indexsymbol))
            {
                return false;
            }
            emitOpCode(Instruction::OPCODE_COMPARE, 0, nullptr);
            emitOpCode(Instruction::OPCODE_EQUAL, 0, nullptr);
            aftertestip = getip();
            opbuf[0] = aftertestip + 6;
            emitOpCode(Instruction::OPCODE_JUMPIFFALSE, 1, opbuf);
            opbuf[0] = 0xdead;
            jumptoafterbodyip = emitOpCode(Instruction::OPCODE_JUMP, 1, opbuf);
            if(jumptoafterbodyip < 0)
            {
                return false;
            }
            if(!readsymbol(sourcesymbol))
            {
                return false;
            }
            if(!readsymbol(indexsymbol))
            {
                return false;
            }
            emitOpCode(Instruction::OPCODE_GETVALUEAT, 0, nullptr);
            auto itersymbol = doDefineSymbol(foreach->m_exprforeachiterator->m_exprpos, foreach->m_exprforeachiterator->m_identvalue, false, false);
            if(itersymbol == nullptr)
            {
                return false;
            }
            if(!storesymbol(itersymbol, true))
            {
                return false;
            }
            /* Body */
            {
                pushcontinueip(updateip);
                {
                    pushbreakip(jumptoafterbodyip);
                    {
                        if(!compilecodeblock(foreach->m_exprforeachbody))
                        {
                            return false;
                        }
                    }
                    popbreakip();
                }
                popcontinueip();
            }
            opbuf[0] = updateip;
            emitOpCode(Instruction::OPCODE_JUMP, 1, opbuf);
            afterbodyip = getip();
            changeOperand(jumptoafterbodyip + 1, afterbodyip);
            symtab->scopeBlockPop();
            return true;
        }

        bool compileIdentExpr(AstExpression* expr)
        {
            AstSymTable* symtab;
            AstSymbol* symbol;
            AstIdentifier* ident;
            symtab = getsymtable();
            auto idex = static_cast<ExprIdent*>(expr);
            ident = idex->m_expridvalue;
            symbol = symtab->resolve(ident->m_identvalue);
            if(symbol == nullptr)
            {
                if(m_config->strictmode)
                {
                    m_ccerrlist->pushFormat(Error::ERRTYP_COMPILING, ident->m_exprpos, "compilation: failed to resolve symbol \"%s\"", ident->m_identvalue);
                    return false;
                }
                else
                {
                    symbol = doDefineSymbol(ident->m_exprpos, ident->m_identvalue, true, false);
                }
            }
            if(!readsymbol(symbol))
            {
                return false;
            }
            return true;
        }

        bool compileCallExpr(AstExpression* expr)
        {
            size_t i;
            AstExpression* argexpr;
            uint64_t opbuf[10];
            auto excall = static_cast<ExprCall*>(expr);
            if(!compileExpression(excall->m_exprcallfunction))
            {
                return false;
            }
            for(i = 0; i < excall->m_exprcallargs.count(); i++)
            {
                argexpr = excall->m_exprcallargs.get(i);
                if(!compileExpression(argexpr))
                {
                    return false;
                }
            }
            opbuf[0] = excall->m_exprcallargs.count();
            emitOpCode(Instruction::OPCODE_CALL, 1, opbuf);
            return true;
        }

        bool compileLogicalExpr(AstExpression* expr)
        {
            int afterrightip;
            int afterleftjumpip;
            ExprLogical* logi;
            uint64_t opbuf[10];
            logi = static_cast<ExprLogical*>(expr);
            if(!compileExpression(logi->m_exprlogleft))
            {
                return false;
            }
            emitOpCode(Instruction::OPCODE_DUP, 0, nullptr);
            afterleftjumpip = 0;
            if(logi->m_exprlogoper == AstExpression::MATHOP_LOGICALAND)
            {
                opbuf[0] = 0xbeef;
                afterleftjumpip = emitOpCode(Instruction::OPCODE_JUMPIFFALSE, 1, opbuf);
            }
            else
            {
                opbuf[0] = 0xbeef;
                afterleftjumpip = emitOpCode(Instruction::OPCODE_JUMPIFTRUE, 1, opbuf);
            }
            if(afterleftjumpip < 0)
            {
                return false;
            }
            emitOpCode(Instruction::OPCODE_POP, 0, nullptr);
            if(!compileExpression(logi->m_exprlogright))
            {
                return false;
            }
            afterrightip = getip();
            changeOperand(afterleftjumpip + 1, afterrightip);
            return true;
        }

        bool compileTernaryExpr(AstExpression* expr)
        {
            int endip;
            int elseip;
            int endjumpip;
            int elsejumpip;
            ExprTernary* ternary;
            uint64_t opbuf[10];
            ternary = static_cast<ExprTernary*>(expr);
            if(!compileExpression(ternary->m_exprterncond))
            {
                return false;
            }
            opbuf[0] = 0xbeef;
            elsejumpip = emitOpCode(Instruction::OPCODE_JUMPIFFALSE, 1, opbuf);
            if(!compileExpression(ternary->m_exprterniftrue))
            {
                return false;
            }
            opbuf[0] = 0xbeef;
            endjumpip = emitOpCode(Instruction::OPCODE_JUMP, 1, opbuf);
            elseip = getip();
            changeOperand(elsejumpip + 1, elseip);
            if(!compileExpression(ternary->m_exprterniffalse))
            {
                return false;
            }
            endip = getip();
            changeOperand(endjumpip + 1, endip);
            return true;
        }

        bool compileForloopStmt(AstExpression* expr)
        {
            int afterbodyip;
            int jumptoafterupdateip;
            int updateip;
            int afterupdateip;
            int aftertestip;
            int jumptoafterbodyip;
            uint64_t opbuf[10];
            auto symtab = getsymtable();
            auto loop = static_cast<ExprLoopStmt*>(expr);
            symtab->scopeBlockPush();
            /* Init */
            jumptoafterupdateip = 0;
            if(loop->m_exprforloopinit != nullptr)
            {
                if(!compileExpression(loop->m_exprforloopinit))
                {
                    return false;
                }
                opbuf[0] = 0xbeef;
                jumptoafterupdateip = emitOpCode(Instruction::OPCODE_JUMP, 1, opbuf);
                if(jumptoafterupdateip < 0)
                {
                    return false;
                }
            }
            /* Update */
            updateip = getip();
            if(loop->m_exprforloopupdate != nullptr)
            {
                if(!compileExpression(loop->m_exprforloopupdate))
                {
                    return false;
                }
                emitOpCode(Instruction::OPCODE_POP, 0, nullptr);
            }
            if(loop->m_exprforloopinit != nullptr)
            {
                afterupdateip = getip();
                changeOperand(jumptoafterupdateip + 1, afterupdateip);
            }
            /* Test */
            if(loop->m_exprforloopcond != nullptr)
            {
                if(!compileExpression(loop->m_exprforloopcond))
                {
                    return false;
                }
            }
            else
            {
                emitOpCode(Instruction::OPCODE_TRUE, 0, nullptr);
            }
            aftertestip = getip();
            opbuf[0] = aftertestip + 6;
            emitOpCode(Instruction::OPCODE_JUMPIFTRUE, 1, opbuf);
            opbuf[0] = 0xdead;
            jumptoafterbodyip = emitOpCode(Instruction::OPCODE_JUMP, 1, opbuf);
            if(jumptoafterbodyip < 0)
            {
                return false;
            }
            /* Body */
            pushcontinueip(updateip);
            pushbreakip(jumptoafterbodyip);
            if(!compilecodeblock(loop->m_exprforloopbody))
            {
                return false;
            }
            popbreakip();
            popcontinueip();
            opbuf[0] = updateip;
            emitOpCode(Instruction::OPCODE_JUMP, 1, opbuf);
            afterbodyip = getip();
            changeOperand(jumptoafterbodyip + 1, afterbodyip);
            symtab->scopeBlockPop();
            return true;
        }

        bool compileExpression(AstExpression* expr)
        {
            bool res;
            m_srcposstack.push(expr->m_exprpos);
            res = false;
            switch(expr->m_exprtype)
            {
                case AstExpression::EXPR_INFIX:
                    {
                        if(!compileInfixExpr(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_LITERALNUMBER:
                    {
                        if(!compileNumberLiteral(expr))
                        {
                            goto error;
                        }
                    }
                    break;

                case AstExpression::EXPR_LITERALSTRING:
                    {
                        if(!compileStringLiteral(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_LITERALNULL:
                    {
                        if(!compileNullLiteral(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_LITERALBOOL:
                    {
                        auto exbool = static_cast<ExprLiteralBool*>(expr);
                        emitOpCode(exbool->m_exprbool ? Instruction::OPCODE_TRUE : Instruction::OPCODE_FALSE, 0, nullptr);
                    }
                    break;
                case AstExpression::EXPR_LITERALARRAY:
                    {
                        if(!compileArrayLiteral(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_LITERALMAP:
                    {
                        if(!compileMapLiteral(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_PREFIX:
                    {
                        if(!compilePrefixExpr(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_IDENT:
                    {
                        if(!compileIdentExpr(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_INDEX:
                    {
                        if(!compileIndexExpr(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_LITERALFUNCTION:
                    {
                        if(!compileFuncLiteral(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_CALL:
                    {
                        if(!compileCallExpr(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_ASSIGN:
                    {
                        if(!compileAssignExpr(expr))
                        {
                            goto error;
                        }
                    }
                    break;

                case AstExpression::EXPR_LOGICAL:
                    {
                        if(!compileLogicalExpr(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_TERNARY:
                    {
                        if(!compileTernaryExpr(expr))
                        {
                            goto error;
                        }
                    }
                    break;

                case AstExpression::EXPR_STMTEXPRESSION:
                    {
                        auto exex = static_cast<ExprExpression*>(expr);
                        if(!compileExpression(exex->m_exprexprvalue))
                        {
                            goto error;
                        }
                        emitOpCode(Instruction::OPCODE_POP, 0, nullptr);
                        
                    }
                    break;
                case AstExpression::EXPR_STMTDEFINE:
                    {
                        if(!compileDefine(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTIF:
                    {
                        if(!compileIfStmt(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTRETURN:
                    {
                        if(!compilereturnstmt(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTLOOPWHILE:
                    {
                        if(!compilewhilestmt(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTBREAK:
                    {
                        if(!compilebreakstmt(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTCONTINUE:
                    {
                        if(!compilecontinuestmt(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTLOOPFOREACH:
                    {
                        if(!compileForeachStmt(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTLOOPFORCLASSIC:
                    {
                        if(!compileForloopStmt(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTBLOCK:
                    {
                        auto block = static_cast<ExprBlock*>(expr);
                        if(!compilecodeblock(block->m_exprblockvalue))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTIMPORT:
                    {
                        if(!compileImportStmt(expr))
                        {
                            goto error;
                        }
                    }
                    break;
                case AstExpression::EXPR_STMTRECOVER:
                    {
                        if(!compileRecoverStmt(expr))
                        {
                            goto error;
                        }
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

        bool compilecodeblock(AstCodeBlock* block)
        {
            size_t i;
            AstSymTable* symtab;
            AstExpression* expr;
            symtab = getsymtable();
            if(symtab == nullptr)
            {
                return false;
            }
            symtab->scopeBlockPush();
            if(block->m_blockstatements.count() == 0)
            {
                emitOpCode(Instruction::OPCODE_NULL, 0, nullptr);
                emitOpCode(Instruction::OPCODE_POP, 0, nullptr);
            }
            for(i = 0; i < block->m_blockstatements.count(); i++)
            {
                expr = block->m_blockstatements.get(i);
                if(!compileExpression(expr))
                {
                    return false;
                }
            }
            symtab->scopeBlockPop();
            return true;
        }

        int addconstant(Value val)
        {
            int pos;
            m_constants.push(val);
            pos = m_constants.count() - 1;
            return pos;
        }

        void changeOperand(int ip, OPValCode operand)
        {
            OPValCode hi;
            OPValCode lo;
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

        template<typename OpTypeT>
        bool lastopcodeis(OpTypeT op)
        {
            auto lastopcode = getLastOpcode();
            return (((OpTypeT)lastopcode) == ((OpTypeT)op));
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
                ip = emitOpCode(Instruction::OPCODE_GETMODULEGLOBAL, 1, opbuf);
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_GLOBALBUILTIN)
            {
                opbuf[0] = symbol->m_symindex;
                ip = emitOpCode(Instruction::OPCODE_GETGLOBALBUILTIN, 1, opbuf);
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_LOCAL)
            {
                opbuf[0] = symbol->m_symindex;
                ip = emitOpCode(Instruction::OPCODE_GETLOCAL, 1, opbuf);
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_FREE)
            {
                opbuf[0] = symbol->m_symindex;
                ip = emitOpCode(Instruction::OPCODE_GETFREE, 1, opbuf);
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_FUNCTION)
            {
                ip = emitOpCode(Instruction::OPCODE_CURRENTFUNCTION, 0, nullptr);
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_THIS)
            {
                ip = emitOpCode(Instruction::OPCODE_GETTHIS, 0, nullptr);
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
                    ip = emitOpCode(Instruction::OPCODE_DEFINEMODULEGLOBAL, 1, opbuf);
                }
                else
                {
                    opbuf[0] = symbol->m_symindex;
                    ip = emitOpCode(Instruction::OPCODE_SETMODULEGLOBAL, 1, opbuf);
                }
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_LOCAL)
            {
                if(define)
                {
                    opbuf[0] = symbol->m_symindex;
                    ip = emitOpCode(Instruction::OPCODE_DEFINELOCAL, 1, opbuf);
                }
                else
                {
                    opbuf[0] = symbol->m_symindex;
                    ip = emitOpCode(Instruction::OPCODE_SETLOCAL, 1, opbuf);
                }
            }
            else if(symbol->m_symtype == AstSymbol::SYMTYP_FREE)
            {
                opbuf[0] = symbol->m_symindex;
                ip = emitOpCode(Instruction::OPCODE_SETFREE, 1, opbuf);
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

        GenericList<SourceLocation>* getsrcpositions()
        {
            AstScopeComp* compscope;
            compscope = getCompilationScope();
            return &compscope->m_scopesrcposlist;
        }

        GenericList<OPValCode>* getbytecode()
        {
            AstScopeComp* compscope;
            compscope = getCompilationScope();
            return &compscope->m_scopecompiledbc;
        }

        bool filescopepush(const char* filepath)
        {
            int globaloffset;
            AstScopeBlock* prevsttopscope;
            AstSymTable* prevst;
            SourceFile* file;
            prevst = nullptr;
            if(m_filescopelist.count() > 0)
            {
                prevst = getsymtable();
            }
            file = Memory::make<SourceFile>(filepath);
            m_sourcefiles->push(file);
            AstScopeFile filescope(m_config, m_ccerrlist, file);
            m_filescopelist.push(filescope);
            globaloffset = 0;
            if(prevst != nullptr)
            {
                prevsttopscope = prevst->scopeBlockGet();
                globaloffset = prevsttopscope->m_blscopeoffset + prevsttopscope->m_blscopenumdefs;
            }
            pushSymtable(globaloffset);
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

        CompiledProgram* compileSource(const char* code, const char* filename)
        {
            AstCompiler compshallowcopy;
            AstScopeComp* compscope;
            CompiledProgram* res;
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
            filescopepush(filename);
            if(!initShallowCopy(&compshallowcopy, this))
            {
                return nullptr;
            }
            if(!doCompileSource(code))
            {
                goto compilefailed;
            }
            emitOpCode(Instruction::OPCODE_HALT, 0, 0);
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
        SourceLocation* m_framesrcposlist = nullptr;
        OPValCode* m_framebytecode = nullptr;
        int64_t m_sourcebcpos = 0;
        int64_t m_bcsize = 0;
        int64_t m_recoverip = 0;
        bool m_isrecovering = false;

    public:
        static MC_INLINE bool init(VMFrame* frame, Value functionobj, int64_t baseptr)
        {
            if(functionobj.getType() != Value::VT_FUNCSCRIPT)
            {
                return false;
            }
            auto function = functionobj.asFunction();
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

        MC_INLINE OPValCode readUint8()
        {
            OPValCode data;
            data = m_framebytecode[m_bcposition];
            m_bcposition++;
            return data;
        }

        MC_INLINE OPValCode readUint16()
        {
            OPValCode* data;
            data = m_framebytecode + m_bcposition;
            m_bcposition += 2;
            return (data[0] << 8) | data[1];
        }

        MC_INLINE uint64_t readUint64()
        {
            uint64_t res;
            OPValCode* data;
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

        MC_INLINE Instruction::Code readOpCode()
        {
            m_sourcebcpos = m_bcposition;
            return (Instruction::Code)readUint8();
        }

        MC_INLINE SourceLocation getPosition()
        {
            if(m_framesrcposlist != nullptr)
            {
                return m_framesrcposlist[m_sourcebcpos];
            }
            return SourceLocation::Invalid();
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

        class ExecInfo
        {
            public:
                size_t thisstpos;
                size_t vsposition;
                Value lastpopped;
                VMFrame* currframe;
                GenericList<Value> valuestack = GenericList<Value>(MinValstackSize);
                GenericList<Value> valthisstack = GenericList<Value>(MinVMThisstackSize);
                GenericList<Value> nativethisstack = GenericList<Value>(MinNativeThisstackSize);
                GenericList<VMFrame> framestack = GenericList<VMFrame>(MinVMFrames);

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

        MC_PROTO void makeDefaultClasses();

        void setOverloadKey(Instruction::Code opc, const char* rawstr)
        {
            Value keyobj;
            keyobj = Value::makeString(rawstr);
            m_operoverloadkeys[(int)opc] = keyobj;
        }

        bool vmInit()
        {
            int i;
            m_hadrecovered = false;
            m_globalvalcount = 0;
            m_execstate.vsposition = 0;
            m_execstate.thisstpos = 0;
            m_execstate.lastpopped = Value::makeNull();
            m_running = false;
            for(i = 0; i < State::MaxOperOverloads; i++)
            {
                m_operoverloadkeys[i] = Value::makeNull();
            }
            setOverloadKey(Instruction::OPCODE_ADD, "__operator_add__");
            setOverloadKey(Instruction::OPCODE_SUB, "__operator_sub__");
            setOverloadKey(Instruction::OPCODE_MUL, "__operator_mul__");
            setOverloadKey(Instruction::OPCODE_DIV, "__operator_div__");
            setOverloadKey(Instruction::OPCODE_MOD, "__operator_mod__");
            setOverloadKey(Instruction::OPCODE_BINOR, "__operator_or__");
            setOverloadKey(Instruction::OPCODE_BINXOR, "__operator_xor__");
            setOverloadKey(Instruction::OPCODE_BINAND, "__operator_and__");
            setOverloadKey(Instruction::OPCODE_LSHIFT, "__operator_lshift__");
            setOverloadKey(Instruction::OPCODE_RSHIFT, "__operator_rshift__");
            setOverloadKey(Instruction::OPCODE_MINUS, "__operator_minus__");
            setOverloadKey(Instruction::OPCODE_BINNOT, "__operator_binnot__");
            setOverloadKey(Instruction::OPCODE_BANG, "__operator_bang__");
            setOverloadKey(Instruction::OPCODE_COMPARE, "__cmp__");
            return true;
        }

        void vmReset()
        {
            m_execstate.vsposition = 0;
            m_execstate.thisstpos = 0;
            while(m_execstate.framestack.count() > 0)
            {
                vmPopFrame();
            }
        }

        MC_INLINE bool vmDoMakeFunction(GenericList<Value>* constants)
        {
            int i;
            OPValCode numfree;
            uint16_t constantix;
            const char* fname;
            const char* tname;
            Value::Type constanttype;
            Value freeval;
            Value functionobj;
            Value* constant;
            constantix = m_execstate.currframe->readUint16();
            numfree = m_execstate.currframe->readUint8();
            constant = constants->getp(constantix);
            if(constant == nullptr)
            {
                pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "constant %d not found", constantix);
                return false;
            }
            constanttype = (*constant).getType();
            if(constanttype != Value::VT_FUNCSCRIPT)
            {
                tname = Value::getTypename(constanttype);
                pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "%s is not a function", tname);
                return false;
            }
            auto constfun = (*constant).asFunction();
            fname = constfun->getName();
            functionobj = Value::makeFuncScript(fname, constfun->m_funcdata.valscriptfunc.compiledprogcode, false, constfun->m_funcdata.valscriptfunc.numlocals, constfun->m_funcdata.valscriptfunc.numargs, numfree);
            if(functionobj.isNull())
            {
                return false;
            }
            for(i = 0; i < numfree; i++)
            {
                freeval = m_execstate.valuestack.get(m_execstate.vsposition - numfree + i);
                functionobj.asFunction()->setFreeValAt(i, freeval);
            }
            setStackPos(m_execstate.vsposition - numfree);
            vmStackPush(functionobj);
            return true;
        }

        MC_INLINE bool vmDoCompareValues(Instruction::Code opcode)
        {
            bool ok;
            bool isoverloaded;
            const char* lefttname;
            const char* righttname;
            Value::CompareResult cres;
            Value res;
            Value left;
            Value right;
            right = vmStackPop();
            left = vmStackPop();
            isoverloaded = false;
            if(!vmTryOverloadOperator(left, right, Instruction::OPCODE_COMPARE, &isoverloaded))
            {
                return false;
            }
            if(!isoverloaded)
            {
                ok = Value::compare(left, right, &cres);
                #if 0
                fprintf(stderr, "compare: ok=%d cres.result=%g\n", ok, cres.result);
                #endif
                if(ok || (opcode == Instruction::OPCODE_COMPAREEQ))
                {
                    res = Value::makeNumber(cres.result);
                    vmStackPush(res);
                }
                else
                {
                    righttname = Value::getTypename(right.getType());
                    lefttname = Value::getTypename(left.getType());
                    pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "cannot compare %s and %s", lefttname, righttname);
                    return false;
                }
            }
            return true;
        }

        MC_INLINE bool vmDoCompareValGreater(Instruction::Code opcode)
        {
            bool resval;
            NumFloat comparisonres;
            Value res;
            Value value;
            value = vmStackPop();
            comparisonres = value.asNumber();
            resval = false;
            switch(opcode)
            {
                case Instruction::OPCODE_EQUAL:
                    {
                        resval = MC_UTIL_CMPFLOAT(comparisonres, 0);
                    }
                    break;
                case Instruction::OPCODE_NOTEQUAL:
                    {
                        resval = !MC_UTIL_CMPFLOAT(comparisonres, 0);
                    }
                    break;
                case Instruction::OPCODE_GREATERTHAN:
                    {
                        resval = comparisonres > 0;
                    }
                    break;
                case Instruction::OPCODE_GREATERTHANEQUAL:
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
            vmStackPush(res);
            return true;
        }

        MC_INLINE bool vmDoMakeArray()
        {
            int i;
            uint16_t count;
            Value item;
            Value arrayobj;
            Value* items;
            count = m_execstate.currframe->readUint16();
            arrayobj = Value::makeArray(count);
            auto arr = arrayobj.asArray();
            if(arrayobj.isNull())
            {
                return false;
            }
            items = m_execstate.valuestack.data() + m_execstate.vsposition - count;
            for(i = 0; i < count; i++)
            {
                item = items[i];
                arr->push(item);
            }
            setStackPos(m_execstate.vsposition - count);
            vmStackPush(arrayobj);
            return true;
        }

        MC_INLINE bool vmDoMakeMapStart()
        {
            uint16_t count;
            Value mapobj;
            count = m_execstate.currframe->readUint16();
            mapobj = Value::makeMap(count);
            if(mapobj.isNull())
            {
                return false;
            }
            vmStackThisPush(mapobj);
            return true;
        }

        MC_INLINE bool vmDoMakeMapEnd()
        {
            int i;
            uint16_t kvpcount;
            uint16_t itemscount;
            const char* keytypename;
            Value::Type keytype;
            Value mapkey;
            Value mapval;
            Value mapobj;
            Value* kvpairs;
            kvpcount = m_execstate.currframe->readUint16();
            itemscount = kvpcount * 2;
            mapobj = vmStackThisPop();
            kvpairs = m_execstate.valuestack.data() + m_execstate.vsposition - itemscount;
            for(i = 0; i < itemscount; i += 2)
            {
                mapkey = kvpairs[i];
                if(!Value::isHashable(mapkey))
                {
                    keytype = mapkey.getType();
                    keytypename = Value::getTypename(keytype);
                    pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "key of type %s is not hashable", keytypename);
                    return false;
                }
                mapval = kvpairs[i + 1];
                mapobj.asMap()->setKV(mapkey, mapval);
            }
            setStackPos(m_execstate.vsposition - itemscount);
            vmStackPush(mapobj);
            return true;
        }

        void vmGCUnmarkAll()
        {
            size_t i;
            Object* data;
            for(i = 0; i < GCMemory::get()->m_gcobjliststored->count(); i++)
            {
                data = GCMemory::get()->m_gcobjliststored->get(i);
                data->m_gcmark = 0;
            }
        }

        void vmGCSweep()
        {
            size_t i;
            Object* data;
            GCMemory::DataPool* pool;
            auto vd = GCMemory::get()->m_gcobjlistremains.data();
            auto cnt = GCMemory::get()->m_gcobjlistremains.count();
            Value::markListOfValues(static_cast<Value*>(vd), cnt);
            MC_ASSERT(GCMemory::get()->m_gcobjlistback->count() >= GCMemory::get()->m_gcobjliststored->count());
            GCMemory::get()->m_gcobjlistback->clear();
            for(i = 0; i < GCMemory::get()->m_gcobjliststored->count(); i++)
            {
                data = GCMemory::get()->m_gcobjliststored->get(i);
                if(data->m_gcmark != 0)
                {
                    /*
                    * this should never fail because m_gcobjlistback's size should be equal to objects
                    */
                    GCMemory::get()->m_gcobjlistback->push(data);
                }
                else
                {
                    if(Value::canStoreInPool(data))
                    {
                        pool = GCMemory::get()->getPoolForType(data->m_odtype);
                        pool->m_pooldata.set(pool->m_poolitemcount, data);
                        pool->m_poolitemcount++;
                    }
                    else
                    {
                        Value::destroyObjData(data);
                        /*
                        if(GCMemory::get()->m_poolonlydata.m_poolitemcount < GCMemory::MinPoolSize)
                        {
                        */
                            GCMemory::get()->m_poolonlydata.m_pooldata.set(GCMemory::get()->m_poolonlydata.m_poolitemcount, data);
                            GCMemory::get()->m_poolonlydata.m_poolitemcount++;
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
            auto objstemp = GCMemory::get()->m_gcobjliststored;
            GCMemory::get()->m_gcobjliststored = GCMemory::get()->m_gcobjlistback;
            GCMemory::get()->m_gcobjlistback = objstemp;
            GCMemory::get()->m_allocssincesweep = 0;
        }

        int vmGCShouldSweep()
        {
            return static_cast<int>(GCMemory::get()->m_allocssincesweep > GCMemory::SweepInterval);
        }

        bool gcDisableFor(Value obj)
        {
            MC_ASSERT(false && "gcDisableFor() is broken!");
            #if 1
                (void)obj;
            #else
            Object* data;
            if(!obj.isAllocated())
            {
                return false;
            }
            data = obj.getAllocatedData<Object>();
            if(data->m_objmem->m_gcobjlistremains.contains(&obj))
            {
                return false;
            }
            return data->m_objmem->m_gcobjlistremains.push(obj);
            #endif
            return false;
        }

        void gcEnableFor(Value obj)
        {
            MC_ASSERT(false && "gcEnableFor() is broken!");
            #if 1
                (void)obj;
            #else
            Object* data;
            if(obj.isAllocated())
            {
                data = obj.getAllocatedData<Object>();
                data->m_objmem->m_gcobjlistremains.removeItem(&obj);
            }
            #endif
        }

        MC_INLINE void vmGCRunCollector(GenericList<Value>* constants)
        {
            size_t i;
            VMFrame* frame;
            vmGCUnmarkAll();
            Value::markListOfValues(m_vmglobalstore->getData(), m_vmglobalstore->getCount());
            Value::markListOfValues(constants->data(), constants->count());
            Value::markListOfValues(m_globalvalstack.data(), m_globalvalcount);
            for(i = 0; i < m_execstate.framestack.count(); i++)
            {
                frame = m_execstate.framestack.getp(i);
                Value::markObject(frame->m_function);
            }
            Value::markListOfValues(m_execstate.valuestack.data(), m_execstate.vsposition);
            Value::markListOfValues(m_execstate.valthisstack.data(), m_execstate.thisstpos);
            Value::markObject(m_execstate.lastpopped);
            Value::markListOfValues(m_operoverloadkeys, State::MaxOperOverloads);
            vmGCSweep();
        }

    public:
        RuntimeConfig m_config;
        ErrList m_errorlist;
        SymStore* m_vmglobalstore;
        GenericList<Value> m_globalvalstack = GenericList<Value>(MinVMGlobals);
        size_t m_globalvalcount;
        bool m_hadrecovered;
        bool m_running;
        Value m_operoverloadkeys[MaxOperOverloads];
        GenericList<SourceFile*> m_sharedfilelist;
        AstCompiler* m_sharedcompiler;
        ExecInfo m_execstate;
        IOHandle* m_stdoutprinter;
        IOHandle* m_stderrprinter;
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
            GCMemory::create();
            vmInit();
            m_stdoutprinter = Memory::make<IOHandle>(stdout);
            m_stderrprinter = Memory::make<IOHandle>(stderr);
            m_vmglobalstore = Memory::make<SymStore>();
            m_sharedcompiler = Memory::make<AstCompiler>(this, &m_config, GCMemory::get(), &m_errorlist, &m_sharedfilelist, m_vmglobalstore, m_stderrprinter);
            makeDefaultClasses();
        }

        void deinit()
        {
            Memory::destroy(m_sharedcompiler);
            Memory::destroy(m_vmglobalstore);
            GCMemory::destroy();
            m_sharedfilelist.deInit(SourceFile::destroy);
            m_errorlist.destroy();
            IOHandle::destroy(m_stdoutprinter);
            IOHandle::destroy(m_stderrprinter);
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
            vmReset();
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
        void pushError(Error::Type type, SourceLocation pos, const char* fmt, ArgsT&&... args)
        {
            m_errorlist.pushFormat(type, pos, fmt, args...);
        }

        template<typename... ArgsT>
        void pushRuntimeError(const char* fmt, ArgsT&&... args)
        {
            pushError(Error::ERRTYP_RUNTIME, SourceLocation::Invalid(), fmt, args...);
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
            mainfn = Value::makeFuncScript("__main__", comp_res, false, 0, 0, 0);
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
            if(program == nullptr)
            {
                m_errorlist.pushFormat(Error::ERRTYP_USER, SourceLocation::Invalid(), "program passed to execute was null.");
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
            if(res.getType() == Value::VT_NONE)
            {
                return Value::makeNull();
            }
            return res;
        }

        bool setGlobalFunction(const char* name, CallbackNativeFN fn, void* data)
        {
            Value obj;
            obj = Value::makeFuncNative(name, fn, data);
            if(obj.isNull())
            {
                return false;
            }
            return setGlobalValue(name, obj);
        }

        MC_INLINE bool setGlobalValue(const char* name, Value obj)
        {
            return m_vmglobalstore->setNamed(name, obj);
        }

        MC_INLINE Value getGlobalByName(const char* name)
        {
            bool ok;
            Value res;
            AstSymbol* symbol;
            AstSymTable* st;
            st = m_sharedcompiler->getsymtable();
            symbol = st->resolve(name);
            if(symbol == nullptr)
            {
                pushError(Error::ERRTYP_USER, SourceLocation::Invalid(), "symbol \"%s\" is not defined", name);
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
                    pushError(Error::ERRTYP_USER, SourceLocation::Invalid(), "failed to get global object at %d", symbol->m_symindex);
                    return Value::makeNull();
                }
            }
            else
            {
                pushError(Error::ERRTYP_USER, SourceLocation::Invalid(), "value associated with symbol \"%s\" could not be loaded", name);
                return Value::makeNull();
            }
            return res;
        }

        //inserthere
        MC_INLINE void saveExecInfo(ExecInfo* est)
        {
            est->thisstpos = m_execstate.thisstpos;
            est->vsposition = m_execstate.vsposition;
            est->currframe = m_execstate.currframe;
        }

        MC_INLINE void restoreExecInfo(ExecInfo* est)
        {
            m_execstate.thisstpos = est->thisstpos;
            m_execstate.vsposition = est->vsposition;
            m_execstate.currframe = est->currframe;
            est->deInit();
        }

        MC_INLINE bool setGlobalByIndex(size_t ix, Value val)
        {
            m_globalvalstack.set(ix, val);
            if(ix >= m_globalvalcount)
            {
                m_globalvalcount = ix + 1;
            }
            return true;
        }

        MC_INLINE Value getGlobalByIndex(size_t ix)
        {
            return m_globalvalstack.get(ix);
        }

        MC_INLINE void setStackPos(size_t nsp)
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
                for(i=(m_execstate.vsposition - 0); (i != bytescount) && (i < m_execstate.valuestack.capacity()); i++)
                {
                    m_execstate.valuestack.getp(i)->m_valtype = Value::VT_NULL;
                }
            }
            #endif
            m_execstate.vsposition = nsp;
        }

        MC_INLINE void vmStackPush(Value obj)
        {
        #if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
            int numlocals;
            VMFrame* frame;
            if(m_execstate.currframe)
            {
                frame = m_execstate.currframe;
                auto currentfunction = frame->m_function.asFunction();
                numlocals = currentfunction->m_funcdata.valscriptfunc.numlocals;
                MC_ASSERT((size_t)m_execstate.vsposition >= (size_t)(frame->m_basepointer + numlocals));
            }
        #endif
            #if 1
                m_execstate.valuestack.set(m_execstate.vsposition, obj);
            #else
                m_execstate.valuestack.push(obj);
            #endif
            m_execstate.vsposition++;
        }

        MC_INLINE Value vmStackPop()
        {
            Value res;
        #if defined(MC_CONF_DEBUG) && (MC_CONF_DEBUG == 1)
            int numlocals;
            VMFrame* frame;
            if(m_execstate.vsposition == 0)
            {
                m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "stack underflow");
                MC_ASSERT(false);
                return Value::makeNull();
            }
            if(m_execstate.currframe)
            {
                frame = m_execstate.currframe;
                auto currentfunction = frame->m_function.asFunction();
                numlocals = currentfunction->m_funcdata.valscriptfunc.numlocals;
                MC_ASSERT((m_execstate.vsposition - 1) >= (frame->m_basepointer + numlocals));
            }
        #endif
            m_execstate.vsposition--;
            res = m_execstate.valuestack.get(m_execstate.vsposition);
            m_execstate.lastpopped = res;
            return res;
        }

        MC_INLINE Value vmStackGet(size_t nthitem)
        {
            size_t ix;
            ix = m_execstate.vsposition - 1 - nthitem;
            return m_execstate.valuestack.get(ix);
        }

        MC_INLINE void vmStackThisPush(Value obj)
        {
            m_execstate.valthisstack.set(m_execstate.thisstpos, obj);
            m_execstate.thisstpos++;
        }

        MC_INLINE Value vmStackThisPop()
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

        MC_INLINE Value vmStackThisGet(size_t nthitem)
        {
            size_t ix;
            size_t cnt;
            Value val;
            (void)val;
            ix = m_execstate.thisstpos - 1 - nthitem;
            cnt = m_execstate.valthisstack.count();
            if((cnt == 0) || (ix > cnt))
            {
                //val = Value::makeMap();
                //m_execstate.valthisstack.set(ix, val);
                //return Value::makeMap();
                return Value::makeNull();
                //return val;
            }
            return m_execstate.valthisstack.get(ix);
        }

        MC_INLINE bool vmPushFrame(const VMFrame& frame)
        {
            m_execstate.framestack.set(m_execstate.framestack.count(), frame);
            m_execstate.currframe = m_execstate.framestack.getp(m_execstate.framestack.count());
            m_execstate.framestack.push(frame);
            auto framefunction = frame.m_function.asFunction();
            setStackPos(frame.m_basepointer + framefunction->m_funcdata.valscriptfunc.numlocals);
            return true;
        }

        MC_INLINE bool vmPopFrame()
        {
            setStackPos(m_execstate.currframe->m_basepointer - 1);
            if(m_execstate.framestack.count() <= 0)
            {
                MC_ASSERT(false);
                m_execstate.currframe = NULL;
                return false;
            }
            m_execstate.framestack.pop(nullptr);
            if(m_execstate.framestack.count() == 0)
            {
                m_execstate.currframe = NULL;
                return false;
            }
            m_execstate.currframe = m_execstate.framestack.getp(m_execstate.framestack.count() - 1);
            return true;
        }

        bool vmTracebackPush(Traceback* traceback)
        {
            int i;
            VMFrame* frame;
            for(i = m_execstate.framestack.count() - 1; i >= 0; i--)
            {
                frame = m_execstate.framestack.getp(i);
                auto name = frame->m_function.asFunction()->getName();
                traceback->push(name, frame->getPosition());
            }
            return true;
        }

        MC_INLINE Value vmCallNativeFunction(Value callee, SourceLocation srcpos, Value selfval, size_t argc, Value* args)
        {
            Value::Type restype;
            Value res;
            Error* err; 
            Traceback* traceback;
            auto nativefun = callee.asFunction();
            auto uptr = nativefun->m_funcdata.valnativefunc.userpointer;
            res = nativefun->m_funcdata.valnativefunc.natptrfn(this, uptr, selfval, argc, args);
            if(mc_util_unlikely(m_errorlist.count() > 0))
            {
                err = m_errorlist.getLast();
                err->m_pos = srcpos;
                err->m_traceback = Memory::make<Traceback>();
                if(err->m_traceback != nullptr)
                {
                    err->m_traceback->push(nativefun->m_funcdata.valnativefunc.natfnname, SourceLocation::Invalid());
                }
                return Value::makeNull();
            }
            restype = res.getType();
            if(mc_util_unlikely(restype == Value::VT_ERROR))
            {
                traceback = Memory::make<Traceback>();
                if(traceback != nullptr)
                {
                    /* error builtin is treated in a special way */
                    if(!mc_util_strequal(nativefun->m_funcdata.valnativefunc.natfnname, "error"))
                    {
                        traceback->push(nativefun->m_funcdata.valnativefunc.natfnname, SourceLocation::Invalid());
                    }
                    vmTracebackPush(traceback);
                    Value::errorSetTraceback(res, traceback);
                }
            }
            return res;
        }

        MC_INLINE Value vmCallValue(GenericList<Value>* constants, Value callee, Value thisval, size_t argc, Value* args)
        {
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
            if(type == Value::VT_FUNCSCRIPT)
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
                if(!execVM(callee, constants, true))
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
            if(type == Value::VT_FUNCNATIVE)
            {
                return vmCallNativeFunction(callee, SourceLocation::Invalid(), thisval, argc, args);
            }
            m_errorlist.pushFormat(Error::ERRTYP_USER, SourceLocation::Invalid(), "object is not callable");
            return Value::makeNull();
        }

        MC_INLINE bool vmCallObject(Value callee, int nargs)
        {
            const char* calleetypename;
            Value::Type calleetype;
            VMFrame calleeframe;
            Value res;
            Value tmpval;
            Value selfval;
            Value* stackpos;
            calleetype = callee.getType();
            selfval = Value::makeNull();
            if(callee.isFuncNative())
            {
                if(!m_execstate.nativethisstack.pop(&tmpval))
                {
                    #if 0
                        m_stderrprinter->format("failed to pop native 'this' for = <");
                        Value::printValue(m_stderrprinter, callee, true);
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
                Value::printValue(m_stderrprinter, selfval, true);
                m_stderrprinter->format(">>>\n");
            }
            #endif
            if(calleetype == Value::VT_FUNCSCRIPT)
            {
                auto calleefunction = callee.asFunction();
                if(nargs != calleefunction->m_funcdata.valscriptfunc.numargs)
                {
                    #if 0
                    m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "invalid number of arguments to \"%s\": expected %d, got %d",
                                      calleefunction->getName(), calleefunction->m_funcdata.valscriptfunc.numargs, nargs);
                    return false;
                    #endif
                }
                if(!VMFrame::init(&calleeframe, callee, m_execstate.vsposition - nargs))
                {
                    m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, SourceLocation::Invalid(), "frame init failed in vmCallObject");
                    return false;
                }
                if(!vmPushFrame(calleeframe))
                {
                    m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, SourceLocation::Invalid(), "pushing frame failed in vmCallObject");
                    return false;
                }
            }
            else if(calleetype == Value::VT_FUNCNATIVE)
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

        MC_INLINE bool vmTryOverloadOperator(Value left, Value right, Instruction::Code op, bool* outoverloadfound)
        {
            #if 0
                *outoverloadfound = false;
                return true;
            #endif
            int numoper;
            Value key;
            Value callee;
            Value::Type lefttype;
            Value::Type righttype;
            *outoverloadfound = false;
            lefttype = left.getType();
            righttype = right.getType();
            if(lefttype != Value::VT_MAP && righttype != Value::VT_MAP)
            {
                *outoverloadfound = false;
                return true;
            }
            numoper = 2;
            if(op == Instruction::OPCODE_MINUS || op == Instruction::OPCODE_BINNOT || op == Instruction::OPCODE_BANG)
            {
                numoper = 1;
            }
            key = m_operoverloadkeys[(int)op];
            callee = Value::makeNull();
            if(lefttype == Value::VT_MAP)
            {
                auto m = left.asMap();
                callee = m->getValue(key);
            }
            if(!callee.isCallable())
            {
                if(righttype == Value::VT_MAP)
                {
                    auto m = right.asMap();
                    callee = m->getValue(key);
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

        MC_INLINE Value callFunctionByName(const char* fname, Value thisval, size_t argc, Value* args)
        {
            Value res;
            Value callee;
            reset();
            callee = getGlobalByName(fname);
            if(callee.getType() == Value::VT_NULL)
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

        MC_INLINE bool vmCheckAssign(Value oldvalue, Value nvalue)
        {
            return true;
            Value::Type nvaluetype;
            Value::Type oldvaluetype;
            oldvaluetype = oldvalue.getType();
            nvaluetype = nvalue.getType();
            if(oldvaluetype == Value::VT_NULL || nvaluetype == Value::VT_NULL)
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

        MC_INLINE bool vmOpAddString(Value valleft, Value valright, Value::Type righttype, Instruction::Code opcode)
        {
            Value nstring;
            (void)opcode;
            (void)righttype;
            nstring = Value::makeStrEmptyCapacity(0);
            nstring.stringAppendValue(valleft);
            nstring.stringAppendValue(valright);
            vmStackPush(nstring);
            return true;
        }

        MC_INLINE bool vmOpMath(Instruction::Code opcode)
        {
            bool overloadfound;
            NumFloat res;
            NumFloat dnright;
            NumFloat dnleft;
            const char* opcodename;
            const char* lefttypename;
            const char* righttypename;
            Value valright;
            Value valleft;
            Value::Type lefttype;
            Value::Type righttype;
            valright = vmStackPop();
            valleft = vmStackPop();
            lefttype = valleft.getType();
            righttype = valright.getType();
            if(lefttype == Value::VT_STRING && opcode == Instruction::OPCODE_ADD)
            {
                if(vmOpAddString(valleft, valright, righttype, opcode))
                {
                    return true;
                }
            }
            else if((valleft.isNumeric() || valleft.isNull()) && (valright.isNumeric() || valright.isNull()))
            {
                dnright = valright.asNumber();
                dnleft = valleft.asNumber();
                res = 0;
                switch(opcode)
                {
                    case Instruction::OPCODE_ADD:
                        {
                            res = mc_mathutil_add(dnleft, dnright);
                        }
                        break;
                    case Instruction::OPCODE_SUB:
                        {
                            res = mc_mathutil_sub(dnleft, dnright);
                        }
                        break;
                    case Instruction::OPCODE_MUL:
                        {
                            res = mc_mathutil_mult(dnleft, dnright);
                        }
                        break;
                    case Instruction::OPCODE_DIV:
                        {
                            res = mc_mathutil_div(dnleft, dnright);
                        }
                        break;
                    case Instruction::OPCODE_MOD:
                        {
                            res = mc_mathutil_mod(dnleft, dnright);
                        }
                        break;
                    case Instruction::OPCODE_BINOR:
                        {
                            res = mc_mathutil_binor(dnleft, dnright);
                        }
                        break;
                    case Instruction::OPCODE_BINXOR:
                        {
                            res = mc_mathutil_binxor(dnleft, dnright);
                        }
                        break;
                    case Instruction::OPCODE_BINAND:
                        {
                            res = mc_mathutil_binand(dnleft, dnright);
                        }
                        break;
                    /*
                    // TODO: shifting, signedness: how does nodejs do it?
                    // enabling checks for <0 breaks sha1.mc!
                    */
                    case Instruction::OPCODE_LSHIFT:
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
                    case Instruction::OPCODE_RSHIFT:
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
            if(!vmTryOverloadOperator(valleft, valright, opcode, &overloadfound))
            {
                return false;
            }
            if(!overloadfound)
            {
                opcodename = Instruction::opdefGetName(opcode);
                lefttypename = Value::getTypename(lefttype);
                righttypename = Value::getTypename(righttype);
                pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "invalid operand types for %s: got %s and %s",
                                  opcodename, lefttypename, righttypename);
                return false;
            }
            return true;
        }

        MC_INLINE ObjClass* findClassForIntern(Value::Type typ)
        {
            (void)typ;
            switch(typ)
            {
                case Value::VT_NUMBER:
                    {
                        return m_stdobjectnumber;
                    }
                    break;
                case Value::VT_STRING:
                    {
                        return m_stdobjectstring;
                    }
                    break;
                case Value::VT_ARRAY:
                    {
                        return m_stdobjectarray;
                    }
                    break;
                case Value::VT_MAP:
                    {
                        return m_stdobjectmap;
                    }
                    break;
                case Value::VT_FUNCNATIVE:
                case Value::VT_FUNCSCRIPT:
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

        MC_INLINE ObjClass* vmFindClassFor(Value::Type typ)
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
                if(strcmp(memb->m_fieldname, name) == 0)
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

        MC_INLINE bool vmFindClassmemberValue(const Value& left, const Value& index, const Value& setval)
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
                idxname = index.stringGetData();
                vdest = vmGetClassMember(cl, idxname);
                if(vdest == nullptr)
                {
                    return false;
                }
                else
                {
                    fnval = Value::makeFuncNative(vdest->m_fieldname, vdest->m_fndest, nullptr);
                    if(vdest->m_ispseudo)
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

        MC_INLINE bool vmGetIndexPartial(const Value& left, Value::Type lefttype, const Value& index, Value::Type indextype, bool fromdot)
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
            if(lefttype == Value::VT_MAP)
            {
                auto m = left.asMap();
                if(m->getValueChecked(index, &res))
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
                        Value::printValue(m_stderrprinter, left, true);
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
                        pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "object type '%s' has no field '%s'", lefttypename, index.stringGetData());
                        vmStackPush(res);
                        return false;
                    }
                }
            }
            if(lefttype != Value::VT_ARRAY && lefttype != Value::VT_MAP && lefttype != Value::VT_STRING)
            {
                pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "getindexpartial: type %s is not indexable", lefttypename);

                return false;
            }
            res = Value::makeNull();
            if(lefttype == Value::VT_ARRAY)
            {
                if(indextype != Value::VT_NUMBER)
                {
                    lefttypename = Value::getTypename(lefttype);
                    indextypename = Value::getTypename(indextype);
                    pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "cannot get partial index of %s with %s", lefttypename, indextypename);
                    return false;
                }
                auto arr = left.asArray();
                ix = (int)index.asNumber();
                if(ix < 0)
                {
                    ix = arr->size() + ix;
                }
                if(ix >= 0 && ix < (int)arr->size())
                {
                    res = arr->get(ix);
                }
            }
            else if(lefttype == Value::VT_STRING)
            {
                if(indextype != Value::VT_NUMBER)
                {
                    lefttypename = Value::getTypename(lefttype);
                    indextypename = Value::getTypename(indextype);
                    pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "cannot index %s with %s", lefttypename, indextypename);
                    return false;
                }
                str = left.stringGetData();
                leftlen = left.stringGetLength();
                ix = (int)index.asNumber();
                if(ix >= 0 && ix < leftlen)
                {
                    resstr[0] = str[ix];
                    res = Value::makeString(resstr, 1);
                }
            }
            finished:
            vmStackPush(res);
            return true;
        }

        MC_INLINE bool vmGetIndexFull()
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

        MC_INLINE bool vmGetDotIndex()
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

        MC_INLINE bool setIndexPartial(const Value& left, Value::Type lefttype, const Value& index, Value::Type indextype, const Value& nvalue)
        {
            bool ok;
            int alen;
            int ix;
            const char* indextypename;
            const char* lefttypename;
            Value oldvalue;
            if(lefttype != Value::VT_ARRAY && lefttype != Value::VT_MAP)
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
            if(lefttype == Value::VT_ARRAY)
            {
                if(indextype != Value::VT_NUMBER)
                {
                    lefttypename = Value::getTypename(lefttype);
                    indextypename = Value::getTypename(indextype);
                    pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "cannot set index of %s with %s", lefttypename, indextypename);
                    return false;
                }
                ix = (int)index.asNumber();
                auto arr = left.asArray();
                ok = arr->set(ix, nvalue);
                alen = arr->size();
                if(!ok)
                {
                    pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "failed to set array index %d (of %d)", ix, alen);
                    return false;
                }
            }
            else if(lefttype == Value::VT_MAP)
            {
                auto m = left.asMap();
                oldvalue = m->getValue(index);
                if(!vmCheckAssign(oldvalue, nvalue))
                {
                    return false;
                }
                if(!left.asMap()->setKV(index, nvalue))
                {
                    return false;
                }
            }
            return true;
        }

        MC_INLINE bool vmSetIndexFull()
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

        MC_INLINE bool vmGetValueAtFull()
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
            if(lefttype != Value::VT_ARRAY && lefttype != Value::VT_MAP && lefttype != Value::VT_STRING)
            {
                lefttypename = Value::getTypename(lefttype);
                pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "getvalueatfull: type %s is not indexable", lefttypename);
                return false;
            }
            res = Value::makeNull();
            if(indextype != Value::VT_NUMBER)
            {
                lefttypename = Value::getTypename(lefttype);
                indextypename = Value::getTypename(indextype);
                pushError(Error::ERRTYP_RUNTIME, m_execstate.currframe->getPosition(), "cannot get full index %s with %s", lefttypename, indextypename);
                return false;
            }
            ix = (int)index.asNumber();
            if(lefttype == Value::VT_ARRAY)
            {
                auto arr = left.asArray();
                res = arr->get(ix);
            }
            else if(lefttype == Value::VT_MAP)
            {
                auto m = left.asMap();
                res = m->getKVPairAt(ix);
            }
            else if(lefttype == Value::VT_STRING)
            {
                str = left.stringGetData();
                leftlen = left.stringGetLength();
                ix = (int)index.asNumber();
                if(ix >= 0 && ix < leftlen)
                {
                    resstr[0] = str[ix];
                    res = Value::makeString(resstr, 1);
                }
            }
            vmStackPush(res);
            return true;
        }

        MC_INLINE bool hasErrors()
        {
            return errorCount() > 0;
        }

        MC_INLINE int errorCount()
        {
            return m_errorlist.count();
        }

        MC_INLINE void errorsClear()
        {
            m_errorlist.clear();
        }

        MC_INLINE Error* errorGet(int index)
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

        CompiledProgram* compileToProgram(const char* code, const char* filename)
        {
            CompiledProgram* compres;
            errorsClear();
            compres = m_sharedcompiler->compileSource(code, filename);
            if(m_errorlist.count() > 0)
            {
                goto err;
            }
            return compres;
        err:
            CompiledProgram::destroy(compres);
            return nullptr;
        }

        MC_PROTO bool execVM(const Value& function, GenericList<Value>* constants, bool nested);
};

// bottom
GCMemory* GCMemory::m_myself = nullptr;
void GCMemory::wrapDestroyObjData(Object* data)
{
    Value::destroyObjData(data);
}

void Value::ValPrinter::valPrintObjError(IOHandle* pr, const Value& obj)
{
    Error::printUserError(pr, obj);
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
            state->pushError(Error::ERRTYP_RUNTIME, SourceLocation::Invalid(), "Invalid number or arguments, got %d instead of %d", argc, expectedargc);
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
                state->pushError(Error::ERRTYP_RUNTIME, SourceLocation::Invalid(), "Invalid argument %d type, got %s, expected %s", i, typestr, expectedtypestr);
                mc_memory_free(expectedtypestr);
            }
            return false;
        }
    }
    return true;
}

#if (defined(__GNUC__) || defined(__CLANG__) || defined(__TINYC__)) && !defined(__CPPCHECK__)
    #define MC_CONF_USECOMPUTEDGOTOS 1
#else
    #define MC_CONF_USECOMPUTEDGOTOS 0
#endif

#if defined(MC_CONF_USECOMPUTEDGOTOS) && (MC_CONF_USECOMPUTEDGOTOS == 1)
    #define MAKELABEL(opname) LABEL_##opname
    #define mcvm_case(opn) LABEL_##opn
#else
    #define mcvm_case(opn) case Instruction::opn
#endif

#define mc_vmmac_break() \
    goto readnextop

void mc_vmutil_getopinfo(Instruction::Code opc, const char** oname)
{
    Instruction::Definition vdef;
    Instruction::Definition* def;
    *oname = "!invalid!";
    def = Instruction::opdefLookup(&vdef, opc);
    if(def != nullptr)
    {
        *oname = def->name;
    }
}

bool State::execVM(const Value& function, GenericList<Value>* constants, bool nested)
{
    int fri;
    int prevcode;
    OPValCode opcode;
    size_t recoverframeix;
    VMFrame createdframe;
    Value errobj;
    VMFrame* frame;
    Error* err;
    const char* oname;
    Value::ObjFunction* targetfunction;
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
        pushError(Error::ERRTYP_USER, SourceLocation::Invalid(), "state is already executing code");
        return false;
    }
    #endif
    /* naming is hard */
    targetfunction = function.asFunction();
    if(!VMFrame::init(&createdframe, function, m_execstate.vsposition - targetfunction->m_funcdata.valscriptfunc.numargs))
    {
        fprintf(stderr, "failed to init frames!\n");
        return false;
    }
    if(!vmPushFrame(createdframe))
    {
        m_errorlist.pushFormat(Error::ERRTYP_USER, SourceLocation::Invalid(), "pushing frame failed");
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
        opcode = (OPValCode)frame->readOpCode();
        if(mc_util_unlikely(m_config.printinstructions))
        {
            mc_vmutil_getopinfo((Instruction::Code)opcode, &oname);
            fprintf(stderr, "opcode=%d (%s)\n", opcode, oname);
        }
        #if defined(MC_CONF_USECOMPUTEDGOTOS) && (MC_CONF_USECOMPUTEDGOTOS == 1)
            goto* dispatchtable[opcode];
        #else
        switch((Instruction::Code)opcode)
        #endif
        {
            #if !defined(MC_CONF_USECOMPUTEDGOTOS) || (MC_CONF_USECOMPUTEDGOTOS == 0)
            default:
                {
                    const char* prevname;
                    const char* thisname;
                    mc_vmutil_getopinfo((Instruction::Code)opcode, &thisname);
                    mc_vmutil_getopinfo((Instruction::Code)prevcode, &prevname);
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
                    if(!vmPopFrame())
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
                    bool ok;
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
                    if(!vmOpMath((Instruction::Code)opcode))
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
                    if(!vmDoCompareValues((Instruction::Code)opcode))
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
                    if(!vmDoCompareValGreater((Instruction::Code)opcode))
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_MINUS):
                {
                    bool overloadfound;
                    NumFloat val;
                    const char* opertname;
                    Value res;
                    Value::Type opertype;
                    Value operand;
                    operand = vmStackPop();
                    opertype = operand.getType();
                    if(mc_util_likely(opertype == Value::VT_NUMBER))
                    {
                        val = operand.asNumber();
                        res = Value::makeNumber(-val);
                        vmStackPush(res);
                    }
                    else
                    {
                        overloadfound = false;
                        if(!vmTryOverloadOperator(operand, Value::makeNull(), Instruction::OPCODE_MINUS, &overloadfound))
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
                    if(opertype == Value::VT_NUMBER)
                    {
                        val = operand.asNumber();
                        res = Value::makeNumber(~val);
                        vmStackPush(res);
                    }
                    else
                    {
                        overloadfound = false;
                        if(!vmTryOverloadOperator(operand, Value::makeNull(), Instruction::OPCODE_BINNOT, &overloadfound))
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
                    if(type == Value::VT_BOOL)
                    {
                        res = Value::makeBool(!operand.asBool());
                        vmStackPush(res);
                    }
                    else if(type == Value::VT_NULL)
                    {
                        res = Value::makeBool(true);
                        vmStackPush(res);
                    }
                    else
                    {
                        overloadfound = false;
                        if(!vmTryOverloadOperator(operand, Value::makeNull(), Instruction::OPCODE_BANG, &overloadfound))
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
                    if(!test.asBool())
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
                    if(test.asBool())
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
                    if(!vmDoMakeArray())
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_MAPSTART):
                {
                    if(!vmDoMakeMapStart())
                    {
                        goto onexecerror;
                    }
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_MAPEND):
                {
                    if(!vmDoMakeMapEnd())
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
                    if(!vmCallObject(callee, nargs))
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
                        IOHandle* pr = m_stderrprinter;
                        pr->format("GETLOCAL: finalpos=%ld val=<<<", finalpos);
                        Value::printValue(pr, val, true);
                        pr->format(">>>\n");
                    }
                    #endif
                    vmStackPush(val);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_GETGLOBALBUILTIN):
                {
                    bool ok;
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
                    if(!vmDoMakeFunction(constants))
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
                    val = frame->m_function.asFunction()->getFreeValAt(freeix);
                    vmStackPush(val);
                }
                mc_vmmac_break();
            mcvm_case(OPCODE_SETFREE):
                {
                    uint16_t freeix;
                    Value val;
                    freeix = frame->readUint8();
                    val = vmStackPop();
                    frame->m_function.asFunction()->setFreeValAt(freeix, val);
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
                    if(type == Value::VT_ARRAY)
                    {
                        len = val.asArray()->size();
                    }
                    else if(type == Value::VT_MAP)
                    {
                        auto m = val.asMap();
                        len = m->count();
                    }
                    else if(type == Value::VT_STRING)
                    {
                        len = val.stringGetLength();
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
                    NumFloat dval;
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
                errobj = Value::makeError(err->m_message);
                if(!errobj.isNull())
                {
                    Value::errorSetTraceback(errobj, err->m_traceback);
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
        if(vmGCShouldSweep() != 0)
        {
            vmGCRunCollector(constants);
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
    vmGCRunCollector(constants);
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
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_ANY))
    {
        return Value::makeNull();
    }
    arg = args[0];
    type = arg.getType();
    ts = Value::getTypename(type);
    return Value::makeString(ts);
}

Value mc_scriptfn_arrayfirst(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    (void)state;
    (void)data;
    (void)thisval;
    (void)argc;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_ARRAY))
    {
        return Value::makeNull();
    }
    auto arr = args[0].asArray();
    return arr->get(0);
}

Value mc_scriptfn_arraylast(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int len;
    (void)state;
    (void)argc;
    (void)thisval;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_ARRAY))
    {
        return Value::makeNull();
    }
    auto arr = args[0].asArray();
    len = arr->size();
    return arr->get(len - 1);
}

Value mc_scriptfn_arrayrest(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int i;
    int len;
    Value res;
    Value item;
    (void)state;
    (void)argc;
    (void)thisval;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_ARRAY))
    {
        return Value::makeNull();
    }
    auto arg = args[0].asArray();
    len = arg->size();
    if(len == 0)
    {
        return Value::makeNull();
    }
    res = Value::makeArray();
    auto arr = res.asArray();
    for(i = 1; i < len; i++)
    {
        item = arr->get(i);
        arr->push(item);
    }
    return res;
}

Value mc_scriptfn_reverse(State* state, void* data, Value thisval, size_t argc, Value* args)
{
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
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_ARRAY | Value::VT_STRING))
    {
        return Value::makeNull();
    }
    arg = args[0];
    type = arg.getType();
    if(type == Value::VT_ARRAY)
    {
        inplen = arg.asArray()->size();
        res = Value::makeArray(inplen);
        auto arr = res.asArray();
        for(i = 0; i < inplen; i++)
        {
            obj = arr->get(i);
            if(!arr->set(inplen - i - 1, obj))
            {
                return Value::makeNull();
            }
        }
        return res;
    }
    if(type == Value::VT_STRING)
    {
        inpstr = arg.stringGetData();
        inplen = arg.stringGetLength();
        res = Value::makeStrEmptyCapacity(inplen);
        if(res.isNull())
        {
            return Value::makeNull();
        }
        resbuf = res.stringGetMutableData();
        for(i = 0; i < inplen; i++)
        {
            resbuf[inplen - i - 1] = inpstr[i];
        }
        resbuf[inplen] = '\0';
        res.stringSetLength(inplen);
        return res;
    }
    return Value::makeNull();
}

Value mc_scriptfn_makearray(State* state, void* data, Value thisval, size_t argc, Value* args)
{
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
        if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER))
        {
            return Value::makeNull();
        }
        capacity = (int)args[0].asNumber();
        res = Value::makeArray(capacity);
        auto arr = res.asArray();
        objnull = Value::makeNull();
        for(i = 0; i < capacity; i++)
        {
            if(!arr->push(objnull))
            {
                return Value::makeNull();
            }
        }
        return res;
    }
    if(argc == 2)
    {
        if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER, Value::VT_ANY))
        {
            return Value::makeNull();
        }
        capacity = (int)args[0].asNumber();
        res = Value::makeArray(capacity);
        auto arr = res.asArray();
        for(i = 0; i < capacity; i++)
        {
            if(!arr->push(args[1]))
            {
                return Value::makeNull();
            }
        }
        return res;
    }
    mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER);
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
    NumFloat a_x;
    NumFloat a_y;
    NumFloat b_x;
    NumFloat b_y;
    Value res;
    Value keyx;
    Value keyy;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    if (!mc_argcheck_check(state, true, argc, args, Value::VT_MAP, Value::VT_MAP))
    {
        return Value::makeNull();
    }
    keyx = Value::makeString("x");
    keyy = Value::makeString("y");
    auto m1 = args[0].asMap();
    auto m2 = args[1].asMap();
    a_x = m1->getValue(keyx).asNumber();
    a_y = m1->getValue(keyy).asNumber();
    b_x = m2->getValue(keyx).asNumber();
    b_y = m2->getValue(keyy).asNumber();
    res = Value::makeMap();
    auto m = res.asMap();
    m->setKV(keyx, Value::makeNumber(a_x + b_x));
    m->setKV(keyy, Value::makeNumber(a_y + b_y));
    return res;
}

Value mc_scriptfn_vec2sub(State *state, void *data, Value thisval, size_t argc, Value *args)
{
    NumFloat a_x;
    NumFloat a_y;
    NumFloat b_y;
    NumFloat b_x;
    Value res;
    Value keyx;
    Value keyy;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    if (!mc_argcheck_check(state, true, argc, args, Value::VT_MAP, Value::VT_MAP))
    {
        return Value::makeNull();
    }
    keyx = Value::makeString("x");
    keyy = Value::makeString("y");
    auto m1 = args[0].asMap();
    auto m2 = args[1].asMap();
    a_x = m1->getValue(keyx).asNumber();
    a_y = m1->getValue(keyy).asNumber();
    b_x = m2->getValue(keyx).asNumber();
    b_y = m2->getValue(keyy).asNumber();
    res = Value::makeMap();
    auto m = res.asMap();
    m->setKV(keyx, Value::makeNumber(a_x - b_x));
    m->setKV(keyy, Value::makeNumber(a_y - b_y));
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
                  Value::VT_NUMBER,
                  Value::VT_ARRAY | Value::VT_MAP,
                  Value::VT_MAP,
                  Value::VT_STRING,
                  Value::VT_NUMBER | Value::VT_BOOL,
                  Value::VT_FUNCSCRIPT | Value::VT_FUNCNATIVE,
                  Value::VT_ANY))
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
    Value mapkeystr;
    Value mapvalstr;
    const char *tname;
    char keybuf[64];
    (void)data;
    (void)thisval;
    if (argc != 1)
    {
        state->pushRuntimeError("invalid type passed to maketestdict, got %d, expected 1", argc);
        return Value::makeNull();
    }    
    if (args[0].getType() != Value::VT_NUMBER)
    {
        tname = Value::getTypename(args[0].getType());
        state->pushRuntimeError("invalid type passed to maketestdict, got %s", tname);
        return Value::makeNull();
    }
    numitems = args[0].asNumber();
    res = Value::makeMap();
    auto m = res.asMap();
    for (i = 0; i < numitems; i++)
    {
        blen = sprintf(keybuf, "%d", i);
        mapkeystr = Value::makeString(keybuf, blen);
        mapvalstr = Value::makeNumber(i);
        m->setKV(mapkeystr, mapvalstr);
    }
    return res;
}

Value mc_scriptfn_squarearray(State *state, void *data, Value thisval, size_t argc, Value *args)
{
    size_t i;
    NumFloat num;
    Value res;
    Value resitem;    
    (void)data;
    (void)thisval;
    res = Value::makeArray(argc);
    auto arr = res.asArray();
    for(i = 0; i < argc; i++)
    {
        if(args[i].getType() != Value::VT_NUMBER)
        {
            state->pushRuntimeError("invalid type passed to squarearray");
            return Value::makeNull();
        }
        num = args[i].asNumber();
        resitem = Value::makeNumber(num * num);
        arr->push(resitem);
    }
    return res;
}

Value mc_scriptfn_print(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t i;
    Value arg;
    IOHandle* pr;
    (void)data;
    (void)thisval;
    (void)thisval;
    pr = state->m_stdoutprinter;
    for(i = 0; i < argc; i++)
    {
        arg = args[i];
        Value::printValue(pr, arg, false);
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
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    arg = args[0];
    IOHandle pr;
    Value::printValue(&pr, arg, false);
    if(pr.m_prfailed)
    {
        pr.release(false);
        return Value::makeNull();
    }
    resstr = pr.getString();
    reslen = pr.getLength();
    res = Value::makeString(resstr, reslen);
    pr.release(false);
    return res;
}

Value mc_nsfnjson_stringify(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int reslen;
    const char* resstr;
    Value arg;
    Value res;
    IOHandle pr;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    arg = args[0];
    pr.m_prconfig.quotstring = true;
    Value::printValue(&pr, arg, false);
    if(pr.m_prfailed)
    {
        pr.release(false);
        return Value::makeNull();
    }
    resstr = pr.getString();
    reslen = pr.getLength();
    res = Value::makeString(resstr, reslen);
    pr.release(false);
    return res;
}

Value mc_objfnnumber_chr(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    NumFloat val;
    char c;
    (void)state;
    (void)argc;
    (void)data;
    (void)args;
    val = thisval.asNumber();
    c = (char)val;
    return Value::makeString(&c, 1);
}
 
Value mc_objfnstring_length(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t len;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    len = thisval.stringGetLength();
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
    if(searchtype == Value::VT_NULL)
    {
        return Value::makeNumber(-1);
    }
    searchstr = nullptr;
    searchlen = 0;
    inpstr = thisval.stringGetData();
    inplen = thisval.stringGetLength();
    MC_ASSERT((searchtype == Value::VT_NUMBER) || (searchtype == Value::VT_STRING));
    if(searchtype == Value::VT_NUMBER)
    {
        tmpch = searchval.asNumber();
        inpstr = &tmpch;
        inplen = 1;
    }
    else if(searchtype == Value::VT_STRING)
    {
        searchstr = searchval.stringGetData();
        searchlen = searchval.stringGetLength();
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
    str = sval.stringGetData();
    len = sval.stringGetLength();
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
    str = sval.stringGetData();
    len = sval.stringGetLength();
    idx = args[0].asNumber();
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
    str = sval.stringGetData();
    len = sval.stringGetLength();
    idx = args[0].asNumber();
    if(idx >= (long)len)
    {
        return Value::makeNull();
    }
    ch = str[idx];
    return Value::makeString(&ch, 1);
}

Value mc_objfnstring_getself(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    const char* str;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    str = thisval.stringGetData();
    fprintf(stderr, "objfnstring_getself: str=\"%s\"\n", str);
    return thisval;
}

Value mc_objfnstring_tonumber(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int stringlen;
    int parsedlen;
    NumFloat result;
    char* end;
    const char* string;
    (void)state;
    (void)argc;
    (void)data;
    (void)args;
    result = 0;
    string = "";
    stringlen = thisval.stringGetLength();
    string = thisval.stringGetData();
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
    state->pushError(Error::ERRTYP_RUNTIME, SourceLocation::Invalid(), "cannot convert \"%s\" to number", string);
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
    (void)state;
    if(argc > 0 && args[0].getType() == Value::VT_NUMBER)
    {
        inpval = thisval;
        posval = args[0];
        inpstr = inpval.stringGetData();
        inplen = inpval.stringGetLength();
        startpos = posval.asNumber();
        /*
        * If the requested startpos is longer than the string then return a new string
        * of the full length.
        */
        if(startpos > (int)inplen)
        {
            return Value::makeString(inpstr, inplen);
        }
        result = (char*)mc_memory_malloc(startpos + 1);
        if(result == nullptr)
        {
            return Value::makeNull();
        }
        strncpy(result, inpstr, startpos);
        result[startpos] = '\0';
        obj = Value::makeString(result, startpos);
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
    (void)state;
    (void)data;
    (void)thisval;
    if(argc > 0 && args[1].getType() == Value::VT_NUMBER)
    {
        inpval = thisval;
        idxval = args[0];
        inpstr = inpval.stringGetData();
        inplen = inpval.stringGetLength();
        startpos = idxval.asNumber();
        /*
        * If the requested startpos is longer than the string then return a new string
        * of the full length.
        */
        if(startpos >= inplen)
        {
            return Value::makeString(inpstr, inplen);
        }
        result = (char*)mc_memory_malloc(startpos + 1);
        if(result == nullptr)
        {
            return Value::makeNull();
        }
        strlength = inplen;
        strncpy(result, inpstr + strlength - startpos, startpos);
        result[startpos] = '\0';
        obj = Value::makeString(result, startpos);
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
    (void)state;
    (void)data;
    (void)argc;
    if(args[0].getType() == Value::VT_STRING && args[1].getType() == Value::VT_STRING)
    {
        inpval = thisval;
        searchval = args[0];
        repval = args[1];
        inpstr = inpval.stringGetData();
        searchstr = searchval.stringGetData();
        replacestr = repval.stringGetData();
        inplen = inpval.stringGetLength();
        searchlen = searchval.stringGetLength();
        replacelen = repval.stringGetLength();
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
        obj = Value::makeString(result);
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
    (void)state;
    (void)data;
    (void)argc;
    if(args[0].getType() == Value::VT_STRING && args[1].getType() == Value::VT_STRING)
    {
        inpval = thisval;
        searchval = args[0];
        repval = args[1];
        inpstr = inpval.stringGetData();
        searchstr = searchval.stringGetData();
        replacestr = repval.stringGetData();
        inplen = inpval.stringGetLength();
        searchlen = searchval.stringGetLength();
        replacelen = repval.stringGetLength();
        temp = strstr(inpstr, searchstr);
        if(temp == nullptr)
        {
            return Value::makeString(inpstr, inplen);
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
        obj = Value::makeString(result, len);
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
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    inpval = thisval;
    inpstr = inpval.stringGetData();
    inplen = inpval.stringGetLength();
    if(inplen == 0)
    {
        return Value::makeString("", 0);
    }
    result = (char*)mc_memory_malloc(inplen + 1);
    if(result == nullptr)
    {
        return Value::makeString("", 0);
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
    obj = Value::makeString(result, k);
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
    inpstr = inpval.stringGetData();
    inplen = inpval.stringGetLength();
    patstr = patval.stringGetData();
    patlen = patval.stringGetLength();
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
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    inpval = thisval;
    inpstr = inpval.stringGetData();
    inplen = inpval.stringGetLength();
    resstr = Value::makeString(inpstr, inplen);
    resstr.getAllocatedData<Value::ObjString>()->m_strbuf->toLowercase();
    return resstr;
}

Value mc_objfnstring_toupper(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int inplen;
    const char* inpstr;
    Value resstr;
    Value inpval;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    inpval = thisval;
    inpstr = inpval.stringGetData();
    inplen = inpval.stringGetLength();
    resstr = Value::makeString(inpstr, inplen);
    resstr.getAllocatedData<Value::ObjString>()->m_strbuf->toUppercase();

    return resstr;    
}

Value mc_objfnstring_split(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t i;
    (void)state;
    (void)data;
    (void)thisval;
    (void)argc;
    (void)args;
    GenericList<StrView> spvals;
    auto inpval = thisval;
    auto delimval = args[0];
    auto inpstr = inpval.stringGetData();
    auto inplen = inpval.stringGetLength();
    if(inplen == 0)
    {
        return Value::makeNull();
    }
    auto delimstr = delimval.stringGetData();
    auto delimlen = delimval.stringGetLength();
    auto arr = Memory::make<GenericList<Value>>();
    if((delimstr == nullptr) || (delimlen == 0))
    {
        return Value::makeArrayFromList(arr);
    }
    //bool mc_util_splitstring(GenericList<char*>* dest, const char* str, const char* delimiter)
    if(!mc_util_splitstring(&spvals, inpstr, delimstr))
    {
        spvals.deInit();
        return Value::makeNull();
    }
    for(i=0; i<spvals.count(); i++)
    {
        auto spstr = spvals.getp(i);
        auto resstr = Value::makeString(spstr->data(), spstr->length());
        arr->push(resstr);
        spstr->deAlloc();
    }
    spvals.deInit();
    return Value::makeArrayFromList(arr);
}

Value mc_objfnarray_length(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t len;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    len = thisval.asArray()->size();
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
    narr = Value::makeArray();
    auto nary = narr.asArray();
    auto ary = thisval.asArray();
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
    size_t i;
    size_t len;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_ARRAY, Value::VT_ANY))
    {
        return Value::makeNull();
    }
    auto arr = thisval.asArray();
    for(i=0; i<argc; i++)
    {
        if(!arr->push(args[i]))
        {
            return Value::makeNull();
        }
    }
    len = arr->size();
    return Value::makeNumber(len);
}

Value mc_objfnarray_pop(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    Value val;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_ARRAY, Value::VT_ANY))
    {
        return Value::makeNull();
    }
    val = Value::arrayPop(thisval);
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
    Value joinee;
    IOHandle pr;
    (void)state;
    (void)thisval;
    (void)argc;
    (void)data;
    havejoinee = false;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_ARRAY, Value::VT_ANY))
    {
        return Value::makeNull();
    }
    auto array = thisval.asArray();
    if(argc > 0)
    {
        havejoinee = true;
        joinee = args[0];
    }
    alen = array->size();
    for(i=0; i<alen; i++)
    {
        item = array->get(i);
        Value::printValue(&pr, item, false);
        if(havejoinee)
        {
            if((i + 1) != alen)
            {
                Value::printValue(&pr, joinee, false);
            }
        }
    }
    str = pr.m_prdestbuf->data();
    slen = pr.m_prdestbuf->length();
    rt = Value::makeString(str, slen);
    pr.release(false);
    return rt;
}

Value mc_objfnmap_length(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t len;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    auto m = thisval.asMap();
    len = m->count();
    return Value::makeNumber(len);
}

Value mc_objfnmap_keys(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int i;
    int len;
    Value strval;
    (void)state;
    (void)data;
    (void)argc;
    (void)args;
    auto map = thisval.asMap();
    len = map->count();
    auto aval = Value::makeArray();
    auto arr = aval.asArray();
    for(i=0; i<len; i++)
    {
        strval = map->getKeyAt(i);
        arr->push(strval);
    }
    return aval;
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
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VT_STRING);
}

Value mc_objfnobject_isarray(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VT_ARRAY);
}

Value mc_objfnobject_ismap(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VT_MAP);
}

Value mc_objfnobject_isnumber(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VT_NUMBER);
}

Value mc_objfnobject_isbool(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VT_BOOL);
}

Value mc_objfnobject_isnull(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VT_NULL);
}

Value mc_objfnobject_isfuncscript(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VT_FUNCSCRIPT);
}

Value mc_objfnobject_isexternal(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VT_EXTERNAL);
}

Value mc_objfnobject_iserror(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VT_ERROR);
}

Value mc_objfnobject_isfuncnative(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    return mc_objfnutil_istype(state, data, thisval, argc, args, Value::VT_FUNCNATIVE);
}

void State::makeDefaultClasses()
{
    {
        m_stdobjectobject = Memory::make<ObjClass>("Object", nullptr);
        m_stdobjectobject->addMember("isString", mc_objfnobject_isstring);
        m_stdobjectobject->addMember("isNumber", mc_objfnobject_isnumber);
        m_stdobjectobject->addMember("isArray", mc_objfnobject_isarray);
        m_stdobjectobject->addMember("isMap", mc_objfnobject_ismap);
        m_stdobjectobject->addMember("isFuncNative", mc_objfnobject_isfuncnative);
        m_stdobjectobject->addMember("isFuncScript", mc_objfnobject_isfuncscript);
        m_stdobjectobject->addMember("isExternal", mc_objfnobject_isexternal);
        m_stdobjectobject->addMember("isError", mc_objfnobject_iserror);
        m_stdobjectobject->addMember("isNull", mc_objfnobject_isnull);
        m_stdobjectobject->addMember("isBool", mc_objfnobject_isbool);
        m_stdobjectobject->addMember("isCallable", mc_objfnobject_iscallable);

    }
    {
        m_stdobjectnumber = Memory::make<ObjClass>("Number", m_stdobjectobject);
        m_stdobjectnumber->addMember("chr", mc_objfnnumber_chr);
        
    }
    {
        m_stdobjectstring = Memory::make<ObjClass>("String", m_stdobjectobject);
        m_stdobjectstring->addPseudo("length", mc_objfnstring_length);
        m_stdobjectstring->addMember("getself", mc_objfnstring_getself);
        m_stdobjectstring->addMember("toNumber", mc_objfnstring_tonumber);
        m_stdobjectstring->addMember("ord", mc_objfnstring_charcodefirst);
        m_stdobjectstring->addMember("charCodeAt", mc_objfnstring_charcodeat);
        m_stdobjectstring->addMember("charAt", mc_objfnstring_charat);
        m_stdobjectstring->addMember("indexOf", mc_objfnstring_indexof);
        m_stdobjectstring->addMember("left", mc_objfnstring_left);
        m_stdobjectstring->addMember("right", mc_objfnstring_right);
        m_stdobjectstring->addMember("replace", mc_objfnstring_replaceall);
        m_stdobjectstring->addMember("replacefirst", mc_objfnstring_replacefirst);
        m_stdobjectstring->addMember("match", mc_objfnstring_matchglobcase);
        m_stdobjectstring->addMember("imatch", mc_objfnstring_matchglobicase);
        m_stdobjectstring->addMember("trim", mc_objfnstring_trim);
        m_stdobjectstring->addMember("toLower", mc_objfnstring_tolower);
        m_stdobjectstring->addMember("toUpper", mc_objfnstring_toupper);
        m_stdobjectstring->addMember("split", mc_objfnstring_split);

    }
    {
        m_stdobjectarray = Memory::make<ObjClass>("Array", m_stdobjectobject);
        m_stdobjectarray->addPseudo("length", mc_objfnarray_length);
        m_stdobjectarray->addMember("push", mc_objfnarray_push);
        m_stdobjectarray->addMember("pop", mc_objfnarray_pop);
        m_stdobjectarray->addMember("join", mc_objfnarray_join);
        m_stdobjectarray->addMember("map", mc_objfnarray_map);
    }
    {
        m_stdobjectmap = Memory::make<ObjClass>("Map", m_stdobjectobject);
        m_stdobjectmap->addPseudo("length", mc_objfnmap_length);
        m_stdobjectmap->addMember("keys", mc_objfnmap_keys);
    }
    {
        m_stdobjectfunction = Memory::make<ObjClass>("Function", m_stdobjectobject);
    }
}

Value mc_scriptfn_isnan(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    NumFloat val;
    bool b;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    val = args[0].asNumber();
    b = false;
    if(val != val)
    {
        b = true;
    }
    return Value::makeBool(b);
}

Value mc_scriptfn_range(State* state, void* data, Value thisval, size_t argc, Value* args)
{
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
        if(type != Value::VT_NUMBER)
        {
            typestr = Value::getTypename(type);
            expectedstr = Value::getTypename(Value::VT_NUMBER);
            state->pushError(Error::ERRTYP_RUNTIME, SourceLocation::Invalid(), "invalid argument %d passed to range, got %s instead of %s", ai, typestr, expectedstr);
            return Value::makeNull();
        }
    }
    start = 0;
    end = 0;
    step = 1;
    if(argc == 1)
    {
        end = (int)args[0].asNumber();
    }
    else if(argc == 2)
    {
        start = (int)args[0].asNumber();
        end = (int)args[1].asNumber();
    }
    else if(argc == 3)
    {
        start = (int)args[0].asNumber();
        end = (int)args[1].asNumber();
        step = (int)args[2].asNumber();
    }
    else
    {
        state->pushError(Error::ERRTYP_RUNTIME, SourceLocation::Invalid(), "invalid number of arguments passed to range, got %d", argc);
        return Value::makeNull();
    }
    if(step == 0)
    {
        state->m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, SourceLocation::Invalid(), "range step cannot be 0");
        return Value::makeNull();
    }
    res = Value::makeArray();
    auto arr = res.asArray();
    if(res.isNull())
    {
        return Value::makeNull();
    }
    for(i = start; i < end; i += step)
    {
        item = Value::makeNumber(i);
        if(!arr->push(item))
        {
            return Value::makeNull();
        }
    }
    return res;
}

Value mc_scriptfn_keys(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int i;
    int len;
    Value arg;
    Value res;
    Value mapkey;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_MAP))
    {
        return Value::makeNull();
    }
    arg = args[0];
    res = Value::makeArray();
    auto arr = res.asArray();
    auto m = arg.asMap();
    len = m->count();
    for(i = 0; i < len; i++)
    {
        mapkey = m->getKeyAt(i);
        if(!arr->push(mapkey))
        {
            return Value::makeNull();
        }
    }
    return res;
}

Value mc_scriptfn_values(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int i;
    int len;
    Value mapval;
    Value arg;
    Value res;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_MAP))
    {
        return Value::makeNull();
    }
    arg = args[0];
    res = Value::makeArray();
    auto arr = res.asArray();
    auto m = arg.asMap();
    len = m->count();
    for(i = 0; i < len; i++)
    {
        mapval = m->getValueAt(i);
        if(!arr->push(mapval))
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
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_ANY))
    {
        return Value::makeNull();
    }
    return Value::copyFlat(args[0]);
}

Value mc_scriptfn_copydeep(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_ANY))
    {
        return Value::makeNull();
    }
    return Value::copyDeep(args[0]);
}

Value mc_scriptfn_remove(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    bool res;
    int ix;
    size_t i;
    Value obj;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_ARRAY, Value::VT_ANY))
    {
        return Value::makeNull();
    }
    ix = -1;
    auto arr = args[0].asArray();;
    for(i = 0; i < arr->size(); i++)
    {
        obj = arr->get(i);
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
    res = Value::arrayRemoveAt(args[0], ix);
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
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_ARRAY, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    type= args[0].getType();
    ix = (int)args[1].asNumber();
    switch(type)
    {
        case Value::VT_ARRAY:
            {
                res = Value::arrayRemoveAt(args[0], ix);
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
    (void)state;
    (void)data;
    (void)thisval;
    if(argc == 1 && args[0].getType() == Value::VT_STRING)
    {
        return Value::makeError(args[0].stringGetData());
    }
    return Value::makeError("");
}

Value mc_scriptfn_crash(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    (void)data;
    (void)thisval;
    if(argc == 1 && args[0].getType() == Value::VT_STRING)
    {
        state->m_errorlist.pushMessage(Error::ERRTYP_RUNTIME, state->m_execstate.currframe->getPosition(), args[0].stringGetData());
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
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_BOOL))
    {
        return Value::makeNull();
    }
    if(!args[0].asBool())
    {
        state->m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, SourceLocation::Invalid(), "assertion failed");
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
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    seed = (int)args[0].asNumber();
    srand(seed);
    return Value::makeBool(true);
}

Value mc_scriptfn_random(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    NumFloat min;
    NumFloat max;
    NumFloat res;
    NumFloat range;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    res = (NumFloat)rand() / RAND_MAX;
    if(argc == 0)
    {
        return Value::makeNumber(res);
    }
    if(argc == 2)
    {
        if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER, Value::VT_NUMBER))
        {
            return Value::makeNull();
        }
        min = args[0].asNumber();
        max = args[1].asNumber();
        if(min >= max)
        {
            state->m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, SourceLocation::Invalid(), "max is bigger than min");
            return Value::makeNull();
        }
        range = max - min;
        res = min + (res * range);
        return Value::makeNumber(res);
    }
    state->m_errorlist.pushFormat(Error::ERRTYP_RUNTIME, SourceLocation::Invalid(), "invalid number or arguments");
    return Value::makeNull();
}

Value mc_scriptutil_slicearray(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    int i;
    int len;
    int index;
    Value res;
    Value item;
    (void)state;
    (void)data;
    (void)argc;
    (void)thisval;
    index = (int)args[1].asNumber();
    len = args[0].asArray()->size();
    if(index < 0)
    {
        index = len + index;
        if(index < 0)
        {
            index = 0;
        }
    }
    res = Value::makeArray(len - index);
    auto arr = res.asArray();
    if(res.isNull())
    {
        return Value::makeNull();
    }
    for(i = index; i < len; i++)
    {
        item = arr->get(i);
        if(!arr->push(item))
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
    (void)state;
    (void)data;
    (void)argc;
    (void)thisval;
    index = (int)args[1].asNumber();
    str = args[0].stringGetData();
    len = args[0].stringGetLength();
    if(index < 0)
    {
        index = len + index;
        if(index < 0)
        {
            return Value::makeString("", 0);
        }
    }
    if(index >= len)
    {
        return Value::makeString("", 0);
    }
    res = Value::makeStrEmptyCapacity(10);
    if(res.isNull())
    {
        return Value::makeNull();
    }
    for(i = index; i < len; i++)
    {
        c = str[i];
        res.stringAppend(&c, 1);
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
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_STRING | Value::VT_ARRAY, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    argtype = args[0].getType();
    if(argtype == Value::VT_ARRAY)
    {
        return mc_scriptutil_slicearray(state, data, thisval, argc, args);
    }
    if(argtype == Value::VT_STRING)
    {
        return mc_scriptutil_slicestring(state, data, thisval, argc, args);
    }
    typestr = Value::getTypename(argtype);
    state->pushError(Error::ERRTYP_RUNTIME, SourceLocation::Invalid(), "invalid argument 0 passed to slice, got %s instead", typestr);
    return Value::makeNull();
}

Value mc_nsfnmath_sqrt(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    NumFloat arg;
    NumFloat res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    arg = args[0].asNumber();
    res = sqrt(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_pow(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    NumFloat arg1;
    NumFloat arg2;
    NumFloat res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    arg1 = args[0].asNumber();
    arg2 = args[1].asNumber();
    res = pow(arg1, arg2);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_sin(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    NumFloat arg;
    NumFloat res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    arg = args[0].asNumber();
    res = sin(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_cos(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    NumFloat arg;
    NumFloat res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    arg = args[0].asNumber();
    res = cos(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_tan(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    NumFloat arg;
    NumFloat res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    arg = args[0].asNumber();
    res = tan(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_log(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    NumFloat arg;
    NumFloat res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    arg = args[0].asNumber();
    res = log(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_ceil(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    NumFloat arg;
    NumFloat res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    arg = args[0].asNumber();
    res = ceil(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_floor(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    NumFloat arg;
    NumFloat res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    arg = args[0].asNumber();
    res = floor(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_abs(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    NumFloat arg;
    NumFloat res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    arg = args[0].asNumber();
    res = fabs(arg);
    return Value::makeNumber(res);
}

Value mc_nsfnmath_hypot(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    NumFloat arg1;
    NumFloat arg2;
    NumFloat res;
    (void)data;
    (void)state;
    (void)argc;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_NUMBER, Value::VT_NUMBER))
    {
        return Value::makeNull();
    }
    arg1 = args[0].asNumber();
    arg2 = args[1].asNumber();
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
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_STRING, Value::VT_STRING))
    {
        return Value::makeNull();
    }
    path = args[0].stringGetData();
    string = args[1].stringGetData();
    slen = args[1].stringGetLength();
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
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_STRING))
    {
        return Value::makeNull();
    }
    path = args[0].stringGetData();
    contents = mc_fsutil_fileread(path, &flen);
    if(contents == nullptr)
    {
        return Value::makeNull();
    }
    res = Value::makeString(contents, flen);
    mc_memory_free(contents);
    contents = nullptr;
    return res;
}

Value mc_nsfnfile_join(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    size_t i;
    Value res;
    Value arg;
    (void)state;
    (void)data;
    (void)thisval;
    res = Value::makeString("", 0);
    for(i=0; i<argc; i++)
    {
        arg = args[i];
        res.stringAppendValue(arg);
        if((i + 1) < argc)
        {
            res.stringAppend("/", 1);
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
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_STRING))
    {
        return Value::makeNull();
    }
    path = args[0].stringGetData();
    return Value::makeBool(osfn_pathisdirectory(path));
}

Value mc_nsfnfile_isfile(State* state, void* data, Value thisval, size_t argc, Value* args)
{
    const char* path;
    (void)state;
    (void)argc;
    (void)data;
    (void)thisval;
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_STRING))
    {
        return Value::makeNull();
    }
    path = args[0].stringGetData();
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
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_STRING))
    {
        return Value::makeNull();
    }
    path = args[0].stringGetData();
    if(stat(path, &st) == 0)
    {
        resmap = Value::makeMap();
        auto m = resmap.asMap();
        fullpath = osfn_realpath(path, fpbuffer);
        m->setValString("dev", Value::makeNumber(st.st_dev));
        m->setValString("ino", Value::makeNumber(st.st_ino));
        m->setValString("mode", Value::makeNumber(st.st_mode));
        m->setValString("nlink", Value::makeNumber(st.st_nlink));
        m->setValString("uid", Value::makeNumber(st.st_uid));
        m->setValString("gid", Value::makeNumber(st.st_gid));
        m->setValString("size", Value::makeNumber(st.st_size));
        m->setValString("path", Value::makeString(fullpath));
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
    if(!mc_argcheck_check(state, true, argc, args, Value::VT_STRING, Value::VT_BOOL))
    {
        return Value::makeNull();
    }
    vpath = args[0];
    path = vpath.stringGetData();
    if(argc > 1)
    {
        joinpaths = args[1].asBool();
    }
    if(reader.openDir(path))
    {
        res = Value::makeArray();
        auto arr = res.asArray();
        while(reader.readItem(&ent))
        {
            isdot = ((strcmp(ent.name, ".") == 0) || (strcmp(ent.name, "..") == 0));
            if(isdot)
            {
                continue;
            }
            vjustname = Value::makeString(ent.name, strlen(ent.name));
            vrespath = vjustname;
            if(joinpaths)
            {
                vfullpath = Value::makeString("", 0);
                vfullpath.stringAppendValue(vpath);
                vfullpath.stringAppend("/", 1);
                vfullpath.stringAppendValue(vjustname);
                vrespath = vfullpath;
            }
            arr->push(vrespath);
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
        CallbackNativeFN fn;
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
    jmap = Value::makeMap();
    auto m = jmap.asMap();
    m->setStrFunc("stringify", mc_nsfnjson_stringify);
    state->setGlobalValue("JSON", jmap);
}

void mc_cli_installjsconsole(State* state)
{
    Value jmap;
    jmap = Value::makeMap();
    auto m = jmap.asMap();
    m->setStrFunc("log", mc_scriptfn_println);
    state->setGlobalValue("console", jmap);
}

void mc_cli_installmath(State* state)
{
    Value jmap;
    jmap = Value::makeMap();
    auto m = jmap.asMap();
    m->setStrFunc("sqrt", mc_nsfnmath_sqrt);
    m->setStrFunc("pow", mc_nsfnmath_pow);
    m->setStrFunc("sin", mc_nsfnmath_sin);
    m->setStrFunc("cos", mc_nsfnmath_cos);
    m->setStrFunc("tan", mc_nsfnmath_tan);
    m->setStrFunc("log", mc_nsfnmath_log);
    m->setStrFunc("ceil", mc_nsfnmath_ceil);
    m->setStrFunc("floor", mc_nsfnmath_floor);
    m->setStrFunc("abs", mc_nsfnmath_abs);
    m->setStrFunc("hypot", mc_nsfnmath_hypot);
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
    map = Value::makeMap();
    auto m = map.asMap();
    m->setStrFunc("read", mc_nsfnfile_readfile);
    m->setStrFunc("write", mc_nsfnfile_writefile);
    m->setStrFunc("put", mc_nsfnfile_writefile);
    m->setStrFunc("join", mc_nsfnfile_join);
    m->setStrFunc("isDirectory", mc_nsfnfile_isdirectory);
    m->setStrFunc("isFile", mc_nsfnfile_isfile);
    m->setStrFunc("stat", mc_nsfnfile_stat);
    state->setGlobalValue("File", map);
}

void mc_cli_installdir(State* state)
{
    Value map;
    map = Value::makeMap();
    auto m = map.asMap();
    m->setStrFunc("read", mc_nsfndir_readdir);
    state->setGlobalValue("Dir", map);
}

void mc_cli_installvmvar(State* state)
{
    Value map;
    map = Value::makeMap();
    auto m = map.asMap();
    m->setStrFunc("hadRecovered", mc_nsfnvm_hadrecovered);
    state->setGlobalValue("VM", map);
}

static int g_extfnvar;

/* use by sanity.mc */
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
    program = state->compileToProgram(source, filename);
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
    argvobj = Value::makeArray();
    auto arr = argvobj.asArray();
    for(i=beginat; i<argc; i++)
    {
        strval = Value::makeString(argv[i]);
        arr->push(strval);
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
    printtypesize(Object);    
    printtypesize(Value);
    printtypesize(Value::ObjFunction);
    printtypesize(Value::ObjUserdata);
    printtypesize(Value::ObjError);
    printtypesize(Value::ObjString);
    printtypesize(Value::CompareResult);
    printtypesize(StrDict<char*, int>);
    printtypesize(ValDict<Value, Value>);
    printtypesize(IOHandle::Config);
    printtypesize(IOHandle);
    printtypesize(Error);
    printtypesize(Traceback);
    printtypesize(SourceFile);
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
    printtypesize(SourceLocation);
    printtypesize(AstToken);
    printtypesize(AstCodeBlock);
    printtypesize(ExprLiteralMap);
    printtypesize(ExprLiteralArray);
    printtypesize(ExprLiteralString);
    printtypesize(ExprPrefix);
    printtypesize(ExprInfix);
    printtypesize(ExprIfCase);
    printtypesize(ExprLiteralFunction);
    printtypesize(ExprCall);
    printtypesize(ExprIndex);
    printtypesize(ExprAssign);
    printtypesize(ExprLogical);
    printtypesize(ExprTernary);
    printtypesize(AstIdentifier);
    printtypesize(ExprFuncParam);
    printtypesize(ExprDefine);
    printtypesize(ExprIfStmt);
    printtypesize(ExprWhileStmt);
    printtypesize(ExprForeachStmt);
    printtypesize(ExprLoopStmt);
    printtypesize(ExprImportStmt);
    printtypesize(ExprRecover);

    printtypesize(Instruction::Definition);
    printtypesize(AstScopeBlock);
    printtypesize(AstScopeFile);
    printtypesize(AstScopeComp);
    printtypesize(AstLexer);
    printtypesize(AstLexData);
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

