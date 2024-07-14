
strings = [
    "ILLEGAL", "EOF",   "=",     "+=",  "-=", "*=",       "/=",       "%=",     "&=",      "|=",    "^=",     "<<=",    ">>=",
    "?",       "+",     "++",    "-",   "--", "!",        "*",        "/",      "<",       "<=",    ">",      ">=",     "==",
    "!=",      "&&",    "||",    "&",   "|",  "^",        "<<",       ">>",     ",",       ";",     ":",      "(",      ")",
    "{",       "}",     "[",     "]",   ".",  "%",        "FUNCTION", "CONST",  "VAR",     "TRUE",  "FALSE",  "IF",     "ELSE",
    "RETURN",  "WHILE", "BREAK", "FOR", "IN", "CONTINUE", "NULL",     "IMPORT", "RECOVER", "IDENT", "NUMBER", "STRING", "TEMPLATE_STRING",
]

toksrc = <<__eos__
    MC_TOK_INVALID,
    MC_TOK_EOF,
    MC_TOK_ASSIGN,
    MC_TOK_ASSIGNPLUS,
    MC_TOK_ASSIGNMINUS,
    MC_TOK_ASSIGNASTERISK,
    MC_TOK_ASSIGNSLASH,
    MC_TOK_ASSIGNPERCENT,
    MC_TOK_ASSIGNBINAND,
    MC_TOK_ASSIGNBINOR,
    MC_TOK_ASSIGNBINXOR,
    MC_TOK_ASSIGNLSHIFT,
    MC_TOK_ASSIGNRSHIFT,
    MC_TOK_QUESTION,
    MC_TOK_PLUS,
    MC_TOK_PLUSPLUS,
    MC_TOK_MINUS,
    MC_TOK_MINUSMINUS,
    MC_TOK_BANG,
    MC_TOK_ASTERISK,
    MC_TOK_SLASH,
    MC_TOK_LT,
    MC_TOK_LTE,
    MC_TOK_GT,
    MC_TOK_GTE,
    MC_TOK_EQ,
    MC_TOK_NOTEQ,
    MC_TOK_AND,
    MC_TOK_OR,
    MC_TOK_BINAND,
    MC_TOK_BINOR,
    MC_TOK_BINXOR,
    MC_TOK_LSHIFT,
    MC_TOK_RSHIFT,
    MC_TOK_COMMA,
    MC_TOK_SEMICOLON,
    MC_TOK_COLON,
    MC_TOK_LPAREN,
    MC_TOK_RPAREN,
    MC_TOK_LBRACE,
    MC_TOK_RBRACE,
    MC_TOK_LBRACKET,
    MC_TOK_RBRACKET,
    MC_TOK_DOT,
    MC_TOK_PERCENT,
    MC_TOK_FUNCTION,
    MC_TOK_CONST,
    MC_TOK_VAR,
    MC_TOK_TRUE,
    MC_TOK_FALSE,
    MC_TOK_IF,
    MC_TOK_ELSE,
    MC_TOK_RETURN,
    MC_TOK_WHILE,
    MC_TOK_BREAK,
    MC_TOK_FOR,
    MC_TOK_IN,
    MC_TOK_CONTINUE,
    MC_TOK_NULL,
    MC_TOK_IMPORT,
    MC_TOK_RECOVER,
    MC_TOK_IDENT,
    MC_TOK_NUMBER,
    MC_TOK_STRING,
    MC_TOK_TEMPLATESTRING,
__eos__

begin
  toks = toksrc.split(/,/).map(&:strip).reject(&:empty?)
  i = 0
  while true do
    str = strings[i]
    tok = toks[i]
    i += 1
    if str == nil then
      break
    end

    print(
    [
      "    case #{tok}:\n",
      sprintf("        return %p;\n", str),
    ].join
    )

  end


end

