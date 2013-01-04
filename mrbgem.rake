MRuby::Gem::Specification.new('mruby-v8') do |spec|
  spec.license = 'MIT'
  spec.authors = 'mattn'
 
  if ENV['OS'] == 'Windows_NT'
    spec.mruby_libs = '-lv8_base -lv8_nosnapshot -lstdc++ -lwinmm -lws2_32'
  else
    spec.mruby_libs = '-lv8_base -lv8_nosnapshot -lstdc++'
  end
end
