slangc.exe -force-glsl-scalar-layout test2.slang -target spirv -O0 -o test2.spv
spirv-cross -V test2.spv