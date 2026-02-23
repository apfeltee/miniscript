
#include <stdio.h>
#include <stdlib.h>
#include "mem.h"

#define MC_CONF_USEALLOCATOR 1

void* g_mymsp = nullptr;

void mc_memory_init()
{
    #if defined(MC_CONF_USEALLOCATOR) && (MC_CONF_USEALLOCATOR == 1)
    g_mymsp = nn_allocator_create();
    #endif
}

void mc_memory_finish()
{
    #if defined(MC_CONF_USEALLOCATOR) && (MC_CONF_USEALLOCATOR == 1)
        nn_allocator_destroy(g_mymsp);
    #endif
}


void* mc_memory_malloc(size_t sz)
{
    void* p;
    #if defined(MC_CONF_USEALLOCATOR) && (MC_CONF_USEALLOCATOR == 1)
        p = (void*)nn_allocuser_malloc(g_mymsp, sz);
    #else
        p = (void*)malloc(sz);    
    #endif
    if(p == NULL)
    {
        fprintf(stderr, "mc_memory_malloc: failed to allocate %ld bytes\n", sz);
    }
    return p;
}

void* mc_memory_realloc(void* p, size_t nsz)
{
    void* retp;
    #if defined(MC_CONF_USEALLOCATOR) && (MC_CONF_USEALLOCATOR == 1)
        retp = (void*)nn_allocuser_realloc(g_mymsp, p, nsz);
    #else
        retp = (void*)realloc(p, nsz);    
    #endif
    if(p == NULL)
    {
        fprintf(stderr, "mc_memory_realloc: failed to allocate %ld bytes\n", nsz);
    }
    return retp;
}

void* mc_memory_calloc(size_t count, size_t typsize)
{
    void* p;
    #if defined(MC_CONF_USEALLOCATOR) && (MC_CONF_USEALLOCATOR == 1)
        p = (void*)nn_allocuser_malloc(g_mymsp, count * typsize);
    #else
        p = (void*)calloc(count, typsize);        
    #endif
    if(p == NULL)
    {
        fprintf(stderr, "mc_memory_calloc: failed to allocate %ld bytes\n", count*typsize);
    }
    return p;
}

void mc_memory_free(void* ptr)
{
    #if defined(MC_CONF_USEALLOCATOR) && (MC_CONF_USEALLOCATOR == 1)
        nn_allocuser_free(g_mymsp, ptr);
    #else
        free(ptr);
    #endif
}


