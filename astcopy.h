
mcastexpression_t* mc_astexpr_copy(mcastexpression_t* expr)
{
    mcastexpression_t* res;
    if(!expr)
    {
        return NULL;
    }
    res = NULL;
    fprintf(stderr, "call to mc_astexpr_copy\n");
    switch(expr->type)
    {
        case MC_EXPR_NONE:
            {
                MC_ASSERT(false);
            }
            break;
        case MC_EXPR_IDENT:
            {
                mcastident_t* ident;
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
                    mc_allocator_free(expr->pstate, stringcopy);
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
                valuescopy = mc_ptrlist_copy(expr->uexpr.exprlitarray.litarritems, (mcitemcopyfn_t)mc_astexpr_copy, (mcitemdestroyfn_t)mc_astexpr_destroy);
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
                keyscopy = mc_ptrlist_copy(expr->uexpr.exprlitmap.keys, (mcitemcopyfn_t)mc_astexpr_copy, (mcitemdestroyfn_t)mc_astexpr_destroy);
                valuescopy = mc_ptrlist_copy(expr->uexpr.exprlitmap.values, (mcitemcopyfn_t)mc_astexpr_copy, (mcitemdestroyfn_t)mc_astexpr_destroy);
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
                rightcopy = mc_astexpr_copy(expr->uexpr.exprprefix.right);
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
                leftcopy = mc_astexpr_copy(expr->uexpr.exprinfix.left);
                rightcopy = mc_astexpr_copy(expr->uexpr.exprinfix.right);
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
                mcastcodeblock_t* bodycopy;
                pacopy = mc_ptrlist_copy(expr->uexpr.exprlitfunction.funcparamlist, (mcitemcopyfn_t)mc_astfuncparam_copy, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                bodycopy = mc_astcodeblock_copy(expr->uexpr.exprlitfunction.body);
                namecopy = mc_util_strdup(expr->pstate, expr->uexpr.exprlitfunction.name);
                if(!pacopy || !bodycopy)
                {
                    mc_ptrlist_destroy(pacopy, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                    mc_astcodeblock_destroy(bodycopy);
                    mc_allocator_free(expr->pstate, namecopy);
                    return NULL;
                }
                res = mc_astexpr_makeliteralfunction(expr->pstate, pacopy, bodycopy);
                if(!res)
                {
                    mc_ptrlist_destroy(pacopy, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
                    mc_astcodeblock_destroy(bodycopy);
                    mc_allocator_free(expr->pstate, namecopy);
                    return NULL;
                }
                res->uexpr.exprlitfunction.name = namecopy;
            }
            break;
        case MC_EXPR_CALL:
            {
                mcastexpression_t* fcopy;
                mcptrlist_t* argscopy;
                fcopy = mc_astexpr_copy(expr->uexpr.exprcall.function);
                argscopy = mc_ptrlist_copy(expr->uexpr.exprcall.args, (mcitemcopyfn_t)mc_astexpr_copy, (mcitemdestroyfn_t)mc_astexpr_destroy);
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
                leftcopy = mc_astexpr_copy(expr->uexpr.exprindex.left);
                indexcopy = mc_astexpr_copy(expr->uexpr.exprindex.index);
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
                destcopy = mc_astexpr_copy(expr->uexpr.exprassign.dest);
                sourcecopy = mc_astexpr_copy(expr->uexpr.exprassign.source);
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
                leftcopy = mc_astexpr_copy(expr->uexpr.exprlogical.left);
                rightcopy = mc_astexpr_copy(expr->uexpr.exprlogical.right);
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
                testcopy = mc_astexpr_copy(expr->uexpr.exprternary.tercond);
                iftruecopy = mc_astexpr_copy(expr->uexpr.exprternary.teriftrue);
                iffalsecopy = mc_astexpr_copy(expr->uexpr.exprternary.teriffalse);
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
                valuecopy = mc_astexpr_copy(expr->uexpr.exprdefine.value);
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
                mcastcodeblock_t* alternativecopy;
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
                valuecopy = mc_astexpr_copy(expr->uexpr.exprreturnvalue);
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
                valuecopy = mc_astexpr_copy(expr->uexpr.exprexpression);
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
                mcastcodeblock_t* bodycopy;
                testcopy = mc_astexpr_copy(expr->uexpr.exprwhileloopstmt.loopcond);
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
                mcastcodeblock_t* bodycopy;
                sourcecopy = mc_astexpr_copy(expr->uexpr.exprforeachloopstmt.source);
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
                mcastcodeblock_t* bodycopy;
                initcopy= mc_astexpr_copy(expr->uexpr.exprforloopstmt.init);
                testcopy = mc_astexpr_copy(expr->uexpr.exprforloopstmt.loopcond);
                updatecopy = mc_astexpr_copy(expr->uexpr.exprforloopstmt.update);
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
                mcastcodeblock_t* blockcopy;
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
                    mc_allocator_free(expr->pstate, pathcopy);
                    return NULL;
                }
            }
            break;
        case MC_EXPR_STMTRECOVER:
            {
                mcastcodeblock_t* bodycopy;
                mcastident_t* erroridentcopy;
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
