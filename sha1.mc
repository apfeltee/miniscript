

function unshiftright(a, b)
{
    var na = a;
    var nb = b;
    if((nb >= 32) || (nb < -32))
    {
        var m = (nb / 32);
        nb = nb - (m * 32);
    }
    if(nb < 0)
    {
        nb = 32 + nb;
    }
    if (nb == 0)
    {
        return ((na >> 1) & 0x7fffffff) * 2 + ((na >> nb) & 1);
    }
    if (na < 0) 
    { 
        na = (na >> 1); 
        na = na & 0x7fffffff; 
        na = na | 0x40000000; 
        na = (na >> (nb - 1)); 
    }
    else
    {
        na = (na >> nb); 
    }
    return na; 
}

// hex output format. 0 - lowercase; 1 - uppercase
var hexcase = 0;

// base-64 pad character. "=" for strict RFC compliance
var b64pad  = "";



// Bitwise rotate a 32-bit number to the left.
function dorol(num, cnt)
{
    var first = (num << cnt);
    var second = unshiftright(num, (32 - cnt));
    return (first | second);
}


// Add integers, wrapping at 2^32.
// This uses 16-bit operations internally to work around bugs in some JS interpreters.
function safe_add(x, y)
{
    if(!x)
    {
        x = 0;
    }
    if(!y)
    {
        y = 0;
    }
    var leftv = (x & 0xFFFF);
    var lsw = leftv + (y & 0xFFFF);
    var msw = (x >> 16) + (y >> 16) + (lsw >> 16);
    return (msw << 16) | (lsw & 0xFFFF);
}


// Perform the appropriate triplet combination function for the current iteration
function sha1_ft(t, b, c, d)
{
    if(t < 20)
    {
        return (b & c) | ((~b) & d);
    }
    if(t < 40)
    {
        return b ^ c ^ d;
    }
    if(t < 60)
    {
        return (b & c) | (b & d) | (c & d);
    }
    return b ^ c ^ d;
}


// Determine the appropriate additive constant for the current iteration 
function sha1_kt(t)
{
    if(t < 20)
    {
        return 1518500249;
    }
    if(t < 40)
    {
        return 1859775393;
    }
    if(t < 60)
    {
        return -1894007588;
    }
    return -899497514;
}

// Calculate the SHA-1 of an array of big-endian words, and a bit length
function core_sha1(x, l)
{
    // append padding
    x[l >> 5] |= 0x80 << (24 - (l % 32));
    x[((l + 64 >> 9) << 4) + 15] = l;
    var w = [];
    var a =  1732584193;
    var b = -271733879;
    var c = -1732584194;
    var d =  271733878;
    var e = -1009589776;
    var i = 0;
    var j = 0;
    while(i < x.length)
    {
        var olda = a;
        var oldb = b;
        var oldc = c;
        var oldd = d;
        var olde = e;
        while(j < 80)
        {
            if(j < 16)
            {
                var xval = x[i + j];
                w[j] = xval;
            }
            else
            {
                var wv1 = w[j - 3];
                var wv2 = w[j - 8];
                var wv3 = w[j - 14];
                var wv4 = w[j - 16];
                if(!wv1)
                {
                    wv1 = 0;
                }
                if(!wv2)
                {
                    wv2 = 0;
                }
                if(!wv3)
                {
                    wv3 = 0;
                }
                if(!wv4)
                {
                    wv4 = 0;
                }
                w[j] = dorol(wv1 ^ wv2 ^ wv3 ^ wv4, 1);
            }
            var tv1 = safe_add(dorol(a, 5), sha1_ft(j, b, c, d));
            var tv2 = safe_add(e, w[j]);
            var tv3 = safe_add(tv2, sha1_kt(j));
            var t = safe_add(tv1, tv3);
            e = d;
            d = c;
            c = dorol(b, 30);
            b = a;
            a = t;
            j += 1;
        }
        a = safe_add(a, olda);
        b = safe_add(b, oldb);
        c = safe_add(c, oldc);
        d = safe_add(d, oldd);
        e = safe_add(e, olde);
        i = i + 16;
    }
    return [a, b, c, d, e];
}

// Convert an 8-bit or 16-bit string to an array of big-endian words
// In 8-bit function, characters >255 have their hi-byte silently ignored.
function str2binb(str)
{
    var bin = [];
    var chrsz   = 8;
    var mask = (1 << chrsz) - 1;
    var i = 0;
    while(i < str.length * chrsz)
    {
        if(!bin[i>>5])
        {
            bin[i>>5] = 0;
        }
        bin[i>>5] |= (ord(str[(i / chrsz)]) & mask) << (24 - i%32);
        i += chrsz;
    }
    return bin;
}

// Calculate the HMAC-SHA1 of a key and some data
function core_hmac_sha1(key, data)
{
    var chrsz   = 8;
    var bkey = str2binb(key);
    if(bkey.length > 16)
    {
        bkey = core_sha1(bkey, key.length * chrsz);
    }
    var ipad = [];
    var opad = [];
    var i = 0;
    while(i < 16) 
    {
        ipad[i] = bkey[i] ^ 0x36363636;
        opad[i] = bkey[i] ^ 0x5C5C5C5C;
        i += 1;
    }
    var hash = core_sha1(ipad + str2binb(data), 512 + data.length * chrsz);
    return core_sha1(opad + hash, 512 + 160);
}


// Convert an array of big-endian words to a string
function binb2str(bin)
{
    var chrsz   = 8;
    var str = "";
    var mask = (1 << chrsz) - 1;
    var i = 0;
    while(i < (bin.length * 32))
    {
        str += chr(unshiftright(bin[i>>5], (24 - i%32)) & mask);
        i += chrsz;
    }
    return str;
}

// Convert an array of big-endian words to a hex string.
function binb2hex(binarray)
{
    var hex_tab = "0123456789abcdef";
    var str = "";
    var i = 0;
    while(i < (binarray.length * 4))
    {
        var biv = binarray[i>>2];
        var c1 = hex_tab[((biv >> (((3 - (i%4)) * 8) + 4)) & 0xF)];
        var c2 = hex_tab[((biv >> ((3  - (i%4)) * 8))      & 0xF)];
        str += c1;
        str += c2;
        i += 1;
    }
    return str;
}

// Convert an array of big-endian words to a base-64 string
function binb2b64(binarray)
{
    const tab = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    var str = [];
    var i = 0;
    while(i < (binarray.length * 4))
    {
        var triplet = (((((binarray[i >> 2] >> (8 * (3 - (i % 4)))) & 255) << 16) | (((binarray[(i + 1) >> 2] >> (8 * (3 - ((i + 1) % 4)))) & 255) << 8)) | ((binarray[(i + 2) >> 2] >> (8 * (3 - ((i + 2) % 4)))) & 255)); 
        var j = 0;
        while(j < 4)
        {
            if ((((i * 8) + (j * 6)) > (binarray.length * 32)))
            {
                arraypush(str, b64pad);
            }
            else
            {
                arraypush(str, tab[((triplet >> (6 * (3 - j))) & 0x3F)]);
            }
            j += 1;
        }
        i += 3;
    }
    return arrayjoin(str, "");
}


// These are the functions you'll usually want to call
// They take string arguments and return either hex or base-64 encoded strings
function hex_sha1(s)
{
    var chrsz   = 8;
    return binb2hex(core_sha1(str2binb(s),s.length * chrsz));
}

function b64_sha1(s)
{
    var chrsz   = 8;
    return binb2b64(core_sha1(str2binb(s),s.length * chrsz));
}

function str_sha1(s)
{
    var chrsz   = 8;
    return binb2str(core_sha1(str2binb(s),s.length * chrsz));
}

function hex_hmac_sha1(key, data)
{
    return binb2hex(core_hmac_sha1(key, data));
}

function b64_hmac_sha1(key, data)
{
    return binb2b64(core_hmac_sha1(key, data));
}

function str_hmac_sha1(key, data)
{
    return binb2str(core_hmac_sha1(key, data));
}

function main()
{
    var demo = [
        ["foo", "0beec7b5ea3f0fdbc95d0dd47f3c5bc275da8a33"],
        ["bar", "62cdb7020ff920e5aa642c3d4066950dd1f01f4d"],
        ["hello world", "2aae6c35c94fcfb415dbe95f408b9ce91ee846ed"],
        ["abcdx", "a96e144dfdc6380c8a4ae43bea1c81cb01215020"],
        ["long text, some spaces, blah blah", "fc0aa2379ecce86eac0dc50d921ab2cef7030d12"]
    ];

    var idx=0;
    while(idx<demo.length)
    {
        var itm = demo[idx];
        idx += 1;
        var k = itm[0];
        var v = itm[1];
        var ma = hex_sha1(k);
        //ma = "blah"
        var m = ma;
        //println("-- (", k, ") [", v, "] = (", len(ma), ") ", ma)
        var okstr = "FAIL";
        if(m == v)
        {
            okstr = "OK  ";
        }
        println(okstr, ": \"",k, "\" => ", m, "");

    }
}
main()