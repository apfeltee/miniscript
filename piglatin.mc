

const vowels = ['a', 'e', 'i', 'o', 'u'];
const num_vowels = vowels.length;

function pig_latin(word)
{
    word = word.toLower();
    var first = word[0];
    for (var i = 0; i < num_vowels; i++)
    {
        if (first != vowels[i])
        {
            continue;
        }
        word += "way";
        return word
    }
    word = slice(word, 1, word.length - 1);
    word += first + "ay";
    return word;
}

println(pig_latin("Apple"));
println(pig_latin("Banana"));
println(pig_latin("Cherry"));
println(pig_latin("Damascus"));
println(pig_latin("Eggplant"));
println(pig_latin("Fig"));

