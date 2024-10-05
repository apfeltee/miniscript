
void mc_astprinter_printast(mcstate_t* state, mcptrlist_t* statements)
{
    mcastprinter_t apr;
    apr.pstate = state;
    apr.pseudolisp = false;
    apr.pdest = state->stderrprinter;
    state->stderrprinter->config.quotstring = true;
    fprintf(stderr, "---AST dump begin---\n");
    mc_astprint_stmtlist(&apr, statements);
    fprintf(stderr, "\n---AST dump end---\n");
    state->stderrprinter->config.quotstring = false;
}

void mc_astprint_stmtlist(mcastprinter_t* apr, mcptrlist_t* statements)
{
    int i;
    int count;
    mcastexpression_t* subex;
    count = mc_ptrlist_count(statements);
    for(i = 0; i < count; i++)
    {
        subex = (mcastexpression_t*)mc_ptrlist_get(statements, i);
        mc_astprint_expression(apr, subex);
        if(i < (count - 1))
        {
            mc_printer_puts(apr->pdest, "\n");
        }
    }
}

void mc_astprint_printfuncliteral(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    size_t i;
    mcastfuncparam_t* param;
    mcastliteralfunction_t* ex;
    ex = &astexpr->uexpr.exprlitfunction;
    if(apr->pseudolisp)
    {
        mc_printer_printf(apr->pdest, "(deffunction '(");
    }
    else
    {
        mc_printer_puts(apr->pdest, "function(");
    }
    for(i = 0; i < mc_ptrlist_count(ex->funcparamlist); i++)
    {
        param = (mcastfuncparam_t*)mc_ptrlist_get(ex->funcparamlist, i);
        mc_printer_puts(apr->pdest, param->ident->value);
        if(i < (mc_ptrlist_count(ex->funcparamlist) - 1))
        {
            mc_printer_puts(apr->pdest, ", ");
        }
    }
    mc_printer_puts(apr->pdest, ") ");
    mc_astprint_codeblock(apr, ex->body);
}

void mc_astprint_printcall(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    size_t i;
    mcastexprcall_t* ex;
    mcastexpression_t* arg;
    ex = &astexpr->uexpr.exprcall;
    mc_astprint_expression(apr, ex->function);
    mc_printer_puts(apr->pdest, "(");
    for(i = 0; i < mc_ptrlist_count(ex->args); i++)
    {
        arg = (mcastexpression_t*)mc_ptrlist_get(ex->args, i);
        mc_astprint_expression(apr, arg);
        if(i < (mc_ptrlist_count(ex->args) - 1))
        {
            mc_printer_puts(apr->pdest, ", ");
        }
    }
    mc_printer_puts(apr->pdest, ")");
}

void mc_astprint_printarrayliteral(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    size_t i;
    size_t len;
    mcastliteralarray_t* ex;
    mcastexpression_t* itemex;
    mcptrlist_t* vl;
    ex = &astexpr->uexpr.exprlitarray;
    vl = ex->litarritems;
    len = mc_ptrlist_count(vl);
    mc_printer_puts(apr->pdest, "[");
    for(i = 0; i < len; i++)
    {
        itemex = (mcastexpression_t*)mc_ptrlist_get(vl, i);
        mc_astprint_expression(apr, itemex);
        if(i < (len - 1))
        {
            mc_printer_puts(apr->pdest, ", ");
        }
    }
    mc_printer_puts(apr->pdest, "]");
}

void mc_astprint_printstringliteral(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    size_t slen;
    const char* sdata;
    mcastliteralstring_t* ex;
    ex = &astexpr->uexpr.exprlitstring;
    sdata = ex->data;
    slen = ex->length;
    if(apr->pdest->config.quotstring)
    {
        mc_printer_printescapedstring(apr->pdest, sdata, slen);
    }
    else
    {
        mc_printer_putlen(apr->pdest, sdata, slen);
    }
}

void mc_astprint_printmapliteral(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    size_t i;
    mcastexpression_t* keyexpr;
    mcastexpression_t* valexpr;
    mcastliteralmap_t* ex;
    ex = &astexpr->uexpr.exprlitmap;
    mc_printer_puts(apr->pdest, "{");
    for(i = 0; i < mc_ptrlist_count(ex->keys); i++)
    {
        keyexpr = (mcastexpression_t*)mc_ptrlist_get(ex->keys, i);
        valexpr = (mcastexpression_t*)mc_ptrlist_get(ex->values, i);
        mc_astprint_expression(apr, keyexpr);
        mc_printer_puts(apr->pdest, " : ");
        mc_astprint_expression(apr, valexpr);
        if(i < (mc_ptrlist_count(ex->keys) - 1))
        {
            mc_printer_puts(apr->pdest, ", ");
        }
    }
    mc_printer_puts(apr->pdest, "}");
}

void mc_astprint_printprefixexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprprefix_t* ex;
    ex = &astexpr->uexpr.exprprefix;
    mc_printer_puts(apr->pdest, "(");
    mc_printer_puts(apr->pdest, mc_util_mathopstring(ex->op));
    mc_astprint_expression(apr, ex->right);
    mc_printer_puts(apr->pdest, ")");
}

void mc_astprint_printinfixexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprinfix_t* ex;
    ex = &astexpr->uexpr.exprinfix;
    mc_printer_puts(apr->pdest, "(");
    mc_astprint_expression(apr, ex->left);
    mc_printer_puts(apr->pdest, " ");
    mc_printer_puts(apr->pdest, mc_util_mathopstring(ex->op));
    mc_printer_puts(apr->pdest, " ");
    mc_astprint_expression(apr, ex->right);
    mc_printer_puts(apr->pdest, ")");
}

void mc_astprint_printindexexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    bool prevquot;
    mcastexprindex_t* ex;
    ex = &astexpr->uexpr.exprindex;
    mc_printer_puts(apr->pdest, "(");
    mc_astprint_expression(apr, ex->left);
    if(ex->isdot)
    {
        mc_printer_puts(apr->pdest, ".");
        prevquot = apr->pdest->config.quotstring;
        apr->pdest->config.quotstring = false;
        mc_astprint_expression(apr, ex->index);
        apr->pdest->config.quotstring = prevquot;
    }
    else
    {
        mc_printer_puts(apr->pdest, "[");
        mc_astprint_expression(apr, ex->index);
        mc_printer_puts(apr->pdest, "]");
    }
    mc_printer_puts(apr->pdest, ")");
}

void mc_astprint_printassignexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprassign_t* ex;
    ex = &astexpr->uexpr.exprassign;
    mc_astprint_expression(apr, ex->dest);
    mc_printer_puts(apr->pdest, " = ");
    mc_astprint_expression(apr, ex->source);
}

void mc_astprint_printlogicalexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprlogical_t* ex;
    ex = &astexpr->uexpr.exprlogical;
    mc_astprint_expression(apr, ex->left);
    mc_printer_puts(apr->pdest, " ");
    mc_printer_puts(apr->pdest, mc_util_mathopstring(ex->op));
    mc_printer_puts(apr->pdest, " ");
    mc_astprint_expression(apr, ex->right);
}

void mc_astprint_printternaryexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprternary_t* ex;
    ex = &astexpr->uexpr.exprternary;
    mc_astprint_expression(apr, ex->tercond);
    mc_printer_puts(apr->pdest, " ? ");
    mc_astprint_expression(apr, ex->teriftrue);
    mc_printer_puts(apr->pdest, " : ");
    mc_astprint_expression(apr, ex->teriffalse);
}

void mc_astprint_printdefineexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprdefine_t* ex;
    ex = &astexpr->uexpr.exprdefine;
    if(ex->assignable)
    {
        mc_printer_puts(apr->pdest, "var ");
    }
    else
    {
        mc_printer_puts(apr->pdest, "const ");
    }
    mc_printer_puts(apr->pdest, ex->name->value);
    mc_printer_puts(apr->pdest, " = ");
    if(ex->value)
    {
        mc_astprint_expression(apr, ex->value);
    }
}

void mc_astprint_printifexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    size_t i;
    mcastifcase_t* ifcase;
    mcastexprstmtif_t* ex;
    ex = &astexpr->uexpr.exprifstmt;
    ifcase = (mcastifcase_t*)mc_ptrlist_get(ex->cases, 0);
    mc_printer_puts(apr->pdest, "if (");
    mc_astprint_expression(apr, ifcase->ifcond);
    mc_printer_puts(apr->pdest, ") ");
    mc_astprint_codeblock(apr, ifcase->consequence);
    for(i = 1; i < mc_ptrlist_count(ex->cases); i++)
    {
        mcastifcase_t* elifcase = (mcastifcase_t*)mc_ptrlist_get(ex->cases, i);
        mc_printer_puts(apr->pdest, " elif (");
        mc_astprint_expression(apr, elifcase->ifcond);
        mc_printer_puts(apr->pdest, ") ");
        mc_astprint_codeblock(apr, elifcase->consequence);
    }
    if(ex->alternative)
    {
        mc_printer_puts(apr->pdest, " else ");
        mc_astprint_codeblock(apr, ex->alternative);
    }
}

void mc_astprint_printwhileexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprstmtwhile_t* ex;
    ex = &astexpr->uexpr.exprwhileloopstmt;
    mc_printer_puts(apr->pdest, "while (");
    mc_astprint_expression(apr, ex->loopcond);
    mc_printer_puts(apr->pdest, ")");
    mc_astprint_codeblock(apr, ex->body);
}

void mc_astprint_printforclassicexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprstmtforloop_t* ex;
    ex = &astexpr->uexpr.exprforloopstmt;
    mc_printer_puts(apr->pdest, "for (");
    if(ex->init)
    {
        mc_astprint_expression(apr, ex->init);
        mc_printer_puts(apr->pdest, " ");
    }
    else
    {
        mc_printer_puts(apr->pdest, ";");
    }
    if(ex->loopcond)
    {
        mc_astprint_expression(apr, ex->loopcond);
        mc_printer_puts(apr->pdest, "; ");
    }
    else
    {
        mc_printer_puts(apr->pdest, ";");
    }
    if(ex->update)
    {
        mc_astprint_expression(apr, ex->loopcond);
    }
    mc_printer_puts(apr->pdest, ")");
    mc_astprint_codeblock(apr, ex->body);
}

void mc_astprint_printforeachexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprstmtforeach_t* ex;
    ex = &astexpr->uexpr.exprforeachloopstmt;
    mc_printer_puts(apr->pdest, "for (");
    mc_printer_printf(apr->pdest, "%s", ex->iterator->value);
    mc_printer_puts(apr->pdest, " in ");
    mc_astprint_expression(apr, ex->source);
    mc_printer_puts(apr->pdest, ")");
    mc_astprint_codeblock(apr, ex->body);
}

void mc_astprint_printimportexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprstmtimport_t* ex;
    ex = &astexpr->uexpr.exprimportstmt;
    mc_printer_printf(apr->pdest, "import \"%s\"", ex->path);
}

void mc_astprint_printrecoverexpr(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    mcastexprstmtrecover_t* ex;
    ex = &astexpr->uexpr.exprrecoverstmt;
    mc_printer_printf(apr->pdest, "recover (%s)", ex->errident->value);
    mc_astprint_codeblock(apr, ex->body);
}

void mc_astprint_expression(mcastprinter_t* apr, mcastexpression_t* astexpr)
{
    switch(astexpr->type)
    {
        case MC_EXPR_IDENT:
            {
                mcastident_t* ex;
                ex = astexpr->uexpr.exprident;
                mc_printer_puts(apr->pdest, ex->value);
            }
            break;
        case MC_EXPR_NUMBERLITERAL:
            {
                mcfloat_t fl;
                fl = astexpr->uexpr.exprlitnumber;
                mc_printer_printf(apr->pdest, "%1.17g", fl);
            }
            break;
        case MC_EXPR_BOOLLITERAL:
            {
                bool bl;
                bl = astexpr->uexpr.exprlitbool;
                mc_printer_printf(apr->pdest, "%s", bl ? "true" : "false");
            }
            break;
        case MC_EXPR_STRINGLITERAL:
            {
                mc_astprint_printstringliteral(apr, astexpr);
            }
            break;
        case MC_EXPR_NULLLITERAL:
            {
                mc_printer_puts(apr->pdest, "null");
            }
            break;
        case MC_EXPR_ARRAYLITERAL:
            {
                mc_astprint_printarrayliteral(apr, astexpr);
            }
            break;
        case MC_EXPR_MAPLITERAL:
            {
                mc_astprint_printmapliteral(apr, astexpr);
            }
            break;
        case MC_EXPR_PREFIX:
            {
                mc_astprint_printprefixexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_INFIX:
            {
                mc_astprint_printinfixexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_FUNCTIONLITERAL:
            {
                mc_astprint_printfuncliteral(apr, astexpr);
            }
            break;
        case MC_EXPR_CALL:
            {
                mc_astprint_printcall(apr, astexpr);
            }
            break;
        case MC_EXPR_INDEX:
            {
                mc_astprint_printindexexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_ASSIGN:
            {
                mc_astprint_printassignexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_LOGICAL:
            {
                mc_astprint_printlogicalexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_TERNARY:
            {
                mc_astprint_printternaryexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_STMTDEFINE:
            {
                mc_astprint_printdefineexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_STMTIF:
            {
                mc_astprint_printifexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_STMTRETURN:
            {
                mcastexpression_t* ex;
                ex = astexpr->uexpr.exprreturnvalue;
                if(ex)
                {
                    mc_printer_puts(apr->pdest, "return ");
                    mc_astprint_expression(apr, ex);
                    mc_printer_puts(apr->pdest, ";");
                }
                else
                {
                    mc_printer_puts(apr->pdest, "return;");
                }
            }
            break;
        case MC_EXPR_STMTEXPRESSION:
            {
                mcastexpression_t* ex;
                ex = astexpr->uexpr.exprexpression;
                if(ex)
                {
                    mc_astprint_expression(apr, ex);
                }
            }
            break;
        case MC_EXPR_STMTLOOPWHILE:
            {
                mc_astprint_printwhileexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_STMTLOOPFORCLASSIC:
            {
                mc_astprint_printforclassicexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_STMTLOOPFOREACH:
            {
                mc_astprint_printforeachexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_STMTBLOCK:
            {
                mcastcodeblock_t* ex;
                ex = astexpr->uexpr.exprblockstmt;
                mc_astprint_codeblock(apr, ex);
            }
            break;
        case MC_EXPR_STMTBREAK:
            {
                mc_printer_puts(apr->pdest, "break");
            }
            break;
        case MC_EXPR_STMTCONTINUE:
            {
                mc_printer_puts(apr->pdest, "continue");
            }
            break;
        case MC_EXPR_STMTIMPORT:
            {
                mc_astprint_printimportexpr(apr, astexpr);
            }
            break;
        case MC_EXPR_NONE:
            {
                mc_printer_puts(apr->pdest, "MC_EXPR_NONE");
            }
            break;
        case MC_EXPR_STMTRECOVER:
            {
                mc_astprint_printrecoverexpr(apr, astexpr);
            }
            break;
        default:
            break;
    }
}

void mc_astprint_codeblock(mcastprinter_t* apr, mcastcodeblock_t* blockexpr)
{
    size_t i;
    size_t cnt;
    mcastexpression_t* istmt;
    cnt = mc_ptrlist_count(blockexpr->statements);
    mc_printer_puts(apr->pdest, "{ ");
    for(i = 0; i < cnt; i++)
    {
        istmt = (mcastexpression_t*)mc_ptrlist_get(blockexpr->statements, i);
        mc_astprint_expression(apr, istmt);
        mc_printer_puts(apr->pdest, "\n");
    }
    mc_printer_puts(apr->pdest, " }");
}

