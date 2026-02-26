

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

class StringBuffer
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
            destroyFromPtr(sb);
            mc_memory_free(sb);
            return true;
        }

        static bool destroyFromPtr(StringBuffer* sb)
        {
            if(sb->m_data != nullptr)
            {
                mc_memory_free(sb->m_data);
            }
            return true;
        }

    public:
        char* m_data = nullptr;
        /* total length of this buffer */
        size_t m_length = 0;

        /* capacity should be >= length+1 to allow for \0 */
        size_t m_capacity = 0;

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
        StringBuffer()
        {
            this->m_length = 0;
            this->m_capacity = 0;
            this->m_data = NULL;
        }

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

        void toUppercase()
        {
            char* pos;
            char* end;
            end = this->m_data + this->m_length;
            for(pos = this->m_data; pos < end; pos++)
            {
                *pos = (char)toupper(*pos);
            }
        }

        void toLowercase()
        {
            char* pos;
            char* end;
            end = this->m_data + this->m_length;
            for(pos = this->m_data; pos < end; pos++)
            {
                *pos = (char)tolower(*pos);
            }
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

#if !defined(va_copy)
    #if defined(__GNUC__) || defined(__CLANG__)
        #define va_copy(d,s) __builtin_va_copy(d,s)
    #else
        #define va_copy(dest, src) memcpy(dest, src, sizeof(va_list))
    #endif
#endif

#if defined(__STRICT_ANSI__)
    void *memccpy(void *dest, const void *src, int c, size_t n);
    int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif



/*
// `n` is the maximum number of bytes to copy including the NULL byte
// copies at most n bytes from `src` to `dst`
// Always appends a NULL terminating byte, unless n is zero.
// Returns a pointer to dst
*/
char* dyn_strutil_safencpy(char* dst, const char* src, size_t n)
{
    if(n == 0)
    {
        return dst;
    }
    /*
    // From The Open Group:
    //   The memccpy() function copies bytes from memory area s2 into s1, stopping
    //   after the first occurrence of byte c is copied, or after n bytes are copied,
    //   whichever comes first. If copying takes place between objects that overlap,
    //   the behaviour is undefined.
    // Returns NULL if character c was not found in the copied memory
    */
    if(memccpy(dst, src, '\0', n - 1) == NULL)
    {
        dst[n - 1] = '\0';
    }
    return dst;
}

/*
// Replaces `sep` with \0 in str
// Returns number of occurances of `sep` character in `str`
// Stores `nptrs` pointers in `ptrs`
*/
size_t dyn_strutil_splitstr(char* str, char sep, char** ptrs, size_t nptrs)
{
    size_t n;
    n = 1;
    if(*str == '\0')
    {
        return 0;
    }
    if(nptrs > 0)
    {
        ptrs[0] = str;
    }
    while((str = strchr(str, sep)) != NULL)
    {
        *str = '\0';
        str++;
        if(n < nptrs)
        {
            ptrs[n] = str;
        }
        n++;
    }
    return n;
}

/*
// Replace one char with another in a string. Return number of replacements made
*/
size_t dyn_strutil_charreplace(char* str, char from, char to)
{
    size_t n;
    n = 0;
    for(; *str; str++)
    {
        if(*str == from)
        {
            n++;
            *str = to;
        }
    }
    return n;
}

/*
// Reverse a string region
*/
void dyn_strutil_reverseregion(char* str, size_t len)
{
    char *a;
    char* b;
    char tmp;
    a = str;
    b = str + len - 1;
    while(a < b)
    {
        tmp = *a;
        *a = *b;
        *b = tmp;
        a++;
        b--;
    }
}

bool dyn_strutil_isallspace(const char* s)
{
    int i;
    for(i = 0; s[i] != '\0' && isspace((int)s[i]); i++)
    {
    }
    return (s[i] == '\0');
}

char* dyn_strutil_nextspace(char* s)
{
    while(*s != '\0' && isspace((int)*s))
    {
        s++;
    }
    return (*s == '\0' ? NULL : s);
}

/*
// Strip whitespace the the start and end of a string.
// Strips whitepace from the end of the string with \0, and returns pointer to
// first non-whitespace character
*/
char* dyn_strutil_trim(char* str)
{
    /* Work backwards */
    char* end;
    end = str + strlen(str);
    while(end > str && isspace((int)*(end - 1)))
    {
        end--;
    }
    *end = '\0';
    /* Work forwards: don't need start < len because will hit \0 */
    while(isspace((int)*str))
    {
        str++;
    }
    return str;
}

/*
// Removes \r and \n from the ends of a string and returns the new length
*/
size_t dyn_strutil_chomp(char* str, size_t len)
{
    while(len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n'))
    {
        len--;
    }
    str[len] = '\0';
    return len;
}

/*
// Returns count
*/
size_t dyn_strutil_countchar(const char* str, char c)
{
    size_t count;
    count = 0;
    while((str = strchr(str, c)) != NULL)
    {
        str++;
        count++;
    }
    return count;
}

/*
// Returns the number of strings resulting from the split
*/
size_t dyn_strutil_split(const char* splitat, const char* sourcetxt, char*** result)
{
    size_t i;
    size_t slen;
    size_t count;
    size_t splitlen;
    size_t txtlen;
    char** arr;
    const char* find;
    const char* plastpos;
    splitlen = strlen(splitat);
    txtlen = strlen(sourcetxt);
    /* result is temporarily held here */
    if(splitlen == 0)
    {
        /* Special case */
        if(txtlen == 0)
        {
            *result = NULL;
            return 0;
        }
        else
        {
            arr = (char**)mc_memory_malloc(txtlen * sizeof(char*));
            for(i = 0; i < txtlen; i++)
            {
                arr[i] = (char*)mc_memory_malloc(2 * sizeof(char));
                arr[i][0] = sourcetxt[i];
                arr[i][1] = '\0';
            }
            *result = arr;
            return txtlen;
        }
    }
    find = sourcetxt;
    /* must have at least one item */
    count = 1;
    for(; (find = strstr(find, splitat)) != NULL; count++, find += splitlen)
    {
    }
    /* Create return array */
    arr = (char**)mc_memory_malloc(count * sizeof(char*));
    count = 0;
    plastpos = sourcetxt;
    while((find = strstr(plastpos, splitat)) != NULL)
    {
        slen = (size_t)(find - plastpos);
        arr[count] = (char*)mc_memory_malloc((slen + 1) * sizeof(char));
        strncpy(arr[count], plastpos, slen);
        arr[count][slen] = '\0';
        count++;
        plastpos = find + splitlen;
    }
    /* Copy last item */
    slen = (size_t)(sourcetxt + txtlen - plastpos);
    arr[count] = (char*)mc_memory_malloc((slen + 1) * sizeof(char));
    if(count == 0)
    {
        strcpy(arr[count], sourcetxt);
    }
    else
    {
        strncpy(arr[count], plastpos, slen);
    }
    arr[count][slen] = '\0';
    count++;
    *result = arr;
    return count;
}
/*
// Constructors / Destructors
*/

/*
// Copy a string or existing string buffer
*/
StringBuffer* dyn_strbuf_makefromstring(const char* str, size_t slen)
{
    StringBuffer* sbuf;
    sbuf = Memory::make<StringBuffer>(slen + 1);
    if(!sbuf)
    {
        return NULL;
    }
    memcpy(sbuf->m_data, str, slen);
    sbuf->m_data[sbuf->m_length = slen] = '\0';
    return sbuf;
}

StringBuffer* dyn_strbuf_makeclone(const StringBuffer* sbuf)
{
    /* One byte for the string end / null char \0 */
    StringBuffer* cpy;
    cpy = Memory::make<StringBuffer>(sbuf->m_length + 1);
    if(!cpy)
    {
        return NULL;
    }
    memcpy(cpy->m_data, sbuf->m_data, sbuf->m_length);
    cpy->m_data[cpy->m_length = sbuf->m_length] = '\0';
    return cpy;
}


/* Clear the content of an existing StringBuffer (sets size to 0) */
void dyn_strbuf_reset(StringBuffer* sb)
{
    if(sb->m_data)
    {
        memset(sb->m_data, 0, sb->m_length);
    }
    sb->m_length = 0;
}


bool dyn_strbuf_containschar(StringBuffer* sb, char ch)
{
    size_t i;
    for(i=0; i<sb->m_length; i++)
    {
        if(sb->m_data[i] == ch)
        {
            return true;
        }
    }
    return false;
}

/* via: https://codereview.stackexchange.com/q/274832 */
void dyn_strutil_faststrncat(char *dest, const char *src, size_t *size)
{
    if(dest && src && size)
    {
        while((dest[*size] = *src++))
        {
            *size += 1;
        }
    }
}

size_t dyn_strutil_strreplace1(char **str, size_t selflen, const char* findstr, size_t findlen, const char *substr, size_t sublen)
{
    size_t i;
    size_t x;
    size_t oldcount;
    char* buff;
    const char *temp;
    (void)selflen;
    oldcount = 0;
    temp = (const char *)(*str);
    for (i = 0; temp[i] != '\0'; ++i)
    {
        if (strstr((const char *)&temp[i], findstr) == &temp[i])
        {
            oldcount++;
            i += findlen - 1;
        }
    }
    buff = (char*)mc_memory_calloc((i + oldcount * (sublen - findlen) + 1), sizeof(char));
    if (!buff)
    {
        perror("bad allocation\n");
        exit(EXIT_FAILURE);
    }
    i = 0;
    while (*temp)
    {
        if (strstr(temp, findstr) == temp)
        {
            x = 0;
            dyn_strutil_faststrncat(&buff[i], substr, &x);
            i += sublen;
            temp += findlen;
        }
        else
        {
            buff[i++] = *temp++;
        }
    }
    mc_memory_free(*str);
    *str = (char*)mc_memory_calloc(i + 1, sizeof(char));
    if (!(*str))
    {
        perror("bad allocation\n");
        exit(EXIT_FAILURE);
    }
    i = 0;
    dyn_strutil_faststrncat(*str, (const char *)buff, &i);
    mc_memory_free(buff);
    return i;
}

size_t dyn_strutil_strrepcount(const char* str, size_t slen, const char* findstr, size_t findlen, size_t sublen)
{
    size_t i;
    size_t count;
    size_t total;
    (void)total;
    total = slen;
    count = 0;
    for(i=0; i<slen; i++)
    {
        if(str[i] == findstr[0])
        {
            if((i + findlen) < slen)
            {
                if(memcmp(&str[i], findstr, findlen) == 0)
                {
                    count++;
                    total += sublen;
                }
            }
        }
    }
    if(count == 0)
    {
        return 0;
    }
    return total + 0;
}

/* via: https://stackoverflow.com/a/32413923 */
void dyn_strutil_strreplace2(char* target, size_t tgtlen, const char *findstr, size_t findlen, const char *substr, size_t sublen)
{
    const char *p;
    const char *tmp;
    char *inspoint;
    char buffer[1024] = {0};
    (void)tgtlen;
    inspoint = &buffer[0];
    tmp = target;
    while(true)
    {
        p = strstr(tmp, findstr);
        /* walked past last occurrence of findstr; copy remaining part */
        if (p == NULL)
        {
            strcpy(inspoint, tmp);
            break;
        }
        /* copy part before findstr */
        memcpy(inspoint, tmp, p - tmp);
        inspoint += p - tmp;
        /* copy substr string */
        memcpy(inspoint, substr, sublen);
        inspoint += sublen;
        /* adjust pointers, move on */
        tmp = p + findlen;
    }
    /* write altered string back to target */
    strcpy(target, buffer);
}

bool dyn_strbuf_fullreplace(StringBuffer* sb, const char* findstr, size_t findlen, const char* substr, size_t sublen)
{
    size_t nl;
    size_t needed;
    needed = dyn_strutil_strrepcount(sb->m_data, sb->m_length, findstr, findlen, sublen);
    if(needed == 0)
    {
        return false;
    }
    sb->resize(sb->m_capacity + needed);
    nl = dyn_strutil_strreplace1(&sb->m_data, sb->m_length, findstr, findlen, substr, sublen);
    sb->m_length = nl;
    return true;
}

bool dyn_strutil_inpreplhelper(char *dest, const char *src, size_t srclen, int findme, const char* substr, size_t sublen, size_t maxlen, size_t* dlen)
{
    /* ch(ar) at pos(ition) */
    int chatpos;
    /* printf("'%p' '%s' %c\n", dest, src, findme); */
    if(*src == findme)
    {
        if(sublen > maxlen)
        {
            return false;
        }
        if(!dyn_strutil_inpreplhelper(dest + sublen, src + 1, srclen, findme, substr, sublen, maxlen - sublen, dlen))
        {
            return false;
        }
        memcpy(dest, substr, sublen);
        *dlen += sublen;
        return true;
    }
    if(maxlen == 0)
    {
        return false;
    }
    chatpos = *src;
    if(*src)
    {
        *dlen += 1;
        if(!dyn_strutil_inpreplhelper(dest + 1, src + 1, srclen, findme, substr, sublen, maxlen - 1, dlen))
        {
            return false;
        }
    }
    *dest = chatpos;
    return true;
}

size_t dyn_strutil_inpreplace(char* target, size_t tgtlen, int findme, const char* substr, size_t sublen, size_t maxlen)
{
    size_t nlen;
    if(findme == 0)
    {
        return 0;
    }
    if(maxlen == 0)
    {
        return 0;
    }
    if(*substr == 0)
    {
        /* Insure target does not shrink. */
        return 0;
    }
    nlen = 0;
    dyn_strutil_inpreplhelper(target, target, tgtlen, findme, substr, sublen, maxlen - 1, &nlen);
    return nlen;
}

bool dyn_strbuf_charreplace(StringBuffer* sb, int findme, const char* substr, size_t sublen)
{
    size_t i;
    size_t nlen;
    size_t needed;
    needed = sb->m_capacity;
    for(i=0; i<sb->m_length; i++)
    {
        if(sb->m_data[i] == findme)
        {
            needed += sublen;
        }
    }
    if(!sb->resize(needed+1))
    {
        return false;
    }
    nlen = dyn_strutil_inpreplace(sb->m_data, sb->m_length, findme, substr, sublen, sb->m_capacity);
    sb->m_length = nlen;
    return true;
}

/* Set string buffer to contain a given string */
void dyn_strbuf_set(StringBuffer* sb, const char* str)
{
    size_t len;
    len = strlen(str);
    sb->ensureCapacity(len);
    memcpy(sb->m_data, str, len);
    sb->m_data[sb->m_length = len] = '\0';
}


/* Set string buffer to match existing string buffer */
void dyn_strbuf_setbuff(StringBuffer* dest, StringBuffer* from)
{
    dest->ensureCapacity(from->m_length);
    memmove(dest->m_data, from->m_data, from->m_length);
    dest->m_data[dest->m_length = from->m_length] = '\0';
}

/* Add a character to the end of this StringBuffer */
bool dyn_strbuf_appendchar(StringBuffer* sb, int c)
{
    sb->ensureCapacity(sb->m_length + 1);
    sb->m_data[sb->m_length] = c;
    sb->m_data[++sb->m_length] = '\0';
    return true;
}



/* Copy a character array to the end of this StringBuffer */
bool dyn_strbuf_appendstr(StringBuffer* sb, const char* str)
{
    return sb->append(str, strlen(str));
}

bool dyn_strbuf_appendbuff(StringBuffer* sb1, const StringBuffer* sb2)
{
    return sb1->append(sb2->m_data, sb2->m_length);
}


/*
 * Integer to string functions adapted from:
 *   https://www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920
 */

#define DYN_STRCONST_P01 10
#define DYN_STRCONST_P02 100
#define DYN_STRCONST_P03 1000
#define DYN_STRCONST_P04 10000
#define DYN_STRCONST_P05 100000
#define DYN_STRCONST_P06 1000000
#define DYN_STRCONST_P07 10000000
#define DYN_STRCONST_P08 100000000
#define DYN_STRCONST_P09 1000000000
#define DYN_STRCONST_P10 10000000000
#define DYN_STRCONST_P11 100000000000
#define DYN_STRCONST_P12 1000000000000

/**
 * Return number of digits required to represent `num` in base 10.
 * Uses binary search to find number.
 * Examples:
 *   dyn_strutil_numofdigits(0)   = 1
 *   dyn_strutil_numofdigits(1)   = 1
 *   dyn_strutil_numofdigits(10)  = 2
 *   dyn_strutil_numofdigits(123) = 3
 */
size_t dyn_strutil_numofdigits(unsigned long v)
{
    if(v < DYN_STRCONST_P01)
    {
        return 1;
    }
    if(v < DYN_STRCONST_P02)
    {
        return 2;
    }
    if(v < DYN_STRCONST_P03)
    {
        return 3;
    }
    if(v < DYN_STRCONST_P12)
    {
        if(v < DYN_STRCONST_P08)
        {
            if(v < DYN_STRCONST_P06)
            {
                if(v < DYN_STRCONST_P04)
                {
                    return 4;
                }
                return 5 + (v >= DYN_STRCONST_P05);
            }
            return 7 + (v >= DYN_STRCONST_P07);
        }
        if(v < DYN_STRCONST_P10)
        {
            return 9 + (v >= DYN_STRCONST_P09);
        }
        return 11 + (v >= DYN_STRCONST_P11);
    }
    return 12 + dyn_strutil_numofdigits(v / DYN_STRCONST_P12);
}


/* Convert integers to string to append */
bool dyn_strbuf_appendnumulong(StringBuffer* buf, unsigned long value)
{
    size_t v;
    size_t pos;
    size_t numdigits;
    char* dst;
    /* Append two digits at a time */
    static const char* digits = (
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899"
    );
    numdigits = dyn_strutil_numofdigits(value);
    pos = numdigits - 1;
    buf->ensureCapacity(buf->m_length + numdigits);
    dst = buf->m_data + buf->m_length;
    while(value >= 100)
    {
        v = value % 100;
        value /= 100;
        dst[pos] = digits[v * 2 + 1];
        dst[pos - 1] = digits[v * 2];
        pos -= 2;
    }
    /* Handle last 1-2 digits */
    if(value < 10)
    {
        dst[pos] = '0' + value;
    }
    else
    {
        dst[pos] = digits[value * 2 + 1];
        dst[pos - 1] = digits[value * 2];
    }
    buf->m_length += numdigits;
    buf->m_data[buf->m_length] = '\0';
    return true;
}

bool dyn_strbuf_appendnumlong(StringBuffer* buf, long value)
{
    /* dyn_strbuf_appendformat(buf, "%li", value); */
    if(value < 0)
    {
        dyn_strbuf_appendchar(buf, '-');
        value = -value;
    }
    return dyn_strbuf_appendnumulong(buf, value);
}


bool dyn_strbuf_appendnumint(StringBuffer* buf, int value)
{
    /* dyn_strbuf_appendformat(buf, "%i", value); */
    return dyn_strbuf_appendnumlong(buf, value);
}


/* Append string converted to lowercase */
bool dyn_strbuf_appendstrnlowercase(StringBuffer* buf, const char* str, size_t len)
{
    char* to;
    const char* plength;
    buf->ensureCapacity(buf->m_length + len);
    to = buf->m_data + buf->m_length;
    plength = str + len;
    for(; str < plength; str++, to++)
    {
        *to = tolower(*str);
    }
    buf->m_length += len;
    buf->m_data[buf->m_length] = '\0';
    return true;
}

/* Append string converted to uppercase */
bool dyn_strbuf_appendstrnuppercase(StringBuffer* buf, const char* str, size_t len)
{
    char* to;
    const char* end;
    buf->ensureCapacity(buf->m_length + len);
    to = buf->m_data + buf->m_length;
    end = str + len;
    for(; str < end; str++, to++)
    {
        *to = toupper(*str);
    }
    buf->m_length += len;
    buf->m_data[buf->m_length] = '\0';
    return true;
}


/* Append char `c` `n` times */
bool dyn_strbuf_appendcharn(StringBuffer* buf, char c, size_t n)
{
    buf->ensureCapacity(buf->m_length + n);
    memset(buf->m_data + buf->m_length, c, n);
    buf->m_length += n;
    buf->m_data[buf->m_length] = '\0';
    return true;
}

void dyn_strbuf_shrink(StringBuffer* sb, size_t len)
{
    sb->m_data[sb->m_length = (len)] = 0;
}

/*
// Remove \r and \n characters from the end of this StringBuffesr
// Returns the number of characters removed
*/
size_t dyn_strbuf_chomp(StringBuffer* sbuf)
{
    size_t oldlen;
    oldlen = sbuf->m_length;
    sbuf->m_length = dyn_strutil_chomp(sbuf->m_data, sbuf->m_length);
    return oldlen - sbuf->m_length;
}

/* Reverse a string */
void dyn_strbuf_reverse(StringBuffer* sbuf)
{
    dyn_strutil_reverseregion(sbuf->m_data, sbuf->m_length);
}

/*
// Get a substring as a new null terminated char array
// (remember to free the returned char* after you're done with it!)
*/
char* dyn_strbuf_substr(const StringBuffer* sbuf, size_t start, size_t len)
{
    char* newstr;
    sbuf->checkBoundsReadRange(start, len);
    newstr = (char*)mc_memory_malloc((len + 1) * sizeof(char));
    strncpy(newstr, sbuf->m_data + start, len);
    newstr[len] = '\0';
    return newstr;
}


/*
// Copy a string to this StringBuffer, overwriting any existing characters
// Note: dstpos + len can be longer the the current dst StringBuffer
*/
void dyn_strbuf_copyover(StringBuffer* dst, size_t dstpos, const char* src, size_t len)
{
    size_t newlen;
    if(src == NULL || len == 0)
    {
    }
    else
    {
        dst->checkBoundsInsert(dstpos);
        /*
        // Check if dst buffer can handle string
        // src may have pointed to dst, which has now moved
        */
        newlen = STRBUF_MAX(dstpos + len, dst->m_length);
        dst->ensureCapacityUpdatePtr(newlen, &src);
        /* memmove instead of strncpy, as it can handle overlapping regions */
        memmove(dst->m_data + dstpos, src, len * sizeof(char));
        if(dstpos + len > dst->m_length)
        {
            /* Extended string - add '\0' char */
            dst->m_length = dstpos + len;
            dst->m_data[dst->m_length] = '\0';
        }
    }
}

/* Insert: copy to a StringBuffer, shifting any existing characters along */
void dyn_strbuf_insert(StringBuffer* dst, size_t dstpos, const char* src, size_t len)
{
    char* insert;
    if(src == NULL || len == 0)
    {
    }
    else
    {
        dst->checkBoundsInsert(dstpos);
        /*
        // Check if dst buffer has capacity for inserted string plus \0
        // src may have pointed to dst, which will be moved in realloc when
        // calling ensure capacity
        */
        dst->ensureCapacityUpdatePtr(dst->m_length + len, &src);
        insert = dst->m_data + dstpos;
        /* dstpos could be at the end (== dst->m_length) */
        if(dstpos < dst->m_length)
        {
            /* Shift some characters up */
            memmove(insert + len, insert, (dst->m_length - dstpos) * sizeof(char));
            if(src >= dst->m_data && src < dst->m_data + dst->m_capacity)
            {
                /* src/dst strings point to the same string in memory */
                if(src < insert)
                {
                    memmove(insert, src, len * sizeof(char));
                }
                else if(src > insert)
                {
                    memmove(insert, src + len, len * sizeof(char));
                }
            }
            else
            {
                memmove(insert, src, len * sizeof(char));
            }
        }
        else
        {
            memmove(insert, src, len * sizeof(char));
        }
        /* Update size */
        dst->m_length += len;
        dst->m_data[dst->m_length] = '\0';
    }
}

/*
// Overwrite dstpos..(dstpos+dstlen-1) with srclen chars from src
// if dstlen != srclen, content to the right of dstlen is shifted
// Example:
//   dyn_strbuf_set(sbuf, "aaabbccc");
//   char *data = "xxx";
//   dyn_strbuf_overwrite(sbuf,3,2,data,strlen(data));
//   // sbuf is now "aaaxxxccc"
//   dyn_strbuf_overwrite(sbuf,3,2,"_",1);
//   // sbuf is now "aaa_ccc"
*/
void dyn_strbuf_overwrite(StringBuffer* dst, size_t dstpos, size_t dstlen, const char* src, size_t srclen)
{
    size_t len;
    size_t newlen;
    char* tgt;
    char* end;
    dst->checkBoundsReadRange(dstpos, dstlen);
    if(src != NULL)
    {
        if(dstlen == srclen)
        {
            dyn_strbuf_copyover(dst, dstpos, src, srclen);
        }
        newlen = dst->m_length + srclen - dstlen;
        dst->ensureCapacityUpdatePtr(newlen, &src);
        if(src >= dst->m_data && src < dst->m_data + dst->m_capacity)
        {
            if(srclen < dstlen)
            {
                /* copy */
                memmove(dst->m_data + dstpos, src, srclen * sizeof(char));
                /* resize (shrink) */
                memmove(dst->m_data + dstpos + srclen, dst->m_data + dstpos + dstlen, (dst->m_length - dstpos - dstlen) * sizeof(char));
            }
            else
            {
                /*
                // Buffer is going to grow and src points to this buffer
                // resize (grow)
                */
                memmove(dst->m_data + dstpos + srclen, dst->m_data + dstpos + dstlen, (dst->m_length - dstpos - dstlen) * sizeof(char));
                tgt = dst->m_data + dstpos;
                end = dst->m_data + dstpos + srclen;
                if(src < tgt + dstlen)
                {
                    len = STRBUF_MIN((size_t)(end - src), srclen);
                    memmove(tgt, src, len);
                    tgt += len;
                    src += len;
                    srclen -= len;
                }
                if(src >= tgt + dstlen)
                {
                    /* shift to account for resizing */
                    src += srclen - dstlen;
                    memmove(tgt, src, srclen);
                }
            }
        }
        else
        {
            /* resize */
            memmove(dst->m_data + dstpos + srclen, dst->m_data + dstpos + dstlen, (dst->m_length - dstpos - dstlen) * sizeof(char));
            /* copy */
            memcpy(dst->m_data + dstpos, src, srclen * sizeof(char));
        }
        dst->m_length = newlen;
        dst->m_data[dst->m_length] = '\0';
    }
}

/*
// Remove characters from the buffer
//   dyn_strbuf_set(sb, "aaaBBccc");
//   dyn_strbuf_erase(sb, 3, 2);
//   // sb is now "aaaccc"
*/
void dyn_strbuf_erase(StringBuffer* sbuf, size_t pos, size_t len)
{
    sbuf->checkBoundsReadRange(pos, len);
    memmove(sbuf->m_data + pos, sbuf->m_data + pos + len, sbuf->m_length - pos - len);
    sbuf->m_length -= len;
    sbuf->m_data[sbuf->m_length] = '\0';
}

/*
// sprintf
*/


/* Trim whitespace characters from the start and end of a string */
void dyn_strbuf_triminplace(StringBuffer* sbuf)
{
    size_t start;
    if(sbuf->m_length > 0)
    {
        /* Trim end first */
        while(sbuf->m_length > 0 && isspace((int)sbuf->m_data[sbuf->m_length - 1]))
        {
            sbuf->m_length--;
        }
        sbuf->m_data[sbuf->m_length] = '\0';
        if(sbuf->m_length > 0)
        {
            start = 0;
            while(start < sbuf->m_length && isspace((int)sbuf->m_data[start]))
            {
                start++;
            }
            if(start != 0)
            {
                sbuf->m_length -= start;
                memmove(sbuf->m_data, sbuf->m_data + start, sbuf->m_length * sizeof(char));
                sbuf->m_data[sbuf->m_length] = '\0';
            }
        }
    }
}

/*
// Trim the characters listed in `list` from the left of `sbuf`
// `list` is a null-terminated string of characters
*/
void dyn_strbuf_trimleftinplace(StringBuffer* sbuf, const char* list)
{
    size_t start;
    start = 0;

    while(start < sbuf->m_length && (strchr(list, sbuf->m_data[start]) != NULL))
    {
        start++;
    }
    if(start != 0)
    {
        sbuf->m_length -= start;
        memmove(sbuf->m_data, sbuf->m_data + start, sbuf->m_length * sizeof(char));
        sbuf->m_data[sbuf->m_length] = '\0';
    }
}

/*
// Trim the characters listed in `list` from the right of `sbuf`
// `list` is a null-terminated string of characters
*/
void dyn_strbuf_trimrightinplace(StringBuffer* sbuf, const char* list)
{
    if(sbuf->m_length > 0)
    {
        while(sbuf->m_length > 0 && strchr(list, sbuf->m_data[sbuf->m_length - 1]) != NULL)
        {
            sbuf->m_length--;
        }
        sbuf->m_data[sbuf->m_length] = '\0';
    }
}

