import "mtestb"
import "module_dir/submc"

{
    var block_scope_var = 2;
}

assert(true) // testing if builtins are resolvable

function inc() {
    return submc::inc()
}

function add(a, b) {
    return a + b
}

// User's native functions should be visible from modules
const squared = squarearray(1, 2, 3)
assert(squared[0] == 1)
assert(squared[1] == 4)
assert(squared[2] == 9)
