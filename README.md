# `finis`

A renderer/game engine for personal study and testing of new graphics technology.
Use MSYS2 and CMake for building.

## `finis_graphic`
The `finis_graphic` sublibrary is a vulkan wrapper completely independent from `Finis`. It provides interfaces for `glTF2.0` files and a very flexible vertex/index/material interface for shaders.

### Overview of `finis_graphic`
- Programmable vertex pulling
    - All vertex attributes supported by `glTF`
    - Flexibale vertex format (In 21st centry, we finally get rid of vertex buffer)
- GPU Driven
    - Using `vk::CommandBuffer::DrawIndexedIndirect`
    - Compute shader ready
    - Only 1 CPU drawcall for the whole scene, the size is only limited by device memory
    - Bindeless:
        - Completely remove vertex and index buffer
        - Bindeless material (should be used with ubershaders)
        - Bindeless Textures indexed by material in the shader directly
- Animation
    - CPU base animations
    - Bindless Morph Targets (Shapekeys) and Skeletons
- Multi-threaded resources loading
- more...

## Build instructions
1. Install [`MSYS2`](https://www.msys2.org/) on Windows and follow the instructions on MSYS2 website to install basic toolchains
2. Install the following Packages for your MSYS2 enviroment:
    - [mingw-w64-vulkan-devel](https://packages.msys2.org/basegroups/mingw-w64-vulkan-devel)
    - [mingw-w64-stb](https://packages.msys2.org/base/mingw-w64-stb)
    - [mingw-w64-simdjson](https://packages.msys2.org/base/mingw-w64-simdjson)
    - [mingw-w64-glfw](https://packages.msys2.org/base/mingw-w64-glfw)
    - [mingw-w64-spirv-cross](https://packages.msys2.org/base/mingw-w64-spirv-cross)
3. [`fastgltf`](https://github.com/spnda/fastgltf), [`slang`](https://github.com/shader-slang/slang) and [`task-thread-pool'](https://github.com/alugowski/task-thread-pool) are included already
4. Install CMake and Ninja in your MSYS2 enviroment
5. Good to go (should be)

## Third party libraries:
- `stb_image` Public Domain
- `fastgltf` MIT
- `GLFW` Zlib
- `task-thread-pool` MIT or other 2
- `simdjson` Apache-2.0
- `spirv-cross` Apache-2.0
- `slang` MIT
