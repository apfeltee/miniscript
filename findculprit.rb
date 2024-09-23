#!/usr/bin/ruby

FUNCPAT = /\b(opt(bits|prs)|stod|mc)_\w+\b/

begin
  data = {}

  inputfile = ARGV[0]
  File.foreach(inputfile) do |line|
    m = line.match(/^\b(?<sym>[\w@]+)\b(.*)<(?<time>\d+[\.\d]+)>/)
    if m then
      sym = m['sym']
      next unless sym.match?(FUNCPAT)
      timestr = m['time']
      time = timestr.to_f
      if (time != timestr.to_f) then
        $stderr.printf("failed to parse time %p!\n", timestr)
        exit(1)
      end
      #$stderr.printf("parsed call to '%p' ... ", sym)
      if !data.key?(sym) then
        #$stderr.printf(" first instance\r")
        data[sym] = { 'time' => 0, 'count' => 1 }
      else
        #$stderr.printf(" seen %d times\r", data[sym]['count'])
        data[sym]['count'] += 1
      end
      data[sym]['time'] = time
    end
  end
  if data.empty? then
    $stderr.printf("failed to extract any data; did you forget to use the '-T' flag when running 'ltrace'?\n")
  else
    data.sort_by{|s, bits| bits['time'] }.each do |sym, bits|
      printf("%d -> %g %s\n", bits['count'], bits['time'], sym)
    end
  end
end


