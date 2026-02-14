#!/usr/bin/ruby

begin
  dt=[]
  $stdin.each_line do |l|
    m=l.scrub.match(/\x27(?<sym>\w+)\x27\s*was\s*not\s*declared/)
    next unless m
    sym = m[:sym]
    if not dt.include?(sym) then
      dt.push(sym)
    end
  end
  print("\\b(" + dt.join("|") + ")\\b")
end
