
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
    mcastexpression_t* expr;
    count = mc_ptrlist_count(statements);
    for(i = 0; i < count; i++)
    {
        expr = (mcastexpression_t*)mc_ptrlist_get(statements, i);
        mc_astprint_expression(apr, expr);
        if(i < (count - 1))
        {
            mc_printer_puts(apr->pdest, "\n");
        }
    }
}

void mc_astprint_printfuncliteral(mcastprinter_t* apr, mcastexpression_t* expr)
{
    size_t i;
    mcastfuncparam_t* param;
    mcastliteralfunction_t* fn;
    fn = &expr->uexpr.exprlitfunction;
    if(apr->pseudolisp)
    {
        mc_printer_printf(apr->pdest, "(deffunction '(");
    }
    else
    {
        mc_printer_puts(apr->pdest, "function(");
    }
    for(i = 0; i < mc_ptrlist_count(fn->funcparamlist); i++)
    {
        param = (mcastfuncparam_t*)mc_ptrlist_get(fn->funcparamlist, i);
        mc_printer_puts(apr->pdest, param->ident->value);
        if(i < (mc_ptrlist_count(fn->funcparamlist) - 1))
        {
            mc_printer_puts(apr->pdest, ", ");
        }
    }
    mc_printer_puts(apr->pdest, ") ");
    mc_astprint_codeblock(apr, fn->body);

}

void mc_astprint_expression(mcastprinter_t* apr, mcastexpression_t* expr)
{
    bool prevquot;
    switch(expr->type)
    {
        case MC_EXPR_IDENT:
            {
                mc_printer_puts(apr->pdest, expr->uexpr.exprident->value);
            }
            break;
        case MC_EXPR_NUMBERLITERAL:
            {
                mc_printer_printf(apr->pdest, "%1.17g", expr->uexpr.exprlitnumber);
            }
            break;
        case MC_EXPR_BOOLLITERAL:
            {
                mc_printer_printf(apr->pdest, "%s", expr->uexpr.exprlitbool ? "true" : "false");
            }
            break;
        case MC_EXPR_STRINGLITERAL:
            {
                if(apr->pdest->config.quotstring)
                {
                    mc_printer_printescapedstring(apr->pdest, expr->uexpr.exprlitstring.data, expr->uexpr.exprlitstring.length);
                }
                else
                {
                    mc_printer_putlen(apr->pdest, expr->uexpr.exprlitstring.data, expr->uexpr.exprlitstring.length);
                }
            }
            break;
        case MC_EXPR_NULLLITERAL:
            {
                mc_printer_puts(apr->pdest, "null");
            }
            break;
        case MC_EXPR_ARRAYLITERAL:
            {
                size_t i;
                mcastexpression_t* arrexpr;
                mc_printer_puts(apr->pdest, "[");
                for(i = 0; i < mc_ptrlist_count(expr->uexpr.exprlitarray.litarritems); i++)
                {
                    arrexpr = (mcastexpression_t*)mc_ptrlist_get(expr->uexpr.exprlitarray.litarritems, i);
                    mc_astprint_expression(apr, arrexpr);
                    if(i < (mc_ptrlist_count(expr->uexpr.exprlitarray.litarritems) - 1))
                    {
                        mc_printer_puts(apr->pdest, ", ");
                    }
                }
                mc_printer_puts(apr->pdest, "]");
            }
            break;
        case MC_EXPR_MAPLITERAL:
            {
                size_t i;
                mcastexpression_t* keyexpr;
                mcastexpression_t* valexpr;
                mcastliteralmap_t* map;
                map = &expr->uexpr.exprlitmap;
                mc_printer_puts(apr->pdest, "{");
                for(i = 0; i < mc_ptrlist_count(map->keys); i++)
                {
                    keyexpr = (mcastexpression_t*)mc_ptrlist_get(map->keys, i);
                    valexpr = (mcastexpression_t*)mc_ptrlist_get(map->values, i);
                    mc_astprint_expression(apr, keyexpr);
                    mc_printer_puts(apr->pdest, " : ");
                    mc_astprint_expression(apr, valexpr);
                    if(i < (mc_ptrlist_count(map->keys) - 1))
                    {
                        mc_printer_puts(apr->pdest, ", ");
                    }
                }
                mc_printer_puts(apr->pdest, "}");
            }
            break;
        case MC_EXPR_PREFIX:
            {
                mc_printer_puts(apr->pdest, "(");
                mc_printer_puts(apr->pdest, mc_util_mathopstring(expr->uexpr.exprinfix.op));
                mc_astprint_expression(apr, expr->uexpr.exprprefix.right);
                mc_printer_puts(apr->pdest, ")");
            }
            break;
        case MC_EXPR_INFIX:
            {
                mc_printer_puts(apr->pdest, "(");
                mc_astprint_expression(apr, expr->uexpr.exprinfix.left);
                mc_printer_puts(apr->pdest, " ");
                mc_printer_puts(apr->pdest, mc_util_mathopstring(expr->uexpr.exprinfix.op));
                mc_printer_puts(apr->pdest, " ");
                mc_astprint_expression(apr, expr->uexpr.exprinfix.right);
                mc_printer_puts(apr->pdest, ")");
            }
            break;
        case MC_EXPR_FUNCTIONLITERAL:
            {
                mc_astprint_printfuncliteral(apr, expr);
            }
            break;
        case MC_EXPR_CALL:
            {
                size_t i;
                mcastexprcall_t* ce;
                mcastexpression_t* arg;
                ce = &expr->uexpr.exprcall;
                mc_astprint_expression(apr, ce->function);
                mc_printer_puts(apr->pdest, "(");
                for(i = 0; i < mc_ptrlist_count(ce->args); i++)
                {
                    arg = (mcastexpression_t*)mc_ptrlist_get(ce->args, i);
                    mc_astprint_expression(apr, arg);
                    if(i < (mc_ptrlist_count(ce->args) - 1))
                    {
                        mc_printer_puts(apr->pdest, ", ");
                    }
                }
                mc_printer_puts(apr->pdest, ")");
            }
            break;
        case MC_EXPR_INDEX:
            {
                mc_printer_puts(apr->pdest, "(");
                mc_astprint_expression(apr, expr->uexpr.exprindex.left);
                if(expr->uexpr.exprindex.isdot)
                {
                    mc_printer_puts(apr->pdest, ".");
                    prevquot = apr->pdest->config.quotstring;
                    apr->pdest->config.quotstring = false;
                    mc_astprint_expression(apr, expr->uexpr.exprindex.index);
                    apr->pdest->config.quotstring = prevquot;
                }
                else
                {
                    mc_printer_puts(apr->pdest, "[");
                    mc_astprint_expression(apr, expr->uexpr.exprindex.index);
                    mc_printer_puts(apr->pdest, "]");
                }
                mc_printer_puts(apr->pdest, ")");
            }
            break;
        case MC_EXPR_ASSIGN:
            {
                mc_astprint_expression(apr, expr->uexpr.exprassign.dest);
                mc_printer_puts(apr->pdest, " = ");
                mc_astprint_expression(apr, expr->uexpr.exprassign.source);
            }
            break;
        case MC_EXPR_LOGICAL:
            {
                mc_astprint_expression(apr, expr->uexpr.exprlogical.left);
                mc_printer_puts(apr->pdest, " ");
                mc_printer_puts(apr->pdest, mc_util_mathopstring(expr->uexpr.exprinfix.op));
                mc_printer_puts(apr->pdest, " ");
                mc_astprint_expression(apr, expr->uexpr.exprlogical.right);
            }
            break;
        case MC_EXPR_TERNARY:
            {
                mc_astprint_expression(apr, expr->uexpr.exprternary.tercond);
                mc_printer_puts(apr->pdest, " ? ");
                mc_astprint_expression(apr, expr->uexpr.exprternary.teriftrue);
                mc_printer_puts(apr->pdest, " : ");
                mc_astprint_expression(apr, expr->uexpr.exprternary.teriffalse);
            }
            break;
        case MC_EXPR_STMTDEFINE:
            {
                mcastexprdefine_t* defstmt;
                defstmt = &expr->uexpr.exprdefine;
                if(expr->uexpr.exprdefine.assignable)
                {
                    mc_printer_puts(apr->pdest, "var ");
                }
                else
                {
                    mc_printer_puts(apr->pdest, "const ");
                }
                mc_printer_puts(apr->pdest, defstmt->name->value);
                mc_printer_puts(apr->pdest, " = ");
                if(defstmt->value)
                {
                    mc_astprint_expression(apr, defstmt->value);
                }
            }
            break;
        case MC_EXPR_STMTIF:
            {
                size_t i;
                mcastifcase_t* ifcase;
                ifcase = (mcastifcase_t*)mc_ptrlist_get(expr->uexpr.exprifstmt.cases, 0);
                mc_printer_puts(apr->pdest, "if (");
                mc_astprint_expression(apr, ifcase->ifcond);
                mc_printer_puts(apr->pdest, ") ");
                mc_astprint_codeblock(apr, ifcase->consequence);
                for(i = 1; i < mc_ptrlist_count(expr->uexpr.exprifstmt.cases); i++)
                {
                    mcastifcase_t* elifcase = (mcastifcase_t*)mc_ptrlist_get(expr->uexpr.exprifstmt.cases, i);
                    mc_printer_puts(apr->pdest, " elif (");
                    mc_astprint_expression(apr, elifcase->ifcond);
                    mc_printer_puts(apr->pdest, ") ");
                    mc_astprint_codeblock(apr, elifcase->consequence);
                }
                if(expr->uexpr.exprifstmt.alternative)
                {
                    mc_printer_puts(apr->pdest, " else ");
                    mc_astprint_codeblock(apr, expr->uexpr.exprifstmt.alternative);
                }
            }
            break;
        case MC_EXPR_STMTRETURN:
            {
                mc_printer_puts(apr->pdest, "return ");
                if(expr->uexpr.exprreturnvalue)
                {
                    mc_astprint_expression(apr, expr->uexpr.exprreturnvalue);
                }
            }
            break;
        case MC_EXPR_STMTEXPRESSION:
            {
                if(expr->uexpr.exprexpression)
                {
                    mc_astprint_expression(apr, expr->uexpr.exprexpression);
                }
            }
            break;
        case MC_EXPR_STMTLOOPWHILE:
            {
                mc_printer_puts(apr->pdest, "while (");
                mc_astprint_expression(apr, expr->uexpr.exprwhileloopstmt.loopcond);
                mc_printer_puts(apr->pdest, ")");
                mc_astprint_codeblock(apr, expr->uexpr.exprwhileloopstmt.body);
            }
            break;
        case MC_EXPR_STMTLOOPFORCLASSIC:
            {
                mc_printer_puts(apr->pdest, "for (");
                if(expr->uexpr.exprforloopstmt.init)
                {
                    mc_astprint_expression(apr, expr->uexpr.exprforloopstmt.init);
                    mc_printer_puts(apr->pdest, " ");
                }
                else
                {
                    mc_printer_puts(apr->pdest, ";");
                }
                if(expr->uexpr.exprforloopstmt.loopcond)
                {
                    mc_astprint_expression(apr, expr->uexpr.exprforloopstmt.loopcond);
                    mc_printer_puts(apr->pdest, "; ");
                }
                else
                {
                    mc_printer_puts(apr->pdest, ";");
                }
                if(expr->uexpr.exprforloopstmt.update)
                {
                    mc_astprint_expression(apr, expr->uexpr.exprforloopstmt.loopcond);
                }
                mc_printer_puts(apr->pdest, ")");
                mc_astprint_codeblock(apr, expr->uexpr.exprforloopstmt.body);
            }
            break;
        case MC_EXPR_STMTLOOPFOREACH:
            {
                mc_printer_puts(apr->pdest, "for (");
                mc_printer_printf(apr->pdest, "%s", expr->uexpr.exprforeachloopstmt.iterator->value);
                mc_printer_puts(apr->pdest, " in ");
                mc_astprint_expression(apr, expr->uexpr.exprforeachloopstmt.source);
                mc_printer_puts(apr->pdest, ")");
                mc_astprint_codeblock(apr, expr->uexpr.exprforeachloopstmt.body);
            }
            break;
        case MC_EXPR_STMTBLOCK:
            {
                mc_astprint_codeblock(apr, expr->uexpr.exprblockstmt);
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
                mc_printer_printf(apr->pdest, "import \"%s\"", expr->uexpr.exprimportstmt.path);
            }
            break;
        case MC_EXPR_NONE:
            {
                mc_printer_puts(apr->pdest, "MC_EXPR_NONE");
            }
            break;
        case MC_EXPR_STMTRECOVER:
            {
                mc_printer_printf(apr->pdest, "recover (%s)", expr->uexpr.exprrecoverstmt.errident->value);
                mc_astprint_codeblock(apr, expr->uexpr.exprrecoverstmt.body);
            }
            break;
        default:
            break;
    }
}

void mc_astprint_codeblock(mcastprinter_t* apr, mcastcodeblock_t* expr)
{
    size_t i;
    mcastexpression_t* istmt;
    mc_printer_puts(apr->pdest, "{ ");
    for(i = 0; i < mc_ptrlist_count(expr->statements); i++)
    {
        istmt = (mcastexpression_t*)mc_ptrlist_get(expr->statements, i);
        mc_astprint_expression(apr, istmt);
        mc_printer_puts(apr->pdest, "\n");
    }
    mc_printer_puts(apr->pdest, " }");
}
