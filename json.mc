
MyJSON = {}

MyJSON.isWhiteSpace = function(c)
{
    return (
        (c == " ") ||
        (c == "\r") ||
        (c == "\n") ||
        (c == "\t")
    );
};

MyJSON.skipWhiteSpace = function(state)
{
    while(true)
    {
        if(state.m_index >= state.m_srclength)
        {
            break;
        }
        if(MyJSON.isWhiteSpace(state.m_source[state.m_index]))
        {
            state.m_index++;
        }
        else
        {
            break;
        }
    }
};

MyJSON.parseValue = function(state)
{
    MyJSON.skipWhiteSpace(state);
    if(state.m_index == state.m_srclength)
    {
        return null;
    }
    var c = state.m_source[state.m_index];
    if (c == "{")
    {
        return MyJSON.parseObject(state);
    }
    else if (c == "[")
    {
        return MyJSON.parseArray(state);
    }
    else if (c == "'")
    {
        return MyJSON.parseString(state, "'");
    }
    else if (c == "\"")
    {
        return MyJSON.parseString(state, "\"");
    }
    else if (state.nfirstchars.indexOf(c) != -1)
    {
        return MyJSON.parseNumber(state);
    }
    else if (c == "t")
    {
        return MyJSON.parseLiteral(state, "true", true);
    }
    else if (c == "f")
    {
        return MyJSON.parseLiteral(state, "false", false);
    }
    else if (c == "n")
    {
        return MyJSON.parseLiteral(state, "null", null);
    }
    error(`Invalid json (c=<${c}>, index=${state.m_index}, max=${state.m_srclength})`);
};

MyJSON.parseLiteral = function(state, literal, value)
{
    if(literal.length > (state.m_srclength - state.m_index))
    {
        error(`Expecting ${literal}`);
    }
    for (var i = 0; i < literal.length; i++)
    {
        if (literal[i] != state.m_source[state.m_index++])
        {
            error(`Expecting ${literal}`);
        }
    }
    return value;
};

MyJSON.parseNumber = function (state)
{
    var startIndex = state.m_index;
    var c = 0;
    while(state.nchars.indexOf(state.m_source[state.m_index]) != -1)
    {
        state.m_index++;
    }
    var str = slice(state.m_source, startIndex, state.m_index);
    println(`parseNumber: str=${str}`);
    return str.toNumber();
};

MyJSON.parseString = function(state, quote)
{
    var startIndex = state.m_index;
    state.m_index++;
    var c = 0;
    var s = "";
    while ((c = state.m_source[state.m_index]) != quote)
    {
        if(c == "\\++"[0])
        {
            state.m_index++;
            c = state.m_source[state.m_index];
            if(c == "r")
            {
                s += "\r";
            }
            else if (c == "n")
            {
                s += "\n";
            }
            else if (c == "t")
            {
                s += "\t";
            }
            else if (c == "f")
            {
                s += "\f";
            }
            // Note escaped unicode not handled
            else
            {
                s += c;
            }
        }
        else
        {
            s += c;
        }
        state.m_index++;
    }
    state.m_index++;
    return s;
};

MyJSON.parseObject = function(state)
{
    state.m_index++;
    MyJSON.skipWhiteSpace(state);
    if(state.m_source[state.m_index] == "}")
    {
        state.m_index++;
        return {};
    }
    var o = {};
    var c = null;
    while (true)
    {
        var name = MyJSON.parseValue(state);
        MyJSON.skipWhiteSpace(state);
        c = state.m_source[state.m_index];
        if (c != ":")
        {
            error("Expecting :");
        }
        state.m_index++;
        MyJSON.skipWhiteSpace(state);
        var value = MyJSON.parseValue(state);
        MyJSON.skipWhiteSpace(state);
        if(name != null)
        {
            o[name] = value;
        }
        c = state.m_source[state.m_index];
        if (c == ",")
        {
            state.m_index++;
            MyJSON.skipWhiteSpace(state);
        }
        else
        {
            break;
        }
    }
    if(c != "}")
    {
        error("Expecting }");
    }
    state.m_index++;
    return o;
};

MyJSON.parseArray = function(state)
{
    state.m_index++;
    MyJSON.skipWhiteSpace(state);
    if(state.m_source[state.m_index] == "]")
    {
        state.m_index++;
        return [];
    }
    var a = [];
    var c = null;
    while(true)
    {
        var value = MyJSON.parseValue(state);
        a.push(value);
        MyJSON.skipWhiteSpace(state);
        c = state.m_source[state.m_index];
        if (c == ",")
        {
            state.m_index++;
            MyJSON.skipWhiteSpace(state);
        }
        else
        {
            break;
        }
    }
    if(c != "]")
    {
        error("Expecting ]");
    }
    state.m_index++;
    return a;
};

MyJSON.parse = function(str)
{
    var state = {};
    state.m_source = str;
    state.m_srclength = str.length;
    state.m_index = 0;
    state.nfirstchars = "-0123456789.";
    state.nchars = "-0123456789.eE";
    //println(MyJSON)
    r = MyJSON.parseValue(state);
    println(state);
    return r
};

src = "[1, 2, 3, {\"name\": \"john doe\", \"flags\": [4, 5, 6]}]";
//src = "[1, 2, 3, 4]"
println("------------")
res = MyJSON.parse(src);
println("res = ", res)

