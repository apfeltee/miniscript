

const $voweldata = ['a', 'e', 'i', 'o', 'u'];
const vowelcount = $voweldata.length;

function pig_latin(str)
{
    str = str.toLower();
    var first = str[0];
    for(var i = 0; i < vowelcount; i++)
    {
        if (first != $voweldata[i])
        {
            continue;
        }
        str += "way";
        return str
    }
    str = slice(str, 1, str.length - 1);
    str += first + "ay";
    return str;
}

var terms = null;
if(ARGV.length > 1)
{
    terms = ARGV;
}
else
{
    terms = [ "Apple", "Banana", "Cherry", "Damascus", "Eggplant", "Fig"]
}

for(term in terms)
{
    println("'", term, "' = ", pig_latin(term));
}
    