
#pragma once

/*
 stream_buffer.h
 project: string_buffer
 url: https://github.com/noporpoise/StringBuffer
 author: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain
 Jan 2015
*/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <new>
#include <cassert>
#include "mem.h"

#if defined(__GNUC__)
    #define DYN_STRBUF_ATTRIBUTE(x) __attribute__(x)
#else
    #define DYN_STRBUF_ATTRIBUTE(x)
#endif

#define STRBUF_MIN(x, y) ((x) < (y) ? (x) : (y))
#define STRBUF_MAX(x, y) ((x) > (y) ? (x) : (y))

#define dyn_strbuf_exitonerror()     \
    do                      \
    {                       \
        abort();            \
        exit(EXIT_FAILURE); \
    } while(0)

/*
********************
*  Bounds checking
********************
*/


#define ROUNDUP2POW(x) dyn_strutil_rndup2pow64(x)

static inline size_t dyn_strutil_rndup2pow64(uint64_t x)
{
    /* long long >=64 bits guaranteed in C99 */
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    ++x;
    return x;
}


/*
// Resizing
*/
static inline void dyn_strutil_cbufcapacity(char** buf, size_t* sizeptr, size_t len)
{
    /* for nul byte */
    len++;
    if(*sizeptr < len)
    {
        *sizeptr = ROUNDUP2POW(len);
        /* fprintf(stderr, "sizeptr=%ld\n", *sizeptr); */
        if((*buf = (char*)mc_memory_realloc(*buf, *sizeptr)) == NULL)
        {
            fprintf(stderr, "[%s:%i] Out of memory\n", __FILE__, __LINE__);
            abort();
        }
    }
}

static inline void dyn_strutil_cbufappendchar(char** buf, size_t* lenptr, size_t* sizeptr, char c)
{
    dyn_strutil_cbufcapacity(buf, sizeptr, *lenptr + 1);
    (*buf)[(*lenptr)++] = c;
    (*buf)[*lenptr] = '\0';
}





typedef struct StringBuffer StringBuffer;

struct StringBuffer
{
    public:
        static StringBuffer* fromPtr(StringBuffer* sbuf, size_t len)
        {
            sbuf->m_length = 0;
            sbuf->m_capacity = ROUNDUP2POW(len + 1);
            sbuf->m_data = (char*)mc_memory_malloc(sbuf->m_capacity);
            if(!sbuf->m_data)
            {
                return NULL;
            }
            sbuf->m_data[0] = '\0';
            return sbuf;
        }


        static bool destroy(StringBuffer* sb)
        {
            mc_memory_free(sb->m_data);
            mc_memory_free(sb);
            return true;
        }

        static bool destroyFromPtr(StringBuffer* sb)
        {
            mc_memory_free(sb->m_data);
            return true;
        }

    public:
        char* m_data;

        /* total length of this buffer */
        size_t m_length;

        /* capacity should be >= length+1 to allow for \0 */
        size_t m_capacity;

    public:
        inline void checkBoundsInsert(size_t pos) const
        {
            if(pos > this->m_length)
            {
                fprintf(stderr, "StringBuffer: out of bounds error [index: %ld, num_of_bits: %ld]\n", pos, this->m_length);
                errno = EDOM;
                dyn_strbuf_exitonerror();
            }
        }

        /* Bounds check when reading a range (start+len < strlen is valid) */
        inline void checkBoundsReadRange(size_t start, size_t len) const
        {
            const char* endstr;
            if(start + len > this->m_length)
            {
                endstr = (this->m_length > 5 ? "..." : "");
                fprintf(stderr,"StringBuffer: out of bounds error [start: %ld; length: %ld; strlen: %ld; buf:%.*s%s]\n", start, len, this->m_length, (int)STRBUF_MIN(5, this->m_length), this->m_data, endstr);
                errno = EDOM;
                dyn_strbuf_exitonerror();
            }
        }

    public:
        StringBuffer(size_t len)
        {
            this->m_length = 0;
            this->m_capacity = 0;
            this->m_data = NULL;
            assert(fromPtr(this, len));
        }

        ~StringBuffer()
        {
            StringBuffer::destroy(this);
        }

        /*
        // Resize the buffer to have capacity to hold a string of length newlen
        // (+ a null terminating character).  Can also be used to downsize the buffer's
        // memory usage.  Returns 1 on success, 0 on failure.
        */
        bool resize(size_t newlen)
        {
            size_t cap;
            char* newbuf;
            cap = ROUNDUP2POW(newlen + 1);
            newbuf = (char*)mc_memory_realloc(this->m_data, cap * sizeof(char));
            if(newbuf == NULL)
            {
                return false;
            }
            this->m_data = newbuf;
            this->m_capacity = cap;
            if(this->m_length > newlen)
            {
                /* Buffer was shrunk - re-add null byte */
                this->m_length = newlen;
                this->m_data[this->m_length] = '\0';
            }
            return true;
        }


        /* Ensure capacity for len characters plus '\0' character - exits on FAILURE */
        void ensureCapacity(size_t len)
        {
            dyn_strutil_cbufcapacity(&this->m_data, &this->m_capacity, len);
        }

        /* Same as above, but update pointer if it pointed to resized array */
        void ensureCapacityUpdatePtr(size_t size, const char** ptr)
        {
            size_t oldcap;
            char* oldbuf;
            if(this->m_capacity <= size + 1)
            {
                oldcap = this->m_capacity;
                oldbuf = this->m_data;
                if(!this->resize(size))
                {
                    fprintf(stderr,
                            "%s:%i:Error: _ensure_capacity_update_ptr couldn't resize "
                            "buffer. [requested %ld bytes; capacity: %ld bytes]\n",
                            __FILE__, __LINE__, size, this->m_capacity);
                    dyn_strbuf_exitonerror();
                }
                /* ptr may have pointed to this, which has now moved */
                if(*ptr >= oldbuf && *ptr < oldbuf + oldcap)
                {
                    *ptr = this->m_data + (*ptr - oldbuf);
                }
            }
        }


    public:
        size_t length() const
        {
            return m_length;
        }

        char* data() const
        {
            return m_data;
        }

        void setLength(size_t sz)
        {
            m_length = sz;
        }

        /*
        // Copy N characters from a character array to the end of this StringBuffer
        // strlen(str) must be >= len
        */
        bool append(const char* str, size_t len)
        {
            this->ensureCapacityUpdatePtr(this->m_length + len, &str);
            memcpy(this->m_data + this->m_length, str, len);
            this->m_data[this->m_length = this->m_length + len] = '\0';
            return true;
        }

        template<typename... ArgsT>
        int appendFormatAtPos(size_t pos, const char* fmt, ArgsT&&... args)
        {
            static auto wrapsnprintf = snprintf;
            static auto wrapsprintf = sprintf;
            size_t buflen;
            int numchars;
            this->checkBoundsInsert(pos);
            /* Length of remaining buffer */
            buflen = this->m_capacity - pos;
            if(buflen == 0 && !this->resize(this->m_capacity << 1))
            {
                fprintf(stderr, "%s:%i:Error: Out of memory\n", __FILE__, __LINE__);
                dyn_strbuf_exitonerror();
            }
            /* Make a copy of the list of args incase we need to resize buff and try again */
            numchars = wrapsnprintf(this->m_data + pos, buflen, fmt, args...);
            /*
            // numchars is the number of chars that would be written (not including '\0')
            // numchars < 0 => failure
            */
            if(numchars < 0)
            {
                fprintf(stderr, "Warning: dyn_strbuf_appendformatv something went wrong..\n");
                dyn_strbuf_exitonerror();
            }
            /* numchars does not include the null terminating byte */
            if((size_t)numchars + 1 > buflen)
            {
                this->ensureCapacity(pos + (size_t)numchars);
                /*
                // now use the argptr copy we made earlier
                // Don't need to use vsnprintf now, vsprintf will do since we know it'll fit
                */
                numchars = wrapsprintf(this->m_data + pos, fmt, args...);
                if(numchars < 0)
                {
                    fprintf(stderr, "Warning: dyn_strbuf_appendformatv something went wrong..\n");
                    dyn_strbuf_exitonerror();
                }
            }
            /*
            // Don't need to NUL terminate, vsprintf/vnsprintf does that for us
            // Update m_length
            */
            this->m_length = pos + (size_t)numchars;
            return numchars;
        }



        /* sprintf to the end of a StringBuffer (adds string terminator after sprint) */
        template<typename... ArgsT>
        int appendFormat(const char* fmt, ArgsT&&... args)
        {
            int numchars;
            numchars = this->appendFormatAtPos(this->m_length, fmt, args...);
            return numchars;
        }

};


