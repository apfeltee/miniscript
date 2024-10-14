mcframelist_t* mc_framelist_make(mcstate_t* state, size_t initialsize)
{
    mcframelist_t* list;
    mcvmframe_t nullframe = {};
    list = (mcframelist_t*)mc_allocator_malloc(state, sizeof(mcframelist_t));
    list->pstate = state;
    list->listcount = 0;
    list->listcapacity = 0;
    list->listitems = NULL;
    if(initialsize > 0)
    {
        mc_framelist_ensurecapacity(list, initialsize, nullframe, true);
    }
    return list;
}

void mc_framelist_destroy(mcframelist_t* list)
{
    #if 0
        fprintf(stderr, "framelist use at end: count=%ld capacity=%ld\n", list->listcount, list->listcapacity);
    #endif
    if(list != NULL)
    {
        mc_memory_free(list->listitems);
        mc_memory_free(list);
        list = NULL;
    }
}

MC_INLINE size_t mc_framelist_count(mcframelist_t* list)
{
    return list->listcount;
}

MC_INLINE mcvmframe_t* mc_framelist_get(mcframelist_t* list, size_t idx)
{
    return &list->listitems[idx];
}

MC_INLINE mcvmframe_t* mc_framelist_set(mcframelist_t* list, size_t idx, mcvmframe_t val)
{
    size_t need;
    mcvmframe_t nullframe = {};
    need = idx + 8;
    if(((idx == 0) || (list->listcapacity == 0)) || (idx >= list->listcapacity))
    {
        mc_framelist_ensurecapacity(list, need, nullframe, false);
    }
    list->listitems[idx] = val;
    return &list->listitems[idx];
}

MC_INLINE void mc_framelist_push(mcframelist_t* list, mcvmframe_t value)
{
    size_t oldcap;
    if(list->listcapacity < list->listcount + 1)
    {
        oldcap = list->listcapacity;
        list->listcapacity = MC_UTIL_INCCAPACITY(oldcap);
        if(list->listitems == NULL)
        {
            list->listitems = (mcvmframe_t*)mc_allocator_malloc(list->pstate, sizeof(mcvmframe_t) * list->listcapacity);
        }
        else
        {
            list->listitems = (mcvmframe_t*)mc_allocator_realloc(list->pstate, list->listitems, sizeof(mcvmframe_t) * list->listcapacity);
        }
    }
    list->listitems[list->listcount] = value;
    list->listcount++;
}

void mc_framelist_ensurecapacity(mcframelist_t* list, size_t needsize, mcvmframe_t fillval, bool first)
{
    size_t i;
    size_t ncap;
    size_t oldcap;
    (void)first;
    if(list->listcapacity < needsize)
    {
        oldcap = list->listcapacity;
        ncap = MC_UTIL_INCCAPACITY(list->listcapacity + needsize);
        list->listcapacity = ncap;
        if(list->listitems == NULL)
        {
            list->listitems = (mcvmframe_t*)mc_allocator_malloc(list->pstate, sizeof(mcvmframe_t) * ncap);
        }
        else
        {
            list->listitems = (mcvmframe_t*)mc_allocator_realloc(list->pstate, list->listitems, sizeof(mcvmframe_t) * ncap);
        }
        for(i = oldcap; i < ncap; i++)
        {
            list->listitems[i] = fillval;
        }
    }
}