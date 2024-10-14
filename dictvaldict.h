
bool mc_valdict_init(mcstate_t* state, mcvaldict_t* dict, unsigned int initialcapacity, size_t ktsz, size_t vtsz, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
{
    unsigned int i;
    dict->pstate = state;
    dict->keytypesize = ktsz;
    dict->valtypesize = vtsz;
    dict->vdcells = NULL;
    dict->vdkeys = NULL;
    dict->vdvalues = NULL;
    dict->vdcellindices = NULL;
    dict->vdhashes = NULL;
    dict->vdcount = 0;
    dict->funccopyfn = copyfn;
    dict->funcdestroyfn = dfn;
    dict->vdcellcapacity = initialcapacity;
    dict->vditemcapacity = (unsigned int)(initialcapacity * 0.7f);
    dict->funckeyequalsfn = NULL;
    dict->funchashfn = NULL;
    dict->vdcells = (unsigned int*)mc_allocator_malloc(dict->pstate, dict->vdcellcapacity * sizeof(unsigned int));
    dict->vdkeys = (char**)mc_allocator_malloc(dict->pstate, dict->vditemcapacity * dict->keytypesize);
    dict->vdvalues = (mcvalue_t**)mc_allocator_malloc(dict->pstate, dict->vditemcapacity * dict->valtypesize);
    dict->vdcellindices = (unsigned int*)mc_allocator_malloc(dict->pstate, dict->vditemcapacity * sizeof(unsigned int));
    dict->vdhashes = (long unsigned int*)mc_allocator_malloc(dict->pstate, dict->vditemcapacity * sizeof(unsigned long));
    if(dict->vdcells == NULL || dict->vdkeys == NULL || dict->vdvalues == NULL || dict->vdcellindices == NULL || dict->vdhashes == NULL)
    {
        goto dictallocfailed;
    }
    for(i = 0; i < dict->vdcellcapacity; i++)
    {
        dict->vdcells[i] = MC_CONF_VALDICTINVALIDIX;
    }
    return true;
dictallocfailed:
    mc_memory_free(dict->vdcells);
    mc_memory_free(dict->vdkeys);
    mc_memory_free(dict->vdvalues);
    mc_memory_free(dict->vdcellindices);
    mc_memory_free(dict->vdhashes);
    return false;
}

void mc_valdict_deinit(mcvaldict_t* dict)
{
    dict->keytypesize = 0;
    dict->valtypesize = 0;
    dict->vdcount = 0;
    dict->vditemcapacity = 0;
    dict->vdcellcapacity = 0;
    mc_memory_free(dict->vdcells);
    mc_memory_free(dict->vdkeys);
    mc_memory_free(dict->vdvalues);
    mc_memory_free(dict->vdcellindices);
    mc_memory_free(dict->vdhashes);
    dict->vdcells = NULL;
    dict->vdkeys = NULL;
    dict->vdvalues = NULL;
    dict->vdcellindices = NULL;
    dict->vdhashes = NULL;
}

mcvaldict_t* mc_valdict_makedefault(mcstate_t* state, size_t ktsz, size_t vtsz, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
{
    return mc_valdict_makecapacity(state, MC_CONF_GENERICDICTINITSIZE, ktsz, vtsz, copyfn, dfn);
}

mcvaldict_t* mc_valdict_makecapacity(mcstate_t* state, unsigned int mincapacity, size_t ktsz, size_t vtsz, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
{
    bool ok;
    unsigned int capacity;
    mcvaldict_t* dict;
    capacity = mc_util_upperpowoftwo(mincapacity * 2);
    dict = (mcvaldict_t*)mc_allocator_malloc(state, sizeof(mcvaldict_t));
    if(!dict)
    {
        return NULL;
    }
    ok = mc_valdict_init(state, dict, capacity, ktsz, vtsz, copyfn, dfn);
    if(!ok)
    {
        mc_memory_free(dict);
        return NULL;
    }
    dict->pstate = state;
    return dict;
}

void mc_valdict_destroy(mcvaldict_t* dict)
{
    if(!dict)
    {
        return;
    }
    mc_valdict_deinit(dict);
    mc_memory_free(dict);
}

MC_INLINE void mc_valdict_sethashfunction(mcvaldict_t* dict, mcitemhashfn_t hashfn)
{
    dict->funchashfn = hashfn;
}

MC_INLINE void mc_valdict_setequalsfunction(mcvaldict_t* dict, mcitemcomparefn_t equalsfn)
{
    dict->funckeyequalsfn = equalsfn;
}

bool mc_valdict_setkvintern(mcvaldict_t* dict, unsigned int cellix, unsigned long hash, void* key, void* value)
{
    bool ok;
    bool found;
    unsigned int lastix;
    if(dict->vdcount >= dict->vditemcapacity)
    {
        ok = mc_valdict_growandrehash(dict);
        if(!ok)
        {
            return false;
        }
        cellix = mc_valdict_getcellindex(dict, key, hash, &found);
    }
    lastix = dict->vdcount;
    dict->vdcount++;
    dict->vdcells[cellix] = lastix;
    mc_valdict_setkeyat(dict, lastix, key);
    mc_valdict_setvalueat(dict, lastix, value);
    dict->vdcellindices[lastix] = cellix;
    dict->vdhashes[lastix] = hash;
    return true;
}

MC_INLINE bool mc_valdict_setkv(mcvaldict_t* dict, void* key, void* value)
{
    bool found;
    unsigned long hash;
    unsigned int cellix;
    unsigned int itemix;
    hash = mc_valdict_hashkey(dict, key);
    found = false;
    cellix = mc_valdict_getcellindex(dict, key, hash, &found);
    if(found)
    {
        itemix = dict->vdcells[cellix];
        mc_valdict_setvalueat(dict, itemix, value);
        return true;
    }
    return mc_valdict_setkvintern(dict, cellix, hash, key, value);
}

MC_INLINE void* mc_valdict_get(mcvaldict_t* dict, void* key)
{
    bool found;
    unsigned int itemix;
    unsigned long hash;
    unsigned long cellix;
    if(dict->vdcount == 0)
    {
        return NULL;
    }
    hash = mc_valdict_hashkey(dict, key);
    found = false;
    cellix = mc_valdict_getcellindex(dict, key, hash, &found);
    if(!found)
    {
        return NULL;
    }
    itemix = dict->vdcells[cellix];
    return mc_valdict_getvalueat(dict, itemix);
}

MC_INLINE void* mc_valdict_getkeyat(mcvaldict_t* dict, unsigned int ix)
{
    if(ix >= dict->vdcount)
    {
        return NULL;
    }
    return (char*)dict->vdkeys + (dict->keytypesize * ix);
}

MC_INLINE void* mc_valdict_getvalueat(mcvaldict_t* dict, unsigned int ix)
{
    if(ix >= dict->vdcount)
    {
        return NULL;
    }
    return (char*)dict->vdvalues + (dict->valtypesize * ix);
}

MC_INLINE unsigned int mc_valdict_getcapacity(mcvaldict_t* dict)
{
    return dict->vditemcapacity;
}

MC_INLINE bool mc_valdict_setvalueat(mcvaldict_t* dict, unsigned int ix, void* value)
{
    size_t offset;
    if(ix >= dict->vdcount)
    {
        return false;
    }
    offset = ix * dict->valtypesize;
    memcpy((char*)dict->vdvalues + offset, value, dict->valtypesize);
    return true;
}

MC_INLINE int mc_valdict_count(mcvaldict_t* dict)
{
    if(!dict)
    {
        return 0;
    }
    return dict->vdcount;
}

MC_INLINE bool mc_valdict_removebykey(mcvaldict_t* dict, void* key)
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
    void* lastkey;
    void* lastvalue;
    hash = mc_valdict_hashkey(dict, key);
    found = false;
    cell = mc_valdict_getcellindex(dict, key, hash, &found);
    if(!found)
    {
        return false;
    }
    itemix = dict->vdcells[cell];
    lastitemix = dict->vdcount - 1;
    if(itemix < lastitemix)
    {
        lastkey = mc_valdict_getkeyat(dict, lastitemix);
        mc_valdict_setkeyat(dict, itemix, lastkey);
        lastvalue = mc_valdict_getkeyat(dict, lastitemix);
        mc_valdict_setvalueat(dict, itemix, lastvalue);
        dict->vdcellindices[itemix] = dict->vdcellindices[lastitemix];
        dict->vdhashes[itemix] = dict->vdhashes[lastitemix];
        dict->vdcells[dict->vdcellindices[itemix]] = itemix;
    }
    dict->vdcount--;
    i = cell;
    j = i;
    for(x = 0; x < (dict->vdcellcapacity - 1); x++)
    {
        j = (j + 1) & (dict->vdcellcapacity - 1);
        if(dict->vdcells[j] == MC_CONF_VALDICTINVALIDIX)
        {
            break;
        }
        k = (unsigned int)(dict->vdhashes[dict->vdcells[j]]) & (dict->vdcellcapacity - 1);
        if((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j)))
        {
            dict->vdcellindices[dict->vdcells[j]] = i;
            dict->vdcells[i] = dict->vdcells[j];
            i = j;
        }
    }
    dict->vdcells[i] = MC_CONF_VALDICTINVALIDIX;
    return true;
}

MC_INLINE void mc_valdict_clear(mcvaldict_t* dict)
{
    unsigned int i;
    dict->vdcount = 0;
    for(i = 0; i < dict->vdcellcapacity; i++)
    {
        dict->vdcells[i] = MC_CONF_VALDICTINVALIDIX;
    }
}

MC_INLINE unsigned int mc_valdict_getcellindex(mcvaldict_t* dict, void* key, unsigned long hash, bool* outfound)
{
    bool areequal;
    unsigned int i;
    unsigned int ix;
    unsigned int cell;
    unsigned int cellix;
    unsigned long hashtocheck;
    void* keytocheck;
    *outfound = false;
    cellix = (unsigned int)hash & (dict->vdcellcapacity - 1);
    for(i = 0; i < dict->vdcellcapacity; i++)
    {
        ix = (cellix + i) & (dict->vdcellcapacity - 1);
        cell = dict->vdcells[ix];
        if(cell == MC_CONF_VALDICTINVALIDIX)
        {
            return ix;
        }
        hashtocheck = dict->vdhashes[cell];
        if(hash != hashtocheck)
        {
            continue;
        }
        keytocheck = mc_valdict_getkeyat(dict, cell);
        areequal = mc_valdict_keysareequal(dict, key, keytocheck);
        if(areequal)
        {
            *outfound = true;
            return ix;
        }
    }
    return MC_CONF_VALDICTINVALIDIX;
}

MC_INLINE bool mc_valdict_growandrehash(mcvaldict_t* dict)
{
    bool ok;
    mcvaldict_t newdict;
    unsigned int i;
    unsigned ncap;
    char* key;
    void* value;
    ncap = MC_UTIL_INCCAPACITY(dict->vdcellcapacity);    
    ok = mc_valdict_init(dict->pstate, &newdict, ncap, dict->keytypesize, dict->valtypesize, dict->funccopyfn, dict->funcdestroyfn);
    if(!ok)
    {
        return false;
    }
    newdict.funckeyequalsfn = dict->funckeyequalsfn;
    newdict.funchashfn = dict->funchashfn;
    for(i = 0; i < dict->vdcount; i++)
    {
        key = (char*)mc_valdict_getkeyat(dict, i);
        value = mc_valdict_getvalueat(dict, i);
        ok = mc_valdict_setkv(&newdict, key, value);
        if(!ok)
        {
            mc_valdict_deinit(&newdict);
            return false;
        }
    }
    mc_valdict_deinit(dict);
    *dict = newdict;
    return true;
}

MC_INLINE bool mc_valdict_setkeyat(mcvaldict_t* dict, unsigned int ix, void* key)
{
    size_t offset;
    if(ix >= dict->vdcount)
    {
        return false;
    }
    offset = ix * dict->keytypesize;
    memcpy((char*)dict->vdkeys + offset, key, dict->keytypesize);
    return true;
}

MC_INLINE bool mc_valdict_keysareequal(mcvaldict_t* dict, void* a, void* b)
{
    if(dict->funckeyequalsfn)
    {
        return dict->funckeyequalsfn(a, b);
    }
    return memcmp(a, b, dict->keytypesize) == 0;
}

MC_INLINE unsigned long mc_valdict_hashkey(mcvaldict_t* dict, void* key)
{
    if(dict->funchashfn)
    {
        return dict->funchashfn(key);
    }
    return mc_util_hashdata(key, dict->keytypesize);
}
