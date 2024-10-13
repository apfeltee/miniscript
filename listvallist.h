
mcvallist_t* mc_vallist_make(mcstate_t* state, const char* name, size_t initialsize)
{
    mcvallist_t* list;
    list = (mcvallist_t*)mc_allocator_malloc(state, sizeof(mcvallist_t));
    list->pstate = state;
    list->listcount = 0;
    list->listcapacity = 0;
    list->listitems = NULL;
    list->listname = name;
    if(initialsize > 0)
    {
        mc_vallist_ensurecapacity(list, initialsize, mc_value_makenull(), true);
    }
    return list;
}

void mc_vallist_destroy(mcvallist_t* list)
{
    #if 0
    if(list->listname != NULL)
    {
        fprintf(stderr, "vallist of '%s' use at end: count=%ld capacity=%ld\n", list->listname, list->listcount, list->listcapacity);
    }
    #endif
    if(list != NULL)
    {
        mc_memory_free(list->listitems);
        mc_memory_free(list);
        list = NULL;
    }
}

MC_INLINE size_t mc_vallist_count(mcvallist_t* list)
{
    return list->listcount;
}

MC_INLINE mcvalue_t* mc_vallist_data(mcvallist_t* list)
{
    return list->listitems;
}

MC_INLINE mcvalue_t mc_vallist_get(mcvallist_t* list, size_t idx)
{
    return list->listitems[idx];
}

MC_INLINE mcvalue_t* mc_vallist_getp(mcvallist_t* list, size_t idx)
{
    return &list->listitems[idx];
}

MC_INLINE bool mc_vallist_set(mcvallist_t* list, size_t idx, mcvalue_t val)
{
    size_t need;
    need = idx + 8;
    if(list->listcount == 0)
    {
        return mc_vallist_push(list, val);
    }
    if(((idx == 0) || (list->listcapacity == 0)) || (idx >= list->listcapacity))
    {
        mc_vallist_ensurecapacity(list, need, mc_value_makenull(), false);
    }
    list->listitems[idx] = val;
    if(idx > list->listcount)
    {
        list->listcount = idx;
    }
    return true;
}

MC_INLINE bool mc_vallist_push(mcvallist_t* list, mcvalue_t value)
{
    size_t oldcap;
    if(list->listcapacity < list->listcount + 1)
    {
        oldcap = list->listcapacity;
        list->listcapacity = MC_UTIL_INCCAPACITY(oldcap);
        if(list->listitems == NULL)
        {
            list->listitems = (mcvalue_t*)mc_allocator_malloc(list->pstate, sizeof(mcvalue_t) * list->listcapacity);
        }
        else
        {
            list->listitems = (mcvalue_t*)mc_allocator_realloc(list->pstate, list->listitems, sizeof(mcvalue_t) * list->listcapacity);
        }
    }

    list->listitems[list->listcount] = value;
    list->listcount++;
    return true;
}

MC_INLINE bool mc_vallist_pop(mcvallist_t* list, mcvalue_t* dest)
{
    if(list->listcount > 0)
    {
        *dest = list->listitems[list->listcount - 1];
        list->listcount--;
        return true;
    }
    return false;
}

bool mc_vallist_removeatintern(mcvallist_t* arr, unsigned int ix)
{
    size_t tomovebytes;
    void* src;
    void* dest;
    if(ix == (arr->listcount - 1))
    {
        arr->listcount--;
        return true;
    }
    tomovebytes = (arr->listcount - 1 - ix) * sizeof(mcvalue_t);
    dest = arr->listitems + (ix * sizeof(mcvalue_t));
    src = arr->listitems + ((ix + 1) * sizeof(mcvalue_t));
    memmove(dest, src, tomovebytes);
    arr->listcount--;
    return true;
}

MC_INLINE bool mc_vallist_removeat(mcvallist_t* arr, unsigned int ix)
{
    if(ix >= arr->listcount)
    {
        return false;
    }
    if(ix == 0)
    {
        arr->listitems += sizeof(mcvalue_t);
        arr->listcapacity--;
        arr->listcount--;
        return true;
    }
    return mc_vallist_removeatintern(arr, ix);
}

void mc_vallist_ensurecapacity(mcvallist_t* list, size_t needsize, mcvalue_t fillval, bool first)
{
    size_t i;
    size_t ncap;
    size_t oldcap;
    (void)first;
    if(list->listcapacity < needsize)
    {
        oldcap = list->listcapacity;
        if(oldcap == 0)
        {
            ncap = needsize;
        }
        else
        {
            ncap = MC_UTIL_INCCAPACITY(list->listcapacity + needsize);
        }
        list->listcapacity = ncap;
        if(list->listitems == NULL)
        {
            list->listitems = (mcvalue_t*)mc_allocator_malloc(list->pstate, sizeof(mcvalue_t) * ncap);
        }
        else
        {
            list->listitems = (mcvalue_t*)mc_allocator_realloc(list->pstate, list->listitems, sizeof(mcvalue_t) * ncap);
        }
        for(i = oldcap; i < ncap; i++)
        {
            list->listitems[i] = fillval;
        }
    }
}

MC_INLINE mcvallist_t* mc_vallist_copy(mcvallist_t* list)
{
    size_t i;
    mcvallist_t* nlist;
    nlist = mc_vallist_make(list->pstate, list->listname, 0);
    for(i=0; i<list->listcount; i++)
    {
        mc_vallist_push(nlist, list->listitems[i]);
    }
    return nlist;
}

MC_INLINE void mc_vallist_setempty(mcvallist_t* list)
{
    if((list->listcapacity > 0) && (list->listitems != NULL))
    {
        memset(list->listitems, 0, sizeof(mcvalue_t) * list->listcapacity);
    }
    list->listcount = 0;
    list->listcapacity = 0;
}
