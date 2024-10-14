
#define MC_BASICLIST_INITIALSIZE (32)

mcptrlist_t* mc_ptrlist_make(mcstate_t* state, size_t tsz, bool isptr)
{
    return mc_ptrlist_makecapacity(state, MC_BASICLIST_INITIALSIZE, tsz, isptr);
}

mcptrlist_t* mc_ptrlist_makecapacity(mcstate_t* state, unsigned int capacity, size_t tsz, bool isptr)
{
    bool ok;
    mcptrlist_t* list;
    list = (mcptrlist_t*)mc_allocator_malloc(state, sizeof(mcptrlist_t));
    if(!list)
    {
        return NULL;
    }
    ok = mc_ptrlist_initcapacity(list, state, capacity, tsz, isptr);
    if(!ok)
    {
        mc_memory_free(list);
        return NULL;
    }
    list->pstate = state;
    return list;
}

bool mc_ptrlist_initcapacity(mcptrlist_t* list, mcstate_t* state, unsigned int capacity, size_t tsz, bool isptr)
{
    list->pstate = state;
    list->isptr = isptr;
    if(capacity > 0)
    {
        list->allocdata = (unsigned char*)mc_allocator_malloc(list->pstate, capacity * tsz);
        list->listitems = list->allocdata;
        if(!list->allocdata)
        {
            return false;
        }
    }
    else
    {
        list->allocdata = NULL;
        list->listitems = NULL;
    }
    list->listcapacity = capacity;
    list->listcount = 0;
    list->typesize = tsz;
    list->caplocked = false;
    return true;
}

void mc_ptrlist_deinit(mcptrlist_t* list)
{
    mc_memory_free(list->allocdata);
}

void mc_ptrlist_destroy(mcptrlist_t* list, mcitemdestroyfn_t dfn)
{
    if(!list)
    {
        return;
    }
    if(dfn)
    {
        mc_ptrlist_clearanddestroy(list, dfn);
    }
    mc_ptrlist_deinit(list);
    mc_memory_free(list);
}


void mc_ptrlist_clearanddestroy(mcptrlist_t* list, mcitemdestroyfn_t dfn)
{
    size_t i;
    void* item;
    for(i = 0; i < mc_ptrlist_count(list); i++)
    {
        item = mc_ptrlist_get(list, i);
        dfn(item);
    }
    mc_ptrlist_clear(list);
}



void mc_ptrlist_orphandata(mcptrlist_t* list)
{
    mc_ptrlist_initcapacity(list, list->pstate, 0, list->typesize, list->isptr);
}

mcptrlist_t* mc_ptrlist_copy(mcptrlist_t* list, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
{
    bool ok;
    size_t i;
    void* item;
    void* itemcopy;
    mcptrlist_t* arrcopy;
    if(copyfn)
    {
        arrcopy = mc_ptrlist_make(list->pstate, list->typesize, list->isptr);
        for(i = 0; i < mc_ptrlist_count(list); i++)
        {
            item = (void*)mc_ptrlist_get(list, i);
            itemcopy = item;
            if(copyfn)
            {
                itemcopy = copyfn(item);
            }
            if(item && !itemcopy)
            {
                goto listcopyfailed;
            }
            ok = mc_ptrlist_push(arrcopy, itemcopy);
            if(!ok)
            {
                goto listcopyfailed;
            }
        }
        return arrcopy;
    }
    arrcopy = (mcptrlist_t*)mc_allocator_malloc(list->pstate, sizeof(mcptrlist_t));
    if(!arrcopy)
    {
        return NULL;
    }
    arrcopy->pstate = list->pstate;
    arrcopy->listcapacity = list->listcapacity;
    arrcopy->listcount = list->listcount;
    arrcopy->typesize = list->typesize;
    arrcopy->caplocked = list->caplocked;
    if(list->allocdata)
    {
        arrcopy->allocdata = (unsigned char*)mc_allocator_malloc(list->pstate, list->listcapacity * list->typesize);
        if(!arrcopy->allocdata)
        {
            mc_memory_free(arrcopy);
            return NULL;
        }
        arrcopy->listitems = arrcopy->allocdata;
        memcpy(arrcopy->allocdata, list->listitems, list->listcapacity * list->typesize);
    }
    else
    {
        arrcopy->allocdata = NULL;
        arrcopy->listitems = NULL;
    }
    return arrcopy;
listcopyfailed:
    mc_ptrlist_destroy(arrcopy, dfn);
    return NULL;
}

MC_INLINE bool mc_ptrlist_push(mcptrlist_t* list, void* value)
{
    unsigned int ncap;
    unsigned char* newdata;
    if(list->listcount >= list->listcapacity)
    {
        MC_ASSERT(!list->caplocked);
        if(list->caplocked)
        {
            return false;
        }
        ncap = MC_UTIL_INCCAPACITY(list->listcapacity);
        #if 1
            newdata = (unsigned char*)mc_allocator_realloc(list->pstate, list->allocdata, ncap * list->typesize);
            if(!newdata)
            {
                return false;
            }
        #else
            /*
            newdata = (unsigned char*)mc_allocator_malloc(list->pstate, ncap * list->typesize);
            if(!newdata)
            {
                return false;
            }
            memcpy(newdata, list->listitems, list->listcount * list->typesize);
            mc_memory_free(list->allocdata);
            */
        #endif
        list->allocdata = newdata;
        list->listitems = list->allocdata;
        list->listcapacity = ncap;
    }
    if(value)
    {
        if(list->isptr)
        {
            ((void**)list->listitems)[list->listcount] = value;
        }
        else
        {
            memcpy(list->listitems + (list->listcount * list->typesize), value, list->typesize);
        }
    }
    list->listcount++;
    return true;
}

MC_INLINE bool mc_ptrlist_pop(mcptrlist_t* list, void** outvalue)
{
    void* res;
    if(list->listcount <= 0)
    {
        return false;
    }
    if(outvalue)
    {
        res = mc_ptrlist_get(list, list->listcount - 1);
        if(list->isptr)
        {
            *outvalue = res;
        }
        else
        {
            memcpy(*outvalue, res, list->typesize);
        }
    }
    mc_ptrlist_removeat(list, list->listcount - 1);
    return true;
}

MC_INLINE void* mc_ptrlist_popret(mcptrlist_t* list)
{
    void* dest;
    if(mc_ptrlist_pop(list, &dest))
    {
        return dest;
    }
    return NULL;
}

MC_INLINE void* mc_ptrlist_top(mcptrlist_t* list)
{
    if(list->listcount <= 0)
    {
        return NULL;
    }
    return mc_ptrlist_get(list, list->listcount - 1);
}

MC_INLINE bool mc_ptrlist_set(mcptrlist_t* list, unsigned int ix, void* value)
{
    size_t offset;
    if(ix >= list->listcount)
    {
        MC_ASSERT(false);
        return false;
    }
    if(list->isptr)
    {
        ((void**)list->listitems)[ix] = value;
    }
    else
    {
        offset = ix * list->typesize;
        memmove(list->listitems + offset, value, list->typesize);
    }
    return true;
}

MC_INLINE void* mc_ptrlist_get(mcptrlist_t* list, unsigned int ix)
{
    size_t offset;
    if(ix >= list->listcount)
    {
        MC_ASSERT(false);
        return NULL;
    }
    if(list->isptr)
    {
        return ((void**)list->listitems)[ix];
    }
    offset = ix * list->typesize;
    return list->listitems + offset;
}

MC_INLINE size_t mc_ptrlist_count(mcptrlist_t* list)
{
    if(!list)
    {
        return 0;
    }
    return list->listcount;
}

bool mc_ptrlist_removeat(mcptrlist_t* list, unsigned int ix)
{
    size_t tomovebytes;
    void* dest;
    void* src;
    if(ix >= list->listcount)
    {
        return false;
    }
    if(!list->isptr)
    {
        if(ix == 0)
        {
            list->listitems += list->typesize;
            list->listcapacity--;
            list->listcount--;
            return true;
        }
        if(ix == (list->listcount - 1))
        {
            list->listcount--;
            return true;
        }
        tomovebytes = (list->listcount - 1 - ix) * list->typesize;
        dest = list->listitems + (ix * list->typesize);
        src = list->listitems + ((ix + 1) * list->typesize);
        memmove(dest, src, tomovebytes);
    }
    list->listcount--;
    return true;
}

void mc_ptrlist_clear(mcptrlist_t* list)
{
    list->listcount = 0;
}

void* mc_ptrlist_data(mcptrlist_t* list)
{
    return list->listitems;
}

MC_INLINE void* mc_ptrlist_getconst(mcptrlist_t* list, unsigned int ix)
{
    size_t offset;
    if(ix >= list->listcount)
    {
        MC_ASSERT(false);
        return NULL;
    }
    offset = ix * list->typesize;
    return list->listitems + offset;
}

int mc_ptrlist_getindex(mcptrlist_t* list, void* ptr)
{
    size_t i;
    for(i = 0; i < mc_ptrlist_count(list); i++)
    {
        if(mc_ptrlist_getconst(list, i) == ptr)
        {
            return i;
        }
    }
    return -1;
}

bool mc_ptrlist_removeitem(mcptrlist_t* list, void* ptr)
{
    int ix;
    ix = mc_ptrlist_getindex(list, ptr);
    if(ix < 0)
    {
        return false;
    }
    return mc_ptrlist_removeat(list, ix);
}

bool mc_ptrlist_contains(mcptrlist_t* list, void* ptr)
{
    return mc_ptrlist_getindex(list, ptr) >= 0;
}

