
bool mc_lexer_init(mcastlexer_t* lex, mcstate_t* state, mcerrlist_t* errs, const char* input, mcastcompiledfile_t* file)
{
    bool ok;
    lex->pstate = state;
    lex->errors = errs;
    lex->inputsource = input;
    lex->inputlength = (int)mc_util_strlen(input);
    lex->position = 0;
    lex->nextposition = 0;
    lex->ch = '\0';
    if(file)
    {
        lex->line = mc_ptrlist_count(file->lines);
    }
    else
    {
        lex->line = 0;
    }
    lex->column = -1;
    lex->file = file;
    ok = mc_lexer_addline(lex, 0);
    if(!ok)
    {
        return false;
    }
    ok = mc_lexer_readchar(lex);
    if(!ok)
    {
        return false;
    }
    lex->failed = false;
    lex->continuetplstring = false;
    memset(&lex->prevstate, 0, sizeof(lex->prevstate));
    mc_asttoken_init(&lex->prevtoken, MC_TOK_INVALID, NULL, 0);
    mc_asttoken_init(&lex->currtoken, MC_TOK_INVALID, NULL, 0);
    mc_asttoken_init(&lex->peektoken, MC_TOK_INVALID, NULL, 0);
    return true;
}

bool mc_lexer_failed(mcastlexer_t* lex)
{
    return lex->failed;
}

void mc_lexer_conttplstring(mcastlexer_t* lex)
{
    lex->continuetplstring = true;
}

bool mc_lexer_currtokenis(mcastlexer_t* lex, mcasttoktype_t type)
{
    return lex->currtoken.toktype == type;
}

bool mc_lexer_peektokenis(mcastlexer_t* lex, mcasttoktype_t type)
{
    return lex->peektoken.toktype == type;
}

bool mc_lexer_nexttoken(mcastlexer_t* lex)
{
    lex->prevtoken = lex->currtoken;
    lex->currtoken = lex->peektoken;
    lex->peektoken = mc_lexer_nexttokinternal(lex);
    return !lex->failed;
}

bool mc_lexer_previoustoken(mcastlexer_t* lex)
{
    if(lex->prevtoken.toktype == MC_TOK_INVALID)
    {
        return false;
    }
    lex->peektoken = lex->currtoken;
    lex->currtoken = lex->prevtoken;
    mc_asttoken_init(&lex->prevtoken, MC_TOK_INVALID, NULL, 0);
    lex->ch = lex->prevstate.ch;
    lex->column = lex->prevstate.column;
    lex->line = lex->prevstate.line;
    lex->position = lex->prevstate.position;
    lex->nextposition = lex->prevstate.nextposition;
    return true;
}

mcasttoken_t mc_lexer_nexttokinternal(mcastlexer_t* lex)
{
    char c;
    mcasttoken_t outtok;
    lex->prevstate.ch = lex->ch;
    lex->prevstate.column = lex->column;
    lex->prevstate.line = lex->line;
    lex->prevstate.position = lex->position;
    lex->prevstate.nextposition = lex->nextposition;
    while(true)
    {
        if(!lex->continuetplstring)
        {
            mc_lexer_skipspace(lex);
        }
        outtok.toktype = MC_TOK_INVALID;
        outtok.tokstrdata = lex->inputsource + lex->position;
        outtok.tokstrlen = 1;
        outtok.pos = mc_astlocation_make(lex->file, lex->line, lex->column);
        c = lex->continuetplstring ? '`' : lex->ch;
        switch(c)
        {
            case '\0':
                {
                    mc_asttoken_init(&outtok, MC_TOK_EOF, "EOF", 3);
                }
                break;
            case '=':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_EQ, "==", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGN, "=", 1);
                    }
                }
                break;
            case '&':
                {
                    if(mc_lexer_peekchar(lex) == '&')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_AND, "&&", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNBINAND, "&=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_BINAND, "&", 1);
                    }
                }
                break;
            case '|':
                {
                    if(mc_lexer_peekchar(lex) == '|')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_OR, "||", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNBINOR, "|=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_BINOR, "|", 1);
                    }
                }
                break;
            case '^':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNBINXOR, "^=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_BINXOR, "^", 1);
                        break;
                    }
                }
                break;
            case '+':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNPLUS, "+=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '+')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_PLUSPLUS, "++", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_PLUS, "+", 1);
                        break;
                    }
                }
                break;
            case '-':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNMINUS, "-=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '-')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_MINUSMINUS, "--", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_UNARYMINUS, "-", 1);
                        break;
                    }
                }
                break;
            case '~':
                {
                    mc_asttoken_init(&outtok, MC_TOK_UNARYBINNOT, "~", 1);
                }
                break;
            case '!':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_NOTEQ, "!=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_BANG, "!", 1);
                    }
                }
                break;
            case '*':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNASTERISK, "*=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASTERISK, "*", 1);
                        break;
                    }
                }
                break;
            case '/':
                {
                    if(mc_lexer_peekchar(lex) == '/')
                    {
                        mc_lexer_readchar(lex);
                        while(lex->ch != '\n' && lex->ch != '\0')
                        {
                            mc_lexer_readchar(lex);
                        }
                        continue;
                    }
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNSLASH, "/=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_SLASH, "/", 1);
                        break;
                    }
                }
                break;
            case '<':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_LTE, "<=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '<')
                    {
                        mc_lexer_readchar(lex);
                        if(mc_lexer_peekchar(lex) == '=')
                        {
                            mc_asttoken_init(&outtok, MC_TOK_ASSIGNLSHIFT, "<<=", 3);
                            mc_lexer_readchar(lex);
                        }
                        else
                        {
                            mc_asttoken_init(&outtok, MC_TOK_LSHIFT, "<<", 2);
                        }
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_LT, "<", 1);
                        break;
                    }
                }
                break;
            case '>':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_GTE, ">=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else if(mc_lexer_peekchar(lex) == '>')
                    {
                        mc_lexer_readchar(lex);
                        if(mc_lexer_peekchar(lex) == '=')
                        {
                            mc_asttoken_init(&outtok, MC_TOK_ASSIGNRSHIFT, ">>=", 3);
                            mc_lexer_readchar(lex);
                        }
                        else
                        {
                            mc_asttoken_init(&outtok, MC_TOK_RSHIFT, ">>", 2);
                        }
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_GT, ">", 1);
                    }
                }
                break;
            case ',':
                {
                    mc_asttoken_init(&outtok, MC_TOK_COMMA, ",", 1);
                }
                break;
            case ';':
                {
                    mc_asttoken_init(&outtok, MC_TOK_SEMICOLON, ";", 1);
                }
                break;
            case ':':
                {
                    mc_asttoken_init(&outtok, MC_TOK_COLON, ":", 1);
                }
                break;
            case '(':
                {
                    mc_asttoken_init(&outtok, MC_TOK_LPAREN, "(", 1);
                }
                break;
            case ')':
                {
                    mc_asttoken_init(&outtok, MC_TOK_RPAREN, ")", 1);
                }
                break;
            case '{':
                {
                    mc_asttoken_init(&outtok, MC_TOK_LBRACE, "{", 1);
                }
                break;
            case '}':
                {
                    mc_asttoken_init(&outtok, MC_TOK_RBRACE, "}", 1);
                }
                break;
            case '[':
                {
                    mc_asttoken_init(&outtok, MC_TOK_LBRACKET, "[", 1);
                }
                break;
            case ']':
                {
                    mc_asttoken_init(&outtok, MC_TOK_RBRACKET, "]", 1);
                }
                break;
            case '.':
                {
                    mc_asttoken_init(&outtok, MC_TOK_DOT, ".", 1);
                }
                break;
            case '?':
                {
                    mc_asttoken_init(&outtok, MC_TOK_QUESTION, "?", 1);
                }
                break;
            case '%':
                {
                    if(mc_lexer_peekchar(lex) == '=')
                    {
                        mc_asttoken_init(&outtok, MC_TOK_ASSIGNPERCENT, "%=", 2);
                        mc_lexer_readchar(lex);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_PERCENT, "%", 1);
                        break;
                    }
                }
                break;
            case '"':
                {
                    int len;
                    const char* str;
                    mc_lexer_readchar(lex);
                    str = mc_lexer_scanstring(lex, '"', false, NULL, &len);
                    if(str)
                    {
                        mc_asttoken_init(&outtok, MC_TOK_STRING, str, len);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_INVALID, NULL, 0);
                    }
                }
                break;
            case '\'':
                {
                    int len;
                    const char* str;
                    mc_lexer_readchar(lex);
                    str = mc_lexer_scanstring(lex, '\'', false, NULL, &len);
                    if(str)
                    {
                        mc_asttoken_init(&outtok, MC_TOK_STRING, str, len);
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_INVALID, NULL, 0);
                    }
                }
                break;
            case '`':
                {
                    int len;
                    bool templatefound;
                    const char* str;
                    if(!lex->continuetplstring)
                    {
                        mc_lexer_readchar(lex);
                    }
                    templatefound = false;
                    str = mc_lexer_scanstring(lex, '`', true, &templatefound, &len);
                    if(str)
                    {
                        if(templatefound)
                        {
                            mc_asttoken_init(&outtok, MC_TOK_TEMPLATESTRING, str, len);
                        }
                        else
                        {
                            mc_asttoken_init(&outtok, MC_TOK_STRING, str, len);
                        }
                    }
                    else
                    {
                        mc_asttoken_init(&outtok, MC_TOK_INVALID, NULL, 0);
                    }
                }
                break;
            default:
                {
                    int identlen;
                    int numberlen;
                    const char* ident;
                    const char* number;
                    mcasttoktype_t type;
                    if(mc_lexer_charisletter(lex->ch))
                    {
                        identlen = 0;
                        ident = mc_lexer_scanident(lex, &identlen);
                        type = mc_lexer_lookupident(ident, identlen);
                        mc_asttoken_init(&outtok, type, ident, identlen);
                        return outtok;
                    }
                    if(mc_lexer_charisdigit(lex->ch))
                    {
                        numberlen = 0;
                        number = mc_lexer_scannumber(lex, &numberlen);
                        mc_asttoken_init(&outtok, MC_TOK_NUMBER, number, numberlen);
                        return outtok;
                    }
                }
                break;
        }
        mc_lexer_readchar(lex);
        if(mc_lexer_failed(lex))
        {
            mc_asttoken_init(&outtok, MC_TOK_INVALID, NULL, 0);
        }
        lex->continuetplstring = false;
        return outtok;
    }
    /* NB. never reached; but keep the compiler from complaining. */
    return outtok;
}

bool mc_lexer_expectcurrent(mcastlexer_t* lex, mcasttoktype_t type)
{
    const char* actualtypestr;
    const char* expectedtypestr;
    if(mc_lexer_failed(lex))
    {
        return false;
    }
    if(!mc_lexer_currtokenis(lex, type))
    {
        expectedtypestr = mc_asttoken_typename(type);
        actualtypestr = mc_asttoken_typename(lex->currtoken.toktype);
        mc_errlist_addf(lex->errors, MC_ERROR_PARSING, lex->currtoken.pos, "expected token \"%s\", but got \"%s\"", expectedtypestr, actualtypestr);
        return false;
    }
    return true;
}

bool mc_lexer_readchar(mcastlexer_t* lex)
{
    bool ok; 
    if(lex->nextposition >= lex->inputlength)
    {
        lex->ch = '\0';
    }
    else
    {
        lex->ch = lex->inputsource[lex->nextposition];
    }
    lex->position = lex->nextposition;
    lex->nextposition++;
    if(lex->ch == '\n')
    {
        lex->line++;
        lex->column = -1;
        ok = mc_lexer_addline(lex, lex->nextposition);
        if(!ok)
        {
            lex->failed = true;
            return false;
        }
    }
    else
    {
        lex->column++;
    }
    return true;
}

char mc_lexer_peekchar(mcastlexer_t* lex)
{
    if(lex->nextposition >= lex->inputlength)
    {
        return '\0';
    }
    return lex->inputsource[lex->nextposition];
}

bool mc_lexer_charisletter(char ch)
{
    return (
        (('a' <= ch) && (ch <= 'z')) || (('A' <= ch) && (ch <= 'Z')) || (ch == '_')
    );
}

bool mc_lexer_charisdigit(char ch)
{
    return (
        (ch >= '0') && (ch <= '9')
    );
}

bool mc_lexer_charisoneof(char ch, const char* allowed, int allowedlen)
{
    int i;
    for(i = 0; i < allowedlen; i++)
    {
        if(ch == allowed[i])
        {
            return true;
        }
    }
    return false;
}

const char* mc_lexer_scanident(mcastlexer_t* lex, int* outlen)
{
    int len;
    int position;
    position = lex->position;
    len = 0;
    while(mc_lexer_charisdigit(lex->ch) || mc_lexer_charisletter(lex->ch) || lex->ch == ':')
    {
        if(lex->ch == ':')
        {
            if(mc_lexer_peekchar(lex) != ':')
            {
                goto end;
            }
            mc_lexer_readchar(lex);
        }
        mc_lexer_readchar(lex);
    }
end:
    len = lex->position - position;
    *outlen = len;
    return lex->inputsource + position;
}

const char* mc_lexer_scannumber(mcastlexer_t* lex, int* outlen)
{
    int len;
    int position;
    static const char allowed[] = ".xXaAbBcCdDeEfF";
    position = lex->position;
    while(mc_lexer_charisdigit(lex->ch) || mc_lexer_charisoneof(lex->ch, allowed, MC_UTIL_STATICARRAYSIZE(allowed) - 1))
    {
        mc_lexer_readchar(lex);
    }
    len = lex->position - position;
    *outlen = len;
    return lex->inputsource + position;
}

const char* mc_lexer_scanstring(mcastlexer_t* lex, char delimiter, bool istemplate, bool* outtemplatefound, int* outlen)
{
    bool escaped;
    int len;
    int position;
    *outlen = 0;
    escaped = false;
    position = lex->position;
    while(true)
    {
        if(lex->ch == '\0')
        {
            return NULL;
        }
        if(lex->ch == delimiter && !escaped)
        {
            break;
        }
        if(istemplate && !escaped && lex->ch == '$' && mc_lexer_peekchar(lex) == '{')
        {
            *outtemplatefound = true;
            break;
        }
        escaped = false;
        if(lex->ch == '\\')
        {
            escaped = true;
        }
        mc_lexer_readchar(lex);
    }
    len = lex->position - position;
    *outlen = len;
    return lex->inputsource + position;
}

mcasttoktype_t mc_lexer_lookupident(const char* ident, int len)
{
    int i;
    int klen;
    static struct
    {
        const char* value;
        mcasttoktype_t type;
    } keywords[] = {
        { "function", MC_TOK_FUNCTION },
        { "const", MC_TOK_CONST },
        { "var", MC_TOK_VAR },
        { "let", MC_TOK_VAR },
        { "true", MC_TOK_TRUE },
        { "false", MC_TOK_FALSE },
        { "if", MC_TOK_IF },
        { "else", MC_TOK_ELSE },
        { "return", MC_TOK_RETURN },
        { "while", MC_TOK_WHILE },
        { "break", MC_TOK_BREAK },
        { "for", MC_TOK_FOR },
        { "in", MC_TOK_IN },
        { "continue", MC_TOK_CONTINUE },
        { "null", MC_TOK_NULL },
        { "import", MC_TOK_IMPORT },
        { "recover", MC_TOK_RECOVER },
        { NULL, (mcasttoktype_t)0}
    };
    for(i = 0; keywords[i].value != NULL; i++)
    {
        klen = mc_util_strlen(keywords[i].value);
        if(klen == len && MC_UTIL_STRNEQ(ident, keywords[i].value, len))
        {
            return keywords[i].type;
        }
    }
    return MC_TOK_IDENT;
}

void mc_lexer_skipspace(mcastlexer_t* lex)
{
    char ch;
    ch = lex->ch;
    while(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
    {
        mc_lexer_readchar(lex);
        ch = lex->ch;
    }
}

bool mc_lexer_addline(mcastlexer_t* lex, int offset)
{
    bool ok;
    size_t linelen;
    char* line;
    const char* linestart;
    const char* newlineptr;
    if(!lex->file)
    {
        return true;
    }
    if(lex->line < mc_ptrlist_count(lex->file->lines))
    {
        return true;
    }
    linestart = lex->inputsource + offset;
    newlineptr = strchr(linestart, '\n');
    line = NULL;
    if(!newlineptr)
    {
        line = mc_util_strdup(lex->pstate, linestart);
    }
    else
    {
        linelen = newlineptr - linestart;
        line = mc_util_strndup(lex->pstate, linestart, linelen);
    }
    if(!line)
    {
        lex->failed = true;
        return false;
    }
    ok = mc_ptrlist_push(lex->file->lines, line);
    if(!ok)
    {
        lex->failed = true;
        mc_memory_free(line);
        return false;
    }
    return true;
}

