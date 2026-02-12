
#include <stdio.h>
#include <stdlib.h>
#include "mem.h"

void* mc_memory_malloc(size_t sz)
{
    void* p;
    p = (void*)malloc(sz);
    if(p == NULL)
    {
        fprintf(stderr, "mc_memory_malloc: failed to allocate %ld bytes\n", sz);
    }
    return p;
}

void* mc_memory_realloc(void* p, size_t nsz)
{
    void* retp;
    retp = (void*)realloc(p, nsz);
    if(p == NULL)
    {
        fprintf(stderr, "mc_memory_realloc: failed to allocate %ld bytes\n", nsz);
    }
    return retp;
}

void* mc_memory_calloc(size_t count, size_t typsize)
{
    void* p;
    p = (void*)calloc(count, typsize);
    if(p == NULL)
    {
        fprintf(stderr, "mc_memory_calloc: failed to allocate %ld bytes\n", count*typsize);
    }
    return p;
}

void mc_memory_free(void* ptr)
{
    free(ptr);
}
