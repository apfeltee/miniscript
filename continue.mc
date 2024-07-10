

var t1 = function()
{
    println("t1: really simple test ...");
    var a = 0;
    while(a < 10)
    {
        a = a + 1;
        if(a > 5)
        {
            continue;
        }
        println("t1: ", a);
    }
    println("t1: done!");
}


var t2 = function()
{
    println("t2: simple test ...");
    var b = 5;
    while (b != 0)
    {
        b = b - 1;
        if (b == 3)
        {
            println("t2: printing three");
            continue;
        }
    }
    println("t2: done!");
}

var t3 = function()
{
    println("t3: other test ...");
    var b = 5;
    while (b != 0)
    {
        b = b - 1;
        if (b == 3) {
            println("t3: 3!");
            continue;
        }

        if (b == 4) {
            println("t3: 4!");
            continue;
        }
    }
}

var t4 = function()
{
    var b = 5;
    var g = "scopes ok";
    while (b != 0) {
        var x = 6;
        var y = 0;
        b = b - 1;
        if (b == 0) {
            println("t4: zero");
            continue;
        }
    }
    println("t4: ", g);
}

var t5 = function()
{
    for (var b = 0; b != 5; b = b + 1) {
        if (b == 0 || b == 1 || b == 2) {
            println("t5: ", b);
            continue;
        }
    }
}

t1()
t2()
t3()
t4()
t5()
