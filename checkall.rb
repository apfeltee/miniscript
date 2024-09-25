#!/usr/bin/ruby

def fmatchin(dir, ext)
  Dir.open(dir) do |dh|
    while (ent = dh.read) != nil do
      ename = File.extname(ent)
      next unless (ename == ext)
      fp = File.join(dir, ent)
      yield fp
    end
  end
end

begin
  fmatchin(__dir__, '.mc') do |file|
    $stderr.printf("** running file %p **\n", file)
    if !system('./run', file) then
      exit(1)
    end
  end
end
