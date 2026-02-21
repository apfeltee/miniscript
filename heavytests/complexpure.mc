
// does not work! need to figure out why.

function Complex(ire, iim)
{
    var self = {}
    self.re = ire;
    self.im = iim;
    self.__operator_mul__ = function(b)
    {
        var vre = (self.re * b.re - self.im * b.im);
        var vim = (self.re * b.im + self.im * b.re);
        return Complex(vre, vim);
    }
    self.__operator_div__ = function(b)
    {
        var r = b.re;
        var i = b.im;
        var ti;
        var tr;
        tr = Math.abs(r);
        ti = Math.abs(i);
        if(tr <= ti)
        {
            ti = r / i;
            tr = i * (1 + ti * ti);
            r = self.re;
            i = self.im;
        }
        else
        {
            ti = -i / r;
            tr = r * (1 + ti * ti);
            r = -self.im;
            i = self.re;
        }
        return Complex((r * ti + i) / tr, (i * ti - r) / tr);
    }
    self.__operator_add__ = function(b)
    {
        return Complex(self.re + b.re, self.im + b.im);
    }
    self.__operator_sub__ = function(b)
    {
        return Complex(self.re - b.re, self.im - b.im);
    }
    return self
}

function dump_num(name, n)
{
    println(`${name}: real=${n.re} imag=${n.im}`);
}

const image_width = 150;
const image_height = 50;
const max_iter = 100;
const escape = 2;

function mandel()
{
    var x;
    var y;
    var n;
    var c;
    var z;
    var start_x = Complex(-2.1888, 0.0);
    var end_x = Complex(1.0, 0.0);
    var start_y = Complex(-1.25, 0.0);
    var step_x = ((end_x - start_x) / Complex(150.0, 0.0));
    var step_y = (((Complex(0, 0) - start_y) - start_y) / Complex(50.0, 0.0));
    var stepval = Complex(0.0, 1.0);
    dump_num("start_x", start_x);
    dump_num("end_x", end_x);
    dump_num("start_y", start_y);
    dump_num("step_x", step_x);
    dump_num("step_y", step_y);
    dump_num("stepval", stepval);
    for(y = 0; y < image_height; y++)
    {
        for(x = 0; x < image_width; x++)
        {
            c = (((Complex(x, 0) * step_x) + start_x) + (((Complex(y, 0) * step_y) + start_y) * stepval));
            z = Complex(0, 0);
            for(n = 0; n < max_iter; n++)
            {
                z = (z * z);
                z = (z + c);
                if(Math.hypot(z.re, z.im) >= escape)
                {
                    break;
                }
            }
            if(n == max_iter)
            {
                print("-");
            }
            else if(n > 6)
            {
                print(".");
            }
            else if(n > 3)
            {
                print("+");
            }
            else if(n > 2)
            {
                print("x");
            }
            else
            {
                print("*");
            }
        }
        print("\n");
    }
}
function main()
{
    mandel();
    return 0;
}
main();
