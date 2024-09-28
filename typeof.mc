
function testtypeof(val, expect)
{
    var ts = typeof(val);
    if(ts != expect)
    {
        println("ERROR: for <", val, "> expected '", expect, "', but got '", ts, "'");
        null();
    }
}

testtypeof(37, "number");
testtypeof(3.14, "number");
testtypeof(42, "number");

// Strings
testtypeof("", "string");
testtypeof("bla", "string");
testtypeof(`template literal`, "string");
testtypeof("1", "string"); // note that a number within a string is still realtypeof string
testtypeof(1, "number"); // realtypeof always returns a string

// Booleans
testtypeof(true, "boolean");
testtypeof(false, "boolean");
testtypeof(!!1, "boolean"); // two calls of the ! (logical NOT) operator are equivalent to Boolean()

// Objects
testtypeof({ a: 1 }, "object");

// use Array.isArray or Object.prototype.toString.call
// to differentiate regular objects from arrays
//testtypeof([1, 2, 4], "object");


// Functions
testtypeof(function () {}, "function");
testtypeof(Math.sin, "function");

println("all good!")
