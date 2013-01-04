MRuby::Gem::Specification.new('mruby-v8') do |spec|
  spec.license = 'MIT'
  spec.authors = 'mattn'
 
  # spec.cflags = ''
 
  # spec.mruby_cflags = ''
  # spec.mruby_ldflags = ''
  spec.mruby_libs = '-lv8_base -lv8_nosnapshot -lstdc++'
 
  spec.rbfiles = Dir.glob("#{dir}/mrblib/*.rb")
  spec.objs = Dir.glob("#{dir}/src/*.{c,cpp,m,asm,S}").map{|f| f.ext('o')}

  spec.test_rbfiles = Dir.glob("#{dir}/test/*.rb")
  spec.test_objs = Dir.glob("#{dir}/test/*.{c,cpp,cc,m,asm,S}").map{|f| f.ext('o')}

  # spec.generated_files = "#{dir}/generated.c"
end
