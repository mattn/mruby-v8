#!mruby

v8 = V8.new
puts v8.eval('1+2')

v8.add_func("plus") do |lhs,rhs|
  lhs + rhs
end

puts v8.eval("plus(2,3)")

v8.eval("var console = {}");
v8.add_func("console.log") do |str|
  puts str
end
v8.eval("console.log(123)")
