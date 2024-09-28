

mcvallist_t* mc_vallist_make(fhstate_t* state, const char* name, size_t initialsize)
{
    mcvallist_t* stck;
    stck = (mcvallist_t)mc_memory_malloc(sizeof(mcvallist_t));
    stck->state = state;
    stck->storedstack = NULL;
    stck->count = 0;
    stck->capacity = 0;
    stck->itemtypesize = sizeof(mcvalue_t);
    return stck;
}

void mc_vallist_destroy(mcvallist_t* stck)
{
    if(stck->storedstack)
    {
        mc_memory_free(stck->storedstack);
        stck->storedstack = NULL;
    }
    stck->count = 0;
    stck->capacity = 0;
}

int mc_vallist_size(mcvallist_t* stck)
{
    return stck->count;
}

mcvalue_t* mc_vallist_getp(mcvallist_t* stck, int index)
{
    if(index < 0 || index >= stck->count)
    {
        return NULL;
    }
    return (char*)stck->storedstack + index * stck->itemtypesize;
}

mcvalue_t mc_vallist_top(mcvallist_t* stck)
{
    return mc_vallist_itemat(stck, stck->count - 1);
}

int mc_vallist_shrinktofit(mcvallist_t* stck)
{
    mcvalue_t* ndata;
    stck->capacity = stck->count;
    if(stck->count == 0)
    {
        if(stck->storedstack)
        {
            mc_memory_free(stck->storedstack);
        }
        stck->storedstack = NULL;
    }
    else
    {
        ndata = mc_memory_realloc(stck->storedstack, stck->count * stck->itemtypesize);
        if(ndata == NULL)
        {
            return -1;
        }
        stck->storedstack = ndata;
    }
    return 0;
}

int mc_vallist_ensurecapacity(mcvallist_t* stck, int nitems)
{
    int ncap;
    mcvalue_t* ndata;
    if(stck->count + nitems > stck->capacity)
    {
        ncap = (stck->count + nitems + 15) / 16 * 16;
        ndata = mc_memory_realloc(stck->storedstack, ncap * stck->itemtypesize);
        if(ndata == NULL)
        {
            return -1;
        }
        stck->storedstack = ndata;
        stck->capacity = ncap;
    }
    return 0;
}

bool mc_vallist_push(mcvallist_t* stck, mcvalue_t item)
{
    if(mc_vallist_ensurecapacity(stck, 1) < 0)
    {
        return NULL;
    }
    if(item)
    {
        memcpy((char*)stck->storedstack + stck->count * stck->itemtypesize, &item, stck->itemtypesize);
    }
    else
    {
        memset((char*)stck->storedstack + stck->count * stck->itemtypesize, 0, stck->itemtypesize);
    }
    return true;
}

bool mc_vallist_pop(mcvallist_t* stck, mcvalue_t* item)
{
    if(stck->count == 0)
    {
        return false;
    }
    stck->count--;
    if(item)
    {
        memcpy(item, (char*)stck->storedstack + stck->count * stck->itemtypesize, stck->itemtypesize);
    }
    return true;
}

mcvalue_t* mc_vallist_data(mcvallist_t* n)
{
    return n->storedstack;
}

