#!/usr/bin/env ruby

require "fileutils"

src = ARGV.shift
dst = ARGV.shift
org = "kernal-901227-03.bin"

input = File.open(src, "r")
output = File.open(dst, "r+")

puts "Patching #{dst}"

def patch(input, output, start, stop)
  input.seek(start)
  output.seek(start)

  output.write(input.read(stop-start))
end

File.open("kernal.log").each_line do |line|
  if line =~ /PATCH 0x([0-9a-f]+) 0x([0-9a-f]+)/
    patch(input, output, $1.to_i(16), $2.to_i(16))
  end
end

input.close
output.close

