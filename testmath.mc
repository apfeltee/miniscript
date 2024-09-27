
let checkmath = function(exval, expect)
{
    if(exval != expect)
    {
        println(`failure detected! expected ${expect} but got ${exval}`)
        assert(false)
    }
}

checkmath(1499 & 7800, 1112)
checkmath(1499 | 7800, 8187)
checkmath(1499 ^ 7800, 7075)
checkmath(1499 / 7800, 0.19217948717948719)
checkmath(1499 * 7800, 11692200)
checkmath(1499 % 7800, 1499)
checkmath(1499 % 7800, 1499)
checkmath(0x777 & 0xFF, 119)
checkmath(1234 & 5678, 1026)
checkmath(12 << 48, 786432)
checkmath(233 >> 2, 58)
checkmath(12 | 48, 60)
checkmath(55 ^ 48, 7)

println("all good")
