
#pragma once
/* Optparse --- portable, reentrant, embeddable, getopt-like option parser
 *
 * This is free and unencumbered software released into the public domain.
 *
 * The POSIX getopt() option parser has three fatal flaws. These flaws
 * are solved by Optparse.
 *
 * 1) Parser state is stored entirely in global variables, some of
 * which are static and inaccessible. This means only one thread can
 * use getopt(). It also means it's not possible to recursively parse
 * nested sub-arguments while in the middle of argument parsing.
 * Optparse fixes this by storing all state on a local struct.
 *
 * 2) The POSIX standard provides no way to properly reset the parser.
 * This means for portable code that getopt() is only good for one
 * run, over one argv with one option string. It also means subcommand
 * options cannot be processed with getopt(). Most implementations
 * provide a method to reset the parser, but it's not portable.
 * Optparse provides an nextPositional() function for stepping over
 * subcommands and continuing parsing of options with another option
 * string. The Optparse struct itself can be passed around to
 * subcommand handlers for additional subcommand option parsing. A
 * full reset can be achieved by with an additional optprs_init().
 *
 * 3) Error messages are printed to stderr. This can be disabled with
 * opterr, but the messages themselves are still inaccessible.
 * Optparse solves this by writing an error message in its errmsg
 * field. The downside to Optparse is that this error message will
 * always be in English rather than the current locale.
 *
 * Optparse should be familiar with anyone accustomed to getopt(), and
 * it could be a nearly drop-in replacement. The option string is the
 * same and the fields have the same names as the getopt() global
 * variables (optarg, optind, optopt).
 *
 * Optparse also supports GNU-style long options with nextLongFlag().
 * The interface is slightly different and simpler than getopt_long().
 *
 * By default, argv is permuted as it is parsed, moving non-option
 * arguments to the end. This can be disabled by setting the `m_willpermute`
 * field to 0 after initialization.
 */

#include <stdio.h>
#include <stdlib.h>

#define OPTPARSE_MSG_INVALID "invalid option"
#define OPTPARSE_MSG_MISSING "option requires an argument"
#define OPTPARSE_MSG_TOOMANY "option takes no arguments"

#define optvaltrue (1)
#define optvalfalse (0)


struct OptParser
{
    public:
        enum ArgType
        {
            OPTPARSE_NONE,
            OPTPARSE_REQUIRED,
            OPTPARSE_OPTIONAL
        };

        struct LongFlags
        {
            const char* m_longname;
            int m_shortname;
            ArgType m_argtype;
            const char* m_helptext;
        };

    public:
        static int isDasDash(const char* arg)
        {
            if(arg != NULL)
            {
                if((arg[0] == '-') && (arg[1] == '-') && (arg[2] == '\0'))
                {
                    return optvaltrue;
                }
            }
            return optvalfalse;
        }

        static int isShortOpt(const char* arg)
        {
            if(arg != NULL)
            {
                if((arg[0] == '-') && (arg[1] != '-') && (arg[1] != '\0'))
                {
                    return optvaltrue;
                }
            }
            return optvalfalse;
        }

        static int isLongOpt(const char* arg)
        {
            if(arg != NULL)
            {
                if((arg[0] == '-') && (arg[1] == '-') && (arg[2] != '\0'))
                {
                    return optvaltrue;
                }
            }
            return optvalfalse;
        }

        static int getArgType(const char* optstring, char c)
        {
            int count;
            count = OPTPARSE_NONE;
            if(c == ':')
            {
                return -1;
            }
            for(; *optstring && c != *optstring; optstring++)
            {
            }
            if(!*optstring)
            {
                return -1;
            }
            if(optstring[1] == ':')
            {
                count += optstring[2] == ':' ? 2 : 1;
            }
            return count;
        }

        static int isLongOptsEnd(const LongFlags* longopts, int i)
        {
            if(!longopts[i].m_longname && !longopts[i].m_shortname)
            {
                return optvaltrue;
            }
            return optvalfalse;
        }

        static void optsbitsFromLong(const LongFlags* longopts, char* optstring)
        {
            int i;
            int a;
            char* p;
            p = optstring;
            for(i = 0; !isLongOptsEnd(longopts, i); i++)
            {
                if(longopts[i].m_shortname && longopts[i].m_shortname < 127)
                {
                    *p++ = longopts[i].m_shortname;
                    for(a = 0; a < (int)longopts[i].m_argtype; a++)
                    {
                        *p++ = ':';
                    }
                }
            }
            *p = '\0';
        }

        /* Unlike strcmp(), handles options containing "=". */
        static int matchLongOpts(const char* longname, const char* option)
        {
            const char *a;
            const char* n;
            a = option;
            n = longname;
            if(longname == 0)
            {
                return 0;
            }
            for(; *a && *n && *a != '='; a++, n++)
            {
                if(*a != *n)
                {
                    return 0;
                }
            }
            return *n == '\0' && (*a == '\0' || *a == '=');
        }

        /* Return the part after "=", or NULL. */
        static char* optsbitsGetLongOptsArg(char* option)
        {
            for(; *option && *option != '='; option++)
            {
            }
            if(*option == '=')
            {
                return option + 1;
            }
            return NULL;
        }

    public:
        char** m_argv;
        int m_argc;
        int m_willpermute;
        int m_optind;
        int m_optopt;
        char* m_optarg;
        char m_errmsg[64];
        int m_subopt;

    public:
        /**
         * Initializes the parser state.
         */
        OptParser(int argc, char** argv)
        {
            m_argv = argv;
            m_argc = argc;
            m_willpermute = 1;
            m_optind = argv[0] != 0;
            m_subopt = 0;
            m_optarg = 0;
            m_errmsg[0] = '\0';
        }

        int makeError(const char* msg, const char* data)
        {
            unsigned p;
            const char* sep;
            p = 0;
            sep = " -- '";
            while(*msg)
            {
                m_errmsg[p++] = *msg++;
            }
            while(*sep)
            {
                m_errmsg[p++] = *sep++;
            }
            while(p < sizeof(m_errmsg) - 2 && *data)
            {
                m_errmsg[p++] = *data++;
            }
            m_errmsg[p++] = '\'';
            m_errmsg[p++] = '\0';
            return '?';
        }

        int longFallback(const LongFlags* longopts, int* longindex)
        {
            int i;
            int result;
            /* 96 ASCII printable characters */
            char optstring[96 * 3 + 1];
            optsbitsFromLong(longopts, optstring);
            result = nextShortFlag(optstring);
            if(longindex != 0)
            {
                *longindex = -1;
                if(result != -1)
                {
                    for(i = 0; !isLongOptsEnd(longopts, i); i++)
                    {
                        if(longopts[i].m_shortname == m_optopt)
                        {
                            *longindex = i;
                        }
                    }
                }
            }
            return result;
        }

        /**
         * Read the next option in the argv array.
         * @param optstring a getopt()-formatted option string.
         * @return the next option character, -1 for done, or '?' for error
         *
         * Just like getopt(), a character followed by no colons means no
         * argument. One colon means the option has a required argument. Two
         * colons means the option takes an optional argument.
         */
        int nextShortFlag(const char* optstring)
        {
            int r;
            int type;
            int index;
            char* next;
            char* option;
            char str[2] = { 0, 0 };
            option = m_argv[m_optind];
            m_errmsg[0] = '\0';
            m_optopt = 0;
            m_optarg = 0;
            if(option == 0)
            {
                return -1;
            }
            else if(isDasDash(option))
            {
                /* consume "--" */
                m_optind++;
                return -1;
            }
            else if(!isShortOpt(option))
            {
                if(m_willpermute)
                {
                    index = m_optind++;
                    r = nextShortFlag(optstring);
                    permute(index);
                    m_optind--;
                    return r;
                }
                else
                {
                    return -1;
                }
            }
            option += m_subopt + 1;
            m_optopt = option[0];
            type = getArgType(optstring, option[0]);
            next = m_argv[m_optind + 1];
            switch(type)
            {
                case -1:
                    {
                        str[1] = 0;
                        str[0] = option[0];
                        m_optind++;
                        return makeError(OPTPARSE_MSG_INVALID, str);
                    }
                    break;
                case OPTPARSE_NONE:
                    {
                        if(option[1])
                        {
                            m_subopt++;
                        }
                        else
                        {
                            m_subopt = 0;
                            m_optind++;
                        }
                        return option[0];
                    }
                    break;
                case OPTPARSE_REQUIRED:
                    {
                        m_subopt = 0;
                        m_optind++;
                        if(option[1])
                        {
                            m_optarg = option + 1;
                        }
                        else if(next != 0)
                        {
                            m_optarg = next;
                            m_optind++;
                        }
                        else
                        {
                            str[1] = 0;
                            str[0] = option[0];
                            m_optarg = 0;
                            return makeError(OPTPARSE_MSG_MISSING, str);
                        }
                        return option[0];
                    }
                    break;
                case OPTPARSE_OPTIONAL:
                    {
                        m_subopt = 0;
                        m_optind++;
                        if(option[1])
                        {
                            m_optarg = option + 1;
                        }
                        else
                        {
                            m_optarg = 0;
                        }
                        return option[0];
                    }
                    break;
            }
            return 0;
        }

        /**
         * Handles GNU-style long options in addition to getopt() options.
         * This works a lot like GNU's getopt_long(). The last option in
         * longopts must be all zeros, marking the end of the array. The
         * longindex argument may be NULL.
         */
        int nextLongFlag(const LongFlags* longopts, int* longindex)
        {
            int i;
            int r;
            int index;
            char* arg;
            char* option;
            const char* name;
            option = m_argv[m_optind];
            if(option == 0)
            {
                return -1;
            }
            else if(isDasDash(option))
            {
                m_optind++; /* consume "--" */
                return -1;
            }
            else if(isShortOpt(option))
            {
                return longFallback(longopts, longindex);
            }
            else if(!isLongOpt(option))
            {
                if(m_willpermute)
                {
                    index = m_optind++;
                    r = nextLongFlag(longopts, longindex);
                    permute(index);
                    m_optind--;
                    return r;
                }
                else
                {
                    return -1;
                }
            }
            /* Parse as long option. */
            m_errmsg[0] = '\0';
            m_optopt = 0;
            m_optarg = 0;
            option += 2; /* skip "--" */
            m_optind++;
            for(i = 0; !isLongOptsEnd(longopts, i); i++)
            {
                name = longopts[i].m_longname;
                if(matchLongOpts(name, option))
                {
                    if(longindex)
                    {
                        *longindex = i;
                    }
                    m_optopt = longopts[i].m_shortname;
                    arg = optsbitsGetLongOptsArg(option);
                    if(longopts[i].m_argtype == OPTPARSE_NONE && arg != 0)
                    {
                        return makeError(OPTPARSE_MSG_TOOMANY, name);
                    }
                    if(arg != 0)
                    {
                        m_optarg = arg;
                    }
                    else if(longopts[i].m_argtype == OPTPARSE_REQUIRED)
                    {
                        m_optarg = m_argv[m_optind];
                        if(m_optarg == 0)
                        {
                            return makeError(OPTPARSE_MSG_MISSING, name);
                        }
                        else
                        {
                            m_optind++;
                        }
                    }
                    return m_optopt;
                }
            }
            return makeError(OPTPARSE_MSG_INVALID, option);
        }

        /**
         * Used for stepping over non-option arguments.
         * @return the next non-option argument, or NULL for no more arguments
         *
         * Argument parsing can continue with optparse() after using this
         * function. That would be used to parse the options for the
         * subcommand returned by nextPositional(). This function allows you to
         * ignore the value of m_optind.
         */
        char* nextPositional()
        {
            char* option;
            option = m_argv[m_optind];
            m_subopt = 0;
            if(option != 0)
            {
                m_optind++;
            }
            return option;
        }

        void permute(int index)
        {
            int i;
            char* nonoption;
            nonoption = m_argv[index];
            for(i = index; i < m_optind - 1; i++)
            {
                m_argv[i] = m_argv[i + 1];
            }
            m_argv[m_optind - 1] = nonoption;
        }
};


void optprs_fprintmaybearg(FILE* out, const char* begin, const char* flagname, size_t flaglen, bool needval, bool maybeval, const char* delim)
{
    fprintf(out, "%s%.*s", begin, (int)flaglen, flagname);
    if(needval)
    {
        if(maybeval)
        {
            fprintf(out, "[");
        }
        if(delim != NULL)
        {
            fprintf(out, "%s", delim);
        }
        fprintf(out, "<val>");
        if(maybeval)
        {
            fprintf(out, "]");
        }
    }
}

void optprs_fprintusage(FILE* out, OptParser::LongFlags* flags)
{
    size_t i;
    char ch;
    bool needval;
    bool maybeval;
    bool hadshort;
    OptParser::LongFlags* flag;
    for(i=0; flags[i].m_longname != NULL; i++)
    {
        flag = &flags[i];
        hadshort = false;
        needval = (flag->m_argtype > OptParser::OPTPARSE_NONE);
        maybeval = (flag->m_argtype == OptParser::OPTPARSE_OPTIONAL);
        if(flag->m_shortname > 0)
        {
            hadshort = true;
            ch = flag->m_shortname;
            fprintf(out, "    ");
            optprs_fprintmaybearg(out, "-", &ch, 1, needval, maybeval, NULL);
        }
        if(flag->m_longname != NULL)
        {
            if(hadshort)
            {
                fprintf(out, ", ");
            }
            else
            {
                fprintf(out, "    ");
            }
            optprs_fprintmaybearg(out, "--", flag->m_longname, strlen(flag->m_longname), needval, maybeval, "=");
        }
        if(flag->m_helptext != NULL)
        {
            fprintf(out, "  -  %s", flag->m_helptext);
        }
        fprintf(out, "\n");
    }
}

void optprs_printusage(char* argv[], OptParser::LongFlags* flags, bool fail)
{
    FILE* out;
    out = fail ? stderr : stdout;
    fprintf(out, "Usage: %s [<options>] [<filename> | -e <code>]\n", argv[0]);
    optprs_fprintusage(out, flags);
}
