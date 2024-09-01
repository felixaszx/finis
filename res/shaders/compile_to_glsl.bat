slangc.exe -force-glsl-scalar-layout test.slang -target spirv -O2 -o test2.spv
spirv-cross -V test2.spv
