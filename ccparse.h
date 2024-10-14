
mcastprecedence_t mc_parser_getprecedence(mcasttoktype_t tk)
{
    switch(tk)
    {
        case MC_TOK_EQ:
        case MC_TOK_NOTEQ:
            return MC_ASTPREC_EQUALS;
        case MC_TOK_LT:
        case MC_TOK_LTE:
        case MC_TOK_GT:
        case MC_TOK_GTE:
            return MC_ASTPREC_LESSGREATER;
        case MC_TOK_PLUS:
        case MC_TOK_UNARYMINUS:
        case MC_TOK_UNARYBINNOT:
            return MC_ASTPREC_SUM;
        case MC_TOK_SLASH:
        case MC_TOK_ASTERISK:
        case MC_TOK_PERCENT:
            return MC_ASTPREC_PRODUCT;
        case MC_TOK_LPAREN:
        case MC_TOK_LBRACKET:
            return MC_ASTPREC_POSTFIX;
        case MC_TOK_ASSIGN:
        case MC_TOK_ASSIGNPLUS:
        case MC_TOK_ASSIGNMINUS:
        case MC_TOK_ASSIGNASTERISK:
        case MC_TOK_ASSIGNSLASH:
        case MC_TOK_ASSIGNPERCENT:
        case MC_TOK_ASSIGNBINAND:
        case MC_TOK_ASSIGNBINOR:
        case MC_TOK_ASSIGNBINXOR:
        case MC_TOK_ASSIGNLSHIFT:
        case MC_TOK_ASSIGNRSHIFT:
            return MC_ASTPREC_ASSIGN;
        case MC_TOK_DOT:
            return MC_ASTPREC_POSTFIX;
        case MC_TOK_AND:
            return MC_ASTPREC_LOGICALAND;
        case MC_TOK_OR:
            return MC_ASTPREC_LOGICALOR;
        case MC_TOK_BINOR:
            return MC_ASTPREC_BINOR;
        case MC_TOK_BINXOR:
            return MC_ASTPREC_BINXOR;
        case MC_TOK_BINAND:
            return MC_ASTPREC_BINAND;
        case MC_TOK_LSHIFT:
        case MC_TOK_RSHIFT:
            return MC_ASTPREC_SHIFT;
        case MC_TOK_QUESTION:
            return MC_ASTPREC_TERNARY;
        case MC_TOK_PLUSPLUS:
        case MC_TOK_MINUSMINUS:
            return MC_ASTPREC_INCDEC;
        default:
            break;
    }
    return MC_ASTPREC_LOWEST;
}

mcastmathoptype_t mc_parser_tokentomathop(mcasttoktype_t tk)
{
    switch(tk)
    {
        case MC_TOK_ASSIGN:
            return MC_MATHOP_ASSIGN;
        case MC_TOK_PLUS:
            return MC_MATHOP_PLUS;
        case MC_TOK_UNARYMINUS:
            return MC_MATHOP_MINUS;
        case MC_TOK_UNARYBINNOT:
            return MC_MATHOP_BINNOT;
        case MC_TOK_BANG:
            return MC_MATHOP_BANG;
        case MC_TOK_ASTERISK:
            return MC_MATHOP_ASTERISK;
        case MC_TOK_SLASH:
            return MC_MATHOP_SLASH;
        case MC_TOK_LT:
            return MC_MATHOP_LT;
        case MC_TOK_LTE:
            return MC_MATHOP_LTE;
        case MC_TOK_GT:
            return MC_MATHOP_GT;
        case MC_TOK_GTE:
            return MC_MATHOP_GTE;
        case MC_TOK_EQ:
            return MC_MATHOP_EQ;
        case MC_TOK_NOTEQ:
            return MC_MATHOP_NOTEQ;
        case MC_TOK_PERCENT:
            return MC_MATHOP_MODULUS;
        case MC_TOK_AND:
            return MC_MATHOP_LOGICALAND;
        case MC_TOK_OR:
            return MC_MATHOP_LOGICALOR;
        case MC_TOK_ASSIGNPLUS:
            return MC_MATHOP_PLUS;
        case MC_TOK_ASSIGNMINUS:
            return MC_MATHOP_MINUS;
        case MC_TOK_ASSIGNASTERISK:
            return MC_MATHOP_ASTERISK;
        case MC_TOK_ASSIGNSLASH:
            return MC_MATHOP_SLASH;
        case MC_TOK_ASSIGNPERCENT:
            return MC_MATHOP_MODULUS;
        case MC_TOK_ASSIGNBINAND:
            return MC_MATHOP_BINAND;
        case MC_TOK_ASSIGNBINOR:
            return MC_MATHOP_BINOR;
        case MC_TOK_ASSIGNBINXOR:
            return MC_MATHOP_BINXOR;
        case MC_TOK_ASSIGNLSHIFT:
            return MC_MATHOP_LSHIFT;
        case MC_TOK_ASSIGNRSHIFT:
            return MC_MATHOP_RSHIFT;
        case MC_TOK_BINAND:
            return MC_MATHOP_BINAND;
        case MC_TOK_BINOR:
            return MC_MATHOP_BINOR;
        case MC_TOK_BINXOR:
            return MC_MATHOP_BINXOR;
        case MC_TOK_LSHIFT:
            return MC_MATHOP_LSHIFT;
        case MC_TOK_RSHIFT:
            return MC_MATHOP_RSHIFT;
        case MC_TOK_PLUSPLUS:
            return MC_MATHOP_PLUS;
        case MC_TOK_MINUSMINUS:
            return MC_MATHOP_MINUS;
        default:
            {
                MC_ASSERT(false);
            }
            break;
    }
    return MC_MATHOP_NONE;
}

char mc_parser_getescapechar(char c)
{
    switch(c)
    {
        case '\"':
            return '\"';
        case '\\':
            return '\\';
        case '/':
            return '/';
        case 'b':
            return '\b';
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case '0':
            return '\0';
        default:
            break;
    }
    return c;
}

char* mc_parser_processandcopystring(mcstate_t* state, const char* input, size_t len)
{
    size_t ini;
    size_t outi;
    char* output;
    output = (char*)mc_allocator_malloc(state, len + 1);
    if(!output)
    {
        return NULL;
    }
    ini = 0;
    outi = 0;
    while(input[ini] != '\0' && ini < len)
    {
        if(input[ini] == '\\')
        {
            ini++;
            output[outi] = mc_parser_getescapechar(input[ini]);
            if(output[outi] < 0)
            {
                goto error;
            }
        }
        else
        {
            output[outi] = input[ini];
        }
        outi++;
        ini++;
    }
    output[outi] = '\0';
    return output;
error:
    mc_memory_free(output);
    return NULL;
}

mcastparser_t* mc_astparser_make(mcstate_t* state, mcconfig_t* config, mcerrlist_t* errors)
{
    mcastparser_t* parser;
    parser = (mcastparser_t*)mc_allocator_malloc(state, sizeof(mcastparser_t));
    if(!parser)
    {
        return NULL;
    }
    memset(parser, 0, sizeof(mcastparser_t));
    parser->pstate = state;
    parser->config = config;
    parser->errors = errors;
    {
        parser->rightassocfuncs[MC_TOK_IDENT] = mc_parser_parseident;
        parser->rightassocfuncs[MC_TOK_NUMBER] = mc_parser_parseliteralnumber;
        parser->rightassocfuncs[MC_TOK_TRUE] = mc_parser_parseliteralbool;
        parser->rightassocfuncs[MC_TOK_FALSE] = mc_parser_parseliteralbool;
        parser->rightassocfuncs[MC_TOK_STRING] = mc_parser_parseliteralstring;
        parser->rightassocfuncs[MC_TOK_TEMPLATESTRING] = mc_parser_parseliteraltemplatestring;
        parser->rightassocfuncs[MC_TOK_NULL] = mc_parser_parseliteralnull;
        parser->rightassocfuncs[MC_TOK_BANG] = mc_parser_parseprefixexpr;
        parser->rightassocfuncs[MC_TOK_UNARYMINUS] = mc_parser_parseprefixexpr;
        parser->rightassocfuncs[MC_TOK_UNARYBINNOT] = mc_parser_parseprefixexpr;
        parser->rightassocfuncs[MC_TOK_LPAREN] = mc_parser_parsegroupedexpr;
        parser->rightassocfuncs[MC_TOK_FUNCTION] = mc_parser_parseliteralfunction;
        parser->rightassocfuncs[MC_TOK_LBRACKET] = mc_parser_parseliteralarray;
        parser->rightassocfuncs[MC_TOK_LBRACE] = mc_parser_parseliteralmap;
        parser->rightassocfuncs[MC_TOK_PLUSPLUS] = mc_parser_parseincdecprefixexpr;
        parser->rightassocfuncs[MC_TOK_MINUSMINUS] = mc_parser_parseincdecprefixexpr;
        #if 0
        parser->rightassocfuncs[MC_TOK_IF] = mc_parser_parseifstmt;
        #endif
    }
    {
        parser->leftassocfuncs[MC_TOK_PLUS] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_UNARYMINUS] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_SLASH] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_ASTERISK] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_PERCENT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_EQ] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_NOTEQ] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_LT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_LTE] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_GT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_GTE] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_LPAREN] = mc_parser_parsecallexpr;
        parser->leftassocfuncs[MC_TOK_LBRACKET] = mc_parser_parseindexexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGN] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNPLUS] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNMINUS] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNSLASH] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNASTERISK] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNPERCENT] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNBINAND] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNBINOR] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNBINXOR] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNLSHIFT] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_ASSIGNRSHIFT] = mc_parser_parseassignexpr;
        parser->leftassocfuncs[MC_TOK_DOT] = mc_parser_parsedotexpression;
        parser->leftassocfuncs[MC_TOK_AND] = mc_parser_parselogicalexpr;
        parser->leftassocfuncs[MC_TOK_OR] = mc_parser_parselogicalexpr;
        parser->leftassocfuncs[MC_TOK_BINAND] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_BINOR] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_BINXOR] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_LSHIFT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_RSHIFT] = mc_parser_parseinfixexpr;
        parser->leftassocfuncs[MC_TOK_QUESTION] = mc_parser_parseternaryexpr;
        parser->leftassocfuncs[MC_TOK_PLUSPLUS] = mc_parser_parseincdecpostfixexpr;
        parser->leftassocfuncs[MC_TOK_MINUSMINUS] = mc_parser_parseincdecpostfixexpr;
    }
    parser->depth = 0;
    return parser;
}

void mc_astparser_destroy(mcastparser_t* parser)
{
    if(!parser)
    {
        return;
    }
    mc_memory_free(parser);
}

mcptrlist_t* mc_astparser_parseall(mcastparser_t* parser, const char* input, mcastcompiledfile_t* file)
{
    bool ok;
    mcastexpression_t* expr;
    mcptrlist_t* statements;
    parser->depth = 0;
    ok = mc_lexer_init(&parser->lexer, parser->pstate, parser->errors, input, file);
    if(!ok)
    {
        return NULL;
    }
    mc_lexer_nexttoken(&parser->lexer);
    mc_lexer_nexttoken(&parser->lexer);
    statements = mc_ptrlist_make(parser->pstate, sizeof(void*), true);
    if(!statements)
    {
        return NULL;
    }
    while(!mc_lexer_currtokenis(&parser->lexer, MC_TOK_EOF))
    {
        if(mc_lexer_currtokenis(&parser->lexer, MC_TOK_SEMICOLON))
        {
            mc_lexer_nexttoken(&parser->lexer);
            continue;
        }
        expr = mc_astparser_parsestatement(parser);
        if(!expr)
        {
            goto err;
        }
        ok = mc_ptrlist_push(statements, expr);
        if(!ok)
        {
            mc_astexpr_destroy(expr);
            goto err;
        }
    }
    if(parser->errors->count > 0)
    {
        goto err;
    }
    return statements;
err:
    mc_ptrlist_destroy(statements, (mcitemdestroyfn_t)mc_astexpr_destroy);
    return NULL;
}

mcastexpression_t* mc_astparser_parsestatement(mcastparser_t* p)
{
    mcastlocation_t pos;
    mcastexpression_t* res;
    pos = p->lexer.currtoken.pos;
    res = NULL;
    switch(p->lexer.currtoken.toktype)
    {
        case MC_TOK_VAR:
        case MC_TOK_CONST:
            {
                res = mc_parser_parsevarletstmt(p);
            }
            break;
        case MC_TOK_IF:
            {
                res = mc_parser_parseifstmt(p);
            }
            break;
        case MC_TOK_RETURN:
            {
                res = mc_parser_parsereturnstmt(p);
            }
            break;
        case MC_TOK_WHILE:
            {
                res = mc_parser_parseloopwhilestmt(p);
            }
            break;
        case MC_TOK_BREAK:
            {
                res = mc_parser_parsebreakstmt(p);
            }
            break;
        case MC_TOK_FOR:
            {
                res = mc_parser_parseloopforstmt(p);
            }
            break;
        case MC_TOK_FUNCTION:
            {
                if(mc_lexer_peektokenis(&p->lexer, MC_TOK_IDENT))
                {
                    res = mc_parser_parsefunctionstmt(p);
                }
                else
                {
                    res = mc_parser_parseexprstmt(p);
                }
            }
            break;
        case MC_TOK_LBRACE:
            {
                if(p->config->replmode && p->depth == 0)
                {
                    res = mc_parser_parseexprstmt(p);
                }
                else
                {
                    res = mc_parser_parseblockstmt(p);
                }
            }
            break;
        case MC_TOK_CONTINUE:
            {
                res = mc_parser_parsecontinuestmt(p);
            }
            break;
        case MC_TOK_IMPORT:
            {
                res = mc_parser_parseimportstmt(p);
            }
            break;
        case MC_TOK_RECOVER:
            {
                res = mc_parser_parserecoverstmt(p);
            }
            break;
        default:
            {
                res = mc_parser_parseexprstmt(p);
            }
            break;
    }
    if(res)
    {
        res->pos = pos;
    }
    return res;
}

mcastexpression_t* mc_parser_parsevarletstmt(mcastparser_t* p)
{
    bool assignable;
    mcastexprident_t* nameident;
    mcastexpression_t* value;
    mcastexpression_t* res;
    nameident = NULL;
    value = NULL;
    assignable = mc_lexer_currtokenis(&p->lexer, MC_TOK_VAR);
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IDENT))
    {
        goto err;
    }
    nameident = mc_astident_make(p->pstate, p->lexer.currtoken);
    if(!nameident)
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    #if 0
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_ASSIGN))
    #else
        if(!mc_lexer_currtokenis(&p->lexer, MC_TOK_ASSIGN))
    #endif
    {
        value = mc_astexpr_makeliteralnull(p->pstate);
        goto finish;
    }
    mc_lexer_nexttoken(&p->lexer);
    value = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!value)
    {
        goto err;
    }
    if(value->exprtype == MC_EXPR_FUNCTIONLITERAL)
    {
        value->uexpr.exprlitfunction.name = mc_util_strdup(p->pstate, nameident->value);
        if(!value->uexpr.exprlitfunction.name)
        {
            goto err;
        }
    }
    finish:
    res = mc_astexpr_makedefineexpr(p->pstate, nameident, value, assignable);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astexpr_destroy(value);
    mc_astident_destroy(nameident);
    return NULL;
}

mcastexpression_t* mc_parser_parseifstmt(mcastparser_t* p)
{
    bool ok;
    mcptrlist_t* cases;
    mcastexprifcase_t* cond;
    mcastexprifcase_t* elif;
    mcastexprcodeblock_t* alternative;
    mcastexpression_t* res;
    cases = NULL;
    alternative = NULL;
    cases = mc_ptrlist_make(p->pstate, sizeof(void*), true);
    if(!cases)
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    cond = mc_astifcase_make(p->pstate, NULL, NULL);
    if(!cond)
    {
        goto err;
    }
    ok = mc_ptrlist_push(cases, cond);
    if(!ok)
    {
        mc_astifcase_destroy(cond);
        goto err;
    }
    cond->ifcond = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!cond->ifcond)
    {
        goto err;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    cond->consequence = mc_parser_parsecodeblock(p);
    if(!cond->consequence)
    {
        goto err;
    }
    while(mc_lexer_currtokenis(&p->lexer, MC_TOK_ELSE))
    {
        mc_lexer_nexttoken(&p->lexer);
        if(mc_lexer_currtokenis(&p->lexer, MC_TOK_IF))
        {
            mc_lexer_nexttoken(&p->lexer);
            if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
            {
                goto err;
            }
            mc_lexer_nexttoken(&p->lexer);
            elif = mc_astifcase_make(p->pstate, NULL, NULL);
            if(!elif)
            {
                goto err;
            }
            ok = mc_ptrlist_push(cases, elif);
            if(!ok)
            {
                mc_astifcase_destroy(elif);
                goto err;
            }
            elif->ifcond = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!elif->ifcond)
            {
                goto err;
            }
            if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
            {
                goto err;
            }
            mc_lexer_nexttoken(&p->lexer);
            elif->consequence = mc_parser_parsecodeblock(p);
            if(!elif->consequence)
            {
                goto err;
            }
        }
        else
        {
            alternative = mc_parser_parsecodeblock(p);
            if(!alternative)
            {
                goto err;
            }
        }
    }
    res = mc_astexpr_makeifexpr(p->pstate, cases, alternative);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_ptrlist_destroy(cases, (mcitemdestroyfn_t)mc_astifcase_destroy);
    mc_astcodeblock_destroy(alternative);
    return NULL;
}

mcastexpression_t* mc_parser_parsereturnstmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* expr;
    expr = NULL;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_currtokenis(&p->lexer, MC_TOK_SEMICOLON) && !mc_lexer_currtokenis(&p->lexer, MC_TOK_RBRACE) && !mc_lexer_currtokenis(&p->lexer, MC_TOK_EOF))
    {
        expr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
        if(!expr)
        {
            return NULL;
        }
    }
    res = mc_astexpr_makereturnexpr(p->pstate, expr);
    if(!res)
    {
        mc_astexpr_destroy(expr);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseexprstmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* expr;
    expr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!expr)
    {
        return NULL;
    }
    if(expr && (!p->config->replmode || p->depth > 0))
    {
        #if 0
        /* this is actually completely unnecessary */
        if(expr->exprtype != MC_EXPR_ASSIGN && expr->exprtype != MC_EXPR_CALL)
        {
            mc_errlist_addf(p->errors, MC_ERROR_PARSING, expr->pos, "only assignments and function calls can be expression statements");
            mc_astexpr_destroy(expr);
            return NULL;
        }
        #endif
    }
    res = mc_astexpr_makeexprstmt(p->pstate, expr);
    if(!res)
    {
        mc_astexpr_destroy(expr);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseloopwhilestmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* test;
    mcastexprcodeblock_t* body;
    test = NULL;
    body = NULL;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    test = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!test)
    {
        goto err;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    body = mc_parser_parsecodeblock(p);
    if(!body)
    {
        goto err;
    }
    res = mc_astexpr_makewhileexpr(p->pstate, test, body);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astcodeblock_destroy(body);
    mc_astexpr_destroy(test);
    return NULL;
}

mcastexpression_t* mc_parser_parsebreakstmt(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    return mc_astexpr_makebreakexpr(p->pstate);
}

mcastexpression_t* mc_parser_parsecontinuestmt(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    return mc_astexpr_makecontinueexpr(p->pstate);
}

mcastexpression_t* mc_parser_parseblockstmt(mcastparser_t* p)
{
    mcastexprcodeblock_t* block;
    mcastexpression_t* res;
    block = mc_parser_parsecodeblock(p);
    if(!block)
    {
        return NULL;
    }
    res = mc_astexpr_makeblockexpr(p->pstate, block);
    if(!res)
    {
        mc_astcodeblock_destroy(block);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseimportstmt(mcastparser_t* p)
{
    char* processedname;
    mcastexpression_t* res;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_STRING))
    {
        return NULL;
    }
    processedname = mc_parser_processandcopystring(p->pstate, p->lexer.currtoken.tokstrdata, p->lexer.currtoken.tokstrlen);
    if(!processedname)
    {
        mc_errlist_addf(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "error when parsing module name");
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    res = mc_astexpr_makeimportexpr(p->pstate, processedname);
    if(!res)
    {
        mc_memory_free(processedname);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parserecoverstmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexprident_t* eid;
    mcastexprcodeblock_t* body;
    eid = NULL;
    body = NULL;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IDENT))
    {
        return NULL;
    }
    eid = mc_astident_make(p->pstate, p->lexer.currtoken);
    if(!eid)
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    body = mc_parser_parsecodeblock(p);
    if(!body)
    {
        goto err;
    }
    res = mc_astexpr_makerecoverexpr(p->pstate, eid, body);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astcodeblock_destroy(body);
    mc_astident_destroy(eid);
    return NULL;
}

mcastexpression_t* mc_parser_parseloopforstmt(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(mc_lexer_currtokenis(&p->lexer, MC_TOK_IDENT) && mc_lexer_peektokenis(&p->lexer, MC_TOK_IN))
    {
        return mc_parser_parseloopforeachstmt(p);
    }
    return mc_parser_parseloopforclassicstmt(p);
}

mcastexpression_t* mc_parser_parseloopforeachstmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* source;
    mcastexprcodeblock_t* body;
    mcastexprident_t* iteratorident;
    source = NULL;
    body = NULL;
    iteratorident = NULL;
    iteratorident = mc_astident_make(p->pstate, p->lexer.currtoken);
    if(!iteratorident)
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    source = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!source)
    {
        goto err;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    body = mc_parser_parsecodeblock(p);
    if(!body)
    {
        goto err;
    }
    res = mc_astexpr_makeforeachexpr(p->pstate, iteratorident, source, body);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astcodeblock_destroy(body);
    mc_astident_destroy(iteratorident);
    mc_astexpr_destroy(source);
    return NULL;
}

mcastexpression_t* mc_parser_parseloopforclassicstmt(mcastparser_t* p)
{
    mcastexpression_t* res;
    mcastexpression_t* init;
    mcastexpression_t* test;
    mcastexpression_t* update;
    mcastexprcodeblock_t* body;
    init = NULL;
    test = NULL;
    update = NULL;
    body = NULL;
    if(!mc_lexer_currtokenis(&p->lexer, MC_TOK_SEMICOLON))
    {
        init = mc_astparser_parsestatement(p);
        if(!init)
        {
            goto err;
        }
        if(init->exprtype != MC_EXPR_STMTDEFINE && init->exprtype != MC_EXPR_STMTEXPRESSION)
        {
            mc_errlist_addf(p->errors, MC_ERROR_PARSING, init->pos, "expected a definition or expression as 'for' loop init clause");
            goto err;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_SEMICOLON))
        {
            goto err;
        }
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_currtokenis(&p->lexer, MC_TOK_SEMICOLON))
    {
        test = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
        if(!test)
        {
            goto err;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_SEMICOLON))
        {
            goto err;
        }
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_currtokenis(&p->lexer, MC_TOK_RPAREN))
    {
        update = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
        if(!update)
        {
            goto err;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
        {
            goto err;
        }
    }
    mc_lexer_nexttoken(&p->lexer);
    body = mc_parser_parsecodeblock(p);
    if(!body)
    {
        goto err;
    }
    res = mc_astexpr_makeforloopexpr(p->pstate, init, test, update, body);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astexpr_destroy(init);
    mc_astexpr_destroy(test);
    mc_astexpr_destroy(update);
    mc_astcodeblock_destroy(body);
    return NULL;
}


mcastexprcodeblock_t* mc_parser_parsecodeblock(mcastparser_t* p)
{
    bool ok;
    mcastexprcodeblock_t* res;
    mcastexpression_t* expr;
    mcptrlist_t* statements;
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LBRACE))
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    p->depth++;
    statements = mc_ptrlist_make(p->pstate, sizeof(void*), true);
    if(!statements)
    {
        goto err;
    }
    while(!mc_lexer_currtokenis(&p->lexer, MC_TOK_RBRACE))
    {
        if(mc_lexer_currtokenis(&p->lexer, MC_TOK_EOF))
        {
            mc_errlist_addf(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "unexpected EOF");
            goto err;
        }
        if(mc_lexer_currtokenis(&p->lexer, MC_TOK_SEMICOLON))
        {
            mc_lexer_nexttoken(&p->lexer);
            continue;
        }
        expr = mc_astparser_parsestatement(p);
        if(!expr)
        {
            goto err;
        }
        ok = mc_ptrlist_push(statements, expr);
        if(!ok)
        {
            mc_astexpr_destroy(expr);
            goto err;
        }
    }
    mc_lexer_nexttoken(&p->lexer);
    p->depth--;
    res = mc_astcodeblock_make(p->pstate, statements);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    p->depth--;
    mc_ptrlist_destroy(statements, (mcitemdestroyfn_t)mc_astexpr_destroy);
    return NULL;
}

mcastexpression_t* mc_parser_parseexpression(mcastparser_t* p, mcastprecedence_t prec)
{
    char* literal;
    mcastlocation_t pos;
    mcleftassocparsefn_t parseleftassoc;
    mcastrightassocparsefn_t parserightassoc;
    mcastexpression_t* newleftexpr;
    mcastexpression_t* leftexpr;
    pos = p->lexer.currtoken.pos;
    if(p->lexer.currtoken.toktype == MC_TOK_INVALID)
    {
        mc_errlist_addf(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "illegal token");
        return NULL;
    }
    parserightassoc = p->rightassocfuncs[p->lexer.currtoken.toktype];
    if(!parserightassoc)
    {
        literal = mc_asttoken_dupliteralstring(p->pstate, &p->lexer.currtoken);
        mc_errlist_addf(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "no prefix parse function for \"%s\" found", literal);
        mc_memory_free(literal);
        return NULL;
    }
    leftexpr = parserightassoc(p);
    if(!leftexpr)
    {
        return NULL;
    }
    leftexpr->pos = pos;
    while(!mc_lexer_currtokenis(&p->lexer, MC_TOK_SEMICOLON) && prec < mc_parser_getprecedence(p->lexer.currtoken.toktype))
    {
        parseleftassoc = p->leftassocfuncs[p->lexer.currtoken.toktype];
        if(!parseleftassoc)
        {
            return leftexpr;
        }
        pos = p->lexer.currtoken.pos;
        newleftexpr = parseleftassoc(p, leftexpr);
        if(!newleftexpr)
        {
            mc_astexpr_destroy(leftexpr);
            return NULL;
        }
        newleftexpr->pos = pos;
        leftexpr = newleftexpr;
    }
    return leftexpr;
}

mcastexpression_t* mc_parser_parseident(mcastparser_t* p)
{
    mcastexprident_t* ident;
    mcastexpression_t* res;
    ident = mc_astident_make(p->pstate, p->lexer.currtoken);
    if(!ident)
    {
        return NULL;
    }
    res = mc_astexpr_makeident(p->pstate, ident);
    if(!res)
    {
        mc_astident_destroy(ident);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    return res;
}

mcastexpression_t* mc_parser_parseliteralnumber(mcastparser_t* p)
{
    mcfloat_t number;
    long parsedlen;
    char* end;
    char* literal;
    number = 0;
    errno = 0;
    number = mc_util_strtod(p->lexer.currtoken.tokstrdata, p->lexer.currtoken.tokstrlen, &end);
    #if 0
        fprintf(stderr, "literal=<%s> number=<%f>\n", p->lexer.currtoken.tokstrdata, number);
    #endif
    parsedlen = end - p->lexer.currtoken.tokstrdata;
    if(errno || parsedlen != p->lexer.currtoken.tokstrlen)
    {
        literal = mc_asttoken_dupliteralstring(p->pstate, &p->lexer.currtoken);
        mc_errlist_addf(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "failed to parse number literal \"%s\"", literal);
        mc_memory_free(literal);
        return NULL;
    }    
    mc_lexer_nexttoken(&p->lexer);
    return mc_astexpr_makeliteralnumber(p->pstate, number);
}

mcastexpression_t* mc_parser_parseliteralbool(mcastparser_t* p)
{
    mcastexpression_t* res;
    res = mc_astexpr_makeliteralbool(p->pstate, p->lexer.currtoken.toktype == MC_TOK_TRUE);
    mc_lexer_nexttoken(&p->lexer);
    return res;
}

mcastexpression_t* mc_parser_parseliteralstring(mcastparser_t* p)
{
    size_t len;
    char* processedliteral;
    mcastexpression_t* res;
    processedliteral = mc_parser_processandcopystring(p->pstate, p->lexer.currtoken.tokstrdata, p->lexer.currtoken.tokstrlen);
    if(!processedliteral)
    {
        mc_errlist_addf(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "error parsing string literal");
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    len = mc_util_strlen(processedliteral);
    res = mc_astexpr_makeliteralstring(p->pstate, processedliteral, len);
    if(!res)
    {
        mc_memory_free(processedliteral);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseliteraltemplatestring(mcastparser_t* p)
{
    size_t len;
    char* processedliteral;
    mcastlocation_t pos;
    mcastexpression_t* leftstringexpr;
    mcastexpression_t* templateexpr;
    mcastexpression_t* tostrcallexpr;
    mcastexpression_t* leftaddexpr;
    mcastexpression_t* rightexpr;
    mcastexpression_t* rightaddexpr;
    processedliteral = NULL;
    leftstringexpr = NULL;
    templateexpr = NULL;
    tostrcallexpr = NULL;
    leftaddexpr = NULL;
    rightexpr = NULL;
    rightaddexpr = NULL;
    processedliteral = mc_parser_processandcopystring(p->pstate, p->lexer.currtoken.tokstrdata, p->lexer.currtoken.tokstrlen);
    if(!processedliteral)
    {
        mc_errlist_addf(p->errors, MC_ERROR_PARSING, p->lexer.currtoken.pos, "error parsing string literal");
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LBRACE))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    pos = p->lexer.currtoken.pos;
    len = mc_util_strlen(processedliteral);
    leftstringexpr = mc_astexpr_makeliteralstring(p->pstate, processedliteral, len);
    if(!leftstringexpr)
    {
        goto err;
    }
    leftstringexpr->pos = pos;
    processedliteral = NULL;
    pos = p->lexer.currtoken.pos;
    templateexpr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!templateexpr)
    {
        goto err;
    }
    tostrcallexpr = mc_parser_makefunccallexpr(p->pstate, templateexpr, "tostring");
    if(!tostrcallexpr)
    {
        goto err;
    }
    tostrcallexpr->pos = pos;
    templateexpr = NULL;
    leftaddexpr = mc_astexpr_makeinfixexpr(p->pstate, MC_MATHOP_PLUS, leftstringexpr, tostrcallexpr);
    if(!leftaddexpr)
    {
        goto err;
    }
    leftaddexpr->pos = pos;
    leftstringexpr = NULL;
    tostrcallexpr = NULL;
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RBRACE))
    {
        goto err;
    }
    mc_lexer_previoustoken(&p->lexer);
    mc_lexer_conttplstring(&p->lexer);
    mc_lexer_nexttoken(&p->lexer);
    mc_lexer_nexttoken(&p->lexer);
    pos = p->lexer.currtoken.pos;
    rightexpr = mc_parser_parseexpression(p, MC_ASTPREC_HIGHEST);
    if(!rightexpr)
    {
        goto err;
    }
    rightaddexpr = mc_astexpr_makeinfixexpr(p->pstate, MC_MATHOP_PLUS, leftaddexpr, rightexpr);
    if(!rightaddexpr)
    {
        goto err;
    }
    rightaddexpr->pos = pos;
    leftaddexpr = NULL;
    rightexpr = NULL;
    return rightaddexpr;
err:
    mc_astexpr_destroy(rightaddexpr);
    mc_astexpr_destroy(rightexpr);
    mc_astexpr_destroy(leftaddexpr);
    mc_astexpr_destroy(tostrcallexpr);
    mc_astexpr_destroy(templateexpr);
    mc_astexpr_destroy(leftstringexpr);
    mc_memory_free(processedliteral);
    return NULL;
}

mcastexpression_t* mc_parser_parseliteralnull(mcastparser_t* p)
{
    mc_lexer_nexttoken(&p->lexer);
    return mc_astexpr_makeliteralnull(p->pstate);
}

mcastexpression_t* mc_parser_parseliteralarray(mcastparser_t* p)
{
    mcptrlist_t* array;
    mcastexpression_t* res;
    array = mc_parser_parseexprlist(p, MC_TOK_LBRACKET, MC_TOK_RBRACKET, true);
    if(!array)
    {
        return NULL;
    }
    res = mc_astexpr_makeliteralarray(p->pstate, array);
    if(!res)
    {
        mc_ptrlist_destroy(array, (mcitemdestroyfn_t)mc_astexpr_destroy);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseliteralmap(mcastparser_t* p)
{
    bool ok;
    size_t len;
    char* str;
    mcptrlist_t* keys;
    mcptrlist_t* values;
    mcastexpression_t* res;
    mcastexpression_t* key;
    mcastexpression_t* value;
    keys = mc_ptrlist_make(p->pstate, sizeof(void*), true);
    values = mc_ptrlist_make(p->pstate, sizeof(void*), true);
    if(!keys || !values)
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    while(!mc_lexer_currtokenis(&p->lexer, MC_TOK_RBRACE))
    {
        key = NULL;
        if(mc_lexer_currtokenis(&p->lexer, MC_TOK_IDENT))
        {
            str = mc_asttoken_dupliteralstring(p->pstate, &p->lexer.currtoken);
            len = mc_util_strlen(str);
            key = mc_astexpr_makeliteralstring(p->pstate, str, len);
            if(!key)
            {
                mc_memory_free(str);
                goto err;
            }
            key->pos = p->lexer.currtoken.pos;
            mc_lexer_nexttoken(&p->lexer);
        }
        else
        {
            key = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
            if(!key)
            {
                goto err;
            }
            switch(key->exprtype)
            {
                case MC_EXPR_STRINGLITERAL:
                case MC_EXPR_NUMBERLITERAL:
                case MC_EXPR_BOOLLITERAL:
                    {
                    }
                    break;
                default:
                    {
                        mc_errlist_addf(p->errors, MC_ERROR_PARSING, key->pos, "can only use primitive types as literal 'map' object keys");
                        mc_astexpr_destroy(key);
                        goto err;
                    }
                    break;
            }
        }
        ok = mc_ptrlist_push(keys, key);
        if(!ok)
        {
            mc_astexpr_destroy(key);
            goto err;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_COLON))
        {
            goto err;
        }
        mc_lexer_nexttoken(&p->lexer);
        value = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
        if(!value)
        {
            goto err;
        }
        ok = mc_ptrlist_push(values, value);
        if(!ok)
        {
            mc_astexpr_destroy(value);
            goto err;
        }
        if(mc_lexer_currtokenis(&p->lexer, MC_TOK_RBRACE))
        {
            break;
        }
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_COMMA))
        {
            goto err;
        }
        mc_lexer_nexttoken(&p->lexer);
    }
    mc_lexer_nexttoken(&p->lexer);
    res = mc_astexpr_makeliteralmap(p->pstate, keys, values);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_ptrlist_destroy(keys, (mcitemdestroyfn_t)mc_astexpr_destroy);
    mc_ptrlist_destroy(values, (mcitemdestroyfn_t)mc_astexpr_destroy);
    return NULL;
}

mcastexpression_t* mc_parser_parseprefixexpr(mcastparser_t* p)
{
    mcastmathoptype_t op;
    mcastexpression_t* res;
    mcastexpression_t* right;
    op = mc_parser_tokentomathop(p->lexer.currtoken.toktype);
    mc_lexer_nexttoken(&p->lexer);
    right = mc_parser_parseexpression(p, MC_ASTPREC_PREFIX);
    if(!right)
    {
        return NULL;
    }
    res = mc_astexpr_makeprefixexpr(p->pstate, op, right);
    if(!res)
    {
        mc_astexpr_destroy(right);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseinfixexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastmathoptype_t op;
    mcastprecedence_t prec;
    mcastexpression_t* res;
    mcastexpression_t* right;
    op = mc_parser_tokentomathop(p->lexer.currtoken.toktype);
    prec = mc_parser_getprecedence(p->lexer.currtoken.toktype);
    mc_lexer_nexttoken(&p->lexer);
    right = mc_parser_parseexpression(p, prec);
    if(!right)
    {
        return NULL;
    }
    res = mc_astexpr_makeinfixexpr(p->pstate, op, left, right);
    if(!res)
    {
        mc_astexpr_destroy(right);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parsegroupedexpr(mcastparser_t* p)
{
    mcastexpression_t* expr;
    mc_lexer_nexttoken(&p->lexer);
    expr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!expr || !mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        mc_astexpr_destroy(expr);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    return expr;
}


bool mc_parser_parsefuncparams(mcastparser_t* p, mcptrlist_t* outparams)
{
    bool ok;
    mcastexprident_t* ident;
    mcastfuncparam_t* param;
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_LPAREN))
    {
        return false;
    }
    mc_lexer_nexttoken(&p->lexer);
    if(mc_lexer_currtokenis(&p->lexer, MC_TOK_RPAREN))
    {
        mc_lexer_nexttoken(&p->lexer);
        return true;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IDENT))
    {
        return false;
    }
    ident = mc_astident_make(p->pstate, p->lexer.currtoken);
    if(!ident)
    {
        return false;
    }
    param = mc_astfuncparam_make(p->pstate, ident);
    ok = mc_ptrlist_push(outparams, param);
    if(!ok)
    {
        mc_astident_destroy(ident);
        return false;
    }
    mc_lexer_nexttoken(&p->lexer);
    while(mc_lexer_currtokenis(&p->lexer, MC_TOK_COMMA))
    {
        mc_lexer_nexttoken(&p->lexer);
        if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IDENT))
        {
            return false;
        }
        ident = mc_astident_make(p->pstate, p->lexer.currtoken);
        if(!ident)
        {
            return false;
        }
        param = mc_astfuncparam_make(p->pstate, ident);
        ok = mc_ptrlist_push(outparams, param);
        if(!ok)
        {
            mc_astfuncparam_destroy(param);
            return false;
        }
        mc_lexer_nexttoken(&p->lexer);
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RPAREN))
    {
        return false;
    }
    mc_lexer_nexttoken(&p->lexer);
    return true;
}

mcastexpression_t* mc_parser_parseliteralfunction(mcastparser_t* p)
{
    bool ok;
    mcptrlist_t* params;
    mcastexprcodeblock_t* body;
    mcastexpression_t* res;
    p->depth++;
    params = NULL;
    body = NULL;
    if(mc_lexer_currtokenis(&p->lexer, MC_TOK_FUNCTION))
    {
        mc_lexer_nexttoken(&p->lexer);
    }
    params = mc_ptrlist_make(p->pstate, sizeof(void*), true);
    ok = mc_parser_parsefuncparams(p, params);
    if(!ok)
    {
        goto err;
    }
    body = mc_parser_parsecodeblock(p);
    if(!body)
    {
        goto err;
    }
    res = mc_astexpr_makeliteralfunction(p->pstate, params, body);
    if(!res)
    {
        goto err;
    }
    p->depth -= 1;
    return res;
err:
    mc_astcodeblock_destroy(body);
    mc_ptrlist_destroy(params, (mcitemdestroyfn_t)mc_astfuncparam_destroy);
    p->depth -= 1;
    return NULL;
}

mcastexpression_t* mc_parser_parsefunctionstmt(mcastparser_t* p)
{
    mcastexprident_t* nameident;
    mcastexpression_t* res;
    mcastexpression_t* value;
    mcastlocation_t pos;
    nameident = NULL;
    value = NULL;
    pos = p->lexer.currtoken.pos;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IDENT))
    {
        goto err;
    }
    nameident = mc_astident_make(p->pstate, p->lexer.currtoken);
    if(!nameident)
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    value = mc_parser_parseliteralfunction(p);
    if(!value)
    {
        goto err;
    }
    value->pos = pos;
    value->uexpr.exprlitfunction.name = mc_util_strdup(p->pstate, nameident->value);
    if(!value->uexpr.exprlitfunction.name)
    {
        goto err;
    }
    res = mc_astexpr_makedefineexpr(p->pstate, nameident, value, false);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astexpr_destroy(value);
    mc_astident_destroy(nameident);
    return NULL;
}


mcastexpression_t* mc_parser_parsecallexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcptrlist_t* args;
    mcastexpression_t* res;
    mcastexpression_t* function;
    function = left;
    args = mc_parser_parseexprlist(p, MC_TOK_LPAREN, MC_TOK_RPAREN, false);
    if(!args)
    {
        return NULL;
    }
    res = mc_astexpr_makecallexpr(p->pstate, function, args);
    if(!res)
    {
        mc_ptrlist_destroy(args, (mcitemdestroyfn_t)mc_astexpr_destroy);
        return NULL;
    }
    return res;
}

mcptrlist_t* mc_parser_parseexprlist(mcastparser_t* p, mcasttoktype_t starttoken, mcasttoktype_t endtoken, bool trailingcommaallowed)
{
    bool ok;
    mcptrlist_t* res;
    mcastexpression_t* argexpr;
    if(!mc_lexer_expectcurrent(&p->lexer, starttoken))
    {
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    res = mc_ptrlist_make(p->pstate, sizeof(void*), true);
    if(mc_lexer_currtokenis(&p->lexer, endtoken))
    {
        mc_lexer_nexttoken(&p->lexer);
        return res;
    }
    argexpr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!argexpr)
    {
        goto err;
    }
    ok = mc_ptrlist_push(res, argexpr);
    if(!ok)
    {
        mc_astexpr_destroy(argexpr);
        goto err;
    }
    while(mc_lexer_currtokenis(&p->lexer, MC_TOK_COMMA))
    {
        mc_lexer_nexttoken(&p->lexer);
        if(trailingcommaallowed && mc_lexer_currtokenis(&p->lexer, endtoken))
        {
            break;
        }
        argexpr = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
        if(!argexpr)
        {
            goto err;
        }
        ok = mc_ptrlist_push(res, argexpr);
        if(!ok)
        {
            mc_astexpr_destroy(argexpr);
            goto err;
        }
    }
    if(!mc_lexer_expectcurrent(&p->lexer, endtoken))
    {
        goto err;
    }
    mc_lexer_nexttoken(&p->lexer);
    return res;
err:
    mc_ptrlist_destroy(res, (mcitemdestroyfn_t)mc_astexpr_destroy);
    return NULL;
}

mcastexpression_t* mc_parser_parseindexexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastexpression_t* res;
    mcastexpression_t* index;
    mc_lexer_nexttoken(&p->lexer);
    index = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!index)
    {
        return NULL;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_RBRACKET))
    {
        mc_astexpr_destroy(index);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    res = mc_astexpr_makeindexexpr(p->pstate, left, index, false);
    if(!res)
    {
        mc_astexpr_destroy(index);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseassignexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastlocation_t pos;
    mcastmathoptype_t op;
    mcasttoktype_t assigntype;
    mcastexpression_t* res;
    mcastexpression_t* source;
    mcastexpression_t* leftcopy;
    mcastexpression_t* newsource;
    source = NULL;
    assigntype = p->lexer.currtoken.toktype;
    mc_lexer_nexttoken(&p->lexer);
    source = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!source)
    {
        goto err;
    }
    switch(assigntype)
    {
        case MC_TOK_ASSIGNPLUS:
        case MC_TOK_ASSIGNMINUS:
        case MC_TOK_ASSIGNSLASH:
        case MC_TOK_ASSIGNASTERISK:
        case MC_TOK_ASSIGNPERCENT:
        case MC_TOK_ASSIGNBINAND:
        case MC_TOK_ASSIGNBINOR:
        case MC_TOK_ASSIGNBINXOR:
        case MC_TOK_ASSIGNLSHIFT:
        case MC_TOK_ASSIGNRSHIFT:
            {
                op = mc_parser_tokentomathop(assigntype);
                leftcopy = mc_astexpr_copyexpr(left);
                if(!leftcopy)
                {
                    goto err;
                }
                pos = source->pos;
                newsource = mc_astexpr_makeinfixexpr(p->pstate, op, leftcopy, source);
                if(!newsource)
                {
                    mc_astexpr_destroy(leftcopy);
                    goto err;
                }
                newsource->pos = pos;
                source = newsource;
            }
            break;
        case MC_TOK_ASSIGN:
            {
            }
            break;
        default:
            {
                MC_ASSERT(false);
            }
            break;
    }
    res = mc_astexpr_makeassignexpr(p->pstate, left, source, false);
    if(!res)
    {
        goto err;
    }
    return res;
err:
    mc_astexpr_destroy(source);
    return NULL;
}

mcastexpression_t* mc_parser_parselogicalexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastmathoptype_t op;
    mcastprecedence_t prec;
    mcastexpression_t* res;
    mcastexpression_t* right;
    op = mc_parser_tokentomathop(p->lexer.currtoken.toktype);
    prec = mc_parser_getprecedence(p->lexer.currtoken.toktype);
    mc_lexer_nexttoken(&p->lexer);
    right = mc_parser_parseexpression(p, prec);
    if(!right)
    {
        return NULL;
    }
    res = mc_astexpr_makelogicalexpr(p->pstate, op, left, right);
    if(!res)
    {
        mc_astexpr_destroy(right);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseternaryexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastexpression_t* res;
    mcastexpression_t* ift;
    mcastexpression_t* iffalse;
    mc_lexer_nexttoken(&p->lexer);
    ift = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!ift)
    {
        return NULL;
    }
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_COLON))
    {
        mc_astexpr_destroy(ift);
        return NULL;
    }
    mc_lexer_nexttoken(&p->lexer);
    iffalse = mc_parser_parseexpression(p, MC_ASTPREC_LOWEST);
    if(!iffalse)
    {
        mc_astexpr_destroy(ift);
        return NULL;
    }
    res = mc_astexpr_maketernaryexpr(p->pstate, left, ift, iffalse);
    if(!res)
    {
        mc_astexpr_destroy(ift);
        mc_astexpr_destroy(iffalse);
        return NULL;
    }
    return res;
}

mcastexpression_t* mc_parser_parseincdecprefixexpr(mcastparser_t* p)
{
    mcastlocation_t pos;
    mcastmathoptype_t op;
    mcasttoktype_t operationtype;
    mcastexpression_t* res;
    mcastexpression_t* dest;
    mcastexpression_t* source;
    mcastexpression_t* destcopy;
    mcastexpression_t* operation;
    mcastexpression_t* oneliteral;
    source = NULL;
    operationtype = p->lexer.currtoken.toktype;
    pos = p->lexer.currtoken.pos;
    mc_lexer_nexttoken(&p->lexer);
    op = mc_parser_tokentomathop(operationtype);
    dest = mc_parser_parseexpression(p, MC_ASTPREC_PREFIX);
    if(!dest)
    {
        goto err;
    }
    oneliteral = mc_astexpr_makeliteralnumber(p->pstate, 1);
    if(!oneliteral)
    {
        mc_astexpr_destroy(dest);
        goto err;
    }
    oneliteral->pos = pos;
    destcopy = mc_astexpr_copyexpr(dest);
    if(!destcopy)
    {
        mc_astexpr_destroy(oneliteral);
        mc_astexpr_destroy(dest);
        goto err;
    }
    operation = mc_astexpr_makeinfixexpr(p->pstate, op, destcopy, oneliteral);
    if(!operation)
    {
        mc_astexpr_destroy(destcopy);
        mc_astexpr_destroy(dest);
        mc_astexpr_destroy(oneliteral);
        goto err;
    }
    operation->pos = pos;
    res = mc_astexpr_makeassignexpr(p->pstate, dest, operation, false);
    if(!res)
    {
        mc_astexpr_destroy(dest);
        mc_astexpr_destroy(operation);
        goto err;
    }
    return res;
err:
    mc_astexpr_destroy(source);
    return NULL;
}

mcastexpression_t* mc_parser_parseincdecpostfixexpr(mcastparser_t* p, mcastexpression_t* left)
{
    mcastlocation_t pos;
    mcastmathoptype_t op;
    mcasttoktype_t operationtype;
    mcastexpression_t* res;
    mcastexpression_t* source;
    mcastexpression_t* leftcopy;
    mcastexpression_t* operation;
    mcastexpression_t* oneliteral;
    source = NULL;
    operationtype = p->lexer.currtoken.toktype;
    pos = p->lexer.currtoken.pos;
    mc_lexer_nexttoken(&p->lexer);
    op = mc_parser_tokentomathop(operationtype);
    leftcopy = mc_astexpr_copyexpr(left);
    if(!leftcopy)
    {
        goto err;
    }
    oneliteral = mc_astexpr_makeliteralnumber(p->pstate, 1);
    if(!oneliteral)
    {
        mc_astexpr_destroy(leftcopy);
        goto err;
    }
    oneliteral->pos = pos;
    operation = mc_astexpr_makeinfixexpr(p->pstate, op, leftcopy, oneliteral);
    if(!operation)
    {
        mc_astexpr_destroy(oneliteral);
        mc_astexpr_destroy(leftcopy);
        goto err;
    }
    operation->pos = pos;
    res = mc_astexpr_makeassignexpr(p->pstate, left, operation, true);
    if(!res)
    {
        mc_astexpr_destroy(operation);
        goto err;
    }
    return res;
err:
    mc_astexpr_destroy(source);
    return NULL;
}

mcastexpression_t* mc_parser_parsedotexpression(mcastparser_t* p, mcastexpression_t* left)
{
    size_t len;
    char* str;
    mcastexpression_t* res;
    mcastexpression_t* index;
    mc_lexer_nexttoken(&p->lexer);
    if(!mc_lexer_expectcurrent(&p->lexer, MC_TOK_IDENT))
    {
        return NULL;
    }
    str = mc_asttoken_dupliteralstring(p->pstate, &p->lexer.currtoken);
    len = mc_util_strlen(str);
    index = mc_astexpr_makeliteralstring(p->pstate, str, len);
    if(!index)
    {
        mc_memory_free(str);
        return NULL;
    }
    index->pos = p->lexer.currtoken.pos;
    mc_lexer_nexttoken(&p->lexer);
    res = mc_astexpr_makeindexexpr(p->pstate, left, index, true);
    if(!res)
    {
        mc_astexpr_destroy(index);
        return NULL;
    }
    return res;
}