

mcmodule_t* mc_module_make(mcstate_t* state, const char* name)
{
    mcmodule_t* module;
    module = (mcmodule_t*)mc_allocator_malloc(state, sizeof(mcmodule_t));
    if(!module)
    {
        return NULL;
    }
    memset(module, 0, sizeof(mcmodule_t));
    module->pstate = state;
    module->name = mc_util_strdup(state, name);
    if(!module->name)
    {
        mc_module_destroy(module);
        return NULL;
    }
    module->modsymbols = mc_ptrlist_make(state, 0, sizeof(void*));
    if(!module->modsymbols)
    {
        mc_module_destroy(module);
        return NULL;
    }
    return module;
}

void mc_module_destroy(mcmodule_t* module)
{
    if(!module)
    {
        return;
    }
    mc_memory_free(module->name);
    mc_ptrlist_destroy(module->modsymbols, (mcitemdestroyfn_t)mc_symbol_destroy);
    mc_memory_free(module);
}

bool mc_module_addsymbol(mcmodule_t* module, mcastsymbol_t* symbol)
{
    bool ok;
    mcastsymbol_t* modulesymbol;
    mcprinter_t* namebuf;
    namebuf = mc_printer_make(module->pstate, NULL);
    if(!namebuf)
    {
        return false;
    }
    ok = mc_printer_printf(namebuf, "%s::%s", module->name, symbol->name);
    if(!ok)
    {
        mc_printer_destroy(namebuf);
        return false;
    }
    modulesymbol = mc_symbol_make(module->pstate, mc_printer_getstring(namebuf), MC_SYM_MODULEGLOBAL, symbol->index, false);
    mc_printer_destroy(namebuf);
    if(!modulesymbol)
    {
        return false;
    }
    ok = mc_ptrlist_push(module->modsymbols, modulesymbol);
    if(!ok)
    {
        mc_symbol_destroy(modulesymbol);
        return false;
    }
    return true;
}


const char* mc_util_getmodulename(const char* path)
{
    const char* lastslashpos;
    lastslashpos = strrchr(path, '/');
    if(lastslashpos)
    {
        return lastslashpos + 1;
    }
    return path;
}

const char* mc_module_findfile(mcstate_t* state, const char* filename)
{
    (void)state;
    return filename;
}

mcmodule_t* mc_module_copy(mcmodule_t* src)
{
    mcmodule_t* copy;
    copy = (mcmodule_t*)mc_allocator_malloc(src->pstate, sizeof(mcmodule_t));
    if(!copy)
    {
        return NULL;
    }
    memset(copy, 0, sizeof(mcmodule_t));
    copy->pstate = src->pstate;
    copy->name = mc_util_strdup(copy->pstate, src->name);
    if(!copy->name)
    {
        mc_module_destroy(copy);
        return NULL;
    }
    copy->modsymbols = mc_ptrlist_copy(src->modsymbols, (mcitemcopyfn_t)mc_symbol_copy, (mcitemdestroyfn_t)mc_symbol_destroy);
    if(!copy->modsymbols)
    {
        mc_module_destroy(copy);
        return NULL;
    }
    return copy;
}
