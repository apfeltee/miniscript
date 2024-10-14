

bool mc_ptrdict_init(mcstate_t* state, mcptrdict_t* dict, unsigned int initialcapacity, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
{
    unsigned int i;
    dict->pstate = state;
    dict->gdcells = NULL;
    dict->gdkeys = NULL;
    dict->gdvalues = NULL;
    dict->gdcellindices = NULL;
    dict->gdhashes = NULL;
    dict->gdcount = 0;
    dict->gdcellcapacity = 0;
    dict->gdcellcapacity += initialcapacity;
    dict->gditemcapacity = (unsigned int)(initialcapacity * 0.7f);
    dict->funccopyfn = copyfn;
    dict->funcdestroyfn = dfn;
    dict->gdcells = (unsigned int*)mc_allocator_malloc(dict->pstate, dict->gdcellcapacity * sizeof(*dict->gdcells));
    dict->gdkeys = (char**)mc_allocator_malloc(dict->pstate, dict->gditemcapacity * sizeof(*dict->gdkeys));
    dict->gdvalues = (void**)mc_allocator_malloc(dict->pstate, dict->gditemcapacity * sizeof(*dict->gdvalues));
    dict->gdcellindices = (unsigned int*)mc_allocator_malloc(dict->pstate, dict->gditemcapacity * sizeof(*dict->gdcellindices));
    dict->gdhashes = (long unsigned int*)mc_allocator_malloc(dict->pstate, dict->gditemcapacity * sizeof(*dict->gdhashes));
    if(dict->gdcells == NULL || dict->gdkeys == NULL || dict->gdvalues == NULL || dict->gdcellindices == NULL || dict->gdhashes == NULL)
    {
        goto dictallocfailed;
    }
    for(i = 0; i < dict->gdcellcapacity; i++)
    {
        dict->gdcells[i] = MC_CONF_GENERICDICTINVALIDIX;
    }
    return true;
dictallocfailed:
    mc_memory_free(dict->gdcells);
    mc_memory_free(dict->gdkeys);
    mc_memory_free(dict->gdvalues);
    mc_memory_free(dict->gdcellindices);
    mc_memory_free(dict->gdhashes);
    return false;
}

mcptrdict_t* mc_ptrdict_make(mcstate_t* state, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
{
    bool ok;
    mcptrdict_t* dict;
    dict = (mcptrdict_t*)mc_allocator_malloc(state, sizeof(mcptrdict_t));
    if(dict == NULL)
    {
        return NULL;
    }
    ok = mc_ptrdict_init(state, dict, MC_CONF_GENERICDICTINITSIZE, copyfn, dfn);
    if(!ok)
    {
        mc_memory_free(dict);
        return NULL;
    }
    dict->pstate = state;
    return dict;
}

void mc_ptrdict_deinit(mcptrdict_t* dict, bool freekeys)
{
    unsigned int i;
    if(freekeys)
    {
        for(i = 0; i < dict->gdcount; i++)
        {
            mc_memory_free(dict->gdkeys[i]);
        }
    }
    dict->gdcount = 0;
    dict->gditemcapacity = 0;
    dict->gdcellcapacity = 0;
    mc_memory_free(dict->gdcells);
    mc_memory_free(dict->gdkeys);
    mc_memory_free(dict->gdvalues);
    mc_memory_free(dict->gdcellindices);
    mc_memory_free(dict->gdhashes);
    dict->gdcells = NULL;
    dict->gdkeys = NULL;
    dict->gdvalues = NULL;
    dict->gdcellindices = NULL;
    dict->gdhashes = NULL;
}

void mc_ptrdict_destroy(mcptrdict_t* dict)
{
    if(!dict)
    {
        return;
    }
    mc_ptrdict_deinit(dict, true);
    mc_memory_free(dict);
}

void mc_ptrdict_destroyitemsanddict(mcptrdict_t* dict)
{
    unsigned int i;
    if(!dict)
    {
        return;
    }
    if(dict->funcdestroyfn)
    {
        for(i = 0; i < dict->gdcount; i++)
        {
            dict->funcdestroyfn(dict->gdvalues[i]);
        }
    }
    mc_ptrdict_destroy(dict);
}

bool mc_ptrdict_growandrehash(mcptrdict_t* dict)
{
    bool ok;
    unsigned int i;
    char* key;
    void* value;
    size_t ncap;
    mcptrdict_t newdict;
    ncap = MC_UTIL_INCCAPACITY(dict->gdcellcapacity);
    ok = mc_ptrdict_init(dict->pstate, &newdict, ncap, dict->funccopyfn, dict->funcdestroyfn);
    if(!ok)
    {
        return false;
    }
    for(i = 0; i < dict->gdcount; i++)
    {
        key = dict->gdkeys[i];
        value = dict->gdvalues[i];
        ok = mc_ptrdict_setactual(&newdict, key, key, value);
        if(!ok)
        {
            mc_ptrdict_deinit(&newdict, false);
            return false;
        }
    }
    mc_ptrdict_deinit(dict, false);
    *dict = newdict;
    return true;
}

MC_INLINE unsigned int mc_ptrdict_getcellindex(mcptrdict_t* dict, const char* key, unsigned long hash, bool* outfound)
{
    unsigned int i;
    unsigned int ix;
    unsigned int cell;
    unsigned int cellix;
    unsigned long hashtocheck;
    const char* keytocheck;
    *outfound = false;
    cellix = (unsigned int)hash & (dict->gdcellcapacity - 1);
    for(i = 0; i < dict->gdcellcapacity; i++)
    {
        ix = (cellix + i) & (dict->gdcellcapacity - 1);
        cell = dict->gdcells[ix];
        if(cell == MC_CONF_GENERICDICTINVALIDIX)
        {
            return ix;
        }
        hashtocheck = dict->gdhashes[cell];
        if(hash != hashtocheck)
        {
            continue;
        }
        keytocheck = dict->gdkeys[cell];
        if(strcmp(key, keytocheck) == 0)
        {
            *outfound = true;
            return ix;
        }
    }
    return MC_CONF_GENERICDICTINVALIDIX;
}

bool mc_ptrdict_setintern(mcptrdict_t* dict, unsigned int cellix, unsigned long hash, const char* ckey, char* mkey, void* value)
{
    bool ok;
    bool found;
    char* keycopy;
    if(dict->gdcount >= dict->gditemcapacity)
    {
        ok = mc_ptrdict_growandrehash(dict);
        if(!ok)
        {
            return false;
        }
        cellix = mc_ptrdict_getcellindex(dict, ckey, hash, &found);
    }
    if(mkey)
    {
        dict->gdkeys[dict->gdcount] = mkey;
    }
    else
    {
        keycopy = mc_util_strdup(dict->pstate, ckey);
        if(!keycopy)
        {
            return false;
        }
        dict->gdkeys[dict->gdcount] = keycopy;
    }
    dict->gdcells[cellix] = dict->gdcount;
    dict->gdvalues[dict->gdcount] = value;
    dict->gdcellindices[dict->gdcount] = cellix;
    dict->gdhashes[dict->gdcount] = hash;
    dict->gdcount++;
    return true;
}

MC_INLINE bool mc_ptrdict_setactual(mcptrdict_t* dict, const char* ckey, char* mkey, void* value)
{
    bool found;
    unsigned int cellix;
    unsigned int itemix;
    unsigned long hash;
    hash = mc_util_hashdata(ckey, mc_util_strlen(ckey));
    found = false;
    cellix = mc_ptrdict_getcellindex(dict, ckey, hash, &found);
    if(found)
    {
        itemix = dict->gdcells[cellix];
        dict->gdvalues[itemix] = value;
        return true;
    }
    return mc_ptrdict_setintern(dict, cellix, hash, ckey, mkey, value);
}

MC_INLINE bool mc_ptrdict_set(mcptrdict_t* dict, const char* key, void* value)
{
    return mc_ptrdict_setactual(dict, key, NULL, value);
}

MC_INLINE void* mc_ptrdict_get(mcptrdict_t* dict, const char* key)
{
    bool found;
    unsigned int itemix;
    unsigned long hash;
    unsigned long cellix;
    hash = mc_util_hashdata(key, mc_util_strlen(key));
    found = false;
    cellix = mc_ptrdict_getcellindex(dict, key, hash, &found);
    if(found == false)
    {
        return NULL;
    }
    itemix = dict->gdcells[cellix];
    return dict->gdvalues[itemix];
}

MC_INLINE void* mc_ptrdict_getvalueat(mcptrdict_t* dict, unsigned int ix)
{
    if(ix >= dict->gdcount)
    {
        return NULL;
    }
    return dict->gdvalues[ix];
}

MC_INLINE const char* mc_ptrdict_getkeyat(mcptrdict_t* dict, unsigned int ix)
{
    if(ix >= dict->gdcount)
    {
        return NULL;
    }
    return dict->gdkeys[ix];
}

MC_INLINE size_t mc_ptrdict_count(mcptrdict_t* dict)
{
    if(!dict)
    {
        return 0;
    }
    return dict->gdcount;
}

MC_INLINE bool mc_ptrdict_remove(mcptrdict_t* dict, const char* key)
{
    bool found;
    unsigned int x;
    unsigned int k;
    unsigned int i;
    unsigned int j;
    unsigned int cell;
    unsigned int itemix;
    unsigned int lastitemix;
    unsigned long hash;
    hash = mc_util_hashdata(key, mc_util_strlen(key));
    found = false;
    cell = mc_ptrdict_getcellindex(dict, key, hash, &found);
    if(!found)
    {
        return false;
    }
    itemix = dict->gdcells[cell];
    mc_memory_free(dict->gdkeys[itemix]);
    lastitemix = dict->gdcount - 1;
    if(itemix < lastitemix)
    {
        dict->gdkeys[itemix] = dict->gdkeys[lastitemix];
        dict->gdvalues[itemix] = dict->gdvalues[lastitemix];
        dict->gdcellindices[itemix] = dict->gdcellindices[lastitemix];
        dict->gdhashes[itemix] = dict->gdhashes[lastitemix];
        dict->gdcells[dict->gdcellindices[itemix]] = itemix;
    }
    dict->gdcount--;
    i = cell;
    j = i;
    for(x = 0; x < (dict->gdcellcapacity - 1); x++)
    {
        j = (j + 1) & (dict->gdcellcapacity - 1);
        if(dict->gdcells[j] == MC_CONF_GENERICDICTINVALIDIX)
        {
            break;
        }
        k = (unsigned int)(dict->gdhashes[dict->gdcells[j]]) & (dict->gdcellcapacity - 1);
        if((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j)))
        {
            dict->gdcellindices[dict->gdcells[j]] = i;
            dict->gdcells[i] = dict->gdcells[j];
            i = j;
        }
    }
    dict->gdcells[i] = MC_CONF_GENERICDICTINVALIDIX;
    return true;
}

mcptrdict_t* mc_ptrdict_copy(mcptrdict_t* dict)
{
    bool ok;
    size_t i;
    void* item;
    void* itemcopy;
    const char* key;
    mcptrdict_t* dictcopy;
    if(!dict->funccopyfn || !dict->funcdestroyfn)
    {
        return NULL;
    }
    dictcopy = mc_ptrdict_make(dict->pstate, dict->funccopyfn, dict->funcdestroyfn);
    if(!dictcopy)
    {
        return NULL;
    }
    dictcopy->pstate = dict->pstate;
    for(i = 0; i < mc_ptrdict_count(dict); i++)
    {
        key = mc_ptrdict_getkeyat(dict, i);
        item = mc_ptrdict_getvalueat(dict, i);
        itemcopy = dictcopy->funccopyfn(item);
        if(item && !itemcopy)
        {
            mc_ptrdict_destroyitemsanddict(dictcopy);
            return NULL;
        }
        ok = mc_ptrdict_set(dictcopy, key, itemcopy);
        if(!ok)
        {
            dictcopy->funcdestroyfn(itemcopy);
            mc_ptrdict_destroyitemsanddict(dictcopy);
            return NULL;
        }
    }
    return dictcopy;
}
