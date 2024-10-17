
function callme(v)
{
    println("**in callback** v = <<<", v, ">>>");
    return v*2
}

a=[44, 55, 66, 77, 88]
b = a.map(callme)
println("after? a=", a, " b=", b, "")

