MRuby::Gem::Specification.new('mruby-v8') do |spec|
  spec.license = 'MIT'
  spec.authors = 'mattn'
 
  if ENV['OS'] == 'Windows_NT'
    spec.linker.libraries << ['v8', 'stdc++', 'winmm', 'ws2_32'].reverse
  else
    spec.linker.libraries << ['v8_base', 'v8_nosnapshot', 'stdc++'].reverse
  end
end
