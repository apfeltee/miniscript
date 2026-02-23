class AstParser
{
    public:
        struct Maybe
        {
            public:
                bool m_havevalue = false;
                AstExpression* m_exprvalue;

            public:
                Maybe(nullptr_t)
                {
                }

                Maybe(AstExpression* ex): m_havevalue(true), m_exprvalue(ex)
                {
                }

                operator AstExpression*()
                {
                    return m_exprvalue;
                }

                bool operator==(nullptr_t)
                {
                    return m_havevalue;
                }
        };


        using mcastrightassocparsefn_t = Maybe (*)(AstParser*);
        using mcastleftassocparsefn_t = Maybe (*)(AstParser*, AstExpression*);


        enum mcastprecedence_t
        {
            MC_ASTPREC_LOWEST = 0,
            MC_ASTPREC_ASSIGN,
            /* a = b */
            MC_ASTPREC_TERNARY,
            /* a ? b : c */
            MC_ASTPREC_LOGICALOR,
            /* || */
            MC_ASTPREC_LOGICALAND,
            /* && */
            MC_ASTPREC_BINOR,
            /* | */
            MC_ASTPREC_BINXOR,
            /* ^ */
            MC_ASTPREC_BINAND,
            /* & */
            MC_ASTPREC_EQUALS,
            /* == != */
            MC_ASTPREC_LESSGREATER,
            /* >, >=, <, <= */
            MC_ASTPREC_SHIFT,
            /* << >> */
            MC_ASTPREC_SUM,
            /* + - */
            MC_ASTPREC_PRODUCT,
            /* * / % */
            MC_ASTPREC_PREFIX,
            /* -x !x ++x --x */
            MC_ASTPREC_INCDEC,
            /* x++ x-- */
            MC_ASTPREC_POSTFIX,
            /* myFunction(x) x["foo"] x.foo */
            MC_ASTPREC_HIGHEST
        };

    public:
        static mcastprecedence_t getPrecedence(AstToken::Type tk)
        {
            switch(tk)
            {
                case AstToken::TOK_EQ:
                case AstToken::TOK_NOTEQ:
                    return MC_ASTPREC_EQUALS;
                case AstToken::TOK_LT:
                case AstToken::TOK_LTE:
                case AstToken::TOK_GT:
                case AstToken::TOK_GTE:
                    return MC_ASTPREC_LESSGREATER;
                case AstToken::TOK_PLUS:
                case AstToken::TOK_UNARYMINUS:
                case AstToken::TOK_UNARYBINNOT:
                    return MC_ASTPREC_SUM;
                case AstToken::TOK_SLASH:
                case AstToken::TOK_ASTERISK:
                case AstToken::TOK_PERCENT:
                    return MC_ASTPREC_PRODUCT;
                case AstToken::TOK_LPAREN:
                case AstToken::TOK_LBRACKET:
                    return MC_ASTPREC_POSTFIX;
                case AstToken::TOK_ASSIGN:
                case AstToken::TOK_ASSIGNPLUS:
                case AstToken::TOK_ASSIGNMINUS:
                case AstToken::TOK_ASSIGNASTERISK:
                case AstToken::TOK_ASSIGNSLASH:
                case AstToken::TOK_ASSIGNPERCENT:
                case AstToken::TOK_ASSIGNBINAND:
                case AstToken::TOK_ASSIGNBINOR:
                case AstToken::TOK_ASSIGNBINXOR:
                case AstToken::TOK_ASSIGNLSHIFT:
                case AstToken::TOK_ASSIGNRSHIFT:
                    return MC_ASTPREC_ASSIGN;
                case AstToken::TOK_DOT:
                    return MC_ASTPREC_POSTFIX;
                case AstToken::TOK_AND:
                    return MC_ASTPREC_LOGICALAND;
                case AstToken::TOK_OR:
                    return MC_ASTPREC_LOGICALOR;
                case AstToken::TOK_BINOR:
                    return MC_ASTPREC_BINOR;
                case AstToken::TOK_BINXOR:
                    return MC_ASTPREC_BINXOR;
                case AstToken::TOK_BINAND:
                    return MC_ASTPREC_BINAND;
                case AstToken::TOK_LSHIFT:
                case AstToken::TOK_RSHIFT:
                    return MC_ASTPREC_SHIFT;
                case AstToken::TOK_QUESTION:
                    return MC_ASTPREC_TERNARY;
                case AstToken::TOK_PLUSPLUS:
                case AstToken::TOK_MINUSMINUS:
                    return MC_ASTPREC_INCDEC;
                default:
                    break;
            }
            return MC_ASTPREC_LOWEST;
        }

        static mcastmathoptype_t tokenToMathOP(AstToken::Type tk)
        {
            switch(tk)
            {
                case AstToken::TOK_ASSIGN:
                    return MC_MATHOP_ASSIGN;
                case AstToken::TOK_PLUS:
                    return MC_MATHOP_PLUS;
                case AstToken::TOK_UNARYMINUS:
                    return MC_MATHOP_MINUS;
                case AstToken::TOK_UNARYBINNOT:
                    return MC_MATHOP_BINNOT;
                case AstToken::TOK_BANG:
                    return MC_MATHOP_BANG;
                case AstToken::TOK_ASTERISK:
                    return MC_MATHOP_ASTERISK;
                case AstToken::TOK_SLASH:
                    return MC_MATHOP_SLASH;
                case AstToken::TOK_LT:
                    return MC_MATHOP_LT;
                case AstToken::TOK_LTE:
                    return MC_MATHOP_LTE;
                case AstToken::TOK_GT:
                    return MC_MATHOP_GT;
                case AstToken::TOK_GTE:
                    return MC_MATHOP_GTE;
                case AstToken::TOK_EQ:
                    return MC_MATHOP_EQ;
                case AstToken::TOK_NOTEQ:
                    return MC_MATHOP_NOTEQ;
                case AstToken::TOK_PERCENT:
                    return MC_MATHOP_MODULUS;
                case AstToken::TOK_AND:
                    return MC_MATHOP_LOGICALAND;
                case AstToken::TOK_OR:
                    return MC_MATHOP_LOGICALOR;
                case AstToken::TOK_ASSIGNPLUS:
                    return MC_MATHOP_PLUS;
                case AstToken::TOK_ASSIGNMINUS:
                    return MC_MATHOP_MINUS;
                case AstToken::TOK_ASSIGNASTERISK:
                    return MC_MATHOP_ASTERISK;
                case AstToken::TOK_ASSIGNSLASH:
                    return MC_MATHOP_SLASH;
                case AstToken::TOK_ASSIGNPERCENT:
                    return MC_MATHOP_MODULUS;
                case AstToken::TOK_ASSIGNBINAND:
                    return MC_MATHOP_BINAND;
                case AstToken::TOK_ASSIGNBINOR:
                    return MC_MATHOP_BINOR;
                case AstToken::TOK_ASSIGNBINXOR:
                    return MC_MATHOP_BINXOR;
                case AstToken::TOK_ASSIGNLSHIFT:
                    return MC_MATHOP_LSHIFT;
                case AstToken::TOK_ASSIGNRSHIFT:
                    return MC_MATHOP_RSHIFT;
                case AstToken::TOK_BINAND:
                    return MC_MATHOP_BINAND;
                case AstToken::TOK_BINOR:
                    return MC_MATHOP_BINOR;
                case AstToken::TOK_BINXOR:
                    return MC_MATHOP_BINXOR;
                case AstToken::TOK_LSHIFT:
                    return MC_MATHOP_LSHIFT;
                case AstToken::TOK_RSHIFT:
                    return MC_MATHOP_RSHIFT;
                case AstToken::TOK_PLUSPLUS:
                    return MC_MATHOP_PLUS;
                case AstToken::TOK_MINUSMINUS:
                    return MC_MATHOP_MINUS;
                default:
                    {
                        MC_ASSERT(false);
                    }
                    break;
            }
            return MC_MATHOP_NONE;
        }

        static char getEscapeChar(char c)
        {
            switch(c)
            {
                case '\"':
                    return '\"';
                case '\\':
                    return '\\';
                case '/':
                    return '/';
                case 'e':
                    return 27;
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

        static char* processAndCopyString(const char* input, size_t len)
        {
            size_t ini;
            size_t outi;
            char* output;
            output = (char*)mc_memory_malloc(len + 1);
            if(!output)
            {
                return nullptr;
            }
            ini = 0;
            outi = 0;
            while(input[ini] != '\0' && ini < len)
            {
                if(input[ini] == '\\')
                {
                    ini++;
                    output[outi] = getEscapeChar(input[ini]);
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
            return nullptr;
        }

        static Maybe callback_parseident(AstParser* p)
        {
            return p->parseIdent();
        }

        static Maybe callback_parseliteralnumber(AstParser* p)
        {
            return p->parseLiteralNumber();
        }

        static Maybe callback_parseliteralbool(AstParser* p)
        {
            return p->parseLiteralBool();
        }

        static Maybe callback_parseliteralstring(AstParser* p)
        {
            return p->parseLiteralString();
        }

        static Maybe callback_parseliteraltemplatestring(AstParser* p)
        {
            return p->parseLiteralTemplateString();
        }

        static Maybe callback_parseliteralnull(AstParser* p)
        {
            return p->parseLiteralNull();
        }

        static Maybe callback_parseliteralarray(AstParser* p)
        {
            return p->parseLiteralArray();
        }

        static Maybe callback_parseliteralmap(AstParser* p)
        {
            return p->parseLiteralMap();
        }

        static Maybe callback_parseprefixexpr(AstParser* p)
        {
            return p->parsePrefixExpr();
        }

        static Maybe callback_parseinfixexpr(AstParser* p, AstExpression* left)
        {
            return p->parseInfixExpr(left);
        }

        static Maybe callback_parseliteralfunction(AstParser* p)
        {
            return p->parseLiteralFunction();
        }

        static Maybe callback_parseindexexpr(AstParser* p, AstExpression* left)
        {
            return p->parseIndexExpr(left);
        }

        static Maybe callback_parseassignexpr(AstParser* p, AstExpression* left)
        {
            return p->parseAssignExpr(left);
        }

        static Maybe callback_parselogicalexpr(AstParser* p, AstExpression* left)
        {
            return p->parseLogicalExpr(left);
        }

        static Maybe callback_parseternaryexpr(AstParser* p, AstExpression* left)
        {
            return p->parseTernaryExpr(left);
        }

        static Maybe callback_parseincdecprefixexpr(AstParser* p)
        {
            return p->parseIncDecPrefixExpr();
        }

        static Maybe callback_parseincdecpostfixexpr(AstParser* p, AstExpression* left)
        {
            return p->parseIncDecPostfixExpr(left);
        }

        static Maybe callback_parsedotexpression(AstParser* p, AstExpression* left)
        {
            return p->parseDotExpr(left);
        }

        static Maybe callback_parserecoverstmt(AstParser* p)
        {
            return p->parseRecoverStmt();
        }

        static Maybe callback_parsegroupedexpr(AstParser* p)
        {
            return p->parseGroupedExpr();
        }

        static Maybe callback_parsecallexpr(AstParser* p, AstExpression* left)
        {
            return p->parseCallExpr(left);
        }

        static void destroy(AstParser* parser)
        {
            if(parser != nullptr)
            {
                mc_memory_free(parser);
            }
        }

        /*
        * these two functions used to be a table; but that made it functionally
        * impossible if a callback for a token type existed or not
        */
        static mcastrightassocparsefn_t getRightAssocParseFunc(AstToken::Type t)
        {
            switch(t)
            {
                case AstToken::TOK_IDENT: return callback_parseident;
                case AstToken::TOK_NUMBER: return callback_parseliteralnumber;
                case AstToken::TOK_TRUE: return callback_parseliteralbool;
                case AstToken::TOK_FALSE: return callback_parseliteralbool;
                case AstToken::TOK_STRING: return callback_parseliteralstring;
                case AstToken::TOK_TEMPLATESTRING: return callback_parseliteraltemplatestring;
                case AstToken::TOK_NULL: return callback_parseliteralnull;
                case AstToken::TOK_BANG: return callback_parseprefixexpr;
                case AstToken::TOK_UNARYMINUS: return callback_parseprefixexpr;
                case AstToken::TOK_UNARYBINNOT: return callback_parseprefixexpr;
                case AstToken::TOK_LPAREN: return callback_parsegroupedexpr;
                case AstToken::TOK_FUNCTION: return callback_parseliteralfunction;
                case AstToken::TOK_LBRACKET: return callback_parseliteralarray;
                case AstToken::TOK_LBRACE: return callback_parseliteralmap;
                case AstToken::TOK_PLUSPLUS: return callback_parseincdecprefixexpr;
                case AstToken::TOK_MINUSMINUS: return callback_parseincdecprefixexpr;
                #if 1
                case AstToken::TOK_RECOVER: return callback_parserecoverstmt;
                #endif
                default:
                    break;
            }
            return nullptr;
        }

        static mcastleftassocparsefn_t getLeftAssocParseFunc(AstToken::Type t)
        {
            switch(t)
            {
                case AstToken::TOK_PLUS: return callback_parseinfixexpr;
                case AstToken::TOK_UNARYMINUS: return callback_parseinfixexpr;
                case AstToken::TOK_SLASH: return callback_parseinfixexpr;
                case AstToken::TOK_ASTERISK: return callback_parseinfixexpr;
                case AstToken::TOK_PERCENT: return callback_parseinfixexpr;
                case AstToken::TOK_EQ: return callback_parseinfixexpr;
                case AstToken::TOK_NOTEQ: return callback_parseinfixexpr;
                case AstToken::TOK_LT: return callback_parseinfixexpr;
                case AstToken::TOK_LTE: return callback_parseinfixexpr;
                case AstToken::TOK_GT: return callback_parseinfixexpr;
                case AstToken::TOK_GTE: return callback_parseinfixexpr;
                case AstToken::TOK_LPAREN: return callback_parsecallexpr;
                case AstToken::TOK_LBRACKET: return callback_parseindexexpr;
                case AstToken::TOK_ASSIGN: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNPLUS: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNMINUS: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNSLASH: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNASTERISK: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNPERCENT: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNBINAND: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNBINOR: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNBINXOR: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNLSHIFT: return callback_parseassignexpr;
                case AstToken::TOK_ASSIGNRSHIFT: return callback_parseassignexpr;
                case AstToken::TOK_DOT: return callback_parsedotexpression;
                case AstToken::TOK_AND: return callback_parselogicalexpr;
                case AstToken::TOK_OR: return callback_parselogicalexpr;
                case AstToken::TOK_BINAND: return callback_parseinfixexpr;
                case AstToken::TOK_BINOR: return callback_parseinfixexpr;
                case AstToken::TOK_BINXOR: return callback_parseinfixexpr;
                case AstToken::TOK_LSHIFT: return callback_parseinfixexpr;
                case AstToken::TOK_RSHIFT: return callback_parseinfixexpr;
                case AstToken::TOK_QUESTION: return callback_parseternaryexpr;
                case AstToken::TOK_PLUSPLUS: return callback_parseincdecpostfixexpr;
                case AstToken::TOK_MINUSMINUS: return callback_parseincdecpostfixexpr;
                default:
                    break;
            }
            return nullptr;
        }

    public:
        mcconfig_t* m_config;
        AstLexer m_lexer;
        ErrList* m_prserrlist;
        int m_parsedepth;

    public:
        AstParser(mcconfig_t* config, ErrList* errors)
        {
            m_config = config;
            m_prserrlist = errors;
            m_parsedepth = 0;
        }


        Maybe parseVarLetStmt()
        {
            bool assignable;
            AstExpression::ExprIdent* nameident;
            AstExpression* value;
            AstExpression* res;
            nameident = nullptr;
            value = nullptr;
            assignable = m_lexer.currentTokenIs(AstToken::TOK_VAR);
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_IDENT))
            {
                goto err;
            }
            nameident = Memory::make<AstExpression::ExprIdent>(m_lexer.m_currtoken);
            m_lexer.nextToken();
            #if 0
                if(!m_lexer.expectCurrent(AstToken::TOK_ASSIGN))
            #else
                if(!m_lexer.currentTokenIs(AstToken::TOK_ASSIGN))
            #endif
            {
                value = AstExpression::makeAstItemLiteralNull();
                goto finish;
            }
            m_lexer.nextToken();
            value = parseExpression(MC_ASTPREC_LOWEST);
            if(!value)
            {
                goto err;
            }
            if(value->m_exprtype == AstExpression::EXPR_FUNCTIONLITERAL)
            {
                value->m_uexpr.exprlitfunction.name = mc_util_strdup(nameident->m_identvalue);
                if(!value->m_uexpr.exprlitfunction.name)
                {
                    goto err;
                }
            }
            finish:
            res = AstExpression::makeAstItemDefine(nameident, value, assignable);
            return res;
        err:
            AstExpression::destroyExpression(value);
            Memory::destroy(nameident);
            return nullptr;
        }

        Maybe parseIfStmt()
        {
            bool ok;
            PtrList* cases;
            AstExpression::ExprIfCase* cond;
            AstExpression::ExprIfCase* elif;
            AstExpression::ExprCodeBlock* alternative;
            (void)ok;
            AstExpression* res;
            cases = nullptr;
            alternative = nullptr;
            cases = Memory::make<PtrList>(sizeof(void*), true);
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_LPAREN))
            {
                goto err;
            }
            m_lexer.nextToken();
            cond = Memory::make<AstExpression::ExprIfCase>(nullptr, nullptr);
            ok = cases->push(cond);
            cond->m_ifcond = parseExpression(MC_ASTPREC_LOWEST);
            if(!cond->m_ifcond)
            {
                goto err;
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
            {
                goto err;
            }
            m_lexer.nextToken();
            cond->m_ifthen = parseCodeBlock();
            if(!cond->m_ifthen)
            {
                goto err;
            }
            while(m_lexer.currentTokenIs(AstToken::TOK_ELSE))
            {
                m_lexer.nextToken();
                if(m_lexer.currentTokenIs(AstToken::TOK_IF))
                {
                    m_lexer.nextToken();
                    if(!m_lexer.expectCurrent(AstToken::TOK_LPAREN))
                    {
                        goto err;
                    }
                    m_lexer.nextToken();
                    elif = Memory::make<AstExpression::ExprIfCase>(nullptr, nullptr);
                    ok = cases->push(elif);
                    elif->m_ifcond = parseExpression(MC_ASTPREC_LOWEST);
                    if(!elif->m_ifcond)
                    {
                        goto err;
                    }
                    if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
                    {
                        goto err;
                    }
                    m_lexer.nextToken();
                    elif->m_ifthen = parseCodeBlock();
                    if(!elif->m_ifthen)
                    {
                        goto err;
                    }
                }
                else
                {
                    alternative = parseCodeBlock();
                    if(!alternative)
                    {
                        goto err;
                    }
                }
            }
            res = AstExpression::makeAstItemIfStmt(cases, alternative);
            return res;
        err:
            PtrList::destroy(cases, (mcitemdestroyfn_t)AstExpression::ExprIfCase::destroy);
            Memory::destroy(alternative);
            return nullptr;
        }

        Maybe parseReturnStmt()
        {
            AstExpression* res;
            AstExpression* expr;
            expr = nullptr;
            m_lexer.nextToken();
            if(!m_lexer.currentTokenIs(AstToken::TOK_SEMICOLON) && !m_lexer.currentTokenIs(AstToken::TOK_RBRACE) && !m_lexer.currentTokenIs(AstToken::TOK_EOF))
            {
                expr = parseExpression(MC_ASTPREC_LOWEST);
                if(!expr)
                {
                    return nullptr;
                }
            }
            res = AstExpression::makeAstItemReturnStmt(expr);
            return res;
        }

        Maybe parseExprStmt()
        {
            AstExpression* res;
            AstExpression* expr;
            expr = parseExpression(MC_ASTPREC_LOWEST);
            if(!expr)
            {
                return nullptr;
            }
            if(expr && (!m_config->replmode || m_parsedepth > 0))
            {
                #if 0
                /* this is actually completely unnecessary */
                if(expr->m_exprtype != AstExpression::EXPR_ASSIGN && expr->m_exprtype != AstExpression::EXPR_CALL)
                {
                    m_prserrlist->pushFormat(MC_ERROR_PARSING, expr->pos, "only assignments and function calls can be expression statements");
                    AstExpression::destroyExpression(expr);
                    return nullptr;
                }
                #endif
            }
            res = AstExpression::makeAstItemExprStmt(expr);
            return res;
        }

        Maybe parseLoopWhileStmt()
        {
            AstExpression* res;
            AstExpression* test;
            AstExpression::ExprCodeBlock* body;
            test = nullptr;
            body = nullptr;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_LPAREN))
            {
                goto err;
            }
            m_lexer.nextToken();
            test = parseExpression(MC_ASTPREC_LOWEST);
            if(!test)
            {
                goto err;
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
            {
                goto err;
            }
            m_lexer.nextToken();
            body = parseCodeBlock();
            if(!body)
            {
                goto err;
            }
            res = AstExpression::makeAstItemWhileStmt(test, body);
            return res;
        err:
            Memory::destroy(body);
            AstExpression::destroyExpression(test);
            return nullptr;
        }

        Maybe parseBreakStmt()
        {
            m_lexer.nextToken();
            return AstExpression::makeAstItemBreakStmt();
        }

        Maybe parseContinueStmt()
        {
            m_lexer.nextToken();
            return AstExpression::makeAstItemContinueStmt();
        }

        Maybe parseBlockStmt()
        {
            AstExpression::ExprCodeBlock* block;
            AstExpression* res;
            block = parseCodeBlock();
            if(!block)
            {
                return nullptr;
            }
            res = AstExpression::makeAstItemBlockStmt(block);
            return res;
        }

        Maybe parseImportStmt()
        {
            char* processedname;
            AstExpression* res;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_STRING))
            {
                return nullptr;
            }
            processedname = processAndCopyString(m_lexer.m_currtoken.m_tokstrdata, m_lexer.m_currtoken.m_tokstrlength);
            if(!processedname)
            {
                m_prserrlist->pushFormat(MC_ERROR_PARSING, m_lexer.m_currtoken.m_tokpos, "error when parsing module name");
                return nullptr;
            }
            m_lexer.nextToken();
            res = AstExpression::makeAstItemImportExpr(processedname);
            return res;
        }

        Maybe parseRecoverStmt()
        {
            AstExpression* res;
            AstExpression::ExprIdent* eid;
            AstExpression::ExprCodeBlock* body;
            eid = nullptr;
            body = nullptr;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_LPAREN))
            {
                return nullptr;
            }
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_IDENT))
            {
                return nullptr;
            }
            eid = Memory::make<AstExpression::ExprIdent>(m_lexer.m_currtoken);
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
            {
                goto err;
            }
            m_lexer.nextToken();
            body = parseCodeBlock();
            if(!body)
            {
                goto err;
            }
            res = AstExpression::makeAstItemRecover(eid, body);
            return res;
        err:
            Memory::destroy(body);
            Memory::destroy(eid);
            return nullptr;
        }

        Maybe parseLoopForBaseStmt()
        {
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_LPAREN))
            {
                return nullptr;
            }
            m_lexer.nextToken();
            if(m_lexer.currentTokenIs(AstToken::TOK_IDENT) && m_lexer.peekTokenIs(AstToken::TOK_IN))
            {
                return parseLoopForeachStmt();
            }
            return thisparseLoopForClassicStmt();
        }

        Maybe parseLoopForeachStmt()
        {
            AstExpression* res;
            AstExpression* source;
            AstExpression::ExprCodeBlock* body;
            AstExpression::ExprIdent* iteratorident;
            source = nullptr;
            body = nullptr;
            iteratorident = nullptr;
            iteratorident = Memory::make<AstExpression::ExprIdent>(m_lexer.m_currtoken);
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_IN))
            {
                goto err;
            }
            m_lexer.nextToken();
            source = parseExpression(MC_ASTPREC_LOWEST);
            if(!source)
            {
                goto err;
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
            {
                goto err;
            }
            m_lexer.nextToken();
            body = parseCodeBlock();
            if(!body)
            {
                goto err;
            }
            res = AstExpression::makeAstItemForeachStmt(iteratorident, source, body);
            return res;
        err:
            Memory::destroy(body);
            Memory::destroy(iteratorident);
            AstExpression::destroyExpression(source);
            return nullptr;
        }

        Maybe thisparseLoopForClassicStmt()
        {
            AstExpression* res;
            AstExpression* init;
            AstExpression* test;
            AstExpression* update;
            AstExpression::ExprCodeBlock* body;
            init = nullptr;
            test = nullptr;
            update = nullptr;
            body = nullptr;
            if(!m_lexer.currentTokenIs(AstToken::TOK_SEMICOLON))
            {
                init = parseStatement();
                if(!init)
                {
                    goto err;
                }
                if(init->m_exprtype != AstExpression::EXPR_STMTDEFINE && init->m_exprtype != AstExpression::EXPR_STMTEXPRESSION)
                {
                    m_prserrlist->pushFormat(MC_ERROR_PARSING, init->m_exprpos, "expected a definition or expression as 'for' loop init clause");
                    goto err;
                }
                if(!m_lexer.expectCurrent(AstToken::TOK_SEMICOLON))
                {
                    goto err;
                }
            }
            m_lexer.nextToken();
            if(!m_lexer.currentTokenIs(AstToken::TOK_SEMICOLON))
            {
                test = parseExpression(MC_ASTPREC_LOWEST);
                if(!test)
                {
                    goto err;
                }
                if(!m_lexer.expectCurrent(AstToken::TOK_SEMICOLON))
                {
                    goto err;
                }
            }
            m_lexer.nextToken();
            if(!m_lexer.currentTokenIs(AstToken::TOK_RPAREN))
            {
                update = parseExpression(MC_ASTPREC_LOWEST);
                if(!update)
                {
                    goto err;
                }
                if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
                {
                    goto err;
                }
            }
            m_lexer.nextToken();
            body = parseCodeBlock();
            if(!body)
            {
                goto err;
            }
            res = AstExpression::makeAstItemForLoopStmt(init, test, update, body);
            return res;
        err:
            AstExpression::destroyExpression(init);
            AstExpression::destroyExpression(test);
            AstExpression::destroyExpression(update);
            Memory::destroy(body);
            return nullptr;
        }

        AstExpression::ExprCodeBlock* parseCodeBlock()
        {
            bool ok;
            AstExpression::ExprCodeBlock* res;
            AstExpression* expr;
            PtrList* statements;
            (void)ok;
            if(!m_lexer.expectCurrent(AstToken::TOK_LBRACE))
            {
                return nullptr;
            }
            m_lexer.nextToken();
            m_parsedepth++;
            statements = Memory::make<PtrList>(sizeof(void*), true);
            while(!m_lexer.currentTokenIs(AstToken::TOK_RBRACE))
            {
                if(m_lexer.currentTokenIs(AstToken::TOK_EOF))
                {
                    m_prserrlist->pushFormat(MC_ERROR_PARSING, m_lexer.m_currtoken.m_tokpos, "unexpected EOF");
                    goto err;
                }
                if(m_lexer.currentTokenIs(AstToken::TOK_SEMICOLON))
                {
                    m_lexer.nextToken();
                    continue;
                }
                expr = parseStatement();
                if(!expr)
                {
                    goto err;
                }
                ok = statements->push(expr);
            }
            m_lexer.nextToken();
            m_parsedepth--;
            res = Memory::make<AstExpression::ExprCodeBlock>(statements);
            return res;
        err:
            m_parsedepth--;
            PtrList::destroy(statements, (mcitemdestroyfn_t)AstExpression::destroyExpression);
            return nullptr;
        }

        Maybe parseExpression(mcastprecedence_t prec)
        {
            char* literal;
            AstLocation pos;
            mcastleftassocparsefn_t parseleftassoc;
            mcastrightassocparsefn_t parserightassoc;
            AstExpression* newleftexpr;
            AstExpression* leftexpr;
            pos = m_lexer.m_currtoken.m_tokpos;
            if(m_lexer.m_currtoken.type() == AstToken::TOK_INVALID)
            {
                m_prserrlist->pushFormat(MC_ERROR_PARSING, m_lexer.m_currtoken.m_tokpos, "illegal token");
                return nullptr;
            }
            parserightassoc = getRightAssocParseFunc(m_lexer.m_currtoken.type());
            if(!parserightassoc)
            {
                literal = m_lexer.m_currtoken.dupLiteralString();
                m_prserrlist->pushFormat(MC_ERROR_PARSING, m_lexer.m_currtoken.m_tokpos, "no prefix parse function for \"%s\" found", literal);
                mc_memory_free(literal);
                return nullptr;
            }
            leftexpr = parserightassoc(this);
            if(!leftexpr)
            {
                return nullptr;
            }
            leftexpr->m_exprpos = pos;
            while(!m_lexer.currentTokenIs(AstToken::TOK_SEMICOLON) && prec < getPrecedence(m_lexer.m_currtoken.type()))
            {
                parseleftassoc = getLeftAssocParseFunc(m_lexer.m_currtoken.m_toktype);
                if(!parseleftassoc)
                {
                    return leftexpr;
                }
                pos = m_lexer.m_currtoken.m_tokpos;
                newleftexpr = parseleftassoc(this, leftexpr);
                if(!newleftexpr)
                {
                    AstExpression::destroyExpression(leftexpr);
                    return nullptr;
                }
                newleftexpr->m_exprpos = pos;
                leftexpr = newleftexpr;
            }
            return leftexpr;
        }

        AstExpression* parseGroupedExpr()
        {
            AstExpression* expr;
            m_lexer.nextToken();
            expr = parseExpression(MC_ASTPREC_LOWEST);
            if(!expr || !m_lexer.expectCurrent(AstToken::TOK_RPAREN))
            {
                AstExpression::destroyExpression(expr);
                return nullptr;
            }
            m_lexer.nextToken();
            return expr;
        }


        bool parseFuncParams(PtrList* outparams)
        {
            bool ok;
            AstExpression::ExprIdent* ident;
            AstExpression::ExprFuncParam* param;
            (void)ok;
            if(!m_lexer.expectCurrent(AstToken::TOK_LPAREN))
            {
                return false;
            }
            m_lexer.nextToken();
            if(m_lexer.currentTokenIs(AstToken::TOK_RPAREN))
            {
                m_lexer.nextToken();
                return true;
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_IDENT))
            {
                return false;
            }
            ident = Memory::make<AstExpression::ExprIdent>(m_lexer.m_currtoken);
            param = Memory::make<AstExpression::ExprFuncParam>(ident);
            ok = outparams->push(param);
            m_lexer.nextToken();
            while(m_lexer.currentTokenIs(AstToken::TOK_COMMA))
            {
                m_lexer.nextToken();
                if(!m_lexer.expectCurrent(AstToken::TOK_IDENT))
                {
                    return false;
                }
                ident = Memory::make<AstExpression::ExprIdent>(m_lexer.m_currtoken);
                param = Memory::make<AstExpression::ExprFuncParam>(ident);
                ok = outparams->push(param);
                m_lexer.nextToken();
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_RPAREN))
            {
                return false;
            }
            m_lexer.nextToken();
            return true;
        }

        AstExpression* parseFunctionStmt()
        {
            AstExpression::ExprIdent* nameident;
            AstExpression* res;
            AstExpression* value;
            AstLocation pos;
            nameident = nullptr;
            value = nullptr;
            pos = m_lexer.m_currtoken.m_tokpos;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_IDENT))
            {
                goto err;
            }
            nameident = Memory::make<AstExpression::ExprIdent>(m_lexer.m_currtoken);
            m_lexer.nextToken();
            value = callback_parseliteralfunction(this);
            if(!value)
            {
                goto err;
            }
            value->m_exprpos = pos;
            value->m_uexpr.exprlitfunction.name = mc_util_strdup(nameident->m_identvalue);
            if(!value->m_uexpr.exprlitfunction.name)
            {
                goto err;
            }
            res = AstExpression::makeAstItemDefine(nameident, value, false);
            return res;
        err:
            AstExpression::destroyExpression(value);
            Memory::destroy(nameident);
            return nullptr;
        }

        //xxhere

        AstExpression* parseTernaryExpr(AstExpression* left)
        {
            AstExpression* res;
            AstExpression* ift;
            AstExpression* iffalse;
            m_lexer.nextToken();
            ift = parseExpression(MC_ASTPREC_LOWEST);
            if(!ift)
            {
                return nullptr;
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_COLON))
            {
                AstExpression::destroyExpression(ift);
                return nullptr;
            }
            m_lexer.nextToken();
            iffalse = parseExpression(MC_ASTPREC_LOWEST);
            if(!iffalse)
            {
                AstExpression::destroyExpression(ift);
                return nullptr;
            }
            res = AstExpression::makeAstItemTernary(left, ift, iffalse);
            return res;
        }

        AstExpression* parseLogicalExpr(AstExpression* left)
        {
            mcastmathoptype_t op;
            mcastprecedence_t prec;
            AstExpression* res;
            AstExpression* right;
            op = tokenToMathOP(m_lexer.m_currtoken.m_toktype);
            prec = getPrecedence(m_lexer.m_currtoken.m_toktype);
            m_lexer.nextToken();
            right = parseExpression(prec);
            if(!right)
            {
                return nullptr;
            }
            res = AstExpression::makeAstItemLogical(op, left, right);
            return res;
        }

        AstExpression* parseIndexExpr(AstExpression* left)
        {
            AstExpression* res;
            AstExpression* index;
            m_lexer.nextToken();
            index = parseExpression(MC_ASTPREC_LOWEST);
            if(!index)
            {
                return nullptr;
            }
            if(!m_lexer.expectCurrent(AstToken::TOK_RBRACKET))
            {
                AstExpression::destroyExpression(index);
                return nullptr;
            }
            m_lexer.nextToken();
            res = AstExpression::makeAstItemIndex(left, index, false);
            return res;
        }

        AstExpression* parseAssignExpr(AstExpression* left)
        {
            AstLocation pos;
            mcastmathoptype_t op;
            AstToken::Type assigntype;
            AstExpression* res;
            AstExpression* source;
            AstExpression* leftcopy;
            AstExpression* newsource;
            source = nullptr;
            assigntype = m_lexer.m_currtoken.m_toktype;
            m_lexer.nextToken();
            source = parseExpression(MC_ASTPREC_LOWEST);
            if(!source)
            {
                goto err;
            }
            switch(assigntype)
            {
                case AstToken::TOK_ASSIGNPLUS:
                case AstToken::TOK_ASSIGNMINUS:
                case AstToken::TOK_ASSIGNSLASH:
                case AstToken::TOK_ASSIGNASTERISK:
                case AstToken::TOK_ASSIGNPERCENT:
                case AstToken::TOK_ASSIGNBINAND:
                case AstToken::TOK_ASSIGNBINOR:
                case AstToken::TOK_ASSIGNBINXOR:
                case AstToken::TOK_ASSIGNLSHIFT:
                case AstToken::TOK_ASSIGNRSHIFT:
                    {
                        op = tokenToMathOP(assigntype);
                        leftcopy = AstExpression::copyExpression(left);
                        if(!leftcopy)
                        {
                            goto err;
                        }
                        pos = source->m_exprpos;
                        newsource = AstExpression::makeAstItemInfix(op, leftcopy, source);
                        newsource->m_exprpos = pos;
                        source = newsource;
                    }
                    break;
                case AstToken::TOK_ASSIGN:
                    {
                    }
                    break;
                default:
                    {
                        MC_ASSERT(false);
                    }
                    break;
            }
            res = AstExpression::makeAstItemAssign(left, source, false);
            return res;
        err:
            AstExpression::destroyExpression(source);
            return nullptr;
        }

        AstExpression* parseIncDecPrefixExpr()
        {
            AstLocation pos;
            mcastmathoptype_t op;
            AstToken::Type operationtype;
            AstExpression* res;
            AstExpression* dest;
            AstExpression* source;
            AstExpression* destcopy;
            AstExpression* operation;
            AstExpression* oneliteral;
            source = nullptr;
            operationtype = m_lexer.m_currtoken.m_toktype;
            pos = m_lexer.m_currtoken.m_tokpos;
            m_lexer.nextToken();
            op = tokenToMathOP(operationtype);
            dest = parseExpression(MC_ASTPREC_PREFIX);
            if(!dest)
            {
                goto err;
            }
            oneliteral = AstExpression::makeAstItemLiteralNumber(1);
            oneliteral->m_exprpos = pos;
            destcopy = AstExpression::copyExpression(dest);
            if(!destcopy)
            {
                AstExpression::destroyExpression(oneliteral);
                AstExpression::destroyExpression(dest);
                goto err;
            }
            operation = AstExpression::makeAstItemInfix(op, destcopy, oneliteral);
            operation->m_exprpos = pos;
            res = AstExpression::makeAstItemAssign(dest, operation, false);
            return res;
        err:
            AstExpression::destroyExpression(source);
            return nullptr;
        }

        AstExpression* parseIncDecPostfixExpr(AstExpression* left)
        {
            AstLocation pos;
            mcastmathoptype_t op;
            AstToken::Type operationtype;
            AstExpression* res;
            AstExpression* source;
            AstExpression* leftcopy;
            AstExpression* operation;
            AstExpression* oneliteral;
            source = nullptr;
            operationtype = m_lexer.m_currtoken.m_toktype;
            pos = m_lexer.m_currtoken.m_tokpos;
            m_lexer.nextToken();
            op = tokenToMathOP(operationtype);
            leftcopy = AstExpression::copyExpression(left);
            if(!leftcopy)
            {
                goto err;
            }
            oneliteral = AstExpression::makeAstItemLiteralNumber(1);
            oneliteral->m_exprpos = pos;
            operation = AstExpression::makeAstItemInfix(op, leftcopy, oneliteral);
            operation->m_exprpos = pos;
            res = AstExpression::makeAstItemAssign(left, operation, true);
            return res;
        err:
            AstExpression::destroyExpression(source);
            return nullptr;
        }

        AstExpression* parsePrefixExpr()
        {
            mcastmathoptype_t op;
            AstExpression* res;
            AstExpression* right;
            op = tokenToMathOP(m_lexer.m_currtoken.m_toktype);
            m_lexer.nextToken();
            right = parseExpression(MC_ASTPREC_PREFIX);
            if(!right)
            {
                return nullptr;
            }
            res = AstExpression::makeAstItemPrefix(op, right);
            return res;
        }

        AstExpression* parseInfixExpr(AstExpression* left)
        {
            mcastmathoptype_t op;
            mcastprecedence_t prec;
            AstExpression* res;
            AstExpression* right;
            op = tokenToMathOP(m_lexer.m_currtoken.m_toktype);
            prec = getPrecedence(m_lexer.m_currtoken.m_toktype);
            m_lexer.nextToken();
            right = parseExpression(prec);
            if(!right)
            {
                return nullptr;
            }
            res = AstExpression::makeAstItemInfix(op, left, right);
            return res;
        }

        AstExpression* parseLiteralFunction()
        {
            bool ok;
            PtrList* params;
            AstExpression::ExprCodeBlock* body;
            AstExpression* res;
            (void)ok;
            m_parsedepth++;
            params = nullptr;
            body = nullptr;
            if(m_lexer.currentTokenIs(AstToken::TOK_FUNCTION))
            {
                m_lexer.nextToken();
            }
            params = Memory::make<PtrList>(sizeof(void*), true);
            ok = parseFuncParams(params);
            if(!ok)
            {
                goto err;
            }
            body = parseCodeBlock();
            if(!body)
            {
                goto err;
            }
            res = AstExpression::makeAstItemLiteralFunction(params, body);
            m_parsedepth -= 1;
            return res;
        err:
            Memory::destroy(body);
            PtrList::destroy(params, (mcitemdestroyfn_t)AstExpression::ExprFuncParam::destroy);
            m_parsedepth -= 1;
            return nullptr;
        }

        AstExpression* parseLiteralArray()
        {
            PtrList* array;
            AstExpression* res;
            array = parseExprList(AstToken::TOK_LBRACKET, AstToken::TOK_RBRACKET, true);
            if(!array)
            {
                return nullptr;
            }
            res = AstExpression::makeAstItemLiteralArray(array);
            return res;
        }

        AstExpression* parseLiteralMap()
        {
            bool ok;
            size_t len;
            char* str;
            PtrList* keys;
            PtrList* values;
            AstExpression* res;
            AstExpression* key;
            AstExpression* value;
            (void)ok;
            keys = Memory::make<PtrList>(sizeof(void*), true);
            values = Memory::make<PtrList>(sizeof(void*), true);
            if(!keys || !values)
            {
                goto err;
            }
            m_lexer.nextToken();
            while(!m_lexer.currentTokenIs(AstToken::TOK_RBRACE))
            {
                key = nullptr;
                if(m_lexer.currentTokenIs(AstToken::TOK_IDENT))
                {
                    str = m_lexer.m_currtoken.dupLiteralString();
                    len = mc_util_strlen(str);
                    key = AstExpression::makeAstItemLiteralString(str, len);
                    key->m_exprpos = m_lexer.m_currtoken.m_tokpos;
                    m_lexer.nextToken();
                }
                else
                {
                    key = parseExpression(MC_ASTPREC_LOWEST);
                    if(!key)
                    {
                        goto err;
                    }
                    switch(key->m_exprtype)
                    {
                        case AstExpression::EXPR_STRINGLITERAL:
                        case AstExpression::EXPR_NUMBERLITERAL:
                        case AstExpression::EXPR_BOOLLITERAL:
                            {
                            }
                            break;
                        default:
                            {
                                m_prserrlist->pushFormat(MC_ERROR_PARSING, key->m_exprpos, "can only use primitive types as literal 'map' object keys");
                                AstExpression::destroyExpression(key);
                                goto err;
                            }
                            break;
                    }
                }
                ok = keys->push(key);
                if(!m_lexer.expectCurrent(AstToken::TOK_COLON))
                {
                    goto err;
                }
                m_lexer.nextToken();
                value = parseExpression(MC_ASTPREC_LOWEST);
                if(!value)
                {
                    goto err;
                }
                ok = values->push(value);
                if(m_lexer.currentTokenIs(AstToken::TOK_RBRACE))
                {
                    break;
                }
                if(!m_lexer.expectCurrent(AstToken::TOK_COMMA))
                {
                    goto err;
                }
                m_lexer.nextToken();
            }
            m_lexer.nextToken();
            res = AstExpression::makeAstItemLiteralMap(keys, values);
            return res;
        err:
            PtrList::destroy(keys, (mcitemdestroyfn_t)AstExpression::destroyExpression);
            PtrList::destroy(values, (mcitemdestroyfn_t)AstExpression::destroyExpression);
            return nullptr;
        }

        AstExpression* parseLiteralTemplateString()
        {
            size_t len;
            char* processedliteral;
            AstLocation pos;
            AstExpression* leftstringexpr;
            AstExpression* templateexpr;
            AstExpression* tostrcallexpr;
            AstExpression* leftaddexpr;
            AstExpression* rightexpr;
            AstExpression* rightaddexpr;
            processedliteral = nullptr;
            leftstringexpr = nullptr;
            templateexpr = nullptr;
            tostrcallexpr = nullptr;
            leftaddexpr = nullptr;
            rightexpr = nullptr;
            rightaddexpr = nullptr;
            processedliteral = processAndCopyString(m_lexer.m_currtoken.m_tokstrdata, m_lexer.m_currtoken.m_tokstrlength);
            if(!processedliteral)
            {
                m_prserrlist->pushFormat(MC_ERROR_PARSING, m_lexer.m_currtoken.m_tokpos, "error parsing string literal");
                return nullptr;
            }
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_LBRACE))
            {
                goto err;
            }
            m_lexer.nextToken();
            pos = m_lexer.m_currtoken.m_tokpos;
            len = mc_util_strlen(processedliteral);
            leftstringexpr = AstExpression::makeAstItemLiteralString(processedliteral, len);
            leftstringexpr->m_exprpos = pos;
            processedliteral = nullptr;
            pos = m_lexer.m_currtoken.m_tokpos;
            templateexpr = parseExpression(MC_ASTPREC_LOWEST);
            if(!templateexpr)
            {
                goto err;
            }
            tostrcallexpr = AstExpression::makeAstItemInlineCall(templateexpr, "tostring");
            tostrcallexpr->m_exprpos = pos;
            templateexpr = nullptr;
            leftaddexpr = AstExpression::makeAstItemInfix(MC_MATHOP_PLUS, leftstringexpr, tostrcallexpr);
            leftaddexpr->m_exprpos = pos;
            leftstringexpr = nullptr;
            tostrcallexpr = nullptr;
            if(!m_lexer.expectCurrent(AstToken::TOK_RBRACE))
            {
                goto err;
            }
            m_lexer.previousToken();
            m_lexer.conttplstring();
            m_lexer.nextToken();
            m_lexer.nextToken();
            pos = m_lexer.m_currtoken.m_tokpos;
            rightexpr = parseExpression(MC_ASTPREC_HIGHEST);
            if(!rightexpr)
            {
                goto err;
            }
            rightaddexpr = AstExpression::makeAstItemInfix(MC_MATHOP_PLUS, leftaddexpr, rightexpr);
            rightaddexpr->m_exprpos = pos;
            leftaddexpr = nullptr;
            rightexpr = nullptr;
            return rightaddexpr;
        err:
            AstExpression::destroyExpression(rightaddexpr);
            AstExpression::destroyExpression(rightexpr);
            AstExpression::destroyExpression(leftaddexpr);
            AstExpression::destroyExpression(tostrcallexpr);
            AstExpression::destroyExpression(templateexpr);
            AstExpression::destroyExpression(leftstringexpr);
            mc_memory_free(processedliteral);
            return nullptr;
        }

        AstExpression* parseLiteralString()
        {
            size_t len;
            char* processedliteral;
            AstExpression* res;
            processedliteral = processAndCopyString(m_lexer.m_currtoken.m_tokstrdata, m_lexer.m_currtoken.m_tokstrlength);
            if(!processedliteral)
            {
                m_prserrlist->pushFormat(MC_ERROR_PARSING, m_lexer.m_currtoken.m_tokpos, "error parsing string literal");
                return nullptr;
            }
            m_lexer.nextToken();
            len = mc_util_strlen(processedliteral);
            res = AstExpression::makeAstItemLiteralString(processedliteral, len);
            return res;
        }

        AstExpression* parseLiteralNull()
        {
            m_lexer.nextToken();
            return AstExpression::makeAstItemLiteralNull();
        }

        AstExpression* parseLiteralBool()
        {
            AstExpression* res;
            res = AstExpression::makeAstItemLiteralBool(m_lexer.m_currtoken.m_toktype == AstToken::TOK_TRUE);
            m_lexer.nextToken();
            return res;
        }

        AstExpression* parseLiteralNumber()
        {
            mcfloat_t number;
            long parsedlen;
            char* end;
            char* literal;
            number = 0;
            errno = 0;
            number = mc_util_strtod(m_lexer.m_currtoken.m_tokstrdata, m_lexer.m_currtoken.m_tokstrlength, &end);
            #if 0
                fprintf(stderr, "literal=<%s> number=<%f>\n", m_lexer.m_currtoken.m_tokstrdata, number);
            #endif
            parsedlen = end - m_lexer.m_currtoken.m_tokstrdata;
            if(errno || parsedlen != m_lexer.m_currtoken.m_tokstrlength)
            {
                literal = m_lexer.m_currtoken.dupLiteralString();
                m_prserrlist->pushFormat(MC_ERROR_PARSING, m_lexer.m_currtoken.m_tokpos, "failed to parse number literal \"%s\"", literal);
                mc_memory_free(literal);
                return nullptr;
            }    
            m_lexer.nextToken();
            return AstExpression::makeAstItemLiteralNumber(number);
        }

        AstExpression* parseDotExpr(AstExpression* left)
        {
            size_t len;
            char* str;
            AstExpression* res;
            AstExpression* index;
            m_lexer.nextToken();
            if(!m_lexer.expectCurrent(AstToken::TOK_IDENT))
            {
                return nullptr;
            }
            str = m_lexer.m_currtoken.dupLiteralString();
            len = mc_util_strlen(str);
            index = AstExpression::makeAstItemLiteralString(str, len);
            index->m_exprpos = m_lexer.m_currtoken.m_tokpos;
            m_lexer.nextToken();
            res = AstExpression::makeAstItemIndex(left, index, true);
            return res;
        }
        
        AstExpression* parseIdent()
        {
            AstExpression::ExprIdent* ident;
            AstExpression* res;
            ident = Memory::make<AstExpression::ExprIdent>(m_lexer.m_currtoken);
            res = AstExpression::makeAstItemIdent(ident);
            m_lexer.nextToken();
            return res;
        }

        AstExpression* parseCallExpr(AstExpression* left)
        {
            PtrList* args;
            AstExpression* res;
            AstExpression* function;
            function = left;
            args = parseExprList(AstToken::TOK_LPAREN, AstToken::TOK_RPAREN, false);
            if(!args)
            {
                return nullptr;
            }
            res = AstExpression::makeAstItemCallExpr(function, args);
            return res;
        }

        PtrList* parseExprList(AstToken::Type starttoken, AstToken::Type endtoken, bool trailingcommaallowed)
        {
            bool ok;
            PtrList* res;
            AstExpression* argexpr;
            (void)ok;
            if(!m_lexer.expectCurrent(starttoken))
            {
                return nullptr;
            }
            m_lexer.nextToken();
            res = Memory::make<PtrList>(sizeof(void*), true);
            if(m_lexer.currentTokenIs(endtoken))
            {
                m_lexer.nextToken();
                return res;
            }
            argexpr = parseExpression(MC_ASTPREC_LOWEST);
            if(!argexpr)
            {
                goto err;
            }
            ok = res->push(argexpr);
            while(m_lexer.currentTokenIs(AstToken::TOK_COMMA))
            {
                m_lexer.nextToken();
                if(trailingcommaallowed && m_lexer.currentTokenIs(endtoken))
                {
                    break;
                }
                argexpr = parseExpression(MC_ASTPREC_LOWEST);
                if(!argexpr)
                {
                    goto err;
                }
                ok = res->push(argexpr);
            }
            if(!m_lexer.expectCurrent(endtoken))
            {
                goto err;
            }
            m_lexer.nextToken();
            return res;
        err:
            PtrList::destroy(res, (mcitemdestroyfn_t)AstExpression::destroyExpression);
            return nullptr;
        }

        AstExpression* parseStatement()
        {
            AstLocation pos;
            AstExpression* expr;
            pos = m_lexer.m_currtoken.m_tokpos;
            expr = nullptr;
            switch(m_lexer.m_currtoken.m_toktype)
            {
                case AstToken::TOK_VAR:
                case AstToken::TOK_CONST:
                    {
                        expr = parseVarLetStmt();
                    }
                    break;
                case AstToken::TOK_IF:
                    {
                        expr = parseIfStmt();
                    }
                    break;
                case AstToken::TOK_RETURN:
                    {
                        expr = parseReturnStmt();
                    }
                    break;
                case AstToken::TOK_WHILE:
                    {
                        expr = parseLoopWhileStmt();
                    }
                    break;
                case AstToken::TOK_BREAK:
                    {
                        expr = parseBreakStmt();
                    }
                    break;
                case AstToken::TOK_FOR:
                    {
                        expr = parseLoopForBaseStmt();
                    }
                    break;
                case AstToken::TOK_FUNCTION:
                    {
                        if(m_lexer.peekTokenIs(AstToken::TOK_IDENT))
                        {
                            expr = parseFunctionStmt();
                        }
                        else
                        {
                            expr = parseExprStmt();
                        }
                    }
                    break;
                case AstToken::TOK_LBRACE:
                    {
                        if(m_config->replmode && m_parsedepth == 0)
                        {
                            expr = parseExprStmt();
                        }
                        else
                        {
                            expr = parseBlockStmt();
                        }
                    }
                    break;
                case AstToken::TOK_CONTINUE:
                    {
                        expr = parseContinueStmt();
                    }
                    break;
                case AstToken::TOK_IMPORT:
                    {
                        expr = parseImportStmt();
                    }
                    break;
                case AstToken::TOK_RECOVER:
                    {
                        expr = parseRecoverStmt();
                    }
                    break;
                default:
                    {
                        expr = parseExprStmt();
                    }
                    break;
            }
            if(expr)
            {
                expr->m_exprpos = pos;
            }
            return expr;
        }

        PtrList* parseAll(const char* input, AstSourceFile* file)
        {
            bool ok;
            AstExpression* expr;
            PtrList* statements;
            (void)ok;
            m_parsedepth = 0;
            ok = AstLexer::init(&m_lexer, m_prserrlist, input, file);
            if(!ok)
            {
                return nullptr;
            }
            m_lexer.nextToken();
            m_lexer.nextToken();
            statements = Memory::make<PtrList>(sizeof(void*), true);
            while(!m_lexer.currentTokenIs(AstToken::TOK_EOF))
            {
                if(m_lexer.currentTokenIs(AstToken::TOK_SEMICOLON))
                {
                    m_lexer.nextToken();
                    continue;
                }
                expr = parseStatement();
                if(!expr)
                {
                    goto err;
                }
                ok = statements->push(expr);
            }
            if(m_prserrlist->count() > 0)
            {
                goto err;
            }
            return statements;
        err:
            PtrList::destroy(statements, (mcitemdestroyfn_t)AstExpression::destroyExpression);
            return nullptr;
        }


};
