
function HTMLGen(out)
{
    self = {}
    self.out = out
    self.put = function(s)
    {
        print(s)
    }
    self.tag = function(name, attribs, fn)
    {
        self.put("<" + name)
        if(attribs.class == Dict)
        {
            var asize = attribs.length
            if(asize > 0)
            {
                var vi = 0;
                self.put(" ")
                var akeys = attribs.keys()
                for(var i=0; i<asize; i++)
                {
                    var k = akeys[i];
                    var val = attribs[k];
                    vi++;
                    self.put("" + k + "=\"" + val + "\"")
                    if((vi+1) < asize)
                    {
                        self.put(" ")
                    }
                }
            }
        }
        self.put(">")
        if(typeof(fn) == "function")
        {
            fn()
        }
        else
        {
            self.put(fn)
        }
        self.put("</" + name + ">")
    }
    return self
}

function App()
{
    self = {}

    self.sendPage = function(ctype, code)
    {
    }

    self.sendError = function(code)
    {
        self.sendPage("text/html", code)
        print("no such page.")
    }

    self.sendIndex = function(a)
    {
        self.sendPage("text/html", 200)
        gh = HTMLGen(null)
        gh.tag("html", {}, function() {
            gh.tag("head", {}, function(){
                gh.tag("title", {}, "dummy document (full of lies)")
            })
            gh.tag("body", {}, function() {
                gh.tag("h1", {}, "files: ")
                gh.tag("ul", {}, function() {
                    //var items = os.readdir(".").filter(function(itm){ return (itm[0] != "."); });
                    var items = ["foo.exe", "stuff.mov", "source.f90", "runme.vbs", "twitter.php", "facebook.java"]
                    var len = items.length;
                    for(var i=0; i<items.length; i++)
                    {
                        var itm = items[i];
                        if((itm == ".") || (itm == ".."))
                        {
                            continue;
                        }
                        url = ("web.cgi?f=" + itm);
                        gh.tag("li", {}, function() {
                            gh.tag("a", {"href": url}, itm)
                        });
                        gh.put("\n");
                    }
                })
            })
        })
    }

    self.main = function(a)
    {
        self.sendIndex()
    }
    return self
}

App().main()
