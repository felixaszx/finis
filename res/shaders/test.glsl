#version 450
#extension GL_EXT_scalar_block_layout : require
layout(row_major) uniform;
layout(row_major) buffer;

#line 23 0
layout(binding = 1)
uniform sampler2D  textures_0[10];


#line 3498 1
layout(location = 0)
out vec4 entryPointParam_main_0;


#line 28 0
void main()
{

#line 28
    entryPointParam_main_0 = (texture((textures_0[0]), (vec2(0.0, 0.0))));

#line 28
    return;
}

