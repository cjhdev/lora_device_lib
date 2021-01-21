require File.expand_path("../wrappers/ruby/lib/ldl/version.rb", __FILE__)
require 'time'

Gem::Specification.new do |s|

  s.name    = "ldl"
  s.version = LDL::VERSION
  s.date = Date.today.to_s
  s.summary = "A ruby wrapper for ldl"
  s.author  = "Cameron Harper"
  s.email = "contact@cjh.id.au"

  s.files = Dir.glob("src/**/*.c")
  s.files += Dir.glob("src/**/*.h")
  s.files += Dir.glob("wrappers/ruby/ext/**/*.{c,h,rb}")
  s.files += Dir.glob("wrappers/ruby/lib/**/*.rb")

  s.extensions = ["wrappers/ruby/ext/ldl/ext_ldl/extconf.rb"]

  s.license = 'MIT'

  s.add_development_dependency 'rake-compiler'
  s.add_development_dependency 'rake'
  s.add_development_dependency 'minitest'
  s.add_runtime_dependency 'pry'

  s.required_ruby_version = '>= 2.0'

  s.require_paths += ['wrappers/ruby/lib']

end
