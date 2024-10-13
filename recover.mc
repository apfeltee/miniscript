
failed = false;
println("hello!")
recover(e)
{
    println("recovered: <<<", e, ">>>")
    failed = true
}
if(!failed)
{
    print("foo"["bar"])
}
println("that's all folks!")