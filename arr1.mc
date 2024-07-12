
var a = ["red", "green", "blue"];
a[2] = "magenta"
println("a:count=", len(a));
println("a      =", a);
arraypush(a, "yellow", "purple", 42, 889);
println("a now  =", a);
println("a[1]   =", a[1]);
a[5]++
println("a inc     =", a);
println("a joined  =", arrayjoin(a));
