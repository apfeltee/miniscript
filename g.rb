
src=<<__eos__



__eos__

begin
  dt = []
  File.foreach('prot.inc').each do |ln|
    ln.strip!
    next if ln.empty?
    next if ln.match?(/^\s*\/\*/)
    m = ln.match(/\b(?<name>\w+)\b\s*\(/)
    name = m['name']
    next if name.match?(/^(mc|cfn)_/)
    puts(name)
  end
end

