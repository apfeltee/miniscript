/*
** Bundled memory allocator.
**
** Beware: this is a HEAVILY CUSTOMIZED version of dlmalloc.
** The original bears the following remark:
**
**   This is a version (aka dlmalloc) of malloc/free/realloc written by
**   Doug Lea and released to the public domain, as explained at
**   http://creativecommons.org/licenses/publicdomain.
**
**   * Version pre-2.8.4 Wed Mar 29 19:46:29 2006    (dl at gee)
**
** No additional copyright is claimed over the customizations.
** Please do NOT bother the original author about this version here!
**
** If you want to use dlmalloc in another project, you should get
** the original from: ftp://gee.cs.oswego.edu/pub/misc/
** For thread-safe derivatives, take a look at:
** - ptmalloc: http://www.malloc.de/
** - nedmalloc: http://www.nedprod.com/programs/portable/nedmalloc/
*/

/* To get the mremap prototype. Must be defined before any system includes. */
#if defined(__linux__) && !defined(_GNU_SOURCE)
    #define _GNU_SOURCE
#endif

/* Target architectures. */
enum
{
    MCALLOCPOOL_ARCH_X86 = 1,
    MCALLOCPOOL_ARCH_X64 = 2,
    MCALLOCPOOL_ARCH_ARM = 3,
};


/* Select native OS if no target OS defined. */
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    #define MCALLOCPOOL_TARGET_WINDOWS
#elif defined(__linux__)
    #define MCALLOCPOOL_TARGET_LINUX
#elif defined(__MACH__) && defined(__APPLE__)
    #define MCALLOCPOOL_TARGET_OSX
#elif(defined(__sun__) && defined(__svr4__)) || defined(__CYGWIN__)
    #define MCALLOCPOOL_TARGET_POSIX
#else
    #define MCALLOCPOOL_TARGET_OTHER
#endif


#if defined(MCALLOCPOOL_TARGET_WINDOWS)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #if defined(_MSC_VER)
        #include <intrin.h>
    #endif
#else
    #include <errno.h>
    #include <sys/mman.h>
#endif

#ifdef __CELLOS_LV2__
    #define MCALLOCPOOL_TARGET_PS3 1
#endif

#ifdef __ORBIS__
    #define MCALLOCPOOL_TARGET_PS4 1
    #undef NULL
    #define NULL ((void*)0)
#endif

/* Set target architecture properties. */

#if defined(__i386) || defined(__i386__) || defined(_M_IX86)
    #define MCALLOCPOOL_ARCH_IS32BIT
#elif defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
    #define MCALLOCPOOL_ARCH_IS64BIT
#elif defined(__arm__) || defined(__arm) || defined(__ARM__) || defined(__ARM)
    #define MCALLOCPOOL_ARCH_IS32BIT
#else
    #error "No support for this architecture (yet)"
#endif

#ifndef MCALLOCPOOL_PAGESIZE
    #define MCALLOCPOOL_PAGESIZE 4096
#endif


#include "mem.h"

#if !defined(MC_LIKELY)
    #if defined(__GNUC__)
        #define MC_LIKELY(x) __builtin_expect(!!(x), 1)
        #define MC_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #else
        #define MC_LIKELY(x) x
        #define MC_UNLIKELY(x) x
    #endif
#endif

/* The bit mask value corresponding to MCALLOCPOOL_CONST_MALLOCALIGNMENT */
#define CHUNK_ALIGN_MASK (MCALLOCPOOL_CONST_MALLOCALIGNMENT - MCALLOCPOOL_CONST_SIZETONE)

/* the number of bytes to offset an address to align it */
#define align_offset(A)                            \
    ((((size_t)(A) & CHUNK_ALIGN_MASK) == 0) ? 0 : \
                                               ((MCALLOCPOOL_CONST_MALLOCALIGNMENT - ((size_t)(A) & CHUNK_ALIGN_MASK)) & CHUNK_ALIGN_MASK))

/* -------------------------- MMAP support ------------------------------- */

#define MFAIL ((void*)(MCALLOCPOOL_CONST_MAXSIZET))
#define CMFAIL ((char*)(MFAIL)) /* defined for convenience */

#define IS_DIRECT_BIT (MCALLOCPOOL_CONST_SIZETONE)

#if defined(MCALLOCPOOL_TARGET_WINDOWS)
    #if defined(MCALLOCPOOL_ARCH_IS64BIT)
        /* Number of top bits of the lower 32 bits of an address that must be zero.
        ** Apparently 0 gives us full 64 bit addresses and 1 gives us the lower 2GB.
        */
        #define NTAVM_ZEROBITS 1
    #else
        #define INIT_MMAP() ((void)0)
    #endif
#else
    #define MMAP_PROT (PROT_READ | PROT_WRITE)
    #if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
        #define MAP_ANONYMOUS MAP_ANON
    #endif
    #define MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)

    #if defined(MCALLOCPOOL_ARCH_IS64BIT)
    /* 64 bit mode needs special support for allocating memory in the lower 2GB. */

        #if defined(MAP_32BIT)
        #elif defined(MCALLOCPOOL_TARGET_OSX) || defined(MCALLOCPOOL_TARGET_PS4) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__)

            /* OSX and FreeBSD mmap() use a naive first-fit linear search.
            ** That's perfect for us. Except that -pagezero_size must be set for OSX,
            ** otherwise the lower 4GB are blocked. And the 32GB RLIMIT_DATA needs
            ** to be reduced to 250MB on FreeBSD.
            */
            #if defined(MCALLOCPOOL_TARGET_OSX)
                #define MMAP_REGION_START ((uintptr_t)0x10000)
            #elif defined(MCALLOCPOOL_TARGET_PS4)
                #define MMAP_REGION_START ((uintptr_t)0x4000)
            #else
                #define MMAP_REGION_START ((uintptr_t)0x10000000)
            #endif
            #define MMAP_REGION_END ((uintptr_t)0x80000000)

            #if(defined(__FreeBSD__) || defined(__FreeBSD_kernel__)) && !defined(MCALLOCPOOL_TARGET_PS4)
                #include <sys/resource.h>
            #endif
        #else
            #error "NYI: need an equivalent of MAP_32BIT for this 64 bit OS"
        #endif
    #endif
    #define INIT_MMAP() ((void)0)
    #define DIRECT_MMAP(s) CALL_MMAP(s)
    #if defined(MCALLOCPOOL_TARGET_LINUX)
        #define CALL_MREMAP(addr, osz, nsz, mv) CALL_MREMAP_((addr), (osz), (nsz), (mv))
        #define CALL_MREMAP_NOMOVE 0
        /* #define CALL_MREMAP_MAYMOVE 1 */
        #if defined(MCALLOCPOOL_ARCH_IS64BIT)
            #define CALL_MREMAP_MV CALL_MREMAP_NOMOVE
        #else
            #define CALL_MREMAP_MV CALL_MREMAP_MAYMOVE
        #endif
    #endif
#endif

#ifndef CALL_MREMAP
    #define CALL_MREMAP(addr, osz, nsz, mv) ((void)osz, MFAIL)
#endif

/* ------------------- Chunks sizes and alignments ----------------------- */

#define MCHUNK_SIZE (sizeof(MCAllocPlainChunk))

#define CHUNK_OVERHEAD (MCALLOCPOOL_CONST_SIZETSIZE)

/* Direct chunks need a second word of overhead ... */
#define DIRECT_CHUNK_OVERHEAD (MCALLOCPOOL_CONST_TWOSIZETSIZES)
/* ... and additional padding for fake next-chunk at foot */
#define DIRECT_FOOT_PAD (MCALLOCPOOL_CONST_FOURSIZETSIZES)

/* The smallest size we can malloc is an aligned minimal chunk */
#define MIN_CHUNK_SIZE \
    ((MCHUNK_SIZE + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)

/* conversion from malloc headers to user pointers, and back */
#define chunk2mem(p) ((void*)((char*)(p) + MCALLOCPOOL_CONST_TWOSIZETSIZES))
#define mem2chunk(mem) ((MCAllocPlainChunk*)((char*)(mem) - MCALLOCPOOL_CONST_TWOSIZETSIZES))
/* chunk associated with aligned address A */
#define align_as_chunk(A) (MCAllocPlainChunk*)((A) + align_offset(chunk2mem(A)))

/* Bounds on request (not chunk) sizes. */
#define MAX_REQUEST ((~MIN_CHUNK_SIZE + 1) << 2)
#define MIN_REQUEST (MIN_CHUNK_SIZE - CHUNK_OVERHEAD - MCALLOCPOOL_CONST_SIZETONE)

/* pad request bytes into a usable size */
#define pad_request(req) \
    (((req) + CHUNK_OVERHEAD + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)

/* pad request, checking for minimum (but not maximum) */
#define request2size(req) \
    (((req) < MIN_REQUEST) ? MIN_CHUNK_SIZE : pad_request(req))

/* ------------------ Operations on head and foot fields ----------------- */

#define PINUSE_BIT (MCALLOCPOOL_CONST_SIZETONE)
#define CINUSE_BIT (MCALLOCPOOL_CONST_SIZETTWO)
#define INUSE_BITS (PINUSE_BIT | CINUSE_BIT)

/* Head value for fenceposts */
#define FENCEPOST_HEAD (INUSE_BITS | MCALLOCPOOL_CONST_SIZETSIZE)

/* extraction of fields from head words */
#define cinuse(p) ((p)->head & CINUSE_BIT)
#define pinuse(p) ((p)->head & PINUSE_BIT)
#define chunksize(p) ((p)->head & ~(INUSE_BITS))

#define clear_pinuse(p) ((p)->head &= ~PINUSE_BIT)

/* Treat space at ptr +/- offset as a chunk */
#define chunk_plus_offset(p, s) ((MCAllocPlainChunk*)(((char*)(p)) + (s)))
#define chunk_minus_offset(p, s) ((MCAllocPlainChunk*)(((char*)(p)) - (s)))

/* Ptr to next or previous physical MCAllocPlainChunk. */
#define next_chunk(p) ((MCAllocPlainChunk*)(((char*)(p)) + ((p)->head & ~INUSE_BITS)))

/* Get/set size at footer */
#define set_foot(p, s) (((MCAllocPlainChunk*)((char*)(p) + (s)))->prev_foot = (s))

/* Set size, pinuse bit, and foot */
#define set_size_and_pinuse_of_free_chunk(p, s) \
    ((p)->head = (s | PINUSE_BIT), set_foot(p, s))

/* Set size, pinuse bit, foot, and clear next pinuse */
#define set_free_with_pinuse(p, s, n) \
    (clear_pinuse(n), set_size_and_pinuse_of_free_chunk(p, s))

#define is_direct(p) \
    (!((p)->head & PINUSE_BIT) && ((p)->prev_foot & IS_DIRECT_BIT))

/* Get the internal overhead associated with chunk p */
#define overhead_for(p) \
    (is_direct(p) ? DIRECT_CHUNK_OVERHEAD : CHUNK_OVERHEAD)

/* ---------------------- Overlaid data structures ----------------------- */

/* A little helper macro for trees */
#define mc_allocpool_leftmostchild(t) ((t)->child[0] != 0 ? (t)->child[0] : (t)->child[1])

#define mc_allocpool_isinitialized(mst) ((mst)->top != 0)

/* -------------------------- system alloc setup ------------------------- */

/* page-align a size */
#define mc_allocpool_pagealign(sz) \
    (((sz) + (MCALLOCPOOL_PAGESIZE - MCALLOCPOOL_CONST_SIZETONE)) & ~(MCALLOCPOOL_PAGESIZE - MCALLOCPOOL_CONST_SIZETONE))

/* granularity-align a size */
#define mc_allocpool_granularityalign(sz)                    \
    (((sz) + (MCALLOCPOOL_CONST_DEFAULTGRANULARITY - MCALLOCPOOL_CONST_SIZETONE)) & ~(MCALLOCPOOL_CONST_DEFAULTGRANULARITY - MCALLOCPOOL_CONST_SIZETONE))

#if defined(MCALLOCPOOL_TARGET_WINDOWS)
    #define mmap_align(sz) mc_allocpool_granularityalign(sz)
#else
    #define mmap_align(sz) mc_allocpool_pagealign(sz)
#endif

/*  True if segment segm holds address A */
#define mc_allocpool_segmentholds(segm, A) \
    ((char*)(A) >= segm->base && (char*)(A) < segm->base + segm->size)

#if defined(MCALLOCPOOL_TARGET_WINDOWS)
    #if defined(MCALLOCPOOL_ARCH_IS64BIT)
        /* Undocumented, but hey, that's what we all love so much about Windows. */
        typedef long (*PNTAVM)(HANDLE handle, void** addr, ULONG zbits, size_t* size, ULONG alloctype, ULONG prot);
        static PNTAVM ntavm;

        static void INIT_MMAP(void)
        {
            ntavm = (PNTAVM)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtAllocateVirtualMemory");
        }

        /* Win64 32 bit MMAP via NtAllocateVirtualMemory. */
        static void* CALL_MMAP(size_t size)
        {
            DWORD olderr = GetLastError();
            void* ptr = NULL;
            long st = ntavm(INVALID_HANDLE_VALUE, &ptr, NTAVM_ZEROBITS, &size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            SetLastError(olderr);
            return st == 0 ? ptr : MFAIL;
        }

        /* For direct MMAP, use MEM_TOP_DOWN to minimize interference */
        static void* DIRECT_MMAP(size_t size)
        {
            long st;
            DWORD olderr;
            void* ptr;
            olderr = GetLastError();
            ptr = NULL;
            st = ntavm(INVALID_HANDLE_VALUE, &ptr, NTAVM_ZEROBITS, &size, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
            SetLastError(olderr);
            return st == 0 ? ptr : MFAIL;
        }
    #else
        /* Win32 MMAP via VirtualAlloc */
        static void* CALL_MMAP(size_t size)
        {
            DWORD olderr = GetLastError();
            void* ptr = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            SetLastError(olderr);
            return ptr ? ptr : MFAIL;
        }

        /* For direct MMAP, use MEM_TOP_DOWN to minimize interference */
        static void* DIRECT_MMAP(size_t size)
        {
            DWORD olderr = GetLastError();
            void* ptr = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN,
                                     PAGE_READWRITE);
            SetLastError(olderr);
            return ptr ? ptr : MFAIL;
        }
    #endif

    /* This function supports releasing coalesed segments */
    static int CALL_MUNMAP(void* ptr, size_t size)
    {
        char* cptr;
        DWORD olderr;
        MEMORY_BASIC_INFORMATION minfo;
        olderr = GetLastError();
        cptr = (char*)ptr;
        while(size)
        {
            if(VirtualQuery(cptr, &minfo, sizeof(minfo)) == 0)
            {
                return -1;
            }
            if(minfo.BaseAddress != cptr || minfo.AllocationBase != cptr || minfo.State != MEM_COMMIT || minfo.RegionSize > size)
            {
                return -1;
            }
            if(VirtualFree(cptr, 0, MEM_RELEASE) == 0)
            {
                return -1;
            }
            cptr += minfo.RegionSize;
            size -= minfo.RegionSize;
        }
        SetLastError(olderr);
        return 0;
    }
#else
    #if defined(MCALLOCPOOL_ARCH_IS64BIT)
    /* 64 bit mode needs special support for allocating memory in the lower 2GB. */
        #if defined(MAP_32BIT)
            /* Actually this only gives us max. 1GB in current Linux kernels. */
            static void* CALL_MMAP(size_t size)
            {
                int olderr = errno;
                void* ptr = mmap(NULL, size, MMAP_PROT, MAP_32BIT | MMAP_FLAGS, -1, 0);
                errno = olderr;
                return ptr;
            }
        #elif defined(MCALLOCPOOL_TARGET_OSX) || defined(MCALLOCPOOL_TARGET_PS4) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun__)
            static void* CALL_MMAP(size_t size)
            {
                int olderr;
                int retry;
                void* p;
                #if(defined(__FreeBSD__) || defined(__FreeBSD_kernel__)) && !defined(MCALLOCPOOL_TARGET_PS4)
                    struct rlimit rlim;
                #endif
                static uintptr_t allochint = MMAP_REGION_START;
                #if(defined(__FreeBSD__) || defined(__FreeBSD_kernel__)) && !defined(MCALLOCPOOL_TARGET_PS4)
                    static int rlimit_modified = 0;
                #endif
                olderr = errno;
                /* Hint for next allocation. Doesn't need to be thread-safe. */
                retry = 0;
                #if(defined(__FreeBSD__) || defined(__FreeBSD_kernel__)) && !defined(MCALLOCPOOL_TARGET_PS4)
                    if(MC_UNLIKELY(rlimit_modified == 0))
                    {
                        rlim.rlim_cur = rlim.rlim_max = MMAP_REGION_START;
                        setrlimit(RLIMIT_DATA, &rlim); /* Ignore result. May fail below. */
                        rlimit_modified = 1;
                    }
                #endif
                for(;;)
                {
                    p = mmap((void*)allochint, size, MMAP_PROT, MMAP_FLAGS, -1, 0);
                    if((uintptr_t)p >= MMAP_REGION_START && (uintptr_t)p + size < MMAP_REGION_END)
                    {
                        allochint = (uintptr_t)p + size;
                        errno = olderr;
                        return p;
                    }
                    if(p != CMFAIL)
                    {
                        munmap(p, size);
                    }
                    #ifdef __sun__
                        allochint += 0x1000000; /* Need near-exhaustive linear scan. */
                        if(allochint + size < MMAP_REGION_END)
                        {
                            continue;
                        }
                    #endif
                    if(retry)
                    {
                        break;
                    }
                    retry = 1;
                    allochint = MMAP_REGION_START;
                }
                errno = olderr;
                return CMFAIL;
            }
        #else
            #error "NYI: need an equivalent of MAP_32BIT for this 64 bit OS"
        #endif
    #else
        /* 32 bit mode is easy. */
        static void* CALL_MMAP(size_t size)
        {
            int olderr;
            void* ptr;
            olderr = errno;
            ptr = mmap(NULL, size, MMAP_PROT, MMAP_FLAGS, -1, 0);
            errno = olderr;
            return ptr;
        }
    #endif
    static int CALL_MUNMAP(void* ptr, size_t size)
    {
        int ret;
        int olderr;
        olderr = errno;
        ret = munmap(ptr, size);
        errno = olderr;
        return ret;
    }

    #if defined(MCALLOCPOOL_TARGET_LINUX)
        /* Need to define _GNU_SOURCE to get the mremap prototype. */
        static void* CALL_MREMAP_(void* ptr, size_t osz, size_t nsz, int flags)
        {
            int olderr;
            olderr = errno;
            ptr = mremap(ptr, osz, nsz, flags);
            errno = olderr;
            return ptr;
        }
    #endif
#endif

static int mc_allocpool_nativebitscanreverse(uint64_t x)
{
    static const char nnalloc_debruijntable[64] = {
        0,  47, 1,  56, 48, 27, 2,  60, 57, 49, 41, 37, 28, 16, 3,  61,
        54, 58, 35, 52, 50, 42, 21, 44, 38, 32, 29, 23, 17, 11, 4,  62,
        46, 55, 26, 59, 40, 36, 15, 53, 34, 51, 20, 43, 31, 22, 10, 45,
        25, 39, 14, 33, 19, 30, 9,  24, 13, 18, 8,  12, 7,  6,  5,  63,
    };
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return nnalloc_debruijntable[(x * 0x03f79d71b4cb0a89) >> 58];
}

static int mc_allocpool_nativebitscanforward(uint64_t x)
{
    uint32_t l, r;
    x &= -x;
    l = x | x >> 32;
    r = !!(x >> 32), r <<= 1;
    r += !!((l & 0xffff0000)), r <<= 1;
    r += !!((l & 0xff00ff00)), r <<= 1;
    r += !!((l & 0xf0f0f0f0)), r <<= 1;
    r += !!((l & 0xcccccccc)), r <<= 1;
    r += !!((l & 0xaaaaaaaa));
    return r;
}

/* Return segment holding given address */
static MCAllocSegment* mc_allocpool_segmentholding(MCAllocState* m, char* addr)
{
    MCAllocSegment* sp = &m->seg;
    for(;;)
    {
        if(addr >= sp->base && addr < sp->base + sp->size)
            return sp;
        if((sp = sp->next) == 0)
            return 0;
    }
}

/* Return true if segment contains a segment link */
static int mc_allocpool_hassegmentlink(MCAllocState* m, MCAllocSegment* ss)
{
    MCAllocSegment* sp = &m->seg;
    for(;;)
    {
        if((char*)sp >= ss->base && (char*)sp < ss->base + ss->size)
            return 1;
        if((sp = sp->next) == 0)
            return 0;
    }
}

/*
  mc_allocpool_topfootsize is padding at the end of a segment, including space
  that may be needed to place segment records and fenceposts when new
  noncontiguous segments are added.
*/
#define mc_allocpool_topfootsize() \
    (align_offset(chunk2mem(0)) + pad_request(sizeof(MCAllocSegment)) + MIN_CHUNK_SIZE)

/* ---------------------------- Indexing Bins ---------------------------- */

#define mc_allocpool_issmall(s) (((s) >> MCALLOCPOOL_CONST_SMALLBINSHIFT) < MCALLOCPOOL_CONST_NSMALLBINS)
#define mc_allocpool_smallindex(s) ((s) >> MCALLOCPOOL_CONST_SMALLBINSHIFT)
#define mc_allocpool_smallindex2size(i) ((i) << MCALLOCPOOL_CONST_SMALLBINSHIFT)

/* addressing by index. See above about smallbin repositioning */
#define mc_allocpool_smallbinat(mst, i) ((MCAllocPlainChunk*)((char*)&((mst)->smallbins[(i) << 1])))
#define mc_allocpool_treebinat(mst, i) (&((mst)->treebins[i]))

/* assign tree index for size sz to variable I */
#define mc_allocpool_computetreeindex(sz, I)                                               \
    {                                                                          \
        unsigned int X = (unsigned int)(sz >> MCALLOCPOOL_CONST_TREEBINSHIFT);                   \
        if(X == 0)                                                             \
        {                                                                      \
            I = 0;                                                             \
        }                                                                      \
        else if(X > 0xFFFF)                                                    \
        {                                                                      \
            I = MCALLOCPOOL_CONST_NTREEBINS - 1;                                                 \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            unsigned int K = mc_allocpool_nativebitscanreverse(X);                                        \
            I = (MCAllocBindex)((K << 1) + ((sz >> (K + (MCALLOCPOOL_CONST_TREEBINSHIFT - 1)) & 1))); \
        }                                                                      \
    }

/* Shift placing maximum resolved bit in a treebin at i as sign bit */
#define mc_allocpool_leftshiftfortreeindex(i) \
    ((i == MCALLOCPOOL_CONST_NTREEBINS - 1) ? 0 :     \
                            ((MCALLOCPOOL_CONST_SIZETBITSIZE - MCALLOCPOOL_CONST_SIZETONE) - (((i) >> 1) + MCALLOCPOOL_CONST_TREEBINSHIFT - 2)))

/* ------------------------ Operations on bin maps ----------------------- */

/* bit corresponding to given index */
#define mc_allocpool_idx2bit(i) ((MCAllocBinMap)(1) << (i))

/* Mark/Clear bits with given index */
#define mc_allocpool_marksmallmap(mst, i) ((mst)->smallmap |= mc_allocpool_idx2bit(i))
#define mc_allocpool_clearsmallmap(mst, i) ((mst)->smallmap &= ~mc_allocpool_idx2bit(i))
#define mc_allocpool_smallmapismarked(mst, i) ((mst)->smallmap & mc_allocpool_idx2bit(i))

#define mc_allocpool_marktreemap(mst, i) ((mst)->treemap |= mc_allocpool_idx2bit(i))
#define mc_allocpool_cleartreemap(mst, i) ((mst)->treemap &= ~mc_allocpool_idx2bit(i))
#define mc_allocpool_treemapismarked(mst, i) ((mst)->treemap & mc_allocpool_idx2bit(i))

/* mask with all bits to left of least bit of x on */
#define left_bits(x) ((x << 1) | (~(x << 1) + 1))

/* Set cinuse bit and pinuse bit of next chunk */
#define mc_allocpool_setinuse(mst, p, s)                                    \
    ((p)->head = (((p)->head & PINUSE_BIT) | s | CINUSE_BIT), \
     ((MCAllocPlainChunk*)(((char*)(p)) + (s)))->head |= PINUSE_BIT)

/* Set cinuse and pinuse of this chunk and pinuse of next chunk */
#define mc_allocpool_setinuseandpinuse(mst, p, s)           \
    ((p)->head = (s | PINUSE_BIT | CINUSE_BIT), \
     ((MCAllocPlainChunk*)(((char*)(p)) + (s)))->head |= PINUSE_BIT)

/* Set size, cinuse and pinuse bit of this chunk */
#define mc_allocpool_setsizeandpinuseofinusechunk(mst, p, s) \
    ((p)->head = (s | PINUSE_BIT | CINUSE_BIT))

/* ----------------------- Operations on smallbins ----------------------- */

/* Link a free chunk into a smallbin  */
#define mc_allocpool_insertsmallchunk(mst, P, sz)      \
    {                                    \
        MCAllocBindex I = mc_allocpool_smallindex(sz);     \
        MCAllocPlainChunk* B = mc_allocpool_smallbinat(mst, I); \
        MCAllocPlainChunk* F = B;                 \
        if(!mc_allocpool_smallmapismarked(mst, I))    \
        { \
            mc_allocpool_marksmallmap(mst, I);         \
        } \
        else \
        { \
            F = B->fd;                   \
        } \
        B->fd = P;                       \
        F->bk = P;                       \
        P->fd = F;                       \
        P->bk = B;                       \
    }

/* Unlink a chunk from a smallbin  */
#define mc_allocpool_unlinksmallchunk(mst, P, sz)  \
    {                                \
        MCAllocPlainChunk* F = P->fd;         \
        MCAllocPlainChunk* B = P->bk;         \
        MCAllocBindex I = mc_allocpool_smallindex(sz); \
        if(F == B)                   \
        {                            \
            mc_allocpool_clearsmallmap(mst, I);    \
        }                            \
        else                         \
        {                            \
            F->bk = B;               \
            B->fd = F;               \
        }                            \
    }

/* Unlink the first chunk from a smallbin */
#define mc_allocpool_unlinkfirstsmallchunk(mst, B, P, I) \
    {                                        \
        MCAllocPlainChunk* F = P->fd;                 \
        if(B == F)                           \
        {                                    \
            mc_allocpool_clearsmallmap(mst, I);            \
        }                                    \
        else                                 \
        {                                    \
            B->fd = F;                       \
            F->bk = B;                       \
        }                                    \
    }

/* Replace dv node, binning the old one */
/* Used only when dvsize known to be small */
#define mc_allocpool_replacedv(mst, P, sz)                 \
    {                                       \
        size_t DVS = mst->dvsize;             \
        if(DVS != 0)                        \
        {                                   \
            MCAllocPlainChunk* DV = mst->dv;           \
            mc_allocpool_insertsmallchunk(mst, DV, DVS); \
        }                                   \
        mst->dvsize = sz;                      \
        mst->dv = P;                          \
    }

/* ------------------------- Operations on trees ------------------------- */

/* Insert chunk into tree */
static void mc_allocpool_insertlargechunk(MCAllocState* mst, MCAllocTreeChunk* tchunk, size_t psz)
{
    MCAllocTreeChunk** hp;
    MCAllocBindex i;
    mc_allocpool_computetreeindex(psz, i);
    hp = mc_allocpool_treebinat(mst, i);
    tchunk->index = i;
    tchunk->child[0] = tchunk->child[1] = 0;
    if(!mc_allocpool_treemapismarked(mst, i))
    {
        mc_allocpool_marktreemap(mst, i);
        *hp = tchunk;
        tchunk->parent = (MCAllocTreeChunk*)hp;
        tchunk->fd = tchunk->bk = tchunk;
    }
    else
    {
        MCAllocTreeChunk* t = *hp;
        size_t k = psz << mc_allocpool_leftshiftfortreeindex(i);
        for(;;)
        {
            if(chunksize(t) != psz)
            {
                MCAllocTreeChunk** cchunk = &(t->child[(k >> (MCALLOCPOOL_CONST_SIZETBITSIZE - MCALLOCPOOL_CONST_SIZETONE)) & 1]);
                k <<= 1;
                if(*cchunk != 0)
                {
                    t = *cchunk;
                }
                else
                {
                    *cchunk = tchunk;
                    tchunk->parent = t;
                    tchunk->fd = tchunk->bk = tchunk;
                    break;
                }
            }
            else
            {
                MCAllocTreeChunk* F = t->fd;
                t->fd = F->bk = tchunk;
                tchunk->fd = F;
                tchunk->bk = t;
                tchunk->parent = 0;
                break;
            }
        }
    }
}

static void mc_allocpool_unlinklargechunk(MCAllocState* mst, MCAllocTreeChunk* tchunk)
{
    MCAllocTreeChunk* xp = tchunk->parent;
    MCAllocTreeChunk* r;
    if(tchunk->bk != tchunk)
    {
        MCAllocTreeChunk* f = tchunk->fd;
        r = tchunk->bk;
        f->bk = r;
        r->fd = f;
    }
    else
    {
        MCAllocTreeChunk** rp;
        if(((r = *(rp = &(tchunk->child[1]))) != 0) || ((r = *(rp = &(tchunk->child[0]))) != 0))
        {
            MCAllocTreeChunk** cp;
            while((*(cp = &(r->child[1])) != 0) || (*(cp = &(r->child[0])) != 0))
            {
                r = *(rp = cp);
            }
            *rp = 0;
        }
    }
    if(xp != 0)
    {
        MCAllocTreeChunk** hp = mc_allocpool_treebinat(mst, tchunk->index);
        if(tchunk == *hp)
        {
            if((*hp = r) == 0)
                mc_allocpool_cleartreemap(mst, tchunk->index);
        }
        else
        {
            if(xp->child[0] == tchunk)
                xp->child[0] = r;
            else
                xp->child[1] = r;
        }
        if(r != 0)
        {
            MCAllocTreeChunk* c0;
            MCAllocTreeChunk* c1;
            r->parent = xp;
            if((c0 = tchunk->child[0]) != 0)
            {
                r->child[0] = c0;
                c0->parent = r;
            }
            if((c1 = tchunk->child[1]) != 0)
            {
                r->child[1] = c1;
                c1->parent = r;
            }
        }
    }
}

/* Relays to large vs small bin operations */

#define mc_allocpool_insertchunk(mst, P, psz)          \
    if(mc_allocpool_issmall(psz))                    \
    {                                  \
        mc_allocpool_insertsmallchunk(mst, P, psz)    \
    }                                  \
    else                               \
    {                                  \
        MCAllocTreeChunk* TP = (MCAllocTreeChunk*)(P); \
        mc_allocpool_insertlargechunk(mst, TP, psz);  \
    }

#define mc_allocpool_unlinkchunk(mst, P, psz)          \
    if(mc_allocpool_issmall(psz))                    \
    {                                  \
        mc_allocpool_unlinksmallchunk(mst, P, psz)    \
    }                                  \
    else                               \
    {                                  \
        MCAllocTreeChunk* TP = (MCAllocTreeChunk*)(P); \
        mc_allocpool_unlinklargechunk(mst, TP);     \
    }

/* -----------------------  Direct-mmapping chunks ----------------------- */

static void* mc_allocpool_directalloc(size_t nb)
{
    size_t mmsize = mmap_align(nb + MCALLOCPOOL_CONST_SIXSIZETSIZES + CHUNK_ALIGN_MASK);
    if(MC_LIKELY(mmsize > nb))
    { /* Check for wrap around 0 */
        char* mm = (char*)(DIRECT_MMAP(mmsize));
        if(mm != CMFAIL)
        {
            size_t offset = align_offset(chunk2mem(mm));
            size_t psize = mmsize - offset - DIRECT_FOOT_PAD;
            MCAllocPlainChunk* p = (MCAllocPlainChunk*)(mm + offset);
            p->prev_foot = offset | IS_DIRECT_BIT;
            p->head = psize | CINUSE_BIT;
            chunk_plus_offset(p, psize)->head = FENCEPOST_HEAD;
            chunk_plus_offset(p, psize + MCALLOCPOOL_CONST_SIZETSIZE)->head = 0;
            return chunk2mem(p);
        }
    }
    return NULL;
}

static MCAllocPlainChunk* mc_allocpool_directresize(MCAllocPlainChunk* oldp, size_t nb)
{
    size_t oldsize = chunksize(oldp);
    if(mc_allocpool_issmall(nb)) /* Can't shrink direct regions below small size */
        return NULL;
    /* Keep old chunk if big enough but not too big */
    if(oldsize >= nb + MCALLOCPOOL_CONST_SIZETSIZE && (oldsize - nb) <= (MCALLOCPOOL_CONST_DEFAULTGRANULARITY >> 1))
    {
        return oldp;
    }
    else
    {
        size_t offset = oldp->prev_foot & ~IS_DIRECT_BIT;
        size_t oldmmsize = oldsize + offset + DIRECT_FOOT_PAD;
        size_t newmmsize = mmap_align(nb + MCALLOCPOOL_CONST_SIXSIZETSIZES + CHUNK_ALIGN_MASK);
        char* cp = (char*)CALL_MREMAP((char*)oldp - offset,
                                      oldmmsize, newmmsize, CALL_MREMAP_MV);
        if(cp != CMFAIL)
        {
            MCAllocPlainChunk* newp = (MCAllocPlainChunk*)(cp + offset);
            size_t psize = newmmsize - offset - DIRECT_FOOT_PAD;
            newp->head = psize | CINUSE_BIT;
            chunk_plus_offset(newp, psize)->head = FENCEPOST_HEAD;
            chunk_plus_offset(newp, psize + MCALLOCPOOL_CONST_SIZETSIZE)->head = 0;
            return newp;
        }
    }
    return NULL;
}

/* -------------------------- mspace management -------------------------- */

/* Initialize top chunk and its size */
static void mc_allocpool_inittop(MCAllocState* m, MCAllocPlainChunk* p, size_t psize)
{
    /* Ensure alignment */
    size_t offset = align_offset(chunk2mem(p));
    p = (MCAllocPlainChunk*)((char*)p + offset);
    psize -= offset;

    m->top = p;
    m->topsize = psize;
    p->head = psize | PINUSE_BIT;
    /* set size of fake trailing chunk holding overhead space only once */
    chunk_plus_offset(p, psize)->head = mc_allocpool_topfootsize();
    m->trim_check = MCALLOCPOOL_CONST_DEFAULTTRIMTHRESHOLD; /* reset on each update */
}

/* Initialize bins for a new state that is otherwise zeroed out */
static void mc_allocpool_initbins(MCAllocState* m)
{
    /* Establish circular links for smallbins */
    MCAllocBindex i;
    for(i = 0; i < MCALLOCPOOL_CONST_NSMALLBINS; i++)
    {
        MCAllocPlainChunk* bin = mc_allocpool_smallbinat(m, i);
        bin->fd = bin->bk = bin;
    }
}

/* Allocate chunk and prepend remainder with chunk in successor base. */
static void* mc_allocpool_prependalloc(MCAllocState* m, char* newbase, char* oldbase, size_t nb)
{
    MCAllocPlainChunk* p = align_as_chunk(newbase);
    MCAllocPlainChunk* oldfirst = align_as_chunk(oldbase);
    size_t psize = (size_t)((char*)oldfirst - (char*)p);
    MCAllocPlainChunk* q = chunk_plus_offset(p, nb);
    size_t qsize = psize - nb;
    mc_allocpool_setsizeandpinuseofinusechunk(m, p, nb);

    /* consolidate remainder with first chunk of old base */
    if(oldfirst == m->top)
    {
        size_t tsize = m->topsize += qsize;
        m->top = q;
        q->head = tsize | PINUSE_BIT;
    }
    else if(oldfirst == m->dv)
    {
        size_t dsize = m->dvsize += qsize;
        m->dv = q;
        set_size_and_pinuse_of_free_chunk(q, dsize);
    }
    else
    {
        if(!cinuse(oldfirst))
        {
            size_t nsize = chunksize(oldfirst);
            mc_allocpool_unlinkchunk(m, oldfirst, nsize);
            oldfirst = chunk_plus_offset(oldfirst, nsize);
            qsize += nsize;
        }
        set_free_with_pinuse(q, qsize, oldfirst);
        mc_allocpool_insertchunk(m, q, qsize);
    }

    return chunk2mem(p);
}

/* Add a segment to hold a new noncontiguous region */
static void mc_allocpool_addsegment(MCAllocState* m, char* tbase, size_t tsize)
{
    /* Determine locations and sizes of segment, fenceposts, old top */
    char* old_top = (char*)m->top;
    MCAllocSegment* oldsp = mc_allocpool_segmentholding(m, old_top);
    char* old_end = oldsp->base + oldsp->size;
    size_t ssize = pad_request(sizeof(MCAllocSegment));
    char* rawsp = old_end - (ssize + MCALLOCPOOL_CONST_FOURSIZETSIZES + CHUNK_ALIGN_MASK);
    size_t offset = align_offset(chunk2mem(rawsp));
    char* asp = rawsp + offset;
    char* csp = (asp < (old_top + MIN_CHUNK_SIZE)) ? old_top : asp;
    MCAllocPlainChunk* sp = (MCAllocPlainChunk*)csp;
    MCAllocSegment* ss = (MCAllocSegment*)(chunk2mem(sp));
    MCAllocPlainChunk* tnext = chunk_plus_offset(sp, ssize);
    MCAllocPlainChunk* p = tnext;

    /* reset top to new space */
    mc_allocpool_inittop(m, (MCAllocPlainChunk*)tbase, tsize - mc_allocpool_topfootsize());

    /* Set up segment record */
    mc_allocpool_setsizeandpinuseofinusechunk(m, sp, ssize);
    *ss = m->seg; /* Push current record */
    m->seg.base = tbase;
    m->seg.size = tsize;
    m->seg.next = ss;

    /* Insert trailing fenceposts */
    for(;;)
    {
        MCAllocPlainChunk* nextp = chunk_plus_offset(p, MCALLOCPOOL_CONST_SIZETSIZE);
        p->head = FENCEPOST_HEAD;
        if((char*)(&(nextp->head)) < old_end)
            p = nextp;
        else
            break;
    }

    /* Insert the rest of old top into a bin as an ordinary free chunk */
    if(csp != old_top)
    {
        MCAllocPlainChunk* q = (MCAllocPlainChunk*)old_top;
        size_t psize = (size_t)(csp - old_top);
        MCAllocPlainChunk* tn = chunk_plus_offset(q, psize);
        set_free_with_pinuse(q, psize, tn);
        mc_allocpool_insertchunk(m, q, psize);
    }
}

/* -------------------------- System allocation -------------------------- */

static void* mc_allocpool_allocsys(MCAllocState* m, size_t nb)
{
    char* tbase = CMFAIL;
    size_t tsize = 0;

    /* Directly map large chunks */
    if(MC_UNLIKELY(nb >= MCALLOCPOOL_CONST_DEFAULTMMAPTHRESHOLD))
    {
        void* mem = mc_allocpool_directalloc(nb);
        if(mem != 0)
            return mem;
    }

    {
        size_t req = nb + mc_allocpool_topfootsize() + MCALLOCPOOL_CONST_SIZETONE;
        size_t rsize = mc_allocpool_granularityalign(req);
        if(MC_LIKELY(rsize > nb))
        { /* Fail if wraps around zero */
            char* mp = (char*)(CALL_MMAP(rsize));
            if(mp != CMFAIL)
            {
                tbase = mp;
                tsize = rsize;
            }
        }
    }

    if(tbase != CMFAIL)
    {
        MCAllocSegment* sp = &m->seg;
        /* Try to merge with an existing segment */
        while(sp != 0 && tbase != sp->base + sp->size)
            sp = sp->next;
        if(sp != 0 && mc_allocpool_segmentholds(sp, m->top))
        { /* append */
            sp->size += tsize;
            mc_allocpool_inittop(m, m->top, m->topsize + tsize);
        }
        else
        {
            sp = &m->seg;
            while(sp != 0 && sp->base != tbase + tsize)
                sp = sp->next;
            if(sp != 0)
            {
                char* oldbase = sp->base;
                sp->base = tbase;
                sp->size += tsize;
                return mc_allocpool_prependalloc(m, tbase, oldbase, nb);
            }
            else
            {
                mc_allocpool_addsegment(m, tbase, tsize);
            }
        }

        if(nb < m->topsize)
        { /* Allocate from new or extended top space */
            size_t rsize = m->topsize -= nb;
            MCAllocPlainChunk* p = m->top;
            MCAllocPlainChunk* r = m->top = chunk_plus_offset(p, nb);
            r->head = rsize | PINUSE_BIT;
            mc_allocpool_setsizeandpinuseofinusechunk(m, p, nb);
            return chunk2mem(p);
        }
    }

    return NULL;
}

/* -----------------------  system deallocation -------------------------- */

/* Unmap and unlink any mmapped segments that don't contain used chunks */
static size_t mc_allocpool_releaseunusedsegments(MCAllocState* m)
{
    size_t released = 0;
    size_t nsegs = 0;
    MCAllocSegment* pred = &m->seg;
    MCAllocSegment* sp = pred->next;
    while(sp != 0)
    {
        char* base = sp->base;
        size_t size = sp->size;
        MCAllocSegment* next = sp->next;
        nsegs++;
        {
            MCAllocPlainChunk* p = align_as_chunk(base);
            size_t psize = chunksize(p);
            /* Can unmap if first chunk holds entire segment and not pinned */
            if(!cinuse(p) && (char*)p + psize >= base + size - mc_allocpool_topfootsize())
            {
                MCAllocTreeChunk* tp = (MCAllocTreeChunk*)p;
                if(p == m->dv)
                {
                    m->dv = 0;
                    m->dvsize = 0;
                }
                else
                {
                    mc_allocpool_unlinklargechunk(m, tp);
                }
                if(CALL_MUNMAP(base, size) == 0)
                {
                    released += size;
                    /* unlink obsoleted record */
                    sp = pred;
                    sp->next = next;
                }
                else
                { /* back out if cannot unmap */
                    mc_allocpool_insertlargechunk(m, tp, psize);
                }
            }
        }
        pred = sp;
        sp = next;
    }
    /* Reset check counter */
    m->release_checks = nsegs > MCALLOCPOOL_CONST_MAXRELEASECHECKRATE ?
                        nsegs :
                        MCALLOCPOOL_CONST_MAXRELEASECHECKRATE;
    return released;
}

static int mc_allocpool_alloctrim(MCAllocState* m, size_t pad)
{
    size_t released = 0;
    if(pad < MAX_REQUEST && mc_allocpool_isinitialized(m))
    {
        pad += mc_allocpool_topfootsize(); /* ensure enough room for segment overhead */

        if(m->topsize > pad)
        {
            /* Shrink top space in granularity-size units, keeping at least one */
            size_t unit = MCALLOCPOOL_CONST_DEFAULTGRANULARITY;
            size_t extra = ((m->topsize - pad + (unit - MCALLOCPOOL_CONST_SIZETONE)) / unit - MCALLOCPOOL_CONST_SIZETONE) * unit;
            MCAllocSegment* sp = mc_allocpool_segmentholding(m, (char*)m->top);

            if(sp->size >= extra && !mc_allocpool_hassegmentlink(m, sp))
            { /* can't shrink if pinned */
                size_t newsize = sp->size - extra;
                /* Prefer mremap, fall back to munmap */
                if((CALL_MREMAP(sp->base, sp->size, newsize, CALL_MREMAP_NOMOVE) != MFAIL) || (CALL_MUNMAP(sp->base + newsize, extra) == 0))
                {
                    released = extra;
                }
            }

            if(released != 0)
            {
                sp->size -= released;
                mc_allocpool_inittop(m, m->top, m->topsize - released);
            }
        }

        /* Unmap any unused mmapped segments */
        released += mc_allocpool_releaseunusedsegments(m);

        /* On failure, disable autotrim to avoid repeated failed future calls */
        if(released == 0 && m->topsize > m->trim_check)
            m->trim_check = MCALLOCPOOL_CONST_MAXSIZET;
    }

    return (released != 0) ? 1 : 0;
}

/* ---------------------------- malloc support --------------------------- */

/* allocate a large request from the best fitting chunk in a treebin */
static void* mc_allocpool_tmalloclarge(MCAllocState* m, size_t nb)
{
    MCAllocTreeChunk* v = 0;
    size_t rsize = ~nb + 1; /* Unsigned negation */
    MCAllocTreeChunk* t;
    MCAllocBindex idx;
    mc_allocpool_computetreeindex(nb, idx);

    if((t = *mc_allocpool_treebinat(m, idx)) != 0)
    {
        /* Traverse tree for this bin looking for node with size == nb */
        size_t sizebits = nb << mc_allocpool_leftshiftfortreeindex(idx);
        MCAllocTreeChunk* rst = 0; /* The deepest untaken right subtree */
        for(;;)
        {
            MCAllocTreeChunk* rt;
            size_t trem = chunksize(t) - nb;
            if(trem < rsize)
            {
                v = t;
                if((rsize = trem) == 0)
                    break;
            }
            rt = t->child[1];
            t = t->child[(sizebits >> (MCALLOCPOOL_CONST_SIZETBITSIZE - MCALLOCPOOL_CONST_SIZETONE)) & 1];
            if(rt != 0 && rt != t)
                rst = rt;
            if(t == 0)
            {
                t = rst; /* set t to least subtree holding sizes > nb */
                break;
            }
            sizebits <<= 1;
        }
    }

    if(t == 0 && v == 0)
    { /* set t to root of next non-empty treebin */
        MCAllocBinMap leftbits = left_bits(mc_allocpool_idx2bit(idx)) & m->treemap;
        if(leftbits != 0)
            t = *mc_allocpool_treebinat(m, mc_allocpool_nativebitscanforward(leftbits));
    }

    while(t != 0)
    { /* find smallest of tree or subtree */
        size_t trem = chunksize(t) - nb;
        if(trem < rsize)
        {
            rsize = trem;
            v = t;
        }
        t = mc_allocpool_leftmostchild(t);
    }

    /*  If dv is a better fit, return NULL so malloc will use it */
    if(v != 0 && rsize < (size_t)(m->dvsize - nb))
    {
        MCAllocPlainChunk* r = chunk_plus_offset(v, nb);
        mc_allocpool_unlinklargechunk(m, v);
        if(rsize < MIN_CHUNK_SIZE)
        {
            mc_allocpool_setinuseandpinuse(m, v, (rsize + nb));
        }
        else
        {
            mc_allocpool_setsizeandpinuseofinusechunk(m, v, nb);
            set_size_and_pinuse_of_free_chunk(r, rsize);
            mc_allocpool_insertchunk(m, r, rsize);
        }
        return chunk2mem(v);
    }
    return NULL;
}

/* allocate a small request from the best fitting chunk in a treebin */
static void* mc_allocpool_tmallocsmall(MCAllocState* m, size_t nb)
{
    MCAllocTreeChunk* t;
    MCAllocTreeChunk* v;
    MCAllocPlainChunk* r;
    size_t rsize;
    MCAllocBindex i = mc_allocpool_nativebitscanforward(m->treemap);

    v = t = *mc_allocpool_treebinat(m, i);
    rsize = chunksize(t) - nb;

    while((t = mc_allocpool_leftmostchild(t)) != 0)
    {
        size_t trem = chunksize(t) - nb;
        if(trem < rsize)
        {
            rsize = trem;
            v = t;
        }
    }

    r = chunk_plus_offset(v, nb);
    mc_allocpool_unlinklargechunk(m, v);
    if(rsize < MIN_CHUNK_SIZE)
    {
        mc_allocpool_setinuseandpinuse(m, v, (rsize + nb));
    }
    else
    {
        mc_allocpool_setsizeandpinuseofinusechunk(m, v, nb);
        set_size_and_pinuse_of_free_chunk(r, rsize);
        mc_allocpool_replacedv(m, r, rsize);
    }
    return chunk2mem(v);
}

/* ----------------------------------------------------------------------- */

void* mc_allocpool_create(void)
{
    size_t tsize = MCALLOCPOOL_CONST_DEFAULTGRANULARITY;
    char* tbase;
    INIT_MMAP();
    tbase = (char*)(CALL_MMAP(tsize));
    if(tbase != CMFAIL)
    {
        size_t msize = pad_request(sizeof(MCAllocState));
        MCAllocPlainChunk* mn;
        MCAllocPlainChunk* msp = align_as_chunk(tbase);
        MCAllocState* m = (MCAllocState*)(chunk2mem(msp));
        memset(m, 0, msize);
        msp->head = (msize | PINUSE_BIT | CINUSE_BIT);
        m->seg.base = tbase;
        m->seg.size = tsize;
        m->release_checks = MCALLOCPOOL_CONST_MAXRELEASECHECKRATE;
        mc_allocpool_initbins(m);
        mn = next_chunk(mem2chunk(m));
        mc_allocpool_inittop(m, mn, (size_t)((tbase + tsize) - (char*)mn) - mc_allocpool_topfootsize());
        return m;
    }
    return NULL;
}

void mc_allocpool_destroy(void* msp)
{
    MCAllocState* ms = (MCAllocState*)msp;
    MCAllocSegment* sp = &ms->seg;
    while(sp != 0)
    {
        char* base = sp->base;
        size_t size = sp->size;
        sp = sp->next;
        CALL_MUNMAP(base, size);
    }
}

void* mc_allocuser_malloc(void* msp, size_t nsize)
{
    MCAllocState* ms = (MCAllocState*)msp;
    void* mem;
    size_t nb;
    if(nsize <= MCALLOCPOOL_CONST_MAXSMALLREQUEST)
    {
        MCAllocBindex idx;
        MCAllocBinMap smallbits;
        nb = (nsize < MIN_REQUEST) ? MIN_CHUNK_SIZE : pad_request(nsize);
        idx = mc_allocpool_smallindex(nb);
        smallbits = ms->smallmap >> idx;

        if((smallbits & 0x3) != 0)
        { /* Remainderless fit to a smallbin. */
            MCAllocPlainChunk* b;
            MCAllocPlainChunk* p;
            idx += ~smallbits & 1; /* Uses next bin if idx empty */
            b = mc_allocpool_smallbinat(ms, idx);
            p = b->fd;
            mc_allocpool_unlinkfirstsmallchunk(ms, b, p, idx);
            mc_allocpool_setinuseandpinuse(ms, p, mc_allocpool_smallindex2size(idx));
            mem = chunk2mem(p);
            return mem;
        }
        else if(nb > ms->dvsize)
        {
            if(smallbits != 0)
            { /* Use chunk in next nonempty smallbin */
                MCAllocPlainChunk* b;
                MCAllocPlainChunk* p;
                MCAllocPlainChunk* r;
                size_t rsize;
                MCAllocBinMap leftbits = (smallbits << idx) & left_bits(mc_allocpool_idx2bit(idx));
                MCAllocBindex i = mc_allocpool_nativebitscanforward(leftbits);
                b = mc_allocpool_smallbinat(ms, i);
                p = b->fd;
                mc_allocpool_unlinkfirstsmallchunk(ms, b, p, i);
                rsize = mc_allocpool_smallindex2size(i) - nb;
                /* Fit here cannot be remainderless if 4byte sizes */
                if(MCALLOCPOOL_CONST_SIZETSIZE != 4 && rsize < MIN_CHUNK_SIZE)
                {
                    mc_allocpool_setinuseandpinuse(ms, p, mc_allocpool_smallindex2size(i));
                }
                else
                {
                    mc_allocpool_setsizeandpinuseofinusechunk(ms, p, nb);
                    r = chunk_plus_offset(p, nb);
                    set_size_and_pinuse_of_free_chunk(r, rsize);
                    mc_allocpool_replacedv(ms, r, rsize);
                }
                mem = chunk2mem(p);
                return mem;
            }
            else if(ms->treemap != 0 && (mem = mc_allocpool_tmallocsmall(ms, nb)) != 0)
            {
                return mem;
            }
        }
    }
    else if(nsize >= MAX_REQUEST)
    {
        nb = MCALLOCPOOL_CONST_MAXSIZET; /* Too big to allocate. Force failure (in sys alloc) */
    }
    else
    {
        nb = pad_request(nsize);
        if(ms->treemap != 0 && (mem = mc_allocpool_tmalloclarge(ms, nb)) != 0)
        {
            return mem;
        }
    }

    if(nb <= ms->dvsize)
    {
        size_t rsize = ms->dvsize - nb;
        MCAllocPlainChunk* p = ms->dv;
        if(rsize >= MIN_CHUNK_SIZE)
        { /* split dv */
            MCAllocPlainChunk* r = ms->dv = chunk_plus_offset(p, nb);
            ms->dvsize = rsize;
            set_size_and_pinuse_of_free_chunk(r, rsize);
            mc_allocpool_setsizeandpinuseofinusechunk(ms, p, nb);
        }
        else
        { /* exhaust dv */
            size_t dvs = ms->dvsize;
            ms->dvsize = 0;
            ms->dv = 0;
            mc_allocpool_setinuseandpinuse(ms, p, dvs);
        }
        mem = chunk2mem(p);
        return mem;
    }
    else if(nb < ms->topsize)
    { /* Split top */
        size_t rsize = ms->topsize -= nb;
        MCAllocPlainChunk* p = ms->top;
        MCAllocPlainChunk* r = ms->top = chunk_plus_offset(p, nb);
        r->head = rsize | PINUSE_BIT;
        mc_allocpool_setsizeandpinuseofinusechunk(ms, p, nb);
        mem = chunk2mem(p);
        return mem;
    }
    return mc_allocpool_allocsys(ms, nb);
}

void* mc_allocuser_free(void* msp, void* ptr)
{
    if(ptr != 0)
    {
        MCAllocPlainChunk* p = mem2chunk(ptr);
        MCAllocState* fm = (MCAllocState*)msp;
        size_t psize = chunksize(p);
        MCAllocPlainChunk* next = chunk_plus_offset(p, psize);
        if(!pinuse(p))
        {
            size_t prevsize = p->prev_foot;
            if((prevsize & IS_DIRECT_BIT) != 0)
            {
                prevsize &= ~IS_DIRECT_BIT;
                psize += prevsize + DIRECT_FOOT_PAD;
                CALL_MUNMAP((char*)p - prevsize, psize);
                return NULL;
            }
            else
            {
                MCAllocPlainChunk* prev = chunk_minus_offset(p, prevsize);
                psize += prevsize;
                p = prev;
                /* consolidate backward */
                if(p != fm->dv)
                {
                    mc_allocpool_unlinkchunk(fm, p, prevsize);
                }
                else if((next->head & INUSE_BITS) == INUSE_BITS)
                {
                    fm->dvsize = psize;
                    set_free_with_pinuse(p, psize, next);
                    return NULL;
                }
            }
        }
        if(!cinuse(next))
        { /* consolidate forward */
            if(next == fm->top)
            {
                size_t tsize = fm->topsize += psize;
                fm->top = p;
                p->head = tsize | PINUSE_BIT;
                if(p == fm->dv)
                {
                    fm->dv = 0;
                    fm->dvsize = 0;
                }
                if(tsize > fm->trim_check)
                    mc_allocpool_alloctrim(fm, 0);
                return NULL;
            }
            else if(next == fm->dv)
            {
                size_t dsize = fm->dvsize += psize;
                fm->dv = p;
                set_size_and_pinuse_of_free_chunk(p, dsize);
                return NULL;
            }
            else
            {
                size_t nsize = chunksize(next);
                psize += nsize;
                mc_allocpool_unlinkchunk(fm, next, nsize);
                set_size_and_pinuse_of_free_chunk(p, psize);
                if(p == fm->dv)
                {
                    fm->dvsize = psize;
                    return NULL;
                }
            }
        }
        else
        {
            set_free_with_pinuse(p, psize, next);
        }

        if(mc_allocpool_issmall(psize))
        {
            mc_allocpool_insertsmallchunk(fm, p, psize);
        }
        else
        {
            MCAllocTreeChunk* tp = (MCAllocTreeChunk*)p;
            mc_allocpool_insertlargechunk(fm, tp, psize);
            if(--fm->release_checks == 0)
                mc_allocpool_releaseunusedsegments(fm);
        }
    }
    return NULL;
}

void* mc_allocuser_realloc(void* msp, void* ptr, size_t nsize)
{
    if(nsize >= MAX_REQUEST)
    {
        return NULL;
    }
    else
    {
        MCAllocState* m = (MCAllocState*)msp;
        MCAllocPlainChunk* oldp = mem2chunk(ptr);
        size_t oldsize = chunksize(oldp);
        MCAllocPlainChunk* next = chunk_plus_offset(oldp, oldsize);
        MCAllocPlainChunk* newp = 0;
        size_t nb = request2size(nsize);

        /* Try to either shrink or extend into top. Else malloc-copy-free */
        if(is_direct(oldp))
        {
            newp = mc_allocpool_directresize(oldp, nb); /* this may return NULL. */
        }
        else if(oldsize >= nb)
        { /* already big enough */
            size_t rsize = oldsize - nb;
            newp = oldp;
            if(rsize >= MIN_CHUNK_SIZE)
            {
                MCAllocPlainChunk* rem = chunk_plus_offset(newp, nb);
                mc_allocpool_setinuse(m, newp, nb);
                mc_allocpool_setinuse(m, rem, rsize);
                mc_allocuser_free(m, chunk2mem(rem));
            }
        }
        else if(next == m->top && oldsize + m->topsize > nb)
        {
            /* Expand into top */
            size_t newsize = oldsize + m->topsize;
            size_t newtopsize = newsize - nb;
            MCAllocPlainChunk* newtop = chunk_plus_offset(oldp, nb);
            mc_allocpool_setinuse(m, oldp, nb);
            newtop->head = newtopsize | PINUSE_BIT;
            m->top = newtop;
            m->topsize = newtopsize;
            newp = oldp;
        }

        if(newp != 0)
        {
            return chunk2mem(newp);
        }
        else
        {
            void* newmem = mc_allocuser_malloc(m, nsize);
            if(newmem != 0)
            {
                size_t oc = oldsize - overhead_for(oldp);
                memcpy(newmem, ptr, oc < nsize ? oc : nsize);
                mc_allocuser_free(m, ptr);
            }
            return newmem;
        }
    }
}


