slangc.exe -force-glsl-scalar-layout test.slang -profile glsl_450 -target spirv -o test.spv -entry main
spirv-cross -V --version 450--no-es test.spv --output glsl
