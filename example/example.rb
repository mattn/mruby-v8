#!mruby

v8 = V8.new
puts v8.eval('1+2')

v8.add_func("bar") do
  puts "hello"
end

puts v8.eval("bar()")
