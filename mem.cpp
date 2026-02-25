
#include <stdio.h>
#include <stdlib.h>
#include "mem.h"

#define MC_CONF_USEALLOCATOR 0

void* g_mymsp = nullptr;
static bool g_mempooldisabledbyenv = false;

void mc_memory_init()
{
    #if defined(MC_CONF_USEALLOCATOR) && (MC_CONF_USEALLOCATOR == 1)
    const char* e;
    e = getenv("DISABLEMEMPOOL");
    g_mempooldisabledbyenv = false;
    if(e != nullptr)
    {
        g_mempooldisabledbyenv = ((e[0] == '1') || (e[0] == 'y'));
    }
    if(g_mempooldisabledbyenv)
    {
        fprintf(stderr, "memory: mempool DISABLED\n");
    }
    else
    {
        fprintf(stderr, "memory: mempool ENABLED\n");
    }
    g_mymsp = nn_allocator_create();
    #endif
}

void mc_memory_finish()
{
    #if defined(MC_CONF_USEALLOCATOR) && (MC_CONF_USEALLOCATOR == 1)
        nn_allocator_destroy(g_mymsp);
    #endif
}

void* mc_memory_stdmalloc(size_t sz)
{
    return malloc(sz);
}

void* mc_memory_stdrealloc(void* p, size_t sz)
{
    return realloc(p, sz);
}

void* mc_memory_stdcalloc(size_t count, size_t sz)
{
    return calloc(count, sz);
}

void mc_memory_stdfree(void* p)
{
    free(p);
}

void* mc_memory_malloc(size_t sz)
{
    void* p;
    #if defined(MC_CONF_USEALLOCATOR) && (MC_CONF_USEALLOCATOR == 1)
        if(g_mempooldisabledbyenv)
        {
            p = mc_memory_stdmalloc(sz);
        }
        else
        {
            p = nn_allocuser_malloc(g_mymsp, sz);
        }
    #else
        p = mc_memory_stdmalloc(sz);
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
        if(g_mempooldisabledbyenv)
        {
            retp = mc_memory_stdrealloc(p, nsz);
        }
        else
        {
            retp = nn_allocuser_realloc(g_mymsp, p, nsz);
        }
    #else
        retp = mc_memory_stdrealloc(p, nsz);    
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
        if(g_mempooldisabledbyenv)
        {
            p = mc_memory_stdcalloc(count, typsize);
        }
        else
        {
            p = nn_allocuser_malloc(g_mymsp, count * typsize);
        }

    #else
        p = mc_memory_stdcalloc(count, typsize);        
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
        if(g_mempooldisabledbyenv)
        {
            mc_memory_stdfree(ptr);
        }
        else
        {
            nn_allocuser_free(g_mymsp, ptr);
        }
    #else
        mc_memory_stdfree(ptr);
    #endif
}


