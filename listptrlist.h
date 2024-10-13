
void mc_ptrlist_setempty(mcptrlist_t* list)
{
    /*
    if((list->listcapacity > 0) && (list->listitems != NULL))
    {
        memset(list->listitems, 0, sizeof(void*) * list->listcapacity);
    }
    */
    list->listcount = 0;
    list->listcapacity = 0;
}

mcptrlist_t* mc_ptrlist_make(mcstate_t* state, size_t capacity, size_t tsz)
{
    mcptrlist_t* list;
    list = (mcptrlist_t*)mc_allocator_malloc(state, sizeof(mcptrlist_t));
    list->pstate = state;
    list->listcount = 0;
    list->livecount = 0;
    list->listcapacity = 0;
    list->listitems = NULL;
    list->typesize = tsz;
    mc_ptrlist_setempty(list);
    if(capacity > 0)
    {
        mc_ptrlist_ensurecapacity(list, capacity, NULL);
    }
    return list;
}

void mc_ptrlist_destroy(mcptrlist_t* list, mcitemdestroyfn_t dfn)
{
    if(list == NULL)
    {
        return;
    }
    if(dfn)
    {
        mc_ptrlist_clearanddestroy(list, dfn);
    }
    list->listcount = 0;
    list->livecount = 0;
    mc_memory_free(list->listitems);
    list->listitems = NULL;
    mc_memory_free(list);
    list = NULL;
}

void mc_ptrlist_ensurecapacity(mcptrlist_t* list, size_t needsize, void* fillval)
{
    size_t i;
    size_t oldcap;
    if(list->listcapacity < needsize)
    {
        oldcap = list->listcapacity;
        list->listcapacity = needsize;
        if(list->listitems == NULL)
        {
            list->listitems = (void**)mc_allocator_malloc(list->pstate, list->typesize * needsize);
        }
        else
        {
            list->listitems = (void**)mc_allocator_realloc(list->pstate, list->listitems, list->typesize * needsize);
        }
        for(i = oldcap; i < needsize; i++)
        {
            list->listitems[i] = fillval;
        }
    }
}

void mc_ptrlist_orphandata(mcptrlist_t* list)
{
    fprintf(stderr, "ERROR: orphandata is NOT implemented!!!\n");
    //mc_ptrlist_initcapacity(list, list->pstate, 0, list->typesize);
}

bool mc_ptrlist_removeat(mcptrlist_t* list, unsigned int ix)
{
    size_t tomovebytes;
    void* dest;
    void* src;
    if(list->typesize != sizeof(void*))
    {
        if(ix >= list->listcount)
        {
            return false;
        }
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

MC_INLINE size_t mc_ptrlist_count(mcptrlist_t* list)
{
    return list->listcount;
}

MC_INLINE size_t mc_ptrlist_capacity(mcptrlist_t* list)
{
    return list->listcount;
}

MC_INLINE void* mc_ptrlist_data(mcptrlist_t* list)
{
    return list->listitems;
}

MC_INLINE void mc_ptrlist_clear(mcptrlist_t* list)
{
    list->listcount = 0;
}

MC_INLINE bool mc_ptrlist_set(mcptrlist_t* list, unsigned int ix, void* value)
{
    size_t offset;
    if(ix >= list->listcount)
    {
        MC_ASSERT(false);
        return false;
    }
    if(mc_util_likely(list->typesize == sizeof(void*)))
    {
        list->listitems[ix] = value;
    }
    else
    {
        offset = ix * list->typesize;
        memmove(list->listitems + offset, value, list->typesize);
    }
    return true;
}

MC_INLINE bool mc_ptrlist_push(mcptrlist_t* list, void* value)
{
    size_t oldcap;
    if(list->listcapacity < list->listcount + 1)
    {
        oldcap = list->listcapacity;
        list->listcapacity = MC_UTIL_INCCAPACITY(oldcap);
        //list->listcapacity = (oldcap == 0) ? (list->typesize * 4) : (oldcap * 2);
        if(list->listitems == NULL)
        {
            list->listitems = (void**)mc_allocator_malloc(list->pstate, list->typesize * list->listcapacity);
        }
        else
        {
            list->listitems = (void**)mc_allocator_realloc(list->pstate, list->listitems, list->typesize * list->listcapacity);
        }
    }
    if(mc_util_likely(list->typesize == sizeof(void*)))
    {
        list->listitems[list->listcount] = value;
    }
    else
    {
        memcpy(list->listitems + (list->listcount * list->typesize), value, list->typesize);
    }
    list->listcount++;
    list->livecount++;
    return true;
}

MC_INLINE void* mc_ptrlist_get(mcptrlist_t* list, unsigned int ix)
{
    size_t offset;
    if(mc_util_likely(list->typesize == sizeof(void*)))
    {
        return list->listitems[ix];
    }
    offset = ix * list->typesize;
    return list->listitems + offset;

}

MC_INLINE void* mc_ptrlist_top(mcptrlist_t* list)
{
    if(list->listcount <= 0)
    {
        return NULL;
    }
    return mc_ptrlist_get(list, list->listcount - 1);
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
        if(mc_util_likely(list->typesize == sizeof(void*)))
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

MC_INLINE bool mc_ptrlist_popret(mcptrlist_t* list)
{
    void* dest;
    if(mc_ptrlist_pop(list, &dest))
    {
        return dest;
    }
    return NULL;
}

mcptrlist_t* mc_ptrlist_copy(mcptrlist_t* list, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
{
    bool ok;
    size_t i;
    void* item;
    void* itemcopy;
    mcptrlist_t* arrcopy;
    arrcopy = mc_ptrlist_make(list->pstate, list->listcapacity, list->typesize);
    if(!arrcopy)
    {
        return NULL;
    }
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
listcopyfailed:
    mc_ptrlist_destroy(arrcopy, dfn);
    return NULL;
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


int mc_ptrlist_getindex(mcptrlist_t* list, void* ptr)
{
    size_t i;
    for(i = 0; i < mc_ptrlist_count(list); i++)
    {
        if(mc_ptrlist_get(list, i) == ptr)
        {
            return i;
        }
    }
    return -1;
}

bool mc_ptrlist_contains(mcptrlist_t* list, void* ptr)
{
    return mc_ptrlist_getindex(list, ptr) >= 0;
}
