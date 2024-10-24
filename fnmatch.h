
#pragma once

/*
* fnmatch compatible functions, adapted from musl-libc.
* musl license:
*
*  Musl Libc
*  Copyright © 2005-2014 Rich Felker, et al.
*
*  Permission is hereby granted, free of charge, to any person obtaining
*  a copy of this software and associated documentation files (the
*  "Software"), to deal in the Software without restriction, including
*  without limitation the rights to use, copy, modify, merge, publish,
*  distribute, sublicense, and/or sell copies of the Software, and to
*  permit persons to whom the Software is furnished to do so, subject to
*  the following conditions:
*
*  The above copyright notice and this permission notice shall be
*  included in all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
*  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
*  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*/


#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#if !defined(MB_CUR_MAX)
    #define MB_CUR_MAX 4
#endif


#define STRFNM_FLAG_PATHNAME 0x1
#define STRFNM_FLAG_NOESCAPE 0x2
#define STRFNM_FLAG_PERIOD 0x4
#define STRFNM_FLAG_LEADING_DIR 0x8
#define STRFNM_FLAG_CASEFOLD 0x10

/*
* these both are unused. probably used only in actual UNIX-esque glob().
* not needed here anyway.
*/
/*
#define STRFNM_FLAG_FILE_NAME STRFNM_FLAG_PATHNAME
#define STRFNM_FLAG_NOSYS (-1)
*/
#define STRFNM_STATUS_NOMATCH 1

#define STRFNM_STATE_END 0
#define STRFNM_STATE_UNMATCHABLE -2
#define STRFNM_STATE_BRACKET -3
#define STRFNM_STATE_QUESTION -4
#define STRFNM_STATE_STAR -5

static inline int strfnmint_nextstring(const char *str, size_t n, size_t *step);
static inline int strfnmint_nextpattern(const char *pat, size_t ofs, size_t *step, int flags);
static inline int strfnmint_casefold(int k);
static inline int strfnmint_dobracket(const char *patptr, int k, int kfold);
static inline int strfnmint_perform(const char *pat, size_t ofs, const char *str, size_t n, int flags);
static inline int strfnmint_textmatchlen(const char *pat, size_t patlen, const char *str, size_t slen, int flags);
static inline bool strfnm_match(const char *pat, size_t patlen, const char *str, size_t slen, int flags);

static inline int strfnmint_nextstring(const char* str, size_t n, size_t* step)
{
    int k;
    wchar_t wc;
    if(!n)
    {
        *step = 0;
        return 0;
    }
    if(((unsigned int)str[0]) >= 128U)
    {
        k = mbtowc(&wc, str, n);
        if(k < 0)
        {
            *step = 1;
            return -1;
        }
        *step = k;
        return wc;
    }
    *step = 1;
    return str[0];
}

static inline int strfnmint_nextpattern(const char* pat, size_t ofs, size_t* step, int flags)
{
    int chv;
    int esc;
    size_t cidx;
    wchar_t wc;
    esc = 0;
    if(!ofs || !*pat)
    {
        *step = 0;
        return STRFNM_STATE_END;
    }
    *step = 1;
    if(pat[0] == '\\' && pat[1] && !(flags & STRFNM_FLAG_NOESCAPE))
    {
        *step = 2;
        pat++;
        esc = 1;
        goto escaped;
    }
    if(pat[0] == '[')
    {
        cidx = 1;
        if(cidx < ofs)
        {
            if(pat[cidx] == '^' || pat[cidx] == '!')
            {
                cidx++;
            }
        }
        if(cidx < ofs)
        {
            if(pat[cidx] == ']')
            {
                cidx++;
            }
        }
        for(; cidx < ofs && pat[cidx] && pat[cidx] != ']'; cidx++)
        {
            if(cidx + 1 < ofs && pat[cidx + 1] && pat[cidx] == '[' && (pat[cidx + 1] == ':' || pat[cidx + 1] == '.' || pat[cidx + 1] == '='))
            {
                int nowidx = pat[cidx + 1];
                cidx += 2;
                if(cidx < ofs && pat[cidx])
                {
                    cidx++;
                }
                while(cidx < ofs && pat[cidx] && (pat[cidx - 1] != nowidx || pat[cidx] != ']'))
                {
                    cidx++;
                }
                if(cidx == ofs || !pat[cidx])
                {
                    break;
                }
            }
        }
        if(cidx == ofs || !pat[cidx])
        {
            *step = 1;
            return '[';
        }
        *step = cidx + 1;
        return STRFNM_STATE_BRACKET;
    }
    if(pat[0] == '*')
    {
        return STRFNM_STATE_STAR;
    }
    if(pat[0] == '?')
    {
        return STRFNM_STATE_QUESTION;
    }
escaped:
    if(((unsigned int)pat[0]) >= 128U)
    {
        chv = mbtowc(&wc, pat, ofs);
        if(chv < 0)
        {
            *step = 0;
            return STRFNM_STATE_UNMATCHABLE;
        }
        *step = chv + esc;
        return wc;
    }
    return pat[0];
}

static inline int strfnmint_casefold(int k)
{
    int c = towupper(k);
    if(c == k)
    {
        return towlower(k);
    }
    return c;
}

static inline int strfnmint_dobracket(const char* patptr, int k, int kfold)
{
    int nowch;
    int inv;
    int lch;
    int r1;
    int r2;
    unsigned int rv1a;
    unsigned int rv2a;
    unsigned int rv1b;
    unsigned int rv2b;
    wchar_t wc;
    wchar_t wc2;
    char buf[16];
    const char* p0;
    inv = 0;
    patptr++;
    if(*patptr == '^' || *patptr == '!')
    {
        inv = 1;
        patptr++;
    }
    if(*patptr == ']')
    {
        if(k == ']')
        {
            return !inv;
        }
        patptr++;
    }
    else if(*patptr == '-')
    {
        if(k == '-')
        {
            return !inv;
        }
        patptr++;
    }
    wc = patptr[-1];
    for(; *patptr != ']'; patptr++)
    {
        if(patptr[0] == '-' && patptr[1] != ']')
        {
            lch = mbtowc(&wc2, patptr + 1, 4);
            if(lch < 0)
            {
                return 0;
            }
            if(wc <= wc2)
            {
                rv1a = (unsigned)(k - wc);
                rv1b = ((unsigned int)(wc2 - wc));
                r1 = (rv1a <= rv1b);
                rv2a = ((unsigned)(kfold - wc));
                rv2b = ((unsigned int)(wc2 - wc));
                r2 = (rv2a <= rv2b);
                if(r1 || r2)
                {
                    return !inv;
                }
            }
            patptr += lch - 1;
            continue;
        }
        if(patptr[0] == '[' && (patptr[1] == ':' || patptr[1] == '.' || patptr[1] == '='))
        {
            p0 = patptr + 2;
            nowch = patptr[1];
            patptr += 3;
            while(patptr[-1] != nowch || patptr[0] != ']')
            {
                patptr++;
            }
            if(nowch == ':' && patptr - 1 - p0 < 16)
            {
                memcpy(buf, p0, patptr - 1 - p0);
                buf[patptr - 1 - p0] = 0;
                if(iswctype(k, wctype(buf)) || iswctype(kfold, wctype(buf)))
                {
                    return !inv;
                }
            }
            continue;
        }
        if(((unsigned int)*patptr) < 128U)
        {
            wc = (unsigned char)*patptr;
        }
        else
        {
            lch = mbtowc(&wc, patptr, 4);
            if(lch < 0)
            {
                return 0;
            }
            patptr += lch - 1;
        }
        if(wc == k || wc == kfold)
        {
            return !inv;
        }
    }
    return inv;
}

static inline int strfnmint_perform(const char* pat, size_t ofs, const char* str, size_t n, int flags)
{
    const char* patptr;
    const char* plast;
    const char* endpat;
    const char* s;
    const char* stail;
    const char* endstr;
    size_t pinc;
    size_t sinc;
    size_t tailcnt = 0;
    int c;
    int k;
    int kfold;
    if(flags & STRFNM_FLAG_PERIOD)
    {
        if(*str == '.' && *pat != '.')
        {
            return STRFNM_STATUS_NOMATCH;
        }
    }
    while(true)
    {
        c = strfnmint_nextpattern(pat, ofs, &pinc, flags);
        switch(c)
        {
            case STRFNM_STATE_UNMATCHABLE:
                {
                    return STRFNM_STATUS_NOMATCH;
                }
                break;
            case STRFNM_STATE_STAR:
                {
                    pat++;
                    ofs--;
                }
                break;
            default:
                {
                    k = strfnmint_nextstring(str, n, &sinc);
                    if(k <= 0)
                    {
                        if(c == STRFNM_STATE_END)
                        {
                            return 0;
                        }
                        return STRFNM_STATUS_NOMATCH;
                    }
                    str += sinc;
                    n -= sinc;
                    kfold = k;
                    if(flags & STRFNM_FLAG_CASEFOLD)
                    {
                        kfold = strfnmint_casefold(k);
                    }
                    if(c == STRFNM_STATE_BRACKET)
                    {
                        if(!strfnmint_dobracket(pat, k, kfold))
                        {
                            return STRFNM_STATUS_NOMATCH;
                        }
                    }
                    else if(c != STRFNM_STATE_QUESTION && k != c && kfold != c)
                    {
                        return STRFNM_STATUS_NOMATCH;
                    }
                    pat += pinc;
                    ofs -= pinc;
                }
                continue;
        }
        break;
    }
    /* Compute real pat length if it was initially unknown/-1 */
    ofs = strnlen(pat, ofs);
    endpat = pat + ofs;
    /* Find the last * in pat and count chars needed after it */
    for(patptr = plast = pat; patptr < endpat; patptr += pinc)
    {
        switch(strfnmint_nextpattern(patptr, endpat - patptr, &pinc, flags))
        {
            case STRFNM_STATE_UNMATCHABLE:
                {
                    return STRFNM_STATUS_NOMATCH;
                }
            case STRFNM_STATE_STAR:
                {
                    tailcnt = 0;
                    plast = patptr + 1;
                }
                break;
            default:
                {
                    tailcnt++;
                }
                break;
        }
    }
    /*
    * Past this point we need not check for STRFNM_STATE_UNMATCHABLE in pat,
    * because all of pat has already been parsed once.
    */
    /* Compute real str length if it was initially unknown/-1 */
    n = strnlen(str, n);
    endstr = str + n;
    if(n < tailcnt)
    {
        return STRFNM_STATUS_NOMATCH;
    }
    /*
    * Find the final tailcnt chars of str, accounting for UTF-8.
    * On illegal sequences we may get it wrong, but in that case
    * we necessarily have a matching failure anyway.
    */
    for(s = endstr; s > str && tailcnt; tailcnt--)
    {
        if((((unsigned int)s[-1]) < 128U) || (MB_CUR_MAX == 1))
        {
            s--;
        }
        else
        {
            while((((unsigned char)*--s - 0x80U) < 0x40) && (s > str))
            {
            }
        }
    }
    if(tailcnt)
    {
        return STRFNM_STATUS_NOMATCH;
    }
    stail = s;
    /* Check that the pat and str tails match */
    patptr = plast;
    while(true)
    {
        c = strfnmint_nextpattern(patptr, endpat - patptr, &pinc, flags);
        patptr += pinc;
        if((k = strfnmint_nextstring(s, endstr - s, &sinc)) <= 0)
        {
            if(c != STRFNM_STATE_END)
            {
                return STRFNM_STATUS_NOMATCH;
            }
            break;
        }
        s += sinc;
        kfold = k;
        if(flags & STRFNM_FLAG_CASEFOLD)
        {
            kfold = strfnmint_casefold(k);
        }
        if(c == STRFNM_STATE_BRACKET)
        {
            if(!strfnmint_dobracket(patptr - pinc, k, kfold))
            {
                return STRFNM_STATUS_NOMATCH;
            }
        }
        else if(c != STRFNM_STATE_QUESTION && k != c && kfold != c)
        {
            return STRFNM_STATUS_NOMATCH;
        }
    }

    /* We're all done with the tails now, so throw them out */
    endstr = stail;
    endpat = plast;

    /* Match pattern components until there are none left */
    while(pat < endpat)
    {
        patptr = pat;
        s = str;
        while(true)
        {
            c = strfnmint_nextpattern(patptr, endpat - patptr, &pinc, flags);
            patptr += pinc;
            /* Encountering * completes/commits a component */
            if(c == STRFNM_STATE_STAR)
            {
                pat = patptr;
                str = s;
                break;
            }
            k = strfnmint_nextstring(s, endstr - s, &sinc);
            if(!k)
            {
                return STRFNM_STATUS_NOMATCH;
            }
            kfold = k; 
            if(flags & STRFNM_FLAG_CASEFOLD)
            {
                kfold = strfnmint_casefold(k);
            }
            if(c == STRFNM_STATE_BRACKET)
            {
                if(!strfnmint_dobracket(patptr - pinc, k, kfold))
                {
                    break;
                }
            }
            else if(c != STRFNM_STATE_QUESTION && k != c && kfold != c)
            {
                break;
            }
            s += sinc;
        }
        if(c == STRFNM_STATE_STAR)
        {
            continue;
        }
        /*
        * If we failed, advance str, by 1 char if it's a valid
        * char, or past all invalid bytes otherwise.
        */
        k = strfnmint_nextstring(str, endstr - str, &sinc);
        if(k > 0)
        {
            str += sinc;
        }
        else
        {
            str++;
            while(strfnmint_nextstring(str, endstr - str, &sinc) < 0)
            {
                str++;
            }
        }
    }
    return 0;
}

/**
 * Matches filename.
 *
 *   - `*` for wildcard
 *   - `?` for single character
 *   - `[abc]` to match character within set
 *   - `[!abc]` to match character not within set
 *   - `\*\?\[\]` for escaping above special syntax
 *
 * @see glob()
 */
static inline int strfnmint_textmatchlen(const char* pat, size_t patlen, const char* str, size_t slen, int flags)
{
    const char* s;
    const char* patptr;
    size_t inc;
    int c;
    /*
    * not used right now; assumes both are null-terminated.
    * good luck if they ain't.
    */
    (void)patlen;
    (void)slen;
    if(flags & STRFNM_FLAG_PATHNAME)
    {
        while(true)
        {
            for(s = str; *s && *s != '/'; s++)
            {
            }
            for(patptr = pat; (c = strfnmint_nextpattern(patptr, -1, &inc, flags)) != STRFNM_STATE_END && c != '/'; patptr += inc)
            {
            }
            if(c != *s && (!*s || !(flags & STRFNM_FLAG_LEADING_DIR)))
            {
                return STRFNM_STATUS_NOMATCH;
            }
            if(strfnmint_perform(pat, patptr - pat, str, s - str, flags))
            {
                return STRFNM_STATUS_NOMATCH;
            }
            if(!c)
            {
                return 0;
            }
            str = s + 1;
            pat = patptr + inc;
        }
    }
    else if(flags & STRFNM_FLAG_LEADING_DIR)
    {
        for(s = str; *s; s++)
        {
            if(*s != '/')
            {
                continue;
            }
            if(!strfnmint_perform(pat, -1, str, s - str, flags))
            {
                return 0;
            }
        }
    }
    return strfnmint_perform(pat, -1, str, -1, flags);
}

static inline bool strfnm_match(const char* pat, size_t patlen, const char* str, size_t slen, int flags)
{
    return (strfnmint_textmatchlen(pat, patlen, str, slen, flags) == 0);
}

