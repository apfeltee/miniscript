
#define MC_BASICLIST_INITIALSIZE (32)

mcbasicarray_t* mc_basicarray_make(mcstate_t* state, size_t tsz)
{
    return mc_basicarray_makecapacity(state, MC_BASICLIST_INITIALSIZE, tsz);
}

mcbasicarray_t* mc_basicarray_makecapacity(mcstate_t* state, unsigned int capacity, size_t tsz)
{
    bool ok;
    mcbasicarray_t* list;
    list = (mcbasicarray_t*)mc_allocator_malloc(state, sizeof(mcbasicarray_t));
    if(!list)
    {
        return NULL;
    }

    ok = mc_basicarray_initcapacity(list, state, capacity, tsz);
    if(!ok)
    {
        mc_memory_free(list);
        return NULL;
    }
    list->pstate = state;
    return list;
}

bool mc_basicarray_initcapacity(mcbasicarray_t* list, mcstate_t* state, unsigned int capacity, size_t tsz)
{
    list->pstate = state;
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

void mc_basicarray_deinit(mcbasicarray_t* list)
{
    mc_memory_free(list->allocdata);
}

void mc_basicarray_destroy(mcbasicarray_t* list)
{
    if(!list)
    {
        return;
    }
    mc_basicarray_deinit(list);
    mc_memory_free(list);
}

mcbasicarray_t* mc_basicarray_copy(mcbasicarray_t* list)
{
    mcbasicarray_t* copy;
    copy = (mcbasicarray_t*)mc_allocator_malloc(list->pstate, sizeof(mcbasicarray_t));
    if(!copy)
    {
        return NULL;
    }
    copy->pstate = list->pstate;
    copy->listcapacity = list->listcapacity;
    copy->listcount = list->listcount;
    copy->typesize = list->typesize;
    copy->caplocked = list->caplocked;
    if(list->allocdata)
    {
        copy->allocdata = (unsigned char*)mc_allocator_malloc(list->pstate, list->listcapacity * list->typesize);
        if(!copy->allocdata)
        {
            mc_memory_free(copy);
            return NULL;
        }
        copy->listitems = copy->allocdata;
        memcpy(copy->allocdata, list->listitems, list->listcapacity * list->typesize);
    }
    else
    {
        copy->allocdata = NULL;
        copy->listitems = NULL;
    }
    return copy;
}

MC_INLINE bool mc_basicarray_push(mcbasicarray_t* list, void* value)
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
        newdata = (unsigned char*)mc_allocator_malloc(list->pstate, ncap * list->typesize);
        if(!newdata)
        {
            return false;
        }
        memcpy(newdata, list->listitems, list->listcount * list->typesize);
        mc_memory_free(list->allocdata);
        list->allocdata = newdata;
        list->listitems = list->allocdata;
        list->listcapacity = ncap;
    }
    if(value)
    {
        memcpy(list->listitems + (list->listcount * list->typesize), value, list->typesize);
    }
    list->listcount++;
    return true;
}

MC_INLINE bool mc_basicarray_pop(mcbasicarray_t* list, void* outvalue)
{
    void* res;
    if(list->listcount <= 0)
    {
        return false;
    }
    if(outvalue)
    {
        res = mc_basicarray_get(list, list->listcount - 1);
        memcpy(outvalue, res, list->typesize);
    }
    mc_basicarray_removeat(list, list->listcount - 1);

    return true;
}

MC_INLINE void* mc_basicarray_top(mcbasicarray_t* list)
{
    if(list->listcount <= 0)
    {
        return NULL;
    }
    return mc_basicarray_get(list, list->listcount - 1);
}

MC_INLINE bool mc_basicarray_set(mcbasicarray_t* list, unsigned int ix, void* value)
{
    size_t offset;
    if(ix >= list->listcount)
    {
        MC_ASSERT(false);
        return false;
    }
    offset = ix * list->typesize;
    memmove(list->listitems + offset, value, list->typesize);
    return true;
}

MC_INLINE void* mc_basicarray_get(mcbasicarray_t* list, unsigned int ix)
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

MC_INLINE size_t mc_basicarray_count(mcbasicarray_t* list)
{
    if(!list)
    {
        return 0;
    }
    return list->listcount;
}

bool mc_basicarray_removeat(mcbasicarray_t* list, unsigned int ix)
{
    size_t tomovebytes;
    void* dest;
    void* src;
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
    list->listcount--;
    return true;
}

void mc_basicarray_clear(mcbasicarray_t* list)
{
    list->listcount = 0;
}

void* mc_basicarray_data(mcbasicarray_t* list)
{
    return list->listitems;
}

void mc_basicarray_orphandata(mcbasicarray_t* list)
{
    mc_basicarray_initcapacity(list, list->pstate, 0, list->typesize);
}

MC_INLINE void* mc_basicarray_getconst(mcbasicarray_t* list, unsigned int ix)
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

int mc_basicarray_getindex(mcbasicarray_t* list, void* ptr)
{
    size_t i;
    for(i = 0; i < mc_basicarray_count(list); i++)
    {
        if(mc_basicarray_getconst(list, i) == ptr)
        {
            return i;
        }
    }
    return -1;
}

bool mc_basicarray_removeitem(mcbasicarray_t* list, void* ptr)
{
    int ix;
    ix = mc_basicarray_getindex(list, ptr);
    if(ix < 0)
    {
        return false;
    }
    return mc_basicarray_removeat(list, ix);
}

bool mc_basicarray_contains(mcbasicarray_t* list, void* ptr)
{
    return mc_basicarray_getindex(list, ptr) >= 0;
}

