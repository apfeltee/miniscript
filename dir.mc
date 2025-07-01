
function recursivescan(path, recursive, fn)
{
    var items = Dir.read(path, true, false)
    for(var i=0; i<items.length; i++)
    {
        var name = items[i];
        {
            //var fp = File.join(path, name)
            var fp = name;
            var isfile = File.isFile(fp);
            var isdir = File.isDirectory(fp);
            if(fn(fp, isfile) == false)
            {
                return;
            }
            if(isdir && !isfile)
            {
                recursivescan(fp, recursive, fn);
                //VM.gc();
            }
        }
    }
}

recursivescan("infer-out", true, function(fp, isfile)
{
    if(isfile)
    {
        println("fp = ", fp);
    }
    return true;
})
