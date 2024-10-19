
function callme(v)
{
    println("**in callback** v = <<<", v, ">>>");
    var tmp = v * 2
    return tmp
}

a=[44, 55, 66, 77, 88]
b = a.map(callme)
println("after? a=", a, " b=", b, "")

