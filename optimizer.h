
mcastexpression_t* mc_optimizer_optexpression(mcastexpression_t* expr)
{
    switch(expr->exprtype)
    {
        case MC_EXPR_INFIX:
            return mc_optimizer_optinfixexpr(expr);
        case MC_EXPR_PREFIX:
            return mc_optimizer_optprefixexpr(expr);
        default:
            break;
    }
    return NULL;
}

mcastexpression_t* mc_optimizer_optinfixexpr(mcastexpression_t* expr)
{
    bool leftisnumeric;
    bool rightisnumeric;
    bool leftisstring;
    bool rightisstring;
    mcfloat_t dnleft;
    mcfloat_t dnright;
    size_t len;
    mcstate_t* state;
    mcastexpression_t* res;
    mcastexpression_t* left;
    mcastexpression_t* right;
    mcastexpression_t* leftoptimized;
    mcastexpression_t* rightoptimized;
    state = expr->pstate;
    left = expr->uexpr.exprinfix.left;
    leftoptimized = mc_optimizer_optexpression(left);
    if(leftoptimized)
    {
        left = leftoptimized;
    }
    right = expr->uexpr.exprinfix.right;
    rightoptimized = mc_optimizer_optexpression(right);
    if(rightoptimized)
    {
        right = rightoptimized;
    }
    res = NULL;
    leftisnumeric = left->exprtype == MC_EXPR_NUMBERLITERAL || left->exprtype == MC_EXPR_BOOLLITERAL;
    rightisnumeric = right->exprtype == MC_EXPR_NUMBERLITERAL || right->exprtype == MC_EXPR_BOOLLITERAL;
    leftisstring = left->exprtype == MC_EXPR_STRINGLITERAL;
    rightisstring = right->exprtype == MC_EXPR_STRINGLITERAL;
    if(leftisnumeric && rightisnumeric)
    {
        dnleft = left->exprtype == MC_EXPR_NUMBERLITERAL ? left->uexpr.exprlitnumber : left->uexpr.exprlitbool;
        dnright = right->exprtype == MC_EXPR_NUMBERLITERAL ? right->uexpr.exprlitnumber : right->uexpr.exprlitbool;
        switch(expr->uexpr.exprinfix.op)
        {
            case MC_MATHOP_PLUS:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_add(dnleft, dnright));
                }
                break;
            case MC_MATHOP_MINUS:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_sub(dnleft, dnright));
                }
                break;
            case MC_MATHOP_ASTERISK:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_mult(dnleft, dnright));
                }
                break;
            case MC_MATHOP_SLASH:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_div(dnleft, dnright));
                }
                break;
            case MC_MATHOP_LT:
                {
                    res = mc_astexpr_makeliteralbool(state, dnleft < dnright);
                }
                break;
            case MC_MATHOP_LTE:
                {
                    res = mc_astexpr_makeliteralbool(state, dnleft <= dnright);
                }
                break;
            case MC_MATHOP_GT:
                {
                    res = mc_astexpr_makeliteralbool(state, dnleft > dnright);
                }
                break;
            case MC_MATHOP_GTE:
                {
                    res = mc_astexpr_makeliteralbool(state, dnleft >= dnright);
                }
                break;
            case MC_MATHOP_EQ:
                {
                    res = mc_astexpr_makeliteralbool(state, MC_UTIL_CMPFLOAT(dnleft, dnright));
                }
                break;
            case MC_MATHOP_NOTEQ:
                {
                    res = mc_astexpr_makeliteralbool(state, !MC_UTIL_CMPFLOAT(dnleft, dnright));
                }
                break;
            case MC_MATHOP_MODULUS:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_mod(dnleft, dnright));
                }
                break;
            case MC_MATHOP_BINAND:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_binand(dnleft, dnright));
                }
                break;
            case MC_MATHOP_BINOR:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_binor(dnleft, dnright));
                }
                break;
            case MC_MATHOP_BINXOR:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_binxor(dnleft, dnright));
                }
                break;
            case MC_MATHOP_LSHIFT:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_binshiftleft(dnleft, dnright));
                }
                break;
            case MC_MATHOP_RSHIFT:
                {
                    res = mc_astexpr_makeliteralnumber(state, mc_mathutil_binshiftright(dnleft, dnright));
                }
                break;
            default:
                {
                }
                break;
        }
    }
    else if(expr->uexpr.exprinfix.op == MC_MATHOP_PLUS && leftisstring && rightisstring)
    {
        /* TODO:FIXME: horrible method of joining strings!!!!!!! */
        char* resstr;
        const char* strleft;
        const char* strright;
        strleft = left->uexpr.exprlitstring.data;
        strright = right->uexpr.exprlitstring.data;
        resstr = mc_util_stringallocfmt(state, "%s%s", strleft, strright);
        len = mc_util_strlen(resstr);
        if(resstr)
        {
            res = mc_astexpr_makeliteralstring(state, resstr, len);
            if(!res)
            {
                mc_memory_free(resstr);
            }
        }
    }
    mc_astexpr_destroy(leftoptimized);
    mc_astexpr_destroy(rightoptimized);
    if(res)
    {
        res->pos = expr->pos;
    }
    return res;
}

mcastexpression_t* mc_optimizer_optprefixexpr(mcastexpression_t* expr)
{
    mcastexpression_t* res;
    mcastexpression_t* right;
    mcastexpression_t* rightoptimized;
    right = expr->uexpr.exprprefix.right;
    rightoptimized = mc_optimizer_optexpression(right);
    if(rightoptimized)
    {
        right = rightoptimized;
    }
    res = NULL;
    if(expr->uexpr.exprprefix.op == MC_MATHOP_MINUS && right->exprtype == MC_EXPR_NUMBERLITERAL)
    {
        res = mc_astexpr_makeliteralnumber(expr->pstate, -right->uexpr.exprlitnumber);
    }
    else if(expr->uexpr.exprprefix.op == MC_MATHOP_BANG && right->exprtype == MC_EXPR_BOOLLITERAL)
    {
        res = mc_astexpr_makeliteralbool(expr->pstate, !right->uexpr.exprlitbool);
    }
    mc_astexpr_destroy(rightoptimized);
    if(res)
    {
        res->pos = expr->pos;
    }
    return res;
}

