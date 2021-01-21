require 'mkmf'

# get from this file back to project root
project_root = File.join("..","..","..","..")

$srcs = Dir[File.join(__dir__, "*.c")]
$srcs += Dir[File.join(project_root, "src", "*.c")]

$VPATH << File.join(project_root, "src")

$INCFLAGS << " -I#{File.join(project_root, "include")}"

$defs << " -DLDL_TARGET_INCLUDE=\\\"platform.h\\\""

CONFIG["debugflags"] = "-ggdb3"
CONFIG["optflags"] = "-O0"

create_makefile('ldl/ext_ldl')
