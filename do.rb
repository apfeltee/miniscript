#!/usr/bin/ruby

begin
        syms = [
        { "sqrt" => :mc_nsfnmath_sqrt },
        { "pow"=> :mc_scriptfn_pow },
        { "sin"=> :mc_scriptfn_sin },
        { "cos"=> :mc_scriptfn_cos },
        { "tan"=> :mc_scriptfn_tan },
        { "log"=> :mc_scriptfn_log },
        { "ceil"=> :mc_scriptfn_ceil },
        { "floor"=> :mc_scriptfn_floor },
        { "abs"=> :mc_scriptfn_abs },
        ]

        syms.each do |b|
                b.each do |k, fn|
                        printf("mc_valmap_setvalstring(jmap, %p, mc_value_makefuncnative(state, %p, %s, NULL, 0));\n", b)
                end
        end
        
end
