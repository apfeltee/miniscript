
mcastscopecomp_t* mc_astcompscope_make(mcstate_t* state, mcastscopecomp_t* outer)
{
    mcastscopecomp_t* scope;
    scope = (mcastscopecomp_t*)mc_allocator_malloc(state, sizeof(mcastscopecomp_t));
    if(!scope)
    {
        return NULL;
    }
    memset(scope, 0, sizeof(mcastscopecomp_t));
    scope->pstate = state;
    scope->outer = outer;
    scope->compiledscopebytecode = mc_basicarray_make(state, sizeof(uint16_t));
    if(!scope->compiledscopebytecode)
    {
        goto scopemakefailed;
    }
    scope->scopesrcposlist = mc_basicarray_make(state, sizeof(mcastlocation_t));
    if(!scope->scopesrcposlist)
    {
        goto scopemakefailed;
    }
    scope->ipstackbreak = mc_basicarray_make(state, sizeof(int));
    if(!scope->ipstackbreak)
    {
        goto scopemakefailed;
    }
    scope->ipstackcontinue = mc_basicarray_make(state, sizeof(int));
    if(!scope->ipstackcontinue)
    {
        goto scopemakefailed;
    }
    return scope;
scopemakefailed:
    mc_astcompscope_destroy(scope);
    return NULL;
}

void mc_astcompscope_destroy(mcastscopecomp_t* scope)
{
    mc_basicarray_destroy(scope->ipstackcontinue);
    mc_basicarray_destroy(scope->ipstackbreak);
    mc_basicarray_destroy(scope->compiledscopebytecode);
    mc_basicarray_destroy(scope->scopesrcposlist);
    mc_memory_free(scope);
}

mccompiledprogram_t* mc_astcompscope_orphanresult(mcastscopecomp_t* scope)
{
    uint16_t* bcdata;
    mcastlocation_t* astlocdata;
    mccompiledprogram_t* res;
    bcdata = (uint16_t*)mc_basicarray_data(scope->compiledscopebytecode);
    astlocdata = (mcastlocation_t*)mc_basicarray_data(scope->scopesrcposlist);
    res = mc_astcompresult_make(scope->pstate, bcdata, astlocdata, mc_basicarray_count(scope->compiledscopebytecode));
    if(!res)
    {
        return NULL;
    }
    mc_basicarray_orphandata(scope->compiledscopebytecode);
    mc_basicarray_orphandata(scope->scopesrcposlist);
    return res;
}

mccompiledprogram_t* mc_astcompresult_make(mcstate_t* state, uint16_t* bytecode, mcastlocation_t* srcposlist, int count)
{
    mccompiledprogram_t* res;
    res = (mccompiledprogram_t*)mc_allocator_malloc(state, sizeof(mccompiledprogram_t));
    if(!res)
    {
        return NULL;
    }
    memset(res, 0, sizeof(mccompiledprogram_t));
    res->pstate = state;
    res->bytecode = bytecode;
    res->progsrcposlist = srcposlist;
    res->count = count;
    return res;
}

void mc_astcompresult_destroy(mccompiledprogram_t* res)
{
    if(!res)
    {
        return;
    }
    mc_memory_free(res->bytecode);
    mc_memory_free(res->progsrcposlist);
    mc_memory_free(res);
}


bool mc_compiler_init(mcastcompiler_t* comp, mcstate_t* state, mcconfig_t* cfg, mcgcmemory_t* mem, mcerrlist_t* errors, mcptrlist_t* files, mcglobalstore_t* gstor)
{
    bool ok;
    memset(comp, 0, sizeof(mcastcompiler_t));
    comp->pstate = state;
    comp->config = cfg;
    comp->mem = mem;
    comp->errors = errors;
    comp->files = files;
    comp->compglobalstore = gstor;
    comp->filescopelist = mc_ptrlist_make(state, 0, sizeof(void*));
    if(!comp->filescopelist)
    {
        goto compilerinitfailed;
    }
    comp->constants = mc_vallist_make(state, "compilerconstants", 0);
    if(!comp->constants)
    {
        goto compilerinitfailed;
    }
    comp->srcposstack = mc_basicarray_make(state, sizeof(mcastlocation_t));
    if(!comp->srcposstack)
    {
        goto compilerinitfailed;
    }
    comp->modules = mc_ptrdict_make(state, (mcitemcopyfn_t)mc_module_copy, (mcitemdestroyfn_t)mc_module_destroy);
    if(!comp->modules)
    {
        goto compilerinitfailed;
    }
    ok = mc_compiler_pushcompilationscope(comp);
    if(!ok)
    {
        goto compilerinitfailed;
    }
    ok = mc_compiler_filescopepush(comp, "none");
    if(!ok)
    {
        goto compilerinitfailed;
    }
    comp->stringconstposdict = mc_ptrdict_make(comp->pstate, NULL, NULL);
    if(!comp->stringconstposdict)
    {
        goto compilerinitfailed;
    }

    return true;
compilerinitfailed:
    mc_compiler_deinit(comp);
    return false;
}

void mc_compiler_deinit(mcastcompiler_t* comp)
{
    size_t i;
    int* val;
    if(!comp)
    {
        return;
    }
    for(i = 0; i < mc_ptrdict_count(comp->stringconstposdict); i++)
    {
        val = (int*)mc_ptrdict_getvalueat(comp->stringconstposdict, i);
        mc_memory_free(val);
    }
    mc_ptrdict_destroy(comp->stringconstposdict);
    while(mc_ptrlist_count(comp->filescopelist) > 0)
    {
        mc_compiler_filescopepop(comp);
    }
    while(mc_compiler_getcompilationscope(comp))
    {
        mc_compiler_popcompilationscope(comp);
    }
    mc_ptrdict_destroyitemsanddict(comp->modules);
    mc_basicarray_destroy(comp->srcposstack);
    mc_vallist_destroy(comp->constants);
    mc_ptrlist_destroy(comp->filescopelist, NULL);
    memset(comp, 0, sizeof(mcastcompiler_t));
}

bool mc_compiler_initshallowcopy(mcastcompiler_t* copy, mcastcompiler_t* src)
{
    bool ok;
    size_t i;
    int* val;
    int* valcopy;
    char* loadednamecopy;
    const char* key;
    const char* loadedname;
    mcptrdict_t* modulescopy;
    mcvallist_t* constantscopy;
    mcptrlist_t* srcloadedmodulenames;
    mcptrlist_t* copyloadedmodulenames;
    mcastsymtable_t* srcst;
    mcastsymtable_t* srcstocopy;
    mcastsymtable_t* copyst;
    mcastscopefile_t* srcfilescope;
    mcastscopefile_t* copyfilescope;
    ok = mc_compiler_init(copy, src->pstate, src->config, src->mem, src->errors, src->files, src->compglobalstore);
    if(!ok)
    {
        return false;
    }
    srcst = mc_compiler_getsymtable(src);
    MC_ASSERT(mc_ptrlist_count(src->filescopelist) == 1);
    MC_ASSERT(srcst->outer == NULL);
    srcstocopy = mc_symtable_copy(srcst);
    if(!srcstocopy)
    {
        goto compilercopyfailed;
    }
    copyst = mc_compiler_getsymtable(copy);
    mc_symtable_destroy(copyst);
    copyst = NULL;
    mc_compiler_setsymtable(copy, srcstocopy);
    modulescopy = mc_ptrdict_copy(src->modules);
    if(!modulescopy)
    {
        goto compilercopyfailed;
    }
    mc_ptrdict_destroyitemsanddict(copy->modules);
    copy->modules = modulescopy;
    constantscopy = mc_vallist_copy(src->constants);
    if(!constantscopy)
    {
        goto compilercopyfailed;
    }
    mc_vallist_destroy(copy->constants);
    copy->constants = constantscopy;
    for(i = 0; i < mc_ptrdict_count(src->stringconstposdict); i++)
    {
        key = mc_ptrdict_getkeyat(src->stringconstposdict, i);
        val = (int*)mc_ptrdict_getvalueat(src->stringconstposdict, i);
        valcopy = (int*)mc_allocator_malloc(src->pstate, sizeof(int));
        if(!valcopy)
        {
            goto compilercopyfailed;
        }
        *valcopy = *val;
        ok = mc_ptrdict_set(copy->stringconstposdict, key, valcopy);
        if(!ok)
        {
            mc_memory_free(valcopy);
            goto compilercopyfailed;
        }
    }
    srcfilescope = (mcastscopefile_t*)mc_ptrlist_top(src->filescopelist);
    copyfilescope = (mcastscopefile_t*)mc_ptrlist_top(copy->filescopelist);
    srcloadedmodulenames = srcfilescope->loadedmodnames;
    copyloadedmodulenames = copyfilescope->loadedmodnames;
    for(i = 0; i < mc_ptrlist_count(srcloadedmodulenames); i++)
    {
        loadedname = (const char*)mc_ptrlist_get(srcloadedmodulenames, i);
        loadednamecopy = mc_util_strdup(copy->pstate, loadedname);
        if(!loadednamecopy)
        {
            goto compilercopyfailed;
        }
        ok = mc_ptrlist_push(copyloadedmodulenames, loadednamecopy);
        if(!ok)
        {
            mc_memory_free(loadednamecopy);
            goto compilercopyfailed;
        }
    }
    return true;
compilercopyfailed:
    mc_compiler_deinit(copy);
    return false;
}

mcopdefinition_t* mc_parser_makedef(mcopdefinition_t* dest, const char* name, int numop, int opa1, int opa2)
{
    dest->name = name;
    dest->numoperands = numop;
    dest->operandwidths[0] = opa1;
    dest->operandwidths[1] = opa2;
    return dest;
}

#define makecase(dest, opc, name, opnum, opa1, opa2) \
    case opc: \
    { \
        return mc_parser_makedef(def, name, opnum, opa1, opa2); \
    } \
    break;

mcopdefinition_t* mc_opdef_lookup(mcopdefinition_t* def, mcinternopcode_t op)
{
    switch(op)
    {
        makecase(def, MC_OPCODE_HALT, "MC_OPCODE_HALT", 0, 0, 0);
        makecase(def, MC_OPCODE_CONSTANT, "MC_OPCODE_CONSTANT", 1, 2, 0);
        makecase(def, MC_OPCODE_ADD, "MC_OPCODE_ADD", 0, 0, 0);
        makecase(def, MC_OPCODE_POP, "MC_OPCODE_POP", 0, 0, 0);
        makecase(def, MC_OPCODE_SUB, "MC_OPCODE_SUB", 0, 0, 0);
        makecase(def, MC_OPCODE_MUL, "MC_OPCODE_MUL", 0, 0, 0);
        makecase(def, MC_OPCODE_DIV, "MC_OPCODE_DIV", 0, 0, 0);
        makecase(def, MC_OPCODE_MOD, "MC_OPCODE_MOD", 0, 0, 0);
        makecase(def, MC_OPCODE_TRUE, "MC_OPCODE_TRUE", 0, 0, 0);
        makecase(def, MC_OPCODE_FALSE, "MC_OPCODE_FALSE", 0, 0, 0);
        makecase(def, MC_OPCODE_COMPARE, "MC_OPCODE_COMPARE", 0, 0, 0);
        makecase(def, MC_OPCODE_COMPAREEQ, "MC_OPCODE_COMPAREEQ", 0, 0, 0);
        makecase(def, MC_OPCODE_EQUAL, "MC_OPCODE_EQUAL", 0, 0, 0);
        makecase(def, MC_OPCODE_NOTEQUAL, "MC_OPCODE_NOTEQUAL", 0, 0, 0);
        makecase(def, MC_OPCODE_GREATERTHAN, "MC_OPCODE_GREATERTHAN", 0, 0, 0);
        makecase(def, MC_OPCODE_GREATERTHANEQUAL, "MC_OPCODE_GREATERTHANEQUAL", 0, 0, 0);
        makecase(def, MC_OPCODE_MINUS, "MC_OPCODE_MINUS", 0, 0, 0);
        makecase(def, MC_OPCODE_BINNOT, "MC_OPCODE_BINNOT", 0, 0, 0);
        makecase(def, MC_OPCODE_BANG, "MC_OPCODE_BANG", 0, 0, 0);
        makecase(def, MC_OPCODE_JUMP, "MC_OPCODE_JUMP", 1, 2, 0);
        makecase(def, MC_OPCODE_JUMPIFFALSE, "MC_OPCODE_JUMPIFFALSE", 1, 2, 0);
        makecase(def, MC_OPCODE_JUMPIFTRUE, "MC_OPCODE_JUMPIFTRUE", 1, 2, 0);
        makecase(def, MC_OPCODE_NULL, "MC_OPCODE_NULL", 0, 0, 0);
        makecase(def, MC_OPCODE_GETMODULEGLOBAL, "MC_OPCODE_GETMODULEGLOBAL", 1, 2, 0);
        makecase(def, MC_OPCODE_SETMODULEGLOBAL, "MC_OPCODE_SETMODULEGLOBAL", 1, 2, 0);
        makecase(def, MC_OPCODE_DEFINEMODULEGLOBAL, "MC_OPCODE_DEFINEMODULEGLOBAL", 1, 2, 0);
        makecase(def, MC_OPCODE_ARRAY, "MC_OPCODE_ARRAY", 1, 2, 0);
        makecase(def, MC_OPCODE_MAPSTART, "MC_OPCODE_MAPSTART", 1, 2, 0);
        makecase(def, MC_OPCODE_MAPEND, "MC_OPCODE_MAPEND", 1, 2, 0);
        makecase(def, MC_OPCODE_GETTHIS, "MC_OPCODE_GETTHIS", 0, 0, 0);
        makecase(def, MC_OPCODE_GETINDEX, "MC_OPCODE_GETINDEX", 0, 0, 0);
        makecase(def, MC_OPCODE_SETINDEX, "MC_OPCODE_SETINDEX", 0, 0, 0);
        makecase(def, MC_OPCODE_GETDOTINDEX, "MC_OPCODE_GETDOTINDEX", 0, 0, 0);
        makecase(def, MC_OPCODE_GETVALUEAT, "MC_OPCODE_GETVALUEAT", 0, 0, 0);
        makecase(def, MC_OPCODE_CALL, "MC_OPCODE_CALL", 1, 1, 0);
        makecase(def, MC_OPCODE_RETURNVALUE, "MC_OPCODE_RETURNVALUE", 0, 0, 0);
        makecase(def, MC_OPCODE_RETURN, "MC_OPCODE_RETURN", 0, 0, 0);
        makecase(def, MC_OPCODE_GETLOCAL, "MC_OPCODE_GETLOCAL", 1, 1, 0);
        makecase(def, MC_OPCODE_DEFINELOCAL, "MC_OPCODE_DEFINELOCAL", 1, 1, 0);
        makecase(def, MC_OPCODE_SETLOCAL, "MC_OPCODE_SETLOCAL", 1, 1, 0);
        makecase(def, MC_OPCODE_GETGLOBALBUILTIN, "MC_OPCODE_GETGLOBALBUILTIN", 1, 2, 0);
        makecase(def, MC_OPCODE_FUNCTION, "MC_OPCODE_FUNCTION", 2, 2, 1);
        makecase(def, MC_OPCODE_GETFREE, "MC_OPCODE_GETFREE", 1, 1, 0);
        makecase(def, MC_OPCODE_SETFREE, "MC_OPCODE_SETFREE", 1, 1, 0);
        makecase(def, MC_OPCODE_CURRENTFUNCTION, "MC_OPCODE_CURRENTFUNCTION", 0, 0, 0);
        makecase(def, MC_OPCODE_DUP, "MC_OPCODE_DUP", 0, 0, 0);
        makecase(def, MC_OPCODE_NUMBER, "MC_OPCODE_NUMBER", 1, 8, 0);
        makecase(def, MC_OPCODE_FOREACHLEN, "MC_OPCODE_FOREACHLEN", 0, 0, 0);
        makecase(def, MC_OPCODE_SETRECOVER, "MC_OPCODE_SETRECOVER", 1, 2, 0);
        makecase(def, MC_OPCODE_BINOR, "MC_OPCODE_BINOR", 0, 0, 0);
        makecase(def, MC_OPCODE_BINXOR, "MC_OPCODE_BINXOR", 0, 0, 0);
        makecase(def, MC_OPCODE_BINAND, "MC_OPCODE_BINAND", 0, 0, 0);
        makecase(def, MC_OPCODE_LSHIFT, "MC_OPCODE_LSHIFT", 0, 0, 0);
        makecase(def, MC_OPCODE_RSHIFT, "MC_OPCODE_RSHIFT", 0, 0, 0);
        makecase(def, MC_OPCODE_MAX, "MC_OPCODE_MAX", 0, 0, 0);
        default:
            {
                return NULL; 
            }
            break;
    }
    return def;
}
#undef makecase

const char* mc_opdef_getname(mcinternopcode_t op)
{
    mcopdefinition_t def;
    return mc_opdef_lookup(&def, op)->name;
}


#define APPEND_BYTE(n) \
    do \
    { \
        val = (uint16_t)(operands[i] >> (n * 8)); \
        ok = mc_basicarray_push(res, &val); \
        if(!ok) \
        { \
            return 0; \
        } \
    } while(0)

int mc_compiler_gencode(mcinternopcode_t op, int operandscount, const uint64_t* operands, mcbasicarray_t* res)
{
    bool ok;
    int i;
    int width;
    int instrlen;
    uint16_t val;
    mcopdefinition_t vdef;
    mcopdefinition_t* def;
    def = mc_opdef_lookup(&vdef, op);
    if(!def)
    {
        return 0;
    }
    instrlen = 1;
    for(i = 0; i < def->numoperands; i++)
    {
        instrlen += def->operandwidths[i];
    }
    val = op;
    ok = false;
    ok = mc_basicarray_push(res, &val);
    if(!ok)
    {
        return 0;
    }
    for(i = 0; i < operandscount; i++)
    {
        width = def->operandwidths[i];
        switch(width)
        {
            case 1:
                {
                    APPEND_BYTE(0);
                }
                break;
            case 2:
                {
                    APPEND_BYTE(1);
                    APPEND_BYTE(0);
                }
                break;
            case 4:
                {
                    APPEND_BYTE(3);
                    APPEND_BYTE(2);
                    APPEND_BYTE(1);
                    APPEND_BYTE(0);
                }
                break;
            case 8:
                {
                    APPEND_BYTE(7);
                    APPEND_BYTE(6);
                    APPEND_BYTE(5);
                    APPEND_BYTE(4);
                    APPEND_BYTE(3);
                    APPEND_BYTE(2);
                    APPEND_BYTE(1);
                    APPEND_BYTE(0);
                }
                break;
            default:
                {
                    MC_ASSERT(false);
                }
                break;
        }
    }
    return instrlen;
}
#undef APPEND_BYTE


int mc_compiler_emit(mcastcompiler_t* comp, mcinternopcode_t op, int operandscount, uint64_t* operands)
{
    bool ok;
    int i;
    int ip;
    int len;
    mcastlocation_t* srcpos;
    mcastscopecomp_t* compscope;
    ip = mc_compiler_getip(comp);
    len = mc_compiler_gencode(op, operandscount, operands, mc_compiler_getbytecode(comp));
    if(len == 0)
    {
        return -1;
    }
    for(i = 0; i < len; i++)
    {
        srcpos = (mcastlocation_t*)mc_basicarray_top(comp->srcposstack);
        /*
        MC_ASSERT(srcpos->line >= 0);
        MC_ASSERT(srcpos->column >= 0);
        */
        ok = mc_basicarray_push(mc_compiler_getsrcpositions(comp), srcpos);
        if(!ok)
        {
            return -1;
        }
    }
    compscope = mc_compiler_getcompilationscope(comp);
    compscope->lastopcode = op;
    return ip;
}

mcastscopecomp_t* mc_compiler_getcompilationscope(mcastcompiler_t* comp)
{
    return comp->compilationscope;
}

bool mc_compiler_pushcompilationscope(mcastcompiler_t* comp)
{
    mcastscopecomp_t* nscope;
    mcastscopecomp_t* currentscope;
    currentscope = mc_compiler_getcompilationscope(comp);
    nscope = mc_astcompscope_make(comp->pstate, currentscope);
    if(!nscope)
    {
        return false;
    }
    mc_compiler_setcompilationscope(comp, nscope);
    return true;
}

void mc_compiler_popcompilationscope(mcastcompiler_t* comp)
{
    mcastscopecomp_t* currentscope;
    currentscope = mc_compiler_getcompilationscope(comp);
    MC_ASSERT(currentscope);
    mc_compiler_setcompilationscope(comp, currentscope->outer);
    mc_astcompscope_destroy(currentscope);
}

bool mc_compiler_pushsymtable(mcastcompiler_t* comp, int globaloffset)
{
    mcastscopefile_t* filescope;
    mcastsymtable_t* currenttable;
    filescope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    if(!filescope)
    {
        MC_ASSERT(false);
        return false;
    }
    currenttable = filescope->filesymtab;
    filescope->filesymtab = mc_symtable_make(comp->pstate, currenttable, comp->compglobalstore, globaloffset);
    if(!filescope->filesymtab)
    {
        filescope->filesymtab = currenttable;
        return false;
    }
    return true;
}

void mc_compiler_popsymtable(mcastcompiler_t* comp)
{
    mcastscopefile_t* filescope;
    mcastsymtable_t* currenttable;
    filescope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    if(!filescope)
    {
        MC_ASSERT(false);
        return;
    }
    currenttable = filescope->filesymtab;
    if(!currenttable)
    {
        MC_ASSERT(false);
        return;
    }
    filescope->filesymtab = currenttable->outer;
    mc_symtable_destroy(currenttable);
}

mcinternopcode_t mc_compiler_getlastopcode(mcastcompiler_t* comp)
{
    mcastscopecomp_t* currentscope;
    currentscope = mc_compiler_getcompilationscope(comp);
    return currentscope->lastopcode;
}

bool mc_compiler_docompilesource(mcastcompiler_t* comp, const char* code)
{
    bool ok;
    mcstate_t* state;
    mcptrlist_t* statements;
    mcastscopefile_t* filescope;
    state = comp->pstate;
    filescope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    MC_ASSERT(filescope);
    statements = mc_astparser_parseall(filescope->parser, code, filescope->file);
    if(!statements)
    {
        /* errors are added by parser */
        return false;
    }
    if(comp->pstate->config.dumpast)
    {
        mc_astprinter_printast(state, statements);
    }
    ok = mc_compiler_compilestmtlist(comp, statements);
    mc_ptrlist_destroy(statements, (mcitemdestroyfn_t)mc_astexpr_destroy);
    if(comp->pstate->config.dumpbytecode)
    {
        mc_printer_printbytecode(state->stderrprinter,
            (uint16_t*)mc_basicarray_data(comp->compilationscope->compiledscopebytecode),
            (mcastlocation_t*)mc_basicarray_data(comp->compilationscope->scopesrcposlist),
            mc_basicarray_count(comp->compilationscope->compiledscopebytecode), false);
    }
    return ok;
}

bool mc_compiler_compilestmtlist(mcastcompiler_t* comp, mcptrlist_t* statements)
{
    bool ok;
    size_t i;
    mcastexpression_t* expr;
    ok = true;
    for(i = 0; i < mc_ptrlist_count(statements); i++)
    {
        expr = (mcastexpression_t*)mc_ptrlist_get(statements, i);
        ok = mc_compiler_compilestatement(comp, expr);
        if(!ok)
        {
            break;
        }
    }
    return ok;
}


bool mc_compiler_compileimport(mcastcompiler_t* comp, mcastexpression_t* importstmt)
{
    bool ok;
    bool result;
    size_t i;
    size_t flen;
    char* code;
    char* filepath;
    char* namecopy;
    const char* modpath;
    const char* modname;
    const char* loadedname;
    const char* searchedpath;
    const char* filepathnoncanonicalised;
    mcprinter_t* filepathbuf;
    mcastsymtable_t* symtab;
    mcastscopefile_t* fs;
    mcmodule_t* module;
    mcastsymtable_t* st;
    mcastscopefile_t* filescope;
    mcastsymbol_t* symbol;
    /* todo: split into smaller functions */
    result = false;
    filepath = NULL;
    code = NULL;
    filescope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    modpath = importstmt->uexpr.exprimportstmt.path;
    modname = mc_util_getmodulename(modpath);
    for(i = 0; i < mc_ptrlist_count(filescope->loadedmodnames); i++)
    {
        loadedname = (const char*)mc_ptrlist_get(filescope->loadedmodnames, i);
        if(mc_util_strequal(loadedname, modname))
        {
            if(comp->pstate->config.fatalcomplaints)
            {
                mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, importstmt->pos, "module \"%s\" was already imported", modname);
                result = false;
            }
            else
            {
                mc_state_complain(comp->pstate, importstmt->pos, "module \"%s\" already imported; ignoring 'import' statement", modname);
                result = true;
            }
            goto end;
        }
    }
    filepathbuf = mc_printer_make(comp->pstate, NULL);
    if(!filepathbuf)
    {
        result = false;
        goto end;
    }
    if(mc_util_pathisabsolute(modpath))
    {
        mc_printer_printf(filepathbuf, "%s.mc", modpath);
    }
    else
    {
        mc_printer_printf(filepathbuf, "%s%s.mc", filescope->file->dir_path, modpath);
    }

    if(filepathbuf->failed)
    {
        mc_printer_destroy(filepathbuf);
        result = false;
        goto end;
    }
    filepathnoncanonicalised = mc_printer_getstring(filepathbuf);
    filepath = mc_util_canonpath(comp->pstate, filepathnoncanonicalised);
    mc_printer_destroy(filepathbuf);
    if(!filepath)
    {
        result = false;
        goto end;
    }
    symtab = mc_compiler_getsymtable(comp);
    if(symtab->outer != NULL || mc_ptrlist_count(symtab->blockscopes) > 1)
    {
        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, importstmt->pos, "modules can only be imported in global scope");
        result = false;
        goto end;
    }
    for(i = 0; i < mc_ptrlist_count(comp->filescopelist); i++)
    {
        fs = (mcastscopefile_t*)mc_ptrlist_get(comp->filescopelist, i);
        if(MC_UTIL_STREQ(fs->file->path, filepath))
        {
            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, importstmt->pos, "cyclic reference of file \"%s\"", filepath);
            result = false;
            goto end;
        }
    }
    module = (mcmodule_t*)mc_ptrdict_get(comp->modules, filepath);
    if(!module)
    {
        /* todo: create new module function */
        searchedpath = mc_module_findfile(comp->pstate, filepath);
        code = mc_fsutil_fileread(comp->pstate, searchedpath, &flen);
        if(!code)
        {
            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, importstmt->pos, "reading module file \"%s\" failed", filepath);
            result = false;
            goto end;
        }
        module = mc_module_make(comp->pstate, modname);
        if(!module)
        {
            result = false;
            goto end;
        }
        ok = mc_compiler_filescopepush(comp, searchedpath);
        if(!ok)
        {
            mc_module_destroy(module);
            result = false;
            goto end;
        }
        ok = mc_compiler_docompilesource(comp, code);
        if(!ok)
        {
            mc_module_destroy(module);
            result = false;
            goto end;
        }
        st = mc_compiler_getsymtable(comp);
        for(i = 0; i < mc_symtable_getmodglobalsymcount(st); i++)
        {
            symbol = mc_symtable_getmodglobalsymat(st, i);
            mc_module_addsymbol(module, symbol);
        }
        mc_compiler_filescopepop(comp);
        ok = mc_ptrdict_set(comp->modules, filepath, module);
        if(!ok)
        {
            mc_module_destroy(module);
            result = false;
            goto end;
        }
    }
    for(i = 0; i < mc_ptrlist_count(module->modsymbols); i++)
    {
        symbol = (mcastsymbol_t*)mc_ptrlist_get(module->modsymbols, i);
        ok = mc_symtable_addmodsymbol(symtab, symbol);
        if(!ok)
        {
            result = false;
            goto end;
        }
    }
    namecopy = mc_util_strdup(comp->pstate, modname);
    if(!namecopy)
    {
        result = false;
        goto end;
    }
    ok = mc_ptrlist_push(filescope->loadedmodnames, namecopy);
    if(!ok)
    {
        mc_memory_free(namecopy);
        result = false;
        goto end;
    }
    result = true;
end:
    mc_memory_free(filepath);
    mc_memory_free(code);
    return result;
}

mcastsymbol_t* mc_compiler_defsymbol(mcastcompiler_t* comp, mcastlocation_t pos, const char* name, bool assignable, bool canshadow)
{
    mcastsymbol_t* symbol;
    mcastsymbol_t* currentsymbol;
    mcastsymtable_t* symtab;
    symtab = mc_compiler_getsymtable(comp);
    if(!canshadow && !mc_symtable_istopglobalscope(symtab))
    {
        currentsymbol = mc_symtable_resolve(symtab, name);
        if(currentsymbol)
        {
            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, pos, "symbol \"%s\" is already defined", name);
            return NULL;
        }
    }
    symbol = mc_symtable_define(symtab, name, assignable);
    if(!symbol)
    {
        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, pos, "cannot define symbol \"%s\"", name);
        return NULL;
    }
    return symbol;
}

bool mc_compiler_compiledefine(mcastcompiler_t* comp, mcastexpression_t* expr)
{
    bool ok;
    mcastsymbol_t* symbol;
    ok = mc_compiler_compileexpression(comp, expr->uexpr.exprdefine.value);
    if(!ok)
    {
        return false;
    }
    symbol = mc_compiler_defsymbol(comp, expr->uexpr.exprdefine.name->pos, expr->uexpr.exprdefine.name->value, expr->uexpr.exprdefine.assignable, false);
    if(!symbol)
    {
        return false;
    }
    ok = mc_compiler_storesymbol(comp, symbol, true);
    if(!ok)
    {
        return false;
    }
    return true;
}

bool mc_compiler_compileifstmt(mcastcompiler_t* comp, uint64_t* opbuf, mcastexpression_t* expr)
{
    bool ok;
    size_t i;
    int afteraltip;
    int nextcasejumpip;
    int jumptoendip;
    int afterelifip;
    int* pos;
    mcastexprifcase_t* ifcase;
    mcastexprstmtif_t* ifstmt;
    mcbasicarray_t* jumptoendips;
    ifstmt = &expr->uexpr.exprifstmt;
    jumptoendips = mc_basicarray_make(comp->pstate, sizeof(int));
    if(!jumptoendips)
    {
        goto statementiferror;
    }
    for(i = 0; i < mc_ptrlist_count(ifstmt->cases); i++)
    {
        ifcase = (mcastexprifcase_t*)mc_ptrlist_get(ifstmt->cases, i);
        ok = mc_compiler_compileexpression(comp, ifcase->ifcond);
        if(!ok)
        {
            goto statementiferror;
        }
        opbuf[0] = 0xbeef;
        nextcasejumpip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFFALSE, 1, opbuf);
        ok = mc_compiler_compilecodeblock(comp, ifcase->consequence);
        if(!ok)
        {
            goto statementiferror;
        }
        /* don't emit jump for the last statement */
        if(i < (mc_ptrlist_count(ifstmt->cases) - 1) || ifstmt->alternative)
        {
            opbuf[0] = 0xbeef;
            jumptoendip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
            ok = mc_basicarray_push(jumptoendips, &jumptoendip);
            if(!ok)
            {
                goto statementiferror;
            }
        }
        afterelifip = mc_compiler_getip(comp);
        mc_compiler_changeuint16operand(comp, nextcasejumpip + 1, afterelifip);
    }
    if(ifstmt->alternative)
    {
        ok = mc_compiler_compilecodeblock(comp, ifstmt->alternative);
        if(!ok)
        {
            goto statementiferror;
        }
    }
    afteraltip = mc_compiler_getip(comp);
    for(i = 0; i < mc_basicarray_count(jumptoendips); i++)
    {
        pos = (int*)mc_basicarray_get(jumptoendips, i);
        mc_compiler_changeuint16operand(comp, *pos + 1, afteraltip);
    }
    mc_basicarray_destroy(jumptoendips);
    return true;
statementiferror:
    mc_basicarray_destroy(jumptoendips);
    return false;
}

bool mc_compiler_compilereturnstmt(mcastcompiler_t* comp, mcastscopecomp_t* compscope, mcastexpression_t* expr)
{
    bool ok;
    int ip;
    if(compscope->outer == NULL)
    {
        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "nothing to return from");
        return false;
    }
    ip = -1;
    if(expr->uexpr.exprreturnvalue)
    {
        ok = mc_compiler_compileexpression(comp, expr->uexpr.exprreturnvalue);
        if(!ok)
        {
            return false;
        }
        ip = mc_compiler_emit(comp, MC_OPCODE_RETURNVALUE, 0, NULL);
    }
    else
    {
        ip = mc_compiler_emit(comp, MC_OPCODE_RETURN, 0, NULL);
    }
    if(ip < 0)
    {
        return false;
    }
    return true;
}

bool mc_compiler_compilewhilestmt(mcastcompiler_t* comp, uint64_t* opbuf, mcastexpression_t* expr)
{
    bool ok;
    int ip;
    int beforetestip;
    int aftertestip;
    int afterbodyip;
    int jumptoafterbodyip;
    mcastexprstmtwhile_t* loop;
    loop = &expr->uexpr.exprwhileloopstmt;
    beforetestip = mc_compiler_getip(comp);
    ok = mc_compiler_compileexpression(comp, loop->loopcond);
    if(!ok)
    {
        return false;
    }
    aftertestip = mc_compiler_getip(comp);
    opbuf[0] = aftertestip + 6;
    ip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFTRUE, 1, opbuf);
    if(ip < 0)
    {
        return false;
    }
    opbuf[0] = 0xdead;
    jumptoafterbodyip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
    if(jumptoafterbodyip < 0)
    {
        return false;
    }
    ok = mc_compiler_pushcontinueip(comp, beforetestip);
    if(!ok)
    {
        return false;
    }
    ok = mc_compiler_pushbreakip(comp, jumptoafterbodyip);
    if(!ok)
    {
        return false;
    }
    ok = mc_compiler_compilecodeblock(comp, loop->body);
    if(!ok)
    {
        return false;
    }
    mc_compiler_popbreakip(comp);
    mc_compiler_popcontinueip(comp);
    opbuf[0] = beforetestip;
    ip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
    if(ip < 0)
    {
        return false;
    }
    afterbodyip = mc_compiler_getip(comp);
    mc_compiler_changeuint16operand(comp, jumptoafterbodyip + 1, afterbodyip);
    return true;
}

bool mc_compiler_compilebreakstmt(mcastcompiler_t* comp, uint64_t* opbuf, mcastexpression_t* expr)
{
    int ip;
    int breakip;
    breakip = mc_compiler_getbreakip(comp);
    if(breakip < 0)
    {
        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "nothing to break from.");
        return false;
    }
    opbuf[0] = breakip;
    ip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
    if(ip < 0)
    {
        return false;
    }
    return true;
}

bool mc_compiler_compilecontinuestmt(mcastcompiler_t* comp, uint64_t* opbuf, mcastexpression_t* expr)
{
    int ip;
    int continueip;
    continueip = mc_compiler_getcontinueip(comp);
    if(continueip < 0)
    {
        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "nothing to continue from.");
        return false;
    }
    opbuf[0] = continueip;
    ip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
    if(ip < 0)
    {
        return false;
    }
    return true;
}

bool mc_compiler_compilestatement(mcastcompiler_t* comp, mcastexpression_t* expr)
{
    bool ok;
    int ip;
    uint64_t opbuf[10];
    mcastscopecomp_t* compscope;
    mcastsymtable_t* symtab;
    ok = false;
    ip = -1;
    ok = mc_basicarray_push(comp->srcposstack, &expr->pos);
    if(!ok)
    {
        return false;
    }
    compscope = mc_compiler_getcompilationscope(comp);
    symtab = mc_compiler_getsymtable(comp);
    switch(expr->exprtype)
    {
        case MC_EXPR_STMTEXPRESSION:
            {
                ok = mc_compiler_compileexpression(comp, expr->uexpr.exprexpression);
                if(!ok)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, NULL);
                if(ip < 0)
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTDEFINE:
            {
                if(!mc_compiler_compiledefine(comp, expr))
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTIF:
            {
                if(!mc_compiler_compileifstmt(comp, opbuf, expr))
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTRETURN:
            {
                if(!mc_compiler_compilereturnstmt(comp, compscope, expr))
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTLOOPWHILE:
            {
                if(!mc_compiler_compilewhilestmt(comp, opbuf, expr))
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTBREAK:
            {
                if(!mc_compiler_compilebreakstmt(comp, opbuf, expr))
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTCONTINUE:
            {
                if(!mc_compiler_compilecontinuestmt(comp, opbuf, expr))
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTLOOPFOREACH:
            {
                int jumptoafterupdateip;
                int updateip;
                int afterupdateip;
                int aftertestip;
                int jumptoafterbodyip;
                int afterbodyip;
                mcastsymbol_t* itersymbol;
                mcastsymbol_t* indexsymbol;
                mcastsymbol_t* sourcesymbol;
                mcastexprstmtforeach_t* foreach;
                foreach = &expr->uexpr.exprforeachloopstmt;
                ok = mc_symtable_pushblockscope(symtab);
                if(!ok)
                {
                    return false;
                }
                /* Init */
                indexsymbol = mc_compiler_defsymbol(comp, expr->pos, "@i", false, true);
                if(!indexsymbol)
                {
                    return false;
                }
                opbuf[0] = 0;
                ip = mc_compiler_emit(comp, MC_OPCODE_NUMBER, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                ok = mc_compiler_storesymbol(comp, indexsymbol, true);
                if(!ok)
                {
                    return false;
                }
                sourcesymbol = NULL;
                if(foreach->source->exprtype == MC_EXPR_IDENT)
                {
                    sourcesymbol = mc_symtable_resolve(symtab, foreach->source->uexpr.exprident->value);
                    if(!sourcesymbol)
                    {
                        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, foreach->source->pos, "symbol \"%s\" could not be resolved", foreach->source->uexpr.exprident->value);
                        return false;
                    }
                }
                else
                {
                    ok = mc_compiler_compileexpression(comp, foreach->source);
                    if(!ok)
                    {
                        return false;
                    }
                    sourcesymbol = mc_compiler_defsymbol(comp, foreach->source->pos, "@source", false, true);
                    if(!sourcesymbol)
                    {
                        return false;
                    }
                    ok = mc_compiler_storesymbol(comp, sourcesymbol, true);
                    if(!ok)
                    {
                        return false;
                    }
                }
                /* Update */
                opbuf[0] = 0xbeef;
                jumptoafterupdateip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
                if(jumptoafterupdateip < 0)
                {
                    return false;
                }
                updateip = mc_compiler_getip(comp);
                ok = mc_compiler_readsymbol(comp, indexsymbol);
                if(!ok)
                {
                    return false;
                }
                opbuf[0] = mc_util_doubletouint64(1);
                ip = mc_compiler_emit(comp, MC_OPCODE_NUMBER, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_ADD, 0, NULL);
                if(ip < 0)
                {
                    return false;
                }
                ok = mc_compiler_storesymbol(comp, indexsymbol, false);
                if(!ok)
                {
                    return false;
                }
                afterupdateip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, jumptoafterupdateip + 1, afterupdateip);
                /* Test */
                ok = mc_basicarray_push(comp->srcposstack, &foreach->source->pos);
                if(!ok)
                {
                    return false;
                }
                ok = mc_compiler_readsymbol(comp, sourcesymbol);
                if(!ok)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_FOREACHLEN, 0, NULL);
                if(ip < 0)
                {
                    return false;
                }
                mc_basicarray_pop(comp->srcposstack, NULL);
                ok = mc_compiler_readsymbol(comp, indexsymbol);
                if(!ok)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_COMPARE, 0, NULL);
                if(ip < 0)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_EQUAL, 0, NULL);
                if(ip < 0)
                {
                    return false;
                }
                aftertestip = mc_compiler_getip(comp);
                opbuf[0] = aftertestip + 6;
                ip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFFALSE, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                opbuf[0] = 0xdead;
                jumptoafterbodyip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
                if(jumptoafterbodyip < 0)
                {
                    return false;
                }
                ok = mc_compiler_readsymbol(comp, sourcesymbol);
                if(!ok)
                {
                    return false;
                }
                ok = mc_compiler_readsymbol(comp, indexsymbol);
                if(!ok)
                {
                    return false;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_GETVALUEAT, 0, NULL);
                if(ip < 0)
                {
                    return false;
                }
                itersymbol = mc_compiler_defsymbol(comp, foreach->iterator->pos, foreach->iterator->value, false, false);
                if(!itersymbol)
                {
                    return false;
                }
                ok = mc_compiler_storesymbol(comp, itersymbol, true);
                if(!ok)
                {
                    return false;
                }
                /* Body */
                ok = mc_compiler_pushcontinueip(comp, updateip);
                if(!ok)
                {
                    return false;
                }
                ok = mc_compiler_pushbreakip(comp, jumptoafterbodyip);
                if(!ok)
                {
                    return false;
                }
                ok = mc_compiler_compilecodeblock(comp, foreach->body);
                if(!ok)
                {
                    return false;
                }
                mc_compiler_popbreakip(comp);
                mc_compiler_popcontinueip(comp);
                opbuf[0] = updateip;
                ip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                afterbodyip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, jumptoafterbodyip + 1, afterbodyip);
                mc_symtable_popblockscope(symtab);
            }
            break;
        case MC_EXPR_STMTLOOPFORCLASSIC:
            {
                int afterbodyip;
                int jumptoafterupdateip;
                int updateip;
                int afterupdateip;
                int aftertestip;
                int jumptoafterbodyip;
                mcastexprstmtforloop_t* loop;
                loop = &expr->uexpr.exprforloopstmt;
                ok = mc_symtable_pushblockscope(symtab);
                if(!ok)
                {
                    return false;
                }
                /* Init */
                jumptoafterupdateip = 0;
                ok = false;
                if(loop->init)
                {
                    ok = mc_compiler_compilestatement(comp, loop->init);
                    if(!ok)
                    {
                        return false;
                    }
                    opbuf[0] = 0xbeef;
                    jumptoafterupdateip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
                    if(jumptoafterupdateip < 0)
                    {
                        return false;
                    }
                }
                /* Update */
                updateip = mc_compiler_getip(comp);
                if(loop->update)
                {
                    ok = mc_compiler_compileexpression(comp, loop->update);
                    if(!ok)
                    {
                        return false;
                    }
                    ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, NULL);
                    if(ip < 0)
                    {
                        return false;
                    }
                }
                if(loop->init)
                {
                    afterupdateip = mc_compiler_getip(comp);
                    mc_compiler_changeuint16operand(comp, jumptoafterupdateip + 1, afterupdateip);
                }
                /* Test */
                if(loop->loopcond)
                {
                    ok = mc_compiler_compileexpression(comp, loop->loopcond);
                    if(!ok)
                    {
                        return false;
                    }
                }
                else
                {
                    ip = mc_compiler_emit(comp, MC_OPCODE_TRUE, 0, NULL);
                    if(ip < 0)
                    {
                        return false;
                    }
                }
                aftertestip = mc_compiler_getip(comp);
                opbuf[0] = aftertestip + 6;
                ip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFTRUE, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                opbuf[0] = 0xdead;
                jumptoafterbodyip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
                if(jumptoafterbodyip < 0)
                {
                    return false;
                }
                /* Body */
                ok = mc_compiler_pushcontinueip(comp, updateip);
                if(!ok)
                {
                    return false;
                }
                ok = mc_compiler_pushbreakip(comp, jumptoafterbodyip);
                if(!ok)
                {
                    return false;
                }

                ok = mc_compiler_compilecodeblock(comp, loop->body);
                if(!ok)
                {
                    return false;
                }
                mc_compiler_popbreakip(comp);
                mc_compiler_popcontinueip(comp);
                opbuf[0] = updateip;
                ip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
                if(ip < 0)
                {
                    return false;
                }
                afterbodyip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, jumptoafterbodyip + 1, afterbodyip);
                mc_symtable_popblockscope(symtab);
            }
            break;
        case MC_EXPR_STMTBLOCK:
            {
                ok = mc_compiler_compilecodeblock(comp, expr->uexpr.exprblockstmt);
                if(!ok)
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTIMPORT:
            {
                ok = mc_compiler_compileimport(comp, expr);
                if(!ok)
                {
                    return false;
                }
            }
            break;
        case MC_EXPR_STMTRECOVER:
            {
                int recip;
                int afterrecoverip;
                int afterjumptorecoverip;
                int jumptoafterrecoverip;
                mcastsymbol_t* errorsymbol;
                mcastexprstmtrecover_t* recover;
                recover = &expr->uexpr.exprrecoverstmt;
                #if 0
                if(mc_symtable_ismodglobalscope(symtab))
                {
                    mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "recover statement cannot be defined in global scope");
                    return false;
                }
                #endif
                if(!mc_symtable_istopblockscope(symtab))
                {
                    mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "recover statement cannot be defined within other statements");
                    return false;
                }
                opbuf[0] = 0xbeef;
                recip = mc_compiler_emit(comp, MC_OPCODE_SETRECOVER, 1, opbuf);
                if(recip < 0)
                {
                    return false;
                }
                opbuf[0] = 0xbeef;
                jumptoafterrecoverip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
                if(jumptoafterrecoverip < 0)
                {
                    return false;
                }
                afterjumptorecoverip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, recip + 1, afterjumptorecoverip);
                ok = mc_symtable_pushblockscope(symtab);
                if(!ok)
                {
                    return false;
                }
                errorsymbol = mc_compiler_defsymbol(comp, recover->errident->pos, recover->errident->value, false, false);
                if(!errorsymbol)
                {
                    return false;
                }
                ok = mc_compiler_storesymbol(comp, errorsymbol, true);
                if(!ok)
                {
                    return false;
                }
                ok = mc_compiler_compilecodeblock(comp, recover->body);
                if(!ok)
                {
                    return false;
                }
                if(!mc_compiler_lastopcodeis(comp, MC_OPCODE_RETURN) && !mc_compiler_lastopcodeis(comp, MC_OPCODE_RETURNVALUE))
                {
                    #if 0
                        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "recover body must end with a return statement");
                        return false;
                    #else
                        mc_state_complain(comp->pstate, expr->pos, "recover body should end with a return statement");
                    #endif
                }
                mc_symtable_popblockscope(symtab);
                afterrecoverip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, jumptoafterrecoverip + 1, afterrecoverip);
            }
            break;
        default:
            {
                MC_ASSERT(false);
                return false;
            }
            break;
    }
    mc_basicarray_pop(comp->srcposstack, NULL);
    return true;
}

bool mc_compiler_compileexpression(mcastcompiler_t* comp, mcastexpression_t* expr)
{
    bool ok;
    bool res;
    int ip;
    uint64_t opbuf[10];
    mcastexpression_t* exproptimized;
    mcastscopecomp_t* compscope;
    mcastsymtable_t* symtab;
    ok = false;
    ip = -1;
    exproptimized = NULL;
    #if 0
    exproptimized = mc_optimizer_optexpression(expr);
    if(exproptimized != NULL)
    {
        expr = exproptimized;
    }
    #endif
    ok = mc_basicarray_push(comp->srcposstack, &expr->pos);
    if(!ok)
    {
        return false;
    }
    compscope = mc_compiler_getcompilationscope(comp);
    symtab = mc_compiler_getsymtable(comp);
    res = false;
    switch(expr->exprtype)
    {
        case MC_EXPR_INFIX:
            {
                bool rearrange;
                mcinternopcode_t op;
                mcastexpression_t* left;
                mcastexpression_t* right;
                rearrange = false;
                op = MC_OPCODE_HALT;
                switch(expr->uexpr.exprinfix.op)
                {
                    case MC_MATHOP_PLUS:
                        op = MC_OPCODE_ADD;
                        break;
                    case MC_MATHOP_MINUS:
                        op = MC_OPCODE_SUB;
                        break;
                    case MC_MATHOP_ASTERISK:
                        op = MC_OPCODE_MUL;
                        break;
                    case MC_MATHOP_SLASH:
                        op = MC_OPCODE_DIV;
                        break;
                    case MC_MATHOP_MODULUS:
                        op = MC_OPCODE_MOD;
                        break;
                    case MC_MATHOP_EQ:
                        op = MC_OPCODE_EQUAL;
                        break;
                    case MC_MATHOP_NOTEQ:
                        op = MC_OPCODE_NOTEQUAL;
                        break;
                    case MC_MATHOP_GT:
                        op = MC_OPCODE_GREATERTHAN;
                        break;
                    case MC_MATHOP_GTE:
                        op = MC_OPCODE_GREATERTHANEQUAL;
                        break;
                    case MC_MATHOP_LT:
                        op = MC_OPCODE_GREATERTHAN;
                        rearrange = true;
                        break;
                    case MC_MATHOP_LTE:
                        op = MC_OPCODE_GREATERTHANEQUAL;
                        rearrange = true;
                        break;
                    case MC_MATHOP_BINOR:
                        op = MC_OPCODE_BINOR;
                        break;
                    case MC_MATHOP_BINXOR:
                        op = MC_OPCODE_BINXOR;
                        break;
                    case MC_MATHOP_BINAND:
                        op = MC_OPCODE_BINAND;
                        break;
                    case MC_MATHOP_LSHIFT:
                        op = MC_OPCODE_LSHIFT;
                        break;
                    case MC_MATHOP_RSHIFT:
                        op = MC_OPCODE_RSHIFT;
                        break;
                    default:
                        {
                            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "unknown infix operator");
                            goto error;
                        }
                        break;
                }
                left = rearrange ? expr->uexpr.exprinfix.right : expr->uexpr.exprinfix.left;
                right = rearrange ? expr->uexpr.exprinfix.left : expr->uexpr.exprinfix.right;
                ok = mc_compiler_compileexpression(comp, left);
                if(!ok)
                {
                    goto error;
                }
                ok = mc_compiler_compileexpression(comp, right);
                if(!ok)
                {
                    goto error;
                }
                switch(expr->uexpr.exprinfix.op)
                {
                    case MC_MATHOP_EQ:
                    case MC_MATHOP_NOTEQ:
                        {
                            ip = mc_compiler_emit(comp, MC_OPCODE_COMPAREEQ, 0, NULL);
                            if(ip < 0)
                            {
                                goto error;
                            }
                        }
                        break;
                    case MC_MATHOP_GT:
                    case MC_MATHOP_GTE:
                    case MC_MATHOP_LT:
                    case MC_MATHOP_LTE:
                        {
                            ip = mc_compiler_emit(comp, MC_OPCODE_COMPARE, 0, NULL);
                            if(ip < 0)
                            {
                                goto error;
                            }
                        }
                        break;
                    default:
                        {
                        }
                        break;
                }
                ip = mc_compiler_emit(comp, op, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;

        case MC_EXPR_NUMBERLITERAL:
            {
                mcfloat_t number;
                number = expr->uexpr.exprlitnumber;
                opbuf[0] = mc_util_doubletouint64(number);
                ip = mc_compiler_emit(comp, MC_OPCODE_NUMBER, 1, opbuf);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;

        case MC_EXPR_STRINGLITERAL:
            {
                int pos = 0;
                int* posval;
                int* currentpos;
                mcvalue_t obj;
                currentpos = (int*)mc_ptrdict_get(comp->stringconstposdict, expr->uexpr.exprlitstring.data);
                if(currentpos)
                {
                    pos = *currentpos;
                }
                else
                {
                    obj = mc_value_makestringlen(comp->pstate, expr->uexpr.exprlitstring.data, expr->uexpr.exprlitstring.length);
                    if(mc_value_isnull(obj))
                    {
                        goto error;
                    }
                    pos = mc_compiler_addconstant(comp, obj);
                    if(pos < 0)
                    {
                        goto error;
                    }
                    posval = (int*)mc_allocator_malloc(comp->pstate, sizeof(int));
                    if(!posval)
                    {
                        goto error;
                    }
                    *posval = pos;
                    ok = mc_ptrdict_set(comp->stringconstposdict, expr->uexpr.exprlitstring.data, posval);
                    if(!ok)
                    {
                        mc_memory_free(posval);
                        goto error;
                    }
                }
                opbuf[0] = pos;
                ip = mc_compiler_emit(comp, MC_OPCODE_CONSTANT, 1, opbuf);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_NULLLITERAL:
            {
                ip = mc_compiler_emit(comp, MC_OPCODE_NULL, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_BOOLLITERAL:
            {
                ip = mc_compiler_emit(comp, expr->uexpr.exprlitbool ? MC_OPCODE_TRUE : MC_OPCODE_FALSE, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_ARRAYLITERAL:
            {
                size_t i;
                for(i = 0; i < mc_ptrlist_count(expr->uexpr.exprlitarray.litarritems); i++)
                {
                    ok = mc_compiler_compileexpression(comp, (mcastexpression_t*)mc_ptrlist_get(expr->uexpr.exprlitarray.litarritems, i));
                    if(!ok)
                    {
                        goto error;
                    }
                }
                opbuf[0] = mc_ptrlist_count(expr->uexpr.exprlitarray.litarritems);
                ip = mc_compiler_emit(comp, MC_OPCODE_ARRAY, 1, opbuf);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_MAPLITERAL:
            {
                size_t i;
                size_t len;
                mcastexpression_t* key;
                mcastexpression_t* val;
                mcastexprliteralmap_t* map;
                map = &expr->uexpr.exprlitmap;
                len = mc_ptrlist_count(map->litmapkeys);
                opbuf[0] = len;
                ip = mc_compiler_emit(comp, MC_OPCODE_MAPSTART, 1, opbuf);
                if(ip < 0)
                {
                    goto error;
                }
                for(i = 0; i < len; i++)
                {
                    key = (mcastexpression_t*)mc_ptrlist_get(map->litmapkeys, i);
                    val = (mcastexpression_t*)mc_ptrlist_get(map->litmapvalues, i);
                    ok = mc_compiler_compileexpression(comp, key);
                    if(!ok)
                    {
                        goto error;
                    }
                    ok = mc_compiler_compileexpression(comp, val);
                    if(!ok)
                    {
                        goto error;
                    }
                }
                opbuf[0] = len;
                ip = mc_compiler_emit(comp, MC_OPCODE_MAPEND, 1, opbuf);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_PREFIX:
            {
                mcinternopcode_t op;
                ok = mc_compiler_compileexpression(comp, expr->uexpr.exprprefix.right);
                if(!ok)
                {
                    goto error;
                }
                op = MC_OPCODE_HALT;
                switch(expr->uexpr.exprprefix.op)
                {
                    case MC_MATHOP_MINUS:
                        op = MC_OPCODE_MINUS;
                        break;
                    case MC_MATHOP_BINNOT:
                        op = MC_OPCODE_BINNOT;
                        break;
                    case MC_MATHOP_BANG:
                        op = MC_OPCODE_BANG;
                        break;
                    default:
                        {
                            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "unknown prefix operator.");
                            goto error;
                        }
                        break;
                }
                ip = mc_compiler_emit(comp, op, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_IDENT:
            {
                mcastsymbol_t* symbol;
                mcastexprident_t* ident;
                ident = expr->uexpr.exprident;
                symbol = mc_symtable_resolve(symtab, ident->value);
                if(!symbol)
                {
                    #if 0
                        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, ident->pos, "compilation: failed to resolve symbol \"%s\"", ident->value);
                        goto error;
                    #else
                        symbol = mc_compiler_defsymbol(comp, ident->pos, ident->value, true, false);
                    #endif

                }
                ok = mc_compiler_readsymbol(comp, symbol);
                if(!ok)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_INDEX:
            {
                mcastexprindex_t* index;
                index = &expr->uexpr.exprindex;
                ok = mc_compiler_compileexpression(comp, index->left);
                if(!ok)
                {
                    goto error;
                }
                ok = mc_compiler_compileexpression(comp, index->index);
                if(!ok)
                {
                    goto error;
                }
                #if 1
                if(index->isdot)
                {
                    ip = mc_compiler_emit(comp, MC_OPCODE_GETDOTINDEX, 0, NULL);
                }
                else
                #endif
                {
                    ip = mc_compiler_emit(comp, MC_OPCODE_GETINDEX, 0, NULL);
                }
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_FUNCTIONLITERAL:
            {
                size_t i;
                int pos;
                int nlocals;
                mcvalue_t obj;
                mcptrlist_t* freesyms;
                mccompiledprogram_t* comp_res;
                mcastexprliteralfunction_t* fn;
                mcastsymbol_t* symbol;
                mcastsymbol_t* fnsymbol;
                mcastsymbol_t* thissymbol;
                mcastsymbol_t* paramsymbol;
                mcastfuncparam_t* param;
                fn = &expr->uexpr.exprlitfunction;
                ok = mc_compiler_pushcompilationscope(comp);
                if(!ok)
                {
                    goto error;
                }
                ok = mc_compiler_pushsymtable(comp, 0);
                if(!ok)
                {
                    goto error;
                }
                compscope = mc_compiler_getcompilationscope(comp);
                symtab = mc_compiler_getsymtable(comp);
                if(fn->name)
                {
                    fnsymbol = mc_symtable_definefunctionname(symtab, fn->name, false);
                    if(!fnsymbol)
                    {
                        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "cannot define symbol \"%s\"", fn->name);
                        goto error;
                    }
                }
                thissymbol = mc_symtable_definethis(symtab);
                if(!thissymbol)
                {
                    mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, expr->pos, "cannot define \"this\" symbol");
                    goto error;
                }
                for(i = 0; i < mc_ptrlist_count(expr->uexpr.exprlitfunction.funcparamlist); i++)
                {
                    param = (mcastfuncparam_t*)mc_ptrlist_get(expr->uexpr.exprlitfunction.funcparamlist, i);
                    paramsymbol = mc_compiler_defsymbol(comp, param->ident->pos, param->ident->value, true, false);
                    if(!paramsymbol)
                    {
                        goto error;
                    }
                }
                ok = mc_compiler_compilestmtlist(comp, fn->body->statements);
                if(!ok)
                {
                    goto error;
                }
                if(!mc_compiler_lastopcodeis(comp, MC_OPCODE_RETURNVALUE) && !mc_compiler_lastopcodeis(comp, MC_OPCODE_RETURN))
                {
                    ip = mc_compiler_emit(comp, MC_OPCODE_RETURN, 0, NULL);
                    if(ip < 0)
                    {
                        goto error;
                    }
                }
                freesyms = symtab->freesymbols;
                /* because it gets destroyed with compiler_pop_compilation_scope() */
                symtab->freesymbols = NULL;
                nlocals = symtab->maxnumdefinitions;
                comp_res = mc_astcompscope_orphanresult(compscope);
                if(!comp_res)
                {
                    mc_ptrlist_destroy(freesyms, (mcitemdestroyfn_t)mc_symbol_destroy);
                    goto error;
                }
                mc_compiler_popsymtable(comp);
                mc_compiler_popcompilationscope(comp);
                compscope = mc_compiler_getcompilationscope(comp);
                symtab = mc_compiler_getsymtable(comp);
                obj = mc_value_makefuncscript(comp->pstate, fn->name, comp_res, true, nlocals, mc_ptrlist_count(fn->funcparamlist), 0);
                if(mc_value_isnull(obj))
                {
                    mc_ptrlist_destroy(freesyms, (mcitemdestroyfn_t)mc_symbol_destroy);
                    mc_astcompresult_destroy(comp_res);
                    goto error;
                }
                for(i = 0; i < mc_ptrlist_count(freesyms); i++)
                {
                    symbol = (mcastsymbol_t*)mc_ptrlist_get(freesyms, i);
                    ok = mc_compiler_readsymbol(comp, symbol);
                    if(!ok)
                    {
                        mc_ptrlist_destroy(freesyms, (mcitemdestroyfn_t)mc_symbol_destroy);
                        goto error;
                    }
                }
                pos = mc_compiler_addconstant(comp, obj);
                if(pos < 0)
                {
                    mc_ptrlist_destroy(freesyms, (mcitemdestroyfn_t)mc_symbol_destroy);
                    goto error;
                }
                opbuf[0] = pos;
                opbuf[1] = mc_ptrlist_count(freesyms);
                ip = mc_compiler_emit(comp, MC_OPCODE_FUNCTION, 2, opbuf);
                if(ip < 0)
                {
                    mc_ptrlist_destroy(freesyms, (mcitemdestroyfn_t)mc_symbol_destroy);
                    goto error;
                }
                mc_ptrlist_destroy(freesyms, (mcitemdestroyfn_t)mc_symbol_destroy);
            }
            break;

        case MC_EXPR_CALL:
            {
                size_t i;
                mcastexpression_t* argexpr;
                ok = mc_compiler_compileexpression(comp, expr->uexpr.exprcall.function);
                if(!ok)
                {
                    goto error;
                }
                for(i = 0; i < mc_ptrlist_count(expr->uexpr.exprcall.args); i++)
                {
                    argexpr = (mcastexpression_t*)mc_ptrlist_get(expr->uexpr.exprcall.args, i);
                    ok = mc_compiler_compileexpression(comp, argexpr);
                    if(!ok)
                    {
                        goto error;
                    }
                }
                opbuf[0] = mc_ptrlist_count(expr->uexpr.exprcall.args);
                ip = mc_compiler_emit(comp, MC_OPCODE_CALL, 1, opbuf);
                if(ip < 0)
                {
                    goto error;
                }
            }
            break;
        case MC_EXPR_ASSIGN:
            {
                mcastexprindex_t* index;
                mcastexprassign_t* assign;
                mcastexprident_t* ident;
                mcastsymbol_t* symbol;
                assign = &expr->uexpr.exprassign;
                if(assign->dest->exprtype != MC_EXPR_IDENT && assign->dest->exprtype != MC_EXPR_INDEX)
                {
                    mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, assign->dest->pos, "expression is not assignable");
                    goto error;
                }
                if(assign->is_postfix)
                {
                    ok = mc_compiler_compileexpression(comp, assign->dest);
                    if(!ok)
                    {
                        goto error;
                    }
                }
                ok = mc_compiler_compileexpression(comp, assign->source);
                if(!ok)
                {
                    goto error;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_DUP, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
                ok = mc_basicarray_push(comp->srcposstack, &assign->dest->pos);
                if(!ok)
                {
                    goto error;
                }
                if(assign->dest->exprtype == MC_EXPR_IDENT)
                {
                    ident = assign->dest->uexpr.exprident;
                    symbol = mc_symtable_resolve(symtab, ident->value);
                    if(!symbol)
                    {
                        #if 0
                            mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, assign->dest->pos, "cannot assign to undeclared symbol \"%s\"", ident->value);
                            goto error;

                        #else
                            symbol = mc_compiler_defsymbol(comp, ident->pos, ident->value, true, false);
                            if(!symbol)
                            {
                                mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, assign->dest->pos, "failed to implicitly create symbol \"%s\"", ident->value);
                                goto error;
                            }
                        #endif
                    }
                    if(!symbol->assignable)
                    {
                        mc_errlist_addf(comp->errors, MC_ERROR_COMPILING, assign->dest->pos, "compilation: cannot assign to readonly symbol \"%s\"", ident->value);
                        goto error;
                    }
                    ok = mc_compiler_storesymbol(comp, symbol, false);
                    if(!ok)
                    {
                        goto error;
                    }
                }
                else if(assign->dest->exprtype == MC_EXPR_INDEX)
                {
                    index = &assign->dest->uexpr.exprindex;
                    ok = mc_compiler_compileexpression(comp, index->left);
                    if(!ok)
                    {
                        goto error;
                    }
                    ok = mc_compiler_compileexpression(comp, index->index);
                    if(!ok)
                    {
                        goto error;
                    }
                    ip = mc_compiler_emit(comp, MC_OPCODE_SETINDEX, 0, NULL);
                    if(ip < 0)
                    {
                        goto error;
                    }
                }
                if(assign->is_postfix)
                {
                    ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, NULL);
                    if(ip < 0)
                    {
                        goto error;
                    }
                }
                mc_basicarray_pop(comp->srcposstack, NULL);
            }
            break;

        case MC_EXPR_LOGICAL:
            {
                int afterrightip;
                int afterleftjumpip;
                mcastexprlogical_t* logi;
                logi = &expr->uexpr.exprlogical;
                ok = mc_compiler_compileexpression(comp, logi->left);
                if(!ok)
                {
                    goto error;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_DUP, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
                afterleftjumpip = 0;
                if(logi->op == MC_MATHOP_LOGICALAND)
                {
                    opbuf[0] = 0xbeef;
                    afterleftjumpip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFFALSE, 1, opbuf);
                }
                else
                {
                    opbuf[0] = 0xbeef;
                    afterleftjumpip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFTRUE, 1, opbuf);
                }
                if(afterleftjumpip < 0)
                {
                    goto error;
                }
                ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, NULL);
                if(ip < 0)
                {
                    goto error;
                }
                ok = mc_compiler_compileexpression(comp, logi->right);
                if(!ok)
                {
                    goto error;
                }
                afterrightip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, afterleftjumpip + 1, afterrightip);
            }
            break;
        case MC_EXPR_TERNARY:
            {
                int endip;
                int elseip;
                int endjumpip;
                int elsejumpip;
                mcastexprternary_t* ternary;
                ternary = &expr->uexpr.exprternary;
                ok = mc_compiler_compileexpression(comp, ternary->tercond);
                if(!ok)
                {
                    goto error;
                }
                opbuf[0] = 0xbeef;
                elsejumpip = mc_compiler_emit(comp, MC_OPCODE_JUMPIFFALSE, 1, opbuf);
                ok = mc_compiler_compileexpression(comp, ternary->teriftrue);
                if(!ok)
                {
                    goto error;
                }
                opbuf[0] = 0xbeef;
                endjumpip = mc_compiler_emit(comp, MC_OPCODE_JUMP, 1, opbuf);
                elseip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, elsejumpip + 1, elseip);
                ok = mc_compiler_compileexpression(comp, ternary->teriffalse);
                if(!ok)
                {
                    goto error;
                }
                endip = mc_compiler_getip(comp);
                mc_compiler_changeuint16operand(comp, endjumpip + 1, endip);
            }
            break;
        default:
            {
                MC_ASSERT(false);
            }
            break;
    }
    res = true;
    goto end;
error:
    res = false;
end:
    mc_basicarray_pop(comp->srcposstack, NULL);
    mc_astexpr_destroy(exproptimized);
    return res;
}

bool mc_compiler_compilecodeblock(mcastcompiler_t* comp, mcastexprcodeblock_t* block)
{
    bool ok;
    size_t i;
    int ip;
    mcastsymtable_t* symtab;
    mcastexpression_t* expr;
    symtab = mc_compiler_getsymtable(comp);
    if(!symtab)
    {
        return false;
    }
    ok = mc_symtable_pushblockscope(symtab);
    if(!ok)
    {
        return false;
    }
    if(mc_ptrlist_count(block->statements) == 0)
    {
        ip = mc_compiler_emit(comp, MC_OPCODE_NULL, 0, NULL);
        if(ip < 0)
        {
            return false;
        }
        ip = mc_compiler_emit(comp, MC_OPCODE_POP, 0, NULL);
        if(ip < 0)
        {
            return false;
        }
    }
    for(i = 0; i < mc_ptrlist_count(block->statements); i++)
    {
        expr = (mcastexpression_t*)mc_ptrlist_get(block->statements, i);
        ok = mc_compiler_compilestatement(comp, expr);
        if(!ok)
        {
            return false;
        }
    }
    mc_symtable_popblockscope(symtab);
    return true;
}

int mc_compiler_addconstant(mcastcompiler_t* comp, mcvalue_t obj)
{
    bool ok;
    int pos;
    ok = mc_vallist_push(comp->constants, obj);
    if(!ok)
    {
        return -1;
    }
    pos = mc_vallist_count(comp->constants) - 1;
    return pos;
}

void mc_compiler_changeuint16operand(mcastcompiler_t* comp, int ip, uint16_t operand)
{
    uint16_t hi;
    uint16_t lo;
    mcbasicarray_t* bytecode;
    bytecode = mc_compiler_getbytecode(comp);
    if((ip + 1) >= (int)mc_basicarray_count(bytecode))
    {
        MC_ASSERT(false);
        return;
    }
    hi = operand >> 8;
    mc_basicarray_set(bytecode, ip, &hi);
    lo = operand;
    mc_basicarray_set(bytecode, ip + 1, &lo);
}

bool mc_compiler_lastopcodeis(mcastcompiler_t* comp, mcinternopcode_t op)
{
    mcinternopcode_t lastopcode;
    lastopcode = mc_compiler_getlastopcode(comp);
    return lastopcode == op;
}

bool mc_compiler_readsymbol(mcastcompiler_t* comp, mcastsymbol_t* symbol)
{
    int ip;
    uint64_t opbuf[10];
    ip = -1;
    if(symbol->symtype == MC_SYM_MODULEGLOBAL)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_GETMODULEGLOBAL, 1, opbuf);
    }
    else if(symbol->symtype == MC_SYM_GLOBALBUILTIN)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_GETGLOBALBUILTIN, 1, opbuf);
    }
    else if(symbol->symtype == MC_SYM_LOCAL)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_GETLOCAL, 1, opbuf);
    }
    else if(symbol->symtype == MC_SYM_FREE)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_GETFREE, 1, opbuf);
    }
    else if(symbol->symtype == MC_SYM_FUNCTION)
    {
        ip = mc_compiler_emit(comp, MC_OPCODE_CURRENTFUNCTION, 0, NULL);
    }
    else if(symbol->symtype == MC_SYM_THIS)
    {
        ip = mc_compiler_emit(comp, MC_OPCODE_GETTHIS, 0, NULL);
    }
    return ip >= 0;
}

bool mc_compiler_storesymbol(mcastcompiler_t* comp, mcastsymbol_t* symbol, bool define)
{
    int ip;
    uint64_t opbuf[10];
    ip = -1;
    if(symbol->symtype == MC_SYM_MODULEGLOBAL)
    {
        if(define)
        {
            opbuf[0] = symbol->index;
            ip = mc_compiler_emit(comp, MC_OPCODE_DEFINEMODULEGLOBAL, 1, opbuf);
        }
        else
        {
            opbuf[0] = symbol->index;
            ip = mc_compiler_emit(comp, MC_OPCODE_SETMODULEGLOBAL, 1, opbuf);
        }
    }
    else if(symbol->symtype == MC_SYM_LOCAL)
    {
        if(define)
        {
            opbuf[0] = symbol->index;
            ip = mc_compiler_emit(comp, MC_OPCODE_DEFINELOCAL, 1, opbuf);
        }
        else
        {
            opbuf[0] = symbol->index;
            ip = mc_compiler_emit(comp, MC_OPCODE_SETLOCAL, 1, opbuf);
        }
    }
    else if(symbol->symtype == MC_SYM_FREE)
    {
        opbuf[0] = symbol->index;
        ip = mc_compiler_emit(comp, MC_OPCODE_SETFREE, 1, opbuf);
    }
    return ip >= 0;
}

bool mc_compiler_pushbreakip(mcastcompiler_t* comp, int ip)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return mc_basicarray_push(compscope->ipstackbreak, &ip);
}

void mc_compiler_popbreakip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    if(mc_basicarray_count(compscope->ipstackbreak) == 0)
    {
        MC_ASSERT(false);
        return;
    }
    mc_basicarray_pop(compscope->ipstackbreak, NULL);
}

int mc_compiler_getbreakip(mcastcompiler_t* comp)
{
    int* res;
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    if(mc_basicarray_count(compscope->ipstackbreak) == 0)
    {
        return -1;
    }
    res = (int*)mc_basicarray_top(compscope->ipstackbreak);
    return *res;
}

bool mc_compiler_pushcontinueip(mcastcompiler_t* comp, int ip)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return mc_basicarray_push(compscope->ipstackcontinue, &ip);
}

void mc_compiler_popcontinueip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    if(mc_basicarray_count(compscope->ipstackcontinue) == 0)
    {
        MC_ASSERT(false);
        return;
    }
    mc_basicarray_pop(compscope->ipstackcontinue, NULL);
}

int mc_compiler_getcontinueip(mcastcompiler_t* comp)
{
    int* res;
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    if(mc_basicarray_count(compscope->ipstackcontinue) == 0)
    {
        MC_ASSERT(false);
        return -1;
    }
    res = (int*)mc_basicarray_top(compscope->ipstackcontinue);
    return *res;
}

int mc_compiler_getip(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return mc_basicarray_count(compscope->compiledscopebytecode);
}

mcbasicarray_t* mc_compiler_getsrcpositions(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return compscope->scopesrcposlist;
}

mcbasicarray_t* mc_compiler_getbytecode(mcastcompiler_t* comp)
{
    mcastscopecomp_t* compscope;
    compscope = mc_compiler_getcompilationscope(comp);
    return compscope->compiledscopebytecode;
}


mcastscopefile_t* mc_compiler_filescopemake(mcastcompiler_t* comp, mcastcompiledfile_t* file)
{
    mcastscopefile_t* filescope;
    filescope = (mcastscopefile_t*)mc_allocator_malloc(comp->pstate, sizeof(mcastscopefile_t));
    if(!filescope)
    {
        return NULL;
    }
    memset(filescope, 0, sizeof(mcastscopefile_t));
    filescope->pstate = comp->pstate;
    filescope->parser = mc_astparser_make(comp->pstate, comp->config, comp->errors);
    if(!filescope->parser)
    {
        goto err;
    }
    filescope->filesymtab = NULL;
    filescope->file = file;
    filescope->loadedmodnames = mc_ptrlist_make(comp->pstate, 0, sizeof(void*));
    if(!filescope->loadedmodnames)
    {
        goto err;
    }
    return filescope;
err:
    mc_compiler_filescopedestroy(filescope);
    return NULL;
}

void mc_compiler_filescopedestroy(mcastscopefile_t* scope)
{
    size_t i;
    void* name;
    for(i = 0; i < mc_ptrlist_count(scope->loadedmodnames); i++)
    {
        name = (void*)mc_ptrlist_get(scope->loadedmodnames, i);
        mc_memory_free(name);
    }
    mc_ptrlist_destroy(scope->loadedmodnames, NULL);
    mc_astparser_destroy(scope->parser);
    mc_memory_free(scope);
}

bool mc_compiler_filescopepush(mcastcompiler_t* comp, const char* filepath)
{
    bool ok;
    int globaloffset;
    mcastscopeblock_t* prevsttopscope;
    mcastsymtable_t* prevst;
    mcastcompiledfile_t* file;
    mcastscopefile_t* filescope;
    prevst = NULL;
    if(mc_ptrlist_count(comp->filescopelist) > 0)
    {
        prevst = mc_compiler_getsymtable(comp);
    }
    file = mc_compiledfile_make(comp->pstate, filepath);
    if(!file)
    {
        return false;
    }
    ok = mc_ptrlist_push(comp->files, file);
    if(!ok)
    {
        mc_compiledfile_destroy(file);
        return false;
    }
    filescope = mc_compiler_filescopemake(comp, file);
    if(!filescope)
    {
        return false;
    }
    ok = mc_ptrlist_push(comp->filescopelist, filescope);
    if(!ok)
    {
        mc_compiler_filescopedestroy(filescope);
        return false;
    }
    globaloffset = 0;
    if(prevst)
    {
        prevsttopscope = mc_symtable_getblockscope(prevst);
        globaloffset = prevsttopscope->offset + prevsttopscope->numdefinitions;
    }
    ok = mc_compiler_pushsymtable(comp, globaloffset);
    if(!ok)
    {
        mc_ptrlist_pop(comp->filescopelist, NULL);
        mc_compiler_filescopedestroy(filescope);
        return false;
    }
    return true;
}

void mc_compiler_filescopepop(mcastcompiler_t* comp)
{
    int poppednumdefs;
    mcastsymtable_t* poppedst;
    mcastsymtable_t* currentst;
    mcastscopefile_t* scope;
    mcastscopeblock_t* currentsttopscope;
    mcastscopeblock_t* poppedsttopscope;
    poppedst = mc_compiler_getsymtable(comp);
    poppedsttopscope = mc_symtable_getblockscope(poppedst);
    poppednumdefs = poppedsttopscope->numdefinitions;
    while(mc_compiler_getsymtable(comp))
    {
        mc_compiler_popsymtable(comp);
    }
    scope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    if(!scope)
    {
        MC_ASSERT(false);
        return;
    }
    mc_compiler_filescopedestroy(scope);
    mc_ptrlist_pop(comp->filescopelist, NULL);
    if(mc_ptrlist_count(comp->filescopelist) > 0)
    {
        currentst = mc_compiler_getsymtable(comp);
        currentsttopscope = mc_symtable_getblockscope(currentst);
        currentsttopscope->numdefinitions += poppednumdefs;
    }
}

void mc_compiler_setcompilationscope(mcastcompiler_t* comp, mcastscopecomp_t* scope)
{
    comp->compilationscope = scope;
}

mcastcompiledfile_t* mc_compiledfile_make(mcstate_t* state, const char* path)
{
    size_t len;
    const char* lastslashpos;
    mcastcompiledfile_t* file;
    file = (mcastcompiledfile_t*)mc_allocator_malloc(state, sizeof(mcastcompiledfile_t));
    if(!file)
    {
        return NULL;
    }
    memset(file, 0, sizeof(mcastcompiledfile_t));
    file->pstate = state;
    lastslashpos = strrchr(path, '/');
    if(lastslashpos)
    {
        len = lastslashpos - path + 1;
        file->dir_path = mc_util_strndup(state, path, len);
    }
    else
    {
        file->dir_path = mc_util_strdup(state, "");
    }
    if(!file->dir_path)
    {
        goto error;
    }
    file->path = mc_util_strdup(state, path);
    if(!file->path)
    {
        goto error;
    }
    file->lines = mc_ptrlist_make(state, 0, sizeof(void*));
    if(!file->lines)
    {
        goto error;
    }
    return file;
error:
    mc_compiledfile_destroy(file);
    return NULL;
}

void mc_compiledfile_destroy(mcastcompiledfile_t* file)
{
    size_t i;
    void* item;
    if(!file)
    {
        return;
    }
    for(i = 0; i < mc_ptrlist_count(file->lines); i++)
    {
        item = (void*)mc_ptrlist_get(file->lines, i);
        mc_memory_free(item);
    }
    mc_ptrlist_destroy(file->lines, NULL);
    mc_memory_free(file->dir_path);
    mc_memory_free(file->path);
    mc_memory_free(file);
}

mcastcompiler_t* mc_compiler_make(mcstate_t* state, mcconfig_t* config, mcgcmemory_t* mem, mcerrlist_t* errors, mcptrlist_t* files, mcglobalstore_t* gstore)
{
    bool ok;
    mcastcompiler_t* comp = (mcastcompiler_t*)mc_allocator_malloc(state, sizeof(mcastcompiler_t));
    if(!comp)
    {
        return NULL;
    }
    ok = mc_compiler_init(comp, state, config, mem, errors, files, gstore);
    if(!ok)
    {
        mc_memory_free(comp);
        return NULL;
    }
    comp->pstate = state; 
    return comp;
}

void mc_compiler_destroy(mcastcompiler_t* comp)
{
    if(!comp)
    {
        return;
    }
    mc_compiler_deinit(comp);
    mc_memory_free(comp);
}

mccompiledprogram_t* mc_compiler_compilesource(mcastcompiler_t* comp, const char* code)
{
    bool ok;
    mcastcompiler_t compshallowcopy;
    mcastscopecomp_t* compscope;
    mccompiledprogram_t* res;
    compscope = mc_compiler_getcompilationscope(comp);
    MC_ASSERT(mc_basicarray_count(comp->srcposstack) == 0);
    MC_ASSERT(mc_basicarray_count(compscope->compiledscopebytecode) == 0);
    MC_ASSERT(mc_basicarray_count(compscope->ipstackbreak) == 0);
    MC_ASSERT(mc_basicarray_count(compscope->ipstackcontinue) == 0);
    mc_basicarray_clear(comp->srcposstack);
    mc_basicarray_clear(compscope->compiledscopebytecode);
    mc_basicarray_clear(compscope->scopesrcposlist);
    mc_basicarray_clear(compscope->ipstackbreak);
    mc_basicarray_clear(compscope->ipstackcontinue);
    ok = mc_compiler_initshallowcopy(&compshallowcopy, comp);
    if(!ok)
    {
        return NULL;
    }
    ok = mc_compiler_docompilesource(comp, code);
    if(!ok)
    {
        goto compilefailed;
    }
    mc_compiler_emit(comp, MC_OPCODE_HALT, 0, 0);
    /* might've changed */
    compscope = mc_compiler_getcompilationscope(comp);
    MC_ASSERT(compscope->outer == NULL);
    compscope = mc_compiler_getcompilationscope(comp);
    res = mc_astcompscope_orphanresult(compscope);
    if(!res)
    {
        goto compilefailed;
    }
    mc_compiler_deinit(&compshallowcopy);
    return res;
compilefailed:
    mc_compiler_deinit(comp);
    *comp = compshallowcopy;
    return NULL;
}

mcastsymtable_t* mc_compiler_getsymtable(mcastcompiler_t* comp)
{
    mcastscopefile_t* filescope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    if(!filescope)
    {
        MC_ASSERT(false);
        return NULL;
    }
    return filescope->filesymtab;
}

void mc_compiler_setsymtable(mcastcompiler_t* comp, mcastsymtable_t* table)
{
    mcastscopefile_t* filescope = (mcastscopefile_t*)mc_ptrlist_top(comp->filescopelist);
    if(!filescope)
    {
        MC_ASSERT(false);
        return;
    }
    filescope->filesymtab = table;
}

mcvallist_t* mc_compiler_getconstants(mcastcompiler_t* comp)
{
    return comp->constants;
}






