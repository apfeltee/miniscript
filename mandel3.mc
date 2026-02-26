
var xmin = -8601;
var xmax = 2867;
var ymin = -4915;
var ymax = 4915;
var maxiter = 32;
var dx = (xmax - xmin) / 79;
var dy = (ymax - ymin) / 24;
var cy = ymin;
const chartab = {
    33: "!",
    34: '"',
    36: "$",
    37: "%",
    35: "#",
    39: "'",
    55: "7",
    40: "(",
    38: "&",
    42: "*",
    64: "@",
    41: ")",
    52: "4",
    43: "+",
    63: "?",
    44: ",",
    60: "<",
    45: "-",
    50: "2",
}
while(cy <= ymax)
{
    var cx = xmin;
    while(cx <= xmax)
    {
        var x = 0;
        var y = 0;
        var x2 = 0;
        var y2 = 0;
        var iter = 0;
        while(iter < maxiter)
        {
            if((x2 + y2) > 16384)
            {
                break;
            }
            y = ((x * y) >> 11) + cy;
            x = x2 - y2 + cx;
            x2 = ((x * x) >> 12);
            y2 = ((y * y) >> 12);
            iter = iter + 1;
        }
        var ch = (32 + iter);
        var pch = "/";
        print(chartab[ch])
        cx = cx + dx;
    }
    print("\n");
    cy = cy + dy;
}


