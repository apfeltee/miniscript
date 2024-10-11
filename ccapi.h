
mcastexpression_t* mc_astexpr_makeident(mcstate_t* state, mcastexprident_t* ident)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_IDENT);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprident = ident;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralnumber(mcstate_t* state, mcfloat_t val)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_NUMBERLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlitnumber = val;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralbool(mcstate_t* state, bool val)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_BOOLLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlitbool = val;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralstring(mcstate_t* state, char* value, size_t len)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STRINGLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlitstring.data = value;
    res->uexpr.exprlitstring.length = len;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralnull(mcstate_t* state)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_NULLLITERAL);
    if(!res)
    {
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralarray(mcstate_t* state, mcptrlist_t* values)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_ARRAYLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlitarray.litarritems = values;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralmap(mcstate_t* state, mcptrlist_t* keys, mcptrlist_t* values)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_MAPLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlitmap.litmapkeys = keys;
    res->uexpr.exprlitmap.litmapvalues = values;
    return res;
}

mcastexpression_t* mc_astexpr_makeprefixexpr(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* right)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_PREFIX);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprprefix.op = op;
    res->uexpr.exprprefix.right = right;
    return res;
}

mcastexpression_t* mc_astexpr_makeinfixexpr(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* left, mcastexpression_t* right)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_INFIX);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprinfix.op = op;
    res->uexpr.exprinfix.left = left;
    res->uexpr.exprinfix.right = right;
    return res;
}

mcastexpression_t* mc_astexpr_makeliteralfunction(mcstate_t* state, mcptrlist_t* params, mcastexprcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_FUNCTIONLITERAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlitfunction.name = NULL;
    res->uexpr.exprlitfunction.funcparamlist = params;
    res->uexpr.exprlitfunction.body = body;
    return res;
}

mcastexpression_t* mc_astexpr_makecallexpr(mcstate_t* state, mcastexpression_t* function, mcptrlist_t* args)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_CALL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprcall.function = function;
    res->uexpr.exprcall.args = args;
    return res;
}

mcastexpression_t* mc_astexpr_makeindexexpr(mcstate_t* state, mcastexpression_t* left, mcastexpression_t* index, bool isdot)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_INDEX);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprindex.isdot = isdot;
    res->uexpr.exprindex.left = left;
    res->uexpr.exprindex.index = index;
    return res;
}

mcastexpression_t* mc_astexpr_makeassignexpr(mcstate_t* state, mcastexpression_t* dest, mcastexpression_t* source, bool is_postfix)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_ASSIGN);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprassign.dest = dest;
    res->uexpr.exprassign.source = source;
    res->uexpr.exprassign.is_postfix = is_postfix;
    return res;
}

mcastexpression_t* mc_astexpr_makelogicalexpr(mcstate_t* state, mcastmathoptype_t op, mcastexpression_t* left, mcastexpression_t* right)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_LOGICAL);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprlogical.op = op;
    res->uexpr.exprlogical.left = left;
    res->uexpr.exprlogical.right = right;
    return res;
}

mcastexpression_t* mc_astexpr_maketernaryexpr(mcstate_t* state, mcastexpression_t* test, mcastexpression_t* ift, mcastexpression_t* iffalse)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_TERNARY);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprternary.tercond = test;
    res->uexpr.exprternary.teriftrue = ift;
    res->uexpr.exprternary.teriffalse = iffalse;
    return res;
}

mcastexpression_t* mc_parser_makefunccallexpr(mcstate_t* state, mcastexpression_t* expr, const char* fname)
{
    bool ok;
    mcasttoken_t fntoken;
    mcastexprident_t* ident;
    mcptrlist_t* args;
    mcastexpression_t* ce;
    mcastexpression_t* functionidentexpr;
    mc_asttoken_init(&fntoken, MC_TOK_IDENT, fname, mc_util_strlen(fname));
    fntoken.pos = expr->pos;
    ident = mc_astident_make(state, fntoken);
    if(!ident)
    {
        return NULL;
    }
    ident->pos = fntoken.pos;
    functionidentexpr = mc_astexpr_makeident(state, ident);
    if(!functionidentexpr)
    {
        mc_astident_destroy(ident);
        return NULL;
    }
    functionidentexpr->pos = expr->pos;
    ident = NULL;
    args = mc_ptrlist_make(state, 0, sizeof(void*));
    if(!args)
    {
        mc_astexpr_destroy(functionidentexpr);
        return NULL;
    }
    ok = mc_ptrlist_push(args, expr);
    if(!ok)
    {
        mc_ptrlist_destroy(args, NULL);
        mc_astexpr_destroy(functionidentexpr);
        return NULL;
    }
    ce = mc_astexpr_makecallexpr(state, functionidentexpr, args);
    if(!ce)
    {
        mc_ptrlist_destroy(args, NULL);
        mc_astexpr_destroy(functionidentexpr);
        return NULL;
    }
    ce->pos = expr->pos;
    return ce;
}

void mc_astexpr_destroy(mcastexpression_t* expr)
{
    if(!expr)
    {
        return;
    }
    switch(expr->exprtype)
    {
        case MC_EXPR_NONE:
            {
                MC_ASSERT(false);
            }
            break;
        case MC_EXPR_IDENT:
            {
                mc_astident_destroy(expr->uexpr.exprident);
            }
            break;
        case MC_EXPR_NUMBERLITERAL:
        case MC_EXPR_BOOLLITERAL:
            {
            }
            break;
        case MC_EXPR_STRINGLITERAL:
            {
                mc_memory_free(expr->uexpr.exprlitstring.data);
            }
            break;
        case MC_EXPR_NULLLITERAL:
            {
            }
            break;
        case MC_EXPR_ARRAYLITERAL:
            {
                mc_ptrlist_destroy(expr->uexpr.exprlitarray.litarritems, (mcitemdestroyfn_t)mc_astexpr_destroy);
            }
            break;
        case MC_EXPR_MAPLITERAL:
            {
                mc_ptrlist_destroy(expr->uexpr.exprlitmap.litmapkeys, (mcitemdestroyfn_t)mc_astexpr_destroy);
                mc_ptrlist_destroy(expr->uexpr.exprlitmap.litmapvalues, (mcitemdestroyfn_t)mc_astexpr_destroy);
            }
            break;
        case MC_EXPR_PREFIX:
            {
                mc_astexpr_destroy(expr->uexpr.exprprefix.right);
            }
            break;
        case MC_EXPR_INFIX:
            {
                mc_astexpr_destroy(expr->uexpr.exprinfix.left);
                mc_astexpr_destroy(expr->uexpr.exprinfix.right);
            }
            break;
        case MC_EXPR_FUNCTIONLITERAL:
            {
                mcastexprliteralfunction_t* fn;
                fn = &expr->uexpr.exprlitfunction;
                mc_memory_free(fn->name);
                mc_ptrlist_destroy(fn->funcparamlist, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                mc_astcodeblock_destroy(fn->body);
            }
            break;
        case MC_EXPR_CALL:
            {
                mc_ptrlist_destroy(expr->uexpr.exprcall.args, (mcitemdestroyfn_t)mc_astexpr_destroy);
                mc_astexpr_destroy(expr->uexpr.exprcall.function);
            }
            break;
        case MC_EXPR_INDEX:
            {
                mc_astexpr_destroy(expr->uexpr.exprindex.left);
                mc_astexpr_destroy(expr->uexpr.exprindex.index);
            }
            break;
        case MC_EXPR_ASSIGN:
            {
                mc_astexpr_destroy(expr->uexpr.exprassign.dest);
                mc_astexpr_destroy(expr->uexpr.exprassign.source);
            }
            break;
        case MC_EXPR_LOGICAL:
            {
                mc_astexpr_destroy(expr->uexpr.exprlogical.left);
                mc_astexpr_destroy(expr->uexpr.exprlogical.right);
            }
            break;
        case MC_EXPR_TERNARY:
            {
                mc_astexpr_destroy(expr->uexpr.exprternary.tercond);
                mc_astexpr_destroy(expr->uexpr.exprternary.teriftrue);
                mc_astexpr_destroy(expr->uexpr.exprternary.teriffalse);
            }
            break;
        case MC_EXPR_STMTDEFINE:
            {
                mc_astident_destroy(expr->uexpr.exprdefine.name);
                mc_astexpr_destroy(expr->uexpr.exprdefine.value);
            }
            break;
        case MC_EXPR_STMTIF:
            {
                mc_ptrlist_destroy(expr->uexpr.exprifstmt.cases, (mcitemdestroyfn_t)mc_astifcase_destroy);
                mc_astcodeblock_destroy(expr->uexpr.exprifstmt.alternative);
            }
            break;
        case MC_EXPR_STMTRETURN:
            {
                mc_astexpr_destroy(expr->uexpr.exprreturnvalue);
            }
            break;
        case MC_EXPR_STMTEXPRESSION:
            {
                mc_astexpr_destroy(expr->uexpr.exprexpression);
            }
            break;
        case MC_EXPR_STMTLOOPWHILE:
            {
                mc_astexpr_destroy(expr->uexpr.exprwhileloopstmt.loopcond);
                mc_astcodeblock_destroy(expr->uexpr.exprwhileloopstmt.body);
            }
            break;
        case MC_EXPR_STMTBREAK:
            {
            }
            break;
        case MC_EXPR_STMTCONTINUE:
            {
            }
            break;
        case MC_EXPR_STMTLOOPFOREACH:
            {
                mc_astident_destroy(expr->uexpr.exprforeachloopstmt.iterator);
                mc_astexpr_destroy(expr->uexpr.exprforeachloopstmt.source);
                mc_astcodeblock_destroy(expr->uexpr.exprforeachloopstmt.body);
            }
            break;
        case MC_EXPR_STMTLOOPFORCLASSIC:
            {
                mc_astexpr_destroy(expr->uexpr.exprforloopstmt.init);
                mc_astexpr_destroy(expr->uexpr.exprforloopstmt.loopcond);
                mc_astexpr_destroy(expr->uexpr.exprforloopstmt.update);
                mc_astcodeblock_destroy(expr->uexpr.exprforloopstmt.body);
            }
            break;
        case MC_EXPR_STMTBLOCK:
            {
                mc_astcodeblock_destroy(expr->uexpr.exprblockstmt);
            }
            break;
        case MC_EXPR_STMTIMPORT:
            {
                mc_memory_free(expr->uexpr.exprimportstmt.path);
            }
            break;
        case MC_EXPR_STMTRECOVER:
            {
                mc_astcodeblock_destroy(expr->uexpr.exprrecoverstmt.body);
                mc_astident_destroy(expr->uexpr.exprrecoverstmt.errident);
            }
            break;
        default:
            {
            }
            break;
    }
    mc_memory_free(expr);
}


mcastexpression_t* mc_astexpr_copyexpr(mcastexpression_t* expr)
{
    mcastexpression_t* res;
    if(!expr)
    {
        return NULL;
    }
    res = NULL;
    switch(expr->exprtype)
    {
        case MC_EXPR_NONE:
            {
                MC_ASSERT(false);
            }
            break;
        case MC_EXPR_IDENT:
            {
                mcastexprident_t* ident;
                ident = mc_astident_copy(expr->uexpr.exprident);
                if(!ident)
                {
                    return NULL;
                }
                res = mc_astexpr_makeident(expr->pstate, ident);
                if(!res)
                {
                    mc_astident_destroy(ident);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_NUMBERLITERAL:
            {
                res = mc_astexpr_makeliteralnumber(expr->pstate, expr->uexpr.exprlitnumber);
            }
            break;
        case MC_EXPR_BOOLLITERAL:
            {
                res = mc_astexpr_makeliteralbool(expr->pstate, expr->uexpr.exprlitbool);
            }
            break;
        case MC_EXPR_STRINGLITERAL:
            {
                char* stringcopy;
                stringcopy = mc_util_strndup(expr->pstate, expr->uexpr.exprlitstring.data, expr->uexpr.exprlitstring.length);
                if(!stringcopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makeliteralstring(expr->pstate, stringcopy, expr->uexpr.exprlitstring.length);
                if(!res)
                {
                    mc_memory_free(stringcopy);
                    return NULL;
                }
            }
            break;

        case MC_EXPR_NULLLITERAL:
            {
                res = mc_astexpr_makeliteralnull(expr->pstate);
            }
            break;
        case MC_EXPR_ARRAYLITERAL:
            {
                mcptrlist_t* valuescopy;
                valuescopy = mc_ptrlist_copy(expr->uexpr.exprlitarray.litarritems, (mcitemcopyfn_t)mc_astexpr_copyexpr, (mcitemdestroyfn_t)mc_astexpr_destroy);
                if(!valuescopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makeliteralarray(expr->pstate, valuescopy);
                if(!res)
                {
                    mc_ptrlist_destroy(valuescopy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    return NULL;
                }
            }
            break;

        case MC_EXPR_MAPLITERAL:
            {
                mcptrlist_t* keyscopy;
                mcptrlist_t* valuescopy;
                keyscopy = mc_ptrlist_copy(expr->uexpr.exprlitmap.litmapkeys, (mcitemcopyfn_t)mc_astexpr_copyexpr, (mcitemdestroyfn_t)mc_astexpr_destroy);
                valuescopy = mc_ptrlist_copy(expr->uexpr.exprlitmap.litmapvalues, (mcitemcopyfn_t)mc_astexpr_copyexpr, (mcitemdestroyfn_t)mc_astexpr_destroy);
                if(!keyscopy || !valuescopy)
                {
                    mc_ptrlist_destroy(keyscopy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    mc_ptrlist_destroy(valuescopy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    return NULL;
                }
                res = mc_astexpr_makeliteralmap(expr->pstate, keyscopy, valuescopy);
                if(!res)
                {
                    mc_ptrlist_destroy(keyscopy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    mc_ptrlist_destroy(valuescopy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_PREFIX:
            {
                mcastexpression_t* rightcopy;
                rightcopy = mc_astexpr_copyexpr(expr->uexpr.exprprefix.right);
                if(!rightcopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makeprefixexpr(expr->pstate, expr->uexpr.exprprefix.op, rightcopy);
                if(!res)
                {
                    mc_astexpr_destroy(rightcopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_INFIX:
            {
                mcastexpression_t* leftcopy;
                mcastexpression_t* rightcopy;
                leftcopy = mc_astexpr_copyexpr(expr->uexpr.exprinfix.left);
                rightcopy = mc_astexpr_copyexpr(expr->uexpr.exprinfix.right);
                if(!leftcopy || !rightcopy)
                {
                    mc_astexpr_destroy(leftcopy);
                    mc_astexpr_destroy(rightcopy);
                    return NULL;
                }
                res = mc_astexpr_makeinfixexpr(expr->pstate, expr->uexpr.exprinfix.op, leftcopy, rightcopy);
                if(!res)
                {
                    mc_astexpr_destroy(leftcopy);
                    mc_astexpr_destroy(rightcopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_FUNCTIONLITERAL:
            {
                char* namecopy;
                mcptrlist_t* pacopy;
                mcastexprcodeblock_t* bodycopy;
                pacopy = mc_ptrlist_copy(expr->uexpr.exprlitfunction.funcparamlist, (mcitemcopyfn_t)mc_astfuncparam_copy, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                bodycopy = mc_astcodeblock_copy(expr->uexpr.exprlitfunction.body);
                namecopy = mc_util_strdup(expr->pstate, expr->uexpr.exprlitfunction.name);
                if(!pacopy || !bodycopy)
                {
                    mc_ptrlist_destroy(pacopy, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                    mc_astcodeblock_destroy(bodycopy);
                    mc_memory_free(namecopy);
                    return NULL;
                }
                res = mc_astexpr_makeliteralfunction(expr->pstate, pacopy, bodycopy);
                if(!res)
                {
                    mc_ptrlist_destroy(pacopy, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                    mc_astcodeblock_destroy(bodycopy);
                    mc_memory_free(namecopy);
                    return NULL;
                }
                res->uexpr.exprlitfunction.name = namecopy;
            }
            break;
        case MC_EXPR_CALL:
            {
                mcastexpression_t* fcopy;
                mcptrlist_t* argscopy;
                fcopy = mc_astexpr_copyexpr(expr->uexpr.exprcall.function);
                argscopy = mc_ptrlist_copy(expr->uexpr.exprcall.args, (mcitemcopyfn_t)mc_astexpr_copyexpr, (mcitemdestroyfn_t)mc_astexpr_destroy);
                if(!fcopy || !argscopy)
                {
                    mc_astexpr_destroy(fcopy);
                    mc_ptrlist_destroy(expr->uexpr.exprcall.args, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    return NULL;
                }
                res = mc_astexpr_makecallexpr(expr->pstate, fcopy, argscopy);
                if(!res)
                {
                    mc_astexpr_destroy(fcopy);
                    mc_ptrlist_destroy(expr->uexpr.exprcall.args, (mcitemdestroyfn_t)mc_astexpr_destroy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_INDEX:
            {
                mcastexpression_t* leftcopy;
                mcastexpression_t* indexcopy;
                leftcopy = mc_astexpr_copyexpr(expr->uexpr.exprindex.left);
                indexcopy = mc_astexpr_copyexpr(expr->uexpr.exprindex.index);
                if(!leftcopy || !indexcopy)
                {
                    mc_astexpr_destroy(leftcopy);
                    mc_astexpr_destroy(indexcopy);
                    return NULL;
                }
                res = mc_astexpr_makeindexexpr(expr->pstate, leftcopy, indexcopy, false);
                if(!res)
                {
                    mc_astexpr_destroy(leftcopy);
                    mc_astexpr_destroy(indexcopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_ASSIGN:
            {
                mcastexpression_t* destcopy;
                mcastexpression_t* sourcecopy;
                destcopy = mc_astexpr_copyexpr(expr->uexpr.exprassign.dest);
                sourcecopy = mc_astexpr_copyexpr(expr->uexpr.exprassign.source);
                if(!destcopy || !sourcecopy)
                {
                    mc_astexpr_destroy(destcopy);
                    mc_astexpr_destroy(sourcecopy);
                    return NULL;
                }
                res = mc_astexpr_makeassignexpr(expr->pstate, destcopy, sourcecopy, expr->uexpr.exprassign.is_postfix);
                if(!res)
                {
                    mc_astexpr_destroy(destcopy);
                    mc_astexpr_destroy(sourcecopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_LOGICAL:
            {
                mcastexpression_t* leftcopy;
                mcastexpression_t* rightcopy;
                leftcopy = mc_astexpr_copyexpr(expr->uexpr.exprlogical.left);
                rightcopy = mc_astexpr_copyexpr(expr->uexpr.exprlogical.right);
                if(!leftcopy || !rightcopy)
                {
                    mc_astexpr_destroy(leftcopy);
                    mc_astexpr_destroy(rightcopy);
                    return NULL;
                }
                res = mc_astexpr_makelogicalexpr(expr->pstate, expr->uexpr.exprlogical.op, leftcopy, rightcopy);
                if(!res)
                {
                    mc_astexpr_destroy(leftcopy);
                    mc_astexpr_destroy(rightcopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_TERNARY:
            {
                mcastexpression_t* testcopy;
                mcastexpression_t* iftruecopy;
                mcastexpression_t* iffalsecopy;
                testcopy = mc_astexpr_copyexpr(expr->uexpr.exprternary.tercond);
                iftruecopy = mc_astexpr_copyexpr(expr->uexpr.exprternary.teriftrue);
                iffalsecopy = mc_astexpr_copyexpr(expr->uexpr.exprternary.teriffalse);
                if(!testcopy || !iftruecopy || !iffalsecopy)
                {
                    mc_astexpr_destroy(testcopy);
                    mc_astexpr_destroy(iftruecopy);
                    mc_astexpr_destroy(iffalsecopy);
                    return NULL;
                }
                res = mc_astexpr_maketernaryexpr(expr->pstate, testcopy, iftruecopy, iffalsecopy);
                if(!res)
                {
                    mc_astexpr_destroy(testcopy);
                    mc_astexpr_destroy(iftruecopy);
                    mc_astexpr_destroy(iffalsecopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTDEFINE:
            {
                mcastexpression_t* valuecopy;
                valuecopy = mc_astexpr_copyexpr(expr->uexpr.exprdefine.value);
                if(!valuecopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makedefineexpr(expr->pstate, mc_astident_copy(expr->uexpr.exprdefine.name), valuecopy, expr->uexpr.exprdefine.assignable);
                if(!res)
                {
                    mc_astexpr_destroy(valuecopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTIF:
            {
                mcptrlist_t* casescopy;
                mcastexprcodeblock_t* alternativecopy;
                casescopy = mc_ptrlist_copy(expr->uexpr.exprifstmt.cases, (mcitemcopyfn_t)mc_astifcase_copy, (mcitemdestroyfn_t)mc_astifcase_destroy);
                alternativecopy = mc_astcodeblock_copy(expr->uexpr.exprifstmt.alternative);
                if(!casescopy || !alternativecopy)
                {
                    mc_ptrlist_destroy(casescopy, (mcitemdestroyfn_t)mc_astifcase_destroy);
                    mc_astcodeblock_destroy(alternativecopy);
                    return NULL;
                }
                res = mc_astexpr_makeifexpr(expr->pstate, casescopy, alternativecopy);
                if(res)
                {
                    mc_ptrlist_destroy(casescopy, (mcitemdestroyfn_t)mc_astifcase_destroy);
                    mc_astcodeblock_destroy(alternativecopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTRETURN:
            {
                mcastexpression_t* valuecopy;
                valuecopy = mc_astexpr_copyexpr(expr->uexpr.exprreturnvalue);
                if(!valuecopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makereturnexpr(expr->pstate, valuecopy);
                if(!res)
                {
                    mc_astexpr_destroy(valuecopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTEXPRESSION:
            {
                mcastexpression_t* valuecopy;
                valuecopy = mc_astexpr_copyexpr(expr->uexpr.exprexpression);
                if(!valuecopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makeexprstmt(expr->pstate, valuecopy);
                if(!res)
                {
                    mc_astexpr_destroy(valuecopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTLOOPWHILE:
            {
                mcastexpression_t* testcopy;
                mcastexprcodeblock_t* bodycopy;
                testcopy = mc_astexpr_copyexpr(expr->uexpr.exprwhileloopstmt.loopcond);
                bodycopy = mc_astcodeblock_copy(expr->uexpr.exprwhileloopstmt.body);
                if(!testcopy || !bodycopy)
                {
                    mc_astexpr_destroy(testcopy);
                    mc_astcodeblock_destroy(bodycopy);
                    return NULL;
                }
                res = mc_astexpr_makewhileexpr(expr->pstate, testcopy, bodycopy);
                if(!res)
                {
                    mc_astexpr_destroy(testcopy);
                    mc_astcodeblock_destroy(bodycopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTBREAK:
            {
                res = mc_astexpr_makebreakexpr(expr->pstate);
            }
            break;
        case MC_EXPR_STMTCONTINUE:
            {
                res = mc_astexpr_makecontinueexpr(expr->pstate);
            }
            break;
        case MC_EXPR_STMTLOOPFOREACH:
            {
                mcastexpression_t* sourcecopy;
                mcastexprcodeblock_t* bodycopy;
                sourcecopy = mc_astexpr_copyexpr(expr->uexpr.exprforeachloopstmt.source);
                bodycopy = mc_astcodeblock_copy(expr->uexpr.exprforeachloopstmt.body);
                if(!sourcecopy || !bodycopy)
                {
                    mc_astexpr_destroy(sourcecopy);
                    mc_astcodeblock_destroy(bodycopy);
                    return NULL;
                }
                res = mc_astexpr_makeforeachexpr(expr->pstate, mc_astident_copy(expr->uexpr.exprforeachloopstmt.iterator), sourcecopy, bodycopy);
                if(!res)
                {
                    mc_astexpr_destroy(sourcecopy);
                    mc_astcodeblock_destroy(bodycopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTLOOPFORCLASSIC:
            {
                mcastexpression_t* initcopy;
                mcastexpression_t* testcopy;
                mcastexpression_t* updatecopy;
                mcastexprcodeblock_t* bodycopy;
                initcopy= mc_astexpr_copyexpr(expr->uexpr.exprforloopstmt.init);
                testcopy = mc_astexpr_copyexpr(expr->uexpr.exprforloopstmt.loopcond);
                updatecopy = mc_astexpr_copyexpr(expr->uexpr.exprforloopstmt.update);
                bodycopy = mc_astcodeblock_copy(expr->uexpr.exprforloopstmt.body);
                if(!initcopy || !testcopy || !updatecopy || !bodycopy)
                {
                    mc_astexpr_destroy(initcopy);
                    mc_astexpr_destroy(testcopy);
                    mc_astexpr_destroy(updatecopy);
                    mc_astcodeblock_destroy(bodycopy);
                    return NULL;
                }
                res = mc_astexpr_makeforloopexpr(expr->pstate, initcopy, testcopy, updatecopy, bodycopy);
                if(!res)
                {
                    mc_astexpr_destroy(initcopy);
                    mc_astexpr_destroy(testcopy);
                    mc_astexpr_destroy(updatecopy);
                    mc_astcodeblock_destroy(bodycopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTBLOCK:
            {
                mcastexprcodeblock_t* blockcopy;
                blockcopy = mc_astcodeblock_copy(expr->uexpr.exprblockstmt);
                if(!blockcopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makeblockexpr(expr->pstate, blockcopy);
                if(!res)
                {
                    mc_astcodeblock_destroy(blockcopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTIMPORT:
            {
                char* pathcopy;
                pathcopy = mc_util_strdup(expr->pstate, expr->uexpr.exprimportstmt.path);
                if(!pathcopy)
                {
                    return NULL;
                }
                res = mc_astexpr_makeimportexpr(expr->pstate, pathcopy);
                if(!res)
                {
                    mc_memory_free(pathcopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTRECOVER:
            {
                mcastexprcodeblock_t* bodycopy;
                mcastexprident_t* erroridentcopy;
                bodycopy = mc_astcodeblock_copy(expr->uexpr.exprrecoverstmt.body);
                erroridentcopy = mc_astident_copy(expr->uexpr.exprrecoverstmt.errident);
                if(!bodycopy || !erroridentcopy)
                {
                    mc_astcodeblock_destroy(bodycopy);
                    mc_astident_destroy(erroridentcopy);
                    return NULL;
                }
                res = mc_astexpr_makerecoverexpr(expr->pstate, erroridentcopy, bodycopy);
                if(!res)
                {
                    mc_astcodeblock_destroy(bodycopy);
                    mc_astident_destroy(erroridentcopy);
                    return NULL;
                }
            }
            break;
        default:
            {
            }
            break;
    }
    if(!res)
    {
        return NULL;
    }
    res->pos = expr->pos;
    return res;
}

mcastexpression_t* mc_astexpr_makedefineexpr(mcstate_t* state, mcastexprident_t* name, mcastexpression_t* value, bool assignable)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTDEFINE);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprdefine.name = name;
    res->uexpr.exprdefine.value = value;
    res->uexpr.exprdefine.assignable = assignable;
    return res;
}

mcastexpression_t* mc_astexpr_makeifexpr(mcstate_t* state, mcptrlist_t* cases, mcastexprcodeblock_t* alternative)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTIF);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprifstmt.cases = cases;
    res->uexpr.exprifstmt.alternative = alternative;
    return res;
}

mcastexpression_t* mc_astexpr_makereturnexpr(mcstate_t* state, mcastexpression_t* value)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTRETURN);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprreturnvalue = value;
    return res;
}

mcastexpression_t* mc_astexpr_makeexprstmt(mcstate_t* state, mcastexpression_t* value)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTEXPRESSION);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprexpression = value;
    return res;
}

mcastexpression_t* mc_astexpr_makewhileexpr(mcstate_t* state, mcastexpression_t* test, mcastexprcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTLOOPWHILE);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprwhileloopstmt.loopcond = test;
    res->uexpr.exprwhileloopstmt.body = body;
    return res;
}

mcastexpression_t* mc_astexpr_makebreakexpr(mcstate_t* state)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTBREAK);
    if(!res)
    {
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_astexpr_makeforeachexpr(mcstate_t* state, mcastexprident_t* iterator, mcastexpression_t* source, mcastexprcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTLOOPFOREACH);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprforeachloopstmt.iterator = iterator;
    res->uexpr.exprforeachloopstmt.source = source;
    res->uexpr.exprforeachloopstmt.body = body;
    return res;
}

mcastexpression_t* mc_astexpr_makeforloopexpr(mcstate_t* state, mcastexpression_t* init, mcastexpression_t* test, mcastexpression_t* update, mcastexprcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTLOOPFORCLASSIC);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprforloopstmt.init = init;
    res->uexpr.exprforloopstmt.loopcond = test;
    res->uexpr.exprforloopstmt.update = update;
    res->uexpr.exprforloopstmt.body = body;
    return res;
}

mcastexpression_t* mc_astexpr_makecontinueexpr(mcstate_t* state)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTCONTINUE);
    if(!res)
    {
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_astexpr_makeblockexpr(mcstate_t* state, mcastexprcodeblock_t* block)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTBLOCK);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprblockstmt = block;
    return res;
}

mcastexpression_t* mc_astexpr_makeimportexpr(mcstate_t* state, char* path)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTIMPORT);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprimportstmt.path = path;
    return res;
}

mcastexpression_t* mc_astexpr_makerecoverexpr(mcstate_t* state, mcastexprident_t* eid, mcastexprcodeblock_t* body)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeexpression(state, MC_EXPR_STMTRECOVER);
    if(!res)
    {
        return NULL;
    }
    res->uexpr.exprrecoverstmt.errident = eid;
    res->uexpr.exprrecoverstmt.body = body;
    return res;
}

mcastexprcodeblock_t* mc_astcodeblock_make(mcstate_t* state, mcptrlist_t* statements)
{
    mcastexprcodeblock_t* block;
    block = (mcastexprcodeblock_t*)mc_allocator_malloc(state, sizeof(mcastexprcodeblock_t));
    if(!block)
    {
        return NULL;
    }
    block->pstate = state;
    block->statements = statements;
    return block;
}

void mc_astcodeblock_destroy(mcastexprcodeblock_t* block)
{
    if(!block)
    {
        return;
    }
    mc_ptrlist_destroy(block->statements, (mcitemdestroyfn_t)mc_astexpr_destroy);
    mc_memory_free(block);
}

mcastexprcodeblock_t* mc_astcodeblock_copy(mcastexprcodeblock_t* block)
{
    mcastexprcodeblock_t* res;
    mcptrlist_t* statementscopy;
    if(!block)
    {
        return NULL;
    }
    statementscopy = mc_ptrlist_copy(block->statements, (mcitemcopyfn_t)mc_astexpr_copyexpr, (mcitemdestroyfn_t)mc_astexpr_destroy);
    if(!statementscopy)
    {
        return NULL;
    }
    res = mc_astcodeblock_make(block->pstate, statementscopy);
    if(!res)
    {
        mc_ptrlist_destroy(statementscopy, (mcitemdestroyfn_t)mc_astexpr_destroy);
        return NULL;
    }
    return res;
}

mcastfuncparam_t* mc_astfuncparam_make(mcstate_t* state, mcastexprident_t* ident)
{
    mcastfuncparam_t* res;
    res = (mcastfuncparam_t*)mc_allocator_malloc(state, sizeof(mcastfuncparam_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->ident = ident;
    if(!res->ident->value)
    {
        mc_memory_free(res);
        return NULL;
    }
    return res;
}

mcastfuncparam_t* mc_astfuncparam_copy(mcastfuncparam_t* param)
{
    mcastfuncparam_t* res;
    res = (mcastfuncparam_t*)mc_allocator_malloc(param->pstate, sizeof(mcastfuncparam_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = param->pstate;
    res->ident = mc_astident_copy(param->ident);
    if(!res->ident->value)
    {
        mc_memory_free(res);
        return NULL;
    }
    return res;
}

void mc_astfuncparam_destroy(mcastfuncparam_t* param)
{
    if(!param)
    {
        return;
    }
    mc_astident_destroy(param->ident);
    mc_memory_free(param);
}

mcastexprident_t* mc_astident_make(mcstate_t* state, mcasttoken_t tok)
{
    mcastexprident_t* res = (mcastexprident_t*)mc_allocator_malloc(state, sizeof(mcastexprident_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->value = mc_asttoken_dupliteralstring(state, &tok);
    if(!res->value)
    {
        mc_memory_free(res);
        return NULL;
    }
    res->pos = tok.pos;
    return res;
}

mcastexprident_t* mc_astident_copy(mcastexprident_t* ident)
{
    mcastexprident_t* res = (mcastexprident_t*)mc_allocator_malloc(ident->pstate, sizeof(mcastexprident_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = ident->pstate;
    res->value = mc_util_strdup(ident->pstate, ident->value);
    if(!res->value)
    {
        mc_memory_free(res);
        return NULL;
    }
    res->pos = ident->pos;
    return res;
}

void mc_astident_destroy(mcastexprident_t* ident)
{
    if(!ident)
    {
        return;
    }
    mc_memory_free(ident->value);
    ident->value = NULL;
    ident->pos = srcposinvalid;
    mc_memory_free(ident);
}

mcastexprifcase_t* mc_astifcase_make(mcstate_t* state, mcastexpression_t* test, mcastexprcodeblock_t* consequence)
{
    mcastexprifcase_t* res;
    res = (mcastexprifcase_t*)mc_allocator_malloc(state, sizeof(mcastexprifcase_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->ifcond = test;
    res->consequence = consequence;
    return res;
}

void mc_astifcase_destroy(mcastexprifcase_t* cond)
{
    if(!cond)
    {
        return;
    }
    mc_astexpr_destroy(cond->ifcond);
    mc_astcodeblock_destroy(cond->consequence);
    mc_memory_free(cond);
}

mcastexprifcase_t* mc_astifcase_copy(mcastexprifcase_t* ifcase)
{
    mcastexpression_t* testcopy;
    mcastexprcodeblock_t* consequencecopy;
    mcastexprifcase_t* ifcasecopy;
    if(!ifcase)
    {
        return NULL;
    }
    testcopy = NULL;
    consequencecopy = NULL;
    ifcasecopy = NULL;
    testcopy = mc_astexpr_copyexpr(ifcase->ifcond);
    if(!testcopy)
    {
        goto err;
    }
    #if 0
    consequencecopy = mc_astcodeblock_copy(ifcase->consequence);
    #else
    consequencecopy = ifcase->consequence;    
    #endif
    if(!testcopy || !consequencecopy)
    {
        goto err;
    }
    ifcasecopy = mc_astifcase_make(ifcase->pstate, testcopy, consequencecopy);
    if(!ifcasecopy)
    {
        goto err;
    }
    return ifcasecopy;
err:
    mc_astexpr_destroy(testcopy);
    mc_astcodeblock_destroy(consequencecopy);
    mc_astifcase_destroy(ifcasecopy);
    return NULL;
}

mcastexpression_t* mc_astexpr_makeexpression(mcstate_t* state, mcastexprtype_t type)
{
    mcastexpression_t* res = (mcastexpression_t*)mc_allocator_malloc(state, sizeof(mcastexpression_t));
    if(!res)
    {
        return NULL;
    }
    res->pstate = state;
    res->exprtype = type;
    res->pos = srcposinvalid;
    return res;
}
