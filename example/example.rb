#!mruby

v8 = V8.new
expr = '1 + 2'
puts "#{expr} = #{v8.eval(expr)}"

v8.add_func("plus") do |lhs,rhs|
  lhs + rhs
end

expr = 'plus(2, 3)'
puts "#{expr} = #{v8.eval(expr)}"

v8.eval("var console = {}");
v8.add_func("console.log") do |str|
  puts str
end
expr = 'console.log(123)'
puts expr
v8.eval(expr)
