# `Finis`

A renderer/game engine for personal study and testing of new graphics technology.

## `finis_graphic`
The `finis_graphic` sublibrary is a vulkan wrapper completely independent from `Finis`. It provide interfaces for `glTF2.0` files and a very flexible vertex/index/material interface for shaders.

### Overview of `finis_graphic`
- Programmable vertex pulling
    - All vertex attributes supported by `glTF`
    - Flexibale vertex format (forget about vertex format)
- GPU Driven
    - Using `vk::CommandBuffer::DrawIndexedIndirect`
    - Compute shader ready
    - Only 1 CPU drawcall for the whole scene, the size is only limited by device memory
    - Bindeless:
        - Completely remove vertex and index buffer
        - Bindeless material (should be used with ubershaders)
        - Textures indexed by material in the shader directly
- Animation
    - CPU base animations
    - Bindless Morph Targets (Shapekeys) and Skeletons
- Multi-threaded resources loading
- more...

## TBD

## Third party libraries:
- `stb_image`
- `fastgltf`
- `GLFW`
- `BS::thread_pool`
- `fltk`
- `simdjson`
- `shaderc`