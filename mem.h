
#ifndef __libmc_mem_h__
#define __libmc_mem_h__

/*
** Bundled memory allocator.
** Donated to the public domain.
* -------------------------------------
* this is the allocator used by LuaJIT.
* most modifications pertain to portability (added generic ffs/fls support),
* de-macro-ification of some very large macros, renaming types and typedefs, and code cleanliness.
* 
*/


#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <type_traits>
#include <concepts>


/* Bin types, widths and sizes */
#define MCALLOCPOOL_CONST_MAXSIZET (~0)
#define MCALLOCPOOL_CONST_MALLOCALIGNMENT (8)

//#define MCALLOCPOOL_CONST_DEFAULTGRANULARITY (128 * 1024)
#define MCALLOCPOOL_CONST_DEFAULTGRANULARITY (32 * 1024)


//#define MCALLOCPOOL_CONST_DEFAULTTRIMTHRESHOLD (2 * (1024 * 1024))
//#define MCALLOCPOOL_CONST_DEFAULTTRIMTHRESHOLD (1 * (1024 * 1024))
#define MCALLOCPOOL_CONST_DEFAULTTRIMTHRESHOLD (1 * (1024))


//#define MCALLOCPOOL_CONST_DEFAULTMMAPTHRESHOLD (128 * 1024)
//#define MCALLOCPOOL_CONST_DEFAULTMMAPTHRESHOLD (32 * 1024)
#define MCALLOCPOOL_CONST_DEFAULTMMAPTHRESHOLD (1 * 1024)

#define MCALLOCPOOL_CONST_MAXRELEASECHECKRATE (255)

/* ------------------- size_t and alignment properties -------------------- */

/* The byte and bit size of a size_t */
#define MCALLOCPOOL_CONST_SIZETSIZE (sizeof(size_t))
#define MCALLOCPOOL_CONST_SIZETBITSIZE (sizeof(size_t) << 3)

/* Some constants coerced to size_t */
/* Annoying but necessary to avoid errors on some platforms */
#define MCALLOCPOOL_CONST_SIZETONE (1)
#define MCALLOCPOOL_CONST_SIZETTWO (2)
#define MCALLOCPOOL_CONST_TWOSIZETSIZES (MCALLOCPOOL_CONST_SIZETSIZE << 1)
#define MCALLOCPOOL_CONST_FOURSIZETSIZES (MCALLOCPOOL_CONST_SIZETSIZE << 2)
#define MCALLOCPOOL_CONST_SIXSIZETSIZES (MCALLOCPOOL_CONST_FOURSIZETSIZES + MCALLOCPOOL_CONST_TWOSIZETSIZES)


#define MCALLOCPOOL_CONST_NSMALLBINS (32)
#define MCALLOCPOOL_CONST_NTREEBINS (32)
#define MCALLOCPOOL_CONST_SMALLBINSHIFT (3)
#define MCALLOCPOOL_CONST_TREEBINSHIFT (8)
#define MCALLOCPOOL_CONST_MINLARGESIZE (MCALLOCPOOL_CONST_SIZETONE << MCALLOCPOOL_CONST_TREEBINSHIFT)
#define MCALLOCPOOL_CONST_MAXSMALLSIZE (MCALLOCPOOL_CONST_MINLARGESIZE - MCALLOCPOOL_CONST_SIZETONE)
#define MCALLOCPOOL_CONST_MAXSMALLREQUEST (MCALLOCPOOL_CONST_MAXSMALLSIZE - CHUNK_ALIGN_MASK - CHUNK_OVERHEAD)

typedef size_t MCAllocBindex;               /* Described below */
typedef unsigned int MCAllocBinMap;         /* Described below */
typedef unsigned int MCAllocFlag;           /* The type of various bit flag sets */

typedef struct MCAllocPlainChunk MCAllocPlainChunk;
typedef struct MCAllocSegment MCAllocSegment;
typedef struct MCAllocTreeChunk MCAllocTreeChunk;
typedef struct MCAllocState MCAllocState;

struct MCAllocPlainChunk
{
    size_t prev_foot;  /* Size of previous chunk (if free).  */
    size_t head;       /* Size and inuse bits. */
    MCAllocPlainChunk *fd;         /* double links -- used only if free. */
    MCAllocPlainChunk *bk;
};

struct MCAllocTreeChunk
{
    /* The first four fields must be compatible with MCAllocPlainChunk */
    size_t prev_foot;
    size_t head;
    MCAllocTreeChunk *fd;
    MCAllocTreeChunk *bk;
    MCAllocTreeChunk *child[2];
    MCAllocTreeChunk *parent;
    MCAllocBindex index;
};

struct MCAllocSegment
{
    char *base;             /* base address */
    size_t size;             /* allocated size */
    MCAllocSegment *next;   /* ptr to next segment */
};

struct MCAllocState
{
    MCAllocBinMap smallmap;
    MCAllocBinMap treemap;
    size_t dvsize;
    size_t topsize;
    MCAllocPlainChunk* dv;
    MCAllocPlainChunk* top;
    size_t trim_check;
    size_t release_checks;
    MCAllocPlainChunk* smallbins[(MCALLOCPOOL_CONST_NSMALLBINS+1)*2];
    MCAllocTreeChunk* treebins[MCALLOCPOOL_CONST_NTREEBINS];
    MCAllocSegment seg;
};

void *mc_allocpool_create(void);
void mc_allocpool_destroy(void *msp);

void *mc_allocuser_malloc(void *msp, size_t nsize);
void *mc_allocuser_free(void *msp, void *ptr);
void *mc_allocuser_realloc(void *msp, void *ptr, size_t nsize);



void mc_memory_init();
void mc_memory_finish();
void* mc_memory_malloc(size_t sz);
void* mc_memory_realloc(void* p, size_t nsz);
void* mc_memory_calloc(size_t count, size_t typsize);
void mc_memory_free(void* ptr);

extern void* g_mymsp;


template <typename ClassT>
concept MemoryClassHasDestroyFunc = requires(ClassT* ptr) {
    { ClassT::destroy(ptr) } -> std::same_as<void>;
};

class Memory
{
    public:
        template<typename ClassT, typename... ArgsT>
        static inline ClassT* make(ArgsT&&... args)
        {
            ClassT* tmp;
            ClassT* ret;
            tmp = (ClassT*)mc_memory_malloc(sizeof(ClassT));
            ret = new(tmp) ClassT(args...);
            return ret;
        }

        template<typename ClassT, typename... ArgsT>
        static inline void destroy(ClassT* cls, ArgsT&&... args)
        {
            if constexpr (MemoryClassHasDestroyFunc<ClassT>)
            {
                //std::cerr << "Memory::destroy: using destroy" << std::endl;
                ClassT::destroy(cls, args...);
            }
            else
            {
                //std::cerr << "Memory::destroy: using free()" << std::endl;
                mc_memory_free(cls);
            }
        }
};


#endif /* __libmc_mem_h__ */
