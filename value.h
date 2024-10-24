

const char* mc_valtype_getname(mcvaltype_t type)
{
    switch(type)
    {
        case MC_VAL_NONE:
        case MC_VAL_FREED:
        case MC_VAL_NULL:
            return "null";
        case MC_VAL_NUMBER:
            return "number";
        case MC_VAL_BOOL:
            return "boolean";
        case MC_VAL_STRING:
            return "string";
        case MC_VAL_FUNCNATIVE:
        case MC_VAL_FUNCSCRIPT:
            return "function";
        case MC_VAL_ARRAY:
            return "array";
        case MC_VAL_MAP:
            return "object";
        case MC_VAL_EXTERNAL:
            return "extern";
        case MC_VAL_ERROR:
            return "error";
        case MC_VAL_ANY:
            return "any";
    }
    return "invalid";
}


const char* mc_util_objtypename(mcvaltype_t type)
{
    switch(type)
    {
        case MC_VAL_NONE:
            return "NONE";
        case MC_VAL_ERROR:
            return "ERROR";
        case MC_VAL_NUMBER:
            return "NUMBER";
        case MC_VAL_BOOL:
            return "BOOL";
        case MC_VAL_STRING:
            return "STRING";
        case MC_VAL_NULL:
            return "NULL";
        case MC_VAL_FUNCNATIVE:
            return "NATIVE_FUNCTION";
        case MC_VAL_ARRAY:
            return "ARRAY";
        case MC_VAL_MAP:
            return "MAP";
        case MC_VAL_FUNCSCRIPT:
            return "FUNCTION";
        case MC_VAL_EXTERNAL:
            return "EXTERNAL";
        case MC_VAL_FREED:
            return "FREED";
        case MC_VAL_ANY:
            return "ANY";
        default:
            break;
    }
    return "NONE";
}

MC_FORCEINLINE mcvalue_t mc_object_makedatafrom(mcvaltype_t type, mcobjdata_t* data)
{
    mcvalue_t object;
    memset(&object, 0, sizeof(mcvalue_t));
    object.valtype = type;
    data->odtype = type;
    object.isallocated = true;
    object.uval.odata = data;
    return object;
}

MC_FORCEINLINE mcvalue_t mc_value_makeempty(mcvaltype_t t)
{
    mcvalue_t o = {};
    o.valtype = t;
    o.isallocated = false;
    return o;
}

MC_FORCEINLINE mcvalue_t mc_value_makenumber(mcfloat_t val)
{
    mcvalue_t o;
    o = mc_value_makeempty(MC_VAL_NUMBER);
    o.uval.valnumber = val;
    return o;
}

MC_FORCEINLINE mcvalue_t mc_value_makebool(bool val)
{
    mcvalue_t o;
    o = mc_value_makeempty(MC_VAL_BOOL);
    o.uval.valbool = val;
    return o;
}

MC_FORCEINLINE mcvalue_t mc_value_makenull()
{
    mcvalue_t o;
    o = mc_value_makeempty(MC_VAL_NULL);
    return o;
}


MC_FORCEINLINE bool mc_value_isallocated(mcvalue_t object)
{
    return object.isallocated;
}

MC_FORCEINLINE bool mc_value_isnumeric(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return type == MC_VAL_NUMBER || type == MC_VAL_BOOL;
}

MC_FORCEINLINE bool mc_value_isnumber(mcvalue_t o)
{
    return (mc_value_gettype(o) == MC_VAL_NUMBER || mc_value_gettype(o) == MC_VAL_BOOL);
}

MC_FORCEINLINE bool mc_value_isnull(mcvalue_t obj)
{
    return mc_value_gettype(obj) == MC_VAL_NULL;
}


MC_FORCEINLINE bool mc_value_isfuncnative(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return (type == MC_VAL_FUNCNATIVE);
}

MC_FORCEINLINE bool mc_value_isfuncscript(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return (type == MC_VAL_FUNCSCRIPT);
}

MC_FORCEINLINE bool mc_value_iscallable(mcvalue_t obj)
{
    return (mc_value_isfuncnative(obj) || mc_value_isfuncscript(obj));
}

MC_FORCEINLINE bool mc_value_isstring(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return (type == MC_VAL_STRING);
}


MC_FORCEINLINE bool mc_value_ismap(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return (type == MC_VAL_MAP);
}


MC_FORCEINLINE bool mc_value_isarray(mcvalue_t obj)
{
    mcvaltype_t type;
    type = mc_value_gettype(obj);
    return (type == MC_VAL_ARRAY);
}


MC_FORCEINLINE bool mc_value_ishashable(mcvalue_t obj)
{
    mcvaltype_t type = mc_value_gettype(obj);
    switch(type)
    {
        case MC_VAL_STRING:
        case MC_VAL_NUMBER:
        case MC_VAL_BOOL:
            return true;
        default:
            break;
    }
    return false;
}


mcvalue_t mc_value_makestrcapacity(mcstate_t* state, int capacity)
{
    mcobjdata_t* data;
    data = mc_gcmemory_getdatafrompool(state, MC_VAL_STRING);
    if(!data)
    {
        data = mc_gcmemory_allocobjectdata(state);
        if(!data)
        {
            return mc_value_makenull();
        }
    }
    data->uvobj.valstring.hash = 0;
    data->uvobj.valstring.strbuf = dyn_strbuf_makeempty(capacity);
    return mc_object_makedatafrom(MC_VAL_STRING, data);
}

mcvalue_t mc_value_makestrformat(mcstate_t* state, const char* fmt, ...)
{
    va_list args;
    mcvalue_t res;
    mcobjdata_t* data;
    data = mc_gcmemory_getdatafrompool(state, MC_VAL_STRING);
    res = mc_value_makestrcapacity(state, 0);
    if(mc_value_isnull(res))
    {
        return mc_value_makenull();
    }
    va_start(args, fmt);
    dyn_strbuf_appendformatv(data->uvobj.valstring.strbuf, fmt, args);
    va_end(args);
    return res;
}

mcvalue_t mc_value_makestringlen(mcstate_t* state, const char* string, size_t len)
{
    bool ok;
    mcvalue_t res;
    res = mc_value_makestrcapacity(state, len);
    if(mc_value_isnull(res))
    {
        return res;
    }
    ok = mc_value_stringappendlen(res, string, len);
    if(!ok)
    {
        return mc_value_makenull();
    }
    return res;
}

mcvalue_t mc_value_makestring(mcstate_t* state, const char* string)
{
    return mc_value_makestringlen(state, string, mc_util_strlen(string));
}

mcvalue_t mc_value_makefuncnative(mcstate_t* state, const char* name, mcnativefn_t fn, void* data)
{
    mcobjdata_t* obj;
    obj = mc_gcmemory_allocobjectdata(state);
    if(!obj)
    {
        return mc_value_makenull();
    }
    obj->uvobj.valnativefunc.name = mc_util_strdup(state, name);
    if(!obj->uvobj.valnativefunc.name)
    {
        return mc_value_makenull();
    }
    obj->uvobj.valnativefunc.natptrfn = fn;
    if(data)
    {
        obj->uvobj.valnativefunc.userpointer = data;
    }
    return mc_object_makedatafrom(MC_VAL_FUNCNATIVE, obj);
}

mcvalue_t mc_value_makearray(mcstate_t* state)
{
    return mc_value_makearraycapacity(state, 8);
}

mcvalue_t mc_value_makearraycapacity(mcstate_t* state, size_t capacity)
{
    mcobjdata_t* data;
    data = mc_gcmemory_getdatafrompool(state, MC_VAL_ARRAY);
    if(data)
    {
        mc_vallist_setempty(data->uvobj.valarray->actualarray);
        return mc_object_makedatafrom(MC_VAL_ARRAY, data);
    }
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->uvobj.valarray = (mcobjarray_t*)mc_memory_malloc(sizeof(mcobjarray_t));
    data->uvobj.valarray->actualarray = mc_vallist_make(state, NULL, capacity);
    if(!data->uvobj.valarray->actualarray)
    {
        return mc_value_makenull();
    }
    return mc_object_makedatafrom(MC_VAL_ARRAY, data);
}

mcvalue_t mc_value_makemap(mcstate_t* state)
{
    return mc_value_makemapcapacity(state, 32);
}

mcvalue_t mc_value_makemapcapacity(mcstate_t* state, size_t capacity)
{
    mcobjdata_t* data;
    data = mc_gcmemory_getdatafrompool(state, MC_VAL_MAP);
    if(data)
    {
        mc_valdict_clear(data->uvobj.valmap->actualmap);
        return mc_object_makedatafrom(MC_VAL_MAP, data);
    }
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->uvobj.valmap = (mcobjmap_t*)mc_memory_malloc(sizeof(mcobjmap_t));
    data->uvobj.valmap->actualmap = mc_valdict_makecapacity(state, capacity, sizeof(mcvalue_t), sizeof(mcvalue_t));
    if(!data->uvobj.valmap->actualmap)
    {
        return mc_value_makenull();
    }
    mc_valdict_sethashfunction(data->uvobj.valmap->actualmap, (mcitemhashfn_t)mc_value_callbackhash);
    mc_valdict_setequalsfunction(data->uvobj.valmap->actualmap, (mcitemcomparefn_t)mc_value_callbackequals);
    return mc_object_makedatafrom(MC_VAL_MAP, data);
}

mcvalue_t mc_value_makeerror(mcstate_t* state, const char* error)
{
    char* errorstr;
    mcvalue_t res;
    errorstr = mc_util_strdup(state, error);
    if(!errorstr)
    {
        return mc_value_makenull();
    }
    res = mc_value_makeerrornocopy(state, errorstr);
    if(mc_value_isnull(res))
    {
        mc_memory_free(errorstr);
        return mc_value_makenull();
    }
    return res;
}

mcvalue_t mc_value_makeerrornocopy(mcstate_t* state, char* error)
{
    mcobjdata_t* data;
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return mc_value_makenull();
    }
    data->uvobj.valerror.message = error;
    data->uvobj.valerror.traceback = NULL;
    return mc_object_makedatafrom(MC_VAL_ERROR, data);
}

mcvalue_t mc_value_makeerrorf(mcstate_t* state, const char* fmt, ...)
{
    int needsz;
    int printedsz;
    char* res;
    va_list args;
    mcvalue_t resobj;
    (void)printedsz;
    va_start(args, fmt);
    needsz = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    va_start(args, fmt);
    res = (char*)mc_memory_malloc(needsz + 1);
    if(!res)
    {
        return mc_value_makenull();
    }
    printedsz = vsprintf(res, fmt, args);
    MC_ASSERT(printedsz == needsz);
    va_end(args);
    resobj = mc_value_makeerrornocopy(state, res);
    if(mc_value_isnull(resobj))
    {
        mc_memory_free(res);
        return mc_value_makenull();
    }
    return resobj;
}

mcvalue_t mc_value_makefuncscript(mcstate_t* state, const char* name, mccompiledprogram_t* cres, bool ownsdt, int nlocals, int nargs, int fvc)
{
    mcobjdata_t* data;
    data = mc_gcmemory_allocobjectdata(state);
    if(!data)
    {
        return mc_value_makenull();
    }
    if(ownsdt)
    {
        data->uvobj.valscriptfunc.unamev.fallocname = name ? mc_util_strdup(state, name) : mc_util_strdup(state, "anonymous");
        if(!data->uvobj.valscriptfunc.unamev.fallocname)
        {
            return mc_value_makenull();
        }
    }
    else
    {
        data->uvobj.valscriptfunc.unamev.fconstname = name ? name : "anonymous";
    }
    data->uvobj.valscriptfunc.compiledprogcode = cres;
    data->uvobj.valscriptfunc.ownsdata = ownsdt;
    data->uvobj.valscriptfunc.numlocals = nlocals;
    data->uvobj.valscriptfunc.numargs = nargs;
    if(fvc >= MC_UTIL_STATICARRAYSIZE(data->uvobj.valscriptfunc.ufv.freevalsstack))
    {
        data->uvobj.valscriptfunc.ufv.freevalsallocated = (mcvalue_t*)mc_memory_malloc(sizeof(mcvalue_t) * fvc);
        if(!data->uvobj.valscriptfunc.ufv.freevalsallocated)
        {
            return mc_value_makenull();
        }
    }
    data->uvobj.valscriptfunc.freevalscount = fvc;
    return mc_object_makedatafrom(MC_VAL_FUNCSCRIPT, data);
}

mcvalue_t mc_value_makeuserobject(mcstate_t* state, void* data)
{
    mcobjdata_t* obj;
    obj = mc_gcmemory_allocobjectdata(state);
    if(!obj)
    {
        return mc_value_makenull();
    }
    obj->uvobj.valuserobject.data = data;
    obj->uvobj.valuserobject.datadestroyfn = NULL;
    obj->uvobj.valuserobject.datacopyfn = NULL;
    return mc_object_makedatafrom(MC_VAL_EXTERNAL, obj);
}

mcvallist_t* mc_value_arraygetactualarray(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valarray->actualarray;
}

bool mc_objfunction_freevalsareallocated(mcobjfuncscript_t* fun)
{
    return fun->freevalscount >= MC_UTIL_STATICARRAYSIZE(fun->ufv.freevalsstack);
}

MC_INLINE char* mc_value_stringgetdataintern(mcvalue_t object)
{
    mcobjdata_t* data;
    data = mc_value_getallocateddata(object);
    MC_ASSERT(data->odtype == MC_VAL_STRING);
    return data->uvobj.valstring.strbuf->data;
}

mcobjuserdata_t* mc_value_userdatagetdata(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_EXTERNAL);
    data = mc_value_getallocateddata(object);
    return &data->uvobj.valuserobject;
}

bool mc_value_userdatasetdata(mcvalue_t object, void* extdata)
{
    mcobjuserdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_EXTERNAL);
    data = mc_value_userdatagetdata(object);
    if(!data)
    {
        return false;
    }
    data->data = extdata;
    return true;
}

bool mc_value_userdatasetdestroyfunction(mcvalue_t object, mcitemdestroyfn_t dfn)
{
    mcobjuserdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_EXTERNAL);
    data = mc_value_userdatagetdata(object);
    if(!data)
    {
        return false;
    }
    data->datadestroyfn = dfn;
    return true;
}

bool mc_value_userdatasetcopyfunction(mcvalue_t object, mcitemcopyfn_t copyfn)
{
    mcobjuserdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_EXTERNAL);
    data = mc_value_userdatagetdata(object);
    if(!data)
    {
        return false;
    }
    data->datacopyfn = copyfn;
    return true;
}

bool mc_value_asbool(mcvalue_t obj)
{
    if(mc_value_isnumber(obj))
    {
        return obj.uval.valnumber;
    }
    return obj.uval.valbool;
}

mcfloat_t mc_value_asnumber(mcvalue_t obj)
{
    if(mc_value_isnumber(obj))
    {
        if(mc_value_gettype(obj) == MC_VAL_BOOL)
        {
            return obj.uval.valbool;
        }
        return obj.uval.valnumber;
    }
    return obj.uval.valnumber;
}

MC_INLINE const char* mc_value_stringgetdata(mcvalue_t object)
{
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    return mc_value_stringgetdataintern(object);
}

int mc_value_stringgetlength(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valstring.strbuf->length;
}

void mc_value_stringsetlength(mcvalue_t object, int len)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    data = mc_value_getallocateddata(object);
    data->uvobj.valstring.strbuf->length = len;
    mc_value_stringrehash(object);
}

bool mc_value_stringreset(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    data = mc_value_getallocateddata(object);
    dyn_strbuf_reset(data->uvobj.valstring.strbuf);
    return true;
}

MC_INLINE char* mc_value_stringgetmutabledata(mcvalue_t object)
{
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_STRING);
    return mc_value_stringgetdataintern(object);
}

bool mc_value_stringappendlen(mcvalue_t obj, const char* src, size_t len)
{
    mcobjdata_t* data;
    mcobjstring_t* string;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_STRING);
    data = mc_value_getallocateddata(obj);
    string = &data->uvobj.valstring;
    dyn_strbuf_appendstrn(string->strbuf, src, len);
    mc_value_stringrehash(obj);
    return true;
}

bool mc_value_stringappend(mcvalue_t obj, const char* src)
{
    return mc_value_stringappendlen(obj, src, strlen(src));
}

bool mc_value_stringappendformatv(mcvalue_t obj, const char* fmt, va_list va)
{
    mcobjdata_t* data;
    mcobjstring_t* string;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_STRING);
    data = mc_value_getallocateddata(obj);
    string = &data->uvobj.valstring;
    dyn_strbuf_appendformatv(string->strbuf, fmt, va);
    mc_value_stringrehash(obj);
    return true;
}

bool mc_value_stringappendformat(mcvalue_t obj, const char* fmt, ...)
{
    bool r;
    va_list va;
    va_start(va, fmt);
    r = mc_value_stringappendformatv(obj, fmt, va);
    va_end(va);
    return r;
}

bool mc_value_stringappendvalue(mcvalue_t destval, mcvalue_t val)
{
    bool ok;
    int vlen;
    const char* vstr;
    if(mc_value_gettype(val) == MC_VAL_NUMBER)
    {
        mc_value_stringappendformat(destval, "%g", mc_value_asnumber(val));
        goto finished;
    }
    if(mc_value_gettype(val) == MC_VAL_STRING)
    {
        vlen = mc_value_stringgetlength(val);
        vstr = mc_value_stringgetdata(val);
        ok = mc_value_stringappendlen(destval, vstr, vlen);
        if(!ok)
        {
            return false;
        }
        goto finished;
    }
    return false;
    finished:
    mc_value_stringrehash(destval);
    return true;
}

size_t mc_value_stringgethash(mcvalue_t obj)
{
    size_t len;
    const char* str;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_STRING);
    data = mc_value_getallocateddata(obj);
    if(data->uvobj.valstring.hash == 0)
    {
        len = mc_value_stringgetlength(obj);
        str = mc_value_stringgetdata(obj);
        data->uvobj.valstring.hash = mc_util_hashdata(str, len);
    }
    return data->uvobj.valstring.hash;
}

bool mc_value_stringrehash(mcvalue_t obj)
{
    size_t len;
    const char* str;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_STRING);
    data = mc_value_getallocateddata(obj);
    len = mc_value_stringgetlength(obj);
    str = mc_value_stringgetdata(obj);
    data->uvobj.valstring.hash = mc_util_hashdata(str, len);
    return true;
}

mcobjfuncscript_t* mc_value_asscriptfunction(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(object);
    return &data->uvobj.valscriptfunc;
}

MC_INLINE mcobjfuncnative_t* mc_value_asnativefunction(mcvalue_t obj)
{
    mcobjdata_t* data = mc_value_getallocateddata(obj);
    return &data->uvobj.valnativefunc;
}

const char* mc_value_functiongetname(mcvalue_t obj)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return NULL;
    }
    if(data->uvobj.valscriptfunc.ownsdata)
    {
        return data->uvobj.valscriptfunc.unamev.fallocname;
    }
    return data->uvobj.valscriptfunc.unamev.fconstname;
}

mcvalue_t mc_value_functiongetfreevalat(mcvalue_t obj, int ix)
{
    mcobjdata_t* data;
    mcobjfuncscript_t* fun;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return mc_value_makenull();
    }
    fun = &data->uvobj.valscriptfunc;
    MC_ASSERT(ix >= 0 && ix < fun->freevalscount);
    if(ix < 0 || ix >= fun->freevalscount)
    {
        return mc_value_makenull();
    }
    if(mc_objfunction_freevalsareallocated(fun))
    {
        return fun->ufv.freevalsallocated[ix];
    }
    return fun->ufv.freevalsstack[ix];
}

void mc_value_functionsetfreevalat(mcvalue_t obj, int ix, mcvalue_t val)
{
    mcobjdata_t* data;
    mcobjfuncscript_t* fun;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return;
    }
    fun = &data->uvobj.valscriptfunc;
    MC_ASSERT(ix >= 0 && ix < fun->freevalscount);
    if(ix < 0 || ix >= fun->freevalscount)
    {
        return;
    }
    if(mc_objfunction_freevalsareallocated(fun))
    {
        fun->ufv.freevalsallocated[ix] = val;
    }
    else
    {
        fun->ufv.freevalsstack[ix] = val;
    }
}

mcvalue_t* mc_value_functiongetfreevals(mcvalue_t obj)
{
    mcobjdata_t* data;
    mcobjfuncscript_t* fun;
    MC_ASSERT(mc_value_gettype(obj) == MC_VAL_FUNCSCRIPT);
    data = mc_value_getallocateddata(obj);
    MC_ASSERT(data);
    if(!data)
    {
        return NULL;
    }
    fun = &data->uvobj.valscriptfunc;
    if(mc_objfunction_freevalsareallocated(fun))
    {
        return fun->ufv.freevalsallocated;
    }
    return fun->ufv.freevalsstack;
}

const char* mc_value_errorgetmessage(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ERROR);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valerror.message;
}

void mc_value_errorsettraceback(mcvalue_t object, mctraceback_t* traceback)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ERROR);
    if(mc_value_gettype(object) != MC_VAL_ERROR)
    {
        return;
    }
    data = mc_value_getallocateddata(object);
    MC_ASSERT(data->uvobj.valerror.traceback == NULL);
    data->uvobj.valerror.traceback = traceback;
}

mctraceback_t* mc_value_errorgettraceback(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ERROR);
    data = mc_value_getallocateddata(object);
    return data->uvobj.valerror.traceback;
}


mcvalue_t mc_value_arraygetvalue(mcvalue_t object, size_t ix)
{
    mcvalue_t* res;
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    if(ix >= mc_vallist_count(array))
    {
        return mc_value_makenull();
    }
    res = (mcvalue_t*)mc_vallist_getp(array, ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

bool mc_value_arraysetvalue(mcvalue_t object, size_t ix, mcvalue_t val)
{
    size_t len;
    size_t toadd;
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    len = mc_vallist_count(array);
    if((ix >= len) || (len == 0))
    {
        toadd = len+1;
        #if 0
            fprintf(stderr, "ix=%d toadd=%d len=%d\n", ix, toadd, len);
        #endif
        while(toadd != (ix+2))
        {
            mc_value_arraypush(object, mc_value_makenull());
            toadd++;
        }
    }
    return mc_vallist_set(array, ix, val);
}

bool mc_value_arraypush(mcvalue_t object, mcvalue_t val)
{
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    return mc_vallist_push(array, val);
}

int mc_value_arraygetlength(mcvalue_t object)
{
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    return mc_vallist_count(array);
}

mcvalue_t mc_valarray_pop(mcvalue_t object)
{
    mcvalue_t dest;
    mcvallist_t* array;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_ARRAY);
    array = mc_value_arraygetactualarray(object);
    if(mc_vallist_pop(array, &dest))
    {
        return dest;
    }
    return mc_value_makenull();
}

bool mc_value_arrayremoveat(mcvalue_t object, int ix)
{
    mcvallist_t* array;
    array = mc_value_arraygetactualarray(object);
    return mc_vallist_removeat(array, ix);
}

int mc_value_mapgetlength(mcvalue_t object)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    return mc_valdict_count(data->uvobj.valmap->actualmap);
}

mcvalue_t mc_value_mapgetkeyat(mcvalue_t object, int ix)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)mc_valdict_getkeyat(data->uvobj.valmap->actualmap, ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

mcvalue_t mc_value_mapgetvalueat(mcvalue_t object, int ix)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)mc_valdict_getvalueat(data->uvobj.valmap->actualmap, ix);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

bool mc_value_mapsetvalueat(mcvalue_t object, int ix, mcvalue_t val)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    if(ix >= mc_value_mapgetlength(object))
    {
        return false;
    }
    data = mc_value_getallocateddata(object);
    return mc_valdict_setvalueat(data->uvobj.valmap->actualmap, ix, &val);
}

mcvalue_t mc_value_mapgetkvpairat(mcstate_t* state, mcvalue_t object, int ix)
{
    mcvalue_t key;
    mcvalue_t val;
    mcvalue_t res;
    mcvalue_t valobj;
    mcvalue_t keyobj;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    if(ix >= mc_valdict_count(data->uvobj.valmap->actualmap))
    {
        return mc_value_makenull();
    }
    key = mc_value_mapgetkeyat(object, ix);
    val = mc_value_mapgetvalueat(object, ix);
    res = mc_value_makemap(state);
    if(mc_value_isnull(res))
    {
        return mc_value_makenull();
    }
    keyobj = mc_value_makestring(state, "key");
    if(mc_value_isnull(keyobj))
    {
        return mc_value_makenull();
    }
    mc_value_mapsetvalue(res, keyobj, key);
    valobj = mc_value_makestring(state, "value");
    if(mc_value_isnull(valobj))
    {
        return mc_value_makenull();
    }
    mc_value_mapsetvalue(res, valobj, val);
    return res;
}

bool mc_value_mapsetvalue(mcvalue_t object, mcvalue_t key, mcvalue_t val)
{
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    return mc_valdict_setkv(data->uvobj.valmap->actualmap, &key, &val);
}

bool mc_value_mapsetvaluestring(mcvalue_t object, const char* strkey, mcvalue_t val)
{
    mcstate_t* state;
    mcvalue_t vkey;
    state = mc_value_getallocateddata(object)->pstate;
    vkey = mc_value_makestring(state, strkey);
    return mc_value_mapsetvalue(object, vkey, val);
}

mcvalue_t mc_value_mapgetvalue(mcvalue_t object, mcvalue_t key)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)mc_valdict_get(data->uvobj.valmap->actualmap, &key);
    if(!res)
    {
        return mc_value_makenull();
    }
    return *res;
}

bool mc_value_mapgetvaluechecked(mcvalue_t object, mcvalue_t key, mcvalue_t* dest)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)mc_valdict_get(data->uvobj.valmap->actualmap, &key);
    if(!res)
    {
        *dest = mc_value_makenull();
        return false;
    }
    *dest = *res;
    return true;
}

bool mc_value_maphaskey(mcvalue_t object, mcvalue_t key)
{
    mcvalue_t* res;
    mcobjdata_t* data;
    MC_ASSERT(mc_value_gettype(object) == MC_VAL_MAP);
    data = mc_value_getallocateddata(object);
    res = (mcvalue_t*)mc_valdict_get(data->uvobj.valmap->actualmap, &key);
    return res != NULL;
}

void mc_objectdata_deinit(mcobjdata_t* data)
{
    switch(data->odtype)
    {
        case MC_VAL_FREED:
            {
                MC_ASSERT(false);
                return;
            }
            break;
        case MC_VAL_STRING:
            {
                dyn_strbuf_destroy(data->uvobj.valstring.strbuf);
            }
            break;
        case MC_VAL_FUNCSCRIPT:
            {
                if(data->uvobj.valscriptfunc.ownsdata)
                {
                    mc_memory_free(data->uvobj.valscriptfunc.unamev.fallocname);
                    mc_astcompresult_destroy(data->uvobj.valscriptfunc.compiledprogcode);
                }
                if(mc_objfunction_freevalsareallocated(&data->uvobj.valscriptfunc))
                {
                    mc_memory_free(data->uvobj.valscriptfunc.ufv.freevalsallocated);
                }
            }
            break;
        case MC_VAL_ARRAY:
            {
                mc_vallist_destroy(data->uvobj.valarray->actualarray);
                mc_memory_free(data->uvobj.valarray);
            }
            break;
        case MC_VAL_MAP:
            {
                mc_valdict_destroy(data->uvobj.valmap->actualmap);
                mc_memory_free(data->uvobj.valmap);
            }
            break;
        case MC_VAL_FUNCNATIVE:
            {
                mc_memory_free(data->uvobj.valnativefunc.name);
            }
            break;
        case MC_VAL_EXTERNAL:
            {
                if(data->uvobj.valuserobject.datadestroyfn)
                {
                    data->uvobj.valuserobject.datadestroyfn(data->uvobj.valuserobject.data);
                }
            }
            break;
        case MC_VAL_ERROR:
            {
                mc_memory_free(data->uvobj.valerror.message);
                mc_traceback_destroy(data->uvobj.valerror.traceback);
            }
            break;
        default:
            {
            }
            break;
    }
    data->odtype = MC_VAL_FREED;
}


MC_FORCEINLINE bool mc_value_compare(mcvalue_t a, mcvalue_t b, mcvalcmpresult_t* cres)
{
    const char* astring;
    const char* bstring;
    int alen;
    int blen;
    intptr_t adataval;
    intptr_t bdataval;
    mcfloat_t dnleft;
    mcfloat_t dnright;
    size_t ahash;
    size_t bhash;
    mcvaltype_t atype;
    mcvaltype_t btype;
    /*
    if(a.odata == b.odata)
    {
        return 0;
    }
    */
    cres->result = 1;
    atype = mc_value_gettype(a);
    btype = mc_value_gettype(b);
    if((atype == MC_VAL_NUMBER || atype == MC_VAL_BOOL || atype == MC_VAL_NULL) && (btype == MC_VAL_NUMBER || btype == MC_VAL_BOOL || btype == MC_VAL_NULL))
    {
        dnleft = mc_value_asnumber(a);
        dnright = mc_value_asnumber(b);
        cres->result = (dnleft - dnright);
        return true;
    }
    if(atype == btype && atype == MC_VAL_STRING)
    {
        alen = mc_value_stringgetlength(a);
        blen = mc_value_stringgetlength(b);
        #if 0
        fprintf(stderr, "mc_value_compare: alen=%d, blen=%d\n", alen, blen);
        #endif
        if(alen != blen)
        {
            cres->result = alen - blen;
            return false;
        }
        ahash = mc_value_stringgethash(a);
        bhash = mc_value_stringgethash(b);
        if(ahash != bhash)
        {
            cres->result = ahash - bhash;
            return false;
        }
        astring = mc_value_stringgetdata(a);
        bstring = mc_value_stringgetdata(b);
        if(strncmp(astring, bstring, alen) == 0)
        {
            cres->result = 0;
            return true;
        }
        else
        {
            cres->result = 1;
        }
        return true;
    }
    if((mc_value_isallocated(a) || mc_value_isnull(a)) && (mc_value_isallocated(b) || mc_value_isnull(b)))
    {
        adataval = (intptr_t)mc_value_getallocateddata(a);
        bdataval = (intptr_t)mc_value_getallocateddata(b);
        cres->result = (mcfloat_t)(adataval - bdataval);
        return true;
    }
    return false;
}

MC_FORCEINLINE bool mc_value_equals(mcvalue_t a, mcvalue_t b)
{
    bool ok;
    mcvalcmpresult_t cres;
    mcvaltype_t atype;
    mcvaltype_t btype;
    (void)ok;
    atype = mc_value_gettype(a);
    btype = mc_value_gettype(b);
    if(atype != btype)
    {
        return false;
    }
    ok = mc_value_compare(a, b, &cres);
    return MC_UTIL_CMPFLOAT(cres.result, 0);
}


bool mc_value_callbackequals(mcvalue_t* aptr, mcvalue_t* bptr)
{
    mcvalue_t a;
    mcvalue_t b;
    a = *aptr;
    b = *bptr;
    return mc_value_equals(a, b);
}

size_t mc_value_callbackhash(mcvalue_t* objptr)
{
    bool bval;
    mcfloat_t dval;
    mcvalue_t obj;
    mcvaltype_t type;
    obj = *objptr;
    type = mc_value_gettype(obj);
    switch(type)
    {
        case MC_VAL_NUMBER:
            {
                dval = mc_value_asnumber(obj);
                return mc_util_hashdouble(dval);
            }
            break;
        case MC_VAL_BOOL:
            {
                bval = mc_value_asbool(obj);
                return bval;
            }
            break;
        case MC_VAL_STRING:
            {
                return mc_value_stringgethash(obj);
            }
            break;
        default:
            {
            }
            break;
    }
    return 0;
}


mcvalue_t mc_value_copydeepfuncscript(mcstate_t* state, mcvalue_t obj, mcvaldict_t* targetdict)
{
    bool ok;
    int i;
    uint16_t* bytecodecopy;
    mcobjfuncscript_t* functioncopy;
    mcvalue_t copy;
    mcvalue_t freeval;
    mcvalue_t freevalcopy;
    mcobjfuncscript_t* function;
    mcastlocation_t* srcpositionscopy;
    mccompiledprogram_t* comprescopy;
    function = mc_value_asscriptfunction(obj);
    bytecodecopy = NULL;
    srcpositionscopy = NULL;
    comprescopy = NULL;
    bytecodecopy = (uint16_t*)mc_allocator_malloc(state, sizeof(uint16_t) * function->compiledprogcode->count);
    if(!bytecodecopy)
    {
        return mc_value_makenull();
    }
    memcpy(bytecodecopy, function->compiledprogcode->bytecode, sizeof(uint16_t) * function->compiledprogcode->count);
    srcpositionscopy = (mcastlocation_t*)mc_allocator_malloc(state, sizeof(mcastlocation_t) * function->compiledprogcode->count);
    if(!srcpositionscopy)
    {
        mc_memory_free(bytecodecopy);
        return mc_value_makenull();
    }
    memcpy(srcpositionscopy, function->compiledprogcode->progsrcposlist, sizeof(mcastlocation_t) * function->compiledprogcode->count);
    comprescopy = mc_astcompresult_make(state, bytecodecopy, srcpositionscopy, function->compiledprogcode->count);
    /*
    * todo: add compilation result copy function
    */
    if(!comprescopy)
    {
        mc_memory_free(srcpositionscopy);
        mc_memory_free(bytecodecopy);
        return mc_value_makenull();
    }
    copy = mc_value_makefuncscript(state, mc_value_functiongetname(obj), comprescopy, true, function->numlocals, function->numargs, 0);
    if(mc_value_isnull(copy))
    {
        mc_astcompresult_destroy(comprescopy);
        return mc_value_makenull();
    }
    ok = mc_valdict_setkv(targetdict, &obj, &copy);
    if(!ok)
    {
        return mc_value_makenull();
    }
    functioncopy = mc_value_asscriptfunction(copy);
    if(mc_objfunction_freevalsareallocated(function))
    {
        functioncopy->ufv.freevalsallocated = (mcvalue_t*)mc_allocator_malloc(state, sizeof(mcvalue_t) * function->freevalscount);
        if(!functioncopy->ufv.freevalsallocated)
        {
            return mc_value_makenull();
        }
    }
    functioncopy->freevalscount = function->freevalscount;
    for(i = 0; i < function->freevalscount; i++)
    {
        freeval = mc_value_functiongetfreevalat(obj, i);
        freevalcopy = mc_value_copydeepintern(state, freeval, targetdict);
        if(!mc_value_isnull(freeval) && mc_value_isnull(freevalcopy))
        {
            return mc_value_makenull();
        }
        mc_value_functionsetfreevalat(copy, i, freevalcopy);
    }
    return copy;
}

mcvalue_t mc_value_copydeeparray(mcstate_t* state, mcvalue_t obj, mcvaldict_t* targetdict)
{
    bool ok;
    int i;
    int len;
    mcvalue_t copy;
    mcvalue_t item;
    mcvalue_t itemcopy;
    len = mc_value_arraygetlength(obj);
    copy = mc_value_makearraycapacity(state, len);
    if(mc_value_isnull(copy))
    {
        return mc_value_makenull();
    }
    ok = mc_valdict_setkv(targetdict, &obj, &copy);
    if(!ok)
    {
        return mc_value_makenull();
    }
    for(i = 0; i < len; i++)
    {
        item = mc_value_arraygetvalue(obj, i);
        itemcopy = mc_value_copydeepintern(state, item, targetdict);
        if(!mc_value_isnull(item) && mc_value_isnull(itemcopy))
        {
            return mc_value_makenull();
        }
        ok = mc_value_arraypush(copy, itemcopy);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return copy;
}

mcvalue_t mc_value_copydeepmap(mcstate_t* state, mcvalue_t obj, mcvaldict_t* targetdict)
{
    bool ok;
    int i;
    mcvalue_t key;
    mcvalue_t val;
    mcvalue_t copy;
    mcvalue_t keycopy;
    mcvalue_t valcopy;
    copy = mc_value_makemap(state);
    if(mc_value_isnull(copy))
    {
        return mc_value_makenull();
    }
    ok = mc_valdict_setkv(targetdict, &obj, &copy);
    if(!ok)
    {
        return mc_value_makenull();
    }
    for(i = 0; i < mc_value_mapgetlength(obj); i++)
    {
        key = mc_value_mapgetkeyat(obj, i);
        val = mc_value_mapgetvalueat(obj, i);
        keycopy = mc_value_copydeepintern(state, key, targetdict);
        if(!mc_value_isnull(key) && mc_value_isnull(keycopy))
        {
            return mc_value_makenull();
        }
        valcopy = mc_value_copydeepintern(state, val, targetdict);
        if(!mc_value_isnull(val) && mc_value_isnull(valcopy))
        {
            return mc_value_makenull();
        }
        ok = mc_value_mapsetvalue(copy, keycopy, valcopy);
        if(!ok)
        {
            return mc_value_makenull();
        }
    }
    return copy;
}

mcvalue_t mc_value_copydeepintern(mcstate_t* state, mcvalue_t obj, mcvaldict_t* targetdict)
{
    mcvaltype_t type;
    mcvalue_t copy;
    mcvalue_t* copyptr;
    copyptr = (mcvalue_t*)mc_valdict_get(targetdict, &obj);
    if(copyptr)
    {
        return *copyptr;
    }
    copy = mc_value_makenull();
    type = mc_value_gettype(obj);
    switch(type)
    {
        case MC_VAL_FREED:
        case MC_VAL_ANY:
        case MC_VAL_NONE:
            {
                MC_ASSERT(false);
                copy = mc_value_makenull();
            }
            break;
        case MC_VAL_NUMBER:
        case MC_VAL_BOOL:
        case MC_VAL_NULL:
        case MC_VAL_FUNCNATIVE:
            {
                copy = obj;
            }
            break;
        case MC_VAL_STRING:
            {
                bool ok;
                int len;
                const char* str;
                str = mc_value_stringgetdata(obj);
                len = mc_value_stringgetlength(obj);
                copy = mc_value_makestringlen(state, str, len);
                ok = mc_valdict_setkv(targetdict, &obj, &copy);
                if(!ok)
                {
                    return mc_value_makenull();
                }
                return copy;
            }
            break;
        case MC_VAL_FUNCSCRIPT:
            {
                return mc_value_copydeepfuncscript(state, obj, targetdict);
            }
            break;
        case MC_VAL_ARRAY:
            {
                return mc_value_copydeeparray(state, obj, targetdict);
            }
            break;
        case MC_VAL_MAP:
            {
                return mc_value_copydeepmap(state, obj, targetdict);
            }
            break;
        case MC_VAL_EXTERNAL:
            {
                copy = mc_value_copyflat(state, obj);
            }
            break;
        case MC_VAL_ERROR:
            {
                copy = obj;
            }
            break;
    }
    return copy;
}

mcvalue_t mc_value_copydeep(mcstate_t* state, mcvalue_t obj)
{
    mcvalue_t res;
    mcvaldict_t* targetdict;
    targetdict = mc_valdict_makedefault(state, sizeof(mcvalue_t), sizeof(mcvalue_t));
    if(!targetdict)
    {
        return mc_value_makenull();
    }
    res = mc_value_copydeepintern(state, obj, targetdict);
    mc_valdict_destroy(targetdict);
    return res;
}

mcvalue_t mc_value_copyflat(mcstate_t* state, mcvalue_t obj)
{
    bool ok;
    mcvalue_t copy;
    mcvaltype_t type;
    copy = mc_value_makenull();
    type = mc_value_gettype(obj);
    switch(type)
    {
        case MC_VAL_ANY:
        case MC_VAL_FREED:
        case MC_VAL_NONE:
            {
                MC_ASSERT(false);
                copy = mc_value_makenull();
            }
            break;
        case MC_VAL_NUMBER:
        case MC_VAL_BOOL:
        case MC_VAL_NULL:
        case MC_VAL_FUNCSCRIPT:
        case MC_VAL_FUNCNATIVE:
        case MC_VAL_ERROR:
            {
                copy = obj;
            }
            break;
        case MC_VAL_STRING:
            {
                size_t len;
                const char* str;
                str = mc_value_stringgetdata(obj);
                len = mc_value_stringgetlength(obj);
                copy = mc_value_makestringlen(state, str, len);
            }
            break;
        case MC_VAL_ARRAY:
            {
                int i;
                int len;
                mcvalue_t item;
                len = mc_value_arraygetlength(obj);
                copy = mc_value_makearraycapacity(state, len);
                if(mc_value_isnull(copy))
                {
                    return mc_value_makenull();
                }
                for(i = 0; i < len; i++)
                {
                    item = mc_value_arraygetvalue(obj, i);
                    ok = mc_value_arraypush(copy, item);
                    if(!ok)
                    {
                        return mc_value_makenull();
                    }
                }
            }
            break;
        case MC_VAL_MAP:
            {
                int i;
                mcvalue_t key;
                mcvalue_t val;
                copy = mc_value_makemap(state);
                for(i = 0; i < mc_value_mapgetlength(obj); i++)
                {
                    key = mc_value_mapgetkeyat(obj, i);
                    val = mc_value_mapgetvalueat(obj, i);
                    ok = mc_value_mapsetvalue(copy, key, val);
                    if(!ok)
                    {
                        return mc_value_makenull();
                    }
                }
            }
            break;
        case MC_VAL_EXTERNAL:
            {
                void* datacopy;
                mcobjuserdata_t* objext;
                copy = mc_value_makeuserobject(state, NULL);
                if(mc_value_isnull(copy))
                {
                    return mc_value_makenull();
                }
                objext = mc_value_userdatagetdata(obj);
                datacopy = NULL;
                if(objext->datacopyfn)
                {
                    datacopy = objext->datacopyfn(objext->data);
                }
                else
                {
                    datacopy = objext->data;
                }
                mc_value_userdatasetdata(copy, datacopy);
                mc_value_userdatasetdestroyfunction(copy, objext->datadestroyfn);
                mc_value_userdatasetcopyfunction(copy, objext->datacopyfn);
            }
            break;
    }
    return copy;
}

