
var xsize = 59;
var ysize = 21;
var minim = -1.0;
var maxim = 1.0;
var minre = -2.0;
var maxre = 1.0;
var stepx = (maxre - minre) / xsize;
var stepy = (maxim - minim) / ysize;
for(var y = 0; y < ysize; y++)
{
    var im = minim + stepy * y;
    for(var x = 0; x < xsize; x++)
    {
        var re = minre + stepx * x;
        var zr = re;
        var zi = im;
        var n
        for(n = 0; n < 30; n++)
        {
            var a = zr * zr;
            var b = zi * zi;
            if(a + b > 4.0)
            {
                break;
            }
            zi = 2 * zr * zi + im;
            zr = a - b + re;
        }
        print((62 - n).chr());
    }
    println();
}
