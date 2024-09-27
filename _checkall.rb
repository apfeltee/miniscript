#!/usr/bin/ruby

require 'optparse'

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
  exe = './run'
  OptParse.new{|prs|
  
  }.parse!
  if ARGV.length > 0 then
    exe = ARGV[0]
  end
  fmatchin(__dir__, '.mc') do |file|
    $stderr.printf("** running file %p ** (exe=%p)\n", file, exe)
    rt = system(exe, file)
    $stderr.printf("returned %s (%d)\n", rt, $?)
    if !rt then
      #exit(1)
    end
  end
end
